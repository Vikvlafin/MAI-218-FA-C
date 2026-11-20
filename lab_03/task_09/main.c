#define _POSIX_C_SOURCE 200809L
#include "../common.h"
#include "../files.h"

typedef struct Node {
    char *word;
    size_t count, depth;
    struct Node *left, *right, *parent;
} Node;
typedef struct {
    const Node *shortest, *longest;
    size_t count, depth;
} Summary;

static int insert(Node **root, const char *word, size_t count, Node **inserted) {
    if (!root || !word || !*word || !count) {
        return 1;
    }
    Node *parent = NULL;
    size_t depth = 1;
    while (*root) {
        parent = *root;
        int cmp = strcmp(word, parent->word);
        if (!cmp) {
            if (inserted || parent->count > SIZE_MAX - count) {
                return 1;
            }
            parent->count += count;
            return 0;
        }
        root = cmp < 0 ? &parent->left : &parent->right;
        ++depth;
    }
    Node *node = calloc(1, sizeof(*node));
    if (!node) {
        return 2;
    }
    int status = copy_text(word, &node->word);
    if (status) {
        free(node);
        return status;
    }
    node->count = count;
    node->parent = parent;
    node->depth = depth;
    *root = node;
    if (inserted) {
        *inserted = node;
    }
    return 0;
}

static int destroy(Node **root) {
    if (!root) {
        return 1;
    }
    Node *node = *root;
    while (node) {
        if (node->left) {
            node = node->left;
            continue;
        }
        if (node->right) {
            node = node->right;
            continue;
        }
        Node *parent = node->parent;
        if (parent) {
            if (parent->left == node) {
                parent->left = NULL;
            } else {
                parent->right = NULL;
            }
        }
        free(node->word);
        free(node);
        node = parent;
    }
    *root = NULL;
    return 0;
}

static int next_preorder(const Node *node, const Node **out) {
    if (!node || !out) {
        return 1;
    }
    if (node->left) {
        *out = node->left;
    } else if (node->right) {
        *out = node->right;
    } else {
        while (node->parent && (node == node->parent->right || !node->parent->right)) {
            node = node->parent;
        }
        *out = node->parent ? node->parent->right : NULL;
    }
    return 0;
}

static int read_words(FILE *in, const unsigned char sep[256], Node **root) {
    if (!in || !sep || !root || *root) {
        return 1;
    }
    size_t capacity = 64, n = 0;
    char *word = malloc(capacity);
    if (!word) {
        return 2;
    }
    int c, status = 0;
    do {
        c = fgetc(in);
        if (c == EOF || sep[c]) {
            if (n) {
                word[n] = '\0';
                status = insert(root, word, 1, NULL);
                n = 0;
            }
        } else if (!c) {
            status = 1;
        } else {
            if (n == capacity - 1) {
                if (capacity > SIZE_MAX / 2) {
                    status = 2;
                    break;
                }
                char *next = realloc(word, capacity * 2);
                if (!next) {
                    status = 2;
                    break;
                }
                word = next;
                capacity *= 2;
            }
            word[n] = (char)c;
            ++n;
        }
    } while (!status && c != EOF);
    if (ferror(in)) {
        status = 3;
    }
    free(word);
    return status;
}

static int save(FILE *out, const Node *node) {
    if (!out) {
        return 1;
    }
    while (node) {
        size_t length = strlen(node->word);
        if (fprintf(out, "%zu %zu\n", node->count, length) < 0) {
            return 3;
        }
        if (fwrite(node->word, 1, length, out) != length) {
            return 3;
        }
        if (fputc('\n', out) == EOF) {
            return 3;
        }
        if (next_preorder(node, &node)) {
            return 1;
        }
    }
    return 0;
}

static int read_key(FILE *in, const unsigned char sep[256], size_t length, char **out) {
    if (!in || !sep || !length || length == SIZE_MAX || !out) {
        return 1;
    }
    size_t limit = length + 1;
    size_t capacity = limit < 64 ? limit : 64;
    char *word = malloc(capacity);
    if (!word) {
        return 2;
    }
    int status = 0;
    for (size_t i = 0; i < length; ++i) {
        int c = fgetc(in);
        if (c == EOF || !c || sep[c]) {
            status = 1;
            break;
        }
        if (i == capacity - 1) {
            size_t next = capacity <= limit / 2 ? capacity * 2 : limit;
            char *grown = realloc(word, next);
            if (!grown) {
                status = 2;
                break;
            }
            word = grown;
            capacity = next;
        }
        word[i] = (char)c;
    }
    if (!status && fgetc(in) != '\n') {
        status = 1;
    }
    if (ferror(in)) {
        status = 3;
    }
    if (status) {
        free(word);
    } else {
        word[length] = '\0';
        *out = word;
    }
    return status;
}

