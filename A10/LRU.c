#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <limits.h>
#include <time.h>
#include <stdbool.h>

#define MEMORY_SIZE (64 * 1024 * 1024)
#define PAGE_SIZE (4 * 1024)
#define TOTAL_FRAMES (MEMORY_SIZE / PAGE_SIZE)
#define OS_FRAMES (16 * 1024 * 1024 / PAGE_SIZE)
#define USER_FRAMES (TOTAL_FRAMES - OS_FRAMES)
#define MAX_FRAME_NUMBER (USER_FRAMES - 1)
#define ESSENTIAL_PAGES 10
#define PAGE_TABLE_SIZE 2048
#define VALID_BIT_MASK 0x8000
#define REF_BIT_MASK 0x4000
#define FRAME_MASK 0x3FFF
#define NFFMIN 1000

typedef struct{
    int process_id;
    int array_size;
    int *search_indices;
    int total_searches;
    int current_search;
    bool is_active;
    unsigned short *page_table;
    unsigned short *history;
    int allocated_frames;

    int page_accesses;
    int page_faults;
    int replacements;
    int attempt1;
    int attempt2;
    int attempt3;
    int attempt4;
} _process;

typedef struct{
    int frame_number;
    int last_owner;
    int last_page;
} _frame_info;

void initializeSimulation(const char *filename);
bool simulate_BS(_process *process);
int getPageOfElement(int element_index);
void loadProcessEssentialPages(_process *process);
int displayFrameNumber(int internal_frame_number);
void updatePageHistory(_process *process);
void releaseProcessFrames(_process *process);
void cleanup();

_process *processes = NULL;
int num_processes;
int num_searches;

_frame_info *free_frames;
int num_free_frames;

int *ready_queue;
int ready_queue_front;
int ready_queue_rear;
int ready_queue_count;

int total_page_accesses = 0;
int total_page_faults = 0;
int total_replacements = 0;
int total_attempt1 = 0;
int total_attempt2 = 0;
int total_attempt3 = 0;
int total_attempt4 = 0;

int displayFrameNumber(int internal_frame_number){
    return MAX_FRAME_NUMBER - internal_frame_number;
}

int main(){
    srand((unsigned int)time(NULL));
    initializeSimulation("search.txt");

    while (ready_queue_count > 0){
        int pid = ready_queue[ready_queue_front];
        ready_queue_front = (ready_queue_front + 1) % num_processes;
        ready_queue_count--;

        _process *current_process = &processes[pid];

#ifdef VERBOSE
        printf("+++ Process %d: Search %d\n", pid, current_process->current_search + 1);
#endif

        if (simulate_BS(current_process)){
            updatePageHistory(current_process);

            current_process->current_search++;

            if (current_process->current_search >= current_process->total_searches){
                releaseProcessFrames(current_process);
                current_process->is_active = false;
            }
            else{
                ready_queue[ready_queue_rear] = pid;
                ready_queue_rear = (ready_queue_rear + 1) % num_processes;
                ready_queue_count++;
            }
        }
    }

    printf("+++ Page access summary\n");
    printf("\tPID     Accesses        Faults         Replacements                        Attempts\n");

    for (int i = 0; i < num_processes; i++){
        _process *proc = &processes[i];
        printf("%5d %10d %8d (%6.2f%%) %8d (%6.2f%%) %5d + %5d + %5d + %5d (%6.2f%% + %6.2f%% + %6.2f%% + %6.2f%%)\n",
               i, proc->page_accesses, proc->page_faults,
               (float)proc->page_faults * 100 / proc->page_accesses,
               proc->replacements,
               (float)proc->replacements * 100 / proc->page_accesses,
               proc->attempt1, proc->attempt2, proc->attempt3, proc->attempt4,
               proc->replacements > 0 ? (float)proc->attempt1 * 100 / proc->replacements : 0,
               proc->replacements > 0 ? (float)proc->attempt2 * 100 / proc->replacements : 0,
               proc->replacements > 0 ? (float)proc->attempt3 * 100 / proc->replacements : 0,
               proc->replacements > 0 ? (float)proc->attempt4 * 100 / proc->replacements : 0);
    }
    printf("\n");
    printf("%7s %10d %8d (%6.2f%%) %8d (%6.2f%%) %5d + %5d + %5d + %5d (%6.2f%% + %6.2f%% + %6.2f%% + %6.2f%%)\n",
           "Total", total_page_accesses, total_page_faults,
           (float)total_page_faults * 100 / total_page_accesses,
           total_replacements,
           (float)total_replacements * 100 / total_page_accesses,
           total_attempt1, total_attempt2, total_attempt3, total_attempt4,
           total_replacements > 0 ? (float)total_attempt1 * 100 / total_replacements : 0,
           total_replacements > 0 ? (float)total_attempt2 * 100 / total_replacements : 0,
           total_replacements > 0 ? (float)total_attempt3 * 100 / total_replacements : 0,
           total_replacements > 0 ? (float)total_attempt4 * 100 / total_replacements : 0);

    cleanup();
    return 0;
}

