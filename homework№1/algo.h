#define BUFFER_SIZE 5000

#define NUMBER_OF_FILES 2 + 1
#define FORK_ERROR -1
#define FORK_SUCCESS 0

//
// Основной алгоритм решение задачи описан в корневом markdown:
// homework1/report.md
//

void exit_error(char *error_string) {
    printf("%s", error_string);
    exit(-1);
}

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
