#include <sys/mman.h>
#include <fcntl.h>
#include <semaphore.h>
#include <time.h>

#include "../tools.h"

hive *shared_hive;
char *shared_buff;
int *shared_pointer;

sem_t *sem_hive;
sem_t *sem_buff;
int shm_fd_hive;
int shm_fd_buff;
int shm_fd_pointer;
int servSock;

void clean() {
    // Удаление семафора
    close(shm_fd_hive);
    close(shm_fd_buff);
    close(shm_fd_pointer);
    sem_close(sem_hive);
    sem_close(sem_buff);
    sem_unlink(SEM_HIVE);
    sem_unlink(SEM_BUFF);
    // Удаление разделенной памяти
    munmap(shared_hive, sizeof(hive));
    munmap(shared_buff, BUFF_LEN * sizeof(char));
    shm_unlink(HIVE);
    shm_unlink(BUFFER);
    shm_unlink(POINTER);
    close(servSock);
}

// Команда прерывания
void handle_exit() {
    // Освобождение данных.
    clean();
    exit(0);
}

int AcceptTCPConnection(int childServSock) {
    int clntSock;
    struct sockaddr_in echoClntAddr;
    unsigned int clntLen;
    clntLen = sizeof(echoClntAddr);

    if ((clntSock = accept(childServSock, (struct sockaddr *) &echoClntAddr, &clntLen)) < 0) {
        exit_error("accept() failed");
    }
    printf("Handling client %s\n", inet_ntoa(echoClntAddr.sin_addr));

    return clntSock;
}

