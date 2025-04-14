#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <limits.h>
#include <stdbool.h>

#define MEMORY_SIZE (64 * 1024 * 1024)
#define PAGE_SIZE (4 * 1024)
#define TOTAL_FRAMES (MEMORY_SIZE / PAGE_SIZE)
#define OS_FRAMES (16 * 1024 * 1024 / PAGE_SIZE)
#define USER_FRAMES (TOTAL_FRAMES - OS_FRAMES)
#define ESSENTIAL_PAGES 10
#define PAGE_TABLE_SIZE 2048
#define VALID_BIT_MASK 0x8000

typedef struct
{
    int process_id;
    int array_size;
    int *search_indices;
    int total_searches;
    int current_search;
    bool is_active;
    unsigned short *page_table;
    int allocated_frames;
} _process;

void initializeSimulation(const char *filename);
bool simulate_BS(_process *process);
int getPageOfElement(int element_index);
void loadProcessEssentialPages(_process *process);
void swapOutProcess(_process *process);
void swapInProcess(_process *process);
void cleanup();

_process *processes = NULL;
int num_processes;
int num_searches;

int *free_frames;
int num_free_frames;

int *ready_queue;
int ready_queue_front;
int ready_queue_rear;
int ready_queue_count;

int *swapped_processes;
int swapped_proc_front;
int swapped_proc_rear;
int swapped_proc_count;

int total_page_accesses = 0;
int total_page_faults = 0;
int total_swaps = 0;
int min_active_processes = INT_MAX;
int active_processes;

int main()
{
    initializeSimulation("search.txt");

    while (ready_queue_count > 0)
    {
        int pid = ready_queue[ready_queue_front];
        ready_queue_front = (ready_queue_front + 1) % num_processes;
        ready_queue_count--;

        _process *current_process = &processes[pid];

#ifdef VERBOSE
        printf("\tSearch %d by Process %d\n", current_process->current_search + 1, pid);
#endif

        if (simulate_BS(current_process))
        {
            current_process->current_search++;

            if (current_process->current_search >= current_process->total_searches)
            {
                swapOutProcess(current_process);
                current_process->is_active = false;
                active_processes--;

                if (swapped_proc_count > 0)
                {
                    int waiting_pid = swapped_processes[swapped_proc_front];
                    swapped_proc_front = (swapped_proc_front + 1) % num_processes;
                    swapped_proc_count--;

                    printf("+++ Swapping in process %4d", waiting_pid);

                    swapInProcess(&processes[waiting_pid]);
                    processes[waiting_pid].is_active = true;

                    ready_queue_front = (ready_queue_front - 1 + num_processes) % num_processes;
                    ready_queue[ready_queue_front] = waiting_pid;
                    ready_queue_count++;

                    printf("  [%d active processes]\n", active_processes);
                }
            }
            else
            {
                ready_queue[ready_queue_rear] = pid;
                ready_queue_rear = (ready_queue_rear + 1) % num_processes;
                ready_queue_count++;
            }
        }
        else
        {
            printf("+++ Swapping out process %4d", pid);

            swapOutProcess(current_process);
            current_process->is_active = false;

            swapped_processes[swapped_proc_rear] = pid;
            swapped_proc_rear = (swapped_proc_rear + 1) % num_processes;
            swapped_proc_count++;

            total_swaps++;
            active_processes--;
            if (active_processes < min_active_processes)
            {
                min_active_processes = active_processes;
            }

            printf("  [%d active processes]\n", active_processes);
        }
    }

    printf("+++ Page access summary\n");
    printf("\tTotal number of page accesses  =  %d\n", total_page_accesses);
    printf("\tTotal number of page faults    =  %d\n", total_page_faults);
    printf("\tTotal number of swaps          =  %d\n", total_swaps);
    printf("\tDegree of multiprogramming     =  %d\n", min_active_processes);

    cleanup();
    return 0;
}