void initializeSimulation(const char *filename){
    FILE *file = fopen(filename, "r");
    if (!file){
        perror("Error opening file");
        exit(1);
    }

    fscanf(file, "%d %d", &num_processes, &num_searches);
    processes = (_process *)malloc(num_processes * sizeof(_process));
    if (!processes){
        perror("Memory allocation failed");
        exit(1);
    }

    for (int i = 0; i < num_processes; i++){
        processes[i].process_id = i;
        processes[i].current_search = 0;
        processes[i].total_searches = num_searches;
        processes[i].is_active = false;
        processes[i].allocated_frames = 0;

        processes[i].page_accesses = 0;
        processes[i].page_faults = 0;
        processes[i].replacements = 0;
        processes[i].attempt1 = 0;
        processes[i].attempt2 = 0;
        processes[i].attempt3 = 0;
        processes[i].attempt4 = 0;

        fscanf(file, "%d", &processes[i].array_size);

        processes[i].search_indices = (int *)malloc(num_searches * sizeof(int));
        if (!processes[i].search_indices){
            perror("Memory allocation failed");
            exit(1);
        }

        for (int j = 0; j < num_searches; j++){
            fscanf(file, "%d", &processes[i].search_indices[j]);
        }

        processes[i].page_table = (unsigned short *)calloc(PAGE_TABLE_SIZE, sizeof(unsigned short));
        if (!processes[i].page_table){
            perror("Memory allocation failed");
            exit(1);
        }

        processes[i].history = (unsigned short *)calloc(PAGE_TABLE_SIZE, sizeof(unsigned short));
        if (!processes[i].history){
            perror("Memory allocation failed");
            exit(1);
        }

        for (int j = 0; j < ESSENTIAL_PAGES; j++){
            processes[i].history[j] = 0xFFFF;
        }
    }

    fclose(file);
    // printf("+++ Simulation data read from file\n");

    num_free_frames = USER_FRAMES;
    free_frames = (_frame_info *)malloc(USER_FRAMES * sizeof(_frame_info));
    if (!free_frames){
        perror("Memory allocation failed");
        exit(1);
    }

    for (int i = 0; i < USER_FRAMES; i++){
        free_frames[i].frame_number = i;
        free_frames[i].last_owner = -1;
        free_frames[i].last_page = -1;
    }

    ready_queue = (int *)malloc(num_processes * sizeof(int));
    if (!ready_queue){
        perror("Memory allocation failed");
        exit(1);
    }

    ready_queue_front = 0;
    ready_queue_rear = 0;
    ready_queue_count = 0;

    for (int i = 0; i < num_processes; i++){
        loadProcessEssentialPages(&processes[i]);
        processes[i].is_active = true;

        ready_queue[ready_queue_rear] = i;
        ready_queue_rear = (ready_queue_rear + 1) % num_processes;
        ready_queue_count++;
    }

    // printf("+++ Kernel data initialized\n");
}

void loadProcessEssentialPages(_process *process){
    for (int i = 0; i < ESSENTIAL_PAGES; i++){
        if (num_free_frames > 0){
            int idx = --num_free_frames;
            int frame = free_frames[idx].frame_number;

            process->page_table[i] = VALID_BIT_MASK | frame;
            process->allocated_frames++;

            process->history[i] = 0xFFFF;
        }
        else{
            fprintf(stderr, "Error: Not enough frames for essential pages of process %d\n", process->process_id);
            exit(1);
        }
    }
}

