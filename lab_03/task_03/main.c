#include "../common.h"
#include <inttypes.h>

enum {
    BYTE_VALUES = 256,
    NODE_LIMIT = 2 * BYTE_VALUES - 1
};

typedef struct {
    uint64_t weight;
    int left;
    int right;
    int byte;
} Node;
typedef struct {
    int index[BYTE_VALUES];
    size_t size;
} Heap;

static int push(Heap *heap, const Node *nodes, int index) {
    if (!heap || !nodes || index < 0 || index >= NODE_LIMIT || heap->size >= BYTE_VALUES) {
        return 1;
    }
    size_t i = heap->size;
    ++heap->size;
    while (i) {
        size_t parent = (i - 1) / 2;
        if (nodes[heap->index[parent]].weight <= nodes[index].weight) {
            break;
        }
        heap->index[i] = heap->index[parent];
        i = parent;
    }
    heap->index[i] = index;
    return 0;
}

static int pop(Heap *heap, const Node *nodes, int *out) {
    if (!heap || !nodes || !out || !heap->size) {
        return 1;
    }
    *out = heap->index[0];
    --heap->size;
    int last = heap->index[heap->size];
    size_t i = 0;
    while (i * 2 + 1 < heap->size) {
        size_t child = i * 2 + 1;
        if (child + 1 < heap->size &&
            nodes[heap->index[child + 1]].weight < nodes[heap->index[child]].weight) {
            ++child;
        }
        if (nodes[last].weight <= nodes[heap->index[child]].weight) {
            break;
        }
        heap->index[i] = heap->index[child];
        i = child;
    }
    if (heap->size) {
        heap->index[i] = last;
    }
    return 0;
}

static int codes(const Node *nodes, int index, char path[BYTE_VALUES], size_t length,
                 char table[BYTE_VALUES][BYTE_VALUES]) {
    if (!nodes || !path || !table || index < 0 || index >= NODE_LIMIT || length >= BYTE_VALUES) {
        return 1;
    }
    const Node *node = &nodes[index];
    if (node->byte >= 0) {
        if (!length) {
            path[length] = '0';
            ++length;
        }
        path[length] = '\0';
        memcpy(table[node->byte], path, length + 1);
        return 0;
    }
    path[length] = '0';
    int status = codes(nodes, node->left, path, length + 1, table);
    if (status) {
        return status;
    }
    path[length] = '1';
    return codes(nodes, node->right, path, length + 1, table);
}

static int frequencies(FILE *in, uint64_t freq[BYTE_VALUES], uint64_t *total) {
    if (!in || !freq || !total) {
        return 1;
    }
    int c;
    while ((c = fgetc(in)) != EOF) {
        if (*total == UINT64_MAX) {
            return 2;
        }
        ++freq[c];
        ++*total;
    }
    return ferror(in) ? 3 : 0;
}

int huffman(const uint64_t freq[BYTE_VALUES], char table[BYTE_VALUES][BYTE_VALUES],
            uint64_t *bits) {
    if (!freq || !table || !bits) {
        return 1;
    }
    Node nodes[NODE_LIMIT];
    Heap heap = {0};
    int used = 0;
    for (int c = 0; c < BYTE_VALUES; ++c) {
        if (freq[c]) {
            nodes[used] = (Node){freq[c], -1, -1, c};
            if (push(&heap, nodes, used)) {
                return 1;
            }
            ++used;
        }
    }
    while (heap.size > 1) {
        int left;
        int right;
        if (pop(&heap, nodes, &left)) {
            return 1;
        }
        if (pop(&heap, nodes, &right)) {
            return 1;
        }
        if (nodes[left].weight > UINT64_MAX - nodes[right].weight) {
            return 2;
        }
        nodes[used] = (Node){nodes[left].weight + nodes[right].weight, left, right, -1};
        if (push(&heap, nodes, used)) {
            return 1;
        }
        ++used;
    }
    *bits = 0;
    if (heap.size) {
        char path[BYTE_VALUES];
        int status = codes(nodes, heap.index[0], path, 0, table);
        if (status) {
            return status;
        }
    }
    for (int c = 0; c < BYTE_VALUES; ++c) {
        if (freq[c]) {
            size_t length = strlen(table[c]);
            if (freq[c] > (UINT64_MAX - *bits) / length) {
                return 2;
            }
            *bits += freq[c] * length;
        }
    }
    return 0;
}

int main(int argc, char **argv) {
    if (argc != 2) {
        return finish(1);
    }
    FILE *in = fopen(argv[1], "rb");
    if (!in) {
        return finish(3);
    }
    uint64_t freq[BYTE_VALUES] = {0}, total = 0, bits = 0;
    char table[BYTE_VALUES][BYTE_VALUES];
    int status = frequencies(in, freq, &total);
    if (fclose(in) == EOF) {
        status = 3;
    }
    if (!status) {
        status = huffman(freq, table, &bits);
    }
    for (int c = 0; c < BYTE_VALUES && !status; ++c) {
        if (freq[c] && printf("%d %" PRIu64 " %s\n", c, freq[c], table[c]) < 0) {
            status = 3;
        }
    }
    uint64_t bytes = bits / 8 + (bits % 8 != 0);
    long double saving = total ? 100.0L * (1.0L - (long double)bytes / total) : 0;
    if (!status && printf("ORIGINAL_BYTES %" PRIu64 "\nENCODED_BITS %" PRIu64
                          "\nENCODED_BYTES %" PRIu64 "\nSAVING %.6Lf\n",
                          total, bits, bytes, saving) < 0) {
        status = 3;
    }
    return finish(status);
}