static int load(FILE *in, const unsigned char sep[256], Node **root) {
    if (!in || !sep || !root || *root) {
        return 1;
    }
    Node *tree = NULL;
    const Node *previous = NULL;
    int status = 0;
    while (!status) {
        char *line = NULL, *part[2];
        size_t n;
        uint64_t count = 0, length = 0;
        status = read_line(in, &line);
        if (status == 4) {
            status = 0;
            break;
        }
        if (!status && (fields(line, part, 2, &n) || n != 2)) {
            status = 1;
        }
        if (!status && (number(part[0], SIZE_MAX, &count) || !count)) {
            status = 1;
        }
        if (!status && (number(part[1], SIZE_MAX - 1, &length) || !length)) {
            status = 1;
        }
        free(line);
        if (status) {
            break;
        }
        char *word = NULL;
        status = read_key(in, sep, (size_t)length, &word);
        if (!status) {
            Node *inserted = NULL;
            status = insert(&tree, word, (size_t)count, &inserted);
            if (!status && previous) {
                const Node *next = NULL;
                if (next_preorder(previous, &next) || next != inserted) {
                    status = 1;
                }
            }
            if (!status) {
                previous = inserted;
            }
        }
        free(word);
    }
    if (ferror(in)) {
        status = 3;
    }
    if (status) {
        if (destroy(&tree)) {
            return 1;
        }
    } else {
        *root = tree;
    }
    return status;
}

static int by_frequency(const void *a, const void *b) {
    const Node *x = *(const Node *const *)a, *y = *(const Node *const *)b;
    if (x->count != y->count) {
        return x->count > y->count ? -1 : 1;
    }
    return strcmp(x->word, y->word);
}

static int measure(const Node *root, Summary *result) {
    if (!result) {
        return 1;
    }
    Summary s = {root, root, 0, 0};
    const Node *node = root;
    while (node) {
        ++s.count;
        if (node->depth > s.depth) {
            s.depth = node->depth;
        }
        if (strlen(node->word) < strlen(s.shortest->word)) {
            s.shortest = node;
        }
        if (strlen(node->word) > strlen(s.longest->word)) {
            s.longest = node;
        }
        if (next_preorder(node, &node)) {
            return 1;
        }
    }
    *result = s;
    return 0;
}

static int rank(const Node *root, size_t count, const Node ***out) {
    if (!out || (!root && count)) {
        return 1;
    }
    if (!count) {
        *out = NULL;
        return 0;
    }
    const Node *node;
    if (count > SIZE_MAX / sizeof(node)) {
        return 2;
    }
    const Node **array = malloc(count * sizeof(*array));
    if (!array) {
        return 2;
    }
    size_t i = 0;
    int status = 0;
    for (node = root; node && i < count && !status;) {
        array[i] = node;
        ++i;
        status = next_preorder(node, &node);
    }
    if (i != count || node) {
        status = 1;
    }
    if (!status) {
        qsort(array, count, sizeof(*array), by_frequency);
    }
    if (status) {
        free(array);
    } else {
        *out = array;
    }
    return status;
}

static int statistics(const Node *root, const char *command, size_t limit) {
    if (!command) {
        return 1;
    }
    Summary s;
    int status = measure(root, &s);
    if (status) {
        return status;
    }
    if (!strcmp(command, "depth")) {
        return printf("%zu\n", s.depth) < 0 ? 3 : 0;
    }
    if (!strcmp(command, "shortest") || !strcmp(command, "longest")) {
        const Node *node = *command == 's' ? s.shortest : s.longest;
        const char *word = node ? node->word : "EMPTY";
        return puts(word) == EOF ? 3 : 0;
    }
    if (!strcmp(command, "list")) {
        limit = s.count;
    } else if (strcmp(command, "top")) {
        return 1;
    }
    const Node **array = NULL;
    status = rank(root, s.count, &array);
    for (size_t i = 0; i < s.count && i < limit && !status; ++i) {
        if (printf("%s %zu\n", array[i]->word, array[i]->count) < 0) {
            status = 3;
        }
    }
    free(array);
    return status;
}

