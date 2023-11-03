#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <stdint.h>
#include <stddef.h>
#include "portable/context.h"

/* TODO:
 * conditional yield (yield if higher priority function
 * is ready)
 * provide isr safe functions for all of the switching
 * functions
 */

enum THREAD_STATES {
	RUNNING_STATE,
	READY_STATE,
	SUSPENDED_STATE
};

/* up to 255 threads */
struct kthread {
	uint8_t            id;
	uint8_t            state;
	uint8_t            priority;
	uint32_t           notifications;
	void               *stack;
	size_t             stack_size;
	struct mcu_context thread_context;
};

/* the following 2 functions are used
 * to enter and exit critical/atomic
 * sections of code
 */
uint8_t KBEGIN_CRITICAL(void);
uint8_t KEXIT_CRITICAL(void);
/* TODO */
uint8_t KIS_CRITICAL(void);
/* queue size is int16_t, because we can have at max 255 threads due
 * to the id limitation i set. it's represented on 16 bits instead
 * of 8
 */
uint8_t kprovide_queue_static(struct kthread *queue, int16_t queue_size);
uint8_t kcreate_thread_static(void (*func)(void *), void *args, void *stack,
                              size_t stack_size, uint8_t priority);
uint8_t ksuspend_thread(uint8_t id);
uint8_t kunsuspend_thread(uint8_t id);
uint8_t kyield(void);
uint8_t kscheduler(void);

void print_id_ready(void);
void print_id_suspended(void);

#endif
