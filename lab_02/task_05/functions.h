#ifndef FA2_PRINT_H
#define FA2_PRINT_H
#include <stdio.h>
int overfprintf(FILE *stream, const char *format, ...);
int oversprintf(char *buffer, const char *format, ...);
#endif
