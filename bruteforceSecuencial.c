#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <gcrypt.h>

void decrypt(unsigned char *key, char *ciph, int len){
    gcry_cipher_hd_t hd;

    // Inicializar el manejador de cifrado (DES ECB)
    gcry_cipher_open(&hd, GCRY_CIPHER_DES, GCRY_CIPHER_MODE_ECB, 0);
    
    // Establecer la clave
    gcry_cipher_setkey(hd, key, 8);  // DES requiere una clave de 8 bytes (64 bits)

    // Desencriptar el bloque
    gcry_cipher_decrypt(hd, (unsigned char *)ciph, len, NULL, 0);

    // Liberar recursos
    gcry_cipher_close(hd);
}

void encrypt(unsigned char *key, char *ciph, int len){
    gcry_cipher_hd_t hd;

    // Inicializar el manejador de cifrado (DES ECB)
    gcry_cipher_open(&hd, GCRY_CIPHER_DES, GCRY_CIPHER_MODE_ECB, 0);

    // Establecer la clave
    gcry_cipher_setkey(hd, key, 8);  // DES requiere una clave de 8 bytes (64 bits)

    // Encriptar el bloque
    gcry_cipher_encrypt(hd, (unsigned char *)ciph, len, NULL, 0);

    // Liberar recursos
    gcry_cipher_close(hd);
}

int tryKey(long key, unsigned char *ciph, int len, const char* original_text) {
    unsigned char key_bytes[8];
    char temp[len + 1];
    memcpy(temp, ciph, len);
    temp[len] = '\0';

    // Convertir el valor long en una clave de 8 bytes
    for (int i = 0; i < 8; i++) {
        key_bytes[i] = (key >> (i * 8)) & 0xFF;
    }

    // Desencriptar el texto usando la clave actual
    decrypt(key_bytes, temp, len);

    // Verificar si el texto desencriptado es el correcto
    if (strcmp(temp, original_text) == 0) {
        printf("¡Clave encontrada! %ld\n", key);
        return 1;  // Clave correcta encontrada
    } else {
        printf("Clave incorrecta: %ld\n", key);
        return 0;  // Continuar probando
    }
}

int main(){
    char input[256];
    unsigned char key[8] = {0};  // DES usa una clave de 8 bytes
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

    // Copiar la clave numérica en el arreglo de bytes (8 bytes)
    for (int i = 0; i < 8; i++) {
        key[i] = (input_key >> (i * 8)) & 0xFF;
    }

    // Cifrar el texto ingresado
    encrypt(key, padded_input, padded_len);
    printf("Texto cifrado (en bytes):\n");
    for (int i = 0; i < padded_len; i++) {
        printf("%02x ", (unsigned char)padded_input[i]);
    }
    printf("\n");

    // Comenzar la búsqueda de la clave correcta
    printf("\nIniciando búsqueda de la clave correcta...\n");

    // Probar claves desde 0 hasta encontrar la correcta
    long found = 0;
    for (long test_key = 0; test_key < 1000000000; ++test_key) {
        if (tryKey(test_key, (unsigned char *)padded_input, padded_len, input)) {
            found = test_key;
            break;
        }
    }

    // Si se encuentra la clave correcta
    if (found != 0) {
        printf("Texto desencriptado correctamente con la clave: %ld\n", found);
    } else {
        printf("No se encontró la clave en el rango probado.\n");
    }

    return 0;
}
