#include <sys/mman.h>
#include <fcntl.h>
#include <semaphore.h>
#include <time.h>

#include "../tools.h"

hive *shared_hive;
sem_t *sem_hive;
int shm_fd;
int servSock;

void clean() {
    // Удаление семафора
    close(shm_fd);
    sem_close(sem_hive);
    sem_unlink(SEMAPHORE);
    // Удаление разделенной памяти
    munmap(shared_hive, sizeof(hive));
    shm_unlink(DATA);
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
    char c;
    if ((recvMsgSize = recv(clntSocket, rcvBuff, 1, 0)) < 0)
        exit_error("recv() failed");
    if (recvMsgSize == 1) {
        if (rcvBuff[0] == BEE_IN) {
            // Закрытие семафоры, складывание меда если есть место
            sem_wait(sem_hive);
            if (shared_hive->honey < 30) {
                ++shared_hive->honey;
                printf("Bee has added one honey. Total honey: %d\n", shared_hive->honey);
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
                    printf("In hive now only %d bee. Winny has taken all honey. Total honey taken: %d\n",
                           shared_hive->bee_in,
                           shared_hive->honey);
                    printf("Winny is running away from bees.\n");
                    shared_hive->honey = 0;
                    c = STOLE;
                    if (send(clntSocket, &c, 1, 0) != 1) {
                        exit_error("send() failed\n");
                    }
                } else {
                    // Иначе - нас кусают
                    printf("In hive now %d bees. Winny could not take honey. Winnie was stung\n", shared_hive->bee_in);
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
            printf("Wrong data\n");
        }
    }
}

void ProcessMain(int childServSock) {
    int clntSock = AcceptTCPConnection(childServSock);
    printf("with child process: %d\n", (unsigned int) getpid());
    while (1) {
        HandleTCPClient(clntSock);
    }
}

int main(int argc, char *argv[]) {
    // Обработка сигнала SIGINT через метод handle_sigint
    prev_server = signal(SIGINT, handle_exit);

    // Сброс сида генерации
    srand(time(NULL));

    // Открываем разделяему память
    if ((shm_fd = shm_open(DATA, O_CREAT | O_RDWR, 0666)) == -1) {
        exit_error("Smth wrong with shm_open call \n");
    }

    // Устанавливаем размер память равный размеру hive
    if (ftruncate(shm_fd, sizeof(hive)) == -1) {
        exit_error("Smth wrong with ftruncate call \n");
    }

    // Перекладываем разделяемую память на виртуальное пространство процесса
    if ((shared_hive = mmap(NULL, sizeof(hive), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0)) == MAP_FAILED) {
        exit_error("Smth wrong with mmap call \n");
    }

    // Создание семафора
    if ((sem_hive = sem_open(SEMAPHORE, O_CREAT, 0666, 1)) == SEM_FAILED) {
        exit_error("Smth wrong with sem_open call \n");
    }

    // Запись значений в улей
    shared_hive->bee_in = N_BEES;
    shared_hive->honey = 0;
    sem_post(sem_hive);
    unsigned short echoServPort;
    pid_t processID;
    unsigned int processCt;

    if (argc != 2) {
        fprintf(stderr, "Usage:  %s <SERVER PORT>\n", argv[0]);
        exit(1);
    }

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
    while (1) {}
}

