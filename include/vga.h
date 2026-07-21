#ifndef VGA_H
# define VGA_H

# include <stdbool.h>
# include <stddef.h>
# include <stdint.h>

# define VGA_WIDTH   80
# define VGA_HEIGHT  25
# define VGA_MEMORY  0xB8000

# define VGA_CRTC_INDEX 0x3D4
# define VGA_CRTC_DATA  0x3D5

# define TAB_WIDTH 8 //Must be either 1, 2, 4, 8 or 16

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

uint8_t  vga_entry_color(enum vga_color fg, enum vga_color bg);
uint16_t vga_entry(char c, uint8_t color);
void     vga_put_entry(size_t x, size_t y, char c, uint8_t color);
void     vga_blit(const uint16_t *buffer);
void     vga_enable_cursor(uint8_t start, uint8_t end);
void     vga_disable_cursor(void);
void     vga_update_cursor(size_t x, size_t y);

#endif