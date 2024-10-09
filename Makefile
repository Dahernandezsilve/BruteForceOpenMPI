all:
	mpicc -o bruteforce4 bruteforce4.c -lgcrypt
	mpicc -o bruteforce3 bruteforce3.c -lgcrypt

run:
	mpirun -np 4 ./bruteforce3 1234567
	mpirun -np 4 ./bruteforce4 1234567
#	mpirun -np 4 ./bruteforce 1234567