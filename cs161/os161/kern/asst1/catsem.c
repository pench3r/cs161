/*
 * catsem.c
 *
 * 30-1-2003 : GWA : Stub functions created for CS161 Asst1.
 *
 * NB: Please use SEMAPHORES to solve the cat syncronization problem in 
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

static struct semaphore	*finish;
static struct semaphore	*catMutex; 
static struct semaphore	*mouseMutex; 
static struct semaphore	*catSem;
static struct semaphore	*mouseSem;
static struct semaphore	*mutex;

static volatile int catsWaiting;
static volatile int catsDone;
static volatile int miceWaiting;
static volatile int miceDone;

static int volatile dish1_busy;
static int volatile dish2_busy;


/*
 * catsem()
 *
 * Arguments:
 *      void * unusedpointer: currently unused.
 *      unsigned long catnumber: holds the cat identifier from 0 to NCATS - 1.
 *
 * Returns:
 *      nothing.
 *
 * Notes:
 *      Write and comment this function using semaphores.
 *
 */

static
void
catsem(void * unusedpointer, 
       unsigned long catnumber)
{
        /*
         * Avoid unused variable warnings.
         */

        (void) unusedpointer;
        (void) catnumber;
	int mydish = 0;

	P(catMutex);
	catsWaiting++;
	V(catMutex);

	//This allows cats into the kitchen if mice are not waiting yet.
	if (miceWaiting < 1 && catsWaiting < 2) {
		V(catSem);
		V(catSem);
	}
	
	
	P(catSem);

	//Apparently kprintf is not atomic.
	//If you do not put a mutex on it, two threads can print at the same time.
	P(mutex);
	if (dish1_busy == 0) {
		dish1_busy = 1;
		mydish = 1;
	}
	else {
		dish2_busy = 1;
		mydish = 2;	
	}
	kprintf("\ncat %d is eating from dish %d", catnumber, mydish);
	V(mutex);

	clocksleep(1);

	P(catMutex);
	catsDone++;
	catsWaiting--;
	if (mydish == 1) {
		dish1_busy = 0;
	} else {
		dish2_busy = 0;
	}

	//If you are the second cat to finish
	//If this is not inside the catMutex, then both cat threads can actually have catsDone=2.
	//We only want it to run when the second cat leaves, so we include the mutex.
	if (catsDone == 2) {
		V(catMutex);
		P(mutex);
		kprintf("\ncat %d left the kitchen", catnumber);
		V(mutex);
		//Prioritize mice, this will always allow mice to enter after two cats have gone.
		if (miceWaiting > 0) {
			//Signal two mice
			V(mouseSem);
			V(mouseSem);

			P(catMutex);
			catsDone = 0;
			V(catMutex);
		}
		else if (catsWaiting > 0){
			P(catMutex);
			catsDone = 0;
			V(catMutex);

			//Signal two more cats
			V(catSem);
			V(catSem);
		}	
		
	}
	//The first cat leaves
	else { 
		V(catMutex);
		P(mutex);
		kprintf("\ncat %d left the kitchen", catnumber);
		V(mutex);
	}
	//Check if the program is over
	if (catsWaiting == 0 && miceWaiting == 0) {
		//This mutex is so the program doesn't decide to finish while the final message is printing
		P(mutex);
		V(finish);
		V(mutex);
	}
		
}

        

/*
 * mousesem()
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
 *      Write and comment this function using semaphores.
 *
 */

