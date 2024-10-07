/*
 * Copyright 2024 Timothy Joseph. Subject to MIT license
 * See LICENSE.txt for details
 */
/*
 * TODO:
 *
 * -> mutexes
 * -> don't swap the context to the scheduler on every tick
 */
#include <stdio.h>
#include <static_rtos/kernel/scheduler.h>
#include <static_rtos/port/port.h>

/* macros */

#define K_ID_TO_INDEX(ID) ((ID) - 1)

/* types */

enum wakeup_reason_t {
	NO_REASON,
	SLEEP_SCHEDULED,
	SLEEP_SCHEDULED_OVERFLOW,
	MUTEX_SCHEDULED
};

/* function declarations */

static int kmake_context_for_all_threads(void);
static int get_next_id(void);
static void make_0_last_run_for_priority(uint8_t priority);
static int kswitch_to_thread_by_id(int id);

/* global variables */

static struct kthread_t *kthreads_arr; /**< The array in which information is
					**< stored about the threads
					*/
static size_t kthreads_arr_allocated_size; /**< The allocated size of
					    **< kthreads_arr
					    */
static size_t kthreads_arr_used_size; /**< The amount of threads that have been
				       **< initialized
				       */
static uint16_t ktickcount; /**< The tick count that is increased from the tick
			     **< isr (TODO: ticktype_t)
			     */
static int kstarted_scheduler; /**< flag used internally to determine if the
				**< scheduler is running
				*/
static int kcurrent_thread_id; /**< the id of the current running thread
				**< 0 = the idle thread, but the idle thread
				**< isn't in kthreads_arr, so a function is
				**< used to translate the id to a index
				*/
/* TODO: describe these */
static mcu_context_t kscheduler_context;

/* function definitions */

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

	kthreads_arr[K_ID_TO_INDEX(id)].status = SUSPENDED;

	if (id == kcurrent_thread_id && !PORT_IS_ATOMIC())
		return kyield();

	return 0;
}

int
kthread_unsuspend(int id)
{
	if (id <= 0 || (size_t)id > kthreads_arr_used_size)
		return 1;
	
	kthreads_arr[K_ID_TO_INDEX(id)].status = READY;
	if (kthreads_arr[K_ID_TO_INDEX(id)].priority >
	    kthreads_arr[K_ID_TO_INDEX(kcurrent_thread_id)].priority &&
	    !PORT_IS_ATOMIC())
		kyield();

	return 0;
}

int
kscheduler_start(void)
{
	int i;

	if (kstarted_scheduler)
		return 1;
	

	/* make the scheduler context and the context for all of the threads */
	port_getcontext(&kscheduler_context);
	if (kmake_context_for_all_threads())
		return 1;

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

int
kscheduler_has_started(void)
{
	return kstarted_scheduler;
}

/* TODO: critical blocks */
int
kyield(void)
{
	/* this function can also be used to yield from a isr or when interrupts
	 * are disabled
	 */
	if (!kstarted_scheduler || kcurrent_thread_id < 0)
		return 1;
	
	return kswitch_to_thread_by_id(0);
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
	
	if (UINT16_MAX - ktickcount < ticks_count) {
		kthreads_arr[K_ID_TO_INDEX(id)].wake_scheduled = SLEEP_SCHEDULED_OVERFLOW;
		kthreads_arr[K_ID_TO_INDEX(id)].wake_up_at = ktickcount + ticks_count;
	} else {
		kthreads_arr[K_ID_TO_INDEX(id)].wake_scheduled = SLEEP_SCHEDULED;
		kthreads_arr[K_ID_TO_INDEX(id)].wake_up_at = ktickcount + ticks_count;
	}

	/* using this instead of suspend */
	kthreads_arr[K_ID_TO_INDEX(id)].status = SUSPENDED;
	ret = kyield();

	kthreads_arr[K_ID_TO_INDEX(id)].wake_scheduled = 0;

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
			if (kthreads_arr[i].wake_scheduled !=
			    SLEEP_SCHEDULED_OVERFLOW)
				continue;
			kthreads_arr[i].wake_scheduled = SLEEP_SCHEDULED;
		}
	}

	ret = 0;
	for (i = 0; i < kthreads_arr_used_size; i++) {
		if (kthreads_arr[i].wake_scheduled != 1)
			continue;
		if (kthreads_arr[i].wake_up_at <= ktickcount) {
			kthreads_arr[i].wake_scheduled = 0;
			kthread_unsuspend(kthreads_arr[i].id);
			/* TODO: check if the priority is higher or equal */
			ret = 1;
		}
	}

	/* TODO: check for mutexes */

	return ret;
}

#if 0
int
kenable_tick_interrupt(void)
{
	PORT_ENABLE_INTERRUPTS();
}

int
KARE_INTERRUPTS_ENABLED(void)
{
	return PORT_ARE_INTERRUPTS_ENABLED();
}

int
KBEGIN_ATOMIC(void)
{
}

int
KEND_ATOMIC(void)
{
}

int
KIS_ATOMIC(void)
{
	return PORT_IS_ATOMIC();
}
#endif

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
	int old_id;
	mcu_context_t *old_context, *new_context;

	if (id < 0)
		return 1;

	old_id = kcurrent_thread_id;
	kcurrent_thread_id = id;

	if (old_id == kcurrent_thread_id)
		return 0;
	
	if (old_id == 0)
		old_context = &kscheduler_context;
	else
		old_context = &kthreads_arr[K_ID_TO_INDEX(old_id)].context;

	if (id == 0)
		new_context = &kscheduler_context;
	else
		new_context = &kthreads_arr[K_ID_TO_INDEX(id)].context;

	if (id) {
		make_0_last_run_for_priority(kthreads_arr[K_ID_TO_INDEX(id)].priority);
		kthreads_arr[K_ID_TO_INDEX(id)].last_run = 1;
	}

	return port_swapcontext(old_context, new_context);
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

