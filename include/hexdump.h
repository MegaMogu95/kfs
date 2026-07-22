#ifndef HEXDUMP_H
# define HEXDUMP_H

# include <stddef.h>

void	hexdump(const void *addr, size_t len);

void	stack_dump();

#endif
