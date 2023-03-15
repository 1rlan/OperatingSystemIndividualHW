#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include "../algo.h"

#define readpipe "/tmp/read"
#define writepipe "/tmp/write"

int main(int argc, char **argv) {
    if (argc != NUMBER_OF_FILES) {                                                  // Проверка на валидное кол-во
        exit_error("Incorrect number of parameters. Enter two files. \n");          // Входных аргументов
    }
    pid_t chpid1;
    
    if ((chpid1 = fork()) == FORK_ERROR) {                                          // Порождение и проверка валидности процесса
        exit_error("Unable to create the first clide. \n");
    } else if (chpid1 == FORK_SUCCESS) {
        pid_t chpid2;
        if ((chpid2 = fork()) == FORK_ERROR) {                                      // Порождение и проверка валидности процесса
            exit_error("Unable to create the first clide. \n");
        } else if (chpid2 == FORK_SUCCESS) {                               // READING
            int file = open(argv[1], O_RDONLY);                                     // Открытие файла
            char str[BUFFER_SIZE + 8];
            ssize_t read_bytes = read(file, str, BUFFER_SIZE);                      // Чтение строки
            str[read_bytes] = '\0';
 
            umask(0);                                                               // Сброс битов прав
            mknod(readpipe, S_IFIFO | 0666, 0);
            
            int fp = open(readpipe, O_WRONLY | O_CREAT);                            // Открытие именованного канала
            write(fp, str, sizeof(str));                                            // Передача в канал
            close(file);                                                            // Закрытие файла
            close(fp);                                                              // Закрытие именованного канала
        } else {                                                           // STRINGS ACTION
            char str[BUFFER_SIZE + 1];                                              // Входная строка
            char ans1[256] = {0}, ans2[256] = {0};                                  // Потенциальные строки разности
            
            umask(0);                                                               // Сброс битов прав
            mknod(readpipe, S_IFIFO | 0666, 0);                                     // Создание временного файла

            int fp = open(readpipe, O_RDONLY);                                      // Открытие именованного канала

            read(fp, str, BUFFER_SIZE);                                             // Чтение строки из канала
            close(fp);                                                              // Закрытие именованного канала

            subtracting(str, ans1, ans2);                                           // Поиск разности

            umask(0);                                                               // Сброс битов прав
            mknod(writepipe, S_IFIFO | 0666, 0);                                    // Создание временного файла
            fp = open(writepipe, O_WRONLY | O_CREAT);
            
            write(fp, ans1, sizeof(ans1));                                          //  Запись ответов в канал
            write(fp, ans2, sizeof(ans2));                                          //
            close(fp);                                                              // Закрытие именованного канала
        }
    } else {                                                               // WRITING
        char ans1[256], ans2[256];
        
        umask(0);                                                                   // Сброс битов прав
        mknod(writepipe, S_IFIFO | 0666, 0);                                        // Создание временного файла
        
        int fp = open(writepipe, O_RDONLY);                                         // Открытие именованного канала
        read(fp, ans1, sizeof(ans1));                                               // Чтение ответов из канала
        read(fp, ans2, sizeof(ans2));                                               //
        close(fp);

        int file = open(argv[2], O_WRONLY | O_CREAT, 0666);                         // Открытие файла

        write(file, ans1, sizeof(ans1));                                            //
        write(file, "\n", 1);                                                       // Запись в файл
        write(file, ans2, sizeof(ans2));                                            //
        
        close(file);                                                                // Закрытие файла
    }
    unlink(readpipe);                                                               // Разлинковочка
    unlink(writepipe);                                                              //
    return 0;
}
