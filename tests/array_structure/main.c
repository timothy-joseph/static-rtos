/* TODO: dynamic aging:
 * 1. for threads with the same priority
 * 2. for all threads
 * for threads with the same priority: round robin
 * configurable
 *
 * For threads with the same priority:
 *	dynamic_priority++ if != current_thread
 */
#include <stdio.h>
#include <stdlib.h>
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
	uint8_t dynamic_priority;
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
	
	if (priority == 0 || priority == UINT8_MAX)
		return 1;

	if (kstarted_scheduler)
		return 1;

	if (kthreads_arr_used_size >= kthreads_arr_allocated_size)
		return 1;

	kthreads_arr[kthreads_arr_used_size].id = kthreads_arr_used_size + 1;
	kthreads_arr[kthreads_arr_used_size].priority = priority;
	kthreads_arr[kthreads_arr_used_size].dynamic_priority = 0;
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

	return 0;
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

	return 0;
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
	size_t i, last_run_index, first_index;
	uint8_t max_priority, set_last_run_index;

	max_priority = 0;
	set_last_run_index = 0;
	for (i = 0; i < kthreads_arr_used_size; i++) {
		if (kthreads_arr[i].status == SUSPENDED)
			continue;
		if (kthreads_arr[i].priority > max_priority) {
			max_priority = kthreads_arr[i].priority;
			first_index = i;
		}
		if (kthreads_arr[i].priority == max_priority &&
		    kthreads_arr[i].dynamic_priority) {
			last_run_index = i;
			set_last_run_index = 1;
		}
	}

	if (!max_priority)
		return -1;
	
	if (!set_last_run_index)
		return first_index + 1;
	
	for (i = last_run_index + 1; i < kthreads_arr_used_size; i++) {
		if (kthreads_arr[i].status == SUSPENDED)
			continue;
		if (kthreads_arr[i].priority == max_priority)
			return i + 1;
	}

	return first_index + 1;
}

void
make_0_last_run_for_priority(uint8_t priority)
{
	size_t i;

	for (i = 0; i < kthreads_arr_used_size; i++) {
		if (kthreads_arr[i].priority == priority)
			kthreads_arr[i].dynamic_priority = 0;
	}
}

/**
 * Code that will not be copied to the kernel
 */

void
print_kthreads_arr(void)
{
	size_t i;

	printf("kthreads_arr:\n");
	for (i = 0; i < kthreads_arr_allocated_size; i++) {
		printf("%d %hhu %hhu %d\n", kthreads_arr[i].id,
					    kthreads_arr[i].priority,
					    kthreads_arr[i].dynamic_priority,
					    kthreads_arr[i].status);
	}
	printf("\n");
}

void
test(void)
{
	int n;
	int i, k, type, id, old_id;
	uint8_t priority;
	static struct kthread_t kthreads_allocated[10];
	struct kthread_t x;

	kprovide_threads_array(kthreads_allocated, 10);

	scanf("%d\n", &n);

	if (n > 10) {
		printf("Must make less than 10 threads\n");
		exit(EXIT_FAILURE);
	}

	for (i = 0; i < n; i++) {
		scanf("%hhu", &priority);
		if (kthread_create_static(1, 1, 1, 1, priority) == -1) {
			printf("Error while creating threads\n");
			exit(EXIT_FAILURE);
		}
	}

	scanf("%d\n", &n);
	for (i = 0; i < n; i++) {
		old_id = kcurrent_thread_id;
		k = get_next_id();
		/* put k into execution */
		kcurrent_thread_id = k;

		/* mark that it was run recently */
		if (old_id > 0)
			make_0_last_run_for_priority(
				kthreads_arr[old_id - 1].priority);
		if (kcurrent_thread_id > 0)
			kthreads_arr[kcurrent_thread_id - 1].dynamic_priority = 1;

		printf("%d\n", k);
		scanf("%d %d\n", &type, &id);

		if (type == SUSPENDED) {
			if (kthread_suspend(id)) {
				printf("kthread_suspend returned an error\n");
			}
		}
		if (type == READY) {
			if (kthread_unsuspend(id)) {
				printf("kthread_unsuspend returned an error\n");
			}
		}
	}

	k = get_next_id();
	printf("%d\n", k);
}

int
main(void)
{

	test();
	return 0;
}

