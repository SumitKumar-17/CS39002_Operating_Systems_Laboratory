#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#define DEFAULT_N_VALUE 10
#define MAXIMUM_N_VALUE 100
#define HASH_TABLE_SIZE 1000

int hash_table[HASH_TABLE_SIZE] = {0};

int hash(int sum) { return sum % HASH_TABLE_SIZE; }

int check_sum(int sum) {
  int h = hash(sum);
  while (hash_table[h] != 0) {
    if (hash_table[h] == sum) {
      return 1;
    }
    h = (h + 1) % HASH_TABLE_SIZE;
  }
  return 0;
}

void insert_sum(int sum) {
  int h = hash(sum);
  while (hash_table[h] != 0) {
    h = (h + 1) % HASH_TABLE_SIZE;
  }
  hash_table[h] = sum;
}

int main(int argc, char *argv[]) {
  int n = DEFAULT_N_VALUE;
  if (argc > 1) {
    n = atoi(argv[1]);
    if (n <= 0 || n > MAXIMUM_N_VALUE) {
      printf("Error: n must be between 1 and %d\n", MAXIMUM_N_VALUE);
      return 1;
    }
  }

  key_t key = ftok("/", 'M');
  if (key == -1) {
    perror("ftok error");
    return 1;
  }

  size_t shm_size = sizeof(int) * (n + 4);
  int shmid = shmget(key, shm_size, IPC_CREAT | IPC_EXCL | 0666);
  if (shmid == -1) {
    perror("shmget error");
    return 1;
  }
  int *M = (int *)shmat(shmid, NULL, 0);
  if (M == (int *)-1) {
    perror("shmat error");
    return 1;
  }

  M[0] = n;
  M[1] = 0;
  M[2] = 0;

  printf("Waiting......\n");

  while (M[1] < n) {
    usleep(100000);
  }

  srand(time(NULL));

  while (1) {
    while (M[2] != 0) {
      if (M[2] == -1) {

        break;
      }
      usleep(1000);
    }

    M[3] = rand() % 99 + 1;

    int sum = M[3];
    for (int i = 1; i <= n; i++) {
      sum += M[3 + i];
    }

    printf("%d", M[3]);
    for (int i = 1; i <= n; i++) {
      printf(" + %d", M[3 + i]);
    }
    printf(" = %d\n", sum);

    if (check_sum(sum)) {
      M[2] = -1;
      break;
    } else {
      insert_sum(sum);
      M[2] = 1;
    }
  }

cleanup:
  while (M[2] != 0) {
    usleep(1000);
  }
  if (shmdt(M) == -1) {
    perror("shmdt error");
  }
  if (shmctl(shmid, IPC_RMID, NULL) == -1) {
    perror("shmctl error");
  }

  return 0;
}