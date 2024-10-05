#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <gcrypt.h>

void decrypt(long key, char *ciph, int len){
    gcry_cipher_hd_t hd;
    long k = 0;

    // Ajustar la clave de 56 bits en un arreglo de 64 bits con paridad
    for(int i = 0; i < 8; ++i){
        key <<= 1;
        k += (key & (0xFE << i*8));  // Ajustar la paridad de cada byte
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

void encrypt(long key, char *ciph, int len){
    gcry_cipher_hd_t hd;
    long k = 0;

    // Ajustar la clave de 56 bits en un arreglo de 64 bits con paridad
    for(int i = 0; i < 8; ++i){
        key <<= 1;
        k += (key & (0xFE << i*8));  // Ajustar la paridad de cada byte
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

int tryKey(long key, char *ciph, int len, const char *search){
    char temp[len + 1];
    memcpy(temp, ciph, len);
    temp[len] = '\0';
    decrypt(key, temp, len);

    // Mostrar intentos fallidos
    if (strstr(temp, search) == NULL) {
        printf("Clave incorrecta: %ld\n", key);
        return 0;
    }
    return 1;
}

int main(){
    char input[256];
    long input_key;

    // Pedir al usuario el texto a cifrar
    printf("Introduce el texto a cifrar (máx 255 caracteres): ");
    fgets(input, 256, stdin);
    int input_len = strlen(input);

    // Eliminar el salto de línea final si está presente
    if (input[input_len - 1] == '\n') {
        input[input_len - 1] = '\0';
        input_len--;
    }

    // Verificar si el texto es múltiplo de 8 bytes (bloques de DES)
    int padded_len = input_len;
    if (padded_len % 8 != 0) {
        padded_len += 8 - (padded_len % 8);  // Rellenar hasta el siguiente múltiplo de 8
    }
    char padded_input[padded_len];
    memset(padded_input, 0, padded_len);
    memcpy(padded_input, input, input_len);

    // Pedir al usuario la clave (long)
    printf("Introduce la clave (long, un número): ");
    scanf("%ld", &input_key);

    // Cifrar el texto ingresado
    encrypt(input_key, padded_input, padded_len);
    printf("Texto cifrado (en bytes):\n");
    for (int i = 0; i < padded_len; i++) {
        printf("%02x ", (unsigned char)padded_input[i]);
    }
    printf("\n");

    // Comenzar la búsqueda de la clave correcta
    printf("\nIniciando búsqueda de la clave correcta...\n");

    // Probar claves desde 0 hasta encontrar la correcta (2^56 claves posibles)
    long upper = (1L << 56);  // Límite superior para claves de DES (2^56)
    long found = 0;
    for (long test_key = 0; test_key < upper; ++test_key) {
        if (tryKey(test_key, (char *)padded_input, padded_len, input)) {
            found = test_key;
            printf("¡Clave encontrada: %ld!\n", found);
            break;
        }
    }

    // Si se encuentra la clave correcta
    if (found != 0) {
        decrypt(found, (char *)padded_input, padded_len);
        printf("Texto descifrado correctamente con la clave: %ld\n", found);
        printf("Texto descifrado: %s\n", padded_input);
    } else {
        printf("No se encontró la clave en el rango probado.\n");
    }

    return 0;
}
