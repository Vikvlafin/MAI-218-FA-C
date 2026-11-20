#include "../common.h"
#include <inttypes.h>
#include <limits.h>

typedef struct Term {
    int64_t coefficient;
    unsigned degree;
    struct Term *next;
} Term;

static int clear(Term **p) {
    if (!p) {
        return 1;
    }
    while (*p) {
        Term *next = (*p)->next;
        free(*p);
        *p = next;
    }
    return 0;
}
static int sum(int64_t a, int64_t b, int subtract, int64_t *out) {
    if (!out) {
        return 1;
    }
    if (subtract) {
        if ((b > 0 && a < INT64_MIN + b) || (b < 0 && a > INT64_MAX + b)) {
            return 2;
        }
        *out = a - b;
    } else {
        if ((b > 0 && a > INT64_MAX - b) || (b < 0 && a < INT64_MIN - b)) {
            return 2;
        }
        *out = a + b;
    }
    return 0;
}
static int product(int64_t a, int64_t b, int64_t *out) {
    if (!out) {
        return 1;
    }
    if (a > 0 && b > 0 && a > INT64_MAX / b) {
        return 2;
    }
    if (a > 0 && b < 0 && b < INT64_MIN / a) {
        return 2;
    }
    if (a < 0 && b > 0 && a < INT64_MIN / b) {
        return 2;
    }
    if (a < 0 && b < 0 && a < INT64_MAX / b) {
        return 2;
    }
    *out = a * b;
    return 0;
}
static int update(Term **p, unsigned degree, int64_t c, int subtract) {
    if (!p) {
        return 1;
    }
    if (!c) {
        return 0;
    }
    while (*p && (*p)->degree > degree) {
        p = &(*p)->next;
    }
    int present = *p && (*p)->degree == degree;
    int64_t value;
    int status = sum(present ? (*p)->coefficient : 0, c, subtract, &value);
    if (status) {
        return status;
    }
    if (present && !value) {
        Term *next = (*p)->next;
        free(*p);
        *p = next;
    } else if (present) {
        (*p)->coefficient = value;
    } else {
        Term *term = malloc(sizeof(*term));
        if (!term) {
            return 2;
        }
        *term = (Term){value, degree, *p};
        *p = term;
    }
    return 0;
}
static int combine(const Term *a, const Term *b, int subtract, Term **out) {
    if (!out || *out) {
        return 1;
    }
    int status = 0;
    while ((a || b) && !status) {
        if (!b || (a && a->degree > b->degree)) {
            status = update(out, a->degree, a->coefficient, 0);
            a = a->next;
        } else if (!a || b->degree > a->degree) {
            status = update(out, b->degree, b->coefficient, subtract);
            b = b->next;
        } else {
            int64_t c;
            status = sum(a->coefficient, b->coefficient, subtract, &c);
            if (!status) {
                status = update(out, a->degree, c, 0);
            }
            a = a->next;
            b = b->next;
        }
        if (!status && *out) {
            out = &(*out)->next;
        }
    }
    return status;
}
static int multiply(const Term *a, const Term *b, Term **out) {
    if (!out || *out) {
        return 1;
    }
    for (; a; a = a->next) {
        for (const Term *p = b; p; p = p->next) {
            int64_t c;
            if (a->degree > UINT_MAX - p->degree) {
                return 2;
            }
            int status = product(a->coefficient, p->coefficient, &c);
            if (!status) {
                status = update(out, a->degree + p->degree, c, 0);
            }
            if (status) {
                return status;
            }
        }
    }
    return 0;
}
static int divide(const Term *a, const Term *b, Term **quotient, Term **remainder) {
    if (!b || !quotient || !remainder || *quotient || *remainder) {
        return 1;
    }
    int status = combine(a, NULL, 0, remainder);
    while (!status && *remainder && (*remainder)->degree >= b->degree) {
        int64_t lead = (*remainder)->coefficient, d = b->coefficient;
        if (lead == INT64_MIN && d == -1) {
            return 2;
        }
        if (lead % d) {
            return 1;
        }
        unsigned shift = (*remainder)->degree - b->degree;
        int64_t coefficient = lead / d;
        status = update(quotient, shift, coefficient, 0);
        for (const Term *p = b; p && !status; p = p->next) {
            int64_t c;
            status = product(coefficient, p->coefficient, &c);
            if (!status) {
                status = update(remainder, p->degree + shift, c, 1);
            }
        }
    }
    return status;
}
static int derivative(const Term *a, Term **out) {
    if (!out || *out) {
        return 1;
    }
    for (; a && a->degree; a = a->next) {
        int64_t c;
        int status = product(a->coefficient, a->degree, &c);
        if (!status) {
            status = update(out, a->degree - 1, c, 0);
        }
        if (status) {
            return status;
        }
    }
    return 0;
}
static int power_int(int64_t base, unsigned degree, int64_t *out) {
    if (!out) {
        return 1;
    }
    int64_t result = 1;
    while (degree) {
        if ((degree % 2 != 0) && product(result, base, &result)) {
            return 2;
        }
        degree /= 2;
        if (degree && product(base, base, &base)) {
            return 2;
        }
    }
    *out = result;
    return 0;
}
static int evaluate(const Term *a, int64_t x, int64_t *out) {
    if (!out) {
        return 1;
    }
    if (!a) {
        *out = 0;
        return 0;
    }
    int64_t value = a->coefficient, factor;
    unsigned degree = a->degree;
    for (a = a->next; a; a = a->next) {
        if (power_int(x, degree - a->degree, &factor)) {
            return 2;
        }
        if (product(value, factor, &value)) {
            return 2;
        }
        if (sum(value, a->coefficient, 0, &value)) {
            return 2;
        }
        degree = a->degree;
    }
    if (power_int(x, degree, &factor)) {
        return 2;
    }
    return product(value, factor, out);
}
static int power_poly(const Term *base, unsigned degree, Term **out) {
    if (!out || *out) {
        return 1;
    }
    Term *power = NULL;
    int status = update(out, 0, 1, 0);
    if (!status && degree) {
        status = combine(base, NULL, 0, &power);
    }
    while (degree && !status) {
        if (degree % 2 != 0) {
            Term *next = NULL;
            status = multiply(*out, power, &next);
            if (clear(out)) {
                status = 1;
            }
            *out = next;
        }
        degree /= 2;
        if (degree && !status) {
            Term *next = NULL;
            status = multiply(power, power, &next);
            if (clear(&power)) {
                status = 1;
            }
            power = next;
        }
    }
    if (clear(&power)) {
        return 1;
    }
    return status;
}
static int compose(const Term *a, const Term *b, Term **out) {
    if (!out || *out) {
        return 1;
    }
    for (; a; a = a->next) {
        Term *power = NULL;
        int status = power_poly(b, a->degree, &power);
        for (const Term *p = power; p && !status; p = p->next) {
            int64_t c;
            status = product(a->coefficient, p->coefficient, &c);
            if (!status) {
                status = update(out, p->degree, c, 0);
            }
        }
        if (clear(&power)) {
            return 1;
        }
        if (status) {
            return status;
        }
    }
    return 0;
}
static int spaces(const char **cursor) {
    if (!cursor || !*cursor) {
        return 1;
    }
    while (isspace((unsigned char)**cursor)) {
        ++*cursor;
    }
    return 0;
}
static int unsigned_token(const char **cursor, uint64_t limit, uint64_t *out) {
    if (!cursor || !*cursor || !out || !isdigit((unsigned char)**cursor)) {
        return 1;
    }
    uint64_t result = 0;
    while (isdigit((unsigned char)**cursor)) {
        unsigned digit = (unsigned)(**cursor - '0');
        ++*cursor;
        if (digit > limit || result > (limit - digit) / 10) {
            return 2;
        }
        result = result * 10 + digit;
    }
    *out = result;
    return 0;
}
static int signed_value(uint64_t magnitude, int negative, int64_t *out) {
    if (!out || magnitude > (uint64_t)INT64_MAX + (unsigned)negative) {
        return 1;
    }
    if (!negative) {
        *out = (int64_t)magnitude;
    } else if (magnitude == (uint64_t)INT64_MAX + 1) {
        *out = INT64_MIN;
    } else {
        *out = -(int64_t)magnitude;
    }
    return 0;
}
static int integer_token(const char **cursor, int64_t *out) {
    if (!out || spaces(cursor)) {
        return 1;
    }
    int negative = **cursor == '-';
    if (**cursor == '-' || **cursor == '+') {
        ++*cursor;
    }
    uint64_t magnitude;
    int status = unsigned_token(cursor, (uint64_t)INT64_MAX + (unsigned)negative, &magnitude);
    if (!status) {
        status = signed_value(magnitude, negative, out);
    }
    if (!status) {
        status = spaces(cursor);
    }
    return status;
}
static int polynomial(const char **cursor, Term **out) {
    if (!cursor || !*cursor || !out || *out || spaces(cursor)) {
        return 1;
    }
    for (;;) {
        int negative = **cursor == '-';
        if (**cursor == '-' || **cursor == '+') {
            ++*cursor;
        }
        if (spaces(cursor)) {
            return 1;
        }
        int has_number = isdigit((unsigned char)**cursor);
        uint64_t magnitude = 1;
        uint64_t degree = 0;
        int status = 0;
        if (has_number) {
            status = unsigned_token(cursor, (uint64_t)INT64_MAX + (unsigned)negative, &magnitude);
            if (status) {
                return status;
            }
        }
        if (spaces(cursor)) {
            return 1;
        }
        int star = **cursor == '*';
        if (star) {
            ++*cursor;
            if (spaces(cursor)) {
                return 1;
            }
        }
        if (**cursor == 'x') {
            if (star && !has_number) {
                return 1;
            }
            ++*cursor;
            degree = 1;
            if (spaces(cursor)) {
                return 1;
            }
            if (**cursor == '^') {
                ++*cursor;
                if (spaces(cursor)) {
                    return 1;
                }
                status = unsigned_token(cursor, UINT_MAX, &degree);
                if (status) {
                    return status;
                }
            }
        } else if (star || !has_number) {
            return 1;
        }
        int64_t coefficient;
        status = signed_value(magnitude, negative, &coefficient);
        if (!status) {
            status = update(out, (unsigned)degree, coefficient, 0);
        }
        if (status) {
            return status;
        }
        if (spaces(cursor)) {
            return 1;
        }
        if (**cursor != '+' && **cursor != '-') {
            return 0;
        }
    }
}
static int print_poly(const Term *p) {
    if (!p) {
        return puts("0") == EOF ? 3 : 0;
    }
    int first = 1;
    while (p) {
        int64_t c = p->coefficient;
        uint64_t magnitude = c < 0 ? 0 - (uint64_t)c : (uint64_t)c;
        if ((c < 0 || !first) && putchar(c < 0 ? '-' : '+') == EOF) {
            return 3;
        }
        if ((!p->degree || magnitude != 1) && printf("%" PRIu64, magnitude) < 0) {
            return 3;
        }
        if (p->degree && putchar('x') == EOF) {
            return 3;
        }
        if (p->degree > 1 && printf("^%u", p->degree) < 0) {
            return 3;
        }
        first = 0;
        p = p->next;
    }
    return putchar('\n') == EOF ? 3 : 0;
}
static int execute(const char *cursor, Term **accumulator) {
    if (!cursor || !accumulator || spaces(&cursor)) {
        return 1;
    }
    char command[5];
    size_t length = 0;
    while (isalpha((unsigned char)*cursor)) {
        if (length == sizeof(command) - 1) {
            return 1;
        }
        command[length] = *cursor;
        ++length;
        ++cursor;
    }
    command[length] = '\0';
    if (spaces(&cursor) || *cursor != '(') {
        return 1;
    }
    ++cursor;
    if (spaces(&cursor)) {
        return 1;
    }
    Term *first = NULL;
    Term *second = NULL;
    Term *result = NULL;
    Term *remainder = NULL;
    int differentiation = !strcmp(command, "Diff");
    int evaluation = !strcmp(command, "Eval");
    int has_first = *cursor != ')';
    int two = strchr(cursor, ',') != NULL;
    int64_t point = 0;
    int status = 0;
    if (evaluation && !two) {
        status = integer_token(&cursor, &point);
    } else if (has_first) {
        status = polynomial(&cursor, &first);
    }
    if (two && !status) {
        if (*cursor != ',') {
            status = 1;
        } else {
            ++cursor;
            status = evaluation ? integer_token(&cursor, &point) : polynomial(&cursor, &second);
        }
    }
    if (!status && *cursor != ')') {
        status = 1;
    }
    if (!status) {
        ++cursor;
        if (spaces(&cursor) || *cursor) {
            status = 1;
        }
    }
    if (!status && (two && differentiation)) {
        status = 1;
    }
    if (!status && !has_first && !differentiation) {
        status = 1;
    }
    const Term *left = two ? first : *accumulator;
    const Term *right = two ? second : first;
    if (!status && differentiation) {
        status = derivative(has_first ? first : *accumulator, &result);
    } else if (!status && evaluation) {
        int64_t value;
        status = evaluate(left, point, &value);
        if (!status && printf("%" PRId64 "\n", value) < 0) {
            status = 3;
        }
    } else if (!status && (!strcmp(command, "Add") || !strcmp(command, "Sub"))) {
        status = combine(left, right, !strcmp(command, "Sub"), &result);
    } else if (!status && !strcmp(command, "Mult")) {
        status = multiply(left, right, &result);
    } else if (!status && (!strcmp(command, "Div") || !strcmp(command, "Mod"))) {
        status = divide(left, right, &result, &remainder);
        if (!status && !strcmp(command, "Mod")) {
            if (clear(&result)) {
                status = 1;
            }
            result = remainder;
            remainder = NULL;
        }
    } else if (!status && !strcmp(command, "Cmps")) {
        status = compose(left, right, &result);
    } else if (!status) {
        status = 1;
    }
    if (!status && !evaluation) {
        status = print_poly(result);
        if (!status) {
            if (clear(accumulator)) {
                status = 1;
            }
            *accumulator = result;
            result = NULL;
        }
    }
    int first_status = clear(&first);
    int second_status = clear(&second);
    int result_status = clear(&result);
    int remainder_status = clear(&remainder);
    if (first_status || second_status || result_status || remainder_status) {
        status = 1;
    }
    return status;
}
static int process(FILE *in) {
    if (!in) {
        return 1;
    }
    Term *accumulator = NULL;
    size_t capacity = 128, n = 0;
    char *buf = malloc(capacity);
    if (!buf) {
        return 2;
    }
    int c;
    int status = 0;
    enum {
        OUTSIDE_COMMENT,
        LINE_COMMENT,
        BLOCK_COMMENT
    } comment = OUTSIDE_COMMENT;
    while (!status && (c = fgetc(in)) != EOF) {
        if (!c) {
            status = 1;
            break;
        }
        if (comment == LINE_COMMENT) {
            if (c != '\n') {
                continue;
            }
            comment = OUTSIDE_COMMENT;
        }
        if (comment == BLOCK_COMMENT) {
            if (c == '[') {
                status = 1;
                break;
            }
            if (c != ']') {
                continue;
            }
            comment = OUTSIDE_COMMENT;
            c = ' ';
        } else if (c == '%' || c == '[') {
            comment = c == '%' ? LINE_COMMENT : BLOCK_COMMENT;
            c = ' ';
        } else if (c == ']') {
            status = 1;
            break;
        }
        if (c == ';') {
            buf[n] = '\0';
            status = execute(buf, &accumulator);
            n = 0;
            continue;
        }
        if (!n && isspace((unsigned char)c)) {
            continue;
        }
        if (n == capacity - 1) {
            if (capacity > SIZE_MAX / 2) {
                status = 2;
                break;
            }
            char *next = realloc(buf, capacity * 2);
            if (!next) {
                status = 2;
                break;
            }
            buf = next;
            capacity *= 2;
        }
        buf[n] = (char)c;
        ++n;
    }
    if (!status && (comment == BLOCK_COMMENT || n)) {
        status = 1;
    }
    if (ferror(in)) {
        status = 3;
    }
    if (clear(&accumulator)) {
        status = 1;
    }
    free(buf);
    return status;
}

int main(int argc, char **argv) {
    if (argc != 2) {
        return finish(1);
    }
    FILE *in = fopen(argv[1], "r");
    if (!in) {
        return finish(3);
    }
    int status = process(in);
    if (fclose(in) == EOF) {
        status = 3;
    }
    return finish(status);
}
