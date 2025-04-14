// reader.c - This program reads data from shared memory

#include <stdio.h>     // For printf
#include <stdlib.h>    // For exit
#include <sys/ipc.h>   // For IPC_CREAT and key_t
#include <sys/shm.h>   // For shmget, shmat, shmdt, shmctl
#include <sys/types.h> // For pid_t

int main() {
  int shmid;     // Shared memory ID
  key_t key = 3; // Shared memory key
  char *ptr;     // Pointer to shared memory

  // Access existing shared memory segment
  shmid = shmget(key, 100, 0666); // 0666 means the perminsiinon of user,group
                                  // and others read and write
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

  printf("Reader: shmid = %d, ptr = %p\n", shmid, (void *)ptr);

  // Read data from shared memory
  printf("Reader: Data read from shared memory: %s\n", ptr);

  // Detach shared memory segment
  if (shmdt(ptr) == -1) {
    perror("shmdt failed");
    return 1;
  }

  // Remove shared memory segment after reading
  if (shmctl(shmid, IPC_RMID, NULL) == -1) {
    perror("shmctl failed");
    return 1;
  }

  printf("Reader: Shared memory removed.\n");

  return 0;
}

// void *shmat(int shmid, const void *shmaddr, int shmflg);
// Virtual address of the
// shared memory segment
// shmat() attaches the shared memory segment identified by shmid to the address
// space of the calling process.