#include "functions.h"
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int free_search(FileMatches *results, size_t count) {
    if (!results && count) return 1;
    if (results) for (size_t i=0; i<count; ++i) free(results[i].matches);
    free(results);
    return 0;
}

static int search_file(const char *path, const char *pattern, size_t m,
                       const size_t *prefix, FileMatches *result) {
    if (!path || !pattern || !m || !prefix || !result) return 1;
    if (m>SIZE_MAX/sizeof(Match)) return 2;
    Match *positions=malloc(m*sizeof(*positions));
    if (!positions) return 2;
    FILE *file=fopen(path,"rb");
    if (!file) { free(positions); return 3; }
    Match *matches=NULL;
    size_t count=0,match_capacity=0,j=0,line=1,column=1,slot=0;
    int status=0,c;
    while (!status && (c=fgetc(file))!=EOF) {
        positions[slot]=(Match){line,column};
        size_t next_slot=slot+1==m ? 0 : slot+1;
        while (j && (unsigned char)c!=(unsigned char)pattern[j]) j=prefix[j-1];
        if ((unsigned char)c==(unsigned char)pattern[j]) ++j;
        if (j==m) {
            if (count==match_capacity) {
                if (match_capacity>SIZE_MAX/2/sizeof(*matches)) { status=2; break; }
                size_t next_size=match_capacity ? match_capacity*2 : 16;
                Match *next=realloc(matches,next_size*sizeof(*matches));
                if (!next) { status=2; break; }
                matches=next; match_capacity=next_size;
            }
            matches[count++]=positions[next_slot];
            j=prefix[j-1];
        }
        slot=next_slot;
        if (c=='\n') {
            if (line==SIZE_MAX) status=3;
            else { ++line; column=1; }
        } else if (column==SIZE_MAX) status=3;
        else ++column;
    }
    if (ferror(file)) status=3;
    if (fclose(file)==EOF) status=3;
    free(positions);
    if (status) free(matches);
    else *result=(FileMatches){path,matches,count};
    return status;
}

int search_files(const char *substring, FileMatches **result, size_t count, ...) {
    if (!substring || !*substring || !result || !count || count>SIZE_MAX/sizeof(FileMatches)) return 1;
    size_t m=strlen(substring);
    if (m>SIZE_MAX/sizeof(size_t)) return 2;
    size_t *prefix=calloc(m,sizeof(*prefix));
    FileMatches *files=calloc(count,sizeof(*files));
    if (!prefix || !files) { free(prefix); free(files); return 2; }
    for (size_t i=1,j=0; i<m; ++i) {
        while (j && substring[i]!=substring[j]) j=prefix[j-1];
        if (substring[i]==substring[j]) ++j;
        prefix[i]=j;
    }
    va_list args; va_start(args,count);
    int status=0;
    for (size_t i=0; i<count && !status; ++i)
        status=search_file(va_arg(args,const char *),substring,m,prefix,&files[i]);
    va_end(args); free(prefix);
    if (status) { if (free_search(files,count)) status=1; }
    else *result=files;
    return status;
}
