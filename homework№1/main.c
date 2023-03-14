#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main () {

    pid_t pid;
    pid = fork();
    pid = fork();

    printf("Fork-Test\n");

    return EXIT_SUCCESS;
}