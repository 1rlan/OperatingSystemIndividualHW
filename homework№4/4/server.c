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
    sem_unlink(SEM_HIVE);
    // Удаление разделенной памяти
    munmap(shared_hive, sizeof(hive));
    shm_unlink(HIVE);
    close(servSock);
}

// Команда прерывания
void handle_exit() {
    // Освобождение данных.
    clean();
    exit(0);
}

int CreateUDPServerSocket(unsigned short port) {
    int sock;
    struct sockaddr_in echoServAddr;

    if ((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
        exit_error("socket() failed");
    }

    memset(&echoServAddr, 0, sizeof(echoServAddr));
    echoServAddr.sin_family = AF_INET;
    echoServAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    echoServAddr.sin_port = htons(port);

    if (bind(sock, (struct sockaddr *) &echoServAddr, sizeof(echoServAddr)) < 0) {
        exit_error("bind() failed\n");
    }

    return sock;
}

void HandleUDPClient(struct sockaddr_in clntAddr, unsigned int clntLen) {
    char rcvBuff[1];
    int recvMsgSize;
    char c;
    
    if ((recvMsgSize = recvfrom(servSock, rcvBuff, 1, 0, (struct sockaddr *) &clntAddr, &clntLen)) < 0) {
        exit_error("recvfrom() failed");
    }
    
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

            // Отправка ответа клиенту
            c = BEE_IN;
            if (sendto(servSock, &c, 1, 0, (struct sockaddr *) &clntAddr, sizeof(clntAddr)) != 1) {
                exit_error("sendto() bee failed\n");
            }
            sem_post(sem_hive);
        } else if (rcvBuff[0] == BEE_OUT) {
            // Закрытие семафоры, берем мед если он есть
            sem_wait(sem_hive);
            if (shared_hive->bee_in != 0) {
                --shared_hive->bee_in;
            }
            // Отправка ответа клиенту
            c = BEE_OUT;
            if (sendto(servSock, &c, 1, 0, (struct sockaddr *) &clntAddr, sizeof(clntAddr)) != 1) {
                exit_error("sendto() bee failed\n");
            }
            sem_post(sem_hive);
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
                    if (sendto(servSock, &c, 1, 0, (struct sockaddr *) &clntAddr, sizeof(clntAddr)) != 1) {
                        exit_error("sendto() failed\n");
                    }
                } else {
                    // Иначе - нас кусают
                    printf("In hive now %d bees. Winny could not take honey. Winnie was stung\n", shared_hive->bee_in);
                    c = STUNG;
                    if (sendto(servSock, &c, 1, 0, (struct sockaddr *) &clntAddr, sizeof(clntAddr)) != 1) {
                        exit_error("sendto() failed\n");
                    }
                }
            } else {
                c = EMPTY;
                if (sendto(servSock, &c, 1, 0, (struct sockaddr *) &clntAddr, sizeof(clntAddr)) != 1) {
                    exit_error("sendto() failed\n");
                }
            }
            sem_post(sem_hive);
        } else {
            printf("Wrong data\n");
        }
    }
}

void ProcessMain() {
    struct sockaddr_in echoClntAddr;
    unsigned int clntLen;
    clntLen = sizeof(echoClntAddr);

    while (1) {
        HandleUDPClient(echoClntAddr, clntLen);
    }
}

int main(int argc, char *argv[]) {
    // Обработка сигнала SIGINT через метод handle_exit
    prev_server = signal(SIGINT, handle_exit);

    // Сброс сида генерации
    srand(time(NULL));

    // Открываем разделяему память
    if ((shm_fd = shm_open(HIVE, O_CREAT | O_RDWR, 0666)) == -1) {
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
    if ((sem_hive = sem_open(SEM_HIVE, O_CREAT, 0666, 1)) == SEM_FAILED) {
        exit_error("Smth wrong with sem_open call \n");
    }

    if (argc != 3) {
        fprintf(stderr, "Usage:  %s <SERVER PORT> <N>\n", argv[0]);
        exit(1);
    }

    // Запись значений в улей
    int N_BEES = atoi(argv[2]);
    shared_hive->bee_in = N_BEES;
    shared_hive->honey = 0;
    sem_post(sem_hive);
    unsigned short echoServPort;
    pid_t processID;
    unsigned int processCt;

    echoServPort = atoi(argv[1]);

    servSock = CreateUDPServerSocket(echoServPort);

    for (processCt = 0; processCt < N_BEES + 1; processCt++) {
        if ((processID = fork()) < 0) {
            exit_error("fork() failed\n");
        } else if (processID == 0) {
            signal(SIGINT, prev_server);
            ProcessMain();
            exit(0);
        }
    }

    while (1) {}
}
