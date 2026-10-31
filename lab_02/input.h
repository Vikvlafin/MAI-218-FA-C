#ifndef FA2_INPUT_H
#define FA2_INPUT_H
#include <ctype.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

static inline int read_line(FILE *file, char **result) {
    if (!file || !result) return 1;
    size_t n=0,capacity=64;
    char *s=malloc(capacity);
    if (!s) return 2;
    int c;
    while ((c=fgetc(file))!=EOF && c!='\n') {
        if (!c) { free(s); return 1; }
        if (n==capacity-1) {
            if (capacity>SIZE_MAX/2) { free(s); return 2; }
            char *next=realloc(s,capacity*2);
            if (!next) { free(s); return 2; }
            s=next; capacity*=2;
        }
        s[n++]=(char)c;
    }
    if (ferror(file)) { free(s); return 3; }
    if (c==EOF && !n) { free(s); return 4; }
    s[n]='\0'; *result=s;
    return 0;
}

static inline int split_fields(char *line, char **fields, size_t count) {
    if (!line || !fields) return 1;
    size_t i=0;
    char *p=line;
    while (*p) {
        while (isspace((unsigned char)*p)) ++p;
        if (!*p) break;
        if (i==count) return 1;
        fields[i++]=p;
        while (*p && !isspace((unsigned char)*p)) ++p;
        if (*p) *p++='\0';
    }
    return i==count ? 0 : 1;
}

static inline int parse_size(const char *s, size_t *result) {
    if (!s || !*s || !result) return 1;
    size_t value=0;
    for (size_t i=0; s[i]; ++i) {
        unsigned int d=(unsigned char)s[i]-'0';
        if (d>9 || value>(SIZE_MAX-d)/10) return 1;
        value=value*10+d;
    }
    *result=value;
    return 0;
}

static inline int parse_real(const char *s, double *result) {
    if (!s || !*s || !result) return 1;
    const char *p=s;
    if (*p=='+' || *p=='-') ++p;
    size_t digits=0;
    int nonzero=0;
    while (isdigit((unsigned char)*p)) { nonzero|=*p!='0'; ++digits; ++p; }
    if (*p=='.') {
        ++p;
        while (isdigit((unsigned char)*p)) { nonzero|=*p!='0'; ++digits; ++p; }
    }
    if (!digits) return 1;
    if (*p=='e' || *p=='E') {
        ++p; if (*p=='+' || *p=='-') ++p;
        if (!isdigit((unsigned char)*p)) return 1;
        while (isdigit((unsigned char)*p)) ++p;
    }
    if (*p) return 1;
    char *end;
    double value=strtod(s,&end);
    if (*end || !isfinite(value) || (value==0 && nonzero)) return 1;
    *result=value;
    return 0;
}
#endif
