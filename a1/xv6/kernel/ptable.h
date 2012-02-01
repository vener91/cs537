#ifndef _PTABLE_H_
#define _PTABLE_H_

//Process table
struct ptable{
  struct spinlock lock;
  struct proc proc[NPROC];
} ptable;

#endif // _PTABLE_H_
