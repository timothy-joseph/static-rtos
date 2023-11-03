#ifndef PORTABLE_H
#define PORTABLE_H

/* TODO:
 * separate this into different files: .h and .c
 * and thus make some of the functions static
 */

#include <stdint.h>
#include <avr/io.h>
#include <avr/interrupts.h>
#include "../scheduler.h"

enum __PORT_ATOMIC {
	__PORT_ATOMIC_BEGIN,
	__PORT_ATOMIC_END
};

uint8_t __PORT_ATOMIC_SECTION(uint8_t begin);
uint8_t PORT_BEGIN_CRITICAL(void);
uint8_t PORT_END_CRITICAL(void);

/* if interrupts are enabled, then when this
 * function begins a atomic section
 * it counts the number of nested atomic
 * sections
 *
 * return 0 on success and 1 on failure
 */
uint8_t
__PORT_ATOMIC_SECTION(uint8_t begin)
{
	static uint8_t interrupts_were_enabled = 0;

	switch (begin) {
	case ATOMIC_BEGIN:
		if (interrupts_were_enabled) {
			interrupts_were_enabled++;
			return 0;
		}
		interrupts_were_enabled = !!(SREG & (1 << 7));
		cli();
		return 0;
		break;

	case ATOMIC_END:
		if (interrupts_were_enabled) {
			interrupts_were_enabled--;
			if (interrupts_were_enabled == 0)
				sei();
		}
		return 0;
		break;
	}

	return 1;
}

/* retuns 0 on success, 1 on failure
 */
uint8_t
PORT_BEGIN_CRITICAL(void)
{
	return __PORT_ATOMIC_SECTION(__PORT_ATOMIC_BEGIN);
}

/* retuns 0 on success, 1 on failure
 */
uint8_t
PORT_END_CRITICAL(void)
{
	return __PORT_ATOMIC_SECTION(__PORT_ATOMIC_END);
}

#endif
