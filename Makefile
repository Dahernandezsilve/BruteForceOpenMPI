all:
	gcc -o bruteforceSecuencial2 bruteforceSecuencial2.c -lgcrypt
	mpicc -o bruteforce3 bruteforce3.c -lgcrypt
	mpicc -o bruteforce4 bruteforce4.c -lgcrypt
	mpicc -o bruteforceVar1 bruteforceVar1.c -lgcrypt
	mpicc -o bruteforceVar2 bruteforceVar2.c -lgcrypt

run:
	./bruteforceSecuencial2 1234567
	mpirun -np 4 ./bruteforce3 1234567
	mpirun -np 4 ./bruteforce4 1234567
	mpirun -np 4 ./bruteforceVar1 1234567
	mpirun -np 4 ./bruteforceVar2 1234567