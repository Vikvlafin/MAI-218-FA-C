#ifndef FA3_FILES_H
#define FA3_FILES_H
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

static inline int open_distinct(FILE *in, const char *path, FILE **out) {
    if (!in || !path || !*path || !out) {
        return 1;
    }
    struct stat input_info;
    struct stat output_info;
    if (fstat(fileno(in), &input_info)) {
        return 3;
    }
    int fd = open(path, O_WRONLY | O_CREAT, 0666);
    if (fd < 0) {
        return 3;
    }
    int status = 0;
    if (fstat(fd, &output_info)) {
        status = 3;
    } else if (input_info.st_dev == output_info.st_dev && input_info.st_ino == output_info.st_ino) {
        status = 1;
    }
    FILE *file = NULL;
    if (!status) {
        file = fdopen(fd, "w");
        if (!file) {
            status = 3;
        }
    }
    if (!status && S_ISREG(output_info.st_mode) && ftruncate(fd, 0)) {
        status = 3;
    }
    if (status) {
        if (file) {
            if (fclose(file) == EOF) {
                status = 3;
            }
        } else if (close(fd)) {
            status = 3;
        }
    } else {
        *out = file;
    }
    return status;
}
#endif
