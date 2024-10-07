/*
 * Copyright 2024 Timothy Joseph. Subject to MIT license
 * See LICENSE.txt for details
 */
#ifndef PORT_H
#define PORT_H

#ifdef STATIC_RTOS_LINUX_TARGET
#include <static_rtos/port/ports/linux_port.h>
#endif /* #ifdef LINUX */

#ifdef STATIC_RTOS_AVR_TARGET
#include <static_rtos/port/ports/avr_port.h>
#endif /* #ifdef AVR */

#ifdef STATIC_RTOS_LIBOPENCM3_TARGET
#include <static_rtos/port/ports/arm_libopencm3_port.h>
#endif /* #ifdef LIBOPENCM3 */

/**
 * The following functions need to be defined by the porter
 */

/**
 * This function is used to get the current context of the cpu and put it into
 * ucp.
 *
 * @param cp The context of the cpu is put into this structure
 *
 * @return 0 on success and -1 on failure
 */
int port_getcontext(mcu_context_t *cp);

/**
 * This function is used to set the current context of the cpu to cp.
 *
 * @param cp
 *
 * @return This function doesn't return on success, and it returns 1 on failure
 */
int port_setcontext(const mcu_context_t *cp);


/**
 * This function saves the current context into oucp and loads the context from
 * cp
 * Returns -1 on error and doesn't return on success
 */
int port_swapcontext(mcu_context_t *oucp, const mcu_context_t *ucp);

/**
 * This function is used to set the context of cp to point to func with
 * argument args. The stack of this context will be stack with size stack_size.
 * successor_ctx is the return context. On linux, the args argument doesn't
 * work
 * 
 * Before calling this function, call port_getcontext to get all other registers
 * initialized
 *
 * @param cp
 * @param stackp The allocated memory for the stack. Must not go out of scope
 * 		 during the operation of the created context
 * @param stack_size
 * @param successor_cp When this context returns, it will return here. Must not
 * 		       go out of scope
 * @param funcp
 * @param funcagrp The argument passed to the function pointer to by funcp. Must
 *		   not go out of scope
 *
 * @return same as port_getcontext
 */
int port_makecontext(mcu_context_t *cp, void *stackp, const size_t stack_size,
		     const mcu_context_t *successor_cp, void (*funcp)(void *),
		     void *funcargp);

/**
 * This function enables global interrupts and then enables the timer interrupt
 * that will handle the tick interrupt
 * 
 * @return 0 on success and 1 on error (such as such timers not being available
 *	   on the current platform)
 */
int port_enable_tick_interrupt(void);

/**
 * This function or macro enables global interrupts
 *
 * @return 0 on success and 1 on failure
 */
int PORT_ENABLE_INTERRUPTS(void);

/**
 * This function or macro disables global interrupts
 *
 * @return 0 on success and 1 on failure
 */
int PORT_DISABLE_INTERRUPTS(void);

/**
 * @return This function or macro returns 1 if interrupts are enabled and 0
 *	   otherwise
 */
int PORT_ARE_INTERRUPTS_ENABLED(void);

/* TODO: these might not need to be port defined */
/**
 * This function begins a atomic block of code. It must support at least 255
 * nested atomic blocks.
 *
 * @return 0 on success and 1 on failure
 */
int PORT_BEGIN_ATOMIC(void);

/**
 * This function ends a atomic block of code. It must support at least 255
 * nested atomic blocks
 *
 * @return 0 on success and 1 on failure
 */
int PORT_END_ATOMIC(void);

/**
 * @return This function returns 1 if a atomic block is active
 */
int PORT_IS_ATOMIC(void);

#endif

