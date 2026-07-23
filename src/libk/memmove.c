#include "libk.h"

void	*memmove(void *dst, const void *src, size_t n)
{
	unsigned char		*dst_chr = dst;
	const unsigned char	*src_chr = src;

	if (dst < src)
		memcpy(dst, src, n);
	else
	{
		for (size_t i = n; i > 0; i--)
			dst_chr[i - 1] = src_chr[i - 1];
	}
	return (dst);
}
