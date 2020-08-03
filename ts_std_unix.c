#include <ctype.h>

#include "ts_std.h"

const char *strlwr(char *s) {
  const char *p = s;
  while (*s) {
    *s = tolower(*s);
    s++;
  }
  return p;
}
