#include "../common.h"
#include "../date.h"

typedef struct {
    char *vehicle;
    const char *station_path;
    double x, y;
    uint64_t arrival, departure;
    char marker;
} Record;
typedef struct Stop {
    double x, y;
    uint64_t arrival, departure;
    struct Stop *next;
} Stop;
typedef struct Route {
    char *vehicle;
    Stop *stops;
    struct Route *next;
} Route;

static int read_station(FILE *in, const char *station_path, Record **rows, size_t *n, size_t *cap) {
    if (!in || !station_path || !*station_path || !rows || !n || !cap) {
        return 1;
    }
    char *line = NULL, *v[6];
    size_t count = 0;
    double x = 0, y = 0;
    int status = read_line(in, &line);
    if (!status && (fields(line, v, 6, &count) || count != 2)) {
        status = 1;
    }
    if (!status) {
        status = real_number(v[0], &x);
    }
    if (!status) {
        status = real_number(v[1], &y);
    }
    free(line);
    while (!status) {
        line = NULL;
        status = read_line(in, &line);
        if (status == 4) {
            return 0;
        }
        if (status) {
            break;
        }
        Record r = {NULL, station_path, x, y, 0, 0, 0};
        if (fields(line, v, 6, &count) || count != 6) {
            status = 1;
        }
        if (!status) {
            status = timestamp(v[1], v[2], '.', &r.arrival);
        }
        if (!status) {
            status = timestamp(v[3], v[4], '.', &r.departure);
        }
        if (!status && (r.departure < r.arrival || strlen(v[5]) != 1 || !strchr("SIE", v[5][0]))) {
            status = 1;
        }
        if (!status && *n == *cap) {
            if (*cap > SIZE_MAX / 2 / sizeof(r)) {
                status = 2;
            } else {
                size_t next_size = *cap ? *cap * 2 : 32;
                Record *next = realloc(*rows, next_size * sizeof(r));
                if (!next) {
                    status = 2;
                } else {
                    *rows = next;
                    *cap = next_size;
                }
            }
        }
        if (!status) {
            status = copy_text(v[0], &r.vehicle);
        }
        if (!status) {
            r.marker = v[5][0];
            (*rows)[*n] = r;
            ++*n;
        }
        free(line);
    }
    return status;
}

static int by_time(const void *a, const void *b) {
    const Record *x = a, *y = b;
    int name = strcmp(x->vehicle, y->vehicle);
    if (name) {
        return name;
    }
    if (x->arrival != y->arrival) {
        return x->arrival < y->arrival ? -1 : 1;
    }
    if (x->departure != y->departure) {
        return x->departure < y->departure ? -1 : 1;
    }
    if (x->marker != y->marker) {
        const char *order = "SIE";
        const char *first_marker = strchr(order, x->marker);
        const char *second_marker = strchr(order, y->marker);
        return first_marker < second_marker ? -1 : 1;
    }
    return strcmp(x->station_path, y->station_path);
}

static int build(Record *rows, size_t n, Route **out) {
    if ((!rows && n) || !out || *out) {
        return 1;
    }
    if (n > 1) {
        qsort(rows, n, sizeof(*rows), by_time);
    }
    Route **tail = out;
    Stop **stop_tail = NULL;
    for (size_t i = 0; i < n; ++i) {
        int needs_start = stop_tail == NULL;
        int starts_route = rows[i].marker == 'S';
        if (starts_route != needs_start) {
            for (size_t j = i + 1; j < n; ++j) {
                if (strcmp(rows[i].vehicle, rows[j].vehicle) ||
                    rows[i].arrival != rows[j].arrival || rows[i].departure != rows[j].departure) {
                    break;
                }
                int next_starts_route = rows[j].marker == 'S';
                if (next_starts_route == needs_start) {
                    Record swap = rows[i];
                    rows[i] = rows[j];
                    rows[j] = swap;
                    break;
                }
            }
        }
        const Record *r = rows + i;
        int same = i && !strcmp(rows[i - 1].vehicle, r->vehicle);
        if ((!same && stop_tail) || (same && rows[i - 1].departure > r->arrival)) {
            return 1;
        }
        if (r->marker == 'S') {
            if (stop_tail) {
                return 1;
            }
            Route *route = calloc(1, sizeof(*route));
            if (!route) {
                return 2;
            }
            int status = copy_text(r->vehicle, &route->vehicle);
            if (status) {
                free(route);
                return status;
            }
            *tail = route;
            tail = &route->next;
            stop_tail = &route->stops;
        } else if (!same || !stop_tail) {
            return 1;
        }
        Stop *stop = malloc(sizeof(*stop));
        if (!stop) {
            return 2;
        }
        *stop = (Stop){r->x, r->y, r->arrival, r->departure, NULL};
        *stop_tail = stop;
        stop_tail = &stop->next;
        if (r->marker == 'E') {
            stop_tail = NULL;
        }
    }
    return stop_tail ? 1 : 0;
}

