#ifndef FILE_H
#define FILE_H

#include <types.h>
#include <vnode.h>
#define MAX_FILETABLE_LENGTH 100

typedef struct openfile{
	int mode;
	int ref_count;
	off_t offset;
	struct vnode* vnode_ptr;
	struct lock* vnode_lock;

} openfile;

#endif //FILE_H
