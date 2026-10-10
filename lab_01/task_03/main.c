#include <stdio.h>
#include <string.h>
#include "../common.h"

struct solution {
    long double coefficients[3];
    long double roots[2];
    int count;
};

int is_multiple(const long long a, const long long b, int *result) {
    if (a == 0 || b == 0 || result == NULL) return INVALID;
    *result = b == -1 ? 1 : a % b == 0;
    return OK;
}

int solve_quadratic(const long double a, const long double b, const long double c,
                    const long double epsilon, long double roots[2], int *count) {
    if (roots == NULL || count == NULL || !isfinite(a) || !isfinite(b) || !isfinite(c) ||
        !isfinite(epsilon) || epsilon <= 0) return INVALID;
    if (fabsl(a) <= epsilon) {
        if (fabsl(b) <= epsilon) *count = fabsl(c) <= epsilon ? -1 : 0;
        else { roots[0] = -c / b; *count = 1; }
    } else {
        const long double discriminant = b * b - 4 * a * c;
        if (!isfinite(discriminant)) return OVERFLOW;
        if (discriminant < -epsilon) *count = 0;
        else if (fabsl(discriminant) <= epsilon) { roots[0] = -b / (2 * a); *count = 1; }
        else {
            const long double q = -0.5L * (b + copysignl(sqrtl(discriminant), b));
            roots[0] = q / a;
            roots[1] = c / q;
            *count = 2;
        }
    }
    for (int i = 0; i < *count; ++i) if (!isfinite(roots[i])) return OVERFLOW;
    return OK;
}

int right_triangle(const long double a, const long double b, const long double c,
                   const long double epsilon, int *result) {
    if (result == NULL || !isfinite(a) || !isfinite(b) || !isfinite(c) || !isfinite(epsilon) ||
        epsilon <= 0 || a <= epsilon || b <= epsilon || c <= epsilon) return INVALID;
    long double sides[3] = {a, b, c};
    for (int i = 0; i < 2; ++i) for (int j = i + 1; j < 3; ++j) {
        if (sides[i] > sides[j]) {
            const long double temp = sides[i]; sides[i] = sides[j]; sides[j] = temp;
        }
    }
    const long double x = sides[0] / sides[2], y = sides[1] / sides[2];
    *result = x + y > 1 && fabsl(x * x + y * y - 1) <= epsilon;
    return OK;
}

int solve_permutations(const long double values[3], const long double epsilon,
                       struct solution solutions[6], int *count) {
    if (values == NULL || solutions == NULL || count == NULL || !isfinite(epsilon) || epsilon <= 0) return INVALID;
    const int orders[6][3] = {{0,1,2},{0,2,1},{1,0,2},{1,2,0},{2,0,1},{2,1,0}};
    *count = 0;
    for (int i = 0; i < 6; ++i) {
        long double current[3];
        for (int j = 0; j < 3; ++j) current[j] = values[orders[i][j]];
        int duplicate = 0;
        for (int k = 0; k < *count; ++k) {
            if (fabsl(current[0] - solutions[k].coefficients[0]) <= epsilon &&
                fabsl(current[1] - solutions[k].coefficients[1]) <= epsilon &&
                fabsl(current[2] - solutions[k].coefficients[2]) <= epsilon) duplicate = 1;
        }
        if (duplicate) continue;
        struct solution *solution = &solutions[*count];
        for (int j = 0; j < 3; ++j) solution->coefficients[j] = current[j];
        const int status = solve_quadratic(current[0], current[1], current[2], epsilon, solution->roots, &solution->count);
        if (status != OK) return status;
        ++*count;
    }
    return OK;
}

int print_solutions(const struct solution *solutions, const int count) {
    if (solutions == NULL || count < 1 || count > 6) return INVALID;
    for (int i = 0; i < count; ++i) {
        const struct solution *solution = &solutions[i];
        if (solution->count < -1 || solution->count > 2) return INVALID;
        for (int j = 0; j < 3; ++j) if (!isfinite(solution->coefficients[j])) return INVALID;
        for (int j = 0; j < solution->count; ++j) if (!isfinite(solution->roots[j])) return INVALID;
        printf("%.12Lg %.12Lg %.12Lg:", solution->coefficients[0], solution->coefficients[1], solution->coefficients[2]);
        if (solution->count == -1) printf(" all real numbers");
        else if (solution->count == 0) printf(" no real roots");
        else for (int j = 0; j < solution->count; ++j) printf(" %.12Lg", solution->roots[j]);
        putchar('\n');
        if (ferror(stdout)) return FILE_ERROR;
    }
    return OK;
}

int main(const int argc, char *argv[]) {
    if (argc < 2 || strlen(argv[1]) != 2 || (argv[1][0] != '-' && argv[1][0] != '/')) return INVALID;
    const char mode = argv[1][1];
    int status = INVALID;
    if (mode == 'm' && argc == 4) {
        long long a, b;
        int result;
        status = parse_integer(argv[2], 10, &a);
        if (status == OK) status = parse_integer(argv[3], 10, &b);
        if (status == OK) status = is_multiple(a, b, &result);
        if (status == OK) puts(result ? "Multiple" : "Not multiple");
    } else if ((mode == 'q' || mode == 't') && argc == 6) {
        long double epsilon, values[3];
        status = parse_real(argv[2], &epsilon);
        if (status == OK && epsilon <= 0) status = INVALID;
        for (int i = 0; status == OK && i < 3; ++i) status = parse_real(argv[i + 3], &values[i]);
        if (status == OK && mode == 'q') {
            struct solution solutions[6];
            int count;
            status = solve_permutations(values, epsilon, solutions, &count);
            if (status == OK) status = print_solutions(solutions, count);
        }
        else if (status == OK) {
            int result;
            status = right_triangle(values[0], values[1], values[2], epsilon, &result);
            if (status == OK) puts(result ? "Right triangle" : "Not right triangle");
        }
    }
    if (status != OK) fprintf(stderr, "Invalid arguments or arithmetic overflow (%d)\n", status);
    if (fflush(stdout) == EOF || ferror(stdout)) return FILE_ERROR;
    return status;
}
