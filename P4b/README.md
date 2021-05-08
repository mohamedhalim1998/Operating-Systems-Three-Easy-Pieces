## Thread System Call

### add to syscall.c

```c
extern int sys_clone(void);
extern int sys_join(void);
//add to (*syscalls[])(void)
[SYS_mprotect] sys_clone,
[SYS_munprotect] sys_join,
```

### add to syscall.h

```c
#define SYS_clone 27
#define SYS_join 28
```

### add to user.h

```c
int clone(void(*fcn)(void *, void *), void *arg1, void *arg2, void *stack);
int join(void **stack);
```

### add to usys.S

```c
SYSCALL(clone)
SYSCALL(join)
```

### add to defs.h

```c
int clone(void(*fcn)(void *, void *), void *arg1, void *arg2, void *stack);
int join(void **stack);
```

### add to sysproc.c

```c
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
```

### add to proc.c

```c
int
clone(void(*fcn)(void *, void *), void *arg1, void *arg2, void *stack)
{
  int i, pid;
  struct proc *np;
  struct proc *curproc = myproc();

  // Allocate process.
  if((np = allocproc()) == 0){
    return -1;
  }


  // Copy process state from proc.
  np->pgdir = curproc->pgdir;
  np->sz = curproc->sz;
  np->parent = curproc;
  *np->tf = *curproc->tf;
  void * retaddr = stack + PGSIZE - 3 * sizeof(void*);
  *(uint*) retaddr  = 0xFFFFFFFF;

  void * arg1add = stack + PGSIZE - 2 * sizeof(void*);
  *(uint*) arg1add  = (uint) arg1;
  void * arg2add = stack + PGSIZE - sizeof(void*);
  *(uint*) arg2add  = (uint) arg2;
  np->tf->esp = (uint) stack;

  np->tf->esp += PGSIZE -3*sizeof(void*) ;
  np->tf->ebp = np->tf->esp;
  np->tf->eip = (uint) fcn;
  // Clear %eax so that fork returns 0 in the child.
  np->tf->eax = 0;

  for(i = 0; i < NOFILE; i++)
    if(curproc->ofile[i])
      np->ofile[i] = filedup(curproc->ofile[i]);
  np->cwd = idup(curproc->cwd);

  safestrcpy(np->name, curproc->name, sizeof(curproc->name));

  pid = np->pid;

  acquire(&ptable.lock);
  np->state = RUNNABLE;
  np->tickets = curproc->tickets;
  release(&ptable.lock);

  return pid;

}
int 
join(void **stack)
{
    struct proc *p;
    struct proc *cp = myproc();
    acquire(&ptable.lock);

    for(;;)
    {
      for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
      {
        if(p->parent != cp && p->parent->pgdir != p->pgdir)
            continue;
        if(p->state == ZOMBIE){
            int pid = p->pid;
            p->state = UNUSED;
            kfree(p->kstack);
            release(&ptable.lock);
            return pid;
        }

      }
      sleep(cp, &ptable.lock);
    }

}
```

## Thread lib

### Add to ulib.c

```c
int 
thread_create(void (*start_routine)(void *, void *), void *arg1, void *arg2)
{
    void * stack = malloc(4096);
    return clone(start_routine, arg1, arg2, stack);
}
int 
thread_join()
{
    void * stack;
    return join(&stack);
   
}


typedef long Align;

union header {
  struct {
    union header *ptr;
    uint size;
  } s;
  Align x;
};

typedef union header Header;

static Header base;
static Header *freep;

void
free(void *ap)
{
  Header *bp, *p;

  bp = (Header*)ap - 1;
  for(p = freep; !(bp > p && bp < p->s.ptr); p = p->s.ptr)
    if(p >= p->s.ptr && (bp > p || bp < p->s.ptr))
      break;
  if(bp + bp->s.size == p->s.ptr){
    bp->s.size += p->s.ptr->s.size;
    bp->s.ptr = p->s.ptr->s.ptr;
  } else
    bp->s.ptr = p->s.ptr;
  if(p + p->s.size == bp){
    p->s.size += bp->s.size;
    p->s.ptr = bp->s.ptr;
  } else
    p->s.ptr = bp;
  freep = p;
}

static Header*
morecore(uint nu)
{
  char *p;
  Header *hp;

  if(nu < 4096)
    nu = 4096;
  p = sbrk(nu * sizeof(Header));
  if(p == (char*)-1)
    return 0;
  hp = (Header*)p;
  hp->s.size = nu;
  free((void*)(hp + 1));
  return freep;
}

void*
malloc(uint nbytes)
{
  Header *p, *prevp;
  uint nunits;

  nunits = (nbytes + sizeof(Header) - 1)/sizeof(Header) + 1;
  if((prevp = freep) == 0){
    base.s.ptr = freep = prevp = &base;
    base.s.size = 0;
  }
  for(p = prevp->s.ptr; ; prevp = p, p = p->s.ptr){
    if(p->s.size >= nunits){
      if(p->s.size == nunits)
        prevp->s.ptr = p->s.ptr;
      else {
        p->s.size -= nunits;
        p += p->s.size;
        p->s.size = nunits;
      }
      freep = prevp;
      return (void*)(p + 1);
    }
    if(p == freep)
      if((p = morecore(nunits)) == 0)
        return 0;
  }
}
```

#### comment umalloc.c file
