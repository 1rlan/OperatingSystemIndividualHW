#define DATA "/data"
#define SEMAPHORE "/semaphore"

#define SEM_KEY 0x1111
#define SHM_KEY 0x2222

#define true 1

void exit_error(char *error_string) {
    printf("%s", error_string);
    exit(-1);
}

// Структура общих данных
typedef struct {
    int honey;  // Кол-во меда в улье
    int bee_in; // Кол-во пчел в улье
} hive;

void process_wait(int N) {
    for (int i = 0; i < N + 1; ++i) {
        int status;
        pid_t child_pid = wait(&status);
    }
}
