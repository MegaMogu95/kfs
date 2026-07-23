#include "shell.h"
#include "terminal.h"
#include "printk.h"
#include "debug.h"
#include "io.h"
#include "libk.h"

const t_command	cmd_arr[] =
{
	{"stack_dump",	"hexdump the live kernel stack",	shell_stack_dump},
	{"clear",		"blank the current screen",			terminal_clear},
	{"reboot",		"reset the machine via the 8042",	shell_reboot},
	{"halt",		"stop the CPU (cli; hlt)",			shell_halt},
	{"help",		"list these commands",				shell_help},
	{0, 0, 0}
};

void	shell_stack_dump(void)
{
	print_stack();
}

/*
** Pulse the keyboard controller's reset line.  Poll until the input buffer
** (status bit 1) is clear, or the 0xFE lands in a full buffer and is dropped.
** The reset is not instant, hence the cli;hlt backstop.
*/
void	shell_reboot(void)
{
	uint8_t status = 0x02;

	while (status & 0x02)
		status = inb(0x64);
	outb(0x64, 0xfe);
	__asm__ volatile ("cli; hlt");
}

void	shell_halt(void)
{
	__asm__ volatile ("cli; hlt");
}

void	shell_help(void)
{
	size_t	pad;

	printk("Available commands:\n");
	for (size_t i = 0; cmd_arr[i].name; i++)
	{
		printk("  %s", cmd_arr[i].name);
		pad = strlen(cmd_arr[i].name);
		while (pad++ < 12)
			printk(" ");
		printk("%s\n", cmd_arr[i].desc);
	}
}
