/*
 * Idea about switching context in a ISR: port dependendant
 *	-> put another return address
 *
 * How to port: port.h universal structure
 */
#include <stdio.h>
#include <kernel/scheduler.h>
#include <port/port.h>

static int kmake_context_for_all_threads(void);
static int get_next_id(void);
static void make_0_last_run_for_priority(uint8_t priority);
static int kswitch_to_thread_by_id(int id);

static struct kthread_t *kthreads_arr; /**< The array in which information is
					**< stored about the threads
					*/
static size_t kthreads_arr_allocated_size; /**< The allocated size of
					    **< kthreads_arr
					    */
static size_t kthreads_arr_used_size; /**< The used size of kthreads_arr */
static int kstarted_scheduler;
static int kcurrent_thread_id;
static mcu_context_t kscheduler_context;

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
	kthreads_arr[kthreads_arr_used_size].last_run = 0;
	kthreads_arr[kthreads_arr_used_size].status = READY;
	kthreads_arr[kthreads_arr_used_size].func = func;
	kthreads_arr[kthreads_arr_used_size].args = args;
	kthreads_arr[kthreads_arr_used_size].stack = stack;
	kthreads_arr[kthreads_arr_used_size].stack_size = stack_size;

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
kscheduler_start(void)
{
	int i;

	if (kstarted_scheduler)
		return 1;
	
	kstarted_scheduler = 1;

	/* make the scheduler context and the context for all of the threads */
	port_getcontext(&kscheduler_context);
	if (kmake_context_for_all_threads())
		return 1;
	
	while (1) {
		i = get_next_id();
		if (i <= 0)
			continue;
		if (kswitch_to_thread_by_id(i) == -1) {
			/* TODO: error handler */
			printf("switch error\n");
		}
	}
}

/* TODO: critical blocks */
int
kyield(void)
{
	if (!kstarted_scheduler || kcurrent_thread_id <= 0)
		return 1;
	port_swapcontext(&kthreads_arr[kcurrent_thread_id - 1].context,
			 &kscheduler_context);
	return 0;
}

/**
 * This is a internal function used to make the context of all threads.
 * It is called at the beginning of kstart_scheduler
 *
 * @return If port_getcontext fails, then it will return 1, and 0 otherwise
 */
static int
kmake_context_for_all_threads(void)
{
	size_t i;

	for (i = 0; i < kthreads_arr_used_size; i++) {
		if (port_getcontext(&kthreads_arr[i].context) != 0)
			return 1;
		port_makecontext(&kthreads_arr[i].context,
				 kthreads_arr[i].stack,
				 kthreads_arr[i].stack_size,
				 &kscheduler_context,
				 kthreads_arr[i].func,
				 kthreads_arr[i].args);
	}

	return 0;
}

/**
 * This is a internal function used inside of the scheduler function to get the
 * id of the next thread to execute
 *
 * @return -1 if there is no READY thread
 *	   the id of the next thread to run
 */
static int
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
		    kthreads_arr[i].last_run) {
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

/**
 * This is a internal function used to switch the context from the scheduler to
 * the thread with id = id. It sets the global variable kcurrent_thread_id.
 *
 * @param id The id of the thread to switch context to
 *
 * @return same as port_swapcontext
 */
static int
kswitch_to_thread_by_id(int id)
{
	kcurrent_thread_id = id;
	make_0_last_run_for_priority(kthreads_arr[id - 1].priority);
	kthreads_arr[id - 1].last_run = 1;
	return port_swapcontext(&kscheduler_context,
				&kthreads_arr[id - 1].context);
}

/**
 * This is a internal function that sets the .last_run field of all of the
 * threads to 0. This is done to ensure a round-robin like execution for threads
 * with the same priority
 */
static void
make_0_last_run_for_priority(uint8_t priority)
{
	size_t i;

	for (i = 0; i < kthreads_arr_used_size; i++)
		if (kthreads_arr[i].priority == priority)
			kthreads_arr[i].last_run = 0;
}

