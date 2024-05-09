#include <fcntl.h>//open()
#include <stdlib.h>
#include <stdio.h>// perror(), fflush()
#include <sys/mman.h>// mmap(), MAP_FAILED, PORT_READ, PORT_WRITE, MAP_SHARED
#include <unistd.h>//close()

#define NLOOPS 1000
#define SIZE sizeof(long)

static int fd1[2], fd2[2];

void tell_parent()
{
    close(fd2[0]);
    if (write(fd2[1], "c", 1) != 1)
    {
        exit(1);
    }
}

void wait_parent()
{
    char c;
    close(fd1[1]);
    if (read(fd1[0], &c, 1) != 1)
    {
        exit(1);
    }
    if (c != 'p')
    {
        exit(1);
    }
}

void tell_child()
{
    close(fd1[0]);
    if (write(fd1[1], "p", 1) != 1)
    {
        exit(1);
    }
}

void wait_child()
{
    char c;
    close(fd2[1]);
    if (read(fd2[0], &c, 1) != 1)
    {
        exit(1);
    }
    if (c != 'c')
    {
        exit(1);
    }
}

int update(long* lptr)
{
    long tmp = *lptr;
    (*lptr)++;
    printf("after updating, current data is %d\n", *lptr);
    return tmp;
}

int main()
{
    int fd;
    pid_t pid;
    void *area;
    if ((fd = open("/dev/zero", O_RDWR)) < 0)
    {
        perror("open error\n");
        exit(1);
    }

    if ((area = mmap(0, SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0)) == MAP_FAILED)
    {
        perror("mmap error\n");
        exit(1);
    }
    close(fd);
    if (pipe(fd1) < 0)
    {
        perror("pipe error");
        exit(1);
    }
    if (pipe(fd2) < 0)
    {
        perror("pipe error");
        exit(1);
    }    
    if ((pid = fork()) < 0)
    {
        perror("fork error\n");
        exit(1);
    }
    else if (pid > 0)/*parent*/
    {
        int i;
        for (i = 0; i < NLOOPS; i += 2)
        {
            if (update((long *)area) != i) {
                perror("update error");
                exit(1);
            }
            tell_child();
            wait_child();
        }
    }
    else/*child*/ 
    {
        int i;
        for (i = 1; i < NLOOPS + 1; i += 2)
        {
            wait_parent();
            if (update((long *)area) != i)
            {
                perror("update error");
                exit(1);
            }
            tell_parent();
        }
    }
    exit(0);
}