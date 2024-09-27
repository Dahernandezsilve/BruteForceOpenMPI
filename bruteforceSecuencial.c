#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <gcrypt.h>
#include <time.h>  // Para medir el tiempo de ejecución

#define GCRYPT_NO_MPI_MACROS
#define GCRYPT_NO_DEPRECATED

// Verificar errores en las llamadas a Libgcrypt
void check_gcrypt_error(gcry_error_t err, const char *msg) {
    if (err) {
        fprintf(stderr, "%s: %s\n", msg, gcry_strerror(err));
        exit(1);
    }
}

void decrypt(long key, char *ciph, int len) {
    gcry_cipher_hd_t hd;
    unsigned char key_bytes[8] = {0};  // Arreglo para almacenar la clave de 8 bytes
    gcry_error_t err;

    // Inicializar Libgcrypt
    if (!gcry_check_version(GCRYPT_VERSION)) {
        fprintf(stderr, "Error de inicialización de Libgcrypt\n");
        exit(2);
    }

    // Extraer los 8 bytes de la clave desde el 'long'
    for (int i = 7; i >= 0; --i) {
        key_bytes[i] = key & 0xFF;
        key >>= 8;
    }

    // Inicializar el manejador de cifrado (DES ECB) y permitir claves débiles
    err = gcry_cipher_open(&hd, GCRY_CIPHER_DES, GCRY_CIPHER_MODE_ECB, GCRY_CIPHER_SECURE);
    check_gcrypt_error(err, "Error al abrir el manejador de cifrado");

    // Permitir claves débiles
    err = gcry_cipher_ctl(hd, GCRYCTL_SET_ALLOW_WEAK_KEY, NULL, 0);  // Uso de la opción correcta
    check_gcrypt_error(err, "Error al habilitar claves débiles");

    // Establecer la clave con 8 bytes
    err = gcry_cipher_setkey(hd, key_bytes, 8);
    check_gcrypt_error(err, "Error al establecer la clave");

    // Desencriptar el bloque
    err = gcry_cipher_decrypt(hd, (unsigned char *)ciph, len, NULL, 0);
    check_gcrypt_error(err, "Error al desencriptar el texto");

    // Liberar recursos
    gcry_cipher_close(hd);
}

char search[] = " the ";

int tryKey(long key, char *ciph, int len) {
    char temp[len+1];
    memcpy(temp, ciph, len);
    temp[len] = 0;
    decrypt(key, temp, len);

    // Mostrar intentos fallidos
    if (strstr((char *)temp, search) == NULL) {
        return 0;
    }
    return 1;
}

unsigned char cipher[] = {108, 245, 65, 63, 125, 200, 150, 66, 17, 170, 207, 170, 34, 31, 70, 215, 0};

int main(int argc, char *argv[]) {
    long upper;
    int ciphlen = strlen((char *)cipher);
    long found = 0;

    // Medición de tiempo
    clock_t start, end;
    double time_taken;

    // Pedir longitud de la llave como argumento de entrada
    if (argc < 2) {
        printf("Uso: %s <longitud de llave en bits>\n", argv[0]);
        return 1;
    }

    int key_length_bits = atoi(argv[1]);

    if (key_length_bits <= 0 || key_length_bits > 56) {
        printf("Longitud de llave inválida. Debe estar entre 1 y 56 bits.\n");
        return 1;
    }

    // Establecer el límite superior en función de la longitud de la llave
    upper = (1L << key_length_bits);

    printf("Buscando clave con una longitud de %d bits (hasta %ld claves)...\n", key_length_bits, upper);

    // Iniciar la medición de tiempo
    start = clock();

    // Búsqueda secuencial de la clave
    for (long i = 0; i < upper; ++i) {
        if (tryKey(i, (char *)cipher, ciphlen)) {
            found = i;

            // Mostrar mensaje cuando se encuentra la clave
            printf("¡Clave encontrada: %li!\n", found);
            break;
        }
    }

    // Finalizar la medición de tiempo
    end = clock();

    // Calcular el tiempo tomado
    time_taken = ((double)(end - start)) / CLOCKS_PER_SEC;

    if (found != 0) {
        // Desencriptar y mostrar el mensaje
        decrypt(found, (char *)cipher, ciphlen);
        printf("Clave correcta: %li\n", found);
        printf("Texto descifrado: %s\n", cipher);
    } else {
        printf("No se encontró una clave válida.\n");
    }

    printf("Tiempo de búsqueda: %.2f segundos\n", time_taken);

    return 0;
}
