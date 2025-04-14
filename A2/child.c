#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#define PLAYING 0
#define CATCHMADE 1
#define CATCHMISSED 2
#define OUTOFGAME 3

int n;
int my_index;
pid_t *child_pids;
int my_status = PLAYING;
int last_action = PLAYING;
int just_missed = 0; // Flag to handle MISS display once

void print_header() {
  if (my_index == 1) {
    printf("\n");
    // printf(" ");
    for (int i = 1; i <= n; i++)
      printf("%8d", i);
    printf("\n+");
    for (int i = 0; i < 8 * n + 8; i++)
      printf("-");
    printf("+\n|");
  }
}

void print_status() {
  printf("   ");

  if (just_missed) {
    printf("MISS ");
    just_missed = 0;
    my_status = OUTOFGAME;
  } else if (my_status == OUTOFGAME) {
    printf("     ");
  } else if (last_action == CATCHMADE) {
    printf("CATCH");
    last_action = PLAYING;
  } else {
    printf(".... ");
  }

  if (my_index == n) {
    printf("        |\n+");
    for (int i = 0; i < 8 * n + 8; i++)
      printf("-");
    printf("+\n|");
  }
  fflush(stdout);
}

void handle_sigusr1(int signo) {
  static int first_print = 1; // Add this to track first print

  if (my_index == 1 && first_print) { // Only print header once at the start
    printf("\n");
    // printf(" ");
    for (int i = 1; i <= n; i++)
      printf("%8d", i);
    printf("\n+");
    for (int i = 0; i < 8 * n + 8; i++)
      printf("-");
    printf("+\n|");
    first_print = 0; // Set to false after first print
  }
  // print_header();
  print_status();

  // again print the values of n

  if (my_index < n) {
    kill(child_pids[my_index], SIGUSR1);
  } else {
    FILE *fp = fopen("dummycpid.txt", "r");
    pid_t dummy_pid;
    fscanf(fp, "%d", &dummy_pid);
    fclose(fp);
    kill(dummy_pid, SIGINT);
  }
}

void handle_sigusr2(int signo) {
  double catch_prob = ((double)rand() / RAND_MAX);

  if (catch_prob < 0.8) {
    last_action = CATCHMADE;
    kill(getppid(), SIGUSR1);
  } else {
    just_missed = 1; // Set flag to show MISS in next print
    kill(getppid(), SIGUSR2);
  }
}

void handle_sigint(int signo) {
  if (my_status != OUTOFGAME) {
    printf("\n+++ Child %d: Yay! I am the winner!\n", my_index);
  }
  exit(0);
}

int main(int argc, char *argv[]) {
  if (argc != 2) {
    fprintf(stderr, "Usage: %s <index>\n", argv[0]);
    exit(1);
  }

  srand(time(NULL) + getpid());
  my_index = atoi(argv[1]);

  sleep(1);

  FILE *fp = fopen("childpid.txt", "r");
  fscanf(fp, "%d", &n);

  child_pids = (pid_t *)malloc(n * sizeof(pid_t));
  for (int i = 0; i < n; i++) {
    fscanf(fp, "%d", &child_pids[i]);
  }
  fclose(fp);

  signal(SIGUSR1, handle_sigusr1);
  signal(SIGUSR2, handle_sigusr2);
  signal(SIGINT, handle_sigint);

  // entering a non-busy wait
  while (1) {
    pause();
  }

  return 0;
}