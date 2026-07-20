#include "vga.h"
#include "io.h"

uint8_t	vga_entry_color(enum vga_color fg, enum vga_color bg)
{
	return ((uint8_t)fg | (uint8_t)bg << 4);
}

uint16_t	vga_entry(char c, uint8_t color)
{
	return ((uint16_t)c | (uint16_t)color << 8);
}

static volatile uint16_t *const vga_buffer = (volatile uint16_t *)0xB8000;

void	vga_put_entry(size_t x, size_t y, char c, uint8_t color)
{
	vga_buffer[x + VGA_WIDTH * y] = vga_entry(c, color);
}

void	vga_blit(const uint16_t *buffer)
{
	uint16_t	entry;

	for (size_t y = 0; y < VGA_HEIGHT; y++)
	{
		for (size_t x = 0; x < VGA_WIDTH; x++)
		{
			entry = buffer[x + y * VGA_WIDTH];
			vga_put_entry(x, y, entry & 0xff, (entry >> 8) & 0xff);
		}
	}
}

void	vga_enable_cursor(uint8_t start, uint8_t end)
{
    outb(VGA_CRTC_INDEX, 0x0A);
    outb(VGA_CRTC_DATA, (inb(VGA_CRTC_DATA) & 0xC0) | start); /* preserve top bits */
    outb(VGA_CRTC_INDEX, 0x0B);
    outb(VGA_CRTC_DATA, (inb(VGA_CRTC_DATA) & 0xE0) | end);
}

void	vga_disable_cursor(void)
{
    outb(VGA_CRTC_INDEX, 0x0A);
    outb(VGA_CRTC_DATA, 0x20);   /* bit 5 = disable */
}

void	vga_update_cursor(size_t col, size_t row)
{
    uint16_t pos = (uint16_t)(row * VGA_WIDTH + col);
    outb(VGA_CRTC_INDEX, 0x0F);
    outb(VGA_CRTC_DATA,  (uint8_t)(pos & 0xFF));
    outb(VGA_CRTC_INDEX, 0x0E);
    outb(VGA_CRTC_DATA,  (uint8_t)((pos >> 8) & 0xFF));
}
