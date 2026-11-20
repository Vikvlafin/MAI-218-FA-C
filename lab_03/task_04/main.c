#include "../common.h"
#include "../date.h"
#include <limits.h>
#include <float.h>
#include <inttypes.h>

typedef struct {
    char *text;
    int length;
} String;
typedef struct {
    String city, street, block, zip;
    uint64_t house, flat;
} Address;
typedef struct {
    Address address;
    String id, created, due, delivered;
    uint64_t creation_time, due_time;
    double weight;
} Mail;
typedef struct {
    const Address *office;
    Mail *mail;
    size_t count, capacity;
} Post;

static int string_clear(String *s) {
    if (!s) {
        return 1;
    }
    free(s->text);
    *s = (String){0};
    return 0;
}
static int string_make(String *s, const char *text) {
    if (!s || !text || s->text || strlen(text) > INT_MAX) {
        return 1;
    }
    int status = copy_text(text, &s->text);
    if (!status) {
        s->length = (int)strlen(text);
    }
    return status;
}
static int string_order(const String *a, const String *b, int *out) {
    if (!a || !b || !a->text || !b->text || !out) {
        return 1;
    }
    if (a->length == b->length) {
        *out = strcmp(a->text, b->text);
    } else {
        *out = a->length > b->length ? 1 : -1;
    }
    return 0;
}
static int string_equal(const String *a, const String *b, int *out) {
    if (!a || !b || !a->text || !b->text || !out) {
        return 1;
    }
    *out = strcmp(a->text, b->text) == 0;
    return 0;
}
static int string_copy(String *dest, const String *src) {
    if (!dest || !src || !src->text) {
        return 1;
    }
    String copy = {0};
    int status = string_make(&copy, src->text);
    if (!status) {
        if (string_clear(dest)) {
            free(copy.text);
            return 1;
        }
        *dest = copy;
    }
    return status;
}
static int string_clone(const String *src, String **out) {
    if (!src || !out) {
        return 1;
    }
    String *s = calloc(1, sizeof(*s));
    if (!s) {
        return 2;
    }
    int status = string_copy(s, src);
    if (status) {
        free(s);
    } else {
        *out = s;
    }
    return status;
}
static int string_append(String *a, const String *b) {
    if (!a || !b || !a->text || !b->text || b->length > INT_MAX - a->length) {
        return 1;
    }
    int n = a->length + b->length;
    char *text = malloc((size_t)n + 1);
    if (!text) {
        return 2;
    }
    memcpy(text, a->text, (size_t)a->length);
    memcpy(text + a->length, b->text, (size_t)b->length + 1);
    free(a->text);
    *a = (String){text, n};
    return 0;
}
static int demo(FILE *out) {
    if (!out) {
        return 1;
    }
    String a = {0}, b = {0}, c = {0}, *copy = NULL;
    int order = 0, equal = 0;
    int status = string_make(&a, "ab");
    if (!status) {
        status = string_make(&b, "c");
    }
    if (!status) {
        status = string_order(&a, &b, &order);
    }
    if (!status) {
        status = string_copy(&c, &a);
    }
    if (!status) {
        status = string_clone(&c, &copy);
    }
    if (!status) {
        status = string_equal(&a, copy, &equal);
    }
    if (!status) {
        status = string_append(&a, &b);
    }
    if (!status && fprintf(out, "%d %d %s %s\n", order, equal, a.text, copy->text) < 0) {
        status = 3;
    }
    if (copy && string_clear(copy)) {
        status = 1;
    }
    free(copy);
    int first_status = string_clear(&a);
    int second_status = string_clear(&b);
    int third_status = string_clear(&c);
    if (first_status || second_status || third_status) {
        status = 1;
    }
    return status;
}
static int digit_text(const char *s, size_t n) {
    if (!s || strlen(s) != n) {
        return 1;
    }
    for (; *s; ++s) {
        if (!isdigit((unsigned char)*s)) {
            return 1;
        }
    }
    return 0;
}
static int clear_address(Address *a) {
    if (!a) {
        return 1;
    }
    int city_status = string_clear(&a->city);
    int street_status = string_clear(&a->street);
    int block_status = string_clear(&a->block);
    int zip_status = string_clear(&a->zip);
    return city_status || street_status || block_status || zip_status;
}
static int parse_address(char *const *v, Address *a) {
    if (!v || !a || !*v[0] || !*v[1]) {
        return 1;
    }
    if (digit_text(v[5], 6)) {
        return 1;
    }
    int status = number(v[2], UINT64_MAX, &a->house);
    if (status || !a->house) {
        return 1;
    }
    status = number(v[4], UINT64_MAX, &a->flat);
    if (status || !a->flat) {
        return 1;
    }
    status = string_make(&a->city, v[0]);
    if (!status) {
        status = string_make(&a->street, v[1]);
    }
    if (!status) {
        status = string_make(&a->block, strcmp(v[3], "-") ? v[3] : "");
    }
    if (!status) {
        status = string_make(&a->zip, v[5]);
    }
    return status;
}
static int time_string(const char *date, const char *time, String *s) {
    if (!date || !time || !s) {
        return 1;
    }
    char buf[20];
    if (snprintf(buf, sizeof(buf), "%s %s", date, time) != 19) {
        return 1;
    }
    return string_make(s, buf);
}
static int clear_mail(Mail *m) {
    if (!m) {
        return 1;
    }
    int address_status = clear_address(&m->address);
    int id_status = string_clear(&m->id);
    int created_status = string_clear(&m->created);
    int due_status = string_clear(&m->due);
    int delivered_status = string_clear(&m->delivered);
    return address_status || id_status || created_status || due_status || delivered_status;
}
static int parse_mail(char *const *v, size_t count, Mail *m) {
    if (!v || !m || (count != 13 && count != 14)) {
        return 1;
    }
    if (digit_text(v[6], 14)) {
        return 1;
    }
    int status = real_number(v[7], &m->weight);
    if (status || m->weight < 0) {
        return 1;
    }
    if (timestamp(v[8], v[9], ':', &m->creation_time)) {
        return 1;
    }
    status = timestamp(v[10], v[11], ':', &m->due_time);
    if (status || m->due_time < m->creation_time) {
        return 1;
    }
    int undelivered = count == 13;
    if (undelivered) {
        if (strcmp(v[12], "-") != 0) {
            return 1;
        }
    } else {
        uint64_t actual;
        if (timestamp(v[12], v[13], ':', &actual) || actual < m->creation_time) {
            return 1;
        }
    }
    status = parse_address(v, &m->address);
    if (!status) {
        status = string_make(&m->id, v[6]);
    }
    if (!status) {
        status = time_string(v[8], v[9], &m->created);
    }
    if (!status) {
        status = time_string(v[10], v[11], &m->due);
    }
    if (!status) {
        if (undelivered) {
            status = string_make(&m->delivered, "-");
        } else {
            status = time_string(v[12], v[13], &m->delivered);
        }
    }
    return status;
}
static int locate(const Post *p, const char *id, size_t *out) {
    if (!p || !out || digit_text(id, 14)) {
        return 1;
    }
    size_t i = 0;
    while (i < p->count && strcmp(p->mail[i].id.text, id)) {
        ++i;
    }
    *out = i;
    return 0;
}
static int add(Post *p, Mail m) {
    if (!p || !m.address.zip.text || !m.id.text) {
        return 1;
    }
    size_t pos;
    if (locate(p, m.id.text, &pos) || pos < p->count) {
        return 1;
    }
    if (p->count == p->capacity) {
        if (p->capacity > SIZE_MAX / 2 / sizeof(Mail)) {
            return 2;
        }
        size_t capacity = p->capacity ? p->capacity * 2 : 8;
        Mail *next = realloc(p->mail, capacity * sizeof(*next));
        if (!next) {
            return 2;
        }
        p->mail = next;
        p->capacity = capacity;
    }
    pos = 0;
    while (pos < p->count) {
        int cmp = strcmp(p->mail[pos].address.zip.text, m.address.zip.text);
        if (cmp > 0 || (!cmp && strcmp(p->mail[pos].id.text, m.id.text) > 0)) {
            break;
        }
        ++pos;
    }
    memmove(p->mail + pos + 1, p->mail + pos, (p->count - pos) * sizeof(Mail));
    p->mail[pos] = m;
    ++p->count;
    return 0;
}
static int print_mail(FILE *out, const Mail *m) {
    if (!out || !m) {
        return 1;
    }
    const Address *a = &m->address;
    const char *block = a->block.length ? a->block.text : "-";
    if (fprintf(out, "%s %s %s %" PRIu64 " %s %" PRIu64 " %s %.17g %s %s %s\n", m->id.text,
                a->city.text, a->street.text, a->house, block, a->flat, a->zip.text, m->weight,
                m->created.text, m->due.text, m->delivered.text) < 0) {
        return 3;
    }
    return 0;
}
static int by_creation(const void *a, const void *b) {
    const Mail *x = *(const Mail *const *)a, *y = *(const Mail *const *)b;
    if (x->creation_time == y->creation_time) {
        return strcmp(x->id.text, y->id.text);
    }
    return x->creation_time > y->creation_time ? 1 : -1;
}
typedef enum {
    MAIL_ALL,
    MAIL_DELIVERED,
    MAIL_OVERDUE
} MailFilter;

