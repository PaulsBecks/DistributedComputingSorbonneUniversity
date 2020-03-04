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
#define ACK 2

int max(int i, int j)
{
  if (i > j)
    return i;
  return j;
}

int normalize(int i, int size)
{
  if (i == -1)
  {
    i = size - 1;
  }
  return i;
}
int main(int argc, char *argv[])
{
  int size;
  int my_rank;
  int round = 1;
  srand(getpid());

  int status = UNDECIDED;
  char *predecessor_message;

  MPI_Status message_status;

  MPI_Init(&argc, &argv);
  MPI_Comm_size(MPI_COMM_WORLD, &size);
  MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);

  int right = (my_rank + 1) % size;
  int left = normalize((my_rank - 1), size);

  int isInitiator = rand() % 2;

  int leader = my_rank;

  printf("%d: I AM AN INITIATOR\n", my_rank);
  MPI_Send(&my_rank, 1, MPI_INT, right, ELEC, MPI_COMM_WORLD);

  MPI_Send(&my_rank, 1, MPI_INT, left, ELEC, MPI_COMM_WORLD);

  while (status != DONE)
  {
    int other_leader;
    MPI_Recv(&other_leader, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &message_status);
    // if you are a looser you need to stop here, forward the message and maybe change the leader and status
    if (status == LOOSER)
    {
      if (message_status.MPI_TAG == ELEC)
      {
        if (other_leader >= leader)
        {
          leader = other_leader;
          //forward to the direction the message did not come from
          if (left == message_status.MPI_SOURCE)
          {
            MPI_Send(&leader, 1, MPI_INT, right, ELEC, MPI_COMM_WORLD);
          }
          else
          {
            MPI_Send(&leader, 1, MPI_INT, left, ELEC, MPI_COMM_WORLD);
          }
        }
      }
      else
      {
        status = DONE;
        if (my_rank != message_status.MPI_SOURCE)
        {
          MPI_Send(&leader, 1, MPI_INT, right, ANNOUNCE, MPI_COMM_WORLD);
        }
        printf("%d: I AM A DONE NOW. AND MY LEADER IS %d\n", my_rank, leader);
      }
      continue;
    }
    int another_leader;

    // receive a message from the other side
    int source = left;
    if (message_status.MPI_SOURCE == left)
    {
      source = right;
    }
    MPI_Recv(&another_leader, 1, MPI_INT, source, MPI_ANY_TAG, MPI_COMM_WORLD, &message_status);

    // if you receive two messages you know a round is over
    other_leader = max(other_leader, another_leader);

    if (message_status.MPI_TAG == ELEC)
    {
      // if you receive a elec message from your self, you know you are the leader
      if (other_leader == my_rank)
      {
        printf("%d: I AM THE LEADER IN ROUND %d\n", my_rank, round);
        MPI_Send(&leader, 1, MPI_INT, right, ANNOUNCE, MPI_COMM_WORLD);
        status = DONE;
      }
      // otherwise you have to check if your neighbors are the leader and change the status
      else
      {
        if (other_leader > leader)
        {
          leader = other_leader;
          status = LOOSER;
          printf("%d: I AM A LOOSER NOW\n", my_rank);
        }
        if (other_leader < leader)
        {
          MPI_Send(&my_rank, 1, MPI_INT, right, ELEC, MPI_COMM_WORLD);
          MPI_Send(&my_rank, 1, MPI_INT, left, ELEC, MPI_COMM_WORLD);
        }
      }
    }
    round++;
  }

  MPI_Finalize();
  return 0;
}
