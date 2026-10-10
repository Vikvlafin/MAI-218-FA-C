#define main application_main
#include "main.c"
#undef main
#include <assert.h>

int main(void) {
    int values[] = {3, -4, 9, 2, -4, 9};
    assert(swap_extremes(values, 6) == OK);
    assert(values[1] == 9 && values[2] == -4 && values[4] == -4 && values[5] == 9);
    int single[] = {7};
    assert(swap_extremes(single, 1) == OK && single[0] == 7);
    assert(swap_extremes(NULL, 1) == INVALID);
    assert(swap_extremes(values, 0) == INVALID);
    const int a[] = {-10, 0, 6, 10};
    const int b[] = {-8, 2, 9};
    int c[4];
    assert(make_sums(a, 4, b, 3, c) == OK);
    assert(c[0] == -18 && c[1] == 2 && c[2] == 15 && c[3] == 19);
    const int tie_a[] = {5};
    const int tie_b[] = {4, 6};
    assert(make_sums(tie_a, 1, tie_b, 2, c) == OK && (c[0] == 9 || c[0] == 11));
    const int large[] = {INT_MAX};
    assert(make_sums(large, 1, large, 1, c) == OVERFLOW);
    assert(make_sums(a, 4, b, 0, c) == INVALID);
    unsigned int state = 1;
    for (int i = 0; i < 10000; ++i) {
        int value;
        assert(random_integer(&state, -1000, 1000, &value) == OK);
        assert(value >= -1000 && value <= 1000);
    }
    assert(random_integer(&state, 2, 1, c) == INVALID);
    puts("task_09: deterministic function checks passed");
    return 0;
}
