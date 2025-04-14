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

int restaurant_id;
int *shared_mem;
int mutex;
int cook;
int waiter;
int customer;

int cmain(int customer_id, int customer_arrv_time, int persons)
{
    print_time(customer_arrv_time);
    printf("Customer %d arrives (count = %d)\n", customer_id, persons);
    fflush(stdout);

    Wait(mutex);
    if (customer_arrv_time > shared_mem[0])
    {
        shared_mem[0] = customer_arrv_time;
    }

    if (shared_mem[0] > END_TIME)
    {
        print_time(customer_arrv_time);
        printf("\t\t\t\t\tCustomer %d leaves ( late arrival )\n", customer_id);
        fflush(stdout);
        Signal(mutex);
        exit(0);
    }

    if (shared_mem[1] == 0)
    {
        print_time(customer_arrv_time);
        printf("\t\t\t\t\tCustomer %d leaves ( no empty table )\n", customer_id);
        fflush(stdout);
        Signal(mutex);
        exit(0);
    }

    shared_mem[1]--;
    int waiter_id = shared_mem[2];
    shared_mem[2] = (shared_mem[2] + 1) % 5;

    int waiter_offset = waiter_id * 200 + 100;
    int back = shared_mem[waiter_offset + 199];
    shared_mem[waiter_offset + back * 2] = customer_id;
    shared_mem[waiter_offset + back * 2 + 1] = persons;
    shared_mem[waiter_offset + 197]++;
    shared_mem[waiter_offset + 199]++;

    Signal(mutex);
    struct sembuf waiterVop = {waiter_id, 1, 0};
    semop(waiter, &waiterVop, 1);

    struct sembuf cVop = {customer_id - 1, -1, 0};
    semop(customer, &cVop, 1);

    usleep(1000 * TIME_SCALE);
    Wait(mutex);
    customer_arrv_time += 1;
    if (customer_arrv_time > shared_mem[0])
    {
        shared_mem[0] = customer_arrv_time;
    }
    print_time(customer_arrv_time);
    printf("\tCustomer %d order placed to waiter %c\n", customer_id, waiter_id + 'U');
    fflush(stdout);
    Signal(mutex);
    struct sembuf custPop = {customer_id - 1, -1, 0};
    semop(customer, &custPop, 1);

    Wait(mutex);
    int new_time = shared_mem[0];

    Signal(mutex);
    print_time(new_time);
    printf("\t\tCustomer %d gets food [Waiting time = %d]\n", customer_id, new_time - customer_arrv_time);
    fflush(stdout);
    
    usleep(30 * 1000 * TIME_SCALE);

    Wait(mutex);
    new_time += 30;
    if (new_time > shared_mem[0])
    {
        shared_mem[0] = new_time;
    }
    else
    {
        // printf("time decrement erorr\n");
    }

    shared_mem[1]++;
    print_time(new_time);
    printf("\t\t\tCustomer %d finishes eating and leaves\n", customer_id);
    fflush(stdout);
    
    Signal(mutex);

    exit(0);
}

int main()
{
    key_t sm_key = ftok("/", 101);
    restaurant_id = shmget(sm_key, 2100 * sizeof(int), IPC_CREAT | 0777);
    shared_mem = (int *)shmat(restaurant_id, NULL, 0);
    
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

    FILE *fp = fopen("customers.txt", "r");
    if (fp == NULL)
    {
        printf("File not found\n");
        return 0;
    }
    int customer_id, customer_arrv_time, persons;
    int prev_arival = 0;
    int childs = 0;
    while (1)
    {
        int res = fscanf(fp, "%d %d %d", &customer_id, &customer_arrv_time, &persons);
        if (res != 3)
        {
            break;
        }
        if (customer_id <= 0)
        {
            break;
        }
        int wait_time = customer_arrv_time - prev_arival;
        if (wait_time > 0)
        {
            usleep(wait_time * 1000 * TIME_SCALE);
        }
        prev_arival = customer_arrv_time;

        pid_t pid = fork();
        if (pid == 0)
        {
            cmain(customer_id, customer_arrv_time, persons);
            exit(0);
        }
        childs++;
    }

    fclose(fp);

    int status;
    for (int i = 0; i < childs; i++)
    {
        wait(&status);
    }

    // for (int i = 0; i < customer_count; i++) {
    //     int status;
    //     waitpid(pid[i], &status, 0);
    //     // Optional: Add debug output
    //     // printf("Child process %d (customer %d) exited with status %d\n", pid[i], i+1, WEXITSTATUS(status));
    // }
    // sem_wait_operation(semid, 0);
    // shared_mem[99] = 1;
    // printf("[%02d:%02d %s] All customers processed. Sending termination signals.\n",
    //     11 + shared_mem[0]/60, shared_mem[0]%60,
    //     (11 + shared_mem[0]/60 >= 12) ? "pm" : "am");
    // sem_signal_operation(semid, 1);
    // for(int i=0; i<5; i++)sem_signal_operation(semid, 2+i);
    // for(int i=0; i<MAX_CUSTOMERS; i++)sem_signal_operation(semid, 7+i);
    // sem_signal_operation(semid, 0);
    // usleep(500000);  // 500ms

    shmctl(restaurant_id, IPC_RMID, NULL);
    semctl(mutex, 0, IPC_RMID, NULL);
    semctl(cook, 0, IPC_RMID, NULL);
    semctl(waiter, 0, IPC_RMID, NULL);
    semctl(customer, 0, IPC_RMID, NULL);

    shmdt(shared_mem);
    return 0;
}