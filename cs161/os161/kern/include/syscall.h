#ifndef _SYSCALL_H_
#define _SYSCALL_H_

/*
 * Prototypes for IN-KERNEL entry points for system call implementations.
 */

int sys_reboot(int code);
int sys_getpid(pid_t *retval);

//File Syscalls
int sys_open(const char *path, int oflag, int *retfd);
int sys_read(int fd, void *buf, size_t nbytes);
int sys_write(int fd, const void *buf, size_t nbytes);
int sys_lseek(int fd, off_t offset, int whence);
int sys_close(int fd, int *retval);
int sys_dup2(int oldfd, int newfd);

// Process Syscalls
int sys_waitpid(int theirPID, int* status, int flags, int *retval);
int sys__exit(int exitCode);
int sys_fork(struct trapframe* ptf, int *retval);
int sys_execv(userptr_t prog, userptr_t argv);

#endif /* _SYSCALL_H_ */
