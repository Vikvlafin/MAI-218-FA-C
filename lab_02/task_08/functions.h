#ifndef FA2_SUM_H
#define FA2_SUM_H
#include <stddef.h>
int add_numbers(const char *a, const char *b, int base, char **result);
int sum_numbers(int base, char **result, size_t count, ...);
#endif
