#define main application_main
#include "main.c"
#undef main
#include <assert.h>

int main(void) {
    long double roots[2];
    int count;
    assert(solve_quadratic(1, -3, 2, 1e-12L, roots, &count) == OK && count == 2);
    assert(fabsl(roots[0] - 2) < 1e-12L && fabsl(roots[1] - 1) < 1e-12L);
    assert(solve_quadratic(1, 2, 1, 1e-12L, roots, &count) == OK && count == 1 && roots[0] == -1);
    assert(solve_quadratic(1, 0, 1, 1e-12L, roots, &count) == OK && count == 0);
    assert(solve_quadratic(0, 2, -4, 1e-12L, roots, &count) == OK && count == 1 && roots[0] == 2);
    assert(solve_quadratic(0, 0, 0, 1e-12L, roots, &count) == OK && count == -1);
    assert(solve_quadratic(0, 0, 1, 1e-12L, roots, &count) == OK && count == 0);
    assert(solve_quadratic(1, 1e12L, 1, 1e-12L, roots, &count) == OK);
    assert(fabsl(roots[1] + 1e-12L) < 1e-20L);
    int result;
    assert(right_triangle(3, 5, 4, 1e-12L, &result) == OK && result);
    assert(right_triangle(3, 4, 5, 0.5L, &result) == OK && result);
    const long double triangle[3] = {0.6L, 0.8L, 1.0L};
    for (int i = 0; i < 3; ++i) for (int j = 0; j < 3; ++j) {
        if (i == j) continue;
        assert(right_triangle(triangle[i], triangle[j], triangle[3 - i - j], 0.5L, &result) == OK && result);
    }
    assert(right_triangle(1, 100, sqrtl(10001), 0.01L, &result) == OK && result);
    assert(right_triangle(2, 2, 3, 1e-12L, &result) == OK && !result);
    assert(right_triangle(-3, 4, 5, 1e-12L, &result) == INVALID);
    assert(is_multiple(LLONG_MIN, -1, &result) == OK && result);
    assert(is_multiple(10, 3, &result) == OK && !result);
    assert(is_multiple(0, 2, &result) == INVALID);
    struct solution solutions[6];
    int solution_count;
    const long double unique[3] = {1, -3, 2};
    const long double repeated[3] = {1, 1, 2};
    assert(solve_permutations(unique, 1e-12L, solutions, &solution_count) == OK && solution_count == 6);
    assert(solutions[0].count == 2 && fabsl(solutions[0].roots[0] - 2) < 1e-12L);
    assert(solve_permutations(repeated, 1e-12L, solutions, &solution_count) == OK && solution_count == 3);
    assert(solve_permutations(NULL, 1e-12L, solutions, &solution_count) == INVALID);
    puts("task_03: equation, triangle and divisibility checks passed");
    return 0;
}
