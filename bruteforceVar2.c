#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <gcrypt.h>
#include <stdbool.h>
#include <mpi.h>

#define BATCH_SIZE 1000000  // Tamaño del lote de claves
#define KEY_SIZE 32         // Tamaño de la clave para ChaCha20 (256 bits)
#define NONCE_SIZE 12       // Tamaño del nonce para ChaCha20 (96 bits)

void decrypt(unsigned char *key, unsigned char *nonce, char *ciph, int len) {
    gcry_cipher_hd_t hd;
    
    // Inicializar el manejador de cifrado (ChaCha20)
    gcry_cipher_open(&hd, GCRY_CIPHER_CHACHA20, GCRY_CIPHER_MODE_STREAM, 0);
    
    // Establecer la clave
    gcry_cipher_setkey(hd, key, KEY_SIZE);
    
    // Establecer el nonce
    gcry_cipher_setiv(hd, nonce, NONCE_SIZE);
    
    // Desencriptar el texto
    gcry_cipher_decrypt(hd, (unsigned char *)ciph, len, NULL, 0);
    
    // Liberar recursos
    gcry_cipher_close(hd);
}

void encrypt(unsigned char *key, unsigned char *nonce, char *ciph, int len) {
    gcry_cipher_hd_t hd;

    // Inicializar el manejador de cifrado (ChaCha20)
    gcry_cipher_open(&hd, GCRY_CIPHER_CHACHA20, GCRY_CIPHER_MODE_STREAM, 0);
    
    // Establecer la clave
    gcry_cipher_setkey(hd, key, KEY_SIZE);
    
    // Establecer el nonce
    gcry_cipher_setiv(hd, nonce, NONCE_SIZE);
    
    // Encriptar el texto
    gcry_cipher_encrypt(hd, (unsigned char *)ciph, len, NULL, 0);
    
    // Liberar recursos
    gcry_cipher_close(hd);
}

int tryKey(unsigned char *key, unsigned char *nonce, char *ciph, int len, const char *search) {
    char temp[len + 1];
    memcpy(temp, ciph, len);
    temp[len] = '\0';
    decrypt(key, nonce, temp, len);

    if (strstr(temp, search) == NULL) {
        return 0;
    }
    return 1;
}

int main(int argc, char *argv[]) {
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (argc < 2) {
        if (rank == 0) {
            printf("Uso: %s <clave>\n", argv[0]);
        }
        MPI_Finalize();
        return 1;
    }

    unsigned char input_key[KEY_SIZE];  // Clave para encriptar el texto
    for (int i = 0; i < KEY_SIZE; ++i) {
        input_key[i] = (unsigned char)(atol(argv[1]) >> (8 * (KEY_SIZE - 1 - i)) & 0xFF);
    }

    unsigned char nonce[NONCE_SIZE] = {0};  // Nonce inicializado en cero
    char input[512];
    const char *search_phrase = "es una prueba de";  // Frase a buscar

    // Leer el texto a cifrar desde el archivo
    if (rank == 0) {
        FILE *file = fopen("prueba.txt", "r");
        if (file == NULL) {
            printf("Error al abrir el archivo.\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
        fgets(input, sizeof(input), file);
        fclose(file);
    }

    // Broadcast del texto a todos los nodos
    MPI_Bcast(input, sizeof(input), MPI_CHAR, 0, MPI_COMM_WORLD);

    int input_len = strlen(input);
    char padded_input[input_len + 1];
    memset(padded_input, 0, sizeof(padded_input));
    memcpy(padded_input, input, input_len);

    if (rank == 0) {
        encrypt(input_key, nonce, padded_input, input_len);
        printf("Texto encriptado (en bytes):\n");
        for (int i = 0; i < input_len; i++) {
            printf("%02x ", (unsigned char)padded_input[i]);
        }
        printf("\n");
    }

    MPI_Bcast(padded_input, input_len, MPI_CHAR, 0, MPI_COMM_WORLD);

    long upper = (1L << 56);  // Límite superior para claves (se puede ajustar según el algoritmo)
    long found = 0;
    bool stop_search = false;

    double start_time = MPI_Wtime();  // Comienza la medición de tiempo

    // Inicialización del rango de claves
    static long current_key = 0;
    while (!stop_search) {
        long mylower, myupper;

        if (rank == 0) {
            // El proceso maestro distribuye rangos de claves
            mylower = current_key;
            myupper = mylower + BATCH_SIZE;

            if (myupper > upper) {
                myupper = upper;
            }
            current_key = myupper;
        }

        // Distribuir el rango de claves a todos los procesos
        MPI_Bcast(&mylower, 1, MPI_LONG, 0, MPI_COMM_WORLD);
        MPI_Bcast(&myupper, 1, MPI_LONG, 0, MPI_COMM_WORLD);

        // Cada proceso intenta su lote de claves
        for (long test_key = mylower + rank; test_key < myupper && !stop_search; test_key += size) {
            unsigned char test_key_bytes[KEY_SIZE];
            for (int i = 0; i < KEY_SIZE; ++i) {
                test_key_bytes[i] = (unsigned char)(test_key >> (8 * (KEY_SIZE - 1 - i)) & 0xFF);
            }

            if (tryKey(test_key_bytes, nonce, padded_input, input_len, search_phrase)) {
                found = test_key;
                printf("¡Clave encontrada por el nodo %d: %ld!\n", rank, found);
                stop_search = true;
                break;
            }
        }

        // Verificar si se encontró la clave o si se llegó al límite
        MPI_Allreduce(MPI_IN_PLACE, &stop_search, 1, MPI_C_BOOL, MPI_LOR, MPI_COMM_WORLD);
        if (mylower >= upper) {
            stop_search = true;
        }

        // Asegurarse de que todos los procesos conozcan la clave encontrada
        MPI_Allreduce(MPI_IN_PLACE, &found, 1, MPI_LONG, MPI_MAX, MPI_COMM_WORLD);
    }

    // Esperar a que todos los procesos terminen
    MPI_Barrier(MPI_COMM_WORLD);

    // Solo el proceso 0 imprime los resultados
    if (rank == 0) {
        if (found != 0) {
            unsigned char found_key[KEY_SIZE];
            for (int i = 0; i < KEY_SIZE; ++i) {
                found_key[i] = (unsigned char)(found >> (8 * (KEY_SIZE - 1 - i)) & 0xFF);
            }
            decrypt(found_key, nonce, padded_input, input_len);
            printf("Clave correcta: %ld\n", found);
            printf("Texto descifrado correctamente: %s\n", padded_input);
        } else {
            printf("No se encontró la clave.\n");
        }

        double end_time = MPI_Wtime();
        double time_taken = end_time - start_time;
        printf("Tiempo de ejecución (seg): %.2f\n", time_taken);
    }

    MPI_Finalize();
    return 0;
}
