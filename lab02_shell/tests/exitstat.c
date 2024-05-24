#include "headers.h"

int main() {
    pid_t chpid;
    int status;

    if ((chpid = fork()) == 0) {
        // child
        printf("child\n");
        kill(getpid(), SIGINT);
        printf("signal sent\n");    // never reached
    }
    else {
        // parent
        printf("parent\n");
        waitpid(chpid, &status, 0);
        printf("status %d\n", status);
        if (WIFEXITED(status)) {
            printf("exit: %d\n", WEXITSTATUS(status));
        }
        if (WIFSIGNALED(status)) {
            printf("signaled: %d\n", WTERMSIG(status));
        }
    }

    return 0;
}