#include <stdlib.h> // NULL, malloc()
#include <stdio.h>// perror()
#include <sys/shm.h>// shmget(), IPC_PRIVATE, 
#define ARRAY_SIZE 40000
#define MALLOC_SIZE 100000
#define SHM_SIZE 100000
#define SHM_MODE 0600/*user read/write*/

char array[ARRAY_SIZE];// uninitial

int main()
{
    int shmid;
    char* ptr = NULL;
    char* shmptr;
    printf("Array from %p to %p\n", (void*)&array[0], (void*)&array[ARRAY_SIZE]);
    printf("stack around %p\n", (void*)&shmid);
    if ((ptr = malloc(MALLOC_SIZE)) == NULL)
    {
        perror("malloc error\n");
        exit(1);
    }
    printf("malloced from %p to %p\n", (void*)ptr, (void*)ptr + MALLOC_SIZE);
    free(ptr);
    ptr = NULL;
    if ((shmid = shmget(IPC_PRIVATE, SHM_SIZE, SHM_MODE)) < 0)/*create share memory*/
    {
        perror("shmget error\n");
        exit(1);
    }
    if ((shmptr = shmat(shmid, 0, 0)) == (void*)(-1))/*attach to current process*/
    {
        perror("shmat error\n");
        exit(1);
    }
    printf("share memory from %p to %p\n", (void*)shmptr, (void*)shmptr + SHM_SIZE);
    if (shmctl(shmid, IPC_RMID, 0) < 0)/*remove share memory*/
    {
        perror("shmctl error\n");
        exit(1);
    }
    exit(0);
}