static int select_mail(const Post *p, MailFilter mode, uint64_t now, const Mail ***out,
                       size_t *count) {
    if (!p || !p->office || (mode != MAIL_ALL && mode != MAIL_DELIVERED && mode != MAIL_OVERDUE) ||
        !out || !count) {
        return 1;
    }
    if (!p->count) {
        *out = NULL;
        *count = 0;
        return 0;
    }
    if (p->count > SIZE_MAX / sizeof(const Mail *)) {
        return 2;
    }
    const Mail **list = malloc(p->count * sizeof(*list));
    if (!list) {
        return 2;
    }
    size_t n = 0;
    for (size_t i = 0; i < p->count; ++i) {
        const Mail *m = &p->mail[i];
        int delivered = strcmp(m->delivered.text, "-") != 0;
        if (mode == MAIL_ALL || (mode == MAIL_DELIVERED && delivered) ||
            (mode == MAIL_OVERDUE && !delivered && m->due_time < now)) {
            list[n] = m;
            ++n;
        }
    }
    if (mode != MAIL_ALL) {
        qsort(list, n, sizeof(*list), by_creation);
    }
    *out = list;
    *count = n;
    return 0;
}
static int show(const Post *p, MailFilter mode) {
    uint64_t now = 0;
    if (mode == MAIL_OVERDUE) {
        time_t t = time(NULL);
        struct tm *local = t == (time_t)-1 ? NULL : localtime(&t);
        if (!local || calendar(local->tm_mday, local->tm_mon + 1, local->tm_year + 1900, &now)) {
            return 3;
        }
        now += (unsigned)(local->tm_hour * 3600 + local->tm_min * 60 + local->tm_sec);
    }
    const Mail **list = NULL;
    size_t n = 0;
    int status = select_mail(p, mode, now, &list, &n);
    for (size_t i = 0; i < n && !status; ++i) {
        status = print_mail(stdout, list[i]);
    }
    free(list);
    return status;
}

