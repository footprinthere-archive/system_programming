#include "headers.h"

static int childexec = 0;

void handler(int sig) {
    printf("handler called\n");
    childexec = 1;
}

int main() {
    int fd[2];
    char buf[2];
    int nbytes;
    int rand;

    pipe(fd);
    if (write(fd[1], "0", 1) < 0) {
        perror("first write error");
    }

    if (fork() != 0) {
        // parent
        printf("parent start\n");
        sleep(0.5);
        nbytes = read(fd[0], buf, 2);
        if (nbytes < 0) {
            perror("read error");
        }
        printf("nbytes = %d\n", nbytes);
        exit(0);
    }

    else {
        // child
        printf("child start\n");
        rand = getpid() & 0x1;

        if (rand) {
            if (write(fd[1], "0", 2) < 0) {
                perror("write error");
            }
            printf("*** write in pipe\n");
        }

        printf("child exit\n");
        exit(0);
    }
}