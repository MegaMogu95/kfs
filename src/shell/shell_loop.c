#include "shell.h"
#include "terminal.h"
#include "keyboard.h"
#include "kbd_buffer.h"
#include "ps2.h"
#include "printk.h"
#include "vga.h"
#include "libk.h"
#include <stddef.h>
#include <stdint.h>

/*
** The line discipline.  One line state per screen, so an F1-F10 switch
** mid-command parks the half-typed line on the terminal we left and resumes
** whatever was pending on the one we land on.
**
** This lives in the shell, not in terminal_t: the terminal layer stays dumb
** about what the characters on it mean.  It is keyed on terminal_current()
** rather than stored by the terminal so that in KFS-5, when a process owns a
** tty, this becomes per-process state without touching the driver.
**
** `prompted` records that a terminal has had its prompt printed at least once
** -- a never-visited screen is blank and needs one on arrival.
*/
typedef struct {
	char	buf[MAX_LINE_LEN];
	size_t	len;
	uint8_t	prompted;
}	line_state_t;

static line_state_t	lines[TERMINAL_COUNT];

static line_state_t	*line_here(void)
{
	return (&lines[terminal_current()]);
}

static int	line_push(char c)
{
	line_state_t	*l = line_here();

	if (l->len + 1 >= MAX_LINE_LEN)		/* keep room for the NUL */
		return (0);
	l->buf[l->len++] = c;
	l->buf[l->len] = '\0';
	return (1);
}

static int	line_pop(void)
{
	line_state_t	*l = line_here();

	if (l->len == 0)					/* won't eat into the prompt */
		return (0);
	l->buf[--l->len] = '\0';
	return (1);
}

static void	line_clear(void)
{
	line_state_t	*l = line_here();

	l->len = 0;
	l->buf[0] = '\0';
}

static void	prompt(void)
{
	printk("%Ckfs", vga_entry_color(VGA_COLOR_CYAN, VGA_COLOR_BLACK));
	printk("%C$ ", vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
	line_here()->prompted = 1;
}

static char	get_char(void)
{
	char	c = 0;

	while (c == 0)
	{
		ps2_poll();
		kbd_buf_pop(&c);
	}
	return (c);
}

/* Reads until Enter and returns the active terminal's line. */
static const char	*get_line(void)
{
	char	c;

	while (1)
	{
		c = get_char();
		if (CHR_F1 <= c && c < CHR_F1 + TERMINAL_COUNT)
		{
			terminal_switch(c - CHR_F1);
			if (!line_here()->prompted)		/* never-visited terminal */
				prompt();
		}
		else if (c == '\n')
		{
			terminal_putchar(c);
			return (line_here()->buf);
		}
		else if (c == '\b')
		{
			if (line_pop())
				terminal_putchar(c);
		}
		else if (line_push(c))
			terminal_putchar(c);
	}
}

void	shell_loop(void)
{
	const char	*line;
	char		known_command;

	prompt();
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
		line_clear();					/* consumed: reset this terminal */
		prompt();
	}
}
