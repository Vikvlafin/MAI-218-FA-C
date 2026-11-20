#include "../common.h"

typedef struct {
    uint64_t id;
    int start;
    int end;
} Talk;

static int by_id(const void *a, const void *b) {
    const Talk *first = a;
    const Talk *second = b;
    if (first->id < second->id) {
        return -1;
    }
    if (first->id > second->id) {
        return 1;
    }
    return 0;
}
static int by_end(const void *a, const void *b) {
    const Talk *x = a, *y = b;
    return x->end - y->end;
}
static int minute(const char *s, int *out) {
    if (!s || !out || strlen(s) != 5 || s[2] != ':') {
        return 1;
    }
    for (size_t i = 0; i < 5; ++i) {
        if (i != 2 && (s[i] < '0' || s[i] > '9')) {
            return 1;
        }
    }
    int hours = (s[0] - '0') * 10 + (s[1] - '0');
    int minutes = (s[3] - '0') * 10 + (s[4] - '0');
    if (hours > 23 || minutes > 59) {
        return 1;
    }
    *out = hours * 60 + minutes;
    return 0;
}

int schedule(const Talk *talks, size_t n, size_t halls, size_t *result) {
    if (!talks || !result || !n || !halls || n > SIZE_MAX / sizeof(Talk)) {
        return 1;
    }
    Talk *sorted = malloc(n * sizeof(*sorted));
    if (!sorted) {
        return 2;
    }
    memcpy(sorted, talks, n * sizeof(*sorted));
    qsort(sorted, n, sizeof(*sorted), by_id);
    for (size_t i = 0; i < n; ++i) {
        Talk t = sorted[i];
        if (!t.id || t.start < 0 || t.end >= 1440 || t.start >= t.end ||
            (i && sorted[i - 1].id == t.id)) {
            free(sorted);
            return 1;
        }
    }
    qsort(sorted, n, sizeof(*sorted), by_end);
    size_t available[1440] = {0};
    size_t count = 0;
    available[0] = halls < n ? halls : n;
    for (size_t i = 0; i < n; ++i) {
        for (int free_since = sorted[i].start; free_since >= 0; --free_since) {
            if (available[free_since]) {
                --available[free_since];
                ++available[sorted[i].end];
                ++count;
                break;
            }
        }
    }
    free(sorted);
    *result = count;
    return 0;
}

int main(int argc, char **argv) {
    (void)argv;
    if (argc != 1) {
        return finish(1);
    }
    char *line = NULL, *part[3];
    size_t count = 0;
    uint64_t talk_count = 0;
    uint64_t hall_count = 0;
    int status = read_line(stdin, &line);
    if (!status && (fields(line, part, 3, &count) || count != 2)) {
        status = 1;
    }
    if (!status && (number(part[0], SIZE_MAX / sizeof(Talk), &talk_count) || !talk_count)) {
        status = 1;
    }
    if (!status && (number(part[1], SIZE_MAX, &hall_count) || !hall_count)) {
        status = 1;
    }
    free(line);
    Talk *talks = NULL;
    size_t capacity = 0;
    for (size_t i = 0; i < talk_count && !status; ++i) {
        Talk talk;
        line = NULL;
        status = read_line(stdin, &line);
        if (!status && (fields(line, part, 3, &count) || count != 3)) {
            status = 1;
        }
        if (!status) {
            status = number(part[0], UINT64_MAX, &talk.id);
        }
        if (!status) {
            status = minute(part[1], &talk.start);
        }
        if (!status) {
            status = minute(part[2], &talk.end);
        }
        free(line);
        if (!status && i == capacity) {
            size_t limit = (size_t)talk_count;
            size_t next = limit < 16 ? limit : 16;
            if (capacity) {
                next = capacity <= limit / 2 ? capacity * 2 : limit;
            }
            Talk *grown = realloc(talks, next * sizeof(*grown));
            if (!grown) {
                status = 2;
            } else {
                talks = grown;
                capacity = next;
            }
        }
        if (!status) {
            talks[i] = talk;
        }
    }
    if (!status) {
        int c;
        while ((c = fgetc(stdin)) != EOF && isspace((unsigned char)c)) {
        }
        if (c != EOF || ferror(stdin)) {
            status = 1;
        }
    }
    size_t result;
    if (!status) {
        status = schedule(talks, (size_t)talk_count, (size_t)hall_count, &result);
    }
    if (!status && printf("%zu\n", result) < 0) {
        status = 3;
    }
    free(talks);
    return finish(status);
}
