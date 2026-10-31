#ifndef FA2_STRINGS_H
#define FA2_STRINGS_H
#include <stddef.h>
int string_length(const char *s, size_t *length);
int string_reverse(const char *s, char **result);
int string_upper_odd(const char *s, char **result);
int string_group(const char *s, char **result);
int string_shuffle(const char *const *strings, size_t count, unsigned int seed, char **result);
#endif
