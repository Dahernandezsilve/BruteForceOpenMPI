#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <gcrypt.h>
#include <time.h>  // Librería para medir el tiempo
#include <mpi.h>

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

    long input_key = atol(argv[1]);  // Clave para encriptar el texto inicialmente
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
    int padded_len = input_len;

    if (padded_len % 8 != 0) {
        padded_len += 8 - (padded_len % 8);  // Rellenar hasta el siguiente múltiplo de 8
    }
    char padded_input[padded_len];
    memset(padded_input, 0, padded_len);
    memcpy(padded_input, input, input_len);

    if (rank == 0) {
        encrypt(input_key, padded_input, padded_len);
        printf("Texto encriptado (en bytes):\n");
        for (int i = 0; i < padded_len; i++) {
            printf("%02x ", (unsigned char)padded_input[i]);
        }
        printf("\n");
    }

    MPI_Bcast(padded_input, padded_len, MPI_CHAR, 0, MPI_COMM_WORLD);

    long upper = (1L << 56);  // Límite superior para claves de DES (2^56)
    long range_per_node = upper / size;
    long mylower = range_per_node * rank;
    long myupper = (rank == size - 1) ? upper : mylower + range_per_node;

    long found = 0;
    MPI_Request req;
    MPI_Irecv(&found, 1, MPI_LONG, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &req);

    clock_t start_time = clock();  // Empezar a medir el tiempo

    for (long test_key = mylower; test_key < myupper && found == 0; ++test_key) {
        printf("Nodo %d probando clave: %ld\n", rank, test_key);

        if (tryKey(test_key, padded_input, padded_len, search_phrase)) {
            found = test_key;
            printf("¡Clave encontrada por el nodo %d: %ld!\n", rank, found);

            for (int node = 0; node < size; node++) {
                MPI_Send(&found, 1, MPI_LONG, node, 0, MPI_COMM_WORLD);
            }
            break;
        }
    }

    // Asegurar que todos los nodos terminan correctamente al encontrar la clave
    int flag;
    while (found == 0) {
        MPI_Test(&req, &flag, MPI_STATUS_IGNORE);
        if (flag) break;
    }

    if (rank == 0) {
        MPI_Wait(&req, MPI_STATUS_IGNORE);

        if (found != 0) {
            decrypt(found, padded_input, padded_len);
            printf("Clave correcta: %ld\n", found);
            printf("Texto descifrado correctamente: %s\n", padded_input);
        } else {
            printf("No se encontró la clave en el rango probado.\n");
        }

        clock_t end_time = clock();
        double time_taken = (double)(end_time - start_time) / CLOCKS_PER_SEC;
        printf("Tiempo de ejecución: %f segundos\n", time_taken);
    }

    MPI_Finalize();
    return 0;
}
