#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#define MAX_BUF_SIZE 512


int main()
{
    pid_t pid;
    int fd[2];
    char buf[MAX_BUF_SIZE];
    if(pipe(fd) < 0) 
    {
        //err_log("Fail to create pipe", ERR_CREATE_PILE);
    }
    else if ((pid = fork()) < 0) 
    {
        //err_log("Fail to create process", ERR_CREATE_PROCESS);
    }
    else if (pid > 0)/*parent process*/
    {
        printf("========== I'm parent =========\n");
        printf("writing .......\n");
        close(fd[0]);
        write(fd[1], "hello world\n", 12);
        printf("========== parent write over ======\n");
    }
    else /*child process*/
    {
        printf("========== I'm child =========\n");
        int read_size = 0;
        close(fd[1]);
        read_size = read(fd[0], buf, MAX_BUF_SIZE);
        write(STDOUT_FILENO, buf, read_size);
        printf("========== child read over ======\n");
    }
    exit(0);
}