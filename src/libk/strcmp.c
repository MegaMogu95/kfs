#include "libk.h"

int	strcmp(const char *a, const char *b)
{
	while (*a == *b && *a)
	{
		a++;
		b++;
	}
	return ((unsigned char)*a - (unsigned char)*b);
}
