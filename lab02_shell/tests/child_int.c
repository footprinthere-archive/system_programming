#include "headers.h"

void int_handler(int sig) {
    printf("SIGINT\n");
    exit(0);
}

void chld_handler(int sig) {
    printf("SIGCHLD\n");
}

int main() {
    pid_t pid;

    signal(SIGINT, int_handler);
    signal(SIGCHLD, chld_handler);

    if ((pid = fork()) != 0) {
        printf("Parent - pid %d pgid %d\n", getpid(), getpgid(getpid()));
        waitpid(pid, NULL, 0);
    }
    else {
        // child
        setpgid(0, 0);              // new group
        printf("Child - pid %d pgid %d\n", getpid(), getpgid(getpid()));
        sleep(2);
        printf("KILL\n");
        kill(getpid(), SIGINT);     // send to itself
    }

    printf("Parent end\n");
}