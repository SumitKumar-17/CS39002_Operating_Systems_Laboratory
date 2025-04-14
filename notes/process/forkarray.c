#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

int main() {
  int A[10], *B, i, t, pid;

  /*
  The srand() function sets its argument as the seed for a new sequence
  of pseudo-random integers to be returned by rand().
   These sequences are repeatable by calling srand() with the same seed value.
    If no seed value is provided, the rand() function is automatically seeded
  with a value of 1.
  */
  srand((unsigned int)time(NULL));

  B = (int *)malloc(10 * sizeof(int));
  t = 1 + rand() % 5;

  if ((pid = fork())) {
    for (i = 0; i < 10; ++i)
      A[i] = B[9 - i] = 10 + i;
    printf("Parent process going to sleep for %d seconds\n", t);
    sleep(t);
    printf("Parent process: A = %p, B = %p\n", A, B);
  } else {
    for (i = 0; i < 10; ++i)
      A[i] = B[9 - i] = i;
    i = t;
    while (i == t)
      t = 1 + rand() % 5;
    printf("Child process going to sleep for %d seconds\n", t);
    sleep(t);
    printf("Child process: A = %p, B = %p\n", A, B);
  }
  printf("A[] =");
  for (i = 0; i < 10; ++i)
    printf(" %d", A[i]);
  printf("\n");
  printf("B[] =");
  for (i = 0; i < 10; ++i)
    printf(" %d", B[i]);
  printf("\n");
  free(B);
  if (pid)
    wait(NULL);
  exit(0);
}