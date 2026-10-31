#ifndef FA2_FORMAT_H
#define FA2_FORMAT_H
#include <ctype.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static inline int custom_format(const char *s, int *id) {
    if (!s || s[0]!='%' || !id) return 1;
    *id=0;
    if (!s[1] || !s[2]) return 0;
    if (s[1]=='R' && s[2]=='o') *id=1;
    else if (s[1]=='Z' && s[2]=='r') *id=2;
    else if (s[1]=='C' && s[2]=='v') *id=3;
    else if (s[1]=='C' && s[2]=='V') *id=4;
    else if (s[1]=='t' && s[2]=='o') *id=5;
    else if (s[1]=='T' && s[2]=='O') *id=6;
    else if (s[1]=='m' && s[2]=='i') *id=7;
    else if (s[1]=='m' && s[2]=='u') *id=8;
    else if (s[1]=='m' && s[2]=='d') *id=9;
    else if (s[1]=='m' && s[2]=='f') *id=10;
    return 0;
}

static inline int parse_format(const char *s, int input, size_t *used, char *conversion,
                               char length[3], int *stars, int *suppressed) {
    if (!s || s[0]!='%' || !used || !conversion || !length || !stars || !suppressed) return 1;
    size_t i=1;
    *stars=0; *suppressed=0; length[0]='\0';
    if (input) { if (s[i]=='*') { *suppressed=1; ++i; } }
    else while (s[i] && strchr("-+ #0",s[i])) ++i;
    if (!input && s[i]=='*') { ++*stars; ++i; }
    else {
        unsigned int width=0;
        while (isdigit((unsigned char)s[i])) {
            unsigned int d=(unsigned int)(s[i]-'0');
            if (width>((unsigned int)INT_MAX-d)/10) return 1;
            width=width*10+d; ++i;
        }
    }
    if (!input && s[i]=='.') {
        ++i;
        if (s[i]=='*') { ++*stars; ++i; }
        else {
            unsigned int precision=0;
            while (isdigit((unsigned char)s[i])) {
                unsigned int d=(unsigned int)(s[i]-'0');
                if (precision>((unsigned int)INT_MAX-d)/10) return 1;
                precision=precision*10+d; ++i;
            }
        }
    }
    if (s[i] && strchr("hljztL",s[i])) {
        length[0]=s[i++]; length[1]='\0';
        if ((length[0]=='h' || length[0]=='l') && s[i]==length[0]) {
            length[1]=s[i++]; length[2]='\0';
        }
    }
    char c=s[i];
    if (!c || !(input ? strchr("diouxXfFeEgGaAcspn[%",c) : strchr("diouxXfFeEgGaAcspn%",c))) return 1;
    if (strchr("diouxXn",c)) {
        if (length[0]=='L') return 1;
    } else if (strchr("fFeEgGaA",c)) {
        if (length[0] && strcmp(length,"l") && strcmp(length,"L")) return 1;
    } else if (c=='c' || c=='s' || c=='[') {
        if (length[0] && strcmp(length,"l")) return 1;
    } else if (length[0]) return 1;
    ++i;
    if (c=='[') {
        if (s[i]=='^') ++i;
        if (s[i]==']') ++i;
        while (s[i] && s[i]!=']') ++i;
        if (!s[i]) return 1;
        ++i;
    }
    if (c=='%' && i!=2) return 1;
    *used=i; *conversion=c;
    return 0;
}

static inline int encode_roman(int number, char *result, size_t capacity) {
    if (!result || number<1 || number>3999) return 1;
    const int values[]={1000,900,500,400,100,90,50,40,10,9,5,4,1};
    const char *const symbols[]={"M","CM","D","CD","C","XC","L","XL","X","IX","V","IV","I"};
    size_t n=0;
    for (size_t i=0; i<13; ++i) while (number>=values[i]) {
        for (size_t j=0; symbols[i][j]; ++j) {
            if (n+1>=capacity) return 1;
            result[n++]=symbols[i][j];
        }
        number-=values[i];
    }
    result[n]='\0';
    return 0;
}

