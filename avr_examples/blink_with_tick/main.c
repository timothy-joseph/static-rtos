/* PORTB5 is led built in */
/* TODO: No port functions easily accesible to the user */
#include <stdio.h>
#include <stdint.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/cpufunc.h>
#include <util/delay.h>
#include <static_rtos/kernel/scheduler.h>
#include <stdlib.h>
#include "serial.h"

#define SET(PORT, PIN) ((PORT) = (PORT) | (1 << (PIN)))
#define UNSET(PORT, PIN) ((PORT) = (PORT) & ~(1 << (PIN)))

void led_on_thread(void *args);
void led_off_thread(void *args);

void
led_on_thread(void *args)
{
	(void)args;

	while (1) {
		PORT_BEGIN_ATOMIC();
		printf("led_on_thread\n");
		PORT_END_ATOMIC();
		SET(PORTB, PORTB5);
		if (ksleep_for_ticks(500)) {
			printf("sleep problem1\n");
		}
	}
}

void
led_off_thread(void *args)
{
	(void)args;

	while (1) {
		PORT_BEGIN_ATOMIC();
		printf("led_off_thread\n");
		PORT_END_ATOMIC();
		UNSET(PORTB, PORTB5);
		if (ksleep_for_ticks(700)) {
			printf("sleep problem2\n");
		}
	}
}

int
main(void)
{
	static struct kthread_t threads[2];
	static uint8_t led_on_thread_stack[512];
	static uint8_t led_off_thread_stack[512];

	start_serial(9600);
	printf("starting\n");

	/* initialize the led pin as output */
	SET(DDRB, DDB5);

	if (kprovide_threads_array(threads, 2))
		printf("threads array problem\n");

	if (kthread_create_static(led_on_thread, NULL, led_on_thread_stack,
				  sizeof(led_on_thread_stack), 2) <= 0)
		printf("led_on_thread problem\n");
	if (kthread_create_static(led_off_thread, NULL, led_off_thread_stack,
				  sizeof(led_off_thread_stack), 1) <= 0)
		printf("led_off_thread problem\n");

	if (kenable_tick_interrupt())
		printf("interrupt problem\n");

	if (kscheduler_start())
		printf("start scheduler problem\n");

	return 0;
}

