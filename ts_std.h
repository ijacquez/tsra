#ifndef TS_STD_H
#define TS_STD_H

#include <stddef.h>

int stricmp(const char *str1, const char *str2);
const char *strlwr(char *s);
int snprintf(char *s, size_t n, const char *format, ...);

#endif
