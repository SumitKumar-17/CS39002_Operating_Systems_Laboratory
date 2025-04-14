// Required header files
#include <stdio.h>     // For printf
#include <stdlib.h>    // For exit
#include <string.h>    // For strcpy
#include <sys/ipc.h>   // For IPC_CREAT and key_t
#include <sys/shm.h>   // For shmget, shmat
#include <sys/types.h> // For pid_t
#include <sys/wait.h>  // For wait
#include <unistd.h>    // For fork, getpid

int main() {
  int shmid;     // Shared memory ID
  key_t key = 2; // Shared memory key
  char *ptr;     // Pointer to shared memory
  pid_t pid;     // Process ID

  // Create shared memory segment of size 100 bytes
  shmid = shmget(key, 100, IPC_CREAT | 0666);
  if (shmid == -1) {
    perror("shmget failed");
    return 1;
  }

  // Attach shared memory segment to pointer
  ptr = (char *)shmat(shmid, NULL, 0);
  if (ptr == (char *)-1) {
    perror("shmat failed");
    return 1;
  }

  printf("shmid = %d, ptr = %p\n", shmid, (void *)ptr);

  // Create child process
  pid = fork();

  if (pid < 0) {
    perror("fork failed");
    return 1;
  } else if (pid == 0) {                     // Child process
    strcpy(ptr, "hello testing removal \n"); // Write to shared memory
    exit(0);
  } else {      // Parent process
    wait(NULL); // Wait for child to finish
    printf("Received from shared memory: %s\n", ptr);

    // // Detach shared memory wew need to explictiy detach shared memory it is
    // not done automatically shmdt(ptr);

    // // Remove shared memory segment
    // shmctl(shmid, IPC_RMID, NULL);
  }

  return 0;
}
