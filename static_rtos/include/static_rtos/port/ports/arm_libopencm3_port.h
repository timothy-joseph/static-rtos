#ifndef ARM_CM3_PORT_H
#define ARM_CM3_PORT_H

#include <stdint.h>

typedef struct cm3_context_t mcu_context_t;

/* type used to avoid a compiler warning */
union cm3_reg_t {
	/* so far, i don't need r_int */
	uint32_t r_int;
	void *r_voidp;
	void (*r_func_makecall)(void (*)(void *), void *, mcu_context_t *);
	void (*r_func_thread)(void *);
};

struct cm3_context_t {
	/* r15 isn't saved, so i don't allocate memory for it */
	union cm3_reg_t r[15];
	uint32_t xpsr;
	uint32_t primask;
	uint32_t faultmask;
	uint32_t basepri;
	uint32_t control;
};

#endif /* ARM_CM3_PORT_H */

