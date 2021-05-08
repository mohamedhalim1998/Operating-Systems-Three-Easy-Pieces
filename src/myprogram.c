#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"
#define NULL (void *)(0)

void t1(){
    printf(1, "Hello from t1\n");
    exit();
}


void t2(void* a, void* b){
    printf(1, "Hello from t2\n");
    printf(1, "x: %d\ny: %d\n",*(int*) a, *(int*) b);
    thread_create(&t1, NULL, NULL);
    thread_join();
    exit();
}
int fib(int n) {
	if (n <= 1) {
		return n;
	} else {
		return fib(n - 1) + fib(n - 2);
	}
}

int
main(int argc, char *argv[])
{
  int x = 5, y = 10;
  printf(1, "Welcome from main\n");
  thread_create(&t2, (void *)&x, (void *)&y);
  thread_join();
  //sleep(5000);
  printf(1, "Exit main\n");
  exit();

}
