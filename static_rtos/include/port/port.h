/*
 * Copyright 2024 Timothy Joseph. Subject to MIT license
 * See LICENSE.txt for details
 */
#ifndef PORT_H
#define PORT_H

#ifdef STATIC_RTOS_LINUX_TARGET
#include <port/ports/linux_port.h>
#endif /* #ifdef LINUX */

#ifdef STATIC_RTOS_AVR_TARGET
#include <port/ports/avr_port.h>
#endif /* #ifdef AVR */

int port_getcontext(mcu_context_t *cp);
int port_setcontext(const mcu_context_t *cp);
int port_swapcontext(mcu_context_t *oucp, const mcu_context_t *ucp);
int port_makecontext(mcu_context_t *cp, void *stackp, const size_t stack_size, const mcu_context_t *successor_cp, void (*funcp)(void *), void *funcargp);

int port_enable_tick_interrupt(void);
int PORT_ARE_INTERRUPTS_ENABLED(void);
int PORT_BEGIN_ATOMIC(void);
int PORT_END_ATOMIC(void);
int PORT_IS_ATOMIC(void);

#endif

