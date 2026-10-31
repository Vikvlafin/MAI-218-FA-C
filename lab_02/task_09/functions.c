#include "functions.h"
#include <float.h>
#include <math.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int decimal_fraction(double number, char *digits, int *denominator_power) {
    if (!digits || !denominator_power || !isfinite(number) || number<=0 || number>=1) return 1;
    char text[64];
    int found=0;
    for (int precision=1; precision<=DBL_DECIMAL_DIG; ++precision) {
        int n=snprintf(text,sizeof(text),"%.*g",precision,number);
        if (n<0 || (size_t)n>=sizeof(text)) return 3;
        double restored=strtod(text,NULL);
        if (!memcmp(&restored,&number,sizeof(number))) { found=1; break; }
    }
    if (!found) return 3;
    int fractional=0,after_point=0,used=0,exponent=0;
    const char *p=text;
    while (*p && *p!='e' && *p!='E') {
        if (*p=='.') after_point=1;
        else {
            if (*p<'0' || *p>'9' || used>=62) return 3;
            digits[used++]=*p;
            fractional+=after_point;
        }
        ++p;
    }
    digits[used]='\0';
    if (*p) {
        ++p; int sign=1;
        if (*p=='-' || *p=='+') { if (*p=='-') sign=-1; ++p; }
        while (*p) {
            if (*p<'0' || *p>'9' || exponent>1000) return 3;
            exponent=exponent*10+*p++-'0';
        }
        exponent*=sign;
    }
    *denominator_power=fractional-exponent;
    return 0;
}

static int prime_power(const char *digits, int prime, int limit, int *power) {
    if (!digits || !power || (prime!=2 && prime!=5) || limit<0) return 1;
    char current[64],quotient[64];
    size_t n=strlen(digits);
    if (n>=sizeof(current)) return 1;
    memcpy(current,digits,n+1);
    *power=0;
    while (*power<limit) {
        int remainder=0;
        for (size_t i=0; i<n; ++i) {
            int value=remainder*10+current[i]-'0';
            quotient[i]=(char)('0'+value/prime); remainder=value%prime;
        }
        quotient[n]='\0';
        if (remainder) break;
        memcpy(current,quotient,n+1); ++*power;
    }
    return 0;
}

int finite_fractions(int base, int **result, size_t count, ...) {
    if (base<2 || !result || count>SIZE_MAX/sizeof(int)) return 1;
    int *out=count ? malloc(count*sizeof(*out)) : NULL;
    if (count && !out) return 2;
    va_list args; va_start(args,count);
    int status=0;
    for (size_t i=0; i<count && !status; ++i) {
        char digits[64]; int k=0,two=0,five=0;
        status=decimal_fraction(va_arg(args,double),digits,&k);
        if (!status && k>0) status=prime_power(digits,2,k,&two);
        if (!status && k>0) status=prime_power(digits,5,k,&five);
        if (!status) out[i]=(k<=0 || ((two>=k || base%2==0) && (five>=k || base%5==0)));
    }
    va_end(args);
    if (status) free(out); else *result=out;
    return status;
}
