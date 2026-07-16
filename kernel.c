#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define VGA_WIDTH   80
#define VGA_HEIGHT  25
#define VGA_MEMORY  0xB8000

/* Hardware text mode color constants. */
enum vga_color {
	VGA_COLOR_BLACK = 0,
	VGA_COLOR_BLUE = 1,
	VGA_COLOR_GREEN = 2,
	VGA_COLOR_CYAN = 3,
	VGA_COLOR_RED = 4,
	VGA_COLOR_MAGENTA = 5,
	VGA_COLOR_BROWN = 6,
	VGA_COLOR_LIGHT_GREY = 7,
	VGA_COLOR_DARK_GREY = 8,
	VGA_COLOR_LIGHT_BLUE = 9,
	VGA_COLOR_LIGHT_GREEN = 10,
	VGA_COLOR_LIGHT_CYAN = 11,
	VGA_COLOR_LIGHT_RED = 12,
	VGA_COLOR_LIGHT_MAGENTA = 13,
	VGA_COLOR_LIGHT_BROWN = 14,
	VGA_COLOR_WHITE = 15,
};

static const char	*ascii_art[11] = 
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

static inline uint8_t vga_entry_color(enum vga_color fg, enum vga_color bg) 
{
	return fg | bg << 4;
}

static inline uint16_t vga_entry(unsigned char uc, uint8_t color) 
{
	return (uint16_t) uc | (uint16_t) color << 8;
}

static uint8_t g_terminal_color;
static uint16_t* g_terminal_buffer = (uint16_t*)VGA_MEMORY;

static void terminal_initialize(void) 
{
	g_terminal_color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_DARK_GREY);
	
	for (size_t y = 0; y < VGA_HEIGHT; y++) {
		for (size_t x = 0; x < VGA_WIDTH; x++) {
			const size_t index = y * VGA_WIDTH + x;
			g_terminal_buffer[index] = vga_entry(' ', g_terminal_color);
		}
	}
}

static void terminal_putentryat(char c, uint8_t color, size_t x, size_t y) 
{
	const size_t index = y * VGA_WIDTH + x;
	g_terminal_buffer[index] = vga_entry(c, color);
}

void kernel_main(void) 
{
	/* Initialize terminal interface */
	terminal_initialize();

	g_terminal_color = vga_entry_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_DARK_GREY);
	for (size_t y = 0; y < 11; y++)
	{
		for (size_t x = 0; x < 80; x++)
		{
			terminal_putentryat(ascii_art[y][x], g_terminal_color, x, y);
		}
	}
}