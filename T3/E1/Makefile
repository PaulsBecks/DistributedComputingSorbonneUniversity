
elect: elect.o timer-MPI.o
	mpicc -o elect elect.o timer-MPI.o

elect.o: elect.c timer-MPI.h msg.h
	mpicc -o elect.o -c elect.c

timer-MPI.o: timer-MPI.c timer-MPI.h msg.h
	mpicc -o timer-MPI.o -c timer-MPI.c 

run:
	mpirun -quiet  --oversubscribe -np 5 elect 2 0
	
clean:
	rm -f *.o elect

