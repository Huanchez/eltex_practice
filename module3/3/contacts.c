#include "contacts.h"

#include <stdint.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

// Утилиты
static void safe_copy(char *dst, size_t cap, const char *src) {
    if (!dst || cap == 0) return;
    if (!src) { dst[0] = '\0'; return; }
    snprintf(dst, cap, "%s", src);
}

// Копирование contact
static void contact_copy(Contact *dst, const Contact *src) {
    *dst = (Contact){0};

    safe_copy(dst->surname, sizeof(dst->surname), src->surname);
    safe_copy(dst->name, sizeof(dst->name), src->name);
    safe_copy(dst->middlename, sizeof(dst->middlename), src->middlename);
    safe_copy(dst->company, sizeof(dst->company), src->company);
    safe_copy(dst->title, sizeof(dst->title), src->title);

    dst->phone_count = (src->phone_count > MAX_PHONES) ? MAX_PHONES : src->phone_count;
    for (size_t i = 0; i < dst->phone_count; ++i)
        safe_copy(dst->phones[i], sizeof(dst->phones[i]), src->phones[i]);

    dst->email_count = (src->email_count > MAX_EMAILS) ? MAX_EMAILS : src->email_count;
    for (size_t i = 0; i < dst->email_count; ++i)
        safe_copy(dst->emails[i], sizeof(dst->emails[i]), src->emails[i]);

    dst->social_count = (src->social_count > MAX_SOCIALS) ? MAX_SOCIALS : src->social_count;
    for (size_t i = 0; i < dst->social_count; ++i)
        safe_copy(dst->socials[i], sizeof(dst->socials[i]), src->socials[i]);
}

// Упорядоченность списка
static int contact_cmp_key(const Contact *a, const Contact *b) {
    int r = strcmp(a->surname, b->surname);
    if (r != 0) return r;

    r = strcmp(a->name, b->name);
    if (r != 0) return r;

    r = strcmp(a->middlename, b->middlename);
    if (r != 0) return r;

    if (a->id < b->id) return -1;
    if (a->id > b->id) return 1;
    return 0;
}

static void list_insert_sorted(ContactBook *cb, ContactNode *node) {
    if (!cb->head) {
        cb->head = cb->tail = node;
        node->prev = node->next = NULL;
        return;
    }

    ContactNode *cur = cb->head;
    while (cur) {
        if (contact_cmp_key(&node->data, &cur->data) < 0) {
            node->next = cur;
            node->prev = cur->prev;

            if (cur->prev) cur->prev->next = node;
            else cb->head = node;

            cur->prev = node;
            return;
        }
        cur = cur->next;
    }

    // Вставка в конец
    node->prev = cb->tail;
    node->next = NULL;
    cb->tail->next = node;
    cb->tail = node;
}

static void list_unlink(ContactBook *cb, ContactNode *node) {
    if (!cb || !node) return;

    if (node->prev) node->prev->next = node->next;
    else cb->head = node->next;

    if (node->next) node->next->prev = node->prev;
    else cb->tail = node->prev;

    node->prev = node->next = NULL;
}

// API
void cb_init(ContactBook *cb) {
    if (!cb) return;
    cb->head = NULL;
    cb->tail = NULL;
    cb->count = 0;
    cb->next_id = 1;
}

void cb_free(ContactBook *cb) {
    if (!cb) return;

    ContactNode *cur = cb->head;
    while (cur) {
        ContactNode *next = cur->next;
        free(cur);
        cur = next;
    }

    cb_init(cb);
}

ContactNode* cb_find_node_by_id(ContactBook *cb, unsigned int id) {
    if (!cb || id == 0) return NULL;

    ContactNode *cur = cb->head;
    while (cur) {
        if (cur->data.id == id) return cur;
        cur = cur->next;
    }
    return NULL;
}

Contact* cb_find_by_id(ContactBook *cb, unsigned int id) {
    ContactNode *n = cb_find_node_by_id(cb, id);
    return n ? &n->data : NULL;
}

bool cb_add(ContactBook *cb, const Contact *src, unsigned int *assigned_id) {
    if (!cb || !src) return false;
    if (cb->count >= MAX_CONTACTS) return false;
    if (src->surname[0] == '\0' || src->name[0] == '\0') return false;

    ContactNode *node = (ContactNode*)malloc(sizeof(ContactNode));
    if (!node) return false;

    node->prev = node->next = NULL;

    contact_copy(&node->data, src);
    node->data.id = cb->next_id++;

    list_insert_sorted(cb, node);
    cb->count++;

    if (assigned_id) *assigned_id = node->data.id;
    return true;
}

bool cb_edit(ContactBook *cb, unsigned int id, const Contact *patch) {
    if (!cb || !patch) return false;
    if (patch->surname[0] == '\0' || patch->name[0] == '\0') return false;

    ContactNode *node = cb_find_node_by_id(cb, id);
    if (!node) return false;

    list_unlink(cb, node);

    Contact tmp = {0};
    contact_copy(&tmp, patch);
    tmp.id = id;
    node->data = tmp;

    list_insert_sorted(cb, node);
    return true;
}

