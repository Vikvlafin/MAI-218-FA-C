#include "functions.h"
#include <stdio.h>

int main(int argc, char **argv) {
    if (argc<3) { fprintf(stderr,"Usage: task_03 substring file [files...]\n"); return 1; }
    int status=0;
    if (argc==4) {
        FileMatches *results=NULL;
        status=search_files(argv[1],&results,2,argv[2],argv[3]);
        if (!status) {
            for (size_t i=0; i<2; ++i) {
                if (printf("FILE %s\n",results[i].path)<0) status=3;
                for (size_t j=0; j<results[i].count; ++j)
                    if (printf("MATCH %zu %zu\n",results[i].matches[j].line,results[i].matches[j].column)<0) status=3;
            }
        }
        if (results && free_search(results,2)) status=1;
    } else {
        for (int i=2; i<argc && !status; ++i) {
            FileMatches *results=NULL;
            status=search_files(argv[1],&results,1,argv[i]);
            if (!status) {
                if (printf("FILE %s\n",results[0].path)<0) status=3;
                for (size_t j=0; j<results[0].count; ++j)
                    if (printf("MATCH %zu %zu\n",results[0].matches[j].line,results[0].matches[j].column)<0) status=3;
            }
            if (results && free_search(results,1)) status=1;
        }
    }
    if (fflush(stdout)==EOF) status=3;
    if (status) fprintf(stderr,"Search failed (%d).\n",status);
    return status;
}
