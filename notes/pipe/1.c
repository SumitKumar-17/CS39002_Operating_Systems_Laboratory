#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#define MAX 100 // Define the buffer size

int main() {
  char *msg = "hello"; // String message to write
  char buff[MAX];      // Buffer to store the message
  int p[2];            // Declare the pipe
  pid_t pid;           // Declare pid

  pipe(p);      // Create the pipe
  pid = fork(); // Create a child process

  if (pid > 0) {
    // Parent process: write to the pipe
    write(p[1], msg, strlen(msg) + 1); // Write the string to the pipe
  } else {
    // Child process: read from the pipe
    sleep(1);              // Ensure the parent writes first
    read(p[0], buff, MAX); // Read from the pipe into the buffer
    printf("%s\n", buff);  // Print the buffer
  }

  return 0;
}
