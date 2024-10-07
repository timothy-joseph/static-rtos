#include <stdint.h>

static uint8_t nested_atomic;
static uint8_t were_interrupts_enabled;

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

