#include "shell.h"
#include "ps2.h"
#include "vga.h"
#include "printk.h"
#include "string.h"
#include "kbd_buffer.h"
#include "terminal.h"
#include "keyboard.h"
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

/*
** Reads until Enter and returns the *active* terminal's line.  All state lives
** in the terminal, so an F1-F10 switch mid-line parks the half-typed command on
** the terminal we left and resumes whatever was pending on the one we land on.
*/
const char	*get_line(void)
{
	char	c;

	while (1)
	{
		c = get_char();
		if (CHR_F1 <= c && c < CHR_F1 + TERMINAL_COUNT)
		{
			terminal_switch(c - CHR_F1);
			if (terminal_needs_prompt())	/* never-visited terminal */
			{
				prompt();
				terminal_mark_prompted();
			}
		}
		else if (c == '\n')
		{
			terminal_putchar(c);
			return (terminal_line());
		}
		else if (c == '\b')
		{
			if (terminal_line_pop())		/* won't eat into the prompt */
				terminal_putchar(c);
		}
		else if (terminal_line_push(c))
			terminal_putchar(c);
	}
}

void	shell_loop()
{
	const char	*line;
	char		known_command;

	prompt();
	terminal_mark_prompted();
	while (1)
	{
		line = get_line();
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
		if (!known_command && line[0] != '\0')
		{
			printk_err("unknown command\n");
			shell_help();
		}
		terminal_line_clear();				/* consumed: reset this terminal */
		prompt();
		terminal_mark_prompted();
	}
}