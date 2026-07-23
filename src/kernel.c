#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "terminal.h"
#include "printk.h"
#include "shell.h"

static const char	*ascii_art[25] = 
{
	"# **************************************************************************** #", 
	"#                                                                              #", 
	"#                                   :::      ::::::::                          #", 
	"#                                 :+:      :+:    :+:                          #", 
	"#                               +:+ +:+         +:+                            #", 
	"#                             +#+  +:+       +#+                               #", 
	"#                           +#+#+#+#+#+   +#+                                  #", 
	"#                                #+#    #+#                                    #", 
	"#                               ###   ########                                 #", 
	"#                                                                              #", 
	"# **************************************************************************** #"
};

void	kernel_main(void)
{
	terminal_initialize();
	for (size_t i = 0; i < 11; i++)
	{
		terminal_write(ascii_art[i], 80);
	}
	shell_loop();
}