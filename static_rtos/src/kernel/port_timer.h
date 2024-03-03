/*
 * Copyright 2024 Timothy Joseph. Subject to MIT license
 * See LICENSE.txt for details
 */
#ifndef PORT_TIMER_H
#define PORT_TIMER_H

#ifdef STATIC_RTOS_LINUX_TARGET
#include "timer_ports/linux_port_timer.h"
#endif /* #ifdef LINUX */

#ifdef STATIC_RTOS_AVR_TARGET
#include "timer_ports/avr_port_timer.h"
#endif /* #ifdef AVR */

#endif

