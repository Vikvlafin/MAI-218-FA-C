#include "functions.h"
#include <stdio.h>

int main(int argc, char **argv) {
    (void)argv;
    if (argc!=1) { fprintf(stderr,"Usage: task_07\n"); return 1; }
    const double x[]={3,4},y[]={-3,-4},z[]={5,0},p=2,matrix[]={2,0,0,1};
    Vector a={x},b={y},c={z};
    NormSpec inf={norm_infinity,NULL},lp={norm_p,&p},quadratic={norm_matrix,matrix};
    NormResult *results=NULL;
    int status=longest_vectors(2,1e-10,&results,3,3,a,b,c,inf,lp,quadratic);
    if (!status) {
        for (size_t i=0; i<3; ++i) {
            if (printf("NORM %zu\n",i+1)<0) status=4;
            for (size_t j=0; j<results[i].count; ++j)
                if (printf("(%.17g, %.17g)\n",results[i].vectors[j].coordinates[0],
                           results[i].vectors[j].coordinates[1])<0) status=4;
        }
    }
    if (results && free_norm_results(results,3)) status=1;
    if (fflush(stdout)==EOF) status=4;
    if (status) fprintf(stderr,"Norm operation failed (%d).\n",status);
    return status;
}
