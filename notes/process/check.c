#include <stdio.h>
#include <unistd.h>

int main() {
  int n = 3; // You can set any value for n
  for (int i = 0; i < n; i++) {
    fork();
    // printf("\nTest1");
    // if (fork() == 0) {
    //     printf("\nTest2");
    // }
  }
  printf("\nTest3");
  return 0;
}

/*

each 1 single call of fork() creates 2 process (one parent ,one child)
When value returned by fork()==0 it means child else means parent , negative
value means error no creation happended


Assuming n starts from (1 to n)
1 fork() call =2 process
2 fork() call =4 process
3 fork() call =8 process
...
n fork() call =2^n process

for for n loops we have 2^n process
Thus "Test1" will be printed 2^n times
and for "Test 2" it will be printed 2^(n-1) times as only child fork() are
considerdd so half it

but in the question loop begins with (0 to n-1)  values so manipulating our
formulae we get with repect to question for "Test1" it will be printed 2^(n)
times for "Test2" it will be printed 2^(n-1) times


finally fro test 3 it will be printed 2^n times as it is printed in both parent
and child process so 2^n times finally manipualting the formulae as per
theindexing of loop we get 2^(n) times for "Test3"
*/