static
void
mousesem(void * unusedpointer, 
         unsigned long mousenumber)
{
        /*
         * Avoid unused variable warnings.
         */

        (void) unusedpointer;
        (void) mousenumber;
	int mydish = 0;
	
	P(mouseMutex);
	miceWaiting++;
	V(mouseMutex);

	//This allows mice into the kitchen, if cats are not waiting yet.
	if (catsWaiting < 1 && miceWaiting < 2) {
		V(mouseSem);
		V(mouseSem);
	}

	P(mouseSem);

	P(mutex);
	if (dish1_busy == 0) {
		dish1_busy = 1;
		mydish = 1;
	}
	else {
		dish2_busy = 1;
		mydish = 2;	
	}
	kprintf("\nmouse %d is eating from dish %d", mousenumber, mydish);
	V(mutex);

	clocksleep(1);

	P(mouseMutex);
	miceDone++;
	miceWaiting--;
	if (mydish == 1) {
		dish1_busy = 0;
	} else {
		dish2_busy = 0;
	}
	//If this is not inside the mouse mutex, then miceDone can actually be 2 for both mice.
	//This results in this conditional running twice, which is bad.
	if (miceDone == 2) {
		miceDone = 0;
		V(mouseMutex);
		P(mutex);
		kprintf("\nmouse %d left the kitchen", mousenumber);
		V(mutex);
		
		//Signal two cats
		V(catSem);
		V(catSem);
	}
	//The first mouse leaves
	else { 
		V(mouseMutex);
		P(mutex);
		kprintf("\nmouse %d left the kitchen", mousenumber);
		V(mutex);
	}
	//Check if the program is done
	if (catsWaiting == 0 && miceWaiting == 0) {
		//This mutex is so the program doesn't decide to finish while the final message is printing
		P(mutex);
		V(finish);
		V(mutex);
	}

}


/*
 * catmousesem()
 *
 * Arguments:
 *      int nargs: unused.
 *      char ** args: unused.
 *
 * Returns:
 *      0 on success.
 *
 * Notes:
 *      Driver code to start up catsem() and mousesem() threads.  Change this 
 *      code as necessary for your solution.
 */

int
catmousesem(int nargs,
            char ** args)
{
        int index, error;
   
        /*
         * Avoid unused variable warnings.
         */

        (void) nargs;
        (void) args;
   
	catMutex = sem_create("catMutex", 1); //= 1
	if (catMutex == NULL)
		panic("catMutex: Out of memory.\n");

	mouseMutex = sem_create("mouseMutex", 1); //= 1
	if (mouseMutex == NULL)
		panic("mouseMutex: Out of memory.\n");

	catSem = sem_create("catSem", 0); //= 0;
	if (catSem == NULL)
		panic("catSem: Out of memory.\n");

	mouseSem = sem_create("mouseSem", 0);
	if (mouseSem == NULL)
		panic("mouseSem: Out of memory.\n");

	mutex = sem_create("mutex", 1);
	if (mutex == NULL)
		panic("mutex: Out of memory.\n");

	finish = sem_create("finish", 0);
	if (finish == NULL)
		panic("finish: Out of memory.\n");


	catsWaiting = 0;
	catsDone = 0;
	miceWaiting = 0;
	miceDone = 0;
	dish1_busy = 0;
	dish2_busy = 0;

        /*
         * Start NCATS catsem() threads.
         */
	

        for (index = 0; index < NCATS; index++) {
           
                error = thread_fork("catsem Thread", 
                                    NULL, 
                                    index, 
                                    catsem, 
                                    NULL
                                    );
                
                /*
                 * panic() on error.
                 */

                if (error) {
                 
                        panic("catsem: thread_fork failed: %s\n", 
                              strerror(error)
                              );
                }
        }
        
        /*
         * Start NMICE mousesem() threads.
         */

        for (index = 0; index < NMICE; index++) {
   
                error = thread_fork("mousesem Thread", 
                                    NULL, 
                                    index, 
                                    mousesem, 
                                    NULL
                                    );
                
                /*
                 * panic() on error.
                 */

                if (error) {
         
                        panic("mousesem: thread_fork failed: %s\n", 
                              strerror(error)
                              );
                }
        }
	//Add cleanup here

	P(finish);
	kprintf("\n");
	sem_destroy(mouseMutex);
	sem_destroy(catMutex);
	sem_destroy(mouseSem);
	sem_destroy(catSem);
	sem_destroy(mutex);
	sem_destroy(finish);
	

        return 0;
}


/*
 * End of catsem.c
 */
