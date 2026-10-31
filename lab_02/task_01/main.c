#include "functions.h"
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
    if (argc<3 || argv[1][0]!='-' || !argv[1][1] || argv[1][2]) {
        fprintf(stderr,"Usage: task_01 -l|-r|-u|-n string; task_01 -c string seed [strings...]\n");
        return 1;
    }
    char flag=argv[1][1], *result=NULL;
    int status=1;
    if (flag=='c') {
        if (argc<4 || !argv[3][0]) return 1;
        unsigned int seed=0;
        for (size_t i=0; argv[3][i]; ++i) {
            unsigned int d=(unsigned char)argv[3][i]-'0';
            if (d>9 || seed>(UINT_MAX-d)/10) return 1;
            seed=seed*10+d;
        }
        size_t count=(size_t)argc-3;
        const char **parts=malloc(count*sizeof(*parts));
        if (!parts) return 2;
        parts[0]=argv[2];
        for (size_t i=1; i<count; ++i) parts[i]=argv[i+3];
        status=string_shuffle(parts,count,seed,&result); free(parts);
    } else {
        if (argc!=3) return 1;
        if (flag=='l') {
            size_t n;
            status=string_length(argv[2],&n);
            if (!status && printf("%zu\n",n)<0) status=3;
            if (fflush(stdout)==EOF) status=3;
            return status;
        }
        if (flag=='r') status=string_reverse(argv[2],&result);
        else if (flag=='u') status=string_upper_odd(argv[2],&result);
        else if (flag=='n') status=string_group(argv[2],&result);
    }
    if (!status && puts(result)==EOF) status=3;
    free(result);
    if (fflush(stdout)==EOF) status=3;
    if (status) fprintf(stderr,"String operation failed (%d).\n",status);
    return status;
}
