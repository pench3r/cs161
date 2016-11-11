#ifndef _SYSCALL_H_
#define _SYSCALL_H_

/*
 * Prototypes for IN-KERNEL entry points for system call implementations.
 */
struct trapframe;

int sys_reboot(int code);
int sys_getpid(pid_t *retval);
int sys_fork(struct trapframe* ptf);

//File Syscalls
int sys_open(const char *path, int oflag, int *retfd);
int sys_read(int fd, void *buf, size_t nbytes);
int sys_write(int fd, const void *buf, size_t nbytes);
int sys_lseek(int fd, off_t pos, int whence);
int sys_close(int fd);
int sys_dup2(int oldfd, int newfd);

// Process Syscalls
int sys_waitpid(int theirPID, int* status, int flags);
int sys__exit(int exitCode);
int sys_execv(userptr_t prog, userptr_t argv);

#endif /* _SYSCALL_H_ */
