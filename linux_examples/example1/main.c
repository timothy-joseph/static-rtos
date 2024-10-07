#include <stdio.h>
#include <stdint.h>
#include <static_rtos/kernel/scheduler.h>

void thread1(void *args);
void thread2(void *args);

void
thread1(void *args)
{
	(void)args;

	while (1) {
		printf("thread1\n");
		kyield();
	}
}

void
thread2(void *args)
{
	(void)args;

	while (1) {
		printf("thread2\n");
		kyield();
	}
}

int
main(void)
{
	static struct kthread_t threads[2];
	static uint8_t thread1_stack[13680];
	static uint8_t thread2_stack[13680];

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

