## Read-only code

### add to syscall.c

```c
extern int sys_mprotect(void);
extern int sys_munprotect(void);
//add to (*syscalls[])(void)
[SYS_mprotect] sys_mprotect,
[SYS_munprotect] sys_munprotect,
```

### add to syscall.h

```c
#define SYS_mprotect 25
#define SYS_munprotect 26
```

### add to user.h

```c
int mprotect(void *addr, int len);
int munprotect(void *addr, int len);
```

### add to usys.S

```c
SYSCALL(mprotect)
SYSCALL(munprotect)
```

### add to defs.h

```c
int             mprotect(void *addr, int len);
int             munprotect(void *addr, int len);
```

### add to sysproc.c

```c
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
```

### add to vm.c

```c
int 
mprotect(void *addr, int len){
    if(len <= 0 || (uint)addr+len*PGSIZE>myproc()->sz){
        cprintf("mprotect: requested memory out of place %d the size is %d \n",(uint)addr+len*PGSIZE, myproc()->sz);
        return -1;
    }
    if((uint) addr % PGSIZE != 0){
        cprintf("mprotect: address must be page aligned\n");
    
        return-1;
    }
    pte_t *page;
    int limit = (int) len * PGSIZE +(int) addr;
    for(int i =(int) addr; i < limit; i++){
        page = walkpgdir(myproc()->pgdir, (void *)i, 0);
        *page  = *page & ~PTE_W;
    }
      lcr3(V2P(myproc()->pgdir));
    return 0;
}
int 
munprotect(void *addr, int len){
    if(len <= 0 || (int)addr+len*PGSIZE>myproc()->sz){
        cprintf("munprotect: requested memory out of place %d the size is %d\n",(int)addr+len*PGSIZE, myproc()->sz);
        return -1;
    }
    if((uint) addr % PGSIZE != 0){
        cprintf("munprotect: address must be page aligned\n");
        return -1;
    }
    pte_t *page;
    int limit =(int) len * PGSIZE + (int) addr;
    for(int i =(int) addr; i < limit; i++){
        page = walkpgdir(myproc()->pgdir, (void *)i, 0);
        *page  = *page | PTE_W;
    }
   lcr3(V2P(myproc()->pgdir));
    return 0;
}
```

## Null-pointer

this version of the system seems to handle null pointer very well without any modification










