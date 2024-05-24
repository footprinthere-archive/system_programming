#include "headers.h"

int main() {
    pid_t chpid;

    if ((chpid = fork()) == 0) {
        // child
        sleep(0);
        printf("child start\n");
        sleep(1);
        printf("child end\n");
        exit(0);
    }
    else {
        // parent
        sleep(0);
        printf("parent start\n");
        sleep(0);
        printf("parent end\n");
        exit(0);
    }
}