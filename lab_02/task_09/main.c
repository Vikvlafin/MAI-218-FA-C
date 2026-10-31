#include "functions.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
    (void)argv;
    if (argc!=1) { fprintf(stderr,"Usage: task_09\n"); return 1; }
    const double values[]={0.1,0.125,0.5};
    int *result=NULL;
    int status=finite_fractions(2,&result,3,values[0],values[1],values[2]);
    if (!status) for (size_t i=0; i<3; ++i)
        if (printf("%.17g: %s\n",values[i],result[i] ? "finite" : "infinite")<0) status=4;
    free(result);
    if (fflush(stdout)==EOF) status=4;
    if (status) fprintf(stderr,"Fraction operation failed (%d).\n",status);
    return status;
}
