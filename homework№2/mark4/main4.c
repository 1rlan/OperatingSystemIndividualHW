#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <semaphore.h>
#include <time.h>

// В этом файле много интересного :))
#include "../tools.h"

// Семафоры и данные
hive *shared_hive;
sem_t *sem_hive;
int shm_fd;

void clean() {
    // Удаление семафора
    close(shm_fd);
    sem_close(sem_hive);
    sem_unlink(SEMAPHORE);
    // Удаление разделенной памяти
    munmap(shared_hive, sizeof(hive));
    shm_unlink(DATA);
}

// Команда прерывания
void handle_exit(int sig) {
    // Освобождение данных.
    clean();
    exit(0);
}

// Работа пчелы
void bee_process(hive *hive, sem_t *sem) {
    // Имитация постоянной работы
    while (true) {
        // Закрытие семафоры, обработка уменьшения улья
        sem_wait(sem);
        if (hive->bee_in != 0) {
            --hive->bee_in;
        }
        sem_post(sem);
        
        // Имитация поиска меда
        float sleep_time = (float)(rand() % 1501 + 500) / 1000.0;
        usleep(sleep_time * 1000000);
        
        // Закрытие семафоры, складывание меда если есть место
        sem_wait(sem);
        if (hive->honey < 30) {
            ++hive->honey;
            printf("Bee has added one honey. Total honey: %d\n", hive->honey);
        }
        // Возвращение в улей
        ++hive->bee_in;
        
        // Открытие семафора
        sem_post(sem);
    }
}

// Работа медведя
void winny_process(hive *shared_hive, sem_t *sem) {
    // Имитация постоянного поиска меда
    while (true) {
        // Закрытие семафора
        sem_wait(sem);
        
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
        sem_post(sem);
    }
}


int main(int argc, char **argv) {
    // Обработка сигнала SIGINT через метод handle_sigint
    signal(SIGINT, handle_exit);
    
    // Сброс сида генерации
    srand(time(NULL));

    // Проверка входных данных
    if (argc != 2) {
        exit_error("Incorrect number of parameters. Enter N number \n");
    }
    if (atoi(argv[1]) < 3) {
        exit_error("Incorrect number of bees. \n");
    }
    
    // Считывание кол-ва пчел
    int N = atoi(argv[1]);
    
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
    shared_hive->bee_in = N;
    shared_hive->honey = 0;
    
    // Запуск процесса медведя
    pid_t winny_pid = fork();
    if (winny_pid == -1) {
        exit_error("Smth wrong with fork call \n");
    } else if (winny_pid == 0) {
        winny_process(shared_hive, sem_hive);
        exit(0);
    }
    
    // Запуск процессов пчел
    for (int i = 0; i < atoi(argv[1]); ++i) {
        pid_t bee_pid = fork();
        if (bee_pid == -1) {
            exit_error("Smth wrong with fork call \n");
        } else if (bee_pid == 0) {
            bee_process(shared_hive, sem_hive);
            exit(0);
        }
    }
    
    // Заверщение процессов N пчел и одного медведя
    process_wait(N + 1);

    // Освобождаем память
    clean();
    return 0;
}
