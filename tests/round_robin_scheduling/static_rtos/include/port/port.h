#ifndef PORT_H
#define PORT_H

#include <ucontext.h>

typedef ucontext_t mcu_context_t;

#define port_getcontext getcontext
#define port_setcontext setcontext
#define port_swapcontext swapcontext

static inline void port_makecontext(mcu_context_t *ucp, void *stack, size_t stack_size,
		      mcu_context_t *successor_ctx, void (*func)(void *),
		      void *args);

static inline void
port_makecontext(mcu_context_t *ucp, void *stack, size_t stack_size,
		      mcu_context_t *successor_ctx, void (*func)(),
		      void *args)
{
	ucp->uc_stack.ss_sp = stack;
	ucp->uc_stack.ss_size = stack_size;
	ucp->uc_link = successor_ctx;
	makecontext(ucp, func, 0);
}

#endif
