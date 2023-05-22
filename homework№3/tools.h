#define DATA "/data"
#define SEMAPHORE "/semaphore"

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>

#define N_BEES 20
#define MAXPENDING 5

void (*prev_server)(int);
void (*prev_bee)(int);

void exit_error(char *error_string) {
    printf("%s", error_string);
    exit(-1);
}

enum CONDITIONS {
  BEAR = 33, BEE_IN = 44, BEE_OUT = 45, STOLE = 66, STUNG = 127, EMPTY = 0,
};

typedef struct {
  int honey;  // Кол-во меда в улье
  int bee_in; // Кол-во пчел в улье
} hive;