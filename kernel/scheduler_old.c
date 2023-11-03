#ifndef SCHEDULER_C
#define SCHEDULER_C

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "scheduler.h"
#include "portable/context.h"
#include "portable/portable.h"

/* TODO:
 * priority queue implementation
 * kerrno
 */

static void __swap_threads(struct kthread *a, struct kthread *b);

static struct kthread *kqueue;
static int16_t          kqueue_size;
/* the end of the ready threads, grows towards the right */
static int16_t         kqueue_end;
/* the end of the suspended threads, grows towards the left */
static int16_t         kqueue_suspended_end;
static uint8_t         klast_id = 0;
static struct kthread *kcurrent_thread = 0xff; /* TODO: modify */

/* used to enter atomic sections of code
 * retuns 0 on success, 1 on failure
 */
uint8_t
KBEGIN_CRITICAL(void)
{
	return PORT_BEGIN_CRITICAL();
}

/* used to end atomic sections of code
 * retuns 0 on success, 1 on failure
 */
uint8_t
KEND_CRITICAL(void)
{
	return PORT_END_CRITICAL();
}

/* the queue parameter should be a statically allocated
 * array of the number of threads you are going to
 * use, and queue_size is supposed to be the number of
 * threads that you allocated memory for
 *
 * the function modifies static global variables
 *
 * returns 1 when given a invalid size value
 * returns 0 on success
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

/* used to create a new thread with statically allocated stack
 * returns a pointer to the thread structure on success
 * returns NULL on failure
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
	/* TODO: heapify */

exit_label:
	KEND_CRITICAL();
	return return_id;
}

/* returns 1 on failure and 0 on success
 * if we aren't in a critical section when the
 * function was called, then the function context
 * immedietly switches, otherwise, it will
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

	/* TODO: heapify */

	KEND_CRITICAL();

	return 0;
}


/* returns 1 on failure and 0 on success
 * if a higher priority function was unsuspended and
 * we aren't in a critical section,
 * then the context immedietly switches to that
 * function. otherwise, it will switch at the first tick
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
	kqueue_end++;
	kqueue[i].state = READY_STATE;
	__swap_threads(&kqueue[i], &kqueue[kqueue_end]);

	if (kqueue_end != kqueue_suspended_end) {
		for (; i > kqueue_suspended_end; i--) {
			__swap_threads(&kqueue[i], &kqueue[i - 1]);
		}
	}
	kqueue_suspended_end++;

	/* TODO: heapify */

	KEND_CRITICAL();

	return 0;
}

/* a internal function of the scheduler module
 * used to more easily swap two kthread structs
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

void
print_id_ready(void)
{
	uint8_t i;

	for (i = 0; i <= kqueue_end; i++)
		printf("%d ", kqueue[i].id);
	printf("\n");
}

void
print_id_suspended(void)
{
	uint8_t i;

	for (i = kqueue_suspended_end; i < kqueue_size; i++)
		printf("%d ", kqueue[i].id);
	printf("\n");
}

#endif
