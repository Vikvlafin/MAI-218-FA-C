#include "functions.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

int choose_backup(const BackupFile *files, size_t count, size_t capacity, BackupResult *result) {
    if (!files || !count || !result || capacity==SIZE_MAX || count==SIZE_MAX) return 1;
    for (size_t i=0; i<count; ++i) {
        if (!files[i].name || !files[i].name[0] || !files[i].size) return 1;
        for (const char *p=files[i].name; *p; ++p) if (isspace((unsigned char)*p)) return 1;
        for (size_t j=0; j<i; ++j) if (!strcmp(files[i].name,files[j].name)) return 1;
    }
    size_t width=capacity+1;
    if (count+1>SIZE_MAX/width || (count+1)*width>SIZE_MAX/sizeof(uintmax_t) ||
        count>SIZE_MAX/sizeof(BackupFile)) return 2;
    uintmax_t *dp=calloc((count+1)*width,sizeof(*dp));
    BackupFile *chosen=malloc(count*sizeof(*chosen));
    if (!dp || !chosen) { free(dp); free(chosen); return 2; }
    int status=0;
    for (size_t i=1; i<=count && !status; ++i) {
        const BackupFile *file=&files[i-1];
        for (size_t c=0; c<=capacity; ++c) {
            uintmax_t best=dp[(i-1)*width+c];
            if (file->size<=c) {
                uintmax_t previous=dp[(i-1)*width+c-file->size];
                if (file->importance>UINTMAX_MAX-previous) { status=3; break; }
                uintmax_t candidate=previous+file->importance;
                if (candidate>best) best=candidate;
            }
            dp[i*width+c]=best;
        }
    }
    BackupResult out={chosen,0,0,0};
    if (!status) {
        size_t c=capacity;
        out.total_importance=dp[count*width+c];
        for (size_t i=count; i>0; --i) {
            if (dp[i*width+c]!=dp[(i-1)*width+c]) {
                chosen[out.count++]=files[i-1];
                c-=files[i-1].size;
            }
        }
        out.total_size=capacity-c;
        for (size_t i=0; i<out.count/2; ++i) {
            BackupFile t=chosen[i]; chosen[i]=chosen[out.count-i-1]; chosen[out.count-i-1]=t;
        }
        *result=out;
    } else free(chosen);
    free(dp);
    return status;
}
