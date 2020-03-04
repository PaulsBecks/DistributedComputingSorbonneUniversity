#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <mpi.h>

#define INITIATOR 0

#define UNDECIDED 0
#define LOOSER 1
#define DONE 2

#define ELEC 0
#define ANNOUNCE 1

int main(int argc, char *argv[])
{
  int size;
  int my_rank;
  srand(getpid());

  int status = UNDECIDED;
  char *predecessor_message;

  MPI_Status message_status;

  MPI_Init(&argc, &argv);
  MPI_Comm_size(MPI_COMM_WORLD, &size);
  MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);

  int isInitiator = rand() % 2;

  int leader = my_rank;

  if (INITIATOR != isInitiator)
  {
    printf("%d: I AM AN INITIATOR\n", my_rank);
    MPI_Send(&my_rank, 1, MPI_INT, (my_rank + 1) % size, ELEC, MPI_COMM_WORLD);
  }

  while (status != DONE)
  {
    int other_leader;
    MPI_Recv(&other_leader, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &message_status);
    if (message_status.MPI_TAG == ELEC)
    {
      printf("%d: I RECEIVED AN ELEC MESSAGE FROM %d WITH LEADER %d\n", my_rank, message_status.MPI_SOURCE, other_leader);
      if (other_leader == my_rank)
      {
        MPI_Send(&leader, 1, MPI_INT, (my_rank + 1) % size, ANNOUNCE, MPI_COMM_WORLD);
      }
      else
      {
        if (other_leader > leader)
        {
          leader = other_leader;
          status = LOOSER;
          MPI_Send(&leader, 1, MPI_INT, (my_rank + 1) % size, ELEC, MPI_COMM_WORLD);
          printf("%d: I AM A LOOSER NOW\n", my_rank);
        }
      }
    }
    else
    {
      status = DONE;
      if (my_rank != message_status.MPI_SOURCE)
      {
        MPI_Send(&leader, 1, MPI_INT, (my_rank + 1) % size, ANNOUNCE, MPI_COMM_WORLD);
      }
      printf("%d: I AM A DONE NOW, AND MY LEADER IS %d\n", my_rank, leader);
    }
  }

  MPI_Finalize();
  return 0;
}
