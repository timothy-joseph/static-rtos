#ifndef PORT_TIMER_H
#define PORT_TIMER_H

#ifdef STATIC_RTOS_LINUX_TARGET
#include <port/ports/linux_port_timer.h>
#endif /* #ifdef LINUX */

#ifdef STATIC_RTOS_AVR_TARGET
#include <port/ports/avr_port_timer.h>
#endif /* #ifdef AVR */

#endif

