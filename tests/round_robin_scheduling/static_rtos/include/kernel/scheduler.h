#ifndef STATIC_RTOS_SCHEDULER_H
#define STATIC_RTOS_SCHEDULER_H

#include <stdint.h>
#include <stddef.h>

#include <port/port.h>

enum kstatus_t {
	SUSPENDED,
	READY,
	RUNNING
};

struct kthread_t {
	int id;
	uint8_t priority;
	uint8_t last_run;
	enum kstatus_t status;
	void (*func)();
	void *args;
	void *stack;
	size_t stack_size;
	mcu_context_t context;
};

/**
 * With this function, the user will provide the array in which information
 * about the threads will be stored (the stacks must be allocated seperatly).
 * Must be called before starting the scheduler.
 *
 * @param arr The allocated array
 * @param arr_size The number of threads allocated for the arr
 *
 * @returns Returns 0 on success and 1 on failure. Sets kerrno appropriatly
 */
int kprovide_threads_array(struct kthread_t *arr, size_t arr_size);

/**
 * Function used to put a thread on the threads array. Must be called before
 * starting the scheduler
 *
 * @param func The function at which the thread will start
 * @param args The argument passed to fun
 * @param stack The stack of the thread. Must be statically allocated by the
 *		user
 * @param stack_size The size allocated for stack
 * @param priority The desired priority of the thread, must be bigger than 0.
 *		   Higher number, higher priority.
 *
 * @return On success, it will return the id of the thread, which is a strictly
 *	   positive integer. On failure, it will return -1 and set kerrno to
 *	   indicate the error (TODO)
 */
int kthread_create_static(void (*func)(), void *args, void *stack,
			  size_t stack_size, uint8_t priority);

/**
 * This function is used to suspend the thread indicated by id
 *
 * @param id The id of the thread to suspend. If id == 0, then suspend the
 *	     current thread
 *
 * @returns Returns 0 on success and 1 on failure.
 */
int kthread_suspend(int id);

/**
 * This function is used to unsuspend the thread indicated by id
 *
 * @param id The id of the thread to unsuspend. If id == 0, then unsuspend the
 *	     current thread
 *
 * @returns Returns 0 on success and 1 on failure.
 */
int kthread_unsuspend(int id);

/**
 * This function is used to start the scheduler. Before starting scheduling
 * this function creates the context for every thread
 *
 * @returns On success this function doesn't return, but on failure, it will
 *	    return 1
 */
int kscheduler_start(void);

/**
 * This function is used to yield execution back to the scheduler
 *
 * @returns Returns 0 on success and 1 on failure.
 */
int kyield(void);

#endif /* #ifndef STATIC_RTOS_SCHEDULER_H */

