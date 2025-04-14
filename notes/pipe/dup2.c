#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

int main() {
  int old, new;
  old = open("input.txt", O_WRONLY | O_APPEND);

  new = dup2(old, 1);

  close(old);
  new = dup(old);
  printf("This will be output to the file named input.txt\n");
  return 0;
}

// The dup2 system call in C is used to duplicate a file descriptor and assign
// it a specific value.

// Syntax:
// c
// Copy
// Edit
// int dup2(int oldfd, int newfd);
// oldfd: The existing file descriptor to be duplicated.
// newfd: The target file descriptor where oldfd should be duplicated.
// Returns: The value of newfd on success, or -1 on failure.
// How dup2 Works
// If newfd is already open, dup2 closes it before reassigning it.
// After dup2(oldfd, newfd), newfd now refers to the same file as oldfd.
// Both file descriptors share the same file offset, meaning reads/writes affect
// both.