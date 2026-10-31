#include "functions.h"

int main(int argc, char **argv) {
    (void)argv;
    if (argc!=1) { fprintf(stderr,"Usage: task_06\n"); return 1; }
    int roman=0,lower=0,upper=0; unsigned int zeck=0;
    if (oversscanf("XIV 1011 ff FF","%Ro %Zr %Cv %CV",&roman,&zeck,&lower,16,&upper,16)!=4) return 1;
    if (printf("%d %u %d %d\n",roman,zeck,lower,upper)<0) return 1;
    FILE *file=tmpfile();
    if (!file) return 1;
    int status=0;
    if (fputs("XIV 1011 ff FF",file)==EOF || fseek(file,0,SEEK_SET)) status=1;
    if (!status && overfscanf(file,"%Ro %Zr %Cv %CV",&roman,&zeck,&lower,16,&upper,16)!=4) status=1;
    if (!status && printf("%d %u %d %d\n",roman,zeck,lower,upper)<0) status=1;
    if (fclose(file)==EOF || fflush(stdout)==EOF) status=1;
    return status;
}
