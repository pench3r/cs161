/*
 * forktest - test fork().
 *
 * This should work correctly when fork is implemented.
 *
 * It should also continue to work after subsequent assignments, most
 * notably after implementing the virtual memory system.
 */

#include <unistd.h>
#include <stdio.h>

int main() {
	int mypid;

	mypid = getpid();
	reboot(RB_REBOOT);
	return 0;
}
