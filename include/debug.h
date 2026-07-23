#ifndef DEBUG_H
# define DEBUG_H

# include <stddef.h>

/*
** hexdump() is generic on purpose -- KFS-3 points it at page tables, KFS-4 at
** an interrupt frame.  print_stack() is the KFS-2 deliverable and is the only
** part that knows where the kernel stack lives.
*/

void	hexdump(const void *addr, size_t len);
void	print_stack(void);

#endif
