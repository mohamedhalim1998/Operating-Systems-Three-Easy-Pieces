#include "types.h"
#include "stat.h"
#include "user.h"
#include "fs.h"

int
main(int argc, char *argv[])
{
//  int x = 0;
  int tickets = atoi(argv[1]);
 // settickets(tickets);
  printf(1, "Tickets: %d\n", tickets);
  settickets(tickets);
  for(long i = 0; i < 1000; i++){
    printf(1, "Tickets %d, num : %d\n", tickets,i);
  }
  exit();
}

