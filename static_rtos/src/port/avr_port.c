/* Some of the contents of this file are written by and released under the
 * MIT license by another author. Other parts of the contents are written by
 * Timothy Joseph and released under the MIT license
 */
/*
  (Original) Author: Artem Boldariev <artem@boldariev.com>
  Modified by: Timothy Joseph
  Link to the original: https://github.com/arbv/avr-context
  License text (doesn't apply to the whole file)
  MIT/Expat License
  
  Copyright (c) 2020 Artem Boldariev <artem@boldariev.com>
  
  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:
  
  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.
  
  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/

/*
This file contains definitions of the context manipulation functions.
It meant to be included after 'avrcontext.h'.
In general, you should include it only once across the project.
*/

#ifndef AVRCONTEXT_IMPL_H
#define AVRCONTEXT_IMPL_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <port/port.h>

#ifdef __AVR__

__attribute__ ((naked)) int port_getcontext(mcu_context_t *cp)
{
    (void)cp; /* to avoid compiler warnings */
    AVR_SAVE_CONTEXT(
        "",
        "mov r30, r24\n"
        "mov r31, r25\n");
    __asm__ __volatile__ ("ldi r24,0\n");
    __asm__ __volatile__ ("ldi r25,0\n");
    __asm__ __volatile__ ("ret\n");
}

__attribute__ ((naked)) int port_setcontext(const mcu_context_t *cp)
{
    (void)cp; /* to avoid compiler warnings */
    AVR_RESTORE_CONTEXT(
        "mov r30, r24\n"
        "mov r31, r25\n");
    __asm__ __volatile__ ("ldi r24,0\n");
    __asm__ __volatile__ ("ldi r25,0\n");
    __asm__ __volatile__ ("ret\n");
}


__attribute__ ((naked)) int port_swapcontext(mcu_context_t *oucp, const mcu_context_t *ucp)
{
    (void)oucp; /* to avoid compiler warnings */
    (void)ucp;
    AVR_SAVE_CONTEXT(
        "",
        "mov r30, r24\n"
        "mov r31, r25\n");
    AVR_RESTORE_CONTEXT(
        "mov r30, r22\n"
        "mov r31, r23\n");
    __asm__ __volatile__ ("ldi r24,0\n");
    __asm__ __volatile__ ("ldi r25,0\n");
    __asm__ __volatile__ ("ret\n");
}

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus **/
static void port_makecontext_callfunc(const mcu_context_t *successor, void (*func)(void *), void *funcarg);
#ifdef __cplusplus
}
#endif /*__cplusplus */
static void port_makecontext_callfunc(const mcu_context_t *successor, void (*func)(void *), void *funcarg)
{
    func(funcarg);
    port_setcontext(successor);
}

int port_makecontext(mcu_context_t *cp, void *stackp, const size_t stack_size, const mcu_context_t *successor_cp, void (*funcp)(void *), void *funcargp)
{
    uint16_t addr;
    uint8_t *p = (uint8_t *)&addr;
    /* initialise stack pointer and program counter */
    cp->sp.ptr = ((uint8_t *)stackp + stack_size - 1);
    cp->pc.ptr = port_makecontext_callfunc;
    /* initialise registers to pass arguments to port_makecontext_callfunc */
    /* successor: registers 24,25; func registers 23, 22; funcarg: 21, 20. */
    addr = (uint16_t)successor_cp;
    cp->r[24] = p[0];
    cp->r[25] = p[1];
    addr = (uint16_t)funcp;
    cp->r[22] = p[0];
    cp->r[23] = p[1];
    addr = (uint16_t)funcp;
    addr = (uint16_t)funcargp;
    cp->r[20] = p[0];
    cp->r[21] = p[1];
    return 0;
}

#if __cplusplus >= 201103L
/*
See bug: https://gcc.gnu.org/bugzilla/show_bug.cgi?id=49171
*/
inline void avr_context_sanity_checks(void)
{
    mcu_context_t test;
    static_assert(reinterpret_cast<uintptr_t>(&test) == reinterpret_cast<uintptr_t>(&test.sreg));
    static_assert(sizeof(mcu_context_t) == 37);
    static_assert(reinterpret_cast<uintptr_t>(&test.sp.part.low) - reinterpret_cast<uintptr_t>(&test) == AVR_CONTEXT_OFFSET_SP_L);
    static_assert(reinterpret_cast<uintptr_t>(&test.sp.part.high) - reinterpret_cast<uintptr_t>(&test) == AVR_CONTEXT_OFFSET_SP_H);
    static_assert(reinterpret_cast<uintptr_t>(&test.pc.part.low) - reinterpret_cast<uintptr_t>(&test) == AVR_CONTEXT_OFFSET_PC_L);
    static_assert(reinterpret_cast<uintptr_t>(&test.pc.part.high) - reinterpret_cast<uintptr_t>(&test) == AVR_CONTEXT_OFFSET_PC_H);
    static_assert(reinterpret_cast<uintptr_t>(&test.sp.part.high) - reinterpret_cast<uintptr_t>(&test.r[26]) == AVR_CONTEXT_BACK_OFFSET_R26);
}
#endif /* __cplusplus */

#endif /* __AVR__ */

#endif /* AVRCONTEXT_IMPL_H */

/* Above this comment, the code is released under the MIT license and mostly
 * written by the mentioned author. Below the code is written by Timothy Joseph
 * and released under the MIT license. See LICENSE.txt under the root directory
 * of the project for the license text
 */

static uint8_t nested_atomic;
static uint8_t were_interrupts_enabled;

int
port_enable_tick_interrupt(void)
{
	/* enable timer1 */
	PRR &= ~(1 << PRTIM1);
	/* enable prescaler to 1024 */
	TCCR1B = 0b101;
	/* enable overflow interrupt */
	TIMSK1 |= 1 << TOIE1;
	/* set the defualt value */
	TCNT1 = TCNT1_1MS;
	/* enable interrupts */
	sei();

	return 0;
}


int
PORT_ARE_INTERRUPTS_ENABLED(void)
{
	return !!(SREG & 0x80);
}

int
PORT_BEGIN_ATOMIC(void)
{
	int interrupts;
	if (nested_atomic == 0xff)
		return 1;
	
	interrupts = PORT_ARE_INTERRUPTS_ENABLED();
	if (nested_atomic == 0 && interrupts)
		PORT_DISABLE_INTERRUPTS();
	were_interrupts_enabled = interrupts;
	nested_atomic++;
	
	return 0;
}

int
PORT_END_ATOMIC(void)
{
	int interrupts;
	if (nested_atomic == 0x0)
		return 1;
	
	interrupts = were_interrupts_enabled;
	nested_atomic--;
	if (nested_atomic == 0 && interrupts) {
		were_interrupts_enabled = 0;
		PORT_ENABLE_INTERRUPTS();
	}
	
	return 0;
}

int
PORT_IS_ATOMIC(void)
{
	return !!nested_atomic;
}
