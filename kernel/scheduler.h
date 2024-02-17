#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <stdint.h>
#include <stddef.h>
#include "portable/context.h"

/* TODO:
 * conditional yield (yield if higher priority function is ready)
 * provide isr safe functions for all of the switching functions
 */

enum THREAD_STATES {
	RUNNING_STATE,
	READY_STATE,
	SUSPENDED_STATE
};

/* variables in struct should be ordered by size in descending order and then by
 * name
 */
/* up to 255 threads due to the id being represented on 8 bits */
struct kthread_t {
	struct mcu_context_t thread_context;
	uint32_t notifications;
	void *stack;
	size_t stack_size;
	uint8_t id;
	uint8_t priority;
	uint8_t state;
};

/* the following 2 functions are used to enter and exit critical/atomic sections
 * of code
 */
uint8_t KBEGIN_CRITICAL(void);
uint8_t KEXIT_CRITICAL(void);
/* TODO */
uint8_t KIS_CRITICAL(void);

uint8_t kprovide_threads_array_static(struct kthread_t *arr, size_t arr_size);
uint8_t kcreate_thread_static(void (*func)(void *), void *args, void *stack,
			      size_t stack_size, uint8_t priority);
uint8_t ksuspend_thread(uint8_t id);
uint8_t kunsuspend_thread(uint8_t id);
uint8_t kyield(void);
uint8_t kscheduler(void);

void print_id_ready(void);
void print_id_suspended(void);

#endif
