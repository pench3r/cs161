#include <types.h>
#include <kern/errno.h>
#include <synch.h>
#include <vfs.h>
#include <vnode.h>
#include <curthread.h>
#include "syscall.h"
#include "file.h"

int sys_open(const char *path, int oflag, int *retfd) {
	int err = 0;
	struct vnode *vn;	
	err = vfs_open(&path, oflag, &vn);

	if (err == 0) {
		openfile *ofile;
		ofile->offset = 0;
		ofile->ref_count = 1;
		ofile->vnode_ptr = vn;
		ofile->vnode_lock = lock_create("olock");
		//If vnode_lock is NULL then we are out of memory
		if (ofile->vnode_lock == NULL)
			return ENOMEM;

		int i = 0;
		for (; i < MAX_FILETABLE_LENGTH; i++) {
			if (curthread->openfileTable[i] == NULL) {
				*retfd = i;	
				break;
			}
		}
	}

	return err;
}
int sys_read(int fd, void *buf, size_t nbytes){
	return 0;
}
int sys_write(int fd, const void *buf, size_t nbytes){
	return 0;
}
int sys_lseek(int fd, off_t offset, int whence){
	return 0;
}
int sys_close(int fd){
	return 0;
}
int sys_dup2(int oldfd, int newfd){
	return 0;
}
