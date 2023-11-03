#ifndef PORTABLE_H
#define PORTABLE_H

/* TODO:
 * separate this into different files: .h and .c
 * and thus make some of the functions static
 */

#include <stdint.h>
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
}

/* retuns 0 on success, 1 on failure
 */
uint8_t
PORT_BEGIN_CRITICAL(void)
{
}

/* retuns 0 on success, 1 on failure
 */
uint8_t
PORT_END_CRITICAL(void)
{
}

#endif