int CreateTCPServerSocket(unsigned short port) {
    int sock;
    struct sockaddr_in echoServAddr;

    if ((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        exit_error("socket() failed");
    }

    memset(&echoServAddr, 0, sizeof(echoServAddr));
    echoServAddr.sin_family = AF_INET;
    echoServAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    echoServAddr.sin_port = htons(port);

    if (bind(sock, (struct sockaddr *) &echoServAddr, sizeof(echoServAddr)) < 0) {
        exit_error("bind() failed\n");
    }

    if (listen(sock, MAXPENDING) < 0) {
        exit_error("listen() failed\n");
    }

    return sock;
}

void HandleTCPClient(int clntSocket) {
    char rcvBuff[1];
    int recvMsgSize;
    char outpMsg[201];
    outpMsg[0] = '\0';
    char c;
    if ((recvMsgSize = recv(clntSocket, rcvBuff, 1, 0)) < 0)
        exit_error("recv() failed");
    if (recvMsgSize == 1) {
        if (rcvBuff[0] == BEE_IN) {
            // Закрытие семафоры, складывание меда если есть место
            sem_wait(sem_hive);
            if (shared_hive->honey < 30) {
                ++shared_hive->honey;
                snprintf(outpMsg, 100, "Bee has added one honey. Total honey: %d\n", shared_hive->honey);
            }
            // Возвращение в улей
            ++shared_hive->bee_in;

            // Открытие семафора
            sem_post(sem_hive);
            c = BEE_IN;
            if (send(clntSocket, &c, 1, 0) != 1) {
                exit_error("send() bee failed\n");
            }
        } else if (rcvBuff[0] == BEE_OUT) {
            // Закрытие семафоры, берем мед если он есть
            sem_wait(sem_hive);
            if (shared_hive->bee_in != 0) {
                --shared_hive->bee_in;
                snprintf(outpMsg, 100, "Bee flew away to find new honey\n");
            }
            // Открытие семафора
            sem_post(sem_hive);
            c = BEE_OUT;
            if (send(clntSocket, &c, 1, 0) != 1) {
                exit_error("send() bee failed\n");
            }
        } else if (rcvBuff[0] == BEAR) {
            sem_wait(sem_hive);
            if (shared_hive->honey >= 15) {
                // Если пчел мало - воруем мед
                if (shared_hive->bee_in < 3) {
                    snprintf(outpMsg,
                             200,
                             "In hive now only %d bee. Winny has taken all honey. Total honey taken: %d\n Winny is running away from bees.\n",
                             shared_hive->bee_in,
                             shared_hive->honey);
                    shared_hive->honey = 0;
                    c = STOLE;
                    if (send(clntSocket, &c, 1, 0) != 1) {
                        exit_error("send() failed\n");
                    }
                } else {
                    // Иначе - нас кусают
                    snprintf(outpMsg,
                             100,
                             "In hive now %d bees. Winny could not take honey. Winnie was stung\n",
                             shared_hive->bee_in);
                    c = STUNG;
                    if (send(clntSocket, &c, 1, 0) != 1) {
                        exit_error("send() failed\n");
                    }
                }
            } else {
                c = EMPTY;
                if (send(clntSocket, &c, 1, 0) != 1) {
                    exit_error("send() failed\n");
                }
            }
            sem_post(sem_hive);
        } else {
            snprintf(outpMsg, 100, "Wrong data received on server\n");
        }
        printf("%s", outpMsg);
        sem_wait(sem_buff);
        memcpy(shared_buff + *shared_pointer, outpMsg, strlen(outpMsg));
        *shared_pointer += strlen(outpMsg);
        sem_post(sem_buff);
    }
}

void ProcessMain(int childServSock) {
    int clntSock = AcceptTCPConnection(childServSock);
    while (1) {
        HandleTCPClient(clntSock);
    }
}

int main(int argc, char *argv[]) {
    // Обработка сигнала SIGINT через метод handle_sigint
    prev_server = signal(SIGINT, handle_exit);

    // Сброс сида генерации
    srand(time(NULL));

    // Открываем разделяему память улья
    if ((shm_fd_hive = shm_open(HIVE, O_CREAT | O_RDWR, 0666)) == -1) {
        exit_error("Smth wrong with shm_open call \n");
    }
    // Устанавливаем размер память равный размеру hive
    if (ftruncate(shm_fd_hive, sizeof(hive)) == -1) {
        exit_error("Smth wrong with ftruncate call \n");
    }
    // Перекладываем разделяемую память на виртуальное пространство процесса
    if ((shared_hive = mmap(NULL, sizeof(hive), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd_hive, 0)) == MAP_FAILED) {
        exit_error("Smth wrong with mmap call \n");
    }

    if ((shm_fd_buff = shm_open(BUFFER, O_CREAT | O_RDWR, 0666)) == -1) {
        exit_error("Smth wrong with shm_open call \n");
    }
    if (ftruncate(shm_fd_buff, sizeof(char) * BUFF_LEN) == -1) {
        exit_error("Smth wrong with ftruncate call \n");
    }
    if ((shared_buff = mmap(NULL, sizeof(char) * BUFF_LEN, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd_buff, 0))
        == MAP_FAILED) {
        exit_error("Smth wrong with mmap call \n");
    }

    if ((shm_fd_pointer = shm_open(POINTER, O_CREAT | O_RDWR, 0666)) == -1) {
        exit_error("Smth wrong with shm_open call \n");
    }
    if (ftruncate(shm_fd_pointer, sizeof(int)) == -1) {
        exit_error("Smth wrong with ftruncate call \n");
    }
    if ((shared_pointer = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd_pointer, 0))
        == MAP_FAILED) {
        exit_error("Smth wrong with mmap call \n");
    }

    // Создание семафоров
    if ((sem_hive = sem_open(SEM_HIVE, O_CREAT, 0666, 1)) == SEM_FAILED) {
        exit_error("Smth wrong with sem_open call \n");
    }
    if ((sem_buff = sem_open(SEM_BUFF, O_CREAT, 0666, 1)) == SEM_FAILED) {
        exit_error("Smth wrong with sem_open call \n");
    }

    if (argc != 3) {
        fprintf(stderr, "Usage:  %s <Port> <N>\n", argv[0]);
        exit(1);
    }

    // Запись значений в улей
    int N_BEES = atoi(argv[2]);
    memset(shared_buff, 0, BUFF_LEN * sizeof(char));
    *shared_pointer = 0;
    shared_hive->bee_in = N_BEES;
    shared_hive->honey = 0;
    int val;
    sem_getvalue(sem_hive, &val);
    while (val > 0) {
        sem_wait(sem_hive);
        sem_getvalue(sem_hive, &val);
    }
    sem_getvalue(sem_buff, &val);
    while (val > 0) {
        sem_wait(sem_buff);
        sem_getvalue(sem_buff, &val);
    }
    sem_post(sem_hive);
    sem_post(sem_buff);
    unsigned short echoServPort;
    pid_t processID;
    unsigned int processCt;



    echoServPort = atoi(argv[1]);

    servSock = CreateTCPServerSocket(echoServPort);
    for (processCt = 0; processCt < N_BEES + 1; processCt++) {
        if ((processID = fork()) < 0) {
            exit_error("fork() failed\n");
        } else if (processID == 0) {
            signal(SIGINT, prev_server);
            ProcessMain(servSock);
            exit(0);
        }
    }
    if ((processID = fork()) < 0) {
        exit_error("fork() failed\n");
    } else if (processID == 0) {
        signal(SIGINT, prev_server);
        int obsClntSock = AcceptTCPConnection(servSock);
        printf("Observer connected\n");
        while (1) {
            sem_wait(sem_buff);
            if (*shared_pointer > 0) {
                if (send(obsClntSock, shared_buff, *shared_pointer, 0) != *shared_pointer) {
                    exit_error("!!!!Send to observer failed\n");
                }
                *shared_pointer = 0;
            }
            sem_post(sem_buff);
        }
    }
    while (1) {}
}

