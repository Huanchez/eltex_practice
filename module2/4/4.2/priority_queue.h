#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Приоритет: 0..255 */
#define PQ_PRIO_MIN 0u
#define PQ_PRIO_MAX 255u
#define PQ_LEVELS   256u

typedef struct Message {
    unsigned int id;
    uint8_t priority;     
    char payload[64];     
} Message;

/* Узел очереди */
typedef struct PQNode {
    Message msg;
    struct PQNode *next;
} PQNode;

/* Обычная очередь */
typedef struct {
    PQNode *head;
    PQNode *tail;
    size_t size;
} FIFOQueue;

/* Очередь с приоритетами: 256 FIFO-очередей */
typedef struct {
    FIFOQueue q[PQ_LEVELS];
    size_t total_size;
} PriorityQueue;

/* Инициализация и очистка */
void pq_init(PriorityQueue *pq);
void pq_free(PriorityQueue *pq);

/* Добавление */
bool pq_push(PriorityQueue *pq, const Message *m);

bool pq_pop_first(PriorityQueue *pq, Message *out);
bool pq_pop_by_priority(PriorityQueue *pq, uint8_t priority, Message *out);
bool pq_pop_not_below(PriorityQueue *pq, uint8_t min_priority, Message *out);

/* Статистика */
size_t pq_size(const PriorityQueue *pq);
bool pq_is_empty(const PriorityQueue *pq);
