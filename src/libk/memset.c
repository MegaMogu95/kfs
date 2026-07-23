#include "libk.h"

void	*memset(void *dst, int c, size_t n)
{
	unsigned char *dst_chr = dst;

	for (size_t i = 0; i < n; i++)
		dst_chr[i] = c;
	return (dst);
}
