#include "headers.h"

int main() {
    pid_t pid;
    int status;

    pid = fork();
    if (pid == 0) {
        // child
        printf("child\n");
        kill(getpid(), SIGTSTP);

        printf("after signal\n");
    }
    else {
        //parent;
        printf("parent\n");
        if (waitpid(pid, &status, WUNTRACED) < 0) {
            perror("wait error");
        }

        if (WIFSTOPPED(status)) {
            printf("stopped\n");
            printf("%d\n", WSTOPSIG(status));
        }
        printf("parent end\n");
        exit(0);
    }
}