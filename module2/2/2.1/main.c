#include "header.h"

/* ===== утилиты ввода ===== */
static void chomp(char *s){ if(!s)return; size_t n=strlen(s); if(n&&s[n-1]=='\n') s[n-1]='\0'; }

static void read_line(char *buf, size_t cap, const char *prompt){
    if(prompt) printf("%s", prompt);
    if(!fgets(buf, (int)cap, stdin)){ if(cap) buf[0]='\0'; return; }
    chomp(buf);
}

static unsigned int read_uint(const char *prompt){
    char s[64]; read_line(s,sizeof(s),prompt);
    return (unsigned int)strtoul(s,NULL,10);
}

/* Редактирование/ввод 2D-списка строк (любой ширины строки).
 * old_base/old_cnt — текущее содержимое (может быть NULL/0).
 * Для КАЖДОГО старого элемента:
 *   Enter — оставить, "-" — удалить, иначе — заменить введённым текстом.
 * Затем можно ДОБАВИТЬ новые строки (Enter — завершить).
 */
static size_t read_list_editable(char *dst_base, size_t max_items, size_t slot_cap,
                                 const char *prompt, const char *old_base, size_t old_cnt)
{
    printf("%s\n", prompt);
    size_t cnt = 0;
    char line[256];

    if (old_base && old_cnt) {
        puts("Текущее:");
        for (size_t i = 0; i < old_cnt; ++i)
            printf("  %zu) %s\n", i + 1, old_base + i * slot_cap);

        puts("(Enter — оставить, '-' — удалить, другая строка — заменить)");
        for (size_t i = 0; i < old_cnt && cnt < max_items; ++i) {
            printf("  #%zu: ", i + 1);
            if (!fgets(line, sizeof(line), stdin)) line[0] = '\0';
            chomp(line);

            if (line[0] == '\0') {
                /* оставить как есть */
                snprintf(dst_base + cnt * slot_cap, slot_cap, "%s", old_base + i * slot_cap);
                cnt++;
            } else if (strcmp(line, "-") == 0) {
                /* удалить — пропускаем */
            } else {
                /* заменить новой строкой */
                snprintf(dst_base + cnt * slot_cap, slot_cap, "%s", line);
                cnt++;
            }
        }
    } else {
        puts("(вводите по одному значению; пустая строка — закончить)");
    }

    /* добавление новых значений */
    while (cnt < max_items) {
        printf("  + #%zu (Enter — закончить): ", cnt + 1);
        if (!fgets(line, sizeof(line), stdin)) break;
        chomp(line);
        if (line[0] == '\0') break;
        snprintf(dst_base + cnt * slot_cap, slot_cap, "%s", line);
        cnt++;
    }
    return cnt;
}

/* Построить контакт: если old_opt != NULL — показываем старые значения и Enter их сохраняет */
static void build_contact(Contact *out, const Contact *old_opt){
    *out = (Contact){0};

    char prompt[256];

    if (old_opt){
        snprintf(prompt,sizeof(prompt),"Фамилия* (Enter: %s): ", old_opt->surname[0]?old_opt->surname:"(пусто)");
        read_line(out->surname, sizeof(out->surname), prompt);
        if(out->surname[0]=='\0') snprintf(out->surname,sizeof(out->surname),"%s", old_opt->surname);

        snprintf(prompt,sizeof(prompt),"Имя* (Enter: %s): ", old_opt->name[0]?old_opt->name:"(пусто)");
        read_line(out->name, sizeof(out->name), prompt);
        if(out->name[0]=='\0') snprintf(out->name,sizeof(out->name),"%s", old_opt->name);

        snprintf(prompt,sizeof(prompt),"Отчество (опц.) (Enter: %s): ", old_opt->middlename[0]?old_opt->middlename:"(пусто)");
        read_line(out->middlename, sizeof(out->middlename), prompt);
        if(out->middlename[0]=='\0') snprintf(out->middlename,sizeof(out->middlename),"%s", old_opt->middlename);

        snprintf(prompt,sizeof(prompt),"Место работы (опц.) (Enter: %s): ", old_opt->company[0]?old_opt->company:"(пусто)");
        read_line(out->company, sizeof(out->company), prompt);
        if(out->company[0]=='\0') snprintf(out->company,sizeof(out->company),"%s", old_opt->company);

        snprintf(prompt,sizeof(prompt),"Должность (опц.) (Enter: %s): ", old_opt->title[0]?old_opt->title:"(пусто)");
        read_line(out->title, sizeof(out->title), prompt);
        if(out->title[0]=='\0') snprintf(out->title,sizeof(out->title),"%s", old_opt->title);
    } else {
        read_line(out->surname, sizeof(out->surname), "Фамилия* : ");
        read_line(out->name,    sizeof(out->name),    "Имя*     : ");
        read_line(out->middlename, sizeof(out->middlename), "Отчество (опц.) : ");
        read_line(out->company, sizeof(out->company), "Место работы (опц.) : ");
        read_line(out->title,   sizeof(out->title),   "Должность   (опц.) : ");
    }

    /* списки: телефоны, e-mail, соцсети — точечное редактирование */
    out->phone_count = read_list_editable((char*)out->phones,  MAX_PHONES,  MAX_PHONE_LEN+1,
                                          "Телефоны:", old_opt ? (char*)old_opt->phones : NULL,
                                          old_opt ? old_opt->phone_count : 0);

    out->email_count = read_list_editable((char*)out->emails,  MAX_EMAILS,  MAX_EMAIL_LEN+1,
                                          "E-mail:",   old_opt ? (char*)old_opt->emails : NULL,
                                          old_opt ? old_opt->email_count : 0);

    out->social_count= read_list_editable((char*)out->socials, MAX_SOCIALS, MAX_SOCIAL_LEN+1,
                                          "Соцсети/мессенджеры:", old_opt ? (char*)old_opt->socials : NULL,
                                          old_opt ? old_opt->social_count : 0);
}

static void menu(void){
    puts("=== Телефонная книга ===");
    puts("[1] Добавить контакт");
    puts("[2] Редактировать контакт (по ID)");
    puts("[3] Удалить контакт (по ID)");
    puts("[4] Показать все контакты");
    puts("[0] Выход");
}

int main(void){
    setvbuf(stdout, NULL, _IONBF, 0); // подсказки печатаются сразу

    ContactBook cb; cb_init(&cb);

    for(;;){
        menu();
        unsigned int choice = read_uint("Выбор: ");
        if(choice==0) break;

        if(choice==1){
            Contact c; build_contact(&c, NULL);
            unsigned int new_id=0;
            if(!cb_add(&cb,&c,&new_id))
                puts("Ошибка: проверьте обязательные поля (Фамилия/Имя) и лимит.");
            else
                printf("Добавлено, ID = %u\n", new_id);

        } else if(choice==2){
            unsigned int id = read_uint("ID для редактирования: ");
            int idx = cb_index_by_id(cb.items, cb.count, id);
            if(idx<0){ puts("Контакт не найден."); continue; }

            puts("Текущая карточка:");
            cb_print_one(&cb.items[idx]);

            Contact patch; build_contact(&patch, &cb.items[idx]);
            if(!cb_edit(&cb, id, &patch)) puts("Ошибка редактирования."); else puts("Изменено.");

        } else if(choice==3){
            unsigned int id = read_uint("ID для удаления: ");
            if(!cb_remove(&cb,id)) puts("Не найден/ошибка удаления."); else puts("Удалено.");

        } else if(choice==4){
            cb_print(&cb);

        } else {
            puts("Неизвестный пункт меню.");
        }
        puts("");
    }
    return 0;
}
