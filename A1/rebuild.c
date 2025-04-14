#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#define FAIL_CODE 1
#define MAX_LINE 256

#define DONE_FILE "done.txt"
#define DEP_FILE "foodep.txt"

void initialize_done_to_file(int n) {
  FILE *file = fopen(DONE_FILE, "w");
  if (!file) {
    printf("Failed to open done.txt");
    exit(FAIL_CODE);
  }
  for (int i = 0; i < n; i++) {
    fprintf(file, "0");
  }
  fprintf(file, "\n");
  fclose(file);
}

void read_file_done(int *visited, int n) {
  FILE *file = fopen(DONE_FILE, "r");
  if (!file) {
    printf("Failed to open done.txt");
    exit(FAIL_CODE);
  }
  for (int i = 0; i < n; i++) {
    visited[i] = fgetc(file) - '0';
  }
  fclose(file);
}

void write_file_done(int *visited, int n) {
  FILE *file = fopen(DONE_FILE, "w");
  if (!file) {
    printf("Failed to open done.txt");
    exit(FAIL_CODE);
  }
  for (int i = 0; i < n; i++) {
    fprintf(file, "%d", visited[i]);
  }
  fprintf(file, "\n");
  fclose(file);
}

void rebuild(int u, int is_root, int n) {
  FILE *dep_file = fopen(DEP_FILE, "r");
  if (!dep_file) {
    printf("Failed to open foodep.txt");
    exit(FAIL_CODE);
  }

  if (is_root) {
    initialize_done_to_file(n);
  }

  char line[MAX_LINE];
  int visited[n];
  int dependencies[MAX_LINE];
  int dep_count = 0;

  fgets(line, MAX_LINE, dep_file);
  int num_foodules = atoi(line);

  while (fgets(line, MAX_LINE, dep_file)) {
    char *token = strtok(line, ": ");
    int foodule = atoi(token);
    if (foodule == u) {
      token = strtok(NULL, " \n");
      while (token != NULL) {
        dependencies[dep_count++] = atoi(token);
        token = strtok(NULL, " \n");
      }
      break;
    }
  }

  fclose(dep_file);

  for (int i = 0; i < dep_count; i++) {
    int dep = dependencies[i];
    read_file_done(visited, n);

    if (!visited[dep - 1]) {
      pid_t pid = fork();
      if (pid < 0) {
        printf("Failed to fork process. Exiting...");
        exit(FAIL_CODE);
      } else if (pid == 0) {
        char depStr[10];
        sprintf(depStr, "%d", dep);
        execl("./rebuild", "rebuild", depStr, "child", NULL);
        printf("Failed to exec");
        exit(FAIL_CODE);
      } else {
        wait(NULL);
      }
    }
  }

  read_file_done(visited, n);
  visited[u - 1] = 1;
  write_file_done(visited, n);

  printf("foo%d rebuilt", u);
  if (dep_count > 0) {
    printf(" from");
  }
  for (int i = 0; i < dep_count; i++) {
    printf(" foo%d", dependencies[i]);
    if (i != dep_count - 1) {
      printf(",");
    }
  }
  printf("\n");
}

int main(int argc, char *argv[]) {
  int u;
  int is_root;
  if (argc < 2) {
    // fprintf(stderr, "Using Command: %s <n> [n stands for root]\n", argv[0]);
    // exit(FAIL_CODE);
    u = 10;
    is_root = 1;
    FILE *dep_file = fopen(DEP_FILE, "r");
    if (!dep_file) {
      printf("Failed to open foodep.txt");
      exit(FAIL_CODE);
    }

    char line[MAX_LINE];
    fgets(line, MAX_LINE, dep_file);
    int n = atoi(line);
    fclose(dep_file);

    rebuild(u, is_root, n);
  } else {

    u = atoi(argv[1]);
    is_root = (argc == 2);

    FILE *dep_file = fopen(DEP_FILE, "r");
    if (!dep_file) {
      printf("Failed to open foodep.txt");
      exit(FAIL_CODE);
    }

    char line[MAX_LINE];
    fgets(line, MAX_LINE, dep_file);
    int n = atoi(line);
    fclose(dep_file);

    rebuild(u, is_root, n);
  }
  return 0;
}