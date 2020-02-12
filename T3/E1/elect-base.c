#include "timer-MPI.h"
//#include <pthread.h>
#include <stdlib.h>

#define TRUE 1
#define NMSG 10


//pthread_t beat;
//pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

MPI_Datatype mon_type;


/* size <= SIZE */
int size, rank, delta, num_seq[SIZE];
int seq = 0;

int scpt[SIZE];

struct timeval timer[SIZE];
struct timeval now, last, nulle;



Buff buff;
void starthb(int,int);
void PrintStat(void);

Message hb = {
	.id = 0,
	.type = HB,
	.content.f = starthb,
};


void check_end() {
	int i;
	for (i=0;i<size;i++)
		if (i!= rank && num_seq[i] < NMSG)
			break;
	if (i==size || (i==size-1 && rank==i)){
		PrintStat();
		terminaison(rank);
	}
}



void starthb(int i, int j) {
	int k;
	if (seq == NMSG) {
	  PrintStat();
	  terminaison(rank);
	}
	
	printf("Proc %d : SEND HD id:%d\n",rank,seq);
	for(k = 0; k < size;k++) {
		buff.seq=seq;
		strcpy(buff.msg,"HB");

		/* TO COMPLETE */
		
		if(k != rank && num_seq[k] < NMSG) MPI_NSend(&buff, 1, mon_type, k, HEARTBEAT, MPI_COMM_WORLD);
	}
	seq++;
	gettimeofday(&hb.deadline, NULL);
	timeval_calcul(&hb.deadline, hb.deadline, hb.delai, ADD);
	insert_elem(hb);
}


int leader() {
  int l=0;

  /* TO COMPLETE */
  
  return l;
}

void end_timer(int id, int i) {
	int k;

	if ((i == rank)) return;
	

	/* TO COMPLETE */
	
	check_end();

}

void reception_heartbeat() {
	Buff b;
	MPI_Status status;
	int i;

	MPI_Recv(&b, 1, mon_type, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
	num_seq[status.MPI_SOURCE] = seq;
	/* TO COMPLETE */
	
	check_end();
	cancel_timer(status.MPI_SOURCE);
	reset_my_timer(num_seq[status.MPI_SOURCE], status.MPI_SOURCE, end_timer);
}


void PrintStat() {
	int i;
	printf("-----------------------------------\n");
	for (i=0; i< size; i++)
	//	if (i!=rank)
		printf("PROC %d: proc %d suspected %d times\n",rank, i, scpt[i]);
	printf("PROC %d, LEADER = %d\n", rank, leader());
	printf("-----------------------------------\n");
	
}

int main(int argc, char** argv) {
	int provided;
	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &size);
	MPI_Status status;
	hb.delai.tv_sec = atoi(argv[1]);
	hb.delai.tv_usec = atoi(argv[2]);

	/* TO COMPLETE */
	int blocklen[2] = {1, 10};

	MPI_Datatype typemsg[2];
	typemsg[0] = MPI_INT;
	typemsg[1] = MPI_CHAR;

	MPI_Aint offmsg[3];
	offmsg[0] = offsetof(Buff,seq);
	offmsg[1] = offsetof(Buff,msg);


	MPI_Type_create_struct(2, blocklen, offmsg, typemsg, &mon_type);
	MPI_Type_commit(&mon_type);


	nulle.tv_sec = 0;
	nulle.tv_usec = 0;
	Initialisation();

	int i;
	hb.dest = rank;
	starthb(rank, 0);

	for(i = 0; i < size; i++) {
		num_seq[i] = 0;
		scpt[i]=0;
		timer[i].tv_sec = hb.delai.tv_sec + 1;
		timer[i].tv_usec = 0;
	}
	for(i = 0; i < size; i++) {
		if (i != rank) set_my_timer(num_seq[i], i, timer[i], end_timer);
	}
	gettimeofday(&now, NULL);
	last = now;
	while(1) {
		reception_heartbeat();

	}
	return 0;
}


