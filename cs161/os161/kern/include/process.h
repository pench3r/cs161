/* 
 * Process header file
 *
 * Robby Lagen
 */
#ifndef _PROCESS_H_
#define _PROCESS_H_
#include <types.h>
#include <vnode.h>
#define MAXPROCESS 100

#define MAXPROCESS 100

// array that holds running processes
extern struct process* proc_table[];

struct process {
   pid_t parent_pid;
   struct cv *wait_cv;
   struct lock *lock_proc;
   int exited;
   int exitcode;
   struct thread *self;
};

int proc_init(struct thread *t);

pid_t add_proc(struct process *proc);
#endif
