#include "timer-MPI.h"

int rank,size, nb_mess, num_seq;


/* date for comparison  */
struct timeval deadline_min, nulle, fault_timer[SIZE];

/* date courante */
struct timeval time_of_day;

/* timer correspondant a notre signal */
struct itimerval timer;

/* gestionnaire de signaux */
struct sigaction prepaSignal, term;

/* table des callout (message d'envoie) */
Message callout[TAILLE];

/* matrice de configuration des canaux */
/* premiere dimension: table des caracteristiques du canal (MOI,i) */
Stats stats[SIZE][NB_CONFIG];

/* cpt_message[i]: nombre de message envoyé à i */
unsigned int cpt_message[SIZE];

/* ptr_stats[i]:pointeur sur la stat courante pour i */
int ptr_stats[SIZE];


void terminaison(int j) {
	timer.it_value.tv_sec = 0;
	timer.it_value.tv_usec = 0;
	setitimer(ITIMER_REAL, &timer, NULL);
	MPI_Finalize();
	printf("End of proc %d\n",j);
	exit(0);
}


void copy(Message* src, Message* dest) {
	memcpy(dest, src, sizeof(src));
	if (dest->type == MESS) {
		dest->content.m = src->content.m;
//		memcpy(dest->content.m.vector, src->content.m.vector, SIZE*4);
	}
	else {
		dest->content.f = src->content.f;
	}
}


void cancel_timer(int dest) {
	int i;
	for(i = 0; i < SIZE; i++) {
		if ((callout[i].id != -1) && (callout[i].type == TIME && callout[i].dest == dest)) {
			callout[i].id = -1;
			//			printf("proc %d: timer %d cancelled\n", rank,dest);
			nb_mess--;
			break;
		}
	}
	for(; i < SIZE-1; i++) {
		copy(&callout[i+1], &callout[i]);
		callout[i+1].id = -1;
	}
}

/*id: num_seq */
int reset_my_timer(int id, int dest, void (*f)(int, int)) {
	Message e = {
		.id = id,
		.dest = dest,
		.type = TIME,
		.content.f = f,
		.delai = fault_timer[dest]
	};
	gettimeofday(&time_of_day, NULL);
	timeval_calcul(&e.deadline, fault_timer[dest], time_of_day, ADD);
	
	return insert_elem(e);
}


void set_my_timer(int id, int dest, struct timeval t, void (*f)(int, int)) {
	fault_timer[dest] = t;
	reset_my_timer(id, dest, f);
}


void vector_display(int vec[]) {
	int i;
	printf("proc %d vector: ", rank);
	for(i = 0; i < size; i++) {
		printf("%d ", vec[i]);
	}
	printf("\n");
}

/* met a jour la table des callout en remontant de n cases chaque entree */
void update_callout(int n) {
	int i;
	for(i = n; i < TAILLE; i++) {
		callout[i-n] = callout[i];
		callout[i].id = -1;
	}
}

/* affiche l'etat des entree de la table de callout */
void afficher_callout() {
	int i;
	for(i = 0; i < TAILLE; i++) {
		printf("callout[%d] = %d, pour %d; deadline: %ld secondes, %ld usecondes; TYPE: %d\n", i, callout[i].id, callout[i].dest, callout[i].deadline.tv_sec,callout[i].deadline.tv_usec, callout[i].type);
	}
	printf("\n\n");
}

/* fonction de calcul (addition et soustraction) pour les struct timeval, stocke dans res le resultat*/
void timeval_calcul(struct timeval* res, struct timeval t1, struct timeval t2, int tag) {
	long usec;
	res->tv_sec = 0;
	res->tv_usec = 0;
	switch (tag) {
	case ADD: {
			usec = t1.tv_usec + t2.tv_usec;
			res->tv_sec = t1.tv_sec + t2.tv_sec;
			while (usec >= 1000000) {
				res->tv_sec++;
				usec -= 1000000;
			}
			res->tv_usec = usec;
			break;
		}
	case SOUS: {
			usec = t1.tv_usec - t2.tv_usec;
			res->tv_sec = t1.tv_sec - t2.tv_sec;
			if (usec < 0) {
				res->tv_sec--;
				res->tv_usec = 1000000 + usec;
			}
			else {
				res->tv_usec = usec;
			}
			break;
		}
	default: {
		break;
		}
	}
}

