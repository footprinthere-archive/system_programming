#include "headers.h"

#define N 3

static int count = 0;

void ch_handler(int sig) {
    int child_status;
    pid_t pid;

    while ((pid = waitpid(-1, &child_status, WNOHANG)) > 0) {
        count--;
        printf("Process %d ended with status %d\n", pid, WEXITSTATUS(child_status));
    }
}

int main() {
    pid_t fgpid;
    pid_t bgpid[N];
    pid_t wpid;
    int i;
    int fd;
    char* buf = "abc\n";

    count = N;
    signal(SIGCHLD, ch_handler);

    // Background jobs
    for (i=0; i<N; i++) {
        if ((bgpid[i] = fork()) == 0) {
            // child: execve() part
            sleep(5);

            fd = open("empty.txt", O_WRONLY|O_APPEND);
            if (fd < 0) {
                perror("file open error");
            }
            write(fd, buf, strlen(buf));
            close(fd);

            exit(0);    // child ended
        }
    }

    // Foreground job
    if ((fgpid = fork()) == 0) {
        // child
        for (i=0; i<5; i++) {
            sleep(0.5);
            printf("foreground: %d\n", i+1);
        }
        printf("foreground process ended\n");
        exit(0);
    }
    else {
        // parent
        while ((wpid = waitpid(fgpid, NULL, WNOHANG)) >= 0) {
            printf("wpid = %d\n", wpid);
            sleep(0);
        }
        printf("wpid = %d\n", wpid);
        printf("Parent end\n");
    }

    // End of shell (wait until all background jobs end)
    while (count > 0) {
        printf("count = %d\n", count);
        pause();
    }
    printf("All ended\n");

    return 0;
}