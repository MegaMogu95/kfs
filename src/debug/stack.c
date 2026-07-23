#include "debug.h"
#include "printk.h"
#include <stdint.h>

/*
** Linker symbol: only the ADDRESS is meaningful, there is no object to read.
** Declaring it as an array makes that explicit and stops `stack_top` from
** silently compiling as a load of whatever .bss variable follows the stack.
*/
extern uint8_t	stack_top[];

/*
** The stack grows down, so the live region is [esp, stack_top).  Everything
** below esp is stale.  Two guards, both load-bearing:
**   - esp >= stack_top means the subtraction would underflow to near SIZE_MAX,
**     and with no paging yet there is nothing to fault on -- it would just
**     print megabytes of garbage until the machine is rebooted by hand.
**   - the clamp keeps a deep call chain from scrolling the useful frames off
**     the screen.  Tune STACK_DUMP_MAX freely; it is a display choice, not a
**     correctness one.
*/
#define STACK_DUMP_MAX	512

void	print_stack(void)
{
	uint8_t	*sp;
	size_t	len;

	__asm__ volatile ("mov %%esp, %0" : "=r" (sp));
	if (sp >= stack_top)
	{
		printk("stack: esp (%p) is above stack_top (%p)\n", sp, stack_top);
		return ;
	}
	len = (size_t)(stack_top - sp);
	if (len > STACK_DUMP_MAX)
		len = STACK_DUMP_MAX;
	hexdump(sp, len);
}
