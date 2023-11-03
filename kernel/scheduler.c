#ifndef SCHEDULER_C
#define SCHEDULER_C

#include <stdio.h>
#include <stdint.h>
#include <string.h>

/* TODO: include folder */
#include "scheduler.h"
#include "portable/context.h"
#include "portable/portable.h"

/* TODO:
 * priority queue implementation
 * kerrno
 * KIS_CRITICAL
 */

/* functions only used in this translation unit */
static void __swap_threads(struct kthread *a, struct kthread *b);
static int __kthreads_cmp(const void *a, const void *b);
static void __heapify(void);

static struct kthread *kqueue;
static int16_t          kqueue_size;
/* the end of the ready threads, grows towards the right */
static int16_t         kqueue_end;
/* the end of the suspended threads, grows towards the left */
static int16_t         kqueue_suspended_end;
static uint8_t         klast_id = 0;
static struct kthread *kcurrent_thread = 0xff; /* TODO: modify */
static uint8_t         kscheduler_started = 0;

/* used to begin atomic sections of code
 * retuns 0 on success, 1 on failure
 *
 * implementations should take into consideration the nesting of
 * multiple atomic sections
 */
uint8_t
KBEGIN_CRITICAL(void)
{
	return PORT_BEGIN_CRITICAL();
}

/* used to end atomic sections of code
 * retuns 0 on success, 1 on failure
 *
 * implementations should take into consideration the nesting of
 * multiple atomic sections
 */
uint8_t
KEND_CRITICAL(void)
{
	return PORT_END_CRITICAL();
}

/* the queue parameter should be a statically allocated array of the
 * number of threads you are going to use, and queue_size is supposed
 * to be the number of threads that you allocated memory for
 *
 * the function modifies static global variables
 *
 * returns 1 when given a invalid size value; returns 0 on success
 */
uint8_t
kprovide_queue_static(struct kthread *queue, int16_t queue_size)
{
	if (queue_size < 0 || queue_size > 255)
		return 1;
	kqueue = queue;
	kqueue_size = queue_size;
	kqueue_end = -1;
	kqueue_suspended_end = queue_size;
	memset(kqueue, 0, queue_size * sizeof(*kqueue));
	return 0;
}

/* used to create a new thread with statically allocated stack returns
 * a pointer to the thread structure on success * returns NULL on
 * failure
 *
 * the returned pointer can be used with the ksuspend, kunsuspend,
 * ksend_notification, and ksend_notification_with_unsuspend functions
 */
uint8_t
kcreate_thread_static(void (*func)(void *), void *args, void *stack,
                      size_t stack_size, uint8_t priority)
{
	uint8_t return_id = 0;

	KBEGIN_CRITICAL();

	if (kqueue == NULL || kqueue_size == 0)
		goto exit_label;
	if (kqueue_end + 1 >= kqueue_size ||
	    kqueue_end + 1 >= kqueue_suspended_end)
		goto exit_label;
	if (klast_id == 0xff)
		goto exit_label;

	kqueue_end++;
	klast_id++;
	return_id = klast_id;

	kqueue[kqueue_end].id = klast_id;
	kqueue[kqueue_end].state = READY_STATE;
	kqueue[kqueue_end].priority = priority;
	kqueue[kqueue_end].notifications = 0;
	kqueue[kqueue_end].stack = stack;
	kqueue[kqueue_end].stack_size = stack_size;

	/* TODO: context starting at func */
	__heapify();

exit_label:
	KEND_CRITICAL();
	return return_id;
}

/* returns 1 on failure and 0 on success
 * if we aren't in a critical section when the function was called,
 * then the function context immedietly switches, otherwise, it will
 * switch at the first tick
 *
 * O(n) time
 */
uint8_t
ksuspend_thread(uint8_t id)
{
	uint8_t i;

	KBEGIN_CRITICAL();

	if (kqueue == NULL || kqueue_size == 0 || kcurrent_thread == NULL) {
		KEND_CRITICAL();
		return 1;
	}
	if (kqueue_end == -1 ||
	    kqueue_suspended_end - 1 < 0) {
	    	KEND_CRITICAL();
	    	return 1;
	}

	if (id == 0)
		id = kcurrent_thread->id;
	for (i = 0; i <= kqueue_end; i++) {
		if (kqueue[i].id == id)
			break;
	}
	if (i > kqueue_end) {
	    	KEND_CRITICAL();
	    	return 1;
	}
	
	kqueue_suspended_end--;
	kqueue[i].state = SUSPENDED_STATE;
	__swap_threads(&kqueue[i], &kqueue[kqueue_suspended_end]);

	if (kqueue_end != kqueue_suspended_end) {
		for (; i < kqueue_end; i++) {
			__swap_threads(&kqueue[i], &kqueue[i + 1]);
		}
	}
	kqueue_end--;

	__heapify();

	KEND_CRITICAL();

	return 0;
}


