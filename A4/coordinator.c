#include "boardgen.c"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define TOTAL_BLOCKS 9
#define BOARD_SIZE 9
#define BLOCK_SIZE 3

typedef struct {
  int readFd;
  int writeFd;
  pid_t pid;
} BlockPipe;

BlockPipe blocks[TOTAL_BLOCKS];
int A[BOARD_SIZE][BOARD_SIZE];
int S[BOARD_SIZE][BOARD_SIZE];
int stdout_copy;
int gameInitialized = 0;

void getRowNeighbors(int block, int *n1, int *n2) {
  int row = block / 3;
  int pos = block % 3;
  *n1 = row * 3 + ((pos + 1) % 3);
  *n2 = row * 3 + ((pos + 2) % 3);
}

void getColNeighbors(int block, int *n1, int *n2) {
  int col = block % 3;
  *n1 = ((block + 3) % 9);
  *n2 = ((block + 6) % 9);
  if (*n1 % 3 != col)
    *n1 = ((*n1 / 3) * 3) + col;
  if (*n2 % 3 != col)
    *n2 = ((*n2 / 3) * 3) + col;
}

void printHelp() {
  printf("\nCommands supported:\n");
  printf("  n      Start new game\n");
  printf("  p b c d  Put digit d[1-9] at cell c[0-8] of block b[0-8]\n");
  printf("  s      Show solution\n");
  printf("  h      Print this help message\n");
  printf("  q      Quit\n\n");
  printf("Numbering scheme for blocks and cells:\n");
  printf("  +---+---+---+\n");
  printf("  | 0 | 1 | 2 |\n");
  printf("  +---+---+---+\n");
  printf("  | 3 | 4 | 5 |\n");
  printf("  +---+---+---+\n");
  printf("  | 6 | 7 | 8 |\n");
  printf("  +---+---+---+\n\n");
  fflush(stdout);
}

void initBlock() {
  for (int i = 0; i < BOARD_SIZE; i++) {
    int fd[2];
    if (pipe(fd) < 0) {
      perror("pipe failed");
      exit(1);
    }
    blocks[i].readFd = fd[0];
    blocks[i].writeFd = fd[1];
  }
  int rowNeighbor1, rowNeighbor2, colNeighbor1, colNeighbor2;
  for (int i = 0; i < BOARD_SIZE; i++) {
    pid_t pid = fork();
    if (pid < 0) {
      perror("fork failed");
      exit(1);
    }
    blocks[i].pid = pid;

    if (pid == 0) {
      char blockNo[10], readFd[10], writeFd[10];
      char rowN1Fd[10], rowN2Fd[10], colN1Fd[10], colN2Fd[10];
      getRowNeighbors(i, &rowNeighbor1, &rowNeighbor2);
      getColNeighbors(i, &colNeighbor1, &colNeighbor2);

      sprintf(blockNo, "%d", i);
      sprintf(readFd, "%d", blocks[i].readFd);
      sprintf(writeFd, "%d", blocks[i].writeFd);
      sprintf(rowN1Fd, "%d", blocks[rowNeighbor1].writeFd);
      sprintf(rowN2Fd, "%d", blocks[rowNeighbor2].writeFd);
      sprintf(colN1Fd, "%d", blocks[colNeighbor1].writeFd);
      sprintf(colN2Fd, "%d", blocks[colNeighbor2].writeFd);

      int base_x = 800;
      int base_y = 100;
      int x_spacing = 250;
      int y_spacing = 200;
      int x = base_x + (i % 3) * x_spacing;
      int y = base_y + (i / 3) * y_spacing;

      char geometry[32];
      sprintf(geometry, "17x8+%d+%d", x, y);

      char title[20];
      sprintf(title, "Block %d", i);

      execlp("xterm", "xterm", "-T", title, "-fa", "Monospace", "-fs", "15",
             "-geometry", geometry, "-bg", "#333333", "-e", "./block", blockNo,
             readFd, writeFd, rowN1Fd, rowN2Fd, colN1Fd, colN2Fd, NULL);

      perror("execlp failed");
      exit(1);
    }
  }
}

void sendBlockData(int arr[BOARD_SIZE][BOARD_SIZE], int block) {
  int rowStart = (block / 3) * 3;
  int colStart = (block % 3) * 3;
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 3; j++) {
      printf("%d ", arr[rowStart + i][colStart + j]);
    }
  }
  printf("\n");
}

void cleanup() {
  for (int i = 0; i < BOARD_SIZE; i++) {
    close(1);
    dup(blocks[i].writeFd);
    printf("q\n");
    fflush(stdout);
  }

  for (int i = 0; i < BOARD_SIZE; i++) {
    wait(NULL);
  }

  close(1);
  dup(stdout_copy);
  printf("Game Over\n");
  sleep(1);
  exit(0);
}

void newGame() {
  newboard(A, S);
  // printf("New game started\n");
  // for(int i = 0; i < BOARD_SIZE; i++)
  // {
  //     for(int j = 0; j < BOARD_SIZE; j++)
  //     {
  //         printf("%d ", A[i][j]);
  //     }
  //     printf("\n");
  // }
  gameInitialized = 1;
  for (int i = 0; i < TOTAL_BLOCKS; i++) {
    close(1);
    dup(blocks[i].writeFd);
    printf("n");
    sendBlockData(A, i);
    fflush(stdout);
  }
  fflush(stdout);
  close(1);
  dup(stdout_copy);
}

void showSolution() {
  for (int i = 0; i < TOTAL_BLOCKS; i++) {
    close(1);
    dup(blocks[i].writeFd);
    printf("s");
    sendBlockData(S, i);
    fflush(stdout);
  }
  close(1);
  dup(stdout_copy);
}

int main() {
  gameInitialized = 0;

  initBlock();

  printHelp();

  char cmd;
  int block, cell, digit;

  while (1) {
    printf("Foodoku> ");
    int stdout_copy = dup(1);
    if (scanf(" %c", &cmd) != 1)
      continue;

    switch (cmd) {
    case 'h':
      printHelp();
      break;

    case 'n':
      newGame();
      break;

    case 'p':
      if (!gameInitialized) {
        printf("Game not initialized\n");
        break;
      }

      if (scanf("%d %d %d", &block, &cell, &digit) != 3) {
        printf("Invalid input format\n");
        continue;
      }

      if (block < 0 || block >= 9 || cell < 0 || cell >= 9 || digit < 1 ||
          digit > 9) {
        printf("Invalid input range\n");
        fflush(stdout);
        break;
      }

      close(1);
      dup(blocks[block].writeFd);
      printf("p %d %d \n", cell, digit);
      fflush(stdout);
      close(1);
      dup(stdout_copy);
      break;

    case 's':
      if (!gameInitialized) {
        printf("Game not initialized\n");
        break;
      }
      showSolution();
      break;

    case 'q':
      cleanup();
      break;

    default:
      printf("Unknown command: %c\n", cmd);
      printHelp();
      break;
    }
  }

  return 0;
}