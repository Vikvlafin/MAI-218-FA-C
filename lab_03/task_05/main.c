#define _POSIX_C_SOURCE 200809L
#include "../common.h"
#include "../date.h"
#include "../files.h"
#include <inttypes.h>

typedef struct {
    uint64_t id;
    char *name, *surname, *group;
    unsigned char *marks;
} Student;

static int clear(Student *s) {
    if (!s) {
        return 1;
    }
    free(s->name);
    free(s->surname);
    free(s->group);
    free(s->marks);
    *s = (Student){0};
    return 0;
}
static int parse_student(char *line, Student *s) {
    if (!line || !s) {
        return 1;
    }
    char *v[9];
    size_t n;
    if (fields(line, v, 9, &n) || n != 9) {
        return 1;
    }
    int status = number(v[0], UINT64_MAX, &s->id);
    if (status) {
        return status;
    }
    if (latin(v[1]) || latin(v[2])) {
        return 1;
    }
    unsigned char marks[5];
    for (size_t i = 0; i < 5; ++i) {
        uint64_t mark;
        if (number(v[4 + i], 255, &mark)) {
            return 1;
        }
        marks[i] = (unsigned char)mark;
    }
    status = copy_text(v[1], &s->name);
    if (!status) {
        status = copy_text(v[2], &s->surname);
    }
    if (!status) {
        status = copy_text(v[3], &s->group);
    }
    if (!status) {
        s->marks = malloc(5 * sizeof(*s->marks));
        if (!s->marks) {
            status = 2;
        } else {
            memcpy(s->marks, marks, sizeof(marks));
        }
    }
    return status;
}
static int by_id(const void *a, const void *b) {
    const Student *first = a;
    const Student *second = b;
    if (first->id < second->id) {
        return -1;
    }
    if (first->id > second->id) {
        return 1;
    }
    return 0;
}
static int by_name(const void *a, const void *b) {
    return strcmp(((const Student *)a)->name, ((const Student *)b)->name);
}
static int by_surname(const void *a, const void *b) {
    return strcmp(((const Student *)a)->surname, ((const Student *)b)->surname);
}
static int by_group(const void *a, const void *b) {
    return strcmp(((const Student *)a)->group, ((const Student *)b)->group);
}
static int sort(Student *a, size_t n, int (*cmp)(const void *, const void *)) {
    if ((!a && n) || !cmp) {
        return 1;
    }
    if (n > 1) {
        qsort(a, n, sizeof(*a), cmp);
    }
    return 0;
}
static int mark_sum(const Student *s, unsigned *out) {
    if (!s || !s->marks || !out) {
        return 1;
    }
    unsigned sum = 0;
    for (size_t i = 0; i < 5; ++i) {
        sum += s->marks[i];
    }
    *out = sum;
    return 0;
}
static int show(FILE *out, const Student *s) {
    unsigned sum;
    if (!out || mark_sum(s, &sum)) {
        return 1;
    }
    return fprintf(out, "%" PRIu64 " %s %s %s %.2f\n", s->id, s->name, s->surname, s->group,
                   sum / 5.0) < 0
               ? 3
               : 0;
}
static int mean_sum_floor(const Student *a, size_t n, unsigned *out) {
    if ((!a && n) || !out) {
        return 1;
    }
    uint64_t total = 0;
    for (size_t i = 0; i < n; ++i) {
        unsigned sum;
        if (mark_sum(a + i, &sum)) {
            return 1;
        }
        if (total > UINT64_MAX - sum) {
            return 2;
        }
        total += sum;
    }
    *out = n ? (unsigned)(total / n) : 0;
    return 0;
}
static int select_above(const Student *a, size_t n, const Student ***out, size_t *count) {
    if ((!a && n) || !out || !count || n > SIZE_MAX / sizeof(const Student *)) {
        return 1;
    }
    if (!n) {
        *out = NULL;
        *count = 0;
        return 0;
    }
    unsigned floor;
    int status = mean_sum_floor(a, n, &floor);
    if (status) {
        return status;
    }
    const Student **selected = malloc(n * sizeof(*selected));
    if (!selected) {
        return 2;
    }
    size_t used = 0;
    for (size_t i = 0; i < n && !status; ++i) {
        unsigned sum;
        status = mark_sum(a + i, &sum);
        if (!status && sum > floor) {
            selected[used] = &a[i];
            ++used;
        }
    }
    if (status) {
        free(selected);
    } else {
        *out = selected;
        *count = used;
    }
    return status;
}
static int dialog(Student *a, size_t n, FILE *trace) {
    if ((!a && n) || !trace) {
        return 1;
    }
    for (;;) {
        char *line = NULL, *v[3];
        size_t count = 0;
        int status = read_line(stdin, &line);
        if (status == 4) {
            return 0;
        }
        if (status) {
            return status;
        }
        status = fields(line, v, 3, &count);
        if (!status && count == 1 && !strcmp(v[0], "quit")) {
            free(line);
            return 0;
        }
        if (!status && count == 1 && !strcmp(v[0], "above")) {
            const Student **selected = NULL;
            size_t used = 0;
            status = select_above(a, n, &selected, &used);
            for (size_t i = 0; i < used && !status; ++i) {
                if (fprintf(trace, "%s %s\n", selected[i]->surname, selected[i]->name) < 0) {
                    status = 3;
                }
            }
            free(selected);
        } else if (!status && count == 1 && !strcmp(v[0], "list")) {
            for (size_t i = 0; i < n && !status; ++i) {
                status = show(stdout, a + i);
            }
        } else if (!status && count == 2 && !strcmp(v[0], "sort")) {
            int (*cmp)(const void *, const void *) = NULL;
            if (!strcmp(v[1], "id")) {
                cmp = by_id;
            } else if (!strcmp(v[1], "name")) {
                cmp = by_name;
            } else if (!strcmp(v[1], "surname")) {
                cmp = by_surname;
            } else if (!strcmp(v[1], "group")) {
                cmp = by_group;
            }
            status = sort(a, n, cmp);
        } else if (!status && ((count == 3 && !strcmp(v[0], "find")) ||
                               (count == 2 && !strcmp(v[0], "trace")))) {
            int write_trace = count == 2;
            const char *key = write_trace ? "id" : v[1];
            const char *value = v[count - 1];
            FILE *out = write_trace ? trace : stdout;
            int by_number = strcmp(key, "id") == 0;
            int by_first_name = strcmp(key, "name") == 0;
            int by_last_name = strcmp(key, "surname") == 0;
            uint64_t id = 0;
            if (by_number) {
                status = number(value, UINT64_MAX, &id);
            } else if (by_first_name || by_last_name) {
                status = latin(value);
            } else if (strcmp(key, "group") != 0) {
                status = 1;
            }
            for (size_t i = 0; i < n && !status; ++i) {
                const Student *student = &a[i];
                int match;
                if (by_number) {
                    match = student->id == id;
                } else if (by_first_name) {
                    match = strcmp(value, student->name) == 0;
                } else if (by_last_name) {
                    match = strcmp(value, student->surname) == 0;
                } else {
                    match = strcmp(value, student->group) == 0;
                }
                if (match) {
                    status = show(out, student);
                }
            }
        } else {
            status = 1;
        }
        free(line);
        if (status == 3) {
            return 3;
        }
        if (fflush(stdout) == EOF) {
            return 3;
        }
        if (fflush(trace) == EOF) {
            return 3;
        }
        if (status && fprintf(stderr, "Command error (%d)\n", status) < 0) {
            return 3;
        }
    }
}

