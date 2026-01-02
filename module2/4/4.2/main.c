#include "priority_queue.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

static uint8_t rand_prio(void) {
    return (uint8_t)(rand() % 256);
}

static void gen_messages(PriorityQueue *pq, unsigned int n) {
    for (unsigned int i = 1; i <= n; i++) {
        Message m;
        m.id = i;
        m.priority = rand_prio();
        snprintf(m.payload, sizeof(m.payload), "msg_%u", i);

        if (!pq_push(pq, &m)) {
            fprintf(stderr, "push failed on id=%u\n", i);
            return;
        }
    }
}

int main(void) {
    srand((unsigned)time(NULL));

    PriorityQueue pq;
    pq_init(&pq);

    /* Генерация */
    const unsigned int N = 30;
    gen_messages(&pq, N);
    printf("Сгенерировано сообщений: %u, в очереди сейчас: %zu\n\n", N, pq_size(&pq));

    /* Извлечь пять первых */
    puts("pop_first (5 шт)");
    for (int i = 0; i < 5; i++) {
        Message m;
        if (pq_pop_first(&pq, &m)) {
            printf("pop_first: id=%u pr=%u payload=%s\n", m.id, m.priority, m.payload);
        } else {
            puts("pop_first: очередь пуста");
            break;
        }
    }
    printf("Осталось в очереди: %zu\n\n", pq_size(&pq));

    /* Извлечь строго по приоритету */
    uint8_t target_pr = (uint8_t)(rand() % 256);
    printf("pop_by_priority (prio=%u) пока есть\n", target_pr);
    int cnt = 0;
    while (1) {
        Message m;
        if (!pq_pop_by_priority(&pq, target_pr, &m)) break;
        printf("pop_by_priority: id=%u pr=%u payload=%s\n", m.id, m.priority, m.payload);
        cnt++;
    }
    if (cnt == 0) puts("(сообщений с таким приоритетом не было)");
    printf("Осталось в очереди: %zu\n\n", pq_size(&pq));

    /* Извлечь с приоритетом выше заданного */
    uint8_t min_pr = 200;
    printf("pop_not_below (min_pr=%u) пока есть\n", min_pr);
    cnt = 0;
    while (1) {
        Message m;
        if (!pq_pop_not_below(&pq, min_pr, &m)) break;
        printf("pop_not_below: id=%u pr=%u payload=%s\n", m.id, m.priority, m.payload);
        cnt++;
    }
    if (cnt == 0) puts("(сообщений с приоритетом >= min_pr не было)");
    printf("Осталось в очереди: %zu\n\n", pq_size(&pq));

    puts("добираем остаток с помощью pop_first");
    while (!pq_is_empty(&pq)) {
        Message m;
        pq_pop_first(&pq, &m);
        printf("pop_first: id=%u pr=%u payload=%s\n", m.id, m.priority, m.payload);
    }

    pq_free(&pq);
    return 0;
}
