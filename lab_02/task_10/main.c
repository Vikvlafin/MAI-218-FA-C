#include "functions.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
    (void)argv;
    if (argc!=1) { fprintf(stderr,"Usage: task_10\n"); return 1; }
    double *g=NULL;
    int status=shift_polynomial(1e-12,2.0,&g,3,1.0,2.0,3.0,4.0);
    if (!status) {
        for (int i=0; i<=3; ++i)
            if (printf("g%d = %.17g\n",i,g[i])<0) status=4;
    }
    free(g);
    if (fflush(stdout)==EOF) status=4;
    if (status) fprintf(stderr,"Polynomial operation failed (%d).\n",status);
    return status;
}