int main(int argc, char **argv) {
    if (argc != 3) {
        return finish(1);
    }
    FILE *in = fopen(argv[1], "r"), *trace = NULL;
    if (!in) {
        return finish(3);
    }
    Student *a = NULL;
    size_t n = 0, capacity = 0;
    int status = 0;
    while (!status) {
        char *line = NULL;
        status = read_line(in, &line);
        if (status == 4) {
            status = 0;
            break;
        }
        if (status) {
            break;
        }
        Student s = {0};
        status = parse_student(line, &s);
        free(line);
        if (!status && n == capacity) {
            if (capacity > SIZE_MAX / 2 / sizeof(*a)) {
                status = 2;
            } else {
                size_t next_size = capacity ? capacity * 2 : 16;
                Student *next = realloc(a, next_size * sizeof(*a));
                if (!next) {
                    status = 2;
                } else {
                    a = next;
                    capacity = next_size;
                }
            }
        }
        if (!status) {
            a[n] = s;
            ++n;
        } else if (clear(&s)) {
            status = 1;
        }
    }
    if (!status) {
        status = open_distinct(in, argv[2], &trace);
    }
    if (!status) {
        status = dialog(a, n, trace);
    }
    if (trace && fclose(trace) == EOF) {
        status = 3;
    }
    if (fclose(in) == EOF) {
        status = 3;
    }
    for (size_t i = 0; i < n; ++i) {
        if (clear(a + i)) {
            status = 1;
        }
    }
    free(a);
    return finish(status);
}
