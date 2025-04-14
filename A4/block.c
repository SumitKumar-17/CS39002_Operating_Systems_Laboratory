#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define BLOCK_SIZE 3
#define BOARD_SIZE 9
#define SLEEP_TIME 2

int A[BLOCK_SIZE][BLOCK_SIZE];
int B[BLOCK_SIZE][BLOCK_SIZE];

void printBlock() {
  // to clear the screen on force rewrite
  printf("\033[H\033[J");
  printf(" +---+---+---+\n");
  for (int i = 0; i < BLOCK_SIZE; i++) {
    printf(" |");
    for (int j = 0; j < BLOCK_SIZE; j++) {
      printf(" ");
      if (B[i][j] != 0) {
        printf("%d", B[i][j]);
      } else {
        printf(" ");
      }
      printf(" |");
    }
    printf("\n +---+---+---+\n");
  }
  fflush(stdout);
}

void newBlockStartingState() {
  for (int i = 0; i < BLOCK_SIZE; i++) {
    for (int j = 0; j < BLOCK_SIZE; j++) {
      scanf("%d", &B[i][j]);
      A[i][j] = B[i][j];
    }
  }
  printBlock();
  fflush(stdout);
  sleep(SLEEP_TIME);
}

void solution() {
  for (int i = 0; i < BLOCK_SIZE; i++) {
    for (int j = 0; j < BLOCK_SIZE; j++) {
      scanf("%d", &B[i][j]);
    }
  }
  printBlock();
  fflush(stdout);
  sleep(SLEEP_TIME);
}

int checkBlockConflict(int digit) {
  for (int i = 0; i < BLOCK_SIZE; i++) {
    for (int j = 0; j < BLOCK_SIZE; j++) {
      if (B[i][j] == digit) {
        return 1;
      }
    }
  }
  return 0;
}

// int checkNeighborsConflict(int fd, char command, int cell, int digit, int
// writeFd)
// {
//     int rowConflict = 0;
//     int stdout_copy = dup(1);
//     close(1);
//     dup(fd);
//     printf("%c %d %d %d\n", command, cell, digit, writeFd);
//     fflush(stdout);
//     close(1);
//     dup(stdout_copy);
//     scanf("%d", &rowConflict);
//     return rowConflict;
// }

int checkNeighborsConflict(int fd, char command, int cell, int digit,
                           int writeFd) {
  int rowConflict = 0;
  int stdout_copy = dup(1);
  if (stdout_copy < 0) {
    perror("dup failed");
    return -1;
  }

  close(1);
  int new_fd = dup(fd);
  if (new_fd < 0) {
    perror("dup failed");
    dup(stdout_copy); // Restore stdout
    close(stdout_copy);
    return -1;
  }

  printf("%c %d %d %d\n", command, cell, digit, writeFd);
  fflush(stdout);

  close(1);
  if (dup(stdout_copy) < 0) {
    perror("dup failed");
    close(stdout_copy);
    return -1;
  }
  close(stdout_copy); // Close the copy after we're done

  scanf("%d", &rowConflict);
  return rowConflict;
}

// void putCommnadBlock(){

// }

void quitting() {
  printf("Bye...\n");
  fflush(stdout);
  sleep(SLEEP_TIME);
  exit(0);
}

int main(int argc, char *argv[]) {
  if (argc != 8) {
    printf("Usage: %s <block> <readFd> <writeFd> <rowN1Fd> <rowN2Fd> <colN1Fd> "
           "<colN2Fd>\n",
           argv[0]);
    exit(1);
  }
  int block = atoi(argv[1]);
  fflush(stdout);
  printf("Block %d ready...\n", block);
  fflush(stdout);
  int readFd = atoi(argv[2]);
  int writeFd = atoi(argv[3]);
  int rowN1Fd = atoi(argv[4]);
  int rowN2Fd = atoi(argv[5]);
  int colN1Fd = atoi(argv[6]);
  int colN2Fd = atoi(argv[7]);

  close(0);
  dup(readFd);
  sleep(SLEEP_TIME);

  char command;

  while (1) {
    scanf(" %c", &command);
    switch (command) {
    case 'n':
      newBlockStartingState();
      break;

    case 's':
      solution();
      break;

    case 'p':
      int cell, digit;
      scanf("%d %d", &cell, &digit);

      int i = cell / BLOCK_SIZE;
      int j = cell % BLOCK_SIZE;

      if (A[i][j] != 0) {
        printf("Read-only cell\n");
        fflush(stdout);
        sleep(SLEEP_TIME);
        printBlock();
        break;
      }

      if (checkBlockConflict(digit)) {
        printf("Block Conflict\n");
        fflush(stdout);
        sleep(SLEEP_TIME);
        printBlock();
        break;
      }

      int rowConflict = 0;
      int colConflict = 0;

      rowConflict = checkNeighborsConflict(rowN1Fd, 'r', i, digit, writeFd) ||
                    checkNeighborsConflict(rowN2Fd, 'r', i, digit, writeFd);
      colConflict = checkNeighborsConflict(colN1Fd, 'c', j, digit, writeFd) ||
                    checkNeighborsConflict(colN2Fd, 'c', j, digit, writeFd);

      if (rowConflict) {
        printf("Row Conflict\n");
        fflush(stdout);
        sleep(SLEEP_TIME);
        printBlock();
        break;
      }

      if (colConflict) {
        printf("Col Conflict\n");
        fflush(stdout);
        sleep(SLEEP_TIME);
        printBlock();
        break;
      }

      B[i][j] = digit;
      printBlock();
      fflush(stdout);
      // putCommnadBlock();
      break;

    case 'r':
      int s1;
      scanf("%d %d %d", &cell, &digit, &s1);
      int existAlreadyRow = 0;
      for (int i = 0; i < BLOCK_SIZE; i++) {
        if (B[cell][i] == digit) {
          existAlreadyRow = 1;
          break;
        }
      }
      int stdout_copy = dup(1);
      close(1);
      dup(s1);
      printf("%d\n", existAlreadyRow);
      fflush(stdout);
      close(1);
      dup(stdout_copy);
      break;

    case 'c':
      int s2;
      scanf("%d %d %d", &cell, &digit, &s2);

      int existAlreadyCol = 0;
      for (int i = 0; i < BLOCK_SIZE; i++) {
        if (B[i][cell] == digit) {
          existAlreadyCol = 1;
          break;
        }
      }

      int stdout_copy2 = dup(1);
      close(1);
      dup(s2);
      printf("%d\n", existAlreadyCol);
      fflush(stdout);
      close(1);
      dup(stdout_copy2);
      break;

    case 'q':
      quitting();
      break;

    default:
      break;
    }
  }

  return 0;
}
