#include <stdio.h>
#include <stdlib.h>
#include <gcrypt.h>
#include <time.h>
#include <string.h>

// Funciones para generar claves y desencriptar (como en tu código original)

// Genera la clave en función de un número dado
void generate_key(unsigned long i, unsigned char *key, size_t key_len) {
    for (size_t j = 0; j < key_len; j++) {
        key[j] = (i >> (8 * j)) & 0xFF;  // Extrae los bytes del número
    }
}

// Desencripta usando DES
void decrypt_des(unsigned char *ciphertext, unsigned char *key, unsigned char *decrypted_text) {
    gcry_cipher_hd_t handle;
    gcry_error_t error;

    // Inicializa el contexto de descifrado con DES en modo ECB
    error = gcry_cipher_open(&handle, GCRY_CIPHER_DES, GCRY_CIPHER_MODE_ECB, 0);
    if (error) {
        fprintf(stderr, "Error al inicializar el contexto de descifrado: %s\n", gcry_strerror(error));
        return;
    }

    // Asegúrate de que la llave tiene exactamente 8 bytes
    unsigned char full_key[8] = {0};
    memcpy(full_key, key, 7);  // Copia los primeros 7 bytes y deja el último byte en 0

    // Establece la clave de descifrado
    error = gcry_cipher_setkey(handle, full_key, 8);  // DES espera una clave de 8 bytes
    if (error) {
        fprintf(stderr, "Error al establecer la clave de descifrado: %s\n", gcry_strerror(error));
        gcry_cipher_close(handle);
        return;
    }

    // Descifra el texto
    error = gcry_cipher_decrypt(handle, decrypted_text, 8, ciphertext, 8);
    if (error) {
        fprintf(stderr, "Error al descifrar: %s\n", gcry_strerror(error));
    }

    // Cierra el contexto de descifrado
    gcry_cipher_close(handle);
}

// Realiza la búsqueda de fuerza bruta
void brute_force_des(unsigned char *ciphertext, unsigned char *known_plaintext, size_t key_len) {
    clock_t start = clock();

    unsigned char key[key_len];
    unsigned char decrypted_text[8];
    int found = 0;  // Variable para indicar si se encontró la clave

    for (unsigned long i = 0; i < (1UL << (key_len * 8)); i++) {
        // Genera la siguiente posible llave
        generate_key(i, key, key_len);

        // Intenta descifrar el texto
        decrypt_des(ciphertext, key, decrypted_text);

        // Verifica si el descifrado es correcto
        if (memcmp(decrypted_text, known_plaintext, 8) == 0) {
            printf("Llave encontrada: %lu\n", i);
            found = 1;  // Se encontró la clave
            break;      // Salir del bucle si se encontró la clave
        }
    }

    clock_t end = clock();
    double time_taken = (double)(end - start) / CLOCKS_PER_SEC;

    printf("Tiempo tomado: %.5f segundos\n", time_taken);
    if (!found) {
        printf("No se encontró ninguna clave válida para longitud de %zu bytes.\n", key_len);
    }
}

int main() {
    // Texto cifrado y texto plano conocidos
    unsigned char ciphertext[8] = {0x85, 0xE8, 0x13, 0x54, 0xAB, 0xE2, 0x38, 0x32};  // Texto cifrado de ejemplo
    unsigned char known_plaintext[8] = "ABCDEFGH";  // Texto plano conocido

    // Probar distintas longitudes de clave
    for (size_t key_len = 1; key_len <= 7; key_len++) {
        printf("Probando claves de longitud: %zu bytes\n", key_len);
        brute_force_des(ciphertext, known_plaintext, key_len);
    }

    return 0;
}
