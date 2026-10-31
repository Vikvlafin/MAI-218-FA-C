#include "functions.h"

int main(int argc, char **argv) {
    (void)argv;
    if (argc!=1) { fprintf(stderr,"Usage: task_05\n"); return 1; }
    char text[512];
    if (oversprintf(text,"%Ro %Zr %Cv %CV %to %TO",1994,4u,255,16,255,16,"ff",16,"FF",16)<0) return 1;
    if (overfprintf(stdout,"%s\n%mi\n%mu\n%md\n%mf\n",text,-1,1u,1.0,1.0)<0) return 1;
    return fflush(stdout)==EOF ? 1 : 0;
}
