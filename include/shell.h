#ifndef SHELL_H
# define SHELL_H

# define MAX_LINE_LEN 256

typedef struct s_command
{
	const char	*name;
	const char	*desc;
	void		(*handler)(void);
}	t_command;

/*
** Defined in src/shell/commands.c, NULL-name terminated.  `help` walks the
** same array the dispatcher walks, so the two can never drift apart.  It is
** declared extern rather than defined here on purpose: a `static const` table
** in a header gives every translation unit its own private copy.
*/
extern const t_command	cmd_arr[];

void	shell_loop(void);

void	shell_stack_dump(void);
void	shell_reboot(void);
void	shell_halt(void);
void	shell_help(void);
void	shell_gdt_dump(void);
void	shell_regs(void);

#endif
