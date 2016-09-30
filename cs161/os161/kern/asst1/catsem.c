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

typedef int bool;
enum {false, true};

/*
 * 
 * Function Definitions
 * 
 */

static struct semaphore	*finish;
static struct semaphore	*catMutex; //= 1
static struct semaphore	*mouseMutex; //= 1
static struct semaphore	*bowlsAvailable;// = 1;
static struct semaphore	*catSem; //= 0;
static struct semaphore	*mouseSem;
static struct semaphore	*kitchenSem;

static volatile int catsWaiting;
static volatile int miceWaiting;


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

	P(catMutex);
	catsWaiting++;
	if (catsWaiting == 1)
		P(kitchenSem);
	catsInThisTurn++;	
	V(catMutex);


	clocksleep(1);

	P(catMutex);
	catsWaiting--;
	catsInThisTurn--;
	if (miceWaiting > 0 && catsInThisTurn == 0) {
		miceInThisTurn = 2;
		V(kitchenSem);
	}
	else if (catsWaiting > 0) {
		catsInThisTurn = 2;
		V(catSem);
	} else {
		V(finish);
	}
	V(catMutex);
		
		

        

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
	

	int mydish;
	bool first_mouse_eat = false;
	bool another_mouse_eat = false;

	P(mutex);
	if (all_dishes_available == true) {
		all_dishes_available = false;
		V(mouse_queue);
	}
	mouse_wait_count++;
	V(mutex);

	P(mouse_queue);
	if (no_mouse_eat = true) {
		no_mouse_eat = false;
		first_mouse_eat = true;
	}
	else first_mouse_eat = false;

	if (first_mouse_eat == true) {
		P(mutex);
		if (mouse_wait_count > 1) {
			another_mouse_eat = true;
			V(mouse_queue);
		}
		V(mutex);
	}

	P(mutex);
	kprintf("Mouse %d in the kitchen.\n", mousenumber);
	V(mutex);

	P(dish_mutex);
	if (dish1_busy == false) {
		dish1_busy = true;
		mydish = 1;
	}
	else {
		if (dish2_busy == true)
			panic("Dish2_busy == true!");
		dish2_busy = true;
		mydish = 2;
	}
	V(dish_mutex);

	P(mutex);
	kprintf("Mouse eating.\n");
	V(mutex);

	clocksleep(1);

	P(mutex);
	kprintf("Finish eating.\n");
	V(mutex);


	//Release dishes
	P(dish_mutex);
	if (mydish == 1)
		dish1_busy = false;
	else 
		dish2_busy = false;
	V(dish_mutex);
	
	P(mutex);
	mouse_wait_count--;
	V(mutex);

	if (first_mouse_eat == true) {
		if (another_mouse_eat == true)
			P(done);

		P(mutex);
		kprintf("First mouse is leaving.\n");
		V(mutex);

		no_mouse_eat = true;

		P(mutex);
		if(mouse_wait_count > 0) 
			V(mouse_queue);
		else if(cats_wait_count > 0)
			V(cats_queue);
		else  {
			all_dishes_available = true;
			V(finish);
		}
		V(mutex);
	}
	else {
		P(mutex);
		kprintf("Second mouse is leaving\n");
		V(mutex);
		V(done);
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
   
	finish = sem_create("finish", 0);
	catMutex = sem_create("catMutex", 1); //= 1
	mouseMutex = sem_create("mouseMutex", 1); //= 1
	bowlsAvailable  = sem_create("bowlsAvailable", 2); // = 1;
	catSem = sem_create("catSem", 0); //= 0;
	mouseSem = sem_create("mouseSem", 0);
	kitchenSem = sem_create("kitchenSem", 1);

	catsWaiting = 0;
	miceWaiting = 0;

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
	

        return 0;
}


/*
 * End of catsem.c
 */
