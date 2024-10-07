#include <stddef.h>

#include <static_rtos/port/port.h>

#include <libopencm3/cm3/cortex.h>
#include <libopencm3/cm3/systick.h>

static void port_makecontext_callfunc(void (*func)(void *), void *args,
				      mcu_context_t *successor_cp);

__attribute__((naked))
int
port_getcontext(mcu_context_t *cp)
{
	/* to avoid a compiler warning: */
	(void)cp;

	/* saving r0 isn't required, i guess; r15 should not be saved */
	__asm__ __volatile__ (
		"	stm r0, {r0-r12}\n"
		"	str r13, [r0, #0x34]\n"
		"	str r14, [r0, #0x38]\n"
		"	add r0, #0x3c\n"
		"	mrs r1, XPSR\n"
		"	mrs r2, PRIMASK\n"
		"	mrs r3, FAULTMASK\n"
		"	stm r0, {r1-r3}\n"
		"	mrs r1, BASEPRI\n"
		"	mrs r2, CONTROL\n"
		"	stm r0, {r1-r2}\n"
		/* return 0 */
		"	mov r0, #0\n"
		"	mov pc, lr\n"
	);
}

__attribute__((naked))
int
port_setcontext(const mcu_context_t *cp)
{
	/* to avoid a compiler warning: */
	(void)cp;

	/* restoring r0, r1 and r2 correctly is required because they are
	 * the parameters to port_makecontext_callfunc
	 */
	__asm__ __volatile__(
		"	mov r1, r0\n"
		"	add r1, #8\n"
		"	ldm r1, {r2-r12}\n"
		"	ldr r13, [r0, #0x34]\n"
		"	ldr r14, [r0, #0x38]\n"
		/* restore primask, faultmask, basepri, control, XPSR */
		"	ldr r1, [r0, #0x4c]\n"
		"	msr CONTROL, r1\n"
		"	ldr r1, [r0, #0x48]\n"
		"	msr BASEPRI, r1\n"
		"	ldr r1, [r0, #0x44]\n"
		"	msr FAULTMASK, r1\n"
		"	ldr r1, [r0, #0x40]\n"
		"	msr PRIMASK, r1\n"
		"	ldr r1, [r0, #0x3c]\n"
		"	msr XPSR, r1\n"
		/* restore r1 */
		"	ldr r1, [r0, #0x4]\n"
		/* r0 needs to be restored last because until here it contains
		 * the address of the mcu_context_t structure
		 */
		"	ldr r0, [r0, #0x0]\n"
		/* return 0 */
		"	mov pc, lr\n"
		/* the program should never reach this point */
		"	mov r0, #-1\n"
		"	mov pc, lr\n"
	);
}

int
port_swapcontext(mcu_context_t *oucp, const mcu_context_t *ucp)
{
	/* to avoid a compiler warning: */
	int ret;

	ret = port_getcontext(oucp);
	if (ret)
		return ret;

	return port_setcontext(ucp);
}

int
port_makecontext(mcu_context_t *cp, void *stackp, const size_t stack_size,
		 const mcu_context_t *successor_cp, void (*funcp)(void *),
		 void *funcargp)
{
	/* this function creates the context at port_makecontext_callfunc
	 * the other registers must be initialized before calling this function
	 */
	cp->r[0].r_func_thread = funcp;
	cp->r[1].r_voidp = funcargp;
	cp->r[2].r_voidp = (void *)successor_cp;
	cp->r[13].r_voidp = (void *)(&((char *)stackp)[stack_size - 4]);
	cp->r[14].r_func_makecall = port_makecontext_callfunc;
	cp->xpsr = 0x21000000;
	cp->primask = 0x0;
	cp->faultmask = 0x0;
	cp->basepri = 0x0;
	cp->control = 0x0;

	return 0;
}

int
port_enable_tick_interrupt(void)
{
	/* NOTE: it's assumed the clock is set at 72Mhz */
	/* 72MHz / 8 => 9000000 counts per second */
	systick_set_clocksource(STK_CSR_CLKSOURCE_AHB_DIV8);

	/* 9000000/9000 = 1000 overflows per second - every 1ms one interrupt */
	/* SysTick interrupt every N clock pulses: set reload to N-1 */
	systick_set_reload(8999);

	/* on startup, the systick counter value is unknown */
	systick_clear();

	systick_interrupt_enable();

	/* Start counting. */
	systick_counter_enable();

	return 0;
}

/**
 * NOTE: for the cm3 port, this function enables both interrupts and faults
 */
int
PORT_ENABLE_INTERRUPTS(void)
{
	cm_enable_interrupts();
	cm_enable_faults();

	return 0;
}

/**
 * NOTE: for the cm3 port, this function disables both interrupts and faults
 */
int
PORT_DISABLE_INTERRUPTS(void)
{
	cm_disable_interrupts();
	cm_disable_faults();

	return 0;
}

/**
 * NOTE: for the cm3 port, this function checks if interrupts or faults are
 * enabled
 */
int
PORT_ARE_INTERRUPTS_ENABLED(void)
{
	return !cm_is_masked_interrupts() || !cm_is_masked_faults();
}

/**
 * Internal helper function for port_makecontext. The LR register gets set to
 * this so when port_setcontext returns, it will return to this function
 */
static void
port_makecontext_callfunc(void (*func)(void *), void *args,
			  mcu_context_t *successor_cp)
{
	if (func)
		func(args);
	if (successor_cp)
		(void)port_setcontext(successor_cp);
}

/* TODO: make this into a .c file */
#include "avr_libopencm3_common.h"

