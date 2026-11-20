#include "../common.h"

static int next_index(size_t index, size_t *out) {
    if (!out) {
        return 1;
    }
    size_t carry = 1;
    while (carry) {
        size_t next = (index & carry) << 1;
        index ^= carry;
        carry = next;
    }
    *out = index;
    return 0;
}

static int digits(uint64_t value, unsigned r, uint64_t mask, char *out, size_t *index) {
    if (!out || !index || r < 1 || r > 5) {
        return 1;
    }
    uint64_t high = value >> r;
    if (high) {
        int status = digits(high, r, mask, out, index);
        if (status) {
            return status;
        }
    }
    out[*index] = "0123456789ABCDEFGHIJKLMNOPQRSTUV"[value & mask];
    return next_index(*index, index);
}

int convert(uint64_t value, unsigned r, int negative, char out[66]) {
    if (!out || r < 1 || r > 5 || (negative != 0 && negative != 1)) {
        return 1;
    }
    size_t index = 0;
    if (negative && value) {
        out[0] = '-';
        index = 1;
    }
    uint64_t mask = (UINT64_MAX << r) ^ UINT64_MAX;
    int status = digits(value, r, mask, out, &index);
    if (!status) {
        out[index] = '\0';
    }
    return status;
}

int main(int argc, char **argv) {
    if (argc != 3) {
        return finish(1);
    }
    int negative = argv[1][0] == '-';
    uint64_t value;
    uint64_t r;
    const char *magnitude = argv[1];
    if (negative || magnitude[0] == '+') {
        ++magnitude;
    }
    if (number(magnitude, UINT64_MAX, &value)) {
        return finish(1);
    }
    if (number(argv[2], 5, &r)) {
        return finish(1);
    }
    char out[66];
    int status = convert(value, (unsigned)r, negative, out);
    if (!status && puts(out) == EOF) {
        status = 3;
    }
    return finish(status);
}