bool cb_remove(ContactBook *cb, unsigned int id) {
    if (!cb || cb->count == 0) return false;

    ContactNode *node = cb_find_node_by_id(cb, id);
    if (!node) return false;

    list_unlink(cb, node);
    free(node);
    cb->count--;
    return true;
}

// Печать
void cb_print_one(const Contact *c) {
    if (!c) return;
    printf("[%u] %s %s", c->id, c->surname, c->name);
    if (c->middlename[0]) printf(" %s", c->middlename);
    printf("\n");

    if (c->company[0]) printf("  Работа  : %s\n", c->company);
    if (c->title[0])   printf("  Должн.  : %s\n", c->title);

    if (c->phone_count) {
        printf("  Телефоны:\n");
        for (size_t i = 0; i < c->phone_count; ++i)
            printf("    - %s\n", c->phones[i]);
    }
    if (c->email_count) {
        printf("  E-mail:\n");
        for (size_t i = 0; i < c->email_count; ++i)
            printf("    - %s\n", c->emails[i]);
    }
    if (c->social_count) {
        printf("  Соц/мессенджеры:\n");
        for (size_t i = 0; i < c->social_count; ++i)
            printf("    - %s\n", c->socials[i]);
    }
}

void cb_print(const ContactBook *cb) {
    if (!cb || cb->count == 0) {
        puts("(список пуст)");
        return;
    }

    const ContactNode *cur = cb->head;
    while (cur) {
        cb_print_one(&cur->data);
        if (cur->next) puts("-----------------------------");
        cur = cur->next;
    }
}

// Хранение в файле
#define CB_MAGIC   0x43424B31u  // 'CBK1'
#define CB_VERSION 1u

typedef struct {
    uint32_t magic;
    uint32_t version;
    uint32_t count;
    uint32_t next_id;
} CBFileHeader;

// Добавление контакта при загрузке из файла
static bool cb_add_loaded(ContactBook *cb, const Contact *src)
{
    if (!cb || !src) return false;
    if (cb->count >= MAX_CONTACTS) return false;
    if (src->surname[0] == '\0' || src->name[0] == '\0') return false;

    ContactNode *node = (ContactNode*)malloc(sizeof(ContactNode));
    if (!node) return false;

    node->prev = node->next = NULL;
    node->data = *src;

    list_insert_sorted(cb, node);
    cb->count++;

    if (src->id >= cb->next_id)
        cb->next_id = src->id + 1;

    return true;
}

bool cb_load(ContactBook *cb, const char *path)
{
    if (!cb || !path) return false;
    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        if (errno == ENOENT) return true; 
        perror("open (load)");
        return false;
    }

    CBFileHeader h;
    ssize_t r = read(fd, &h, (ssize_t)sizeof(h));
    if (r == 0) {
        close(fd);
        return true; 
    }
    if (r != (ssize_t)sizeof(h)) {
        fprintf(stderr, "Файл контактов повреждён\n");
        close(fd);
        return false;
    }

    if (h.magic != CB_MAGIC || h.version != CB_VERSION) {
        fprintf(stderr, "Неверный формат файла контактов\n");
        close(fd);
        return false;
    }

    if (h.count > MAX_CONTACTS) {
        fprintf(stderr, "Слишком много контактов (%u)\n", h.count);
        close(fd);
        return false;
    }

    cb_free(cb);
    cb_init(cb);

    for (uint32_t i = 0; i < h.count; ++i) {
        Contact c;
        ssize_t n = read(fd, &c, (ssize_t)sizeof(c));
        if (n != (ssize_t)sizeof(c)) {
            fprintf(stderr, "Файл контактов повреждён\n");
            close(fd);
            cb_free(cb);
            cb_init(cb);
            return false;
        }
        (void)cb_add_loaded(cb, &c);
    }

    if (h.next_id > cb->next_id)
        cb->next_id = h.next_id;

    close(fd);
    return true;
}

bool cb_save(const ContactBook *cb, const char *path)
{
    if (!cb || !path) return false;

    char tmp_path[4096];
    snprintf(tmp_path, sizeof(tmp_path), "%s.tmp", path);

    int fd = open(tmp_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        perror("open (save)");
        return false;
    }

    CBFileHeader h;
    h.magic = CB_MAGIC;
    h.version = CB_VERSION;
    h.count = (uint32_t)cb->count;
    h.next_id = (uint32_t)cb->next_id;

    if (write(fd, &h, (ssize_t)sizeof(h)) != (ssize_t)sizeof(h)) {
        perror("write (header)");
        close(fd);
        return false;
    }

    const ContactNode *cur = cb->head;
    while (cur) {
        if (write(fd, &cur->data, (ssize_t)sizeof(cur->data)) != (ssize_t)sizeof(cur->data)) {
            perror("write (contact)");
            close(fd);
            return false;
        }
        cur = cur->next;
    }

    if (close(fd) < 0) {
        perror("close");
        return false;
    }

    if (rename(tmp_path, path) < 0) {
        perror("rename");
        return false;
    }

    return true;
}
