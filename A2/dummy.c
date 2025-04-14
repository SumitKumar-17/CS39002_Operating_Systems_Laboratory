#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void handle_sigint(int signo) { exit(0); }

int main() {
  signal(SIGINT, handle_sigint);

  while (1) {
    pause();
  }

  return 0;
}