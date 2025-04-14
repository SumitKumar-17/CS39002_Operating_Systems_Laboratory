#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

#define MAX_BURSTS 100
#define MAX_PROCESSES 1000
#define INFINITY 1000000000

/*
  READY state means the PCB of the process is present in the ready queue.
  RUNNING state means the process is currently running on the CPU.abort
  WAITING means that the process is currently paused waiting for I/O operation
  to complete. FINISHED means the process has completed all its CPU bursts and
  has exited from the queue.
*/
typedef enum { READY, RUNNING, WAITING, FINISHED } ProcessState;

typedef struct {
  int id;
  int arrival_time;
  int cpu_bursts[MAX_BURSTS];
  int io_bursts[MAX_BURSTS];
  int num_cpu_bursts;
  int current_burst;
  ProcessState state;
  int remaining_time;
  int wait_time;
  int turnaround_time;
  int completion_time;
  int total_execution_time;
  int last_state_change;
} PCB;

typedef enum {
  PROCESS_ARRIVAL,
  CPU_BURST_END,
  IO_BURST_END,
  CPU_TIMEOUT
} EventType;

typedef struct {
  int time;
  int process_index;
  EventType type;
} Event;

PCB processes[MAX_PROCESSES];
int num_processes = 0;

// FIFO Ready Queue
typedef struct {
  int data[MAX_PROCESSES];
  int front, rear;
  int size;
} Queue;

void init_queue(Queue *q) {
  q->front = q->rear = -1;
  q->size = 0;
}

int is_empty(Queue *q) { return q->size == 0; }

void enqueue(Queue *q, int value) {
  if (q->size == 0) {
    q->front = q->rear = 0;
  } else {
    q->rear = (q->rear + 1) % MAX_PROCESSES;
  }
  q->data[q->rear] = value;
  q->size++;
}

int dequeue(Queue *q) {
  if (is_empty(q))
    return -1;

  int value = q->data[q->front];
  q->size--;

  if (q->size == 0) {
    q->front = q->rear = -1;
  } else {
    q->front = (q->front + 1) % MAX_PROCESSES;
  }

  return value;
}

// Event Queue MIN-HEAP
typedef struct {
  Event data[MAX_PROCESSES * 4];
  int size;
} EventQueue;

void init_event_queue(EventQueue *eq) { eq->size = 0; }

void swap_events(Event *a, Event *b) {
  Event temp = *a;
  *a = *b;
  *b = temp;
}

int compare_events(Event *a, Event *b) {
  if (a->time != b->time)
    return a->time - b->time;
  if (a->type != b->type) {
    // PROCESS_ARRIVAL and IO_BURST_END have higher priority than CPU_TIMEOUT
    if ((a->type == PROCESS_ARRIVAL || a->type == IO_BURST_END) &&
        b->type == CPU_TIMEOUT)
      return -1;
    if ((b->type == PROCESS_ARRIVAL || b->type == IO_BURST_END) &&
        a->type == CPU_TIMEOUT)
      return 1;
  }
  // For same type events, lower process ID has priority as per the priority
  // scheduling.
  return processes[a->process_index].id - processes[b->process_index].id;
}

void push_event(EventQueue *eq, Event event) {
  int current = eq->size;
  eq->data[current] = event;
  eq->size++;

  while (current > 0) {
    int parent = (current - 1) / 2;
    if (compare_events(&eq->data[parent], &eq->data[current]) > 0) {
      swap_events(&eq->data[parent], &eq->data[current]);
      current = parent;
    } else {
      break;
    }
  }
}

Event pop_event(EventQueue *eq) {
  Event result = eq->data[0];
  eq->size--;

  if (eq->size > 0) {
    eq->data[0] = eq->data[eq->size];
    int current = 0;

    while (1) {
      int left = 2 * current + 1;
      int right = 2 * current + 2;
      int smallest = current;

      if (left < eq->size &&
          compare_events(&eq->data[left], &eq->data[smallest]) < 0) {
        smallest = left;
      }

      if (right < eq->size &&
          compare_events(&eq->data[right], &eq->data[smallest]) < 0) {
        smallest = right;
      }

      if (smallest == current)
        break;

      swap_events(&eq->data[current], &eq->data[smallest]);
      current = smallest;
    }
  }

  return result;
}

void read_input() {
  FILE *fp = fopen("proc.txt", "r");
  if (!fp) {
    printf("Error opening proc.txt\n");
    exit(1);
  }

  fscanf(fp, "%d", &num_processes);

  for (int i = 0; i < num_processes; i++) {
    PCB *p = &processes[i];
    p->state = READY;
    p->current_burst = 0;
    p->wait_time = 0;
    p->remaining_time = 0;
    p->completion_time = 0;
    p->turnaround_time = 0;
    p->total_execution_time = 0;
    p->last_state_change = 0;

    fscanf(fp, "%d %d", &p->id, &p->arrival_time);

    int j = 0;
    int last_io = 0;
    while (1) {
      if (fscanf(fp, "%d", &p->cpu_bursts[j]) != 1) {
        printf("Error reading CPU burst for process %d\n", p->id);
        exit(1);
      }

      p->total_execution_time += p->cpu_bursts[j];

      if (fscanf(fp, "%d", &last_io) != 1) {
        printf("Error reading IO burst for process %d\n", p->id);
        exit(1);
      }

      if (last_io == -1) {
        p->num_cpu_bursts = j + 1;
        break;
      }

      p->io_bursts[j] = last_io;
      p->total_execution_time += last_io;
      j++;

      if (j >= MAX_BURSTS) {
        printf("Error: Too many bursts for process %d\n", p->id);
        exit(1);
      }
    }
  }

  fclose(fp);
}

