#ifndef TERMINAL_H
# define TERMINAL_H

# include <stdint.h>
# include <stddef.h>
# include "string.h"
# include "vga.h"

# define TERMINAL_COUNT 10
# define MAX_LINE_LEN 256
# define C_DEFAULT vga_entry_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK) 

typedef struct {
    uint16_t	buffer[VGA_WIDTH * VGA_HEIGHT];
	char		line[MAX_LINE_LEN];	/* command being typed on THIS terminal */
	size_t		line_len;
    size_t		x;
    size_t		y;
    uint8_t		color;
	uint8_t		prompted;			/* a prompt is already on screen */
} terminal_t;

void    terminal_initialize(void);
void    terminal_putchar(char c);
void    terminal_write(const char *data, size_t size);
void    terminal_writestring(const char *str);
void    terminal_set_color(uint8_t color);
uint8_t terminal_get_color(void);
void    terminal_clear(void);
void    terminal_switch(size_t n);

/* line editing — always acts on the *active* terminal */
int         terminal_line_push(char c);  /* append; 0 if the line is full */
int         terminal_line_pop(void);     /* backspace; 0 if the line is empty */
void        terminal_line_clear(void);
const char *terminal_line(void);         /* NUL-terminated */
size_t      terminal_line_len(void);

int     terminal_needs_prompt(void);
void    terminal_mark_prompted(void);

#endif