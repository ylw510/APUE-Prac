#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <stdio.h>

static int fd1[2], fd2[2];

void tell_parent()
{
    if (write(fd2[1], "c", 1) != 1)
    {
        exit(1);
    }
    printf("Already send parent from child\n");
}

void wait_parent()
{
    char c;
    if (read(fd1[0], &c, 1) != 1)
    {
        exit(1);
    }
    if (c != 'p')
    {
        exit(1);
    }
    printf("Already get from parent \n");
}

void tell_child()
{
    if (write(fd1[1], "p", 1) != 1)
    {
        exit(1);
    }
    printf("Already send child from parent \n");
}

void wait_child()
{
    char c;
    if (read(fd2[0], &c, 1) != 1)
    {
        exit(1);
    }
    if (c != 'c')
    {
        exit(1);
    }
    printf("Already get from child \n");
}

int main()
{

    pid_t pid;
    if (pipe(fd1) < 0)
    {
        exit(1);
    }
    if (pipe(fd2) < 0)
    {
        exit(1);
    }
    if ((pid = fork()) < 0)
    {
        exit(1);
    }
    else if (pid > 0)/*parent*/
    {
        tell_child();
        wait_child();
    }
    else/*child*/
    {
        wait_parent();
        tell_parent();
    }


}