#include <pthread.h>
#include "mpi_server.h"
#include <signal.h>

static volatile int keepRunning = 1;

void *create_server(void *arg)
{
  printf("hey\n");
  int time = 0;
  MPI_Status status;
  while (keepRunning)
  {
    printf("start listening\n");
    MPI_Recv(&time, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
    printf("Received from %d number %d\n", status.MPI_SOURCE, time);
  }
  pthread_exit(NULL);
}

void intHandler(int dummy)
{
  keepRunning = 0;
}

int main(int argc, char *argv[])
{
  int provided, flag, claimed, rank, size;
  pthread_t thread_id;
  signal(SIGINT, intHandler);

  MPI_Init_thread(&argc, &argv, MPI_THREAD_SERIALIZED, &provided);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);

  if (rank == 0)
  {

    pthread_create(&thread_id, NULL, create_server, NULL);
  }
  else
  {
    MPI_Ssend(&rank, 1, MPI_INT, 0, 99, MPI_COMM_WORLD);
  }
  pthread_join(thread_id, NULL);
  MPI_Finalize();
}
