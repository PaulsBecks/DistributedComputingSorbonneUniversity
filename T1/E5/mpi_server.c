#include <sys/types.h>
#include <pthread.h>
#include "mpi_server.h"

static Server server;

void start_server(void (*callback)(int tag, int source))
{
  printf("server started");
}

void destroy_server()
{
  printf("destroy server");
}

pthread_mutex_t *getMutex()
{
  return &server.mutex;
}