static int frequency(const Node *node, const char *word, size_t *out) {
    if (!word || !*word || !out) {
        return 1;
    }
    while (node) {
        int cmp = strcmp(word, node->word);
        if (!cmp) {
            break;
        }
        node = cmp < 0 ? node->left : node->right;
    }
    *out = node ? node->count : 0;
    return 0;
}

static int hex_digit(char c, unsigned *out) {
    if (!out) {
        return 1;
    }
    if (c >= '0' && c <= '9') {
        *out = (unsigned)(c - '0');
    } else if (c >= 'a' && c <= 'f') {
        *out = (unsigned)(c - 'a' + 10);
    } else if (c >= 'A' && c <= 'F') {
        *out = (unsigned)(c - 'A' + 10);
    } else {
        return 1;
    }
    return 0;
}

static int decode_word(char *s) {
    if (!s || !*s) {
        return 1;
    }
    char *out = s;
    while (*s) {
        unsigned high;
        unsigned low;
        if (hex_digit(s[0], &high)) {
            return 1;
        }
        if (!s[1]) {
            return 1;
        }
        if (hex_digit(s[1], &low)) {
            return 1;
        }
        unsigned value = high * 16 + low;
        if (!value) {
            return 1;
        }
        *out = (char)(unsigned char)value;
        ++out;
        s += 2;
    }
    *out = '\0';
    return 0;
}

static int dialog(FILE *input, const unsigned char sep[256], Node **root) {
    if (!input || !sep || !root) {
        return 1;
    }
    for (;;) {
        char *line = NULL, *arg[2];
        size_t n = 0;
        int status = read_line(stdin, &line);
        if (status == 4) {
            return 0;
        }
        if (status) {
            return status;
        }
        if (!strncmp(line, "find ", 5) && line[5]) {
            size_t count;
            status = frequency(*root, line + 5, &count);
            free(line);
            if (status) {
                return status;
            }
            if (printf("%zu\n", count) < 0) {
                return 3;
            }
            if (fflush(stdout) == EOF) {
                return 3;
            }
            continue;
        }
        status = fields(line, arg, 2, &n);
        if (!status && n == 1 && !strcmp(arg[0], "quit")) {
            free(line);
            return 0;
        }
        if (!status && n == 2 && !strcmp(arg[0], "find-hex")) {
            size_t count;
            status = decode_word(arg[1]);
            if (!status) {
                status = frequency(*root, arg[1], &count);
            }
            if (!status && printf("%zu\n", count) < 0) {
                status = 3;
            }
        } else if (!status && n == 2 && !strcmp(arg[0], "top")) {
            uint64_t k;
            status = number(arg[1], SIZE_MAX, &k);
            if (!status && k) {
                status = statistics(*root, "top", (size_t)k);
            } else {
                status = 1;
            }
        } else if (!status && n == 1) {
            status = statistics(*root, arg[0], 0);
        } else if (!status && n == 2 && !strcmp(arg[0], "save")) {
            FILE *out = NULL;
            status = open_distinct(input, arg[1], &out);
            if (!status) {
                status = save(out, *root);
            }
            if (out && fclose(out) == EOF) {
                status = 3;
            }
        } else if (!status && n == 2 && !strcmp(arg[0], "load")) {
            FILE *in = fopen(arg[1], "rb");
            if (!in) {
                status = 3;
            } else {
                Node *candidate = NULL;
                status = load(in, sep, &candidate);
                if (fclose(in) == EOF) {
                    status = 3;
                }
                if (!status) {
                    status = destroy(root);
                }
                if (!status) {
                    *root = candidate;
                } else if (destroy(&candidate)) {
                    status = 1;
                }
            }
        } else {
            status = 1;
        }
        free(line);
        if (status && fprintf(stderr, "Command error (%d)\n", status) < 0) {
            return 3;
        }
        if (ferror(stdout) || fflush(stdout) == EOF) {
            return 3;
        }
    }
}

int main(int argc, char **argv) {
    if (argc < 3) {
        return finish(1);
    }
    unsigned char separators[256] = {0};
    for (int i = 2; i < argc; ++i) {
        if (strlen(argv[i]) != 1) {
            return finish(1);
        }
        separators[(unsigned char)argv[i][0]] = 1;
    }
    FILE *in = fopen(argv[1], "rb");
    if (!in) {
        return finish(3);
    }
    Node *root = NULL;
    int status = read_words(in, separators, &root);
    if (!status) {
        status = dialog(in, separators, &root);
    }
    if (destroy(&root)) {
        status = 1;
    }
    if (fclose(in) == EOF) {
        status = 3;
    }
    return finish(status);
}
