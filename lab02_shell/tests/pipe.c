#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

int main() {
    int pipefd[2];
    pid_t chpid;
    char wbuf[2] = "1";
    char rbuf[2];

    if (pipe(pipefd) == -1) {
        printf("pipe error\n");
        return 1;
    }

    write(pipefd[1], "0", 2);

    chpid = fork();
    
    // Child
    if (chpid == 0) {
        close(pipefd[0]);   // close reading end
        printf("child pid %d\n", getpid());
        if (getpid() & 0x1) {
            fflush(pipefd[1]);
            write(pipefd[1], wbuf, 2);
        }
        else
            sleep(1);
        printf("child end\n");
        return 0;
    }

    // Parent
    else {
        close(pipefd[1]);   // close writing end
        if (read(pipefd[0], rbuf, 2) == 0) {
            printf("nothing\n");
        }

        printf("rbuf %s\n", rbuf);
    }

    return 0;
}