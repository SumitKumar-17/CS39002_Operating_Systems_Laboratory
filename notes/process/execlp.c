/*
                EXECUTING AN APPLICATION USING EXEC COMMANDS

        The execlp() family of commands can be used to execute an
        application from a process. The system call execlp()
        replaces the executing process by a new process image
        which executes the application specified as its parameter.
        Arguments can also be specified. Refer to online man pages.
*/

#include <stdio.h>
#include <sys/ipc.h>
#include <unistd.h>

int main() {
  /*  "cal" is an application which shows the calendar of the
      current year and month. "cal" with an argument specifying
      year (for example "cal 1999") shows the calendar of that
      year. Try out the "cal" command from your command prompt.

      Here we execute "cal 1752" using the execlp() system call.
      Note that we specify "cal" in the first two arguments. The
      reason for this is given in the online man pages for execlp()
  */

  execlp("cal",
         "cal"
         "2025",
         NULL);

  // note if you use execlp("cal","2025",NULL) then it will not work as the
  // first argument should be the name of the file

  /*
  The execlp() system call does not return.  whatever code is written after
  it.Note that the following statement will not be executed.
  */

  // Line is not getting printed
  printf("This statement is not executed if execlp succeeds.\n");
}