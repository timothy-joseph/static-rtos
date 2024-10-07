/*
 * Copyright 2024 Timothy Joseph. Subject to MIT license
 * See LICENSE.txt for details
 */
#include <stdio.h>

#include <static_rtos/kernel/scheduler.h>
#include <static_rtos/port/ports/avr_port.h>

#include <avr/io.h>
#include <avr/interrupt.h>

ISR(TIMER1_OVF_vect)
{
	TCNT1 = TCNT1_1MS;

	if (!kscheduler_has_started())
		return;
	
	if (kincrease_tickcount())
		kyield();
}

