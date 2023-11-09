#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/wait.h>
#include <string.h>
#include <sys/time.h>

#define SHM_KEY 1234
#define SEM_KEY 5678

// Buffer data structure
typedef struct
{
    int buffer[10]; // Example buffer size of 10
    int in;         // Index for next item produced
    int out;        // Index for next item consumed
} shared_data;

// Union for semaphore operations
union semun
{
    int val;
    struct semid_ds *buf;
    unsigned short *array;
};

// Semaphore operation function
void sem_op(int semid, int semnum, int val)
{
    struct sembuf op;
    op.sem_num = semnum;
    op.sem_op = val;
    op.sem_flg = 0;
    if (semop(semid, &op, 1) == -1)
    {
        perror("semop");
        exit(1);
    }
}

// Producer function
void producer(int semid, shared_data *shm, int X)
{
    for (int i = 0; i < X; i++)
    {
        sem_op(semid, 0, -1);         // Wait on empty
        shm->buffer[shm->in] = i;     // Produce item
        shm->in = (shm->in + 1) % 10; // Circular buffer
        sem_op(semid, 1, 1);          // Signal full
    }
}

// Consumer function
void consumer(int semid, shared_data *shm, int X)
{
    for (int i = 0; i < X; i++)
    {
        sem_op(semid, 1, -1);             // Wait on full
        int item = shm->buffer[shm->out]; // Consume item
        shm->out = (shm->out + 1) % 10;   // Circular buffer
        sem_op(semid, 0, 1);              // Signal empty
    }
}

int main(int argc, char *argv[])
{
    if (argc != 6)
    {
        fprintf(stderr, "Usage: %s <B> <P> <C> <X> <N>\n", argv[0]);
        exit(1);
    }

    int B = atoi(argv[1]);
    int P = atoi(argv[2]);
    int C = atoi(argv[3]);
    int X = atoi(argv[4]);
    int N = atoi(argv[5]);

    // Create shared memory
    int shmid = shmget(SHM_KEY, sizeof(shared_data), IPC_CREAT | 0666);
    if (shmid == -1)
    {
        perror("shmget");
        exit(1);
    }

    // Attach shared memory
    shared_data *shmaddr = shmat(shmid, NULL, 0);
    if (shmaddr == (void *)-1)
    {
        perror("shmat");
        exit(1);
    }

    // Initialize shared memory
    shmaddr->in = 0;
    shmaddr->out = 0;

    // Create semaphores
    int semid = semget(SEM_KEY, 2, IPC_CREAT | 0666);
    if (semid == -1)
    {
        perror("semget");
        exit(1);
    }

    // Initialize semaphores
    union semun semopts;
    semopts.val = 10; // Initial value for empty slots
    if (semctl(semid, 0, SETVAL, semopts) == -1)
    {
        perror("semctl");
        exit(1);
    }

    semopts.val = 0; // Initial value for full slots
    if (semctl(semid, 1, SETVAL, semopts) == -1)
    {
        perror("semctl");
        exit(1);
    }

    // Measure execution time
    struct timeval start, end;
    gettimeofday(&start, NULL);

    // Fork producer processes
    for (int i = 0; i < P; i++)
    {
        pid_t pid = fork();
        if (pid == 0)
        {
            producer(semid, shmaddr, X);
            exit(0);
        }
        else if (pid < 0)
        {
            perror("fork");
            exit(1);
        }
    }

    // Fork consumer processes
    for (int i = 0; i < C; i++)
    {
        pid_t pid = fork();
        if (pid == 0)
        {
            consumer(semid, shmaddr, X);
            exit(0);
        }
        else if (pid < 0)
        {
            perror("fork");
            exit(1);
        }
    }

    // Wait for all child processes to finish
    while (wait(NULL) > 0)
        ;

    // Measure end time./paster2 2 1 3 10 1
    gettimeofday(&end, NULL);
    double execution_time = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1e6;
    printf("Execution time: %.2f seconds\n", execution_time);

    // Detach from shared memory
    shmdt(shmaddr);

    // Remove shared memory and semaphores
    shmctl(shmid, IPC_RMID, NULL);
    semctl(semid, 0, IPC_RMID);

    return 0;
}
