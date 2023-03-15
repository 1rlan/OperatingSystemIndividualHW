#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "../algo.h"


int main(int argc, char **argv) {
    if (argc != NUMBER_OF_FILES) {                                              // Проверка на валидное кол-во
            exit_error("Incorrect number of parameters. Enter two files. \n");  // Входных аргументов
        }
    
    int fd1[2], fd2[2];
    pid_t chpid1;
    
    pipe(fd1);                                                                  // Создание каналов
    pipe(fd2);                                                                  //
    
    if ((chpid1 = fork()) == FORK_ERROR) {                                      // Порождение и проверка валидности процесса
        exit_error("Unable to create the first clide. \n");
    } else if (chpid1 == FORK_SUCCESS) {                            // READING
        int file = open(argv[1], O_RDONLY);                                     // Открытие файла
        char str[BUFFER_SIZE + 8];
        ssize_t read_bytes = read(file, str, BUFFER_SIZE);                      // Чтение строки
        str[read_bytes] = '\0';
        
        close(fd2[0]);
        write(fd2[1], str, sizeof(str));                                        // Передача в канал
        close(file);
        
        
        int status = 0;
        chpid1 = wait(&status);                                                 // Ожидание завершения процесса
        
                                                                    // WRITING
        char ans1[256], ans2[256];
        close(fd1[1]);
        read(fd1[0], ans1, sizeof(ans1));                                       // Чтение ответов из канала
        read(fd1[0], ans2, sizeof(ans2));                                       //
        
        file = open(argv[2], O_WRONLY | O_CREAT, 0666);                         // Открытие файла

        write(file, ans1, sizeof(ans1));                                        //
        write(file, "\n", 1);                                                   // Запись в файл
        write(file, ans2, sizeof(ans2));                                        //
        
        close(file);                                                            // Закрытие файла
    } else {                                                        // STRINGS ACTION
        close(fd2[1]);
        
        char str[BUFFER_SIZE + 1];                                              // Входная
        int len = read(fd2[0], str, BUFFER_SIZE);                               // Чтение входной строки из канала
        char ans1[256] = {0}, ans2[256] = {0};                                  // Потенциальные строки разности
        subtracting(str, ans1, ans2);                                           // Поиск разности

        close(fd1[0]);
        write(fd1[1], ans1, sizeof(ans1));                                      //  Запись ответов в канал
        write(fd1[1], ans2, sizeof(ans2));                                      //
    }

    return 0;
}