/* fonction de comparaison pour les struct timeval*/
/* valeur retournee: -1 si t1 < t2,
					0 si t1 = t2,
					 1 sinon*/
int timeval_cmp(struct timeval t1, struct timeval t2) {

	if(t1.tv_sec == t2.tv_sec) {
		if(t1.tv_usec == t2.tv_usec) return 0;
		if(t1.tv_usec < t2.tv_usec) return -1;
		if(t1.tv_usec > t2.tv_usec) return 1;
	}
	if(t1.tv_sec < t2.tv_sec) return -1;
	if(t1.tv_sec > t2.tv_sec) return 1;
}

/* handler du gestionnaire*/
void handler(int signal) {

	int i = 0;
	int j = 0;
	
	int cpt = 0; /*nombre de message envoyé durant l'appel*/
	float tempo = 0;
	Message m;
	/* tant qu'il ya des message a envoyer */
	while((i < TAILLE && callout[i].id != -1) && ((timeval_cmp(callout[i].delai, nulle) == 0) || i == 0)) {
		/* si c'est le timer qui s'est ecoule */
		if (callout[i].type != MESS) {
			callout[i].content.f(callout[i].id, callout[i].dest);
		}
		/* si c'est un message et qu'il n'est pas perdu */
		else if (callout[i].success == SUCCESS) {
			MPI_Send(&(callout[i].content.m), callout[i].count, callout[i].datatype, callout[i].dest, callout[i].tag, MPI_COMM_WORLD);
		} 
		callout[i].id = -1;
		i++;
		cpt++;
		nb_mess--;
	}
	/*s'il reste encore des messages, on met a jour le timer*/
	if (callout[i].id != -1) {
		update_callout(cpt);
		timer_update(callout[0]);
	}
}

/*fonction d'initialisation de l'environnement*/
int Initialisation() {
	FILE* f;
	int i, j, k;
	Stats s;
	
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	
	num_seq = 0;
	nb_mess = 0;
	k = 0;
	nulle.tv_sec = 0;
	nulle.tv_usec = 0;
	f = fopen("./config", "r");
	if (f == NULL) {
		fprintf(stderr, "Impossible d'ouvrir le fichier de config\n");
	//	return 1;
	}
	for (i = 0; i < SIZE; i++) {
		ptr_stats[i] = 0;
		cpt_message[i] = 0;
		for(j = 0; j < NB_CONFIG; j++) {		
			stats[i][j].latence.tv_sec = 0;
			stats[i][j].latence.tv_usec = 0;
			stats[i][j].perte = 0;
			stats[i][j].nbMessage = -1;
			stats[i][j].variance = 0;
		}				

	}
	for (i = 0; i < TAILLE; i++) {
		callout[i].id = -1;
	}
	
	char buff[500];
	fgets(buff, 500, f);
	fgets(buff, 500, f);
	
	while (fscanf(f, "%d %d %ld %ld %d %d %d\n", &i, &j, &s.latence.tv_sec, &s.latence.tv_usec, &s.variance, &s.perte, &s.nbMessage) != -1) {
		if(i == rank){
			stats[j][k] = s;
			k++;
		} else {
			if(j == rank) {
				stats[i][k] = s;
				k++;
			}
		}
		if (s.nbMessage == -1) k = 0;
	}
	fclose(f);
	
	prepaSignal.sa_handler=&handler;
	sigemptyset(&prepaSignal.sa_mask);

	term.sa_handler = &terminaison;
	sigemptyset(&term.sa_mask);
	sigaction(SIGINT, &term, 0);
	return 0;
}

