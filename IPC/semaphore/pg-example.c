#include <semaphore.h>//sem_open(), sem_t
#include <stdio.h>//snprintf()
#include <sys/stat.h>
#include <fcntl.h>//O_CREATE
#include <errno.h>//error, EEXIST, EACCES, EINTR
#include <stdlib.h>//malloc
#include <stdbool.h>//bool, false
#include <time.h> //timespec
#include <unistd.h>//fork(), sleep()
#include <sys/wait.h>//waitpid()
#include <sys/shm.h>

#define SEM_NAME_SIZE 64
#define IPC_PROTECTION (0600)
#define CACHE_LINE_SIZE 128
#define MAX_SEMA_SIZE 1024
#define N_SEMAS 64
/* Support for dynamic adjustment of spins_per_delay */
#define MIN_SPINS_PER_DELAY 10
#define DEFAULT_SPINS_PER_DELAY  100
#define NUM_DELAYS			1000
#define MIN_DELAY_USEC		1000L
#define MAX_DELAY_USEC		1000000L
#define MAX_SPINS_PER_DELAY 1000
#define SHM_SIZE 4
#define SHM_MODE 0600/*user read/write*/

#define TAY_ACQUIRE_SPINLOCK(lock)	try_acquire_spinlock_sema(lock)
#define S_LOCK(lock) (TAY_ACQUIRE_SPINLOCK(lock) ? s_lock(lock) : 0 )
#define S_UNLOCK(lock) s_unlock_sema(lock)
#define TAS_SPIN(lock)	TAY_ACQUIRE_SPINLOCK(lock)

/*
 * Max
 *		Return the maximum of two numbers.
 */
#define Max(x, y)		((x) > (y) ? (x) : (y))

/*
 * Min
 *		Return the minimum of two numbers.
 */
#define Min(x, y)		((x) < (y) ? (x) : (y))

typedef union _MySemaphore
{
	sem_t		sem;
	char		pad[CACHE_LINE_SIZE];
} MySemaphore;

typedef int slock_t;

typedef struct _SpinDelayStatus
{
	int			spins;/* 自旋的次数，即执行TAS_SPIN(lock)为真的次数 */
	int			delays;/* 执行my_sleep 的次数 */
	int			cur_delay;/* 当前要sleep 的时间，单位是us */
	// const char *file;
	// int			line;
	// const char *func;
} SpinDelayStatus;


static sem_t **my_sem_pointers;	/* keep track of created semaphores */

//Static variables are initialized to 0 by default
static int next_sem_key;
static int cur_sem_num;
MySemaphore** spinlock_sema_array;
static int	spins_per_delay = DEFAULT_SPINS_PER_DELAY;

static int res_counter;

/*
 *   This is an example of a named semaphore 
 *
 * 
 */

static sem_t* posix_semaphore_create()
{
    int sem_key;
    char sem_name[SEM_NAME_SIZE];
    sem_t* my_sem;
    for (;;)
    {
        sem_key = next_sem_key++;
        snprintf(sem_name, sizeof(sem_name), "/mysem-%d", sem_key);
        my_sem = sem_open(sem_name, O_CREAT | O_EXCL, (mode_t) IPC_PROTECTION, (unsigned) 1);
        if (my_sem != (sem_t*) (-1))
        {
            break;//create succ!
        }
        if (errno == EEXIST || errno == EACCES || errno == EINTR)
        {
            continue;//occur collision
        }
    }
    sem_unlink(sem_name);
    return my_sem;
}

MySemaphore* semaphore_create(void)
{
    MySemaphore* sema;
    sem_t* new_sem;
    if (cur_sem_num > MAX_SEMA_SIZE)
    {
        perror("exceed max semaphore size.");
        exit(1);
    }
    new_sem = posix_semaphore_create();
    my_sem_pointers[cur_sem_num] = new_sem;
    sema = (MySemaphore*) new_sem;
    cur_sem_num++;
    return sema;
}

void posix_semaphore_lock(MySemaphore* sema)
{
    int err_status;
    do
    {
        err_status = sem_wait(&sema->sem);
    } while (err_status < 0 && errno == EINTR);

    if (err_status < 0)
    {
        perror("sem_wait failed\n");
        exit(1);
    }
}

void posix_semaphore_unlock(MySemaphore* sema)
{
    int err_status;
    do
    {
        err_status = sem_post(&sema->sem);
    } while (err_status < 0 && errno == EINTR);
    
    if (err_status < 0)
    {
        perror("sem_post failed\n");
        exit(1);
    }
}

void reserve_semaphores()
{
    my_sem_pointers = (sem_t**) malloc(MAX_SEMA_SIZE * sizeof(sem_t*));
    if (my_sem_pointers == NULL)
    {
        perror("out of memory");
        exit(1);
    }
}

void spinlock_sema_init()
{
    MySemaphore** spin_semas;
    int i;
    spin_semas = (MySemaphore**) malloc(N_SEMAS * sizeof(MySemaphore*));
    for (i = 0; i < N_SEMAS; i++)
    {
        spin_semas[i] = semaphore_create();//创建信号量
    }
    spinlock_sema_array = spin_semas;
}

static inline void s_check_valid(int  lockndx)
{
    if (lockndx <= 0 || lockndx > N_SEMAS)
    {
        perror("invalid spinlock number");
        exit(1);
    }
}

