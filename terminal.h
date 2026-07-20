#ifndef TERMINAL_H
# define TERMINAL_H

# include <stdint.h>
# include <stddef.h>
# include "string.h"
# include "vga.h"

# define TERMINAL_COUNT 8
# define C_DEFAULT vga_entry_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK) 

typedef struct {
    uint16_t buffer[VGA_WIDTH * VGA_HEIGHT];
    size_t   x;
    size_t   y;
    uint8_t  color;
} terminal_t;

void    terminal_initialize(void);
void    terminal_putchar(char c);
void    terminal_write(const char *data, size_t size);
void    terminal_writestring(const char *str);
void    terminal_set_color(uint8_t color);
uint8_t terminal_get_color(void);
void    terminal_clear(void);
void    terminal_switch(size_t n);

#endif