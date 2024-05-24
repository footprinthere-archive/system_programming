#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>

#define N 10

static int count = 0;

void ch_handler(int sig) {
    int child_status;
    pid_t pid;

    while ((pid = waitpid(-1, &child_status, WNOHANG)) > 0) {
        count--;
        printf("signal %d from process %d\n", sig, pid);
    }
}

int main() {
    pid_t pid[N];
    int i, child_status;

    count = N;
    signal(SIGCHLD, ch_handler);    // install

    for (i=0; i<N; i++) {
        if ((pid[i] = fork()) == 0) {
            // child
            sleep(1);
            exit(0);
        }
    }

    // Parent
    while (count > 0) {
        printf("count = %d\n", count);
        pause();
    }

    return 0;
}