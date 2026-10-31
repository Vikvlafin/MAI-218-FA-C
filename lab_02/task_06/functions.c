#include "functions.h"
#include "../format.h"
#include <stddef.h>
#include <stdarg.h>
#include <sys/types.h>
#include <wchar.h>

static int assign_count(va_list *args, const char *length, size_t count) {
    if (count>INT_MAX) return 1;
    if (!strcmp(length,"hh")) { signed char *p=va_arg(*args,signed char *); if (!p || count>SCHAR_MAX) return 1; *p=(signed char)count; }
    else if (!strcmp(length,"h")) { short *p=va_arg(*args,short *); if (!p || count>SHRT_MAX) return 1; *p=(short)count; }
    else if (!strcmp(length,"ll")) { long long *p=va_arg(*args,long long *); if (!p) return 1; *p=(long long)count; }
    else if (!strcmp(length,"l")) { long *p=va_arg(*args,long *); if (!p) return 1; *p=(long)count; }
    else if (!strcmp(length,"j")) { intmax_t *p=va_arg(*args,intmax_t *); if (!p) return 1; *p=(intmax_t)count; }
    else if (!strcmp(length,"z")) { ssize_t *p=va_arg(*args,ssize_t *); if (!p) return 1; *p=(ssize_t)count; }
    else if (!strcmp(length,"t")) { ptrdiff_t *p=va_arg(*args,ptrdiff_t *); if (!p) return 1; *p=(ptrdiff_t)count; }
    else { int *p=va_arg(*args,int *); if (!p) return 1; *p=(int)count; }
    return 0;
}

static int scan_standard(FILE *file, const char *token, char conversion, const char *length,
                         int suppressed, va_list *args, int *consumed) {
    if (!file || !token || !length || !args || !consumed) return -2;
    *consumed=-1;
    if (suppressed) return fscanf(file,token,consumed);
    int matched;
#define SCAN_VALUE(type) do { type *p=va_arg(*args,type *); if (!p) return -2; matched=fscanf(file,token,p,consumed); } while (0)
    if (conversion=='d' || conversion=='i') {
        if (!strcmp(length,"hh")) SCAN_VALUE(signed char);
        else if (!strcmp(length,"h")) SCAN_VALUE(short);
        else if (!strcmp(length,"ll")) SCAN_VALUE(long long);
        else if (!strcmp(length,"l")) SCAN_VALUE(long);
        else if (!strcmp(length,"j")) SCAN_VALUE(intmax_t);
        else if (!strcmp(length,"z")) SCAN_VALUE(ssize_t);
        else if (!strcmp(length,"t")) SCAN_VALUE(ptrdiff_t);
        else SCAN_VALUE(int);
    } else if (strchr("ouxX",conversion)) {
        if (!strcmp(length,"hh")) SCAN_VALUE(unsigned char);
        else if (!strcmp(length,"h")) SCAN_VALUE(unsigned short);
        else if (!strcmp(length,"ll")) SCAN_VALUE(unsigned long long);
        else if (!strcmp(length,"l")) SCAN_VALUE(unsigned long);
        else if (!strcmp(length,"j")) SCAN_VALUE(uintmax_t);
        else if (!strcmp(length,"z") || !strcmp(length,"t")) SCAN_VALUE(size_t);
        else SCAN_VALUE(unsigned int);
    } else if (strchr("fFeEgGaA",conversion)) {
        if (!strcmp(length,"L")) SCAN_VALUE(long double);
        else if (!strcmp(length,"l")) SCAN_VALUE(double);
        else SCAN_VALUE(float);
    } else if (conversion=='s' || conversion=='c' || conversion=='[') {
        if (!strcmp(length,"l")) SCAN_VALUE(wchar_t);
        else SCAN_VALUE(char);
    } else if (conversion=='p') {
        void **p=va_arg(*args,void **);
        if (!p) return -2;
        matched=fscanf(file,token,p,consumed);
    } else return -2;
#undef SCAN_VALUE
    return matched;
}

static int custom_character(int c, int id, int base, size_t position, int *result) {
    if (!result || id<1 || id>4 || base<2 || base>36) return 1;
    *result=0;
    if (c==EOF) return 0;
    if (id==1) { *result=c!=0 && strchr("IVXLCDM",c)!=NULL; return 0; }
    if (id==2) { *result=c=='0' || c=='1'; return 0; }
    if (!position && (c=='-' || c=='+')) { *result=1; return 0; }
    int d=-1;
    if (c>='0' && c<='9') d=c-'0';
    else if (id==3 && c>='a' && c<='z') d=c-'a'+10;
    else if (id==4 && c>='A' && c<='Z') d=c-'A'+10;
    *result=d>=0 && d<base;
    return 0;
}

