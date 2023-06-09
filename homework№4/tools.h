#define HIVE "/data"
#define BUFFER "/buffer"
#define POINTER "/pointer"
#define SEM_HIVE "/semaphore_hive"
#define SEM_BUFF "/semaphore_buff"

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


#define MAXPENDING 5
#define BUFF_LEN 10000
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
