#ifndef STRING_H
# define STRING_H

# include <stddef.h>

size_t strlen(const char *s);
void  *memset(void *dst, int c, size_t n);
void  *memcpy(void *dst, const void *src, size_t n);
void  *memmove(void *dst, const void *src, size_t n);
int    strcmp(const char *a, const char *b);

#endif