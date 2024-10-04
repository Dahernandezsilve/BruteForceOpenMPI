#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <gcrypt.h>

#define MAX_TEXT_LENGTH 1000
#define PROGRESS_INTERVAL 1000000 // Print progress every 1 million attempts

void encrypt(const unsigned char *key, const unsigned char *plaintext, unsigned char *ciphertext, int text_length) {
    gcry_error_t err;
    gcry_cipher_hd_t handle;

    // Initialize the cipher
    err = gcry_cipher_open(&handle, GCRY_CIPHER_DES, GCRY_CIPHER_MODE_ECB, 0);
    if (err) {
        fprintf(stderr, "gcry_cipher_open: %s\n", gcry_strerror(err));
        exit(EXIT_FAILURE);
    }

    // Set the key
    err = gcry_cipher_setkey(handle, key, 8);
    if (err) {
        fprintf(stderr, "gcry_cipher_setkey: %s\n", gcry_strerror(err));
        exit(EXIT_FAILURE);
    }

    // Encrypt the plaintext
    for (int i = 0; i < text_length; i += 8) {
        err = gcry_cipher_encrypt(handle, ciphertext + i, 8, plaintext + i, 8);
        if (err) {
            fprintf(stderr, "gcry_cipher_encrypt: %s\n", gcry_strerror(err));
            exit(EXIT_FAILURE);
        }
    }

    gcry_cipher_close(handle);
}

void decrypt(const unsigned char *key, const unsigned char *ciphertext, unsigned char *decrypted, int text_length) {
    gcry_error_t err;
    gcry_cipher_hd_t handle;

    // Initialize the cipher
    err = gcry_cipher_open(&handle, GCRY_CIPHER_DES, GCRY_CIPHER_MODE_ECB, 0);
    if (err) {
        fprintf(stderr, "gcry_cipher_open: %s\n", gcry_strerror(err));
        exit(EXIT_FAILURE);
    }

    // Set the key
    err = gcry_cipher_setkey(handle, key, 8);
    if (err) {
        fprintf(stderr, "gcry_cipher_setkey: %s\n", gcry_strerror(err));
        exit(EXIT_FAILURE);
    }

    // Decrypt the ciphertext
    for (int i = 0; i < text_length; i += 8) {
        err = gcry_cipher_decrypt(handle, decrypted + i, 8, ciphertext + i, 8);
        if (err) {
            fprintf(stderr, "gcry_cipher_decrypt: %s\n", gcry_strerror(err));
            exit(EXIT_FAILURE);
        }
    }

    gcry_cipher_close(handle);
}

int try_key(const unsigned char *key, const unsigned char *ciphertext, const unsigned char *original_text, int text_length) {
    unsigned char decrypted[MAX_TEXT_LENGTH];
    decrypt(key, ciphertext, decrypted, text_length);
    return memcmp(decrypted, original_text, text_length) == 0;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <key>\n", argv[0]);
        return 1;
    }

    unsigned char key[8];
    // Convert the input key to bytes
    if (strlen(argv[1]) != 8) {
        printf("Key must be exactly 8 characters long.\n");
        return 1;
    }
    memcpy(key, argv[1], 8); // Copy the user input key to the key variable

    FILE *file = fopen("prueba.txt", "r");
    if (file == NULL) {
        printf("Error opening file\n");
        return 1;
    }

    unsigned char plaintext[MAX_TEXT_LENGTH];
    size_t text_length = fread(plaintext, 1, MAX_TEXT_LENGTH, file);
    fclose(file);

    // Pad the text to a multiple of 8 bytes
    while (text_length % 8 != 0) {
        plaintext[text_length++] = '\0';
    }

    unsigned char ciphertext[MAX_TEXT_LENGTH];

    printf("Original text: %.*s\n", (int)text_length, plaintext);
    printf("Provided key: %s\n", argv[1]);

    encrypt(key, plaintext, ciphertext, text_length);

    printf("Encrypted text: ");
    for (int i = 0; i < text_length; i++) {
        printf("%02x", ciphertext[i]);
    }
    printf("\n");

    clock_t start = clock();

    // Brute force attempt
    unsigned long long attempts = 0;
    int found = 0;
    unsigned char found_key[8];

    for (unsigned long long i = 0; i <= 0xFFFFFFFFFFFFFF; i++) {  // 56-bit key space
        memcpy(found_key, &i, sizeof(unsigned long long));

        attempts++;
        if (try_key(found_key, ciphertext, plaintext, text_length)) {
            found = 1;
            break;
        }

        // Print progress
        if (attempts % PROGRESS_INTERVAL == 0) {
            printf("Attempts: %llu\n", attempts);
        }
    }

    clock_t end = clock();
    double cpu_time_used = ((double)(end - start)) / CLOCKS_PER_SEC;

    if (found) {
        printf("Key found: %llu\n", *(unsigned long long*)found_key);
    } else {
        printf("Key not found\n");
    }

    printf("Time taken: %f seconds\n", cpu_time_used);
    printf("Attempts: %llu\n", attempts);

    return 0;
}
