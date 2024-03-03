/*
 * Copyright 2024 Timothy Joseph. Subject to MIT license
 * See LICENSE.txt for details
 */

/**
 * Usage of the scheduler
 * 
 * In order to use the rtos, the user must follow these steps:
 * 
 * 1. Statically allocate space for the idle thread stack
 * 2. Call the function `kprovide_idle_thread_stack` in order to give the scheduler
 * the address of the idle thread stack
 * 3. Statically allocate space for the array of threads
 * 4. Call the function `kprovide_threads_array` in order to give the scheduler
 * the address of the threads array.\
 * For each thread:
 * 	5. Statically allocate the stack for a thread
 * 	6. Call the function `kthread_create_static` to place the new thread on the
 * array.\
 * Afterwards
 * 7. Call `kenable_tick_interrupt` if the user wants to use ticks
 * 8. Call `kscheduler_start` in order to start the scheduler.
 */

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
	mcu_context_t context;
	size_t stack_size;
	void (*func)(void *);
	void *args;
	void *stack;
	enum kstatus_t status;
	int id;
	uint16_t wake_up_at;
	uint8_t priority;
	uint8_t last_run;
	uint8_t wake_scheduled;
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
 * With this function, the user will provide the stack for the idle thread
 *
 * @param stack The allocated stack
 * @param stack_size The size of the allocated stack
 *
 * @returns Returns 0 on success and 1 on failure. Sets kerrno appropriatly
 */
int kprovide_idle_thread_stack(void *stack, size_t stack_size);

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
int kthread_create_static(void (*func)(void *), void *args, void *stack,
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

/**
 * This function enables interrupts for the processor and then enables the timer
 * interrupt that handles ticks. Ports must define a function called
 * port_enable_tick_interrupt that does this exact same thing and returns the
 * same
 *
 * @returns Returns 0 on success and 1 on failure
 */
int kenable_tick_interrupt(void);

/**
 * This function suspends the current thread and schedules it to become ready
 * after a certain period
 *
 * @param ticks_count The amount of ticks to sleep for
 *
 * @returns Returns 0 on success and 1 otherwise
 */
int ksleep_for_ticks(uint16_t ticks_count);

/**
 * This function is used to increase the tick count. If the tick count increase
 * reached a wake-up value, then it readies the specific threads
 *
 * @returns Returns 1 if a new thread is readied and 0 otherwise
 */
int kincrease_tickcount(void);

/* TODO */
int kernel_enable_tick_interrupt(void);
int KERNEL_ARE_INTERRUPTS_ENABLED(void);
int KERNEL_BEGIN_ATOMIC(void);
int KERNEL_END_ATOMIC(void);
int KERNEL_IS_ATOMIC(void);

#endif /* #ifndef STATIC_RTOS_SCHEDULER_H */

