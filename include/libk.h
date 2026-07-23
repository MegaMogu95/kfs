#ifndef LIBK_H
# define LIBK_H

# include <stddef.h>

/*
** Reimplementations of the libc functions the kernel needs.  They keep their
** standard names, unprefixed, because they *are* those functions: GCC emits
** calls to memcpy/memset on its own (struct assignment, array init) even under
** -ffreestanding, so they must exist under exactly these names.
*/

size_t	strlen(const char *s);
int		strcmp(const char *a, const char *b);
void	*memset(void *dst, int c, size_t n);
void	*memcpy(void *dst, const void *src, size_t n);
void	*memmove(void *dst, const void *src, size_t n);

#endif
