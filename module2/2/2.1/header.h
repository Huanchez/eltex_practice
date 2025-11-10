#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

/* Ограничения */
#define MAX_CONTACTS      1000

#define MAX_NAME_LEN      30
#define MAX_SURNAME_LEN   30
#define MAX_MIDNAME_LEN   30
#define MAX_COMPANY_LEN   64
#define MAX_TITLE_LEN     64

#define MAX_PHONES        5
#define MAX_PHONE_LEN     20

#define MAX_EMAILS        5
#define MAX_EMAIL_LEN     64

#define MAX_SOCIALS       5
#define MAX_SOCIAL_LEN    64

/* Модель данных */
typedef struct {
    unsigned int id;                                  // автоинкремент
    char surname[MAX_SURNAME_LEN + 1];                // обяз.
    char name[MAX_NAME_LEN + 1];                      // обяз.
    char middlename[MAX_MIDNAME_LEN + 1];             // опц.
    char company[MAX_COMPANY_LEN + 1];                // опц.
    char title[MAX_TITLE_LEN + 1];                    // опц.

    char phones[MAX_PHONES][MAX_PHONE_LEN + 1];       // опц.
    size_t phone_count;

    char emails[MAX_EMAILS][MAX_EMAIL_LEN + 1];       // опц.
    size_t email_count;

    char socials[MAX_SOCIALS][MAX_SOCIAL_LEN + 1];    // опц.
    size_t social_count;
} Contact;

typedef struct {
    Contact items[MAX_CONTACTS];  // массив
    size_t count;
    unsigned int next_id;
} ContactBook;

/* Операции */
void   cb_init(ContactBook *cb);
bool   cb_add(ContactBook *cb, const Contact *src, unsigned int *assigned_id);
int    cb_index_by_id(const Contact *arr, size_t n, unsigned int id);
bool   cb_edit(ContactBook *cb, unsigned int id, const Contact *patch);
bool   cb_remove(ContactBook *cb, unsigned int id);

/* Вывод */
void   cb_print_one(const Contact *c);
void   cb_print(const ContactBook *cb);