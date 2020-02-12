#include <stdio.h>
#include <string.h>
#include <mpi.h>

#define MASTER 0

int main(int argc, char *argv[])
{
  int size;
  int my_rank;
  char *predecessor_message;
  char *message = "I AM YOUR PREDECESSOR";

  int i = 100;

  MPI_Status status;

  MPI_Init(&argc, &argv);
  MPI_Comm_size(MPI_COMM_WORLD, &size);
  MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);

  MPI_Ssend(&i, 1, MPI_INT, (my_rank + 1) % size, 99, MPI_COMM_WORLD);
  MPI_Recv(&i, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

  printf("I am %d and received %d from %d\n", my_rank, i, status.MPI_SOURCE);
  MPI_Finalize();
  return 0;
}
