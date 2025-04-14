#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/shm.h>
#include <string.h>

#define END_TIME 240
#define TIME_SCALE 100
#define Wait(s) semop(s, &pop, 1)
#define Signal(s) semop(s, &vop, 1)

struct sembuf pop = {0, -1, 0};
struct sembuf vop = {0, 1, 0};

void print_time(int time)
{
    int hour = 11 + time / 60;
    int minute = time % 60;
    char day_night[3] = "am";

    if (hour >= 12)
    {
        if (hour > 12)
            hour -= 12;
        strcpy(day_night, "pm");
    }
    printf("[%02d:%02d %s] ", hour, minute, day_night);
    fflush(stdout);
}

int *shared_mem;
int restaurant_id;
int mutex;
int cook;
int waiter;
int customer;

void cmain(char cook_id)
{
    if (cook_id == 'C')
    {
        print_time(0);
        printf("Cook %c Ready\n", cook_id);
        fflush(stdout);
    }
    else
    {
        print_time(0);
        printf("\tCook %c Ready\n", cook_id);
        fflush(stdout);
    }

    while (1)
    {
        Wait(cook);
        Wait(mutex);

        int time = shared_mem[0];
        int front = shared_mem[1999];
        int waiter_id = shared_mem[1100 + front * 3];
        int customer_id = shared_mem[1100 + front * 3 + 1];
        int persons = shared_mem[1100 + front * 3 + 2];
        shared_mem[1999]++;
        shared_mem[3]--;
        Signal(mutex);
        if (cook_id == 'C')
        {
            print_time(time);
            printf("Cook %c: Preparing order (Waiter %c, Customer %d, Count %d)\n", cook_id, 'U' + waiter_id, customer_id, persons);
            fflush(stdout);
        }
        else
        {
            print_time(time);
            printf("\tCook %c: Preparing order (Waiter %c, Customer %d, Count %d)\n", cook_id, 'U' + waiter_id, customer_id, persons);
            fflush(stdout);
        }

        usleep(5 * 1000 * TIME_SCALE * persons);
        Wait(mutex);
        int new_time = time + (5 * persons);
        if (new_time < shared_mem[0])
        {
            // printf("time decrement erorr\n");
        }

        shared_mem[0] = new_time;
        int waiter_offset = waiter_id * 200 + 100;
        int food_ready = waiter_offset + 196;
        shared_mem[food_ready] = customer_id;

        if (cook_id == 'C')
        {
            print_time(new_time);
            printf("Cook %c: Prepared order (Waiter %c, Customer %d, Count %d)\n",cook_id, 'U' + waiter_id, customer_id, persons);
            fflush(stdout);
        }
        else
        {
            print_time(new_time);
            printf("\tCook %c: Prepared order (Waiter %c, Customer %d, Count %d)\n",cook_id, 'U' + waiter_id, customer_id, persons);
            fflush(stdout);
        }

        if (shared_mem[0] > END_TIME && shared_mem[1999] >= shared_mem[2000])
        {
            if (shared_mem[3] <= 0)
            {
                if (cook_id == 'C')
                {
                    print_time(shared_mem[0]);
                    printf("Cook C: leaving\n");
                    fflush(stdout);
                }
                else
                {
                    print_time(shared_mem[0]);
                    printf("\tCook D: leaving\n");
                    fflush(stdout);
                }
                Signal(mutex);
                struct sembuf wup = {waiter_id, 1, 0};
                semop(waiter, &wup, 1);
                exit(0);
            }
        }

        Signal(mutex);
        struct sembuf wup = {waiter_id, 1, 0};
        semop(waiter, &wup, 1);
    }
}

int main()
{
    key_t sm_key = ftok("/", 101);
    restaurant_id = shmget(sm_key, 2100 * sizeof(int), IPC_CREAT | 0777);
    shared_mem = (int *)shmat(restaurant_id, NULL, 0);
    memset(shared_mem, 0, 2100 * sizeof(int));

    shared_mem[0] = 0;
    shared_mem[1] = 10;
    shared_mem[2] = 0;
    shared_mem[3] = 0;

    key_t sem_key = ftok("/", 2);
    mutex = semget(sem_key, 1, IPC_CREAT | 0777);
    semctl(mutex, 0, SETVAL, 1);

    key_t cook_key_sem = ftok("/", 3);
    cook = semget(cook_key_sem, 1, IPC_CREAT | 0777);
    semctl(cook, 0, SETVAL, 0);

    key_t waiter_sem_key = ftok("/", 4);
    waiter = semget(waiter_sem_key, 5, IPC_CREAT | 0777);
    for (int i = 0; i < 5; i++)semctl(waiter, i, SETVAL, 0);

    key_t customer_sem_key = ftok("/", 5);
    customer = semget(customer_sem_key, 200, IPC_CREAT | 0777);
    for (int i = 0; i < 200; i++)semctl(customer, i, SETVAL, 0);

    for (int i = 0; i < 2; i++)
    {
        pid_t pid = fork();
        if (pid == 0)
        {
            if (i == 0)
            {
                cmain('C');
            }
            else
            {
                cmain('D');
            }
            exit(0);
        }
        else if (pid < 0)
        {
            // printf("fork error\n");
        }
    }

    int status;
    for (int i = 0; i < 2; i++)
    {
        wait(&status);
    }

    shmdt(shared_mem);

    shmctl(restaurant_id, IPC_RMID, NULL);
    semctl(mutex, 0, IPC_RMID);
    semctl(cook, 0, IPC_RMID);
    semctl(waiter, 0, IPC_RMID);
    semctl(customer, 0, IPC_RMID);

    return 0;
}