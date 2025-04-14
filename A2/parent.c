#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

// Game variables
#define PLAYING 0
#define CATCHMADE 1
#define CATCHMISSED 2
#define OUTOFGAME 3

int n;
pid_t *child_pids;
// parent maintain status
int *child_status;

int current_player = 0;
int players_remaining;

pid_t dummy_pid;
int received_signal = 0;

void handle_sigusr1(int signo) { received_signal = SIGUSR1; }

void handle_sigusr2(int signo) { received_signal = SIGUSR2; }

int find_next_player(int current) {
  int next = (current + 1) % n;
  while (child_status[next] == OUTOFGAME) {
    next = (next + 1) % n;
    if (next == current)
      break;
  }
  return next;
}

int main(int argc, char *argv[]) {
  if (argc != 2) {
    fprintf(stderr, "Usage: %s <number_of_children>\n", argv[0]);
    exit(1);
  }

  n = atoi(argv[1]);
  if (n <= 0) {
    fprintf(stderr, "Number of children must be positive\n");
    exit(1);
  }

  child_pids = (pid_t *)malloc(n * sizeof(pid_t));
  child_status = (int *)malloc(n * sizeof(int));
  players_remaining = n;

  for (int i = 0; i < n; i++) {
    child_status[i] = PLAYING; // init
  }

  for (int i = 0; i < n; i++) {
    pid_t pid = fork();

    if (pid < 0) {
      perror("Fork failed");
      exit(1);
    } else if (pid == 0) {
      char index[20];
      sprintf(index, "%d", i + 1);
      execl("./child", "child", index, NULL);
      perror("Exec failed");
      exit(1);
    } else {
      child_pids[i] = pid; // parent call
    }
  }

  FILE *fp = fopen("childpid.txt", "w");
  fprintf(fp, "%d\n", n);
  for (int i = 0; i < n; i++) {
    fprintf(fp, "%d\n", child_pids[i]);
  }
  fclose(fp);

  printf("Parent: %d child processes created\n", n);
  printf("Parent: Waiting for child processes to read child database\n");
  fflush(stdout);

  sleep(2);

  signal(SIGUSR1, handle_sigusr1);
  signal(SIGUSR2, handle_sigusr2);

  while (players_remaining > 1) {
    received_signal = 0;
    // sending sigusr2 to current player
    kill(child_pids[current_player], SIGUSR2);

    // infinite loop
    while (received_signal == 0) {
      pause();
    }

    if (received_signal == SIGUSR2) {
      child_status[current_player] = OUTOFGAME;
      players_remaining--;
    }

    pid_t dummy = fork();
    if (dummy == 0) {
      execl("./dummy", "dummy", NULL);
      exit(0);
    }
    dummy_pid = dummy;

    fp = fopen("dummycpid.txt", "w");
    fprintf(fp, "%d\n", dummy_pid);
    fclose(fp);

    kill(child_pids[0], SIGUSR1);
    waitpid(dummy_pid, NULL, 0);

    if (players_remaining > 1) {
      current_player = find_next_player(current_player);
    }
  }

  for (int i = 0; i < n; i++) {
    kill(child_pids[i], SIGINT);
    waitpid(child_pids[i], NULL, 0);
  }

  free(child_pids);
  free(child_status);
  return 0;
}