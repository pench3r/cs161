#include <types.h>
#include <kern/errno.h>
#include <synch.h>
#include <vfs.h>
#include <uio.h>
#include <vnode.h>
#include <curthread.h>
#include <thread.h>
#include "syscall.h"
#include "file.h"
#include "process.h"


// Process Syscalls
int sys_waitpid(int theirPID, int* status, int flags, int *retval) {
	return 0;
}
int sys__exit(int exitCode) {
	return 0;
}
int sys_fork(struct trapframe* ptf, int *retval) {
	return 0;
}
int sys_execv(userptr_t prog, userptr_t argv) {
	return 0;
}
