#include "functions.h"
#include <ctype.h>
#include <stdint.h>
#include <stdlib.h>

int string_length(const char *s, size_t *length) {
    if (!s || !length) return 1;
    size_t n = 0;
    while (s[n]) ++n;
    *length = n;
    return 0;
}

static int allocate_string(const char *s, char **result, size_t *n) {
    if (!result || string_length(s,n) || *n == SIZE_MAX) return 1;
    *result = malloc(*n + 1);
    if (!*result) return 2;
    (*result)[*n] = '\0';
    return 0;
}

int string_reverse(const char *s, char **result) {
    size_t n;
    int status = allocate_string(s,result,&n);
    if (status) return status;
    for (size_t i=0; i<n; ++i) (*result)[i]=s[n-i-1];
    return 0;
}

int string_upper_odd(const char *s, char **result) {
    size_t n;
    int status = allocate_string(s,result,&n);
    if (status) return status;
    for (size_t i=0; i<n; ++i)
        (*result)[i] = i%2 ? (char)toupper((unsigned char)s[i]) : s[i];
    return 0;
}

int string_group(const char *s, char **result) {
    size_t n;
    int status = allocate_string(s,result,&n);
    if (status) return status;
    size_t k=0;
    for (int group=0; group<3; ++group) {
        for (size_t i=0; i<n; ++i) {
            unsigned char c=(unsigned char)s[i];
            int category=isdigit(c) ? 0 : isalpha(c) ? 1 : 2;
            if (category==group) (*result)[k++]=s[i];
        }
    }
    return 0;
}

int string_shuffle(const char *const *strings, size_t count, unsigned int seed, char **result) {
    if (!strings || !count || !result || count>SIZE_MAX/sizeof(size_t)) return 1;
    size_t total=0;
    for (size_t i=0; i<count; ++i) {
        size_t n;
        if (string_length(strings[i],&n) || n>=SIZE_MAX-total) return 1;
        total+=n;
    }
    size_t *order=malloc(count*sizeof(*order));
    char *out=malloc(total+1);
    if (!order || !out) { free(order); free(out); return 2; }
    for (size_t i=0; i<count; ++i) order[i]=i;
    srand(seed);
    for (size_t i=count; i>1; --i) {
        size_t j=(size_t)rand()%i;
        size_t t=order[i-1]; order[i-1]=order[j]; order[j]=t;
    }
    size_t k=0;
    for (size_t i=0; i<count; ++i)
        for (size_t j=0; strings[order[i]][j]; ++j) out[k++]=strings[order[i]][j];
    out[k]='\0'; free(order); *result=out;
    return 0;
}
