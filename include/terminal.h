#ifndef TERMINAL_H
# define TERMINAL_H

# include <stdint.h>
# include <stddef.h>
# include "libk.h"
# include "vga.h"

# define TERMINAL_COUNT 10		/* F1..F10 */

/*
** One shadow buffer per screen.  Writes to a screen that is not currently
** mirrored to 0xB8000 update only this struct; terminal_switch blits it back.
**
** Deliberately dumb: a terminal knows what is on it and where the cursor is,
** and nothing about what the text means.  The line being typed belongs to the
** shell's line editor (src/shell/shell_loop.c), not here.
*/
typedef struct {
    uint16_t	buffer[VGA_WIDTH * VGA_HEIGHT];
    size_t		x;				/* column */
    size_t		y;				/* row */
    uint8_t		color;
} terminal_t;

void    terminal_initialize(void);
void    terminal_putchar(char c);
void    terminal_write(const char *data, size_t size);
void    terminal_writestring(const char *str);
void    terminal_set_color(uint8_t color);
uint8_t terminal_get_color(void);
void    terminal_clear(void);
void    terminal_switch(size_t n);

/* Which screen is active -- the shell keys its per-terminal line state on it. */
size_t  terminal_current(void);

#endif
