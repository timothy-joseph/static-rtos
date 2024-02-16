#include <stdio.h>
#include <kernel/scheduler.h>
#include <port/port.h>

static int get_next_id(void);
static int kmake_context_for_all_threads(void);

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
kthread_create_static(void (*func)(), void *args, void *stack,
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
		kcurrent_thread_id = i;
		if (port_swapcontext(&kscheduler_context,
				     &kthreads_arr[i - 1].context) == -1) {
			/* TODO: error handler */
			printf("swap error\n");
		}

	}
}

/* TODO */
int
kyield(void)
{
	if (!kstarted_scheduler || kcurrent_thread_id <= 0)
		return 1;
	port_swapcontext(&kthreads_arr[kcurrent_thread_id - 1].context,
			 &kscheduler_context);
	return 0;
}

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

static int
get_next_id(void)
{
	size_t ret;

	ret = 1;
	if (kcurrent_thread_id > 0)
		ret = kcurrent_thread_id + 1;
	if (ret > kthreads_arr_used_size)
		ret = 1;
	
	return ret;
}

