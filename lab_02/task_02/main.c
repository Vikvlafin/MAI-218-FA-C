#define _POSIX_C_SOURCE 200809L
#include "functions.h"
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

static int open_output(const char *path, const struct stat *input, FILE **result) {
    if (!path || !input || !result) return 1;
    int descriptor=open(path,O_WRONLY|O_CREAT,0666);
    if (descriptor<0) return 3;
    struct stat output;
    int status=0;
    if (fstat(descriptor,&output)!=0) status=3;
    else if (input->st_dev==output.st_dev && input->st_ino==output.st_ino) status=1;
    FILE *file=NULL;
    if (!status) {
        file=fdopen(descriptor,"w");
        if (!file) status=3;
    }
    if (!status && S_ISREG(output.st_mode) && ftruncate(descriptor,0)!=0) status=3;
    if (status) {
        if (file) { if (fclose(file)==EOF) status=3; }
        else if (close(descriptor)!=0) status=3;
    } else *result=file;
    return status;
}

int main(int argc, char **argv) {
    if (argc!=4 || (argv[2][0]!='-' && argv[2][0]!='/') ||
        (argv[2][1]!='a' && argv[2][1]!='d') || argv[2][2]) {
        fprintf(stderr,"Usage: task_02 input -a|-d|/a|/d output\n"); return 1;
    }
    FILE *in=fopen(argv[1],"r");
    if (!in) { fprintf(stderr,"Cannot open input.\n"); return 3; }
    struct stat input;
    Employee *items=NULL; size_t count=0;
    int status=fstat(fileno(in),&input)==0 ? 0 : 3;
    if (!status) status=read_employees(in,1e-9,&items,&count);
    if (!status) status=sort_employees(items,count,argv[2][1]=='d');
    if (!status) {
        FILE *out=NULL;
        status=open_output(argv[3],&input,&out);
        if (!status) {
            status=write_employees(out,items,count);
            if (fclose(out)==EOF) status=3;
        }
    }
    if (fclose(in)==EOF) status=3;
    if (free_employees(items,count)) status=1;
    if (status) fprintf(stderr,"Employee operation failed (%d).\n",status);
    return status;
}