int main(int argc, char **argv) {
    if (argc == 2 && !strcmp(argv[1], "string")) {
        return finish(demo(stdout));
    }
    if (argc != 1) {
        return finish(1);
    }
    char *line = NULL, *v[15];
    size_t n = 0;
    Address office = {0};
    Post post = {&office, NULL, 0, 0};
    int status = read_line(stdin, &line);
    if (!status) {
        status = fields(line, v, 15, &n);
    }
    if (!status && n != 7) {
        status = 1;
    }
    if (!status && strcmp(v[0], "office") != 0) {
        status = 1;
    }
    if (!status) {
        status = parse_address(v + 1, &office);
    }
    free(line);
    while (!status) {
        line = NULL;
        status = read_line(stdin, &line);
        if (status == 4) {
            status = 0;
            break;
        }
        if (status) {
            break;
        }
        int command = fields(line, v, 15, &n);
        if (!command && n == 1 && !strcmp(v[0], "quit")) {
            free(line);
            break;
        }
        if (!command && n > 1 && !strcmp(v[0], "add")) {
            Mail mail = {0};
            command = parse_mail(v + 1, n - 1, &mail);
            if (!command) {
                command = add(&post, mail);
            }
            if (command && clear_mail(&mail)) {
                command = 1;
            }
        } else if (!command && n == 2 && (!strcmp(v[0], "find") || !strcmp(v[0], "remove"))) {
            size_t i;
            command = locate(&post, v[1], &i);
            if (!command && i == post.count) {
                command = 1;
            }
            if (!command && !strcmp(v[0], "find")) {
                command = print_mail(stdout, &post.mail[i]);
            } else if (!command) {
                command = clear_mail(&post.mail[i]);
                --post.count;
                memmove(post.mail + i, post.mail + i + 1, (post.count - i) * sizeof(Mail));
            }
        } else if (!command && n == 1 && !strcmp(v[0], "list")) {
            command = show(&post, MAIL_ALL);
        } else if (!command && n == 1 && !strcmp(v[0], "delivered")) {
            command = show(&post, MAIL_DELIVERED);
        } else if (!command && n == 1 && !strcmp(v[0], "overdue")) {
            command = show(&post, MAIL_OVERDUE);
        } else {
            command = 1;
        }
        free(line);
        if (command && fprintf(stderr, "Command error (%d)\n", command) < 0) {
            status = 3;
        }
        if (ferror(stdout) || fflush(stdout) == EOF) {
            status = 3;
        }
    }
    for (size_t i = 0; i < post.count; ++i) {
        if (clear_mail(&post.mail[i])) {
            status = 1;
        }
    }
    free(post.mail);
    if (clear_address(&office)) {
        status = 1;
    }
    return finish(status);
}
