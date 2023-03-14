#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define BUFFER_SIZE 5000

#define NUMBER_OF_FILES 2 + 1
#define FORK_ERROR -1
#define FORK_SUCCESS 0

void exit_error(char *error_string) {
    printf("%s", error_string);
    exit(-1);
}

//
// Основной алгоритм решение задачи описан в корневом markdown:
// homework1/report.md
//

int is_delimeter(char character) {
    return character == ' ' || character == '\t' || character == '\n';
}

int find_delimeter(char *string) {
    int delimeter = 0;

    while (!is_delimeter(string[delimeter])) {
        delimeter++;
    }
    
    return delimeter;
}

void subtracting(char *string, char *ans1, char *ans2) {
    int delimeter = find_delimeter(string);
    
    int dict1[256] = {0};
    int dict2[256] = {0};

    for (int i = 0; i < delimeter; i++) {
        ++dict1[(int) string[i]];
    }
        
    for (int i = delimeter + 1; string[i] != '\0'; i++) {
        ++dict2[(int) string[i]];
    }
        
    int k1 = 0, k2 = 0;
    for (int i = 0; i < 256; i++) {
        if (dict1[i] > 0 && dict2[i] == 0) {
            ans1[k1++] = (char) i;
        }
            
        
        if (dict2[i] > 0 && dict1[i] == 0) {
            ans2[k2++] = (char) i;
        }
    }
    
    ans1[k1] = '\0';
    ans2[k2] = '\0';
}

//
//


int main(int argc, char **argv) {
    if (argc != NUMBER_OF_FILES) {                                                  // Проверка на валидное кол-во
        exit_error("Incorrect number of parameters. Enter two files. \n");          // Входных аргументов
    }
    
    int fd1[2];
    pid_t chpid1;
    pipe(fd1);                                                                      // Создание канала
    if ((chpid1 = fork()) == FORK_ERROR) {                                          // Порождение и проверка валидности процесса
        exit_error("Unable to create the first clide. \n");
    } else if (chpid1 == FORK_SUCCESS) {
        int fd2[2];
        pid_t chpid2;
        pipe(fd2);                                                                  // Создание канала
        if ((chpid2 = fork()) == FORK_ERROR) {                                      // Порождение и проверка валидности процесса
            exit_error("Unable to create the first clide. \n");
        } else if (chpid2 == FORK_SUCCESS) {                              // READING
            int file = open(argv[1], O_RDONLY);                                     // Открытие файла
            char str[BUFFER_SIZE + 1];
            ssize_t read_bytes = read(file, str, BUFFER_SIZE);                      // Чтение строки
            str[read_bytes] = '\0';

            close(fd2[0]);
            write(fd2[1], str, read_bytes + 1);                                     // Передача в канал
            close(file);                                                            // Закрытие файла
        } else {                                                           // STRINGS ACTION
            close(fd2[1]);
            
            char str[BUFFER_SIZE + 1];                                              // Входная
            int len = read(fd2[0], str, BUFFER_SIZE);                               // Чтение входной строки из канала
            char ans1[256] = {0}, ans2[256] = {0};                                  // Потенциальные строки разности
            subtracting(str, ans1, ans2);                                           // Поиск разности

            close(fd1[0]);
            write(fd1[1], ans1, sizeof(ans1));                                      //  Запись ответов в канал
            write(fd1[1], ans2, sizeof(ans2));                                      //
        }
    } else {                                                               // WRITING
        char ans1[256], ans2[256];
        close(fd1[1]);
        read(fd1[0], ans1, sizeof(ans1));                                           // Чтение ответов из канала
        read(fd1[0], ans2, sizeof(ans2));                                           //

        
        int file = open(argv[2], O_WRONLY | O_CREAT, 0666);                         // Открытие файла
        
        write(file, ans1, sizeof(ans1));                                            //
        write(file, "\n", 1);                                                       // Запись в файл
        write(file, ans2, sizeof(ans2));                                            //
        
        close(file);                                                                // Закрытие файла
    }

    return 0;
}
