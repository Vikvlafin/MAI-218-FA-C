#include "functions.h"
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static int digit_value(char c, int *digit) {
    if (!digit) return 1;
    if (c>='0' && c<='9') *digit=c-'0';
    else if (c>='a' && c<='z') *digit=c-'a'+10;
    else if (c>='A' && c<='Z') *digit=c-'A'+10;
    else return 1;
    return 0;
}

static int validate_number(const char *s, int base, size_t *length) {
    if (!s || !length || !*s || base<2 || base>36) return 1;
    size_t n=0;
    for (; s[n]; ++n) {
        int d;
        if (digit_value(s[n],&d) || d>=base) return 1;
    }
    *length=n;
    return 0;
}

int add_numbers(const char *a, const char *b, int base, char **result) {
    size_t na,nb;
    if (!result || validate_number(a,base,&na) || validate_number(b,base,&nb)) return 1;
    size_t n=na>nb ? na : nb;
    if (n>SIZE_MAX-2) return 1;
    char *out=malloc(n+2);
    if (!out) return 2;
    out[n+1]='\0';
    int carry=0;
    for (size_t i=0; i<n; ++i) {
        int x=0,y=0;
        if (i<na) digit_value(a[na-1-i],&x);
        if (i<nb) digit_value(b[nb-1-i],&y);
        int sum=x+y+carry;
        int d=sum%base; carry=sum/base;
        out[n-i]=(char)(d<10 ? '0'+d : 'A'+d-10);
    }
    out[0]=(char)('0'+carry);
    size_t start=0;
    while (out[start]=='0' && out[start+1]) ++start;
    memmove(out,out+start,n+2-start);
    *result=out;
    return 0;
}

int sum_numbers(int base, char **result, size_t count, ...) {
    if (!result || base<2 || base>36) return 1;
    char *total=malloc(2);
    if (!total) return 2;
    total[0]='0'; total[1]='\0';
    va_list args; va_start(args,count);
    int status=0;
    for (size_t i=0; i<count && !status; ++i) {
        char *next=NULL;
        status=add_numbers(total,va_arg(args,const char *),base,&next);
        if (!status) { free(total); total=next; }
    }
    va_end(args);
    if (status) free(total); else *result=total;
    return status;
}
