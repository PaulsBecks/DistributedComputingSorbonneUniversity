/* Maximum number of processes */

#define SIZE 10

/* Hearbeat message format */

typedef struct buff {
  int seq;
  char msg[10];
}Buff;
