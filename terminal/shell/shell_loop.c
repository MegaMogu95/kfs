#include "shell.h"
#include "ps2.h"
#include "vga.h"
#include "printk.h"
#include "string.h"
#include "kbd_buffer.h"
#include "terminal.h"
#include <stddef.h>

void	prompt()
{
	printk("%Ckfs", vga_entry_color(VGA_COLOR_CYAN, VGA_COLOR_BLACK));
	printk("%C$ ", vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
}

char	get_char()
{
	char	c = 0;

	while (c == 0)
	{
		ps2_poll();
		kbd_buf_pop(&c);
	}
	return (c);
}

size_t	get_line(char *line, size_t len)
{
	char	c;
	size_t	i = 0;

	while (i < len)
	{
		c = get_char();
		if (c == '\b')
		{
			if (i > 0)
			{
				terminal_putchar(c);
				i--;
			}
		}
		else
		{
			terminal_putchar(c);
			if (c == '\n')
				break;
			line[i] = c;
			i++;
		}
	}
	if (i < len)
		line[i] = '\0';
	return (i);
}

void	shell_loop()
{
	char	line[MAX_LINE_LEN];
	char	known_command;

	while (1)
	{
		prompt();
		get_line(line, MAX_LINE_LEN);
		known_command = 0;
		for (size_t i = 0; cmd_arr[i].name != 0; i++)
		{
			if (strcmp(line, cmd_arr[i].name) == 0)
			{
				known_command = 1;
				cmd_arr[i].handler();
				break;
			}
		}
		if (!known_command)
		{
			printk_err("unknown command\n");
			shell_help();
		}
	}
}