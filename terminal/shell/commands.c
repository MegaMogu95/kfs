#include "shell.h"
#include "printk.h"
#include "hexdump.h"
#include "io.h"

extern uint32_t	stack_top;

void	shell_stack_dump()
{
	void	*sp;

	__asm__ volatile (
		"mov %%esp, %0"
		: "=r" (sp)
	);
	hexdump(sp, stack_top - (uint32_t)sp);
}

void	shell_reboot()
{
	uint8_t status = 0x02;

	while (status & 0x02)
		status = inb(0x64);
	outb(0x64, 0xfe);
	__asm__ volatile ("cli;hlt");
}

void	shell_halt()
{
	__asm__ volatile(
		"cli;hlt"
	);
}

void	shell_help()
{
	printk("Available commands:\n");
	for (size_t i = 0; cmd_arr[i].name; i++)
		printk("\t%s\n", cmd_arr[i].name);
}