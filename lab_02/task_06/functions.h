#ifndef FA2_SCAN_H
#define FA2_SCAN_H
#include <stdio.h>
int overfscanf(FILE *stream, const char *format, ...);
int oversscanf(const char *buffer, const char *format, ...);
#endif