static int scan_custom(FILE *file, int id, va_list *args, size_t *consumed) {
    if (!file || !args || !consumed || id<1 || id>4) return -2;
    int *integer=NULL; unsigned int *zeck=NULL;
    if (id==2) zeck=va_arg(*args,unsigned int *);
    else integer=va_arg(*args,int *);
    int base=10;
    if (id==3 || id==4) { base=va_arg(*args,int); if (base<2 || base>36) base=10; }
    if ((!integer && id!=2) || (!zeck && id==2)) return -2;
    size_t n=0,capacity=64;
    char *text=malloc(capacity);
    if (!text) return -2;
    int c;
    *consumed=0;
    do { c=fgetc(file); if (c!=EOF && isspace((unsigned char)c)) ++*consumed; }
    while (c!=EOF && isspace((unsigned char)c));
    for (;;) {
        int allowed=0;
        if (custom_character(c,id,base,n,&allowed)) { free(text); return -2; }
        if (!allowed) break;
        if (n==capacity-1) {
            if (capacity>SIZE_MAX/2) { free(text); return -2; }
            char *next=realloc(text,capacity*2);
            if (!next) { free(text); return -2; }
            text=next; capacity*=2;
        }
        text[n++]=(char)c; ++*consumed;
        if (id==2 && n>=2 && text[n-1]=='1' && text[n-2]=='1') { c=EOF; break; }
        c=fgetc(file);
    }
    if (c!=EOF && ungetc(c,file)==EOF) { free(text); return -2; }
    text[n]='\0';
    int status;
    if (id==1) status=decode_roman(text,integer);
    else if (id==2) status=decode_zeck(text,zeck);
    else status=decode_base(text,base,id==4,integer);
    free(text);
    if (ferror(file)) return -2;
    if (!n && feof(file)) return EOF;
    return status ? 0 : 1;
}

static int scan(FILE *file, const char *format, va_list *args) {
    if (!file || !format || !args) return EOF;
    int assignments=0;
    size_t total=0;
    for (size_t i=0; format[i];) {
        if (isspace((unsigned char)format[i])) {
            while (isspace((unsigned char)format[i])) ++i;
            int c;
            do { c=fgetc(file); if (c!=EOF && isspace((unsigned char)c)) ++total; }
            while (c!=EOF && isspace((unsigned char)c));
            if (c!=EOF && ungetc(c,file)==EOF) return assignments ? assignments : EOF;
            continue;
        }
        if (format[i]!='%' || format[i+1]=='%') {
            char expected=format[i]; i+=format[i]=='%' ? 2 : 1;
            int c=fgetc(file);
            if (expected=='%') {
                while (c!=EOF && isspace((unsigned char)c)) { ++total; c=fgetc(file); }
            }
            if (c==EOF) return assignments ? assignments : EOF;
            if (c!=(unsigned char)expected) {
                if (ungetc(c,file)==EOF) return assignments ? assignments : EOF;
                return assignments;
            }
            ++total; continue;
        }
        int id=0;
        if (custom_format(format+i,&id)) return assignments ? assignments : EOF;
        if (id) {
            size_t consumed=0;
            int matched=scan_custom(file,id,args,&consumed);
            if (matched!=1) return assignments ? assignments : matched<0 ? EOF : 0;
            if (consumed>SIZE_MAX-total || assignments==INT_MAX) return EOF;
            total+=consumed; ++assignments; i+=3; continue;
        }
        size_t used=0; char conversion,length[3]; int stars,suppressed;
        if (parse_format(format+i,1,&used,&conversion,length,&stars,&suppressed)) return assignments ? assignments : EOF;
        if (conversion=='n') {
            if (suppressed || used!=2+strlen(length) || assign_count(args,length,total)) return assignments ? assignments : EOF;
            i+=used; continue;
        }
        if (used>SIZE_MAX-3) return assignments ? assignments : EOF;
        char *token=malloc(used+3);
        if (!token) return assignments ? assignments : EOF;
        memcpy(token,format+i,used); memcpy(token+used,"%n",3);
        int consumed=-1;
        int matched=scan_standard(file,token,conversion,length,suppressed,args,&consumed);
        free(token);
        if (consumed<0) return assignments ? assignments : matched<0 ? EOF : 0;
        if ((size_t)consumed>SIZE_MAX-total || assignments==INT_MAX) return EOF;
        total+=(size_t)consumed;
        if (!suppressed) ++assignments;
        i+=used;
    }
    return assignments;
}

int overfscanf(FILE *stream, const char *format, ...) {
    if (!stream || !format) return EOF;
    va_list args; va_start(args,format);
    int result=scan(stream,format,&args); va_end(args);
    return result;
}

int oversscanf(const char *buffer, const char *format, ...) {
    if (!buffer || !format) return EOF;
    FILE *file=tmpfile();
    if (!file) return EOF;
    size_t n=strlen(buffer);
    int result=EOF;
    if (fwrite(buffer,1,n,file)==n && fseek(file,0,SEEK_SET)==0) {
        va_list args; va_start(args,format);
        result=scan(file,format,&args); va_end(args);
    }
    if (fclose(file)==EOF) result=EOF;
    return result;
}
