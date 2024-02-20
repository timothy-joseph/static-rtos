#ifndef AVR_PORT_TIMER_H
#define AVR_PORT_TIMER_H

#include <avr/io.h>
#include <avr/interrupt.h>

ISR(TIMER1_OVF_vect)
{
	TCNT1 = TCNT1_1MS;

	if (!kstarted_scheduler)
		return;
	
	kincrease_tickcount();
	kyield();
}

#endif

