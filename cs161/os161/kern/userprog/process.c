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
#include <addrspace.h>
#include <machine/trapframe.h>
#include <test.h>
#include <kern/limits.h>
#include <kern/unistd.h>

struct process* ptable[MAXPROCESS];

int sys_fork(struct trapframe* ptf) {

	int spl = splhigh();
	int error = 0;
	struct thread *child;
	struct trapframe* ctf = (struct trapframe*)kmalloc(sizeof(struct trapframe));
	if(ctf == NULL) {
		splx(spl);
		return EFAULT;
	}

	*ctf = *ptf;

	struct addrspace *cas;
	
	error = as_copy(curthread->t_vmspace, &cas);
	if (error) {
		splx(spl);
		return error;
	}
	
	error = thread_fork(curthread->t_name, entered_forked_proc, ctf, (unsigned long) cas, &child);
	if (error) {
		splx(spl);
		return error;
	}

	int i;
	for(i = 0; i < MAX_FILETABLE_LENGTH; i++) {
		child->openfileTable[i] = curthread->openfileTable[i];
		if(child->openfileTable[i] != NULL) 
			child->openfileTable[i]->ref_count++;
	}

	ptable[child->t_pid]->parent_pid = curthread->t_pid;

	splx(spl);
	
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
	struct vnode *v;
	vaddr_t entrypoint, stackptr;
	int result;
	char* kbuf = (char*)kmalloc(128*sizeof(char*));	

	//copy in the prog name
	size_t dog;
	char* prog_name;
	result = copyin(prog, prog_name, PATH_MAX, &dog);
	if(result != 0) {
		return result;
	}
	if(dog == 1) 
		return EINVAL;

	//copy in the argv
	result = copyin(&argv, &kbuf, sizeof(char*));
	if (result != 0)
		return result;

	// Open the executable. 
	result = vfs_open(prog, O_RDONLY, &v);
	if (result) {
		return result;
	}

	/* We should be a new thread. */
	if(curthread->t_vmspace == NULL) {
		return -1;
	}

	/* Create a new address space. */
	curthread->t_vmspace = as_create();
	if (curthread->t_vmspace==NULL) {
		vfs_close(v);
		return ENOMEM;
	}

	/* Activate it. */
	as_activate(curthread->t_vmspace);

	/* Load the executable. */
	result = load_elf(v, &entrypoint);
	if (result) {
		/* thread_exit destroys curthread->t_vmspace */
		vfs_close(v);
		return result;
	}

	/* Done with the file now. */
	vfs_close(v);

	/* Define the user stack in the address space */
	result = as_define_stack(curthread->t_vmspace, &stackptr);
	if (result) {
		/* thread_exit destroys curthread->t_vmspace */
		return result;
	}

	/* Warp to user mode. */
	md_usermode(0 /*argc*/, NULL /*userspace addr of argv*/,
		    stackptr, entrypoint);
	
	/* md_usermode does not return */
	panic("md_usermode returned\n");
	return EINVAL;
	return 0;
}

void entered_forked_proc(void* xtf, unsigned long xul) {
	struct trapframe *tf = (struct trapframe*) xtf;
	struct addrspace *as = (struct addrspace*) xul;
	struct trapframe tf_stack;

	tf_stack = *tf;
 	tf_stack.tf_a3 = 0;
	tf_stack.tf_v0 = 0;
	tf_stack.tf_epc += 4;

	as_copy(as, &curthread->t_vmspace);
	as_activate(curthread->t_vmspace);
	mips_usermode(&tf_stack);	
}

pid_t proc_init(struct thread* t) {
	struct process *p = kmalloc(sizeof(struct process));
	if (p == NULL)
		return -1;

	pid_t pid;
	if(ptable[2] == NULL) {
		p->parent_pid = -1;
	} else {
		p->parent_pid = curthread->t_pid;
	}
	p->exitcode = -1;
	p->exited = 0;
	p->self = t;
	p->wait_cv = cv_create("process_cv");
	p->lock_proc = lock_create("process_lock");
	pid = add_proc(p);
	if(pid < 0)
		return -1;

	return pid;
}		

pid_t add_proc(struct process* p) {
	int i;
	for(i = 2; i<MAXPROCESS + 2; i++) {
		if(ptable[i] == NULL) {
			ptable[i] = p;
			return i;
		}
	}
	return -1;
}
