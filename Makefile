all:
	mpicc -o bruteforce bruteforce3.c -lgcrypt

run:
	mpirun ./bruteforce