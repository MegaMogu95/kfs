#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "terminal.h"
#include "ps2.h"
#include "keyboard.h"
#include "kbd_buffer.h"
#include "printk.h"

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

void	kernel_main()
{
	terminal_initialize();
	for (size_t i = 0; i < 11; i++)
	{
		terminal_write(ascii_art[i], 80);
	}
	printk_info("terminal ready: %dx%d\n", VGA_WIDTH, VGA_HEIGHT);
	printk_err("fault at %p, code %#x\n", (void *)0xdeadbeef, 14);
	printk("%C42%C\n", vga_entry_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK), C_DEFAULT);
    for (;;) {
        ps2_poll();                        /* producer (poll) */
        char c;
        while (kbd_buf_pop(&c))            /* consumer */
            terminal_putchar(c);
    }
}