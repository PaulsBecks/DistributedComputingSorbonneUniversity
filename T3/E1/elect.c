#include "timer-MPI.h"
//#include <pthread.h>
#include <stdlib.h>

/*** Terminaison parameters ***/

/* Max number of heartbeat sent by a node */
#define NMSG 12

/*** Failure injection parameter ***/

/* Node FNODE fails after FTIME msg */
#define FNODE 0
/* Node 0 fail after FTIME message */
#define FTIME 3

MPI_Datatype my_type;

/* size <= SIZE */
int size, rank, delta, num_seq[SIZE];
int seq = 0;

int trusted = 0;

int scpt[SIZE][SIZE];

struct timeval timer[SIZE];
struct timeval now, last, nulle;

Buff buff;
void starthb(int, int);
void PrintStat(void);

Message hb = {
    .id = 0,
    .type = HB,
    .content.f = starthb,
};

void print_scpt(int *scpt)
{
  for (int i = 0; i < SIZE; i++)
  {
    printf("received %d for %d\n", i, scpt[i]);
  }
}

/* hearbeat function periodically called */

void starthb(int id, int proc)
{
  int k;
  if ((rank == FNODE && seq == FTIME) || seq == NMSG)
  {
    if (rank == FNODE)
      printf("Proc %d CRASH\n", FNODE);
    PrintStat();
    terminaison(rank);
  }

  printf("Proc %d : SEND HD id:%d\n", rank, seq);
  for (k = rank + 1; k < size; k++)
  {
    buff.seq = seq;
    strcpy(buff.msg, "HB");

    /* TO COMPLETE */

    memcpy(buff.count, scpt[rank], SIZE * sizeof(int));

    if (k != rank && num_seq[k] < NMSG)
      MPI_NSend(&buff, 1, my_type, k, HEARTBEAT, MPI_COMM_WORLD);
  }
  seq++;

  /* Set next heartbeat function call */
  gettimeofday(&hb.deadline, NULL);
  timeval_calcul(&hb.deadline, hb.deadline, hb.delai, ADD);
  insert_elem(hb);
}

int scpt_sum(int proc)
{
  int sum = 0;
  for (int i = 0; i < SIZE; i++)
  {
    sum += scpt[i][proc];
  }
  return sum;
}

int leader()
{
  int l = 0;

  /* TO COMPLETE */
  for (int i = 1; i < SIZE; i++)
  {
    if (scpt_sum(l) > scpt_sum(i))
    {
      l = i;
    }
  }
  return trusted;
}

void end_timer(int id, int proc)
{
  int k;

  printf("Proc %d: %d is down\n", rank, proc);
  if ((proc == rank))
    return;

  if (proc == trusted && proc < rank)
  {
    trusted++;
  }
  /* TO COMPLETE */
  scpt[rank][proc]++;
  reset_my_timer(num_seq[proc], proc, end_timer);
}

void reception_heartbeat()
{
  Buff b;
  MPI_Status status;
  int i;

  if (MPI_Recv(&b, 1, my_type, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status) != MPI_SUCCESS)
    terminaison(rank);

  if (status.MPI_SOURCE < trusted)
  {
    trusted = status.MPI_SOURCE;
  }
  printf("Proc %d: %d alive <seq:%d,data:%s>, leader %d\n", rank, status.MPI_SOURCE, b.seq, b.msg, leader());
  num_seq[status.MPI_SOURCE] = seq;

  /* TO COMPLETE */

  memcpy(scpt[status.MPI_SOURCE], b.count, SIZE * sizeof(int));

  cancel_timer(status.MPI_SOURCE);
  reset_my_timer(num_seq[status.MPI_SOURCE], status.MPI_SOURCE, end_timer);
}

void PrintStat()
{
  int i;
  printf("-----------------------------------\n");
  for (i = 0; i < size; i++)
    printf("PROC %d: proc %d suspected %d times\n", rank, i, scpt_sum(i));
  printf("PROC %d, LEADER = %d\n", rank, leader());
  printf("-----------------------------------\n");
}

int main(int argc, char **argv)
{
  int provided;
  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);
  MPI_Status status;
  hb.delai.tv_sec = atoi(argv[1]);
  hb.delai.tv_usec = atoi(argv[2]);

  /* TO COMPLETE */
  int blocklen[3] = {1, 10, 10};

  MPI_Datatype typemsg[3];
  typemsg[0] = MPI_INT;
  typemsg[1] = MPI_CHAR;
  typemsg[2] = MPI_INT;

  MPI_Aint offmsg[3];
  offmsg[0] = offsetof(Buff, seq);
  offmsg[1] = offsetof(Buff, msg);
  offmsg[2] = offsetof(Buff, count);

  MPI_Type_create_struct(3, blocklen, offmsg, typemsg, &my_type);
  MPI_Type_commit(&my_type);

  nulle.tv_sec = 0;
  nulle.tv_usec = 0;
  Initialisation();

  int i;
  hb.dest = rank;
  starthb(rank, 0);

  /* Initialization of timers and arrays */
  for (i = 0; i < size; i++)
  {
    num_seq[i] = 0;
    for (int j = 0; j < SIZE; j++)
    {
      scpt[i][j] = 0;
    }
    timer[i].tv_sec = hb.delai.tv_sec + 1;
    timer[i].tv_usec = 0;
  }
  for (i = 0; i < size; i++)
  {
    if (i != rank)
      set_my_timer(num_seq[i], i, timer[i], end_timer);
  }
  gettimeofday(&now, NULL);
  last = now;

  /* Main loop */

  while (1)
  {
    reception_heartbeat();
  }
  return 0;
}
