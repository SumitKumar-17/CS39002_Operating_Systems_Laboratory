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

int wmain(int waiter_id)
{
   
    print_time(0);
    switch (waiter_id)
    {
    case 0:
        printf("Waiter U is ready\n");
        fflush(stdout);
        break;
    case 1:
        printf("\tWaiter V is ready\n");
        fflush(stdout);
        break;
    case 2:
        printf("\t\tWaiter W is ready\n");
        fflush(stdout);
        break;
    case 3:
        printf("\t\t\tWaiter X is ready\n");
        fflush(stdout);
        break;
    case 4:
        printf("\t\t\t\tWaiter Y is ready\n");
        fflush(stdout);
        break;
    }

    int waiter_offset = waiter_id * 200 + 100;
    struct sembuf waiterPop = {waiter_id, -1, 0};

    int food_ready = waiter_offset + 196;
    int pending_order = waiter_offset + 197;

    int wfront = waiter_offset + 198;
    while (1)
    {
        semop(waiter, &waiterPop, 1);
        Wait(mutex);
        int curr_time = shared_mem[0];

        int food_ready_customer = shared_mem[food_ready];
        if (food_ready_customer > 0)
        {
            shared_mem[food_ready] = 0;
            Signal(mutex);
            print_time(curr_time);
            switch (waiter_id)
            {
            case 0:
                printf("Waiter U: Serving food to Customer %d\n", food_ready_customer);
                fflush(stdout);
                break;
            case 1:
                printf("\tWaiter V: Serving food to Customer %d\n", food_ready_customer);
                fflush(stdout);
                break;
            case 2:
                printf("\t\tWaiter W: Serving food to Customer %d\n", food_ready_customer);
                fflush(stdout);
                break;
            case 3:
                printf("\t\t\tWaiter X: Serving food to Customer %d\n", food_ready_customer);
                fflush(stdout);
                break;
            case 4:
                printf("\t\t\t\tWaiter Y: Serving food to Customer %d\n", food_ready_customer);
                fflush(stdout);
                break;
            }

            struct sembuf custVop = {food_ready_customer - 1, 1, 0};
            semop(customer, &custVop, 1);
        }
        else
        {
            int po_count = shared_mem[pending_order];
            if (po_count > 0)
            {
                int front = shared_mem[wfront];
                int cust_id = shared_mem[waiter_offset + front * 2];
                int cust_cnt = shared_mem[waiter_offset + 1 + front * 2];

                shared_mem[wfront]++;
                shared_mem[pending_order]--;

                Signal(mutex);
                struct sembuf cVop = {cust_id - 1, 1, 0};
                semop(customer, &cVop, 1);

                usleep(1 * 1000 * TIME_SCALE);

                Wait(mutex);
                int new_time = curr_time + 1;
                if (new_time > shared_mem[0])
                {
                    shared_mem[0] = new_time;
                }
                else
                {
                    // printf("time decrement erorr\n");
                }

                print_time(new_time);
                switch (waiter_id)
                {
                case 0:
                    printf("Waiter U: Placing order for Customer %d (count = %d)\n", cust_id, cust_cnt);
                    fflush(stdout);
                    break;
                case 1:
                    printf("\tWaiter V: Placing order for Customer %d (count = %d)\n", cust_id, cust_cnt);
                    fflush(stdout);
                    break;
                case 2:
                    printf("\t\tWaiter W: Placing order for Customer %d (count = %d)\n", cust_id, cust_cnt);
                    fflush(stdout);
                    break;
                case 3:
                    printf("\t\t\tWaiter X: Placing order for Customer %d (count = %d)\n", cust_id, cust_cnt);
                    fflush(stdout);
                    break;
                case 4:
                    printf("\t\t\t\tWaiter Y: Placing order for Customer %d (count = %d)\n", cust_id, cust_cnt);
                    fflush(stdout);
                    break;
                }

                int back = shared_mem[2000];
                shared_mem[1100 + back * 3] = waiter_id;
                shared_mem[1100 + back * 3 + 1] = cust_id;
                shared_mem[1100 + back * 3 + 2] = cust_cnt;
                shared_mem[2000]++;
                shared_mem[3]++;

                curr_time = shared_mem[0];
                if (curr_time > END_TIME)
                {
                    int po_count = shared_mem[pending_order];
                    int food_ready_customer = shared_mem[food_ready];

                    if (po_count <= 0 && food_ready_customer == 0)
                    {
                        print_time(curr_time);
                        switch (waiter_id)
                        {
                        case 0:
                            printf("Waiter U leaving (no more customer to serve)\n");
                            fflush(stdout);
                            break;
                        case 1:
                            printf("\tWaiter V leaving (no more customer to serve)\n");
                            fflush(stdout);
                            break;
                        case 2:
                            printf("\t\tWaiter W leaving (no more customer to serve)\n");
                            fflush(stdout);
                            break;
                        case 3:
                            printf("\t\t\tWaiter X leaving (no more customer to serve)\n");
                            fflush(stdout);
                            break;
                        case 4:
                            printf("\t\t\t\tWaiter Y leaving (no more customer to serve)\n");
                            fflush(stdout);
                            break;
                        }
                        Signal(cook);
                        Signal(mutex);
                        exit(0);
                    }
                    else
                    {
                        // printf("Waiter %c working\n", name_waiter);
                    }
                }

                Signal(mutex);
                Signal(cook);
            }
            else
            {
                curr_time = shared_mem[0];
                if (curr_time > END_TIME)
                {
                    int po_count = shared_mem[pending_order];
                    int food_ready_customer = shared_mem[food_ready];

                    if (po_count <= 0 && food_ready_customer == 0)
                    {
                        print_time(curr_time);
                        switch (waiter_id)
                        {
                        case 0:
                            printf("Waiter U leaving (no more customer to serve)\n");
                            fflush(stdout);
                            break;
                        case 1:
                            printf("\tWaiter V leaving (no more customer to serve)\n");
                            fflush(stdout);
                            break;
                        case 2:
                            printf("\t\tWaiter W leaving (no more customer to serve)\n");
                            fflush(stdout);
                            break;
                        case 3:
                            printf("\t\t\tWaiter X leaving (no more customer to serve)\n");
                            fflush(stdout);
                            break;
                        case 4:
                            printf("\t\t\t\tWaiter Y leaving (no more customer to serve)\n");
                            fflush(stdout);
                            break;
                        }
                        Signal(mutex);
                        exit(0);
                    }
                    else
                    {
                        // printf("Waiter %c working\n", name_waiter);
                    }
                }
                Signal(mutex);
            }
        }
    }
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

    for (int i = 0; i < 5; i++){
        pid_t pid = fork();
        if (pid == 0)
        {
            wmain(i);
            exit(0);
        }
        else if (pid < 0)
        {
            // printf("fork error\n");
        }
    }

    int status;
    for (int i = 0; i < 5; i++)
    {
        wait(&status);
    }
    shmdt(shared_mem);

    shmctl(restaurant_id, IPC_RMID, NULL);
    semctl(mutex, 0, IPC_RMID);
    semctl(cook, 0, IPC_RMID);
    semctl(waiter, 0, IPC_RMID);
    semctl(customer, 0, IPC_RMID);
}