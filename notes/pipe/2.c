#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX 1000 // Define the buffer size

int main() {
  char *msg = "hello";
  char buff[MAX];

  int p[2];
  pipe(p);
  int pid = fork();
  if (pid == 0) {
    // sleep(1);
    printf("child exiting");
    // write(p[1], msg, MAX);
  } else {
    // sleep(1);
    // strcpy(buff, "sumit kumar");
    // write(p[1], msg, MAX);
    for (int i = 0; i < 10; i++) {
      write(p[1], msg, MAX);
      // printf("%s", buff);
    }

    close(p[1]);
    int count = 0;
    while (count < 10) {
      read(p[0], buff, MAX);
      printf("%s\n", buff);
      count++;
    }
    // read(p[0], buff, MAX);
    // char buff2[MAX];
    // read(p[0], buff2, MAX);
    // printf("%s   %s\n", buff,buff2);
  }

  return 0;
}
