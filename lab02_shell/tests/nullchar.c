#include "headers.h"

int main() {
    int fd[2];
    char buf[4];
    int nbytes;

    pipe(fd);
    write(fd[1], "1", 1);
    
    printf("pid %d\n", getpid());
    if (getpid() & 0x1)
        write(fd[1], "2", 1);

    nbytes = read(fd[0], buf, 2);
    printf("%d: %s|\n", nbytes, buf);

    return 0;
}