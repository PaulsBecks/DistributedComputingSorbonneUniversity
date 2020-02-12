#include "timer-MPI.h"
#include <stdlib.h>

#define TRUE 1

/* Stop after MAXHB received */
#define MAXHB 10

/* Node 0 fail after FTIME message */
#define FTIME 3  

/* Max number of heartbeat sent by a node */
#define NMSG

MPI_Datatype mon_type;


/* size <= SIZE */
int size, rank, delta, num_seq[SIZE];
int seq = 0;

int suspect[SIZE];
int scpt[SIZE];

struct timeval timer[SIZE];
struct timeval now, last, nulle;

int pleader = 0;

Buff buff;
void starthb(int,int);
void PrintStat(void);

Message hb = {
	.id = 0,
	.type = HB,
	.content.f = starthb,
};


void check_end() {
  /*	int i;
	for (i=0;i<size;i++)
		if (i!= rank && num_seq[i] < NMSG)
			break;
	if (i==size || (i==size-1 && rank==i)){
		PrintStat();
		terminaison(rank);
	}
  */
}



void starthb(int i, int j) {
	int k;
	static int cpt=0;
	if ((rank == 0 && seq == FTIME) || seq >= NMSG) {
	  PrintStat();
	  terminaison(rank);
	}
	printf("Proc %d: Leader : %d\n", rank, pleader);	
	if (pleader == rank) {
	printf("Proc %d : SEND HD id:%d\n",rank,seq);
	for(k = rank; k < size;k++) {
		buff.seq=seq;
		strcpy(buff.msg,"HB");
		memcpy(buff.count, scpt, sizeof(int)*size);
		if(k != rank && seq < NMSG) MPI_NSend(&buff, 1, mon_type, k, HEARTBEAT, MPI_COMM_WORLD);
	}
	seq++;
	}

	gettimeofday(&hb.deadline, NULL);
	timeval_calcul(&hb.deadline, hb.deadline, hb.delai, ADD);
	insert_elem(hb);
}


int leader() {
	return pleader;
}

void end_timer(int id, int i) {
	int k;

	if ((i == rank)) return;
	//	printf("proc %d: %i suspected\n",rank, i);
	suspect[i] = 1;
	scpt[i]++;
	pleader = (pleader +1) % size;
	//	check_end();

}

void reception_heartbeat() {
	Buff b;
	MPI_Status status;
	int i;
	static int cpt =0;

	MPI_Recv(&b, 1, mon_type, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
	suspect[status.MPI_SOURCE] = -1;
	num_seq[status.MPI_SOURCE] = seq;
	for (i=0;i<size;i++) 
		scpt[i] = MAX(scpt[i],b.count[i]);

	if (status.MPI_SOURCE < pleader)
		pleader = status.MPI_SOURCE;
	
	if (cpt++ == MAXHB) {
	  PrintStat();
	  terminaison(rank);
	}	  

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

	int blocklen[3] = {1, 10, 10};

	MPI_Datatype typemsg[3];
	typemsg[0] = MPI_INT;
	typemsg[1] = MPI_CHAR;
	typemsg[2] = MPI_INT;

	MPI_Aint offmsg[3];
	offmsg[0] = offsetof(Buff,seq);
	offmsg[1] = offsetof(Buff,msg);
	offmsg[2] = offsetof(Buff,count);

	MPI_Type_create_struct(3, blocklen, offmsg, typemsg, &mon_type);
	MPI_Type_commit(&mon_type);


	nulle.tv_sec = 0;
	nulle.tv_usec = 0;
	Initialisation();

	int i;
	hb.dest = rank;
	starthb(rank, 0);

	for(i = 0; i < size; i++) {
		num_seq[i] = 0;
		suspect[i] = -1;
		scpt[i]=0;
		timer[i].tv_sec = hb.delai.tv_sec + 1;
		timer[i].tv_usec = 0;
	}
	for(i = 0; i < rank; i++) {
		set_my_timer(num_seq[i], i, timer[i], end_timer);
	}
	gettimeofday(&now, NULL);
	last = now;
	while(1) {
		reception_heartbeat();

	}
	return 0;
}


