#include <static_rtos/kernel/scheduler.h>

#include <libopencm3/cm3/cortex.h>
#include <libopencm3/cm3/systick.h>
#include <libopencm3/cm3/nvic.h>

void
sys_tick_handler(void)
{
	if (!kscheduler_has_started())
		return;
	
	if (kincrease_tickcount())
		kyield();
}

