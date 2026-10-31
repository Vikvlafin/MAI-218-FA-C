#include "functions.h"
#include "../format.h"
#include <float.h>
#include <math.h>
#include <stddef.h>
#include <stdarg.h>
#include <sys/types.h>
#include <wchar.h>

typedef struct { char *data; size_t size; size_t capacity; } Buffer;

static int append(Buffer *out, const char *data, size_t count) {
    if (!out || (!data && count) || count>(size_t)INT_MAX-out->size) return 1;
    size_t needed=out->size+count+1;
    if (needed>out->capacity) {
        size_t capacity=out->capacity ? out->capacity : 64;
        while (capacity<needed) {
            if (capacity>SIZE_MAX/2) { capacity=needed; break; }
            capacity*=2;
        }
        char *next=realloc(out->data,capacity);
        if (!next) return 2;
        out->data=next; out->capacity=capacity;
    }
    if (count) memcpy(out->data+out->size,data,count);
    out->size+=count; out->data[out->size]='\0';
    return 0;
}

static int store_count(va_list *args, const char *length, size_t count) {
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

static int advance_argument(va_list *args, char conversion, const char *length, int stars) {
    for (int i=0; i<stars; ++i) (void)va_arg(*args,int);
    if (conversion=='d' || conversion=='i') {
        if (!strcmp(length,"ll")) (void)va_arg(*args,long long);
        else if (!strcmp(length,"l")) (void)va_arg(*args,long);
        else if (!strcmp(length,"j")) (void)va_arg(*args,intmax_t);
        else if (!strcmp(length,"z")) (void)va_arg(*args,ssize_t);
        else if (!strcmp(length,"t")) (void)va_arg(*args,ptrdiff_t);
        else (void)va_arg(*args,int);
    } else if (strchr("ouxX",conversion)) {
        if (!strcmp(length,"ll")) (void)va_arg(*args,unsigned long long);
        else if (!strcmp(length,"l")) (void)va_arg(*args,unsigned long);
        else if (!strcmp(length,"j")) (void)va_arg(*args,uintmax_t);
        else if (!strcmp(length,"z") || !strcmp(length,"t")) (void)va_arg(*args,size_t);
        else if (length[0]=='h') (void)va_arg(*args,int);
        else (void)va_arg(*args,unsigned int);
    } else if (strchr("fFeEgGaA",conversion)) {
        if (length[0]=='L') (void)va_arg(*args,long double); else (void)va_arg(*args,double);
    } else if (conversion=='c') {
        if (length[0]=='l') (void)va_arg(*args,wint_t); else (void)va_arg(*args,int);
    } else if (conversion=='s') {
        if (length[0]=='l') { if (!va_arg(*args,const wchar_t *)) return 1; }
        else if (!va_arg(*args,const char *)) return 1;
    } else if (conversion=='p') (void)va_arg(*args,void *);
    return 0;
}

static int dump_bytes(const void *value, size_t count, char *text, size_t capacity) {
    if (!value || !text || count>(SIZE_MAX-1)/9 || capacity<count*9) return 1;
    const unsigned char *bytes=value;
    size_t n=0;
    for (size_t i=0; i<count; ++i) {
        if (i) text[n++]=' ';
        for (int bit=7; bit>=0; --bit) text[n++]=(char)('0'+((bytes[i]>>bit)&1));
    }
    text[n]='\0';
    return 0;
}

static int custom_output(Buffer *out, int id, va_list *args) {
    char text[sizeof(long double)*CHAR_BIT*2+128];
    int status=0;
    switch (id) {
        case 1: status=encode_roman(va_arg(*args,int),text,sizeof(text)); break;
        case 2: status=encode_zeck(va_arg(*args,unsigned int),text,sizeof(text)); break;
        case 3: case 4: {
            int number=va_arg(*args,int),base=va_arg(*args,int);
            status=encode_base(number,base,id==4,text,sizeof(text)); break;
        }
        case 5: case 6: {
            const char *source=va_arg(*args,const char *); int base=va_arg(*args,int),number;
            status=decode_base(source,base,id==6,&number);
            if (!status) status=encode_base(number,10,0,text,sizeof(text));
            break;
        }
        case 7: {
            int number=va_arg(*args,int);
            if (sizeof(number)!=4 || CHAR_BIT!=8) return 1;
            status=dump_bytes(&number,sizeof(number),text,sizeof(text)); break;
        }
        case 8: {
            unsigned int number=va_arg(*args,unsigned int);
            if (sizeof(number)!=4 || CHAR_BIT!=8) return 1;
            status=dump_bytes(&number,sizeof(number),text,sizeof(text)); break;
        }
        case 9: {
            double number=va_arg(*args,double);
            if (CHAR_BIT!=8) return 1;
            status=dump_bytes(&number,sizeof(number),text,sizeof(text)); break;
        }
        case 10: {
            double promoted=va_arg(*args,double);
            if (isfinite(promoted) && fabs(promoted)>FLT_MAX) return 1;
            float number=(float)promoted;
            if (CHAR_BIT!=8) return 1;
            status=dump_bytes(&number,sizeof(number),text,sizeof(text)); break;
        }
        default: return 1;
    }
    return status ? status : append(out,text,strlen(text));
}

static int render(Buffer *out, const char *format, va_list *args) {
    if (!out || !format) return 1;
    int status=append(out,"",0);
    for (size_t i=0; format[i] && !status;) {
        if (format[i]!='%') { status=append(out,format+i,1); ++i; continue; }
        int id=0;
        if (custom_format(format+i,&id)) return 1;
        if (id) { status=custom_output(out,id,args); i+=3; continue; }
        size_t used=0; char conversion,length[3]; int stars,suppressed;
        status=parse_format(format+i,0,&used,&conversion,length,&stars,&suppressed);
        if (status) break;
        if (conversion=='n') {
            if (stars || used!=2+strlen(length)) return 1;
            status=store_count(args,length,out->size); i+=used; continue;
        }
        char *token=malloc(used+1);
        if (!token) return 2;
        memcpy(token,format+i,used); token[used]='\0';
        va_list check; va_copy(check,*args);
        status=advance_argument(&check,conversion,length,stars); va_end(check);
        if (!status) {
            va_list copy; va_copy(copy,*args);
            int count=vsnprintf(NULL,0,token,copy); va_end(copy);
            if (count<0 || (size_t)count>(size_t)INT_MAX-out->size) status=3;
            else {
                char *text=malloc((size_t)count+1);
                if (!text) status=2;
                else {
                    va_copy(copy,*args);
                    int written=vsnprintf(text,(size_t)count+1,token,copy); va_end(copy);
                    if (written!=count) status=3;
                    else status=append(out,text,(size_t)count);
                    free(text);
                }
            }
        }
        if (!status) status=advance_argument(args,conversion,length,stars);
        free(token); i+=used;
    }
    return status;
}

int oversprintf(char *buffer, const char *format, ...) {
    if (!buffer || !format) return -1;
    Buffer out={0};
    va_list args; va_start(args,format);
    int status=render(&out,format,&args); va_end(args);
    if (!status) memcpy(buffer,out.data,out.size+1);
    free(out.data);
    return status ? -1 : (int)out.size;
}

int overfprintf(FILE *stream, const char *format, ...) {
    if (!stream || !format) return -1;
    Buffer out={0};
    va_list args; va_start(args,format);
    int status=render(&out,format,&args); va_end(args);
    if (!status && fwrite(out.data,1,out.size,stream)!=out.size) status=3;
    free(out.data);
    return status ? -1 : (int)out.size;
}
