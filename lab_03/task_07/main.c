#define _POSIX_C_SOURCE 200809L
#include "../common.h"
#include "../date.h"
#include "../files.h"

typedef struct {
    char *surname, *name, *patronymic;
    char birth[11], sex;
    uint64_t date;
    double income;
} Liver;
typedef struct Node {
    Liver person;
    size_t id;
    struct Node *next;
} Node;
typedef enum {
    CHANGE_NONE,
    CHANGE_ADD,
    CHANGE_REMOVE,
    CHANGE_EDIT
} Operation;

typedef struct Change {
    Operation operation;
    Node *node;
    Liver old;
    struct Change *next;
} Change;

static int clear(Liver *p) {
    if (!p) {
        return 1;
    }
    free(p->surname);
    free(p->name);
    free(p->patronymic);
    *p = (Liver){0};
    return 0;
}
static int parse(char *const *v, Liver *p) {
    if (!v || !p) {
        return 1;
    }
    if (latin(v[0]) || latin(v[1]) || (strcmp(v[2], "-") && latin(v[2]))) {
        return 1;
    }
    if (timestamp(v[3], NULL, '.', &p->date)) {
        return 1;
    }
    if (strlen(v[4]) != 1 || !strchr("MW", v[4][0])) {
        return 1;
    }
    int status = real_number(v[5], &p->income);
    if (status || p->income < 0) {
        return 1;
    }
    memcpy(p->birth, v[3], 11);
    p->sex = v[4][0];
    status = copy_text(v[0], &p->surname);
    if (!status) {
        status = copy_text(v[1], &p->name);
    }
    if (!status) {
        status = copy_text(strcmp(v[2], "-") ? v[2] : "", &p->patronymic);
    }
    return status;
}
static int insert(Node **root, Node *node) {
    if (!root || !node) {
        return 1;
    }
    while (*root && ((*root)->person.date > node->person.date ||
                     ((*root)->person.date == node->person.date && (*root)->id < node->id))) {
        root = &(*root)->next;
    }
    node->next = *root;
    *root = node;
    return 0;
}
static int detach(Node **root, Node *node) {
    if (!root || !node) {
        return 1;
    }
    while (*root && *root != node) {
        root = &(*root)->next;
    }
    if (!*root) {
        return 1;
    }
    *root = node->next;
    node->next = NULL;
    return 0;
}
static int print(FILE *out, const Node *node, int show_id) {
    if (!out || !node) {
        return 1;
    }
    const Liver *p = &node->person;
    if (show_id && fprintf(out, "%zu ", node->id) < 0) {
        return 3;
    }
    const char *patronymic = *p->patronymic ? p->patronymic : "-";
    if (fprintf(out, "%s %s %s %s %c %.17g\n", p->surname, p->name, patronymic, p->birth, p->sex,
                p->income) < 0) {
        return 3;
    }
    return 0;
}
static int discard(Change **history) {
    if (!history) {
        return 1;
    }
    while (*history) {
        Change *change = *history;
        *history = change->next;
        if (change->operation == CHANGE_REMOVE) {
            if (clear(&change->node->person)) {
                return 1;
            }
            free(change->node);
        } else if (change->operation == CHANGE_EDIT && clear(&change->old)) {
            return 1;
        }
        free(change);
    }
    return 0;
}
static int undo(Node **root, Change **history, size_t *count) {
    if (!root || !history || !count) {
        return 1;
    }
    for (size_t i = 0; i < *count / 2; ++i) {
        Change *change = *history;
        if (!change) {
            return 1;
        }
        Node *node = change->node;
        if (change->operation == CHANGE_ADD) {
            if (detach(root, node)) {
                return 1;
            }
            if (clear(&node->person)) {
                return 1;
            }
            free(node);
        } else if (change->operation == CHANGE_REMOVE) {
            if (insert(root, node)) {
                return 1;
            }
        } else if (change->operation == CHANGE_EDIT) {
            if (detach(root, node)) {
                return 1;
            }
            if (clear(&node->person)) {
                return 1;
            }
            node->person = change->old;
            if (insert(root, node)) {
                return 1;
            }
        } else {
            return 1;
        }
        *history = change->next;
        free(change);
    }
    *count = 0;
    return discard(history);
}
static int modify(Node **root, Change **history, size_t *count, size_t *next_id, char *const *v,
                  size_t n) {
    if (!root || !history || !count || !next_id || !v || !n || *count == SIZE_MAX) {
        return 1;
    }
    Operation operation = CHANGE_NONE;
    if (n == 7 && !strcmp(v[0], "add")) {
        operation = CHANGE_ADD;
    }
    if (n == 2 && !strcmp(v[0], "remove")) {
        operation = CHANGE_REMOVE;
    }
    if (n == 8 && !strcmp(v[0], "edit")) {
        operation = CHANGE_EDIT;
    }
    if (!operation) {
        return 1;
    }
    Node *node = NULL;
    if (operation != CHANGE_ADD) {
        uint64_t id;
        if (number(v[1], SIZE_MAX, &id) || !id) {
            return 1;
        }
        node = *root;
        while (node && node->id != id) {
            node = node->next;
        }
        if (!node) {
            return 1;
        }
    } else if (*next_id == SIZE_MAX) {
        return 2;
    }
    Liver person = {0};
    int status = 0;
    if (operation == CHANGE_ADD) {
        status = parse(v + 1, &person);
    } else if (operation == CHANGE_EDIT) {
        status = parse(v + 2, &person);
    }
    Change *change = NULL;
    if (!status) {
        change = calloc(1, sizeof(*change));
        if (!change) {
            status = 2;
        }
    }
    if (!status && operation == CHANGE_ADD) {
        node = calloc(1, sizeof(*node));
        if (!node) {
            status = 2;
        } else {
            node->id = *next_id;
            ++*next_id;
            node->person = person;
            person = (Liver){0};
            status = insert(root, node);
        }
    } else if (!status) {
        status = detach(root, node);
        if (!status && operation == CHANGE_EDIT) {
            change->old = node->person;
            node->person = person;
            person = (Liver){0};
            status = insert(root, node);
        }
    }
    if (status) {
        free(change);
    } else {
        change->operation = operation;
        change->node = node;
        change->next = *history;
        *history = change;
        ++*count;
    }
    if (clear(&person)) {
        return 1;
    }
    return status;
}
static int matches(const Liver *p, const char *key, const char *value, double epsilon, int *out) {
    if (!key || !value || !out || epsilon <= 0) {
        return 1;
    }
    if (!strcmp(key, "income")) {
        double income;
        if (real_number(value, &income) || income < 0) {
            return 1;
        }
        *out = p && fabs(p->income - income) <= epsilon;
    } else if (!strcmp(key, "birth")) {
        uint64_t date;
        if (timestamp(value, NULL, '.', &date)) {
            return 1;
        }
        *out = p && p->date == date;
    } else if (!strcmp(key, "sex")) {
        if (strlen(value) != 1 || !strchr("MW", *value)) {
            return 1;
        }
        *out = p && p->sex == *value;
    } else {
        const char *candidate = NULL;
        int patronymic = strcmp(key, "patronymic") == 0;
        if (strcmp(key, "surname") == 0) {
            if (p) {
                candidate = p->surname;
            }
        } else if (strcmp(key, "name") == 0) {
            if (p) {
                candidate = p->name;
            }
        } else if (patronymic) {
            if (p) {
                candidate = p->patronymic;
            }
        } else {
            return 1;
        }
        if (patronymic && strcmp(value, "-") == 0) {
            value = "";
        } else if (latin(value)) {
            return 1;
        }
        *out = candidate && strcmp(value, candidate) == 0;
    }
    return 0;
}

