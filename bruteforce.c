#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sodium.h>
#include <mpi.h>

#define AES_KEY_SIZE 32 // Tamaño de la clave para AES-256
#define AES_BLOCK_SIZE 16

void encrypt(const unsigned char *key, const unsigned char *plaintext, unsigned char *ciphertext) {
    unsigned char nonce[crypto_aead_aes256gcm_NPUBBYTES];
    randombytes_buf(nonce, sizeof nonce);
    
    unsigned long long ciphertext_len;
    crypto_aead_aes256gcm_encrypt(ciphertext, &ciphertext_len, plaintext, strlen((const char *)plaintext), NULL, 0, NULL, nonce, key);
}

void decrypt(const unsigned char *key, const unsigned char *ciphertext, unsigned char *plaintext) {
    unsigned long long plaintext_len;
    unsigned char nonce[crypto_aead_aes256gcm_NPUBBYTES];

    randombytes_buf(nonce, sizeof nonce);

    if (crypto_aead_aes256gcm_decrypt(plaintext, &plaintext_len, NULL, ciphertext, strlen((const char *)ciphertext), NULL, 0, nonce, key) != 0) {
        // Manejar error de descifrado
        printf("Error al descifrar\n");
    }
    plaintext[plaintext_len] = '\0'; // Asegúrate de que el texto plano esté terminado en null
}

char search[] = " the ";
int tryKey(const unsigned char *key, const unsigned char *ciphertext, long keyIndex) {
    unsigned char plaintext[128]; // Tamaño del buffer de texto plano
    decrypt(key, ciphertext, plaintext);
    
    if (strstr((char *)plaintext, search) != NULL) {
        return 1; // Se encontró la palabra
    } else {
        printf("Prueba fallida para la clave: %li\n", keyIndex); // Información sobre la clave que se probó
        return 0; // No se encontró la palabra
    }
}

unsigned char cipher[] = {108, 245, 65, 63, 125, 200, 150, 66, 17, 170, 207, 170, 34, 31, 70, 215, 0};

int main(int argc, char *argv[]) {
    int N, id;
    long upper = (1L << 56); // límite superior de claves AES
    long mylower, myupper;
    MPI_Status st;
    MPI_Request req;
    unsigned char found_key[AES_KEY_SIZE];
    unsigned char key[AES_KEY_SIZE];

    // Inicializar libsodium
    if (sodium_init() < 0) {
        return 1; // no se puede inicializar libsodium
    }

    MPI_Comm comm = MPI_COMM_WORLD;
    MPI_Init(NULL, NULL);
    MPI_Comm_size(comm, &N);
    MPI_Comm_rank(comm, &id);

    // Inicializar la clave (deberías generar esto de manera segura)
    randombytes_buf(key, sizeof key);

    int range_per_node = upper / N;
    mylower = range_per_node * id;
    myupper = range_per_node * (id + 1) - 1;
    if (id == N - 1) {
        myupper = upper;
    }

    long found = 0;

    MPI_Irecv(&found, 1, MPI_LONG, MPI_ANY_SOURCE, MPI_ANY_TAG, comm, &req);

    for (long i = mylower; i < myupper && (found == 0); ++i) {
        memcpy(found_key, key, AES_KEY_SIZE); // Cualquier modificación a la clave podría hacerse aquí
        if (tryKey(found_key, (unsigned char *)cipher, i)) { // Pasar i como parámetro
            found = i;
            for (int node = 0; node < N; node++) {
                MPI_Send(&found, 1, MPI_LONG, node, 0, MPI_COMM_WORLD);
            }
            break;
        }
    }

    if (id == 0) {
        MPI_Wait(&req, &st);
        unsigned char plaintext[128]; // Asegúrate de que el buffer sea lo suficientemente grande
        decrypt(found_key, (unsigned char *)cipher, plaintext);
        printf("%li %s\n", found, plaintext);
    }

    MPI_Finalize();
    return 0;
}
