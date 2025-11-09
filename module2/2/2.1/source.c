#include "header.h"

/* ===== Внутренние утилиты ===== */
static void safe_copy(char *dst, size_t cap, const char *src) {
    if (!dst || cap == 0) return;
    if (!src) { dst[0] = '\0'; return; }
    strncpy(dst, src, cap - 1);
    dst[cap - 1] = '\0';
}

/* ===== Реализация API ===== */
void cb_init(ContactBook *cb) {
    cb->count = 0;
    cb->next_id = 1;
}

int cb_index_by_id(const Contact *arr, size_t n, unsigned int id) {
    for (size_t i = 0; i < n; ++i)
        if (arr[i].id == id) return (int)i;
    return -1;
}

bool cb_add(ContactBook *cb, const Contact *src, unsigned int *assigned_id) {
    if (!cb || !src) return false;
    if (cb->count >= MAX_CONTACTS) return false;
    if (src->surname[0] == '\0' || src->name[0] == '\0') return false;

    Contact *dst = &cb->items[cb->count];
    *dst = (Contact){0};
    dst->id = cb->next_id++;

    safe_copy(dst->surname, sizeof(dst->surname), src->surname);
    safe_copy(dst->name,    sizeof(dst->name),    src->name);
    safe_copy(dst->middlename, sizeof(dst->middlename), src->middlename);
    safe_copy(dst->company, sizeof(dst->company), src->company);
    safe_copy(dst->title,   sizeof(dst->title),   src->title);

    dst->phone_count = (src->phone_count > MAX_PHONES) ? MAX_PHONES : src->phone_count;
    for (size_t i = 0; i < dst->phone_count; ++i)
        safe_copy(dst->phones[i], sizeof(dst->phones[i]), src->phones[i]);

    dst->email_count = (src->email_count > MAX_EMAILS) ? MAX_EMAILS : src->email_count;
    for (size_t i = 0; i < dst->email_count; ++i)
        safe_copy(dst->emails[i], sizeof(dst->emails[i]), src->emails[i]);

    dst->social_count = (src->social_count > MAX_SOCIALS) ? MAX_SOCIALS : src->social_count;
    for (size_t i = 0; i < dst->social_count; ++i)
        safe_copy(dst->socials[i], sizeof(dst->socials[i]), src->socials[i]);

    if (assigned_id) *assigned_id = dst->id;
    cb->count++;
    return true;
}

bool cb_edit(ContactBook *cb, unsigned int id, const Contact *patch) {
    if (!cb || !patch) return false;
    int idx = cb_index_by_id(cb->items, cb->count, id);
    if (idx < 0) return false;
    if (patch->surname[0] == '\0' || patch->name[0] == '\0') return false;

    Contact *dst = &cb->items[idx];
    unsigned int keep_id = dst->id;
    *dst = (Contact){0};
    dst->id = keep_id;

    safe_copy(dst->surname, sizeof(dst->surname), patch->surname);
    safe_copy(dst->name,    sizeof(dst->name),    patch->name);
    safe_copy(dst->middlename, sizeof(dst->middlename), patch->middlename);
    safe_copy(dst->company, sizeof(dst->company), patch->company);
    safe_copy(dst->title,   sizeof(dst->title),   patch->title);

    dst->phone_count = (patch->phone_count > MAX_PHONES) ? MAX_PHONES : patch->phone_count;
    for (size_t i = 0; i < dst->phone_count; ++i)
        safe_copy(dst->phones[i], sizeof(dst->phones[i]), patch->phones[i]);

    dst->email_count = (patch->email_count > MAX_EMAILS) ? MAX_EMAILS : patch->email_count;
    for (size_t i = 0; i < dst->email_count; ++i)
        safe_copy(dst->emails[i], sizeof(dst->emails[i]), patch->emails[i]);

    dst->social_count = (patch->social_count > MAX_SOCIALS) ? MAX_SOCIALS : patch->social_count;
    for (size_t i = 0; i < dst->social_count; ++i)
        safe_copy(dst->socials[i], sizeof(dst->socials[i]), patch->socials[i]);

    return true;
}

bool cb_remove(ContactBook *cb, unsigned int id) {
    if (!cb || cb->count == 0) return false;
    int idx = cb_index_by_id(cb->items, cb->count, id);
    if (idx < 0) return false;
    for (size_t i = (size_t)idx + 1; i < cb->count; ++i)
        cb->items[i - 1] = cb->items[i];
    cb->count--;
    return true;
}

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
    for (size_t i = 0; i < cb->count; ++i) {
        cb_print_one(&cb->items[i]);
        if (i + 1 < cb->count) puts("-----------------------------");
    }
}