bool simulate_BS(_process *process){
    int search_key = process->search_indices[process->current_search];
    int left = 0;
    int right = process->array_size - 1;

    while (left < right){
        int mid = (left + right) / 2;

        process->page_accesses++;
        total_page_accesses++;
        int page_number = getPageOfElement(mid);

        if (process->page_table[page_number] & VALID_BIT_MASK){
            process->page_table[page_number] |= REF_BIT_MASK;
        }
        else{
            process->page_faults++;
            total_page_faults++;
            // printf("---%d-",num_free_frames);
            if (num_free_frames > NFFMIN){
            // printf("hello");
#ifdef VERBOSE
                printf("\tFault on Page  %d: Free frame %d found\n", page_number,
                       displayFrameNumber(free_frames[num_free_frames - 1].frame_number));
#endif
                int idx = --num_free_frames;
                int frame = free_frames[idx].frame_number;

                process->page_table[page_number] = VALID_BIT_MASK | REF_BIT_MASK | (frame & FRAME_MASK);
                process->allocated_frames++;

                process->history[page_number] = 0xFFFF;
            }
            else{
                process->replacements++;
                total_replacements++;

                int victim_page = -1;
                unsigned short min_history = 0xFFFF;

                for (int i = ESSENTIAL_PAGES; i < PAGE_TABLE_SIZE; i++){
                    if ((process->page_table[i] & VALID_BIT_MASK) &&
                        process->history[i] < min_history){
                        min_history = process->history[i];
                        victim_page = i;
                    }
                }

                if (victim_page == -1){
                    fprintf(stderr, "Error: Could not find a victim page for replacement\n");
                    return false;
                }

                int victim_frame = process->page_table[victim_page] & FRAME_MASK;

                process->page_table[victim_page] &= ~VALID_BIT_MASK;

                int replacement_frame = -1;

#ifdef VERBOSE
                printf("\tFault on Page  %d: To replace Page %d at Frame %d [history = %d]\n",
                       page_number, victim_page, displayFrameNumber(victim_frame), min_history);
#endif

                // Attempt 1: Check if page was previously loaded and replaced
                for (int i = 0; i < num_free_frames; i++){
                    if (free_frames[i].last_owner == process->process_id &&
                        free_frames[i].last_page == page_number){
                        replacement_frame = free_frames[i].frame_number;

                        free_frames[i] = free_frames[num_free_frames - 1];
                        num_free_frames--;

                        process->attempt1++;
                        total_attempt1++;
#ifdef VERBOSE
                        printf("\t\tAttempt 1: Page found in free frame %d\n",
                               displayFrameNumber(replacement_frame));
#endif
                        break;
                    }
                }

                // Attempt 2: Use a frame with no owner
                if (replacement_frame == -1){
                    for (int i = 0; i < num_free_frames; i++){
                        if (free_frames[i].last_owner == -1){
                            replacement_frame = free_frames[i].frame_number;

                            free_frames[i] = free_frames[num_free_frames - 1];
                            num_free_frames--;

                            process->attempt2++;
                            total_attempt2++;
#ifdef VERBOSE
                            printf("\t\tAttempt 2: Free frame %d owned by no process found\n",
                                   displayFrameNumber(replacement_frame));
#endif
                            break;
                        }
                    }
                }

                // Attempt 3: Use a frame previously owned by this process
                if (replacement_frame == -1){
                    for (int i = 0; i < num_free_frames; i++){
                        // printf(" %d%d ",free_frames[i].last_page != page_number?1:0,free_frames[i].last_owner == process->process_id?1:0);
                        if (free_frames[i].last_owner == process->process_id && free_frames[i].last_page != page_number){
                            // printf("\nentered here\n");
                            replacement_frame = free_frames[i].frame_number;

                            free_frames[i] = free_frames[num_free_frames - 1];
                            num_free_frames--;

                            process->attempt3++;
                            total_attempt3++;
#ifdef VERBOSE
                            printf("\t\tAttempt 3: Own page %d found in free frame %d\n",
                                   free_frames[i].last_page, displayFrameNumber(replacement_frame));
#endif
                            break;
                        }
                    }
                }

                // Attempt 4: Use any available frame
                if (replacement_frame == -1){
                    int random_index = rand() % num_free_frames;
                    replacement_frame = free_frames[random_index].frame_number;

#ifdef VERBOSE
                    printf("\t\tAttempt 4: Free frame %d owned by Process %d chosen\n",
                           displayFrameNumber(replacement_frame), free_frames[random_index].last_owner);
#endif

                    free_frames[random_index] = free_frames[num_free_frames - 1];
                    num_free_frames--;

                    process->attempt4++;
                    total_attempt4++;
                }

                free_frames[num_free_frames].frame_number = victim_frame;
                free_frames[num_free_frames].last_owner = process->process_id;
                free_frames[num_free_frames].last_page = victim_page;
                num_free_frames++;

                process->page_table[page_number] = VALID_BIT_MASK | REF_BIT_MASK | (replacement_frame & FRAME_MASK);

                process->history[page_number] = 0xFFFF;
            }
        }

        if (search_key <= mid){
            right = mid;
        }
        else{
            left = mid + 1;
        }
    }

    return true;
}

int getPageOfElement(int element_index){
    return ESSENTIAL_PAGES + (element_index * sizeof(int)) / PAGE_SIZE;
}

void updatePageHistory(_process *process){
    for (int i = 0; i < PAGE_TABLE_SIZE; i++){
        if (process->page_table[i] & VALID_BIT_MASK){
            process->history[i] = (process->history[i] >> 1) |
                                  ((process->page_table[i] & REF_BIT_MASK) ? 0x8000 : 0);

            process->page_table[i] &= ~REF_BIT_MASK;
        }
    }
}

void releaseProcessFrames(_process *process){
    for (int i = 0; i < PAGE_TABLE_SIZE; i++){
        if (process->page_table[i] & VALID_BIT_MASK){
            int frame = process->page_table[i] & FRAME_MASK;

            free_frames[num_free_frames].frame_number = frame;
            free_frames[num_free_frames].last_owner = -1;
            free_frames[num_free_frames].last_page = -1;
            num_free_frames++;

            process->page_table[i] &= ~VALID_BIT_MASK;
        }
    }

    process->allocated_frames = 0;
}

void cleanup(){
    for (int i = 0; i < num_processes; i++){
        free(processes[i].search_indices);
        free(processes[i].page_table);
        free(processes[i].history);
    }

    free(processes);
    free(free_frames);
    free(ready_queue);
}