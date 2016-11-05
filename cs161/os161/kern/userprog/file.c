#include <types.h>
#include <kern/errno.h>
#include <synch.h>
#include <vfs.h>
#include <uio.h>
#include <vnode.h>
#include <curthread.h>
#include "syscall.h"
#include "file.h"
#include <thread.h>

int sys_open(const char *path, int oflag, int *retfd) {
	int err = 0;
	struct vnode *vn = {0};	
	err = vfs_open(&path, oflag, &vn);

	if (err == 0) {
		openfile *ofile = {0};
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
	openfile* op = curthread->openfileTable[fd];
	struct uio userio;
	lock_acquire(op->vnode_lock);
	mk_kuio(&userio, &op, sizeof(nbytes), 0, UIO_READ);
	int result;
	result = VOP_READ(op->vnode_ptr, &userio);
	lock_release(op->vnode_lock);
	return result;
}
int sys_write(int fd, const void *buf, size_t nbytes){
	openfile* op = curthread->openfileTable[fd];
	struct uio userio;
	lock_acquire(op->vnode_lock);
	mk_kuio(&userio, &op, sizeof(nbytes), 0, UIO_WRITE);
	int result;
	result = VOP_WRITE(op->vnode_ptr, &userio);
	lock_release(op->vnode_lock);
	return result;
}
int sys_lseek(int fd, off_t offset, int whence){
	return 0;
}
int sys_close(int fd, int *retval){
	struct lock *lockptr;
	int flag = 0;

	if (fd < 0 || fd >= MAX_FILETABLE_LENGTH) {
		return EBADF;
	}
	
	if (curthread->openfileTable[fd] == NULL) {
		*retval = -1;
		return EBADF;
	}



	openfile *ofile = curthread->openfileTable[fd];
	lock_acquire(ofile->vnode_lock);
	lockptr = ofile->vnode_lock;
	ofile->ref_count--;

	if (ofile->ref_count == 0) {
		vfs_close(ofile->vnode_ptr);
		kfree(ofile);
		flag = 1;
	}

	curthread->openfileTable[fd] = NULL;
	*retval = 0;
	lock_release(lockptr);

	if (flag == 1) {
		lock_destroy(lockptr);
	}

	return 0;
}
int sys_dup2(int oldfd, int newfd, int *retval){
	return 0;
}