void schedule(int quantum) {
  Queue ready_queue;
  EventQueue event_queue;
  int current_time = 0;
  int cpu_idle_time = 0;
  int last_event_time = 0;
  int running_process = -1;

  init_queue(&ready_queue);
  init_event_queue(&event_queue);

  // Reset process states
  for (int i = 0; i < num_processes; i++) {
    processes[i].state = READY;
    processes[i].current_burst = 0;
    processes[i].remaining_time = processes[i].cpu_bursts[0];
    processes[i].last_state_change = 0;
  }

  // Initialize event queue with process arrivals
  for (int i = 0; i < num_processes; i++) {
    Event e = {processes[i].arrival_time, i, PROCESS_ARRIVAL};
    push_event(&event_queue, e);
  }

#ifdef VERBOSE
  printf("0 : Starting\n");
#endif

  while (event_queue.size > 0) {
    Event current_event = pop_event(&event_queue);
    current_time = current_event.time;

    if (current_time > last_event_time && running_process == -1) {
      cpu_idle_time += current_time - last_event_time;
    }

    PCB *p = &processes[current_event.process_index];

    switch (current_event.type) {
    case PROCESS_ARRIVAL:
      p->state = READY;
      if (p->current_burst == 0) {
        p->remaining_time = p->cpu_bursts[0];
      }
      enqueue(&ready_queue, current_event.process_index);
      p->last_state_change = current_time;

#ifdef VERBOSE
      printf("%d : Process %d joins ready queue upon arrival\n", current_time,
             p->id);
#endif
      break;

    case CPU_BURST_END:
      p->current_burst++;
      running_process = -1;

      if (p->current_burst >= p->num_cpu_bursts) {
        p->state = FINISHED;
        p->completion_time = current_time;
        p->turnaround_time = p->completion_time - p->arrival_time;

        printf("%-10d: Process %6d exits. Turnaround time = %4d (%3d%%), Wait "
               "time = %d\n",
               current_time, p->id, p->turnaround_time,
               (p->turnaround_time * 100) / p->total_execution_time,
               p->wait_time);
      } else {
        p->state = WAITING;
        p->remaining_time = p->cpu_bursts[p->current_burst];
        Event io_completion = {current_time +
                                   p->io_bursts[p->current_burst - 1],
                               current_event.process_index, IO_BURST_END};
        push_event(&event_queue, io_completion);

#ifdef VERBOSE
        printf("%d : CPU goes idle\n", current_time);
#endif
      }
      p->last_state_change = current_time;
      break;

    case IO_BURST_END:
      p->state = READY;
      enqueue(&ready_queue, current_event.process_index);
      p->last_state_change = current_time;

#ifdef VERBOSE
      printf("%d : Process %d joins ready queue after IO completion\n",
             current_time, p->id);
#endif
      break;

    case CPU_TIMEOUT:
      running_process = -1;
      p->state = READY;
      enqueue(&ready_queue, current_event.process_index);
      p->last_state_change = current_time;

#ifdef VERBOSE
      printf("%d : Process %d joins ready queue after timeout\n", current_time,
             p->id);
#endif
      break;
    }

    // Schedule next process if CPU is idle and ready queue is not empty
    if (running_process == -1 && !is_empty(&ready_queue)) {
      int next_process = dequeue(&ready_queue);
      PCB *next_p = &processes[next_process];
      running_process = next_process;
      next_p->state = RUNNING;

      // Update wait time
      next_p->wait_time += current_time - next_p->last_state_change;
      next_p->last_state_change = current_time;

      int remaining_burst = next_p->cpu_bursts[next_p->current_burst];
      if (next_p->remaining_time > 0) {
        remaining_burst = next_p->remaining_time;
      }

      int run_time = (quantum < remaining_burst) ? quantum : remaining_burst;
      next_p->remaining_time = remaining_burst - run_time;

      Event next_event;
      next_event.time = current_time + run_time;
      next_event.process_index = next_process;

      if (run_time == remaining_burst) {
        next_event.type = CPU_BURST_END;
      } else {
        next_event.type = CPU_TIMEOUT;
      }

      push_event(&event_queue, next_event);

#ifdef VERBOSE
      printf("%d : Process %d is scheduled to run for time %d\n", current_time,
             next_p->id, run_time);
#endif
    }

    last_event_time = current_time;
  }

  // Print  statistics
  double total_wait_time = 0;
  for (int i = 0; i < num_processes; i++) {
    total_wait_time += processes[i].wait_time;
  }

  printf("Average wait time = %.2f\n", total_wait_time / num_processes);
  printf("Total turnaround time = %d\n", current_time);
  printf("CPU idle time = %d\n", cpu_idle_time);
  printf("CPU utilization = %.2f%%\n",
         ((double)(current_time - cpu_idle_time) / current_time) * 100);
}

void reset_processes() {
  for (int i = 0; i < num_processes; i++) {
    processes[i].state = READY;
    processes[i].current_burst = 0;
    processes[i].wait_time = 0;
    processes[i].remaining_time = 0;
    processes[i].completion_time = 0;
    processes[i].turnaround_time = 0;
    processes[i].last_state_change = 0;
  }
}

int main() {
  read_input();

  printf("**** FCFS Scheduling ****\n");
  schedule(INFINITY);

  reset_processes();
  printf("\n**** RR Scheduling with q = 10 ****\n");
  schedule(10);

  reset_processes();
  printf("\n**** RR Scheduling with q = 5 ****\n");
  schedule(5);

  return 0;
}