#include "types.h"
#include "stat.h"
#include "user.h"
#include "fs.h"
#include "pstat.h"
int
main(int argc, char *argv[])
{
//  int x = 0;
  int ticket = atoi(argv[1]);
 // settickets(tickets);
  printf(1, "Tickets: %d\n", ticket);
  settickets(ticket);
//  int inuse[64];
 // int pid[64];
 // int tickets[64];
 // int ticks[64]
  struct pstat p;
   for(int i = 0; i < 1000; i++){ 
    getpinfo(&p);
    for(int j = 0; j < 64; j++)
      if(p.pid[j] == getpid())
       printf(1, "ticks: %d , tickets = %d \n", p.ticks[j] ,p.tickets[j]);  
   }
  exit();
}

