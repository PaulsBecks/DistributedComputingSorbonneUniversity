#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <mpi.h>
#include <math.h>

#define MAX_CS 4

#define REQUEST 0
#define REPLY 1
#define END 2

int *queue;
int queue_counter = 0;
int size;
int count = 0;
int count_ended = 0;
int clock = 0;
int last_cs_asked = 0;
int my_rank = 0;
int cs_received = 0;

int max(int i, int j)
{
  if (i > j)
    return i;
  return j;
}
//wait msg:
// receive MPI
// switch between different options
int wait_for_messages()
{
  MPI_Status status;
  int other_clock;
  MPI_Recv(&other_clock, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
  clock = max(clock, other_clock) + 1;
  int tag = status.MPI_TAG;
  if (tag == REQUEST)
  {
    // HANDLE REQUEST

    // check if you should reply
    //printf("Received REQUEST from %d at %d\n", status.MPI_SOURCE, my_rank);
    if (last_cs_asked > other_clock || (last_cs_asked == other_clock && my_rank > status.MPI_SOURCE))
    {
      MPI_Send(&clock, 1, MPI_INT, status.MPI_SOURCE, REPLY, MPI_COMM_WORLD);
    }
    else
    {
      //printf("Added %d to queue of %d\n", status.MPI_SOURCE, my_rank);
      queue[queue_counter] = status.MPI_SOURCE;
      queue_counter++;
    }
  }
  if (tag == REPLY)
  {
    // HANDLE REPLY
    //printf("Received REPLY from %d at %d\n", status.MPI_SOURCE, my_rank);
    count++;
  }
  if (tag == END)
  {
    // HANDLE END
    printf("Received END from %d at %d\n", status.MPI_SOURCE, my_rank);
    count_ended++;
  }
}

int request_cs()
{
  clock++;
  last_cs_asked = clock;
  count = 0;
  for (int i = 0; i < size; i++)
  {
    if (my_rank != i)
    {
      //printf("SEND REQUEST from %d to %d\n", my_rank, i);
      MPI_Send(&clock, 1, MPI_INT, i, REQUEST, MPI_COMM_WORLD);
    }
  }
}

int end_it()
{
  clock++;
  last_cs_asked = clock;
  count = 0;
  for (int i = 0; i < size; i++)
  {
    if (my_rank != i)
    {
      //printf("SEND END from %d to %d\n", my_rank, i);
      MPI_Send(&clock, 1, MPI_INT, i, END, MPI_COMM_WORLD);
    }
  }
}

int release_cs()
{
  clock++;
  for (int i = 0; i < queue_counter; i++)
  {
    //printf("SEND RELEASE from %d to %d\n", my_rank, queue[i]);
    MPI_Send(&clock, 1, MPI_INT, queue[i], REPLY, MPI_COMM_WORLD);
  }
  queue_counter = 0;
}

int main(int argc, char *argv[])
{
  MPI_Init(&argc, &argv);
  MPI_Comm_size(MPI_COMM_WORLD, &size);
  MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
  queue = malloc(sizeof(int) * size);
  while (cs_received < MAX_CS)
  {
    // REQUEST CS
    request_cs();
    while (count < size - 1)
    {
      //printf("Waiting for messages at %d cs_received: %d count: %d, count_ended: %d\n", my_rank, cs_received, count, count_ended);
      wait_for_messages();
    }
    //printf("Going into the CS in process %d\n", my_rank);
    cs_received++;
    release_cs();
  }
  printf("CS RECEIVED %d in %d AND TERMINATES\n", cs_received, my_rank);
  end_it();
  while (count_ended < size - 1)
  {
    wait_for_messages();
  }
  MPI_Finalize();
  return 0;
}