void initializeSimulation(const char *filename)
{
    FILE *file = fopen(filename, "r");
    if (!file)
    {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    fscanf(file, "%d %d", &num_processes, &num_searches);
    processes = (_process *)malloc(num_processes * sizeof(_process));
    if (!processes)
    {
        perror("Memory allocation failed");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < num_processes; i++)
    {
        processes[i].process_id = i;
        processes[i].current_search = 0;
        processes[i].total_searches = num_searches;
        processes[i].is_active = false;
        processes[i].allocated_frames = 0;

        fscanf(file, "%d", &processes[i].array_size);

        processes[i].search_indices = (int *)malloc(num_searches * sizeof(int));
        if (!processes[i].search_indices)
        {
            perror("Memory allocation failed");
            exit(EXIT_FAILURE);
        }

        for (int j = 0; j < num_searches; j++)
        {
            fscanf(file, "%d", &processes[i].search_indices[j]);
        }

        processes[i].page_table = (unsigned short *)calloc(PAGE_TABLE_SIZE, sizeof(unsigned short));
        if (!processes[i].page_table)
        {
            perror("Memory allocation failed");
            exit(EXIT_FAILURE);
        }
    }

    fclose(file);
    printf("+++ Simulation data read from file\n");

    num_free_frames = USER_FRAMES;
    free_frames = (int *)malloc(USER_FRAMES * sizeof(int));
    if (!free_frames)
    {
        perror("Memory allocation failed");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < USER_FRAMES; i++)
    {
        free_frames[i] = i;
    }

    ready_queue = (int *)malloc(num_processes * sizeof(int));
    if (!ready_queue)
    {
        perror("Memory allocation failed");
        exit(EXIT_FAILURE);
    }

    ready_queue_front = 0;
    ready_queue_rear = 0;
    ready_queue_count = 0;

    swapped_processes = (int *)malloc(num_processes * sizeof(int));
    if (!swapped_processes)
    {
        perror("Memory allocation failed");
        exit(EXIT_FAILURE);
    }

    swapped_proc_front = 0;
    swapped_proc_rear = 0;
    swapped_proc_count = 0;

    for (int i = 0; i < num_processes; i++)
    {
        loadProcessEssentialPages(&processes[i]);
        processes[i].is_active = true;

        ready_queue[ready_queue_rear] = i;
        ready_queue_rear = (ready_queue_rear + 1) % num_processes;
        ready_queue_count++;
    }

    active_processes = num_processes;
    min_active_processes = num_processes;

    printf("+++ Kernel data initialized\n");
}

void loadProcessEssentialPages(_process *process)
{
    for (int i = 0; i < ESSENTIAL_PAGES; i++)
    {
        if (num_free_frames > 0)
        {
            int frame = free_frames[--num_free_frames];
            process->page_table[i] = VALID_BIT_MASK | frame;
            process->allocated_frames++;
        }
        else
        {
            fprintf(stderr, "Error: Not enough frames for essential pages of process %d\n", process->process_id);
            exit(EXIT_FAILURE);
        }
    }
}

bool simulate_BS(_process *process)
{
    int search_key = process->search_indices[process->current_search];
    int left = 0;
    int right = process->array_size - 1;

    while (left < right)
    {
        int mid = (left + right) / 2;

        total_page_accesses++;
        int page_number = getPageOfElement(mid);

        if (!(process->page_table[page_number] & VALID_BIT_MASK))
        {
            total_page_faults++;

            if (num_free_frames == 0)
            {
                return false;
            }

            int frame = free_frames[--num_free_frames];
            process->page_table[page_number] = VALID_BIT_MASK | frame;
            process->allocated_frames++;
        }

        if (search_key <= mid)
        {
            right = mid;
        }
        else
        {
            left = mid + 1;
        }
    }

    return true;
}

int getPageOfElement(int element_index)
{
    return ESSENTIAL_PAGES + (element_index * sizeof(int)) / PAGE_SIZE;
}

void swapOutProcess(_process *process)
{
    for (int i = 0; i < PAGE_TABLE_SIZE; i++)
    {
        if (process->page_table[i] & VALID_BIT_MASK)
        {
            int frame = process->page_table[i] & ~VALID_BIT_MASK;
            free_frames[num_free_frames++] = frame;
            process->page_table[i] &= ~VALID_BIT_MASK;
        }
    }

    process->allocated_frames = 0;
}

void swapInProcess(_process *process)
{
    for (int i = 0; i < ESSENTIAL_PAGES; i++)
    {
        if (num_free_frames > 0)
        {
            int frame = free_frames[--num_free_frames];
            process->page_table[i] = VALID_BIT_MASK | frame;
            process->allocated_frames++;
        }
        else
        {
            fprintf(stderr, "Error: No free frames available for essential pages during swap-in\n");
            exit(EXIT_FAILURE);
        }
    }

    active_processes++;
    // total_swaps++;
}

void cleanup()
{
    for (int i = 0; i < num_processes; i++)
    {
        free(processes[i].search_indices);
        free(processes[i].page_table);
    }

    free(processes);
    free(free_frames);
    free(ready_queue);
    free(swapped_processes);
}