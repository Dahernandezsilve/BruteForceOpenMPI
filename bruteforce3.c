#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <unistd.h>
#include <gcrypt.h>

void decrypt(long key, char *ciph, int len){
    gcry_cipher_hd_t hd;
    long k = 0;
    for(int i = 0; i < 8; ++i){
        key <<= 1;
        k += (key & (0xFE << i*8));
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
    for(int i = 0; i < 8; ++i){
        key <<= 1;
        k += (key & (0xFE << i*8));
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

char search[] = " the ";

int tryKey(long key, char *ciph, int len){
    char temp[len+1];
    memcpy(temp, ciph, len);
    temp[len] = 0;
    decrypt(key, temp, len);

    // Mostrar intentos fallidos
    if (strstr((char *)temp, search) == NULL) {
        printf("Intento fallido con la clave: %li\n", key);
        return 0;
    }
    return 1;
}

unsigned char cipher[] = {108, 245, 65, 63, 125, 200, 150, 66, 17, 170, 207, 170, 34, 31, 70, 215, 0};

int main(int argc, char *argv[]){ 
    int N, id;
    long upper = (1L << 56); // upper bound DES keys 2^56
    long mylower, myupper;
    MPI_Status st;
    MPI_Request req;
    int flag;
    int ciphlen = strlen((char *)cipher);
    MPI_Comm comm = MPI_COMM_WORLD;

    MPI_Init(NULL, NULL);
    MPI_Comm_size(comm, &N);
    MPI_Comm_rank(comm, &id);

    int range_per_node = upper / N;
    mylower = range_per_node * id;
    myupper = range_per_node * (id+1) -1;
    if (id == N-1){
        // compensar residuo
        myupper = upper;
    }

    long found = 0;

    MPI_Irecv(&found, 1, MPI_LONG, MPI_ANY_SOURCE, MPI_ANY_TAG, comm, &req);

    for (long i = mylower; i < myupper && (found == 0); ++i) {
        if (tryKey(i, (char *)cipher, ciphlen)){
            found = i;

            // Mostrar mensaje cuando se encuentra la clave
            printf("¡Clave encontrada por el nodo %d: %li!\n", id, found);

            for (int node = 0; node < N; node++){
                MPI_Send(&found, 1, MPI_LONG, node, 0, MPI_COMM_WORLD);
            }
            break;
        }
    }

    if (id == 0){
        MPI_Wait(&req, &st);
        decrypt(found, (char *)cipher, ciphlen);

        // Mostrar la clave encontrada y el mensaje descifrado
        printf("Clave correcta: %li\n", found);
        printf("Texto descifrado: %s\n", cipher);
    }

    MPI_Finalize();
}
