#include <stdio.h>

void emptyline_test() {
    char *line = "";
    printf("|%c|\n", *line);
    printf("%d\n", *line);
}

void getline_test() {
    char *filename = "traces/yi.trace";
    FILE *file = fopen(filename, "r");
    char *line;
    size_t len = 0;
    int size;

    while ((size = getline(&line, &len, file)) > 0) {
        printf("%d %ld\n", size, len);
        printf("%s\n", line);
    }
}

int main() {
    getline_test();
    return 0;
}