/*fonction d'insertion pour la table de callout*/
int insert_elem(Message e) {
	/*p1: entree de callout pour insert e*/
	int p1, p2;
	p1 = 0;
	if (nb_mess == TAILLE) {
		printf("no callout space nb_mess = %d\n", nb_mess);
		return 1;
	}
	/*tant que l'deadline de e est plus grande que le message courant */
	while ((p1 < TAILLE) && ((callout[p1].id != -1) && (timeval_cmp(callout[p1].deadline,e.deadline) <= 0))) {
		/* delai correspond au temps entre mon deadline et celui du message precedent*/
		timeval_calcul(&e.delai,e.deadline,callout[p1].deadline, SOUS);
		p1++;
	}
	/*plus de place*/
	if(p1 >= TAILLE-1 && callout[p1].id != -1) {
		printf("proc %d: no callout space p1 = %d\n", rank, p1);
		return 1;
	}

	if (callout[p1].id != -1) {
		timeval_calcul(&callout[p1].delai,callout[p1].deadline,e.deadline, SOUS);
		p2 = nb_mess-1;
		while(p2 >= p1) {
			callout[p2+1] = callout[p2];
			p2--;
		}
	}

	
	callout[p1] = e;
	nb_mess++;
	num_seq++;

//	printf("proc %d: p1 = %d\n", rank,p1);
	/* si on insere e en tete, on met a jour le timer*/
	if (p1 == 0) {
		timer_update(e);
	}
	return 0;
}

/*fonction de mise a jour du timer*/
int timer_update(Message e) {
	/*si e doit etre envoyé dans l'immediat, on l'envoie*/
	if (timeval_cmp(e.delai, nulle) == 0) {
//		printf("coucou update\n");
		handler(14);
		return 1;
	}
	/*le test est important sinon le timer des detruit*/
	timer.it_value = e.delai;
	sigaction(SIGALRM, &prepaSignal, 0);
	setitimer(ITIMER_REAL, &timer, NULL);
	return 0;
}

/* fonction d'envoie de message MPI*/
int MPI_NSend(void* d, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm communicator) {

	Message e;
	Stats s;
	int range;
	struct timeval tmp;
	
	cpt_message[dest]++;
	if ((stats[dest][ptr_stats[dest]].nbMessage != -1) && (cpt_message[dest] > stats[dest][ptr_stats[dest]].nbMessage)) {
		ptr_stats[dest]++;
		cpt_message[dest] = 1;
	}
	if ((stats[dest][ptr_stats[dest]].perte != 0) && (cpt_message[dest] % stats[dest][ptr_stats[dest]].perte == 0)) {
		e.success = FAIL;
	}
	else {
		e.success = SUCCESS;
	}
	Buff* data = (Buff*) d;
	
	MPI_Comm_size(MPI_COMM_WORLD, &size);
	s = stats[dest][ptr_stats[dest]];
	e.id = num_seq;
	num_seq++;

	memcpy((Buff *) &e.content.m,data,sizeof(*data));
	e.dest = dest;
	e.count = count;
	e.datatype = datatype;
	e.tag = tag;
	e.type = MESS;
	/*par defaut delai = latence*/
	/*sinon, le delai correspond au temps entre mon deadline et celui du message precedent*/
	e.delai = s.latence;
	
	
	/* calcul de l'deadline du message */
	gettimeofday(&time_of_day, NULL);
	
	/*calcul du nouveau delai et de la nouvelle deadline*/
	tmp.tv_sec = e.delai.tv_sec*s.variance/100.0;
	tmp.tv_usec = e.delai.tv_usec*s.variance/100.0;
	range = rand() % 3;
	switch (range) {
	case 1:{
		timeval_calcul(&e.delai, e.delai, tmp, ADD);
		break;
		}
	case 2:{
		timeval_calcul(&e.delai, e.delai, tmp, SOUS);
		break;
		}
	default:{}
	}
	timeval_calcul(&e.deadline,time_of_day,e.delai, ADD);
//	printf("proc %d: préinsertion du message %d pour %d %ld sec %ld usec\n",rank, e.m.id, e.dest,e.delai.tv_sec, e.delai.tv_usec);
	int ret = insert_elem(e);
	return ret;
}