/* returns 1 on failure and 0 on success if a higher priority function
 * was unsuspended and * we aren't in a critical section, then the
 * context immedietly switches to that function. otherwise, it will
 * switch at the first tick
 *
 * O(n) time
 */
uint8_t
kunsuspend_thread(uint8_t id)
{
	uint8_t i;

	KBEGIN_CRITICAL();

	if (kqueue == NULL || kqueue_size == 0 || kcurrent_thread == NULL) {
		KEND_CRITICAL();
		return 1;
	}
	if (kqueue_suspended_end == kqueue_size ||
	    kqueue_end + 1 >= kqueue_size) {
		KEND_CRITICAL();
		return 1;
	}

	if (id == 0)
		id = kcurrent_thread->id;
	for (i = kqueue_suspended_end; i < kqueue_size; i++) {
		if (kqueue[i].id == id)
			break;
	}
	if (i >= kqueue_size) {
		KEND_CRITICAL();
		return 1;
	}
	kqueue_end++;
	kqueue[i].state = READY_STATE;
	__swap_threads(&kqueue[i], &kqueue[kqueue_end]);

	if (kqueue_end != kqueue_suspended_end) {
		for (; i > kqueue_suspended_end; i--) {
			__swap_threads(&kqueue[i], &kqueue[i - 1]);
		}
	}
	kqueue_suspended_end++;

	__heapify();

	KEND_CRITICAL();

	return 0;
}

/* a internal function of the scheduler module used to more easily
 * swap two kthread structs
 */
static void
__swap_threads(struct kthread *a, struct kthread *b)
{
	struct kthread tmp;

	KBEGIN_CRITICAL();

	if (a == NULL || b == NULL) {
		KEND_CRITICAL();
		return;
	}

	tmp = *a;
	*a = *b;
	*b = tmp;

	KEND_CRITICAL();
}


/* used to yield the context back to the scheduler
 * returns 0 on success and 1 on failure
 */
uint8_t
kyield(void)
{
}

/* the main function of the scheduler
 * it decides based on the priority queue which thread should run. the
 * after the first scheduling, the priority queue will have on
 * position 0 the running thread and on positions 1...kqueue_end will
 * be the actual priority_queue. it checks if a thread wake up due to a
 * timer event * the scheduler heapifies on every tick. 
 *
 * this function should never return upon succesful execution and it
 * should return 1 on failure
 */
uint8_t
kscheduler(void)
{
}

/* this function is the isr that increments the tick and also
 * heapifies.
 *
 * within your port, your timer isr should call this function
 * and also reset the count timer
 */
void
kscheduler_tick_isr(void)
{
}


/* a internal function of the scheduler module used to compare two
 * thread's priority
 */
static int
__kthreads_cmp(const void *a, const void *b)
{
	return ((struct kthread *)b)->priority -
	       ((struct kthread *)a)->priority;
}

/* heapifies the priority queue between 0...kqueue_end on if the
 * scheduler hasn't started and between 1...kqueue_end on all other
 * calls
 */
static void
__heapify(void)
{
	uint8_t start;

	if (kqueue == NULL || kqueue_end <= -1)
		return;

	if (!kscheduler_started)
		start = 0;
	else
		start = 1;

	/* TODO: actual priority_queue */
	qsort(kqueue + start, kqueue_end - start + 1, sizeof(struct kthread),
	      __kthreads_cmp);
}

/* TODO: remove */
void
print_id_ready(void)
{
	uint8_t i;

	for (i = 0; i <= kqueue_end; i++)
		printf("%d ", kqueue[i].id);
	printf("\n");
}

/* TODO: remove */
void
print_id_suspended(void)
{
	uint8_t i;

	for (i = kqueue_suspended_end; i < kqueue_size; i++)
		printf("%d ", kqueue[i].id);
	printf("\n");
}

#endif
