#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

int main() {
  int file_desc = open("dup.txt", O_WRONLY | O_APPEND);

  if (file_desc < 0) {
    printf("Error in opening file\n");
    return 1;
  }

  int copy_desc = dup(file_desc);
  /*
 write() syscall takes 3 arguments:  first where to write ,
 give the descriptor localtion ,
 then give the data to write, then teh size of the data
  */

  write(copy_desc, "This will be output to the file named dup.txt\n", 46);
  write(file_desc, "This will also be output to the file named dup.txt\n", 51);

  return 0;
}