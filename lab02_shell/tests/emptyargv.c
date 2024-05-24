#include <stdio.h>

int main() {
    char *argv[] = {"Hi", "Hello"};
    char **pt = argv;

    printf("%d\n", sizeof(argv)/sizeof(argv[1]));
    printf("%d\n", sizeof(pt)/sizeof(pt[0]));

    return 0;
}