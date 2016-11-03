#include <types.h>
#include <kern/errno.h>
#include <synch.h>
#include <vfs.h>
#include <uio.h>
#include <vnode.h>
#include <curthread.h>
#include "syscall.h"
#include "file.h"
#include "process.h"
#include <thread.h>

struct process* ptable[MAXPROCESS];

int sys_fork(struct trapframe* ptf) {
	return 0;
}

// Process Syscalls
int sys_waitpid(int theirPID, int* status, int flags) {
	int error;
	if(theirPID >= MAXPROCESS || theirPID < 0 || ptable[theirPID] == NULL) {
		return EFAULT;
	}
	if(status == NULL) {
		return EFAULT;
	}
	if(flags != 0) {
		return EINVAL;
	}
	if(curthread->t_pid != ptable[theirPID]->parent_pid) {
		return EFAULT;
	}
	if(ptable[theirPID]->exited == 0) {
		lock_acquire(ptable[theirPID]->lock_proc);
		cv_wait(ptable[theirPID]->wait_cv, ptable[theirPID]->lock_proc);
	}
	lock_release(ptable[theirPID]->lock_proc);
	cv_destroy(ptable[theirPID]->wait_cv);
	lock_destroy(ptable[theirPID]->lock_proc);
	kfree(ptable[theirPID]);
	ptable[theirPID] == NULL;
	return 0;
}

int sys__exit(int exitCode) {
	lock_acquire(ptable[curthread->t_pid]->lock_proc);

	//exitCode = _MKWAIT_EXIT(exitCode);
	ptable[curthread->t_pid]->exitcode = exitCode;
	ptable[curthread->t_pid]->exited = 1;
	int pid = 0;
	for (pid=0; pid < MAXPROCESS; pid++) {
		if(ptable[pid] == NULL) {
		} else if (ptable[pid]->parent_pid == curthread->t_pid) {
			ptable[pid]->parent_pid = -1;
		}
	}
	
	cv_broadcast(ptable[curthread->t_pid]->wait_cv, ptable[curthread->t_pid]->lock_proc);
	lock_release(ptable[curthread->t_pid]->lock_proc);
	thread_exit();
	return 0;
}
int sys_execv(userptr_t prog, userptr_t argv) {
	return 0;
}
