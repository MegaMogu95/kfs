#include "terminal.h"

static terminal_t  screens[TERMINAL_COUNT];
static terminal_t *active;

static void terminal_scroll(void)
{
	memmove(active->buffer, active->buffer + VGA_WIDTH, 2 * VGA_WIDTH * (VGA_HEIGHT - 1));
	active->y -= 1;
	for (size_t i = 0; i < VGA_WIDTH; i++)
		active->buffer[i + VGA_WIDTH * active->y] = vga_entry(' ', active->color);
	vga_blit(active->buffer);
}

//when x == VGA_WIDTH. No blit, only update_cursor
static void terminal_newline(void)
{
	active->x = 0;
	active->y += 1;

	if (active->y == VGA_HEIGHT)
		terminal_scroll();
	vga_update_cursor(active->x, active->y);
}

void    terminal_switch(size_t n)
{
	if (n >= TERMINAL_COUNT)
		return;
	active = screens + n;
	vga_blit(active->buffer);
	vga_enable_cursor(0, 15);
	vga_update_cursor(active->x, active->y);
}

void	terminal_initialize(void)
{
	for (size_t i = 0; i < TERMINAL_COUNT; i++)
	{
		screens[i].x = 0;
		screens[i].y = 0;
		screens[i].color = C_DEFAULT;
		for (size_t j = 0; j < VGA_WIDTH * VGA_HEIGHT; j++)
			screens[i].buffer[j] = vga_entry(' ', screens[i].color);
	}
	terminal_switch(0);
}

size_t	terminal_current(void)
{
	return ((size_t)(active - screens));
}

void	terminal_putchar(char c)
{
	if (c == '\b')
	{
		if (active->x > 0)
			active->x -= 1;
		else if (active->y > 0)
		{
			active->x = VGA_WIDTH - 1;
			active->y -= 1;
		}
		else
			return ;
		active->buffer[active->x + active->y * VGA_WIDTH] = vga_entry(' ', active->color);
		vga_put_entry(active->x, active->y, ' ', active->color);
		vga_update_cursor(active->x, active->y);
	}
	else if (c == '\n')
		terminal_newline();
	else if (c == '\t')
	{
		size_t next = (active->x + TAB_WIDTH) & ~(TAB_WIDTH - 1);
		if (next >= VGA_WIDTH)
			next = VGA_WIDTH - 1;
		while (active->x < next)
			terminal_putchar(' ');
	}
	else
	{
		active->buffer[active->x + active->y * VGA_WIDTH] = vga_entry(c, active->color);
		vga_put_entry(active->x, active->y, c, active->color);
		if (active->x < VGA_WIDTH - 1)
		{
			active->x++;
			vga_update_cursor(active->x, active->y);
		}
		else
			terminal_newline();
	}
}

void    terminal_write(const char *data, size_t size)
{
	for (size_t i = 0; i < size; i++)
		terminal_putchar(data[i]);
}

void    terminal_writestring(const char *str)
{
	for (size_t i = 0; str[i]; i++)
		terminal_putchar(str[i]);
}

void    terminal_set_color(uint8_t color)
{
	active->color = color;
}

uint8_t terminal_get_color(void)
{
	return (active->color);
}

void    terminal_clear(void)
{
	for (size_t i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++)
		active->buffer[i] = vga_entry(' ', C_DEFAULT);
	vga_blit(active->buffer);
	active->x = 0;
	active->y = 0;
	vga_update_cursor(0, 0);
}