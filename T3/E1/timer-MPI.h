#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <mpi.h>
#include <pthread.h>
#include "msg.h"

#define SIZE 10
#define NB_CONFIG 3

#define TAILLE 50

#define FAIL 0
#define SUCCESS 1


#define HB 2
#define TIME 1
#define MESS 0


#define ADD 0
#define SOUS 1

#define STOP 7
#define HEARTBEAT 6
#define TOKEN 5
#define CONT 4
#define OK 2
#define BCAST 1
#define SEQ 0

#define MAX(x,y) ((x) > (y) ? (x) : (y))

typedef struct message {
	/* pour les message FUNC, id correspond au numero de sequence du timer */
	int id;
	int type;
	union {
		Buff m;
		void (*f)(int, int);
	} content;
	int dest;
	int count;
	MPI_Datatype datatype;
	int tag;
	int success;
	
	/* deadline pour comparer dans callout, et delai pour le timer */
	struct timeval deadline, delai;
} Message;

typedef struct stats {
	
	/* id du destinataire */
	int dest;

	/* latence en seconde et usecondes*/
	struct timeval latence;
	
	/*variance de la latence*/
	int variance;
	
	/* taux de perte: 1 perte toute les perte messages envoyé */
	int perte;
	
	/* nombre de message de cette configuration */
	int nbMessage;

} Stats;

void terminaison(int);


void vector_display(int vec[]);

void update_callout(int n);

void afficher_callout();

void timeval_calcul(struct timeval* res, struct timeval t1, struct timeval t2, int tag);

int timeval_min(struct timeval t1, struct timeval t2);

void handler(int signal);

int Initialisation();

int insert_elem(Message e);

int timer_update(Message e);

void cancel_timer(int dest);
void set_my_timer(int id, int dest, struct timeval t, void (*f)(int, int));
int reset_my_timer(int id, int dest, void (*f)(int, int));

int MPI_NSend(void* data, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm communicator);
