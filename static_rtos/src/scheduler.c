/*
 * TODO: blocked state
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
static void idle_thread(void *args);

static struct kthread_t *kthreads_arr; /**< The array in which information is
					**< stored about the threads
					*/
static size_t kthreads_arr_allocated_size; /**< The allocated size of
					    **< kthreads_arr
					    */
static size_t kthreads_arr_used_size; /**< The used size of kthreads_arr */
static uint16_t ktickcount;
static int kstarted_scheduler;
static int kcurrent_thread_id;
static mcu_context_t kscheduler_context;
static mcu_context_t kidle_thread_context;
void *kidle_thread_stack;
size_t kidle_thread_stack_size;

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
kprovide_idle_thread_stack(void *stack, size_t stack_size)
{
	if (!stack || !stack_size)
		return 1;

	kidle_thread_stack = stack;
	kidle_thread_stack_size = stack_size;

	return 0;
}

int
kthread_create_static(void (*func)(void *), void *args, void *stack,
		      size_t stack_size, uint8_t priority)
{
	if (!func || !stack || !stack_size)
		return -1;
	
	if (priority == 0 || priority == UINT8_MAX)
		return -1;

	if (kstarted_scheduler)
		return -1;

	if (kthreads_arr_used_size >= kthreads_arr_allocated_size)
		return -1;

	kthreads_arr[kthreads_arr_used_size].stack_size = stack_size;
	kthreads_arr[kthreads_arr_used_size].func = func;
	kthreads_arr[kthreads_arr_used_size].args = args;
	kthreads_arr[kthreads_arr_used_size].stack = stack;
	kthreads_arr[kthreads_arr_used_size].status = READY;
	kthreads_arr[kthreads_arr_used_size].id = kthreads_arr_used_size + 1;
	kthreads_arr[kthreads_arr_used_size].wake_up_at = 0;
	kthreads_arr[kthreads_arr_used_size].priority = priority;
	kthreads_arr[kthreads_arr_used_size].last_run = 0;
	kthreads_arr[kthreads_arr_used_size].wake_scheduled = 0;

	kthreads_arr_used_size++;

	return kthreads_arr_used_size;
}

int
kthread_suspend(int id)
{
	if (id < 0 || (size_t)id > kthreads_arr_used_size)
		return 1;

	if (id == 0) {
		id = kcurrent_thread_id;

		if (id <= 0)
			return 1;
	}

	kthreads_arr[id - 1].status = SUSPENDED;

	/* if not in atomic block (TODO) */
	if (id == kcurrent_thread_id && !PORT_IS_ATOMIC())
		return kyield();

	return 0;
}

int
kthread_unsuspend(int id)
{
	if (id <= 0 || (size_t)id > kthreads_arr_used_size)
		return 1;
	
	kthreads_arr[id - 1].status = READY;
	if (kthreads_arr[id - 1].priority >
	    kthreads_arr[kcurrent_thread_id].priority &&
	    !PORT_IS_ATOMIC())
		kyield();

	return 0;
}

/* TODO */
int
kscheduler_start(void)
{
	int i;

	if (kstarted_scheduler)
		return 1;
	

	/* make the scheduler context and the context for all of the threads */
	port_getcontext(&kscheduler_context);
	port_getcontext(&kidle_thread_context);
	port_makecontext(&kidle_thread_context, kidle_thread_stack,
			 kidle_thread_stack_size, &kscheduler_context,
			 idle_thread, NULL);
	if (kmake_context_for_all_threads())
		return 1;
	
	/* disable interrupts inside of the scheduler. not doing this will
	 * cause the timer interrupt to save the scheduler context in the wrong
	 * place and make it run out of memory causing a reset
	 */
	PORT_DISABLE_INTERRUPTS();
	kstarted_scheduler = 1;
	while (1) {
		i = get_next_id();
		if (i < 0)
			i = 0;
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
	/* or kis_critical */
	if (!kstarted_scheduler || kcurrent_thread_id < 0)
		return 1;
	if (kcurrent_thread_id == 0) {
		return port_swapcontext(&kidle_thread_context,
					&kscheduler_context);
	} else {
		return port_swapcontext(
				&kthreads_arr[kcurrent_thread_id - 1].context,
				&kscheduler_context
				       );
	}
	return 0;
}

int
kenable_tick_interrupt(void)
{
	return port_enable_tick_interrupt();
}

int
ksleep_for_ticks(uint16_t ticks_count)
{
	int id, ret, interrupts;
	if (!kstarted_scheduler)
		return 1;

	id = kcurrent_thread_id;
	if (id <= 0)
		return 1;
	
	/* Interrupts disabled inside of this function so the tick won't
	 * increase unexpectendly. I don't really know why, but not disabling
	 * interrupts caused boot looping
	 */
	interrupts = PORT_ARE_INTERRUPTS_ENABLED();
	if (interrupts)
		PORT_DISABLE_INTERRUPTS();
	
#if 0
	TODO
	if (UINT16_MAX - 
#endif

	kthreads_arr[id - 1].wake_scheduled = 1;
	kthreads_arr[id - 1].wake_up_at = ktickcount + ticks_count;

	/* using this instead of suspend */
	kthreads_arr[id - 1].status = SUSPENDED; /* TODO: blocked, waiting */
	ret = kyield();

	kthreads_arr[id - 1].wake_scheduled = 0;

	if (interrupts)
		PORT_ENABLE_INTERRUPTS();

	return ret;
}

int
kincrease_tickcount(void)
{
	int ret;
	size_t i;

	if (!kstarted_scheduler)
		return 0;
	
	ktickcount++;
	
	if (ktickcount == 0) {
			for (i = 0; i < kthreads_arr_used_size; i++) {
				if (kthreads_arr[i].wake_scheduled != 2)
					continue;
			kthreads_arr[i].wake_scheduled = 1;
		}
	}

	ret = 0;
	for (i = 0; i < kthreads_arr_used_size; i++) {
		if (kthreads_arr[i].wake_scheduled != 1)
			continue;
		if (kthreads_arr[i].wake_up_at <= ktickcount) {
			kthread_unsuspend(kthreads_arr[i].id);
			ret = 1;
		}
	}

	return ret;
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
 * @return 0 if there is no READY thread
 *	   the id of the next thread to run
 */
static int
get_next_id(void)
{
	size_t i, last_run_index, first_index;
	uint8_t max_priority, set_last_run_index;

	max_priority = 0;
	set_last_run_index = 0;
	first_index = 0;
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
		return 0;
	
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
	if (id < 0)
		return 1;
	kcurrent_thread_id = id;
	if (id == 0) {
		return port_swapcontext(&kscheduler_context,
					&kidle_thread_context);
	} else {
		make_0_last_run_for_priority(kthreads_arr[id - 1].priority);
		kthreads_arr[id - 1].last_run = 1;
		return port_swapcontext(&kscheduler_context,
					&kthreads_arr[id - 1].context);
	}

	return 1;
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

/**
 * This is the thread that is executed whenever there is no other active thread
 */
static void
idle_thread(void *args)
{
	(void)args;

	/* might have to remove this */
	PORT_ENABLE_INTERRUPTS();
	while (1) {
	}
}

/* Timer interrupts is port defined */

#include <port/port_timer.h>

