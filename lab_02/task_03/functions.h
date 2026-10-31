#ifndef FA2_SEARCH_H
#define FA2_SEARCH_H
#include <stddef.h>
typedef struct { size_t line; size_t column; } Match;
typedef struct { const char *path; Match *matches; size_t count; } FileMatches;
int search_files(const char *substring, FileMatches **result, size_t count, ...);
int free_search(FileMatches *results, size_t count);
#endif
