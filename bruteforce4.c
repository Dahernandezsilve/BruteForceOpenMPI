#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <unistd.h>
#include <gcrypt.h>
#include <time.h>

#define DES_KEY_LENGTH 8
#define BLOCK_SIZE 8

// Función para ajustar la clave a 8 bytes (64 bits) para DES
void adjust_key(unsigned long key_value, unsigned char *des_key) {
    for (int i = 0; i < DES_KEY_LENGTH; i++) {
        des_key[DES_KEY_LENGTH - i - 1] = (unsigned char)(key_value & 0xFF);
        key_value >>= 8;
    }
}

// Función para aplicar padding PKCS#7
int apply_padding(char *data, int len) {
    int pad_size = BLOCK_SIZE - (len % BLOCK_SIZE);
    for (int i = len; i < len + pad_size; i++) {
        data[i] = pad_size;
    }
    return len + pad_size;
}

// Función para remover el padding PKCS#7
int remove_padding(char *data, int len) {
    int pad_size = data[len - 1];
    if (pad_size > BLOCK_SIZE || pad_size <= 0) {
        printf("Error: Padding inválido\n");
        return len;  // Si el padding es inválido, regresamos la longitud original
    }
    return len - pad_size;
}

// Función para descifrar
void decrypt(unsigned char *key, char *ciph, int *len) {
    gcry_cipher_hd_t hd;
    
    gcry_error_t err = gcry_cipher_open(&hd, GCRY_CIPHER_DES, GCRY_CIPHER_MODE_ECB, 0);
    if (err) {
        printf("Error al abrir el manejador de cifrado: %s\n", gcry_strerror(err));
        return;
    }

    err = gcry_cipher_setkey(hd, key, DES_KEY_LENGTH);
    if (err) {
        printf("Error al establecer la clave de descifrado: %s\n", gcry_strerror(err));
        gcry_cipher_close(hd);
        return;
    }

    err = gcry_cipher_decrypt(hd, (unsigned char *)ciph, *len, NULL, 0);
    if (err) {
        printf("Error al descifrar: %s\n", gcry_strerror(err));
        gcry_cipher_close(hd);
        return;
    }

    // Verificamos si el padding es válido
    int new_len = remove_padding(ciph, *len);
    if (new_len < *len) {
        *len = new_len;
    }
    gcry_cipher_close(hd);
}

// Función para cifrar
void encrypt(unsigned char *key, char *ciph, int *len) {
    gcry_cipher_hd_t hd;

    gcry_error_t err = gcry_cipher_open(&hd, GCRY_CIPHER_DES, GCRY_CIPHER_MODE_ECB, 0);
    if (err) {
        printf("Error al abrir el manejador de cifrado: %s\n", gcry_strerror(err));
        return;
    }

    err = gcry_cipher_setkey(hd, key, DES_KEY_LENGTH);
    if (err) {
        printf("Error al establecer la clave de cifrado: %s\n", gcry_strerror(err));
        gcry_cipher_close(hd);
        return;
    }

    // Aplicar el padding antes del cifrado
    *len = apply_padding(ciph, *len);

    err = gcry_cipher_encrypt(hd, (unsigned char *)ciph, *len, NULL, 0);
    if (err) {
        printf("Error al cifrar: %s\n", gcry_strerror(err));
    }

    gcry_cipher_close(hd);
}

// Función que intenta descifrar con una clave y verifica si contiene la palabra buscada
int tryKey(unsigned long key_value, char *ciph, int len, const char *search) {
    unsigned char test_key[DES_KEY_LENGTH];
    adjust_key(key_value, test_key);

    char temp[len + 1];
    memcpy(temp, ciph, len);
    temp[len] = 0;

    decrypt(test_key, temp, &len);

    if (strstr(temp, search) == NULL) {
        return 0;
    }
    return 1;
}

// Leer el texto desde un archivo
char* read_text_from_file(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Error al abrir el archivo");
        exit(1);
    }

    fseek(file, 0, SEEK_END);
    long fsize = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *text = malloc(fsize + BLOCK_SIZE);  // Asignar memoria adicional para el padding
    fread(text, 1, fsize, file);
    fclose(file);

    text[fsize] = '\0';
    return text;
}

int main(int argc, char *argv[]) {
    int N, id;
    long upper = (1L << 56);  // upper bound DES keys 2^56
    long mylower, myupper;
    MPI_Status st;
    MPI_Request req;
    int flag;
    MPI_Comm comm = MPI_COMM_WORLD;

    if (argc != 3) {
        printf("Uso: %s <archivo.txt> <clave>\n", argv[0]);
        return 1;
    }

    MPI_Init(&argc, &argv);
    MPI_Comm_size(comm, &N);
    MPI_Comm_rank(comm, &id);

    gcry_check_version(NULL);  // Inicializa libgcrypt

    // Leer la clave y el archivo de texto a cifrar
    char *input_key = argv[2];
    char *input_text = read_text_from_file(argv[1]);
    int len = strlen(input_text);

    // Ajustar la clave a 8 bytes
    unsigned char des_key[DES_KEY_LENGTH];
    adjust_key(strtoul(input_key, NULL, 10), des_key);

    // Cifrar el texto con la clave
    unsigned char cipher[len + BLOCK_SIZE];
    memcpy(cipher, input_text, len);
    encrypt(des_key, (char *)cipher, &len);

    // Distribuir rangos de llaves para los procesos
    long range_per_node = upper / N;
    mylower = range_per_node * id;
    myupper = range_per_node * (id + 1) - 1;
    if (id == N - 1) {
        myupper = upper;
    }

    long found = 0;
    char search[] = "es una prueba de";

    clock_t start = clock();

    // Recepción no bloqueante de la clave encontrada
    MPI_Irecv(&found, 1, MPI_LONG, MPI_ANY_SOURCE, MPI_ANY_TAG, comm, &req);

    for (long i = mylower; i < myupper && (found == 0); ++i) {
        if (tryKey(i, (char *)cipher, len, search)) {
            found = i;
            printf("¡Clave encontrada por el nodo %d: %li!\n", id, found);

            // Enviar la clave encontrada a todos los nodos
            for (int node = 0; node < N; node++) {
                MPI_Send(&found, 1, MPI_LONG, node, 0, MPI_COMM_WORLD);
            }
            break;
        }
    }

    // El nodo 0 espera la clave correcta
    if (id == 0) {
        MPI_Wait(&req, &st);
        decrypt(des_key, (char *)cipher, &len);

        clock_t end = clock();
        double time_spent = (double)(end - start) / CLOCKS_PER_SEC;

        printf("Clave correcta: %li\n", found);
        printf("Texto descifrado: %s\n", cipher);
        printf("Tiempo total de ejecución: %f segundos\n", time_spent);
    }

    MPI_Finalize();
    free(input_text);
    return 0;
}
