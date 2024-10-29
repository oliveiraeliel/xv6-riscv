#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"
#include "proc_metrics.h"

uint64
sys_exit(void)
{
  int n;
  argint(0, &n);
  exit(n);
  return 0; // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return fork();
}

uint64
sys_wait(void)
{
  uint64 p;
  argaddr(0, &p);
  return wait(p);
}

uint64
sys_sbrk(void)
{
  uint64 addr;
  int n;

  argint(0, &n);
  addr = myproc()->sz;
  if (growproc(n) < 0)
    return -1;
  return addr;
}

uint64
sys_sleep(void)
{
  int n;
  uint ticks0;

  argint(0, &n);
  if (n < 0)
    n = 0;
  acquire(&tickslock);
  ticks0 = ticks;
  while (ticks - ticks0 < n)
  {
    if (killed(myproc()))
    {
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

uint64
sys_kill(void)
{
  int pid;

  argint(0, &pid);
  return kill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

uint64
sys_getprocmetrics(void)
{
  struct proc *p;
  uint64 addr;
  argaddr(0, &addr);

  if (addr < 0)
    return -1;

  p = myproc();

  struct proc_metrics * metrics = get_my_proc_metrics(p->pid);

  if (copyout(p->pagetable, addr, (char *)metrics, sizeof(*metrics)) < 0)
    return -1;

  return 0;
}

uint64
sys_getprocmetricsbypid(void)
{
  uint64 pid;
  uint64 addr;
  argaddr(0, &addr);
  argaddr(1, &pid);

  if (addr < 0)
    return -1;

  // struct proc_metrics *metrics = get_proc_metrics(pid);

  // if (copyout(p->pagetable, addr, (char *)metrics, sizeof(*metrics)) < 0)
  //   return -1;

  return 0;
}

uint64
sys_observeprocputs(void)
{
  struct proc *p = myproc();
  init_puts_table();
  set_proc_to_observe_puts(p);
  return 0;
}

uint64
sys_getprocputs(void)
{
  struct proc *p = myproc();
  uint8 *puts_table;
  uint64 addr;
  int num_procs;

  argaddr(0, &addr);
  argint(1, &num_procs);

  if (addr < 0)
    return -1;

  for (;;) {
    int sum = 0;
    for (int i = 1; i < num_procs; i++)
      sum += i * get_puts_table()[i];
    if (sum == num_procs)
      break;
  }

  puts_table = get_puts_table();

  if (copyout(p->pagetable, addr, (char *)puts_table, sizeof(uint8) * NPROC) < 0)
    return -1;
  
  return 0;
}

uint64
sys_waitandgetmetrics(void) {
  uint64 addr1, addr2;
  argaddr(0, &addr1);
  argaddr(1, &addr2);
  // printf("Waiting for pid, addr1: %ld, addr2: %ld\n", addr1, addr2);
  int pid = wait(addr1);

  // printf("Copying metrics for pid: %d\n", pid);

  if (pid < 0 || copyout(myproc()->pagetable, addr2, (char *)get_proc_metrics(pid), sizeof(struct proc_metrics)) < 0)
    return -1;
  return pid;
}