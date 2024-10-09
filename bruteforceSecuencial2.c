#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <gcrypt.h>
#include <time.h>  // Librería para medir el tiempo

void decrypt(long key, char *ciph, int len) {
    gcry_cipher_hd_t hd;
    long k = 0;

    // Ajustar la clave de 56 bits en un arreglo de 64 bits con paridad
    for (int i = 0; i < 8; ++i) {
        key <<= 1;
        k += (key & (0xFE << i * 8));  // Ajustar la paridad de cada byte
    }

    // Inicializar el manejador de cifrado (DES ECB)
    gcry_cipher_open(&hd, GCRY_CIPHER_DES, GCRY_CIPHER_MODE_ECB, 0);

    // Establecer la clave con paridad ajustada
    gcry_cipher_setkey(hd, &k, 8);

    // Desencriptar el bloque
    gcry_cipher_decrypt(hd, (unsigned char *)ciph, len, NULL, 0);

    // Liberar recursos
    gcry_cipher_close(hd);
}

void encrypt(long key, char *ciph, int len) {
    gcry_cipher_hd_t hd;
    long k = 0;

    // Ajustar la clave de 56 bits en un arreglo de 64 bits con paridad
    for (int i = 0; i < 8; ++i) {
        key <<= 1;
        k += (key & (0xFE << i * 8));  // Ajustar la paridad de cada byte
    }

    // Inicializar el manejador de cifrado (DES ECB)
    gcry_cipher_open(&hd, GCRY_CIPHER_DES, GCRY_CIPHER_MODE_ECB, 0);

    // Establecer la clave con paridad ajustada
    gcry_cipher_setkey(hd, &k, 8);

    // Encriptar el bloque
    gcry_cipher_encrypt(hd, (unsigned char *)ciph, len, NULL, 0);

    // Liberar recursos
    gcry_cipher_close(hd);
}

int tryKey(long key, char *ciph, int len, const char *search) {
    char temp[len + 1];
    memcpy(temp, ciph, len);
    temp[len] = '\0';
    decrypt(key, temp, len);

    // Mostrar intentos fallidos
    if (strstr(temp, search) == NULL) {
        //printf("Clave incorrecta: %ld\n", key);
        return 0;
    }
    return 1;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Uso: %s <clave>\n", argv[0]);
        return 1;
    }

    // Obtener la clave desde los argumentos
    long input_key = atol(argv[1]);  // Convertir el argumento a un número long

    char input[512];  // Para leer el texto del archivo
    const char *search_phrase = "es una prueba de";  // Frase a buscar

    // Leer el texto a cifrar desde el archivo
    FILE *file = fopen("prueba.txt", "r");
    if (file == NULL) {
        printf("Error al abrir el archivo.\n");
        return 1;
    }

    // Leer el contenido del archivo
    fgets(input, sizeof(input), file);
    fclose(file);

    int input_len = strlen(input);

    // Verificar si el texto es múltiplo de 8 bytes (bloques de DES)
    int padded_len = input_len;
    if (padded_len % 8 != 0) {
        padded_len += 8 - (padded_len % 8);  // Rellenar hasta el siguiente múltiplo de 8
    }
    char padded_input[padded_len];
    memset(padded_input, 0, padded_len);
    memcpy(padded_input, input, input_len);

    // Cifrar el texto leído del archivo
    encrypt(input_key, padded_input, padded_len);

    // Comenzar la búsqueda de la clave correcta
    printf("\nIniciando búsqueda de la clave correcta...\n");

    // Medir el tiempo de ejecución
    clock_t start_time = clock();  // Empezar a medir el tiempo

    // Probar claves desde 0 hasta encontrar la correcta (2^56 claves posibles)
    long upper = (1L << 56);  // Límite superior para claves de DES (2^56)
    long found = 0;
    for (long test_key = 0; test_key < upper; ++test_key) {
        if (tryKey(test_key, (char *)padded_input, padded_len, search_phrase)) {
            found = test_key;
            printf("¡Clave encontrada: %ld!\n", found);
            break;
        }
    }

    // Medir el tiempo de ejecución después de encontrar la clave
    clock_t end_time = clock();  // Detener la medición del tiempo
    double time_taken = (double)(end_time - start_time) / CLOCKS_PER_SEC;  // Calcular tiempo en segundos

    // Si se encuentra la clave correcta
    if (found != 0) {
        decrypt(found, (char *)padded_input, padded_len);
        printf("Texto cifrado (en bytes):\n");
        for (int i = 0; i < padded_len; i++) {
            printf("%02x ", (unsigned char)padded_input[i]);
        }
        printf("\n");
        printf("Texto descifrado correctamente con la clave: %ld\n", found);
        printf("Texto descifrado: %s\n", padded_input);
    } else {
        printf("No se encontró la clave en el rango probado.\n");
    }

    // Mostrar el tiempo de ejecución
    printf("Tiempo de ejecución (seg): %.2f\n", time_taken);

    return 0;
}
