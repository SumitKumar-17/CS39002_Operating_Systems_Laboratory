// writer.c - This program writes data to shared memory

#include <stdio.h>     // For printf
#include <stdlib.h>    // For exit
#include <string.h>    // For strcpy
#include <sys/ipc.h>   // For IPC_CREAT and key_t
#include <sys/shm.h>   // For shmget, shmat, shmdt
#include <sys/types.h> // For pid_t

int main() {
  int shmid;     // Shared memory ID
  key_t key = 3; // Shared memory key
  char *ptr;     // Pointer to shared memory

  // Create shared memory segment of size 100 bytes
  shmid = shmget(key, 100, IPC_CREAT | 0666);
  if (shmid == -1) {
    perror("shmget failed");
    return 1;
  }

  // Attach shared memory segment
  ptr = (char *)shmat(shmid, NULL, 0);
  if (ptr == (char *)-1) {
    perror("shmat failed");
    return 1;
  }

  printf("Writer: shmid = %d, ptr = %p\n", shmid, (void *)ptr);

  // Write data to shared memory
  strcpy(ptr, "hello");

  // Detach shared memory segment
  if (shmdt(ptr) == -1) {
    perror("shmdt failed");
    return 1;
  }

  printf("Writer: Data written to shared memory.\n");

  return 0;
}