static inline int decode_roman(const char *text, int *result) {
    if (!text || !*text || !result) return 1;
    int total=0,previous=0;
    for (size_t i=strlen(text); i>0; --i) {
        int value=0;
        switch (text[i-1]) {
            case 'I': value=1; break; case 'V': value=5; break;
            case 'X': value=10; break; case 'L': value=50; break;
            case 'C': value=100; break; case 'D': value=500; break;
            case 'M': value=1000; break; default: return 1;
        }
        total+=value<previous ? -value : value;
        previous=value;
        if (total>3999) return 1;
    }
    char canonical[32];
    if (encode_roman(total,canonical,sizeof(canonical)) || strcmp(text,canonical)) return 1;
    *result=total;
    return 0;
}

static inline int encode_zeck(unsigned int number, char *result, size_t capacity) {
    if (!result || capacity<3) return 1;
    if (!number) { memcpy(result,"01",3); return 0; }
    uintmax_t fibonacci[sizeof(unsigned int)*CHAR_BIT*2];
    size_t n=2; fibonacci[0]=1; fibonacci[1]=2;
    while (fibonacci[n-1]<=UINT_MAX && n<sizeof(fibonacci)/sizeof(*fibonacci)) {
        fibonacci[n]=fibonacci[n-1]+fibonacci[n-2]; ++n;
    }
    while (n>1 && fibonacci[n-1]>number) --n;
    if (capacity<n+2) return 1;
    memset(result,'0',n);
    for (size_t i=n; i>0; --i) if (fibonacci[i-1]<=number) {
        result[i-1]='1'; number-=(unsigned int)fibonacci[i-1];
    }
    result[n]='1'; result[n+1]='\0';
    return 0;
}

static inline int decode_zeck(const char *text, unsigned int *result) {
    if (!text || !result) return 1;
    size_t n=strlen(text);
    if (n<2 || text[n-1]!='1' || n>sizeof(unsigned int)*CHAR_BIT*2) return 1;
    uintmax_t a=1,b=2,total=0;
    for (size_t i=0; i<n-1; ++i) {
        if (text[i]!='0' && text[i]!='1') return 1;
        if (text[i]=='1') {
            if (i && text[i-1]=='1') return 1;
            if (a>UINT_MAX-total) return 1;
            total+=a;
        }
        if (i+1<n-1) {
            if (b>UINTMAX_MAX-a) return 1;
            uintmax_t next=a+b; a=b; b=next;
        }
    }
    char canonical[sizeof(unsigned int)*CHAR_BIT*2+2];
    if (encode_zeck((unsigned int)total,canonical,sizeof(canonical)) || strcmp(text,canonical)) return 1;
    *result=(unsigned int)total;
    return 0;
}

static inline int encode_base(int number, int base, int upper, char *result, size_t capacity) {
    if (!result) return 1;
    if (base<2 || base>36) base=10;
    unsigned int magnitude=number<0 ? 0u-(unsigned int)number : (unsigned int)number;
    char reverse[sizeof(int)*CHAR_BIT+1]; size_t n=0,k=0;
    do {
        unsigned int d=magnitude%(unsigned int)base;
        reverse[n++]=(char)(d<10 ? '0'+d : (upper ? 'A' : 'a')+d-10);
        magnitude/=(unsigned int)base;
    } while (magnitude);
    if (capacity<n+(number<0 ? 2u : 1u)) return 1;
    if (number<0) result[k++]='-';
    while (n) result[k++]=reverse[--n];
    result[k]='\0';
    return 0;
}

static inline int decode_base(const char *text, int base, int upper, int *result) {
    if (!text || !*text || !result) return 1;
    if (base<2 || base>36) base=10;
    int negative=*text=='-';
    if (*text=='-' || *text=='+') ++text;
    if (!*text) return 1;
    unsigned int limit=negative ? 0u-(unsigned int)INT_MIN : (unsigned int)INT_MAX,value=0;
    for (; *text; ++text) {
        unsigned int d;
        if (*text>='0' && *text<='9') d=(unsigned int)(*text-'0');
        else if (upper && *text>='A' && *text<='Z') d=(unsigned int)(*text-'A'+10);
        else if (!upper && *text>='a' && *text<='z') d=(unsigned int)(*text-'a'+10);
        else return 1;
        if (d>=(unsigned int)base || value>(limit-d)/(unsigned int)base) return 1;
        value=value*(unsigned int)base+d;
    }
    *result=negative ? (value==0u-(unsigned int)INT_MIN ? INT_MIN : -(int)value) : (int)value;
    return 0;
}
#endif
