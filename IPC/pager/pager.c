#include <define.h>
#include <unistd.h>/*pipe(), dup2(), execl() */
#include <sys/types.h>/*pid_t*/
#include <stdlib.h>/*exit(), getenv()*/
#include <stdio.h> /*fopen(), printf(), */
#include <string.h> /*strlen()*/
#include <sys/wait.h> /* waitpid()*/

//maybe like:  echo 'afdd' | more
/*
    相当于把文件的内容写进管道，再通过dup2将标准输入绑定到管道的读端。
*/

int main(int argc, char* argv[])
{
    pid_t pid;
    int fd[2];
    char buf[MAX_BUF_SIZE];
    FILE* fp;
    char* pager = NULL;
    int n;
    if (argc != 2) 
    {
        printf("Usage: a.out <path name>\n");
        return 0;
    }
    if ((fp = fopen(argv[1], "r")) == NULL)
    {
        exit(1);
    }
    if (pipe(fd) < 0) 
    {
        exit(1);
    }
    else if ((pid = fork()) < 0)
    {
        exit(1);

    }
    else if (pid > 0)/*parent process*/
    {
        close(fd[0]);
        
        while ((fgets(buf, MAX_BUF_SIZE, fp) != NULL))
        {
            n = strlen(buf);//real read size
            if (write(fd[1], buf, n) != n)
            {
                exit(1);
            }
        }
        if (ferror(fp) != 0)
        {            
            exit(1);
        }
        close(fd[1]);
        if (waitpid(pid, NULL, 0) < 0)
        {
            exit(1);
        }
        exit(0);//parent process exit
    }
    else/*child process*/
    {
        close(fd[1]);
        char* argv0 = NULL;
        if (fd[0] != STDIN_FILENO)
        {
            if (dup2(fd[0], STDIN_FILENO) != STDIN_FILENO)
            {
                exit(1);
            }
            close(fd[0]);//don't need fd[0] after dup2
        }
        if ((pager = getenv("PAGER")) == NULL)
        {
            pager = DEFAULT_PAGER;
        }
        if ((argv0 = strrchr(pager, '/')) == NULL)
        {
            argv0++;//step rightmost '/'
        }
        else
        {
            argv0 = pager;
        }
        /*https://www.coder.work/article/2104105
          This article explains that 'more' takes data from standard input(namely STDIN_FILENO) by default.
        */
        if (execl(pager, argv0, (char*)(0)) < 0)
        {
            exit(1);
        }
    }
    exit(0);//child process exit
}