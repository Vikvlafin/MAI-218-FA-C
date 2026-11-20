#define _POSIX_C_SOURCE 200809L
#include "../common.h"
#include "../files.h"

typedef struct {
    const char *label;
    size_t length;
    size_t depth;
    size_t parent;
} Node;
typedef struct {
    Node *nodes;
    size_t count;
    size_t capacity;
} Tree;

static int append(Tree *tree, Node node) {
    if (!tree || !node.label || !node.length) {
        return 1;
    }
    if (tree->count == tree->capacity) {
        if (tree->capacity > SIZE_MAX / 2 / sizeof(Node)) {
            return 2;
        }
        size_t capacity = tree->capacity ? tree->capacity * 2 : 16;
        Node *next = realloc(tree->nodes, capacity * sizeof(*next));
        if (!next) {
            return 2;
        }
        tree->nodes = next;
        tree->capacity = capacity;
    }
    tree->nodes[tree->count] = node;
    ++tree->count;
    return 0;
}

static int parse(const char *cursor, Tree *tree) {
    if (!cursor || !tree) {
        return 1;
    }
    size_t parent = SIZE_MAX;
    size_t depth = 0;
    int expect_node = 1;
    int can_open = 0;
    while (*cursor) {
        if (isspace((unsigned char)*cursor)) {
            ++cursor;
            continue;
        }
        if (expect_node) {
            const char *start = cursor;
            while (*cursor && *cursor != '(' && *cursor != ')' && *cursor != ',' &&
                   !isspace((unsigned char)*cursor)) {
                ++cursor;
            }
            if (cursor == start) {
                return 1;
            }
            int status = append(tree, (Node){start, (size_t)(cursor - start), depth, parent});
            if (status) {
                return status;
            }
            expect_node = 0;
            can_open = 1;
        } else if (*cursor == '(' && can_open) {
            parent = tree->count - 1;
            ++depth;
            expect_node = 1;
            ++cursor;
        } else if (*cursor == ',' && parent != SIZE_MAX) {
            expect_node = 1;
            ++cursor;
        } else if (*cursor == ')' && parent != SIZE_MAX) {
            parent = tree->nodes[parent].parent;
            --depth;
            can_open = 0;
            ++cursor;
        } else {
            return 1;
        }
    }
    return expect_node || parent != SIZE_MAX;
}

static int print_tree(FILE *out, const Tree *tree) {
    if (!out || !tree || !tree->count) {
        return 1;
    }
    for (size_t i = 0; i < tree->count; ++i) {
        Node node = tree->nodes[i];
        for (size_t j = 0; j < node.depth; ++j) {
            if (fputs("  ", out) == EOF) {
                return 3;
            }
        }
        if (fwrite(node.label, 1, node.length, out) != node.length) {
            return 3;
        }
        if (fputc('\n', out) == EOF) {
            return 3;
        }
    }
    return fputc('\n', out) == EOF ? 3 : 0;
}

int main(int argc, char **argv) {
    if (argc != 3) {
        return finish(1);
    }
    FILE *in = fopen(argv[1], "r"), *out = NULL;
    if (!in) {
        return finish(3);
    }
    int status = open_distinct(in, argv[2], &out);
    while (!status) {
        char *line = NULL;
        status = read_line(in, &line);
        if (status == 4) {
            status = 0;
            break;
        }
        Tree tree = {0};
        if (!status) {
            status = parse(line, &tree);
        }
        if (!status) {
            status = print_tree(out, &tree);
        }
        free(tree.nodes);
        free(line);
    }
    if (out && fclose(out) == EOF) {
        status = 3;
    }
    if (fclose(in) == EOF) {
        status = 3;
    }
    return finish(status);
}
