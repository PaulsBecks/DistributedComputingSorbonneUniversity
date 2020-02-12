#include <stdio.h>
#include <string.h>
#include <mpi.h>

#define COMM MPI_COMM_WORLD
#define TAG 99

int local_event(int *clock)
{
  *clock += 1;
}

int main(int argc, char *argv[])
{
  int rank;
  int size;
  int clock = 1;
  int received_clock;
  MPI_Status status;

  MPI_Init(&argc, &argv);
  MPI_Comm_size(COMM, &size);
  MPI_Comm_rank(COMM, &rank);

  MPI_Send(&clock, 1, MPI_INT, (rank + 1) % size, TAG, COMM);
  MPI_Recv(&received_clock, 1, MPI_INT, MPI_ANY_SOURCE, TAG, COMM, &status);

  clock = received_clock + 1;

  printf("Rank: %d - Clock: %d\n", rank, clock);

  local_event(&clock);

  printf("Rank: %d - Clock: %d\n", rank, clock);

  MPI_Finalize();
  return 0;
}
