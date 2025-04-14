#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define BUFFER_SIZE 1024

int main() {

  FILE *file = popen("ls -l", "r");

  if (file == NULL) {
    perror("Error int popen");
  } else {
    char buffer[BUFFER_SIZE];

    // while(
    fgets(buffer, BUFFER_SIZE, file);
    // !=NULL){
    printf("%s", buffer);
    // }
  }
}