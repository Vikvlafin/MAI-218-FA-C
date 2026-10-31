#include "functions.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
    int base=16,status;
    char *result=NULL;
    if (argc==1) status=sum_numbers(base,&result,3,"FFFF","1","10");
    else if (argc==4) {
        if (!argv[1][0]) return 1;
        base=0;
        for (size_t i=0; argv[1][i]; ++i) {
            if (argv[1][i]<'0' || argv[1][i]>'9' || base>36) return 1;
            base=base*10+argv[1][i]-'0';
        }
        status=sum_numbers(base,&result,2,argv[2],argv[3]);
    } else { fprintf(stderr,"Usage: task_08 [base number number]\n"); return 1; }
    if (!status && puts(result)==EOF) status=3;
    free(result);
    if (fflush(stdout)==EOF) status=3;
    if (status) fprintf(stderr,"Addition failed (%d).\n",status);
    return status;
}