static int destroy(Route *route) {
    while (route) {
        Route *next = route->next;
        Stop *stop = route->stops;
        while (stop) {
            Stop *after = stop->next;
            free(stop);
            stop = after;
        }
        free(route->vehicle);
        free(route);
        route = next;
    }
    return 0;
}

typedef enum {
    METRIC_INVALID = -1,
    METRIC_ROUTE_COUNT,
    METRIC_PATH,
    METRIC_ROUTE_LENGTH,
    METRIC_STOP,
    METRIC_IDLE
} Metric;

static int query(const Route *route, Metric metric, int maximum, long double epsilon,
                 const char **vehicle, long double *value) {
    if (metric < METRIC_ROUTE_COUNT || metric > METRIC_IDLE || !vehicle || !value || epsilon <= 0) {
        return 1;
    }
    *vehicle = NULL;
    *value = 0;
    while (route) {
        const char *name = route->vehicle;
        long double total = 0;
        long double path = 0;
        long double route_best = 0;
        long double stop_best = 0;
        size_t count = 0;
        int have_stop = 0;
        while (route && !strcmp(route->vehicle, name)) {
            long double length = 0;
            ++count;
            for (const Stop *s = route->stops; s; s = s->next) {
                long double duration = s->departure - s->arrival;
                if (!have_stop || (maximum ? duration > stop_best : duration < stop_best)) {
                    stop_best = duration;
                }
                have_stop = 1;
                total += duration;
                if (s->next) {
                    length +=
                        hypotl((long double)s->x - s->next->x, (long double)s->y - s->next->y);
                }
            }
            if (count == 1 || (maximum ? length - route_best : route_best - length) > epsilon) {
                route_best = length;
            }
            path += length;
            route = route->next;
        }
        long double candidate = total;
        if (metric == METRIC_ROUTE_COUNT) {
            candidate = (long double)count;
        } else if (metric == METRIC_PATH) {
            candidate = path;
        } else if (metric == METRIC_ROUTE_LENGTH) {
            candidate = route_best;
        } else if (metric == METRIC_STOP) {
            candidate = stop_best;
        }
        if (!isfinite(candidate)) {
            return 2;
        }
        if (!*vehicle || (maximum ? candidate - *value : *value - candidate) > epsilon) {
            *value = candidate;
            *vehicle = name;
        }
    }
    return 0;
}

static int dialog(const Route *routes) {
    for (;;) {
        char *line = NULL, *v[2];
        size_t n;
        int status = read_line(stdin, &line);
        if (status == 4) {
            return 0;
        }
        if (status) {
            return status;
        }
        status = fields(line, v, 2, &n);
        if (!status && n == 1 && !strcmp(v[0], "quit")) {
            free(line);
            return 0;
        }
        if (!status && n == 2 && (!strcmp(v[1], "max") || !strcmp(v[1], "min"))) {
            Metric metric = METRIC_INVALID;
            if (!strcmp(v[0], "routes")) {
                metric = METRIC_ROUTE_COUNT;
            } else if (!strcmp(v[0], "path")) {
                metric = METRIC_PATH;
            } else if (!strcmp(v[0], "route")) {
                metric = METRIC_ROUTE_LENGTH;
            } else if (!strcmp(v[0], "stop")) {
                metric = METRIC_STOP;
            } else if (!strcmp(v[0], "idle") && !strcmp(v[1], "max")) {
                metric = METRIC_IDLE;
            }
            const char *vehicle = NULL;
            long double value;
            status = query(routes, metric, !strcmp(v[1], "max"), 1e-9L, &vehicle, &value);
            if (!status) {
                int printed = vehicle ? printf("%s %.12Lg\n", vehicle, value) : puts("EMPTY");
                if (printed < 0) {
                    status = 3;
                }
            }
        } else {
            status = 1;
        }
        free(line);
        if (status == 3 || fflush(stdout) == EOF) {
            return 3;
        }
        if (status && fprintf(stderr, "Command error (%d)\n", status) < 0) {
            return 3;
        }
    }
}

int main(int argc, char **argv) {
    if (argc < 2) {
        return finish(1);
    }
    Record *rows = NULL;
    size_t count = 0, capacity = 0;
    int status = 0;
    for (int i = 1; i < argc && !status; ++i) {
        FILE *in = fopen(argv[i], "r");
        if (!in) {
            status = 3;
        } else {
            status = read_station(in, argv[i], &rows, &count, &capacity);
            if (fclose(in) == EOF) {
                status = 3;
            }
        }
    }
    Route *routes = NULL;
    if (!status) {
        status = build(rows, count, &routes);
    }
    for (size_t i = 0; i < count; ++i) {
        free(rows[i].vehicle);
    }
    free(rows);
    if (!status) {
        status = dialog(routes);
    }
    if (destroy(routes)) {
        status = 1;
    }
    return finish(status);
}