void s_init_lock_sema(slock_t* lock)
{
    static u_int32_t counter = 0;//用static声明没法在不同进程中使用它的特性, 所以每个进程的counter均为0
    u_int32_t offset = 1;
    u_int32_t idx;
    idx = (counter++ % N_SEMAS) + offset;
    s_check_valid(idx);
    *lock = idx;
}
bool semaphore_try_lock(MySemaphore* sema)
{
    int err_status;
    do
    {
        err_status = sem_trywait(&sema->sem);
    } while (err_status < 0 && errno == EINTR);

    if (err_status < 0)
    {
        if (errno == EAGAIN || errno == EDEADLK)
        {
            return false;
        }
        else
        {
            perror("sem_trywait failed\n");
            exit(1);
        }
    }
    return true;
}

int  try_acquire_spinlock_sema(volatile slock_t* lock)
{
    int lockndx = *lock;
    s_check_valid(lockndx);
    return !semaphore_try_lock(spinlock_sema_array[lockndx - 1]);
}


static inline void init_spin_delay(SpinDelayStatus *status)
{
	status->spins = 0;
	status->delays = 0;
	status->cur_delay = 0;
}

void my_sleep(long micro_sec)
{
	if (micro_sec > 0)
	{
		struct timespec delay;
		delay.tv_sec = micro_sec / 1000000L;
		delay.tv_nsec = (micro_sec % 1000000L) * 1000;
		(void) nanosleep(&delay, NULL);    
    }
}
//每执行一次代表自旋一次, 表现为 ++(status->spins)
void perform_spin_delay(SpinDelayStatus* status)
{
    if (++(status->spins) >= spins_per_delay)
    {
        if (++(status->delays) > NUM_DELAYS)
        {
            perror("stuck spinlock\n");
            exit(1);
        }
        if (status->cur_delay == 0)//首次自旋时间在初始化时设置为0
        {
            status->cur_delay = MIN_DELAY_USEC;//首次自旋的时间
        }
        my_sleep(status->cur_delay);//sleep
		/* increase delay by a random fraction between 1X and 2X */
		status->cur_delay += (int) (status->cur_delay *
									2 + 0.5);//下一次自旋的时间
		/* wrap back to minimum delay when max is exceeded */
		if (status->cur_delay > MAX_DELAY_USEC)
			status->cur_delay = MIN_DELAY_USEC;

		status->spins = 0;
    }
}

void finish_spin_delay(SpinDelayStatus *status)
{
	if (status->cur_delay == 0)
	{
		/* we never had to delay */
		if (spins_per_delay < MAX_SPINS_PER_DELAY)
			spins_per_delay = Min(spins_per_delay + 100, MAX_SPINS_PER_DELAY);
	}
	else
	{
		if (spins_per_delay > MIN_SPINS_PER_DELAY)
			spins_per_delay = Max(spins_per_delay - 1, MIN_SPINS_PER_DELAY);
	}
}

int s_lock(volatile slock_t* lock)
{
    SpinDelayStatus delay_status;
    init_spin_delay(&delay_status);

    while (TAS_SPIN(lock))
    {
        perform_spin_delay(&delay_status);//睡眠
    }
    finish_spin_delay(&delay_status);
    return delay_status.delays;
}

void s_unlock_sema(volatile slock_t* lock)
{
    int lockndx = *lock;
    s_check_valid(lockndx);
    posix_semaphore_unlock(spinlock_sema_array[lockndx - 1]);
}

void semaphore_pointer_array_create()
{
    my_sem_pointers = (sem_t**) malloc(MAX_SEMA_SIZE * sizeof(sem_t*));
}

int main()
{
    /*
     * test case:
     *    (1) fork 100 process
     *    (2) use spinlock to get data
    */
    int i;
    pid_t pid;
    int* shmptr;
    int shmid;
    semaphore_pointer_array_create();
    spinlock_sema_init();//Create 64 spin locks
    if ((shmid = shmget(IPC_PRIVATE, SHM_SIZE, SHM_MODE)) < 0)/*create share memory*/
    {
        perror("shmget error\n");
        exit(1);
    }
    if ((shmptr = (int*)shmat(shmid, 0, 0)) == (void*)(-1))/*attach to current process*/
    {
        perror("shmat error\n");
        exit(1);
    }  
    for (i = 0; i < 100; i++)
    {
        if ((pid = fork()) < 0)
        {
            perror("fork failed\n");
            exit(1);
        }
        else if (pid > 0)/*parent*/
        {
            // int status;
            // int child_pid = waitpid(pid, &status, 0);
            // if (child_pid == 0)
            // {
            //     perror("waitpid failed\n");
            //     exit(1);
            // }
        }
        else/*child*/
        {
            /*
            * 需要注意的是我们的自旋锁默认有64个, 而创建的进程数为100, 所以一定有锁的竞争,
            * 并且自旋锁利用的是信号量, 信号量的值为1
            */
            slock_t lock;
            srand(time(0));
            sleep(rand() % 5 + 1);// 睡眠随机时间, 打乱获取自旋锁的时间
            s_init_lock_sema(&lock);//虽然我们创建了很多信号量但测试自旋锁, 只用lock = 1的信号量
            S_LOCK(&lock);//尝试加锁
            printf("I'm the %d subprocess and get the spinlock. The lock id = %d\n", i, lock);
            (*shmptr)++;
            printf("current res_counter = %d\n", *shmptr);
            S_UNLOCK(&lock);
            exit(0);//必须有这一句, 不然子进程会继续执行for循环导致最后生成的进程数>100
        }
    }
    sleep(20);//睡眠200秒, 等待所有进程创建完成并开是抢占自旋锁
    if (shmctl(shmid, IPC_RMID, 0) < 0)/*remove share memory*/
    {
        perror("shmctl error\n");
        exit(1);
    }    
}