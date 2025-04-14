#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define DEFAULT_NF 1

void processFollow(int *M, int followerId) {
  srand(time(NULL) ^ (followerId << 16));

  while (1) {
    while (M[2] != followerId && M[2] != -followerId) {
      usleep(1000);
    }

    if (M[2] == -followerId) {
      printf("follower %d leaves\n", followerId);

      if (followerId == M[0]) {
        M[2] = 0;
      } else {
        M[2] = -(followerId + 1);
      }
      break;
    }

    M[3 + followerId] = rand() % 9 + 1;

    if (followerId == M[0]) {
      M[2] = 0;
    } else {
      M[2] = followerId + 1;
    }
  }
}

int main(int argc, char *argv[]) {

  int nf = DEFAULT_NF;

  if (argc > 1) {
    nf = atoi(argv[1]);
    if (nf <= 0) {
      printf("Error: nf must be positive\n");
      return 1;
    }
  }

  key_t key = ftok("/", 'M');
  if (key == -1) {
    perror("ftok error");
    return 1;
  }

  int shmid = shmget(key, 0, 0666);
  if (shmid == -1) {
    perror("shmget error: Is the leader running?");
    return 1;
  }

  int *M = (int *)shmat(shmid, NULL, 0);
  if (M == (int *)-1) {
    perror("shmat error");
    return 1;
  }

  pid_t *children = malloc(nf * sizeof(pid_t));
  if (!children) {
    perror("malloc error");
    return 1;
  }

  for (int i = 0; i < nf; i++) {
    children[i] = fork();

    if (children[i] == -1) {
      printf("Error: fork failed\n");
      return 1;
    }

    if (children[i] == 0) {
      int followerId;

      // followerId = __sync_fetch_and_add(&M[1], 1) + 1;

      followerId = M[1] + 1;
      M[1] = followerId;

      if (followerId > M[0]) {
        printf("follower error: %d followers have already joined\n", M[0]);
        exit(1);
      }

      printf("follower %d joins\n", followerId);

      processFollow(M, followerId);

      if (shmdt(M) == -1) {
        perror("shmdt failed\n");
      }

      exit(0);
    }
  }

  for (int i = 0; i < nf; i++) {
    waitpid(children[i], NULL, 0);
  }

  free(children);
  if (shmdt(M) == -1) {
    printf("Error: shmdt failed\n");
  }

  return 0;
}
