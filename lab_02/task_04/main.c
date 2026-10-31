#include "functions.h"
#include "../input.h"
#include <inttypes.h>

int main(int argc, char **argv) {
    (void)argv;
    if (argc!=1) { fprintf(stderr,"Usage: task_04 < input\n"); return 1; }
    char *line=NULL,*fields[3];
    size_t n=0,capacity=0,loaded=0;
    int status=read_line(stdin,&line);
    if (!status && (split_fields(line,fields,2) || parse_size(fields[0],&n) || !n ||
        parse_size(fields[1],&capacity) || n>SIZE_MAX/sizeof(BackupFile))) status=1;
    free(line);
    BackupFile *files=NULL;
    if (!status) { files=calloc(n,sizeof(*files)); if (!files) status=2; }
    for (size_t i=0; i<n && !status; ++i) {
        line=NULL; status=read_line(stdin,&line);
        if (!status && (split_fields(line,fields,3) || parse_size(fields[1],&files[i].size) ||
            !files[i].size || parse_size(fields[2],&files[i].importance))) status=1;
        if (!status) {
            size_t len=0; while (fields[0][len]) ++len;
            files[i].name=malloc(len+1);
            if (!files[i].name) status=2;
            else { for (size_t j=0; j<=len; ++j) files[i].name[j]=fields[0][j]; ++loaded; }
        }
        free(line);
    }
    if (!status) {
        int c;
        while ((c=fgetc(stdin))!=EOF) if (!isspace((unsigned char)c)) { status=1; break; }
        if (ferror(stdin)) status=3;
    }
    BackupResult result={0};
    if (!status) status=choose_backup(files,n,capacity,&result);
    if (!status) {
        if (printf("TOTAL_SIZE %zu\nTOTAL_IMPORTANCE %" PRIuMAX "\nFILES %zu\n",
                   result.total_size,result.total_importance,result.count)<0) status=3;
        for (size_t i=0; i<result.count; ++i) if (puts(result.files[i].name)==EOF) status=3;
    }
    free(result.files);
    for (size_t i=0; i<loaded; ++i) free(files[i].name);
    free(files);
    if (fflush(stdout)==EOF) status=3;
    if (status) fprintf(stderr,"Backup operation failed (%d).\n",status);
    return status;
}
