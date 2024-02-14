#include <stdio.h>
#include <stdint.h>

#define BASE_RR 0x3

enum kstatus_t {
	SUSPENDED,
	READY,
	RUNNING
};

struct kthread_t {
	int id;
	uint8_t priority;
	uint8_t rr_priority;
	enum kstatus_t status;
};

/**
 * With this function, the user will provide the array in which information
 * about the threads will be stored (the stacks must be allocated seperatly).
 * Must be called before starting the scheduler.
 *
 * @param arr The allocated array
 * @param arr_size The size allocated for the arr
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
int kthread_start_scheduler(void);

/**
 * This function is used to yield execution back to the scheduler
 *
 * @returns Returns 0 on success and 1 on failure.
 */
int kyield(void);

static void reset_round_robin_priority(void);

static struct kthread_t *kthreads_arr; /**< The array in which information is
					**< stored about the threads
					*/
static size_t kthreads_arr_allocated_size; /**< The allocated size of
					    **< kthreads_arr
					    */
static size_t kthreads_arr_used_size; /**< The used size of kthreads_arr */
static int kmax_priority_id; /**< Stores the id of the task with the highest
			      **< priority. If that is unknown, then it
			      **< stores the value -1
			      */
static int kstarted_scheduler;
static int kcurrent_thread_id;

int
kprovide_threads_array(struct kthread_t *arr, size_t arr_size)
{
	size_t i;

	if (!arr || !arr_size)
		return 1;
	
	if (kthreads_arr || kthreads_arr_allocated_size ||
	    kthreads_arr_used_size)
		return 1;

	if (kstarted_scheduler)
		return 1;

	kthreads_arr = arr;
	kthreads_arr_allocated_size = arr_size;
	kthreads_arr_used_size = 0;

	for (i = 0; i < kthreads_arr_allocated_size; i++)
		kthreads_arr[i].status = SUSPENDED;

	return 0;
}

int
kthread_create_static(void (*func)(void *), void *args, void *stack,
		      size_t stack_size, uint8_t priority)
{
	if (!func || !stack || !stack_size)
		return 1;

	if (kstarted_scheduler)
		return 1;

	if (kthreads_arr_used_size >= kthreads_arr_allocated_size)
		return 1;

	kthreads_arr[kthreads_arr_used_size].id = kthreads_arr_used_size + 1;
	kthreads_arr[kthreads_arr_used_size].priority = priority;
	kthreads_arr[kthreads_arr_used_size].rr_priority = BASE_RR;
	kthreads_arr[kthreads_arr_used_size].status = READY;

	kthreads_arr_used_size++;

	return 0;
}

int
kthread_suspend(int id)
{
	if (id < 0 || (size_t)id > kthreads_arr_used_size)
		return 1;

	if (id == 0) {
		id = kcurrent_thread_id;

		if (id == 0)
			return 1;
	}

	kthreads_arr[id - 1].status = SUSPENDED;

#if 0
	/* if not in atomic block */
	if (id == kcurrent_thread_id)
		kyield();
	
	reset round robin priority
#endif
	reset_round_robin_priority();
}

int
kthread_unsuspend(int id)
{
	if (id < 0 || (size_t)id > kthreads_arr_used_size)
		return 1;

	kthreads_arr[id - 1].status = READY;
#if 0
	/* if not in atomic block */
	if (kthreads_arr[id - 1].priority >
	    kthreads_arr[kcurrent_thread_id].priority)
		kyield();
	
	reset round robin priority
#endif
	reset_round_robin_priority();
}

/* TODO */
int
kthread_start_scheduler(void)
{
}

/* TODO */
int
kyield(void)
{
}

int
get_next_id(void)
{
	size_t i, index; 
	uint8_t max_priority = 0, max_rr_priority = 0;

	for (i = 0; i < kthreads_arr_used_size; i++) {
		if (kthreads_arr[i].status == SUSPENDED)
			continue;

		if (kthreads_arr[i].priority > max_priority) {
			max_priority = kthreads_arr[i].priority;
			max_rr_priority = kthreads_arr[i].rr_priority;
			index = i;
		}
		if (kthreads_arr[i].priority == max_priority &&
		    kthreads_arr[i].rr_priority > max_rr_priority) {
		    	max_rr_priority = kthreads_arr[i].rr_priority;
			index = i;
		}
	}

	if (!max_priority)
		return -1;
	
	return index + 1;
}

static void
reset_round_robin_priority(void)
{
	size_t i;

	for (i = 0; i < kthreads_arr_used_size; i++)
		kthreads_arr[i].rr_priority = BASE_RR;
}

int
main(void)
{
	int i, k;
	static struct kthread_t kthreads_allocated[10];
	struct kthread_t x;

	kprovide_threads_array(kthreads_allocated, 10);

	kthread_create_static(1, 1, 1, 1, 3);
	kthread_create_static(1, 1, 1, 1, 2);
	kthread_create_static(1, 1, 1, 1, 2);
	kthread_create_static(1, 1, 1, 1, 1);
	kthread_create_static(1, 1, 1, 1, 1);

	for (i = 0; i < 10; i++) {
		x = kthreads_arr[i];

		printf("%d %d %d\n", i, x.id, x.rr_priority);

		if (x.rr_priority == 0)
			reset_round_robin_priority();
	}

	kthread_suspend(1);

	k = 0;
	while (k++ < 10) {
		i = get_next_id() - 1;

		printf("%d %d\n", kthreads_arr[i].id,
				  kthreads_arr[i].rr_priority);
		kthreads_arr[i].rr_priority--;


		if (kthreads_arr[i].rr_priority == 0)
			reset_round_robin_priority();
	}

	return 0;
}

