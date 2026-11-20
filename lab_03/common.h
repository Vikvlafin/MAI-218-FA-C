#ifndef FA3_COMMON_H
#define FA3_COMMON_H
#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

static inline int read_line(FILE *in, char **out) {
    if (!in || !out) {
        return 1;
    }
    size_t size = 0;
    size_t capacity = 64;
    char *line = malloc(capacity);
    if (!line) {
        return 2;
    }
    int c;
    while ((c = fgetc(in)) != EOF && c != '\n') {
        if (!c) {
            free(line);
            return 1;
        }
        if (size == capacity - 1) {
            if (capacity > SIZE_MAX / 2) {
                free(line);
                return 2;
            }
            char *next = realloc(line, capacity * 2);
            if (!next) {
                free(line);
                return 2;
            }
            line = next;
            capacity *= 2;
        }
        line[size] = (char)c;
        ++size;
    }
    if (ferror(in)) {
        free(line);
        return 3;
    }
    if (c == EOF && !size) {
        free(line);
        return 4;
    }
    if (size && line[size - 1] == '\r') {
        --size;
    }
    line[size] = '\0';
    *out = line;
    return 0;
}

static inline int fields(char *line, char **out, size_t limit, size_t *count) {
    if (!line || !out || !count) {
        return 1;
    }
    size_t n = 0;
    while (*line) {
        while (isspace((unsigned char)*line)) {
            ++line;
        }
        if (!*line) {
            break;
        }
        if (n == limit) {
            return 1;
        }
        out[n] = line;
        ++n;
        while (*line && !isspace((unsigned char)*line)) {
            ++line;
        }
        if (*line) {
            *line = '\0';
            ++line;
        }
    }
    *count = n;
    return 0;
}

static inline int number(const char *s, uint64_t max, uint64_t *out) {
    if (!s || !*s || !out) {
        return 1;
    }
    uint64_t n = 0;
    while (*s) {
        unsigned digit = (unsigned char)*s - '0';
        if (digit > 9 || digit > max || n > (max - digit) / 10) {
            return 1;
        }
        n = n * 10 + digit;
        ++s;
    }
    *out = n;
    return 0;
}

static inline int real_number(const char *s, double *out) {
    if (!s || !*s || !out) {
        return 1;
    }
    const char *p = s;
    if (*p == '+' || *p == '-') {
        ++p;
    }
    int have_digit = 0;
    int nonzero = 0;
    while (isdigit((unsigned char)*p)) {
        have_digit = 1;
        if (*p != '0') {
            nonzero = 1;
        }
        ++p;
    }
    if (*p == '.') {
        ++p;
        while (isdigit((unsigned char)*p)) {
            have_digit = 1;
            if (*p != '0') {
                nonzero = 1;
            }
            ++p;
        }
    }
    if (!have_digit) {
        return 1;
    }
    if (*p == 'e' || *p == 'E') {
        ++p;
        if (*p == '+' || *p == '-') {
            ++p;
        }
        if (!isdigit((unsigned char)*p)) {
            return 1;
        }
        while (isdigit((unsigned char)*p)) {
            ++p;
        }
    }
    if (*p) {
        return 1;
    }
    char *end;
    double n = strtod(s, &end);
    if (*end || !isfinite(n) || (n == 0 && nonzero)) {
        return 1;
    }
    *out = n;
    return 0;
}

static inline int copy_text(const char *s, char **out) {
    if (!s || !out) {
        return 1;
    }
    size_t n = strlen(s);
    char *copy = malloc(n + 1);
    if (!copy) {
        return 2;
    }
    memcpy(copy, s, n + 1);
    *out = copy;
    return 0;
}

static inline int finish(int status) {
    if (fflush(stdout) == EOF && !status) {
        status = 3;
    }
    if (status) {
        fprintf(stderr, "Error (%d)\n", status);
    }
    return status;
}
#endif
