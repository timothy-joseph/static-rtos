#ifndef PORT_H
#define PORT_H

#ifdef STATIC_RTOS_LINUX_TARGET
#include <port/ports/linux_port.h>
#endif /* #ifdef LINUX */

#ifdef STATIC_RTOS_AVR_TARGET
#include <port/ports/avr_port.h>
#endif /* #ifdef AVR */

#endif
