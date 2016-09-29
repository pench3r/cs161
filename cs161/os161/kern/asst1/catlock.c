/*
 * catlock.c
 *
 * 30-1-2003 : GWA : Stub functions created for CS161 Asst1.
 *
 * NB: Please use LOCKS/CV'S to solve the cat syncronization problem in 
 * this file.
 */


/*
 * 
 * Includes
 *
 */

#include <types.h>
#include <lib.h>
#include <test.h>
#include <thread.h>
#include <synch.h>

/*
 * 
 * Constants
 *
 */

/*
 * Number of food bowls.
 */

#define NFOODBOWLS 2

/*
 * Number of cats.
 */

#define NCATS 6

/*
 * Number of mice.
 */

#define NMICE 2


/*
 * 
 * Function Definitions
 * 
 */

/*
 * Global variables
 */
static struct lock *mutex;
static struct lock *drivermutex;
static struct cv *turn_cv;
static struct cv *done_cv;

// 0) NOCATMOUSE 1) CAT 2) MOUSE
static volatile int turn_type;
static int volatile cats_wait_count;
static int volatile mice_wait_count;
static int volatile cats_in_this_turn;
static int volatile mice_in_this_turn;
static int volatile cats_eat_count;
static int volatile mice_eat_count;
static int volatile dish1_busy;
static int volatile dish2_busy;
static int volatile cats_total_eat;
static int volatile mice_total_eat;

/*
 * catlock()
 *
 * Arguments:
 *      void * unusedpointer: currently unused.
 *      unsigned long catnumber: holds the cat identifier from 0 to NCATS -
 *      1.
 *
 * Returns:
 *      nothing.
 *
 * Notes:
 *      Write and comment this function using locks/cv's.
 *
 */

static
void
catlock(void * unusedpointer, 
        unsigned long catnumber)
{
        /*
         * Avoid unused variable warnings.
         */

        (void) unusedpointer;
        (void) catnumber;

	int mydish = 0;

	lock_acquire(mutex);
	cats_wait_count++;
	if (turn_type == 0) {
		turn_type = 1;
		cats_in_this_turn = 2;
	}
	while (turn_type == 2 || cats_in_this_turn == 0) {
		cv_wait(turn_cv, mutex);
	}
	cats_in_this_turn--;
	cats_eat_count++;
	cats_total_eat++;
	int thiscat = cats_total_eat;
	kprintf("Cat %d  enters the kitchen.\n", thiscat);
	
	if (dish1_busy == 0) {
		dish1_busy = 1;
		mydish = 1;
	}
	else {
		dish2_busy = 1;
		mydish = 2;	
	}
	
	kprintf("Cat %d is eating.\n", thiscat);

	lock_release(mutex);

	clocksleep(1);

	lock_acquire(mutex);

	kprintf("Cat %d finished eating.\n", thiscat);
	
	if (mydish == 1) {
		dish1_busy = 0;
	} else {
		dish2_busy = 0;
	}

	cats_eat_count--;
	cats_wait_count--;

	if (mice_wait_count > 0 && cats_eat_count == 0) {
		mice_in_this_turn = 2;
		turn_type = 2;
		kprintf("It's the mice turn now.\n");
		cv_broadcast(turn_cv, mutex);
		lock_release(mutex);
	} else if (cats_wait_count > 0 && cats_eat_count == 0) {
		cats_in_this_turn = 2;
		cv_broadcast(turn_cv, mutex);
		lock_release(mutex);
	} else if (cats_wait_count == 0 && mice_wait_count == 0) {
		turn_type = 0;
		cv_signal(done_cv, mutex);
		lock_release(mutex);
	} else {
		lock_release(mutex);
	}
	return;
}
	

/*
 * mouselock()
 *
 * Arguments:
 *      void * unusedpointer: currently unused.
 *      unsigned long mousenumber: holds the mouse identifier from 0 to 
 *              NMICE - 1.
 *
 * Returns:
 *      nothing.
 *
 * Notes:
 *      Write and comment this function using locks/cv's.
 *
 */

