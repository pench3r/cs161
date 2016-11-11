#include <types.h>
#include <vnode.h>
#include <kern/errno.h>
#include <kern/unistd.h>
#include <synch.h>
#include <addrspace.h>
#include <vfs.h>
#include <uio.h>
#include <curthread.h>
#include <kern/stat.h>
#include <thread.h>
#include "syscall.h"
#include "file.h"

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
	if(fd < 0 || fd > MAX_FILETABLE_LENGTH || curthread->openfileTable[fd] == NULL) {
		return EBADF;
	}
	openfile* op = curthread->openfileTable[fd];
	struct uio userio;
	lock_acquire(op->vnode_lock);
	mk_kuio(&userio, &buf, sizeof(nbytes), op->offset, UIO_READ);
	int result;
	result = VOP_READ(op->vnode_ptr, &userio);
	lock_release(op->vnode_lock);
	return result;
}

int sys_write(int fd, const void *buf, size_t nbytes){
	if(fd <  0 || fd > MAX_FILETABLE_LENGTH || curthread->openfileTable[fd] == NULL) {
		return EBADF;
	}	
	openfile* op = curthread->openfileTable[fd];
	struct uio userio;
	lock_acquire(op->vnode_lock);
	mk_kuio(&userio, &buf, sizeof(nbytes), op->offset, UIO_WRITE);
	int result;
	result = VOP_WRITE(op->vnode_ptr, &userio);
	lock_release(op->vnode_lock);
	return result;
}

int sys_lseek(int fd, off_t pos, int whence){
	struct openfile * ofile;
	struct stat fstat;
	int err = 0;
	off_t temp = 0;

	if (fd < 0 || fd >= MAX_FILETABLE_LENGTH || curthread->openfileTable[fd] == NULL)
		return EBADF;

	ofile = curthread->openfileTable[fd];
	lock_acquire(ofile->vnode_lock);

	switch(whence) {
		case SEEK_SET:
		err = VOP_TRYSEEK(ofile->vnode_ptr, pos);
		if (err) {
			lock_release(ofile->vnode_lock);
			return err;
		}
		ofile->offset = pos;
		break;

		case SEEK_CUR:
		temp = ofile->offset + pos;
		err = VOP_TRYSEEK(ofile->vnode_ptr, temp);
		if (err) {
			lock_release(ofile->vnode_lock);
			return err;
		}
		ofile->offset = temp;
		break;

		case SEEK_END:
		err = VOP_STAT(curthread->openfileTable[fd]->vnode_ptr, &fstat);
		if (err) {
			lock_release(ofile->vnode_lock);
			return err;
		}
		temp = fstat.st_size + pos;
		err = VOP_TRYSEEK(ofile->vnode_ptr, temp);
		if (err) {
			lock_release(ofile->vnode_lock);
			return err;
		}
		ofile->offset = temp;
		break;

		default:
		lock_release(ofile->vnode_lock);
		return EINVAL;
		break;
	}

	lock_release(ofile->vnode_lock);
	
	return ofile->offset;
}

int sys_close(int fd){
	struct lock *lockptr;
	int flag = 0;

	if (fd < 0 || fd >= MAX_FILETABLE_LENGTH) {
		return EBADF;
	}
	
	if (curthread->openfileTable[fd] == NULL) {
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
	lock_release(lockptr);

	if (flag == 1) {
		lock_destroy(lockptr);
	}

	return 0;
}
int sys_dup2(int oldfd, int newfd) {

	int err = 0;

	if (oldfd < 0 || oldfd >= MAX_FILETABLE_LENGTH || newfd < 0 || newfd >= MAX_FILETABLE_LENGTH || curthread->openfileTable[oldfd] == NULL) {
		return EBADF;
	}

	if (oldfd == newfd) {
		return newfd;
	}

	if (curthread->openfileTable[newfd] != NULL) {
		err = sys_close(newfd);
		if (err)
			return err;
	}

	openfile *ofile = curthread->openfileTable[oldfd];
	lock_acquire(ofile->vnode_lock);
	curthread->openfileTable[newfd] = ofile;
	curthread->openfileTable[newfd]->ref_count++;
	lock_release(ofile->vnode_lock);

	return newfd;
}