int main(int argc, char **argv) {
    if (argc != 2) {
        return finish(1);
    }
    FILE *in = fopen(argv[1], "r");
    if (!in) {
        return finish(3);
    }
    int status = 0;
    Node *root = NULL;
    size_t next_id = 1, count = 0;
    Change *history = NULL;
    while (!status) {
        char *line = NULL, *v[6];
        size_t n;
        status = read_line(in, &line);
        if (status == 4) {
            status = 0;
            break;
        }
        if (status) {
            break;
        }
        Liver p = {0};
        if (fields(line, v, 6, &n) || n != 6) {
            status = 1;
        } else {
            status = parse(v, &p);
        }
        free(line);
        if (!status && next_id == SIZE_MAX) {
            status = 2;
        }
        if (!status) {
            Node *node = malloc(sizeof(*node));
            if (!node) {
                status = 2;
            } else {
                *node = (Node){p, next_id, NULL};
                ++next_id;
                p = (Liver){0};
                status = insert(&root, node);
            }
        }
        if (clear(&p)) {
            status = 1;
        }
    }
    while (!status) {
        char *line = NULL, *v[13];
        size_t n = 0;
        status = read_line(stdin, &line);
        if (status == 4) {
            status = 0;
            break;
        }
        if (status) {
            break;
        }
        int command = fields(line, v, 13, &n);
        if (!command && n == 1 && !strcmp(v[0], "quit")) {
            free(line);
            break;
        }
        if (!command && n == 1 && !strcmp(v[0], "undo")) {
            command = undo(&root, &history, &count);
        } else if (!command && n == 2 && !strcmp(v[0], "save")) {
            FILE *out = NULL;
            command = open_distinct(in, v[1], &out);
            for (const Node *p = root; p && !command; p = p->next) {
                command = print(out, p, 0);
            }
            if (out && fclose(out) == EOF) {
                command = 3;
            }
        } else if (!command && n == 1 && !strcmp(v[0], "list")) {
            for (const Node *p = root; p && !command; p = p->next) {
                command = print(stdout, p, 1);
            }
        } else if (!command && n >= 3 && n % 2 == 1 && !strcmp(v[0], "find")) {
            int matched;
            for (size_t i = 1; i < n && !command; i += 2) {
                command = matches(NULL, v[i], v[i + 1], 1e-9, &matched);
            }
            for (const Node *p = root; p && !command; p = p->next) {
                matched = 1;
                for (size_t i = 1; i < n && matched && !command; i += 2) {
                    command = matches(&p->person, v[i], v[i + 1], 1e-9, &matched);
                }
                if (!command && matched) {
                    command = print(stdout, p, 1);
                }
            }
        } else if (!command) {
            command = modify(&root, &history, &count, &next_id, v, n);
        }
        free(line);
        if (command && fprintf(stderr, "Command error (%d)\n", command) < 0) {
            status = 3;
        }
        if (ferror(stdout) || fflush(stdout) == EOF) {
            status = 3;
        }
    }
    if (discard(&history)) {
        status = 1;
    }
    while (root) {
        Node *next = root->next;
        if (clear(&root->person)) {
            status = 1;
        }
        free(root);
        root = next;
    }
    if (fclose(in) == EOF) {
        status = 3;
    }
    return finish(status);
}
