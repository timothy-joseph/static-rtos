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
#include <stdint.h>
#include <kernel/scheduler.h>

void thread1();
void thread2();

int ok = 1;

void
thread1()
{
	while (1) {
		printf("thread1\n");
		kyield();
	}
}

void
thread2()
{
	while (1) {
		printf("thread2\n");
		kyield();
	}
}

int
main(void)
{
	static struct kthread_t threads[2];
	static uint8_t thread1_stack[16384];
	static uint8_t thread2_stack[16384];

	if (kprovide_threads_array(threads, 2))
		printf("threads array problem\n");

	if (kthread_create_static(thread1, NULL, thread1_stack,
				  sizeof(thread1_stack), 1))
		printf("thread1 problem\n");
	if (kthread_create_static(thread2, NULL, thread2_stack,
				  sizeof(thread2_stack), 1))
		printf("thread2 problem\n");

	if (kscheduler_start())
		printf("start scheduler problem\n");

	return 0;
}

