#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include "../common.h"

int random_integer(unsigned int *state, const int low, const int high, int *value) {
    if (state == NULL || value == NULL || low > high) return INVALID;
    uint32_t next = (uint32_t)*state;
    if (next == 0) next = 1;
    next ^= next << 13;
    next ^= next >> 17;
    next ^= next << 5;
    *state = next;
    const uint64_t width = (uint64_t)((long long)high - low) + 1;
    *value = (int)((long long)low + (long long)(next % width));
    return OK;
}

int fill_array(int *values, const size_t count, const int low, const int high, unsigned int *state) {
    if (values == NULL || count == 0 || low > high || state == NULL) return INVALID;
    for (size_t i = 0; i < count; ++i) {
        const int status = random_integer(state, low, high, &values[i]);
        if (status != OK) return status;
    }
    return OK;
}

int swap_extremes(int *values, const size_t count) {
    if (values == NULL || count == 0) return INVALID;
    size_t smallest = 0, largest = 0;
    for (size_t i = 1; i < count; ++i) {
        if (values[i] < values[smallest]) smallest = i;
        if (values[i] > values[largest]) largest = i;
    }
    const int temp = values[smallest];
    values[smallest] = values[largest];
    values[largest] = temp;
    return OK;
}

int make_sums(const int *a, const size_t count_a, const int *b, const size_t count_b, int *c) {
    if (a == NULL || b == NULL || c == NULL || count_a == 0 || count_b == 0) return INVALID;
    for (size_t i = 0; i < count_a; ++i) {
        size_t nearest = 0;
        long long distance = llabs((long long)a[i] - b[0]);
        for (size_t j = 1; j < count_b; ++j) {
            const long long current = llabs((long long)a[i] - b[j]);
            if (current < distance) { nearest = j; distance = current; }
        }
        const long long sum = (long long)a[i] + b[nearest];
        if (sum < INT_MIN || sum > INT_MAX) return OVERFLOW;
        c[i] = (int)sum;
    }
    return OK;
}

int print_array(const char *name, const int *values, const size_t count) {
    if (name == NULL || values == NULL || count == 0) return INVALID;
    if (printf("%s:", name) < 0) return FILE_ERROR;
    for (size_t i = 0; i < count; ++i) if (printf(" %d", values[i]) < 0) return FILE_ERROR;
    return putchar('\n') == EOF ? FILE_ERROR : OK;
}

int main(const int argc, char *argv[]) {
    long long low, high;
    if (argc != 3 || parse_integer(argv[1], 10, &low) != OK || parse_integer(argv[2], 10, &high) != OK ||
        low < INT_MIN || high > INT_MAX || low > high) {
        fprintf(stderr, "Usage: task_09 a b (INT_MIN <= a <= b <= INT_MAX)\n");
        return INVALID;
    }
    unsigned int state = (unsigned int)time(NULL);
    int fixed[20];
    int status = fill_array(fixed, 20, (int)low, (int)high, &state);
    if (status == OK) status = print_array("before", fixed, 20);
    if (status == OK) status = swap_extremes(fixed, 20);
    if (status == OK) status = print_array("after", fixed, 20);
    int count_a, count_b;
    if (status == OK) status = random_integer(&state, 10, 10000, &count_a);
    if (status == OK) status = random_integer(&state, 10, 10000, &count_b);
    if (status != OK) return status;
    int *a = malloc((size_t)count_a * sizeof(*a));
    int *b = malloc((size_t)count_b * sizeof(*b));
    int *c = malloc((size_t)count_a * sizeof(*c));
    if (a == NULL || b == NULL || c == NULL) status = MEMORY_ERROR;
    if (status == OK) status = fill_array(a, (size_t)count_a, -1000, 1000, &state);
    if (status == OK) status = fill_array(b, (size_t)count_b, -1000, 1000, &state);
    if (status == OK) status = make_sums(a, (size_t)count_a, b, (size_t)count_b, c);
    if (status == OK) status = print_array("A", a, (size_t)count_a);
    if (status == OK) status = print_array("B", b, (size_t)count_b);
    if (status == OK) status = print_array("C", c, (size_t)count_a);
    free(a);
    free(b);
    free(c);
    if (status != OK) fprintf(stderr, "Array processing failed (%d)\n", status);
    if (fflush(stdout) == EOF || ferror(stdout)) return FILE_ERROR;
    return status;
}
