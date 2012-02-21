#include "types.h"
#include "x86.h"
#include "defs.h"
#include "param.h"
#include "mmu.h"
#include "proc.h"
#include "sysfunc.h"
#include "pstat.h"
#include "spinlock.h"
#include "ptable.h"

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
  return proc->pid;
}

int
sys_settickets(void)
{
	int num;
	if(argint(0, &num) < 0){
		return -1;
	}
	//Needs to be higher than 1
	if(num < 1){
		return -1;
	}
	proc->tickets = num;
	return 0;
}

int
sys_getpinfo(void)
{
	struct pstat* ps;
	if(argptr(0, (void*)&ps, sizeof(*ps)) < 0){
		return -1;
	}
	//Fill er up
	struct proc *p;
	// Loop over process table looking for process to run.
	acquire(&ptable.lock);
	int i;
	for(i = 0; i < NPROC; i++){
		p = &ptable.proc[i];
		ps->pid[i] = p->pid;
		if(p->state == UNUSED){ 
			ps->inuse[i] = 0;
		}else{
			ps->inuse[i] = 1;
		}
		ps->ticks[i] = p->ticks;
	}
	release(&ptable.lock);
	return 0;
}

int
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  addr = proc->sz;
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
    if(proc->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

// return how many clock tick interrupts have occurred
// since boot.
int
sys_uptime(void)
{
  uint xticks;
  
  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}
