#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <fcntl.h>
#include <semaphore.h>
#include <time.h>

// В этом файле много интересного :))
#include "../tools.h"

// Семафоры и данные
hive *shared_hive;
int sem_hive, shmid, semid;

void clean() {
    semctl(sem_hive, 1, IPC_RMID, 0);
    shmdt(shared_hive);
    shmctl(shmid, IPC_RMID, 0);
}

// Команда прерывания
void handle_sigint(int sig) {
    clean();
    exit(0);
}

// Работа пчелы
void bee_process(hive *hive, int sem) {
    struct sembuf sem_op = {0, -1, 0}; // ожидаем семафор
    struct sembuf sem_op2 = {0, 1, 0}; // освобождаем семафор
    
    // Имитация постоянной работы
    while (true) {
        // Закрытие семафоры, обработка уменьшения улья
        semop(sem, &sem_op, 1);
        if (hive->bee_in != 0) {
            --hive->bee_in;
        }
        semop(sem, &sem_op2, 1);
        
        // Имитация поиска меда
        float sleep_time = (float)(rand() % 1501 + 500) / 1000.0;
        usleep(sleep_time * 1000000);
        
        // Закрытие семафоры, складывание меда если есть место
        semop(sem, &sem_op, 1);
        if (hive->honey < 30) {
            ++hive->honey;
            printf("Bee has added one honey. Total honey: %d\n", hive->honey);
        }
        // Возвращение в улей
        ++hive->bee_in;
        
        // Открытие семафора
        semop(sem, &sem_op2, 1);
    }
}

// Работа медведя
void winny_process(hive *shared_hive, int sem) {
    struct sembuf sem_op = {0, -1, 0}; // ожидаем семафор
    struct sembuf sem_op2 = {0, 1, 0}; // освобождаем семафор
    
    // Имитация постоянного поиска меда
    while (true) {
        // Закрытие семафора
        semop(sem, &sem_op, 1);
        
        // Проверка на количество меда
        if (shared_hive->honey >= 15) {
            // Если пчел мало - воруем мед
            if (shared_hive->bee_in < 3) {
                printf("In hive now only %d bee. Winny has taken all honey. Total honey taken: %d\n", shared_hive->bee_in, shared_hive->honey);
                printf("Winny is running away from bees.\n");
                shared_hive->honey = 0;
            // Иначе - нас кусают
            } else {
                printf("In hive now %d bees. Winny could not take honey. Winnie was stung\n", shared_hive->bee_in);
                // Имитация лечения от укуса
                float sleep_time = (float)(rand() % 1501 + 500) / 1000.0;
                usleep(sleep_time * 1000000);
            }
        }
        
        // Открытие семафора
        semop(sem, &sem_op2, 1);
    }
}


int main(int argc, char **argv) {
    // Обработка сигнала SIGINT через метод handle_sigint
    signal(SIGINT, handle_sigint);
    
    // Сброс сида генерации
    srand(time(NULL));

    // Проверка входных данных
    if (argc != 2) {
        exit_error("Incorrect number of parameters. Enter N number \n");
    }
    if (atoi(argv[1]) < 3) {
        exit_error("Incorrect number of bees.\n");
    }
    // Считывание кол-ва пчел
    int N = atoi(argv[1]);

    // Создание разделяемой памяти
    if ((shmid = shmget(SHM_KEY, sizeof(hive), IPC_CREAT | 0666)) == -1) {
        exit_error("Smth wrong with shmget call");
    }
    // Линковка разделяемой памяти с адресом пространства
    shared_hive = (hive *) shmat(shmid, NULL, 0);

    // Создание семафора
    if ((semid = semget(SEM_KEY, 1, IPC_CREAT | 0666)) == -1) {
        exit_error("Smth wrong with semget call");
    }

    // Установка начального значения семафора
    if (semctl(semid, 0, SETVAL, 0) == -1) {
        exit_error("Smth wrong with semctl call");
    }
    
    // Запись значений в улей
    shared_hive->bee_in = N;
    shared_hive->honey = 0; 
    
    // Запуск процесса медведя
    pid_t winny_pid = fork();
    if (winny_pid == -1) {
        exit_error("Smth wrong with fork call");
    } else if (winny_pid == 0) {
        winny_process(shared_hive, sem_hive);
        exit(0);
    }
    
    // Запуск процессов пчел
    for (int i = 0; i < atoi(argv[1]); ++i) {
        pid_t bee_pid = fork();
        if (bee_pid == -1) {
            exit_error("Smth wrong with fork call");
        } else if (bee_pid == 0) {
            bee_process(shared_hive, sem_hive);
            exit(0);
        }
    }
    
    // Дожидаемся всех процессов
    process_wait(N + 1);

    // Освобождаем память
    clean();
    return 0;
}
