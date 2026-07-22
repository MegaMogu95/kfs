#ifndef SHELL_H
# define SHELL_H

# include "terminal.h"

# define MAX_LINE_LEN 256

typedef struct s_command
{
	const char			*name;
	void				(*handler)(void);
}	t_command;

void	shell_stack_dump();
void	shell_reboot();
void	shell_halt();
void	shell_help();

static const t_command	cmd_arr[] =
{
	{"stack_dump", shell_stack_dump},
	{"reboot", shell_reboot},
	{"halt", shell_halt},
	{"help", shell_help},
	{"clear", terminal_clear},
	{0, 0}
};

void	shell_loop();

#endif