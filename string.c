#include "string.h"

size_t	strlen(const char *s)
{
	size_t	len = 0;

	while (s[len])
		len++;
	return (len);
}

void	*memset(void *dst, int c, size_t n)
{
	unsigned char *dst_chr = dst;

	for (size_t i = 0; i < n; i++)
		dst_chr[i] = c;
}

void	*memcpy(void *dst, const void *src, size_t n)
{
	unsigned char	*dst_chr = dst;
	unsigned char	*src_chr = src;

	for (size_t i = 0; i < n; i++)
		dst_chr[i] = src_chr[i];
}

void	*memmove(void *dst, const void *src, size_t n)
{
	unsigned char	*dst_chr = dst;
	unsigned char	*src_chr = src;

	if (dst < src)
		memcpy(dst, src, n);
	else
	{
		for (size_t i = n; i > 0; i--)
			dst_chr[i - 1] = src_chr[i - 1];
	}
}

int	strcmp(const char *a, const char *b)
{
	while (*a == *b && *a)
	{
		a++;
		b++;
	}
	return ((unsigned char)*a - (unsigned char)*b);
}