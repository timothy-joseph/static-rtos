#include <stdio.h>
#include <stdlib.h>
#include "scheduler.h"

int
main(void)
{
	int i;
	static struct kthread q[5];

	kprovide_queue_static(q, sizeof(q) / sizeof(*q));

	printf("%d\n", kcreate_thread_static(NULL, NULL, NULL, 0, 1));
	printf("%d\n", kcreate_thread_static(NULL, NULL, NULL, 0, 2));
	printf("%d\n", kcreate_thread_static(NULL, NULL, NULL, 0, 3));
	printf("%d\n", kcreate_thread_static(NULL, NULL, NULL, 0, 5));
	printf("%d\n", kcreate_thread_static(NULL, NULL, NULL, 0, 4));

	printf("pass 1:\n");
	print_id_ready();
	print_id_suspended();

	ksuspend_thread(1);
	printf("pass 2:\n");
	print_id_ready();
	print_id_suspended();

	ksuspend_thread(4);
	printf("pass 3:\n");
	print_id_ready();
	print_id_suspended();

	ksuspend_thread(3);
	printf("pass 4:\n");
	print_id_ready();
	print_id_suspended();

	kunsuspend_thread(5);
	printf("pass 5:\n");
	print_id_ready();
	print_id_suspended();

	kunsuspend_thread(4);
	printf("pass 6:\n");
	print_id_ready();
	print_id_suspended();

	kunsuspend_thread(1);
	printf("pass 6:\n");
	print_id_ready();
	print_id_suspended();


	return 0;
}
