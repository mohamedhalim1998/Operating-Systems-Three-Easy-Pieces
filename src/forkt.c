#include "types.h"
#include "stat.h"
#include "user.h"
#include "fs.h"
#include "pstat.h"
int
main(int argc, char *argv[])
{
//  int x = 0;
 // int ticket = atoi(argv[1]);
 // settickets(tickets);
 // printf(1, "Tickets: %d\n", ticket);
//  settickets(ticket);
//  struct pstat p;
  for(int j = 1; j < 4; j++){
  int pid = fork();
  if(pid == 0){
      settickets(j * 100);
      for(long i = 0; i < 10000000000; i++){
       //  getpinfo(&p);

    //       for(int i = 0; i < 64; i++){
  //          if(p.pid[i] == getpid())
//              printf(1, "proc %d tickets: %d, ticks = %d \n",p.pid[i], p.tickets[i], p.ticks[i]);
          
      }
      }  else {
//      wait();
  }
  }
    //  getpinfo(&p);
//      for(int i = 0; i < 64; i++){
  //         printf(1, "proc %d tickets: %d\n",p.pid[i], p.tickets[i]);
          
     // }


  exit();
}

