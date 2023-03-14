#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <unistd.h>
#include "../algo.h"

#define BUFFER_SIZE 5000

#define NUMBER_OF_FILES 2 + 1
#define FORK_ERROR -1
#define FORK_SUCCESS 0

#define input "tmp/FIFO-1"
#define output "tmp/FIFO-2"

void processString(char *str, int n_bytes, char *used1, char *used2) {
    int id = 0;
    for (int i = 0; i < 26; ++i) {
        used1[i] = 0;
        used2[i] = 0;
    }
    while (id < n_bytes && str[id] != '\n') {
        used1[str[id] - 'a'] = 1;
        ++id;
    }
    ++id;
    while (id < n_bytes && str[id] != '\0') {
        used2[str[id] - 'a'] = 1;
        ++id;
    }
}

int processUsed(char *used, char *str) {
    int id = 0;
    for (int i = 0; i < 26; ++i) {
        if (used[i] == 0) {
            str[id] = 'a' + i;
            ++id;
        }
    }
    return id;
}

int main(int argc, char **argv) {
    if (argc != NUMBER_OF_FILES) {
        exit_error("Incorrect number of parameters. Enter two files. \n");
    }
    
    pid_t chpid1;

    if ((chpid1 = fork()) == FORK_ERROR) {
            exit_error("Unable to create the first clide. \n");
    } else if (chpid1 == 0) {
        pid_t chpid2;
        if ((chpid2 = fork()) == FORK_ERROR) {
            exit_error("Unable to create the first clide. \n");
        } else if (chpid2 == 0) {
            int fp;
            char str[5010];
            int file = open(argv[1], O_RDONLY);
            int n_bytes = read(file, str, 5000);
            str[n_bytes] = '\0';

            umask(0);
            mknod(input, S_IFIFO|0777, 0);
            fp = open(input, O_WRONLY|O_CREAT);
            int a = write(fp, str, n_bytes + 1);
            printf("%i", a);
            close(fp);
            close(file);
            exit(0);
        } else {
            int status = 0;
            chpid1 = wait(&status);
            int fp;
            char str[5010];
            umask(0);
            mknod(input, S_IFIFO|0666, 0);
            fp = open(input, O_RDONLY);
            int n_bytes = read(fp, str, 5000);
            printf("%s\n", str);
            close(fp);
            
            char used1[26], used2[26];
            processString(str, n_bytes, used1, used2);

            umask(0);
            mknod(output, S_IFIFO|0666, 0);
            fp = open(output, O_WRONLY|O_CREAT);
            write(fp, used1, sizeof(used1));
            write(fp, used2, sizeof(used2));
            close(fp);
            exit(0);
        }
    } else {
        int status = 0;
        chpid1 = wait(&status);
        int fp;
        char used1[26];
        char used2[26];
        umask(0);
        mknod(output, S_IFIFO|0666, 0);
        fp = open(output, O_RDONLY);
        read(fp, used1, sizeof(used1));
        read(fp, used2, sizeof(used2));
        close(fp);

        int file = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC);
        char str1[26];
        int n_bytes = processUsed(used2, str1);
        write(file, str1, n_bytes);
        write(file, "\n", 1);
        n_bytes = processUsed(used1, str1);
        write(file, str1, n_bytes);
        close(file);
    }
    unlink(input);
    unlink(output);
    return 0;
}
