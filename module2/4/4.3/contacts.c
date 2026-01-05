#include "contacts.h"

// Копирование строки
static void safe_copy(char *dst, size_t cap, const char *src) {
    if (!dst || cap == 0) return;
    if (!src) { dst[0] = '\0'; return; }
    snprintf(dst, cap, "%s", src);
}

// Копирование контакта с обрезкой 
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

// Создание узла
static TreeNode* node_new(const Contact *c) {
    TreeNode *n = (TreeNode*)malloc(sizeof(TreeNode));
    if (!n) return NULL;
    n->left = n->right = NULL;
    n->data = *c;
    return n;
}

// Освобождение дерева
static void node_free_all(TreeNode *root) {
    if (!root) return;
    node_free_all(root->left);
    node_free_all(root->right);
    free(root);
}

// Вставка по id
static TreeNode* bst_insert(TreeNode *root, TreeNode *node) {
    if (!root) return node;
    if (node->data.id < root->data.id) root->left = bst_insert(root->left, node);
    else root->right = bst_insert(root->right, node);
    return root;
}

// Поиск min в поддереве
static TreeNode* bst_min(TreeNode *root) {
    TreeNode *cur = root;
    while (cur && cur->left) cur = cur->left;
    return cur;
}

// Удаление
static TreeNode* bst_remove(TreeNode *root, unsigned int id, bool *removed) {
    if (!root) return NULL;

    if (id < root->data.id) {
        root->left = bst_remove(root->left, id, removed);
        return root;
    }
    if (id > root->data.id) {
        root->right = bst_remove(root->right, id, removed);
        return root;
    }

    *removed = true;

    if (!root->left && !root->right) {
        free(root);
        return NULL;
    }
    if (!root->left) {
        TreeNode *r = root->right;
        free(root);
        return r;
    }
    if (!root->right) {
        TreeNode *l = root->left;
        free(root);
        return l;
    }

    TreeNode *succ = bst_min(root->right);
    root->data = succ->data;
    root->right = bst_remove(root->right, succ->data.id, removed);
    return root;
}

// Inorder сбор узлов в массив
static void inorder_collect(TreeNode *root, TreeNode **arr, size_t *idx) {
    if (!root) return;
    inorder_collect(root->left, arr, idx);
    arr[(*idx)++] = root;
    inorder_collect(root->right, arr, idx);
}

// Построение сбалансированного дерева 
static TreeNode* build_balanced(TreeNode **arr, int l, int r) {
    if (l > r) return NULL;
    int m = l + (r - l) / 2;
    TreeNode *root = arr[m];
    root->left = build_balanced(arr, l, m - 1);
    root->right = build_balanced(arr, m + 1, r);
    return root;
}

// Периодическая балансировка
void cb_balance(ContactBook *cb) {
    if (!cb) return;
    if (!cb->root || cb->count < 2) {
        cb->ops_since_balance = 0;
        return;
    }

    TreeNode **arr = (TreeNode**)malloc(cb->count * sizeof(TreeNode*));
    if (!arr) return;

    size_t idx = 0;
    inorder_collect(cb->root, arr, &idx);

    for (size_t i = 0; i < idx; i++) {
        arr[i]->left = NULL;
        arr[i]->right = NULL;
    }

    cb->root = build_balanced(arr, 0, (int)idx - 1);
    cb->ops_since_balance = 0;

    free(arr);
}

// Увеличить счётчик и при необходимости сбалансировать
static void maybe_balance(ContactBook *cb) {
    cb->ops_since_balance++;
    if (cb->ops_since_balance >= BALANCE_EVERY_OPS) {
        cb_balance(cb);
    }
}

void cb_init(ContactBook *cb) {
    if (!cb) return;
    cb->root = NULL;
    cb->count = 0;
    cb->next_id = 1;
    cb->ops_since_balance = 0;
}

void cb_free(ContactBook *cb) {
    if (!cb) return;
    node_free_all(cb->root);
    cb_init(cb);
}

TreeNode* cb_find_node_by_id(ContactBook *cb, unsigned int id) {
    if (!cb || id == 0) return NULL;
    TreeNode *cur = cb->root;
    while (cur) {
        if (id < cur->data.id) cur = cur->left;
        else if (id > cur->data.id) cur = cur->right;
        else return cur;
    }
    return NULL;
}

Contact* cb_find_by_id(ContactBook *cb, unsigned int id) {
    TreeNode *n = cb_find_node_by_id(cb, id);
    return n ? &n->data : NULL;
}

bool cb_add(ContactBook *cb, const Contact *src, unsigned int *assigned_id) {
    if (!cb || !src) return false;
    if (cb->count >= MAX_CONTACTS) return false;
    if (src->surname[0] == '\0' || src->name[0] == '\0') return false;

    Contact c = {0};
    contact_copy(&c, src);
    c.id = cb->next_id++;

    TreeNode *node = node_new(&c);
    if (!node) return false;

    cb->root = bst_insert(cb->root, node);
    cb->count++;

    if (assigned_id) *assigned_id = c.id;
    maybe_balance(cb);
    return true;
}

bool cb_edit(ContactBook *cb, unsigned int id, const Contact *patch) {
    if (!cb || !patch) return false;
    if (patch->surname[0] == '\0' || patch->name[0] == '\0') return false;

    TreeNode *n = cb_find_node_by_id(cb, id);
    if (!n) return false;

    unsigned int keep_id = n->data.id;

    Contact tmp = {0};
    contact_copy(&tmp, patch);
    tmp.id = keep_id;

    n->data = tmp;

    maybe_balance(cb);
    return true;
}

bool cb_remove(ContactBook *cb, unsigned int id) {
    if (!cb || cb->count == 0) return false;

    bool removed = false;
    cb->root = bst_remove(cb->root, id, &removed);
    if (!removed) return false;

    cb->count--;
    maybe_balance(cb);
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

// Inorder печать с разделителями между контактами
static void inorder_print(TreeNode *root, bool *first) {
    if (!root) return;
    inorder_print(root->left, first);

    if (!*first) puts("-----------------------------");
    *first = false;

    cb_print_one(&root->data);

    inorder_print(root->right, first);
}

void cb_print(const ContactBook *cb) {
    if (!cb || cb->count == 0 || !cb->root) {
        puts("(список пуст)");
        return;
    }
    bool first = true;
    inorder_print(cb->root, &first);
}

// Высота узла
static size_t node_height(TreeNode *root) {
    if (!root) return 0;
    size_t hl = node_height(root->left);
    size_t hr = node_height(root->right);
    return (hl > hr ? hl : hr) + 1;
}

// Высота дерева
size_t cb_height(const ContactBook *cb) {
    if (!cb) return 0;
    return node_height(cb->root);
}
