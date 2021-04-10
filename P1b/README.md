### add to syscall.c

```c
extern int sys_getreadcount(void);
//add to (*syscalls[])(void)
[SYS_getreadcount] sys_getreadcount,
```

### add to syscall.h

```c
#define SYS_getreadcount 22
```

### add to user.h

```c
int getreadcount(void);
```

### add to usys.S

```c
SYSCALL(getreadcount)
```

### add to sysfile.c

```c
static int readcount = 0;
struct spinlock lk;
// add into sys_read function
acquire(&lk);
readcount++;
release(&lk);


int
sys_getreadcount(void)
{ 
  return readcount;
}
```
