#include "types.h"
#include "x86.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "pstat.h"

struct pstat pstat;

int
sys_fork(void)
{
  return fork();
}

int
sys_exit(void)
{
  exit();
  return 0;  // not reached
}

int
sys_wait(void)
{
  return wait();
}

int
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

int
sys_getpid(void)
{
  return myproc()->pid;
}

int
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

int
sys_sleep(void)
{
  int n;
  uint ticks0;

  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(myproc()->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

// return how many clock tick interrupts have occurred
// since start.
int
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

int
sys_settickets(void)
{
    int n;
    if(argint(0, &n) < 0)
        return -1;
    if(n < 0)
        return -1;
    myproc()->tickets = n;
    return 0;
}
int sys_getpinfo(void){
   struct pstat *ps;
   if(argint(0,  (int*)(&ps)) < 0)
       return -1;
   getpstat(ps);
 return 0;
}
int
sys_mprotect(void)
{
    int addr;
    int len;
    if(argint(0,& addr) < 0)
        return -1;
    if(argint(1, &len) < 0)
        return -1;
    return mprotect((void *)addr,  len);
}
int
sys_munprotect(void)
{
    int addr;
    int len;
    if(argint(0,& addr) < 0)
        return -1;
    if(argint(1, &len) < 0)
        return -1;

    return munprotect((void * )addr,  len);
}
int
sys_clone(void)
{
    int fcn;
    int arg1;
    int arg2;
    int stack;
    if(argint(0, &fcn) < 0)
        return -1;
    if(argint(1, &arg1) < 0)
        return -1;
    if(argint(2, &arg2) < 0)
        return -1;
    if(argint(3, &stack) < 0)
        return -1;
    return clone((void *)fcn, (void *)arg1, (void *)arg2, (void *)stack); 
}

int 
sys_join(void)
{
    int stack;
    if(argint(0, &stack) < 0)
        return -1;
    return join((void **) stack);
}
