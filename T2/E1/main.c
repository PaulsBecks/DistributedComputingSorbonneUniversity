#include <stdio.h>
#include <mpi.h>

//************   LES TAGS
#define WANNA_CHOPSTICK 0 // Demande de baguette
#define CHOPSTICK_YOURS 1 // Cession de baguette
#define DONE_EATING 2     // Annonce de terminaison

//************   LES ETATS D'UN NOEUD
#define THINKING 1 // N'a pas consomme tous ses repas & n'attend pas de baguette
#define HUNGRY 0   // En attente de baguette
#define DONE 2     // A consomme tous ses repas

//************   LES REPAS
#define NB_MEALS 3 // nb total de repas a consommer par noeud

//************   LES VARIABLES MPI
int NB;          // nb total de processus
int rank;        // mon identifiant
int left, right; // les identifiants de mes voisins gauche et droit

//************   LA TEMPORISATION
int local_clock = 0;      // horloge locale
int clock_val;            // valeur d'horloge recue
int meal_times[NB_MEALS]; // dates de mes repas

//************   LES ETATS LOCAUX
int local_state = THINKING; // moi
int left_state = THINKING;  // mon voisin de gauche
int right_state = THINKING; // mon voisin de droite

//************   LES BAGUETTES
int left_chopstick = 0;  // je n'ai pas celle de gauche
int right_chopstick = 0; // je n'ai pas celle de droite

//************   LES REPAS
int meals_eaten = 0; // nb de repas consommes localement

//************   LES FONCTIONS   ***************************
int max(int a, int b)
{
  return (a > b ? a : b);
}

void receive_message(MPI_Status *status)
{
  // A completer
  int i;
  MPI_Recv(&i, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, status);
  local_clock = max(local_clock, i) + 1;
}

void send_message(int dest, int tag)
{
  local_clock++;
  //printf("%d sends %d\n", rank, local_state);
  MPI_Send(&local_clock, 1, MPI_INT, dest, tag, MPI_COMM_WORLD);
}

/* renvoie 0 si le noeud local et ses 2 voisins sont termines */
int check_termination()
{
  //printf("%d, %d, %d, %d, %d\n", local_state, left_state, right_state, meals_eaten, rank);
  // A completer
  if (local_state == DONE && left_state == DONE && right_state == DONE)
  {
    return 0;
  }
  return 1;
}

//************   LE PROGRAMME   ***************************
int main(int argc, char *argv[])
{

  MPI_Status status;

  MPI_Init(&argc, &argv);
  MPI_Comm_size(MPI_COMM_WORLD, &NB);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  left = (rank + NB - 1) % NB;
  right = (rank + 1) % NB;

  while (check_termination())
  {

    /* Tant qu'on n'a pas fini tous ses repas, redemander les 2 baguettes  
         a chaque fin de repas */

    //if you are thinking change to hungry
    if (local_state == THINKING)
    {
      local_state = HUNGRY;
    }

    // tell the neighbors what's up
    send_message(left, local_state);
    send_message(right, local_state);

    receive_message(&status);

    if (status.MPI_TAG == DONE_EATING || status.MPI_TAG == CHOPSTICK_YOURS)
    {
      // left sends me his status
      if (status.MPI_SOURCE == left)
      {
        // I will save his status
        left_state = status.MPI_TAG;
        // and I will get the chopstick
        left_chopstick = 1;
      }
      // right sends me his status
      else if (status.MPI_SOURCE == right)
      {
        // I will save his status
        right_state = status.MPI_TAG;
        // and I will get the chopstick
        right_chopstick = 1;
      }
      // I have both shopsticks now
      if (left_chopstick && right_chopstick)
      {
        // and I am hungry
        if (local_state == HUNGRY)
        {
          // I eat
          meals_eaten++;
          printf("%d eats a deliciose meal, it's his %d. already, take a break dude. Btw it's time: %d\n", rank, meals_eaten, local_clock);
          // when I am done
          if (meals_eaten >= NB)
          {
            // I  finish
            local_state = DONE;
            // and tell my neighbors I am done
            send_message(left, DONE);
            send_message(right, DONE);
          }
          else
          {
            // if I am not done I start to think
            local_state = THINKING;
          }
        }
      }
    }
    else
    {
      // A Completer
      if (status.MPI_SOURCE == left)
      {
        left_state = status.MPI_TAG;
        if (max(rank, left) == rank)
        {
          // the left one should eat first
          send_message(left, CHOPSTICK_YOURS);
        }
      }
      else if (status.MPI_SOURCE == right)
      {
        right_state = status.MPI_TAG;
        if (max(rank, right) == rank)
        {
          //the right should eat first
          send_message(right, CHOPSTICK_YOURS);
        }
      }
    }
  }

  MPI_Finalize();
  return 0;
}