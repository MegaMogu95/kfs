#include "libk.h"

void	*memcpy(void *dst, const void *src, size_t n)
{
	unsigned char		*dst_chr = dst;
	const unsigned char	*src_chr = src;

	for (size_t i = 0; i < n; i++)
		dst_chr[i] = src_chr[i];
	return (dst);
}
