#include "debug.h"
#include "printk.h"

void	hexdump(const void *addr, size_t len)
{
	const unsigned char	*cast_addr = addr;
	char				c;

	while (len > 0)
	{
		printk("%p\t", cast_addr);
		for (size_t i = 0; i < 16; i++)
		{
			if (i < len)
				printk("%2x", cast_addr[i]);
			else
				printk("  ");
			if (i % 4 == 3)
				printk("  ");
		}
		printk("|");
		for (size_t i = 0; i < 16 && i < len; i++)
		{
			c = (cast_addr[i] >= 0x20 && cast_addr[i] <= 0x7e ? cast_addr[i] : '.');
			printk("%c", c);
		}
		printk("|\n");
		if (len > 16)
			len -= 16;
		else
			len = 0;
		cast_addr += 16;
	}
}