static
void
mouselock(void * unusedpointer,
          unsigned long mousenumber)
{
        /*
         * Avoid unused variable warnings.
         */
        
        (void) unusedpointer;
        (void) mousenumber;

	int mydish = 0;

	lock_acquire(mutex);
	mice_wait_count++;
	if (turn_type == 0) {
		turn_type = 2;
		mice_in_this_turn = 2;
	}
	while (turn_type == 1 || mice_in_this_turn == 0) {
		cv_wait(turn_cv, mutex);
	}
	mice_in_this_turn--;
	mice_eat_count++;
	mice_total_eat++;
	int thismouse = mice_total_eat;
	kprintf("Mouse %d enters the kitchen.\n", thismouse);
	
	if (dish1_busy == 0) {
		dish1_busy = 1;
		mydish = 1;
	}
	else {
		dish2_busy = 1;
		mydish = 2;	
	}
	
	kprintf("Mouse %d is eating.\n", thismouse);

	lock_release(mutex);

	clocksleep(1);

	lock_acquire(mutex);

	kprintf("Mouse %d finished eating.\n", thismouse);
	
	if (mydish == 1) {
		dish1_busy = 0;
	} else {
		dish2_busy = 0;
	}
	
	mice_eat_count--;
	mice_wait_count--;
	
	if (cats_wait_count > 0 && mice_eat_count == 0) {
		turn_type = 1;
		cats_in_this_turn = 2;
		kprintf("It is the cats' turn now.\n");
		cv_broadcast(turn_cv, mutex);
		lock_release(mutex);
	} else if (mice_wait_count > 0 && mice_eat_count == 0) {
		mice_in_this_turn = 2;
		cv_broadcast(turn_cv, mutex);
		lock_release(mutex);
	} else if (cats_wait_count == 0 && mice_wait_count == 0) {
		turn_type = 0;
		cv_signal(done_cv, mutex);
		lock_release(mutex);
	} else {
		lock_release(mutex);
	}
	return;
}


/*
 * catmouselock()
 *
 * Arguments:
 *      int nargs: unused.
 *      char ** args: unused.
 *
 * Returns:
 *      0 on success.
 *
 * Notes:
 *      Driver code to start up catlock() and mouselock() threads.  Change
 *      this code as necessary for your solution.
 */

int
catmouselock(int nargs,
             char ** args)
{
        int index, error;
   	
	// Initialize locks and CVs
	mutex = lock_create("catlock and mouselock mutex");
	drivermutex = lock_create("driver mutex");
	turn_cv = cv_create("turn cv");
	done_cv = cv_create("done cv");

	cats_wait_count = 0;
	mice_wait_count = 0;
	cats_in_this_turn = 0;
	mice_in_this_turn = 0;
	cats_eat_count = 0;
	mice_eat_count = 0;
	dish1_busy = 0;
	dish2_busy = 0;
	cats_total_eat = 0;
	mice_total_eat = 0;
        /*
         * Avoid unused variable warnings.
         */

        (void) nargs;
        (void) args;
   
        /*
         * Start NCATS catlock() threads.
         */

        for (index = 0; index < NCATS; index++) {
           
                error = thread_fork("catlock thread", 
                                    NULL, 
                                    index, 
                                    catlock, 
                                    NULL
                                    );
                
                /*
                 * panic() on error.
                 */

                if (error) {
                 
                        panic("catlock: thread_fork failed: %s\n", 
                              strerror(error)
                              );
                }
        }

        /*
         * Start NMICE mouselock() threads.
         */

        for (index = 0; index < NMICE; index++) {
   
                error = thread_fork("mouselock thread", 
                                    NULL, 
                                    index, 
                                    mouselock, 
                                    NULL
                                    );
      
                /*
                 * panic() on error.
                 */

                if (error) {
         
                        panic("mouselock: thread_fork failed: %s\n", 
                              strerror(error)
                              );
                }
        }
	
	lock_acquire(mutex);
	while (cats_total_eat < 6 || mice_total_eat < 2) {
		cv_wait(done_cv, mutex);
	}
	lock_release(mutex);
	cv_destroy(turn_cv);
	cv_destroy(done_cv);
	lock_destroy(mutex);
	lock_destroy(drivermutex);

        return 0;
}

/*
 * End of catlock.c
 */
