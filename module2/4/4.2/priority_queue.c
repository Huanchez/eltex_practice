#include "priority_queue.h"

#include <stdlib.h>
#include <string.h>

static void fifo_init(FIFOQueue *q) {
    q->head = q->tail = NULL;
    q->size = 0;
}

static void fifo_free(FIFOQueue *q) {
    PQNode *cur = q->head;
    while (cur) {
        PQNode *nxt = cur->next;
        free(cur);
        cur = nxt;
    }
    fifo_init(q);
}

static bool fifo_push(FIFOQueue *q, const Message *m) {
    PQNode *node = (PQNode*)malloc(sizeof(PQNode));
    if (!node) return false;
    node->next = NULL;
    node->msg = *m;

    if (!q->head) {
        q->head = q->tail = node;
    } else {
        q->tail->next = node;
        q->tail = node;
    }
    q->size++;
    return true;
}

static bool fifo_pop(FIFOQueue *q, Message *out) {
    if (!q->head) return false;
    PQNode *node = q->head;
    if (out) *out = node->msg;

    q->head = node->next;
    if (!q->head) q->tail = NULL;

    free(node);
    q->size--;
    return true;
}

void pq_init(PriorityQueue *pq) {
    if (!pq) return;
    for (size_t i = 0; i < PQ_LEVELS; i++) {
        fifo_init(&pq->q[i]);
    }
    pq->total_size = 0;
}

void pq_free(PriorityQueue *pq) {
    if (!pq) return;
    for (size_t i = 0; i < PQ_LEVELS; i++) {
        fifo_free(&pq->q[i]);
    }
    pq->total_size = 0;
}

bool pq_push(PriorityQueue *pq, const Message *m) {
    if (!pq || !m) return false;

    if (!fifo_push(&pq->q[m->priority], m)) return false;
    pq->total_size++;
    return true;
}

bool pq_pop_by_priority(PriorityQueue *pq, uint8_t priority, Message *out) {
    if (!pq) return false;

    if (!fifo_pop(&pq->q[priority], out)) return false;
    pq->total_size--;
    return true;
}

bool pq_pop_first(PriorityQueue *pq, Message *out) {
    if (!pq || pq->total_size == 0) return false;

    /* 255 самый высокий приоритет */
    for (int pr = (int)PQ_PRIO_MAX; pr >= (int)PQ_PRIO_MIN; pr--) {
        if (pq->q[pr].size > 0) {
            bool ok = fifo_pop(&pq->q[pr], out);
            if (ok) pq->total_size--;
            return ok;
        }
    }
    return false;
}

bool pq_pop_not_below(PriorityQueue *pq, uint8_t min_priority, Message *out) {
    if (!pq || pq->total_size == 0) return false;

    for (int pr = (int)PQ_PRIO_MAX; pr >= (int)min_priority; pr--) {
        if (pq->q[pr].size > 0) {
            bool ok = fifo_pop(&pq->q[pr], out);
            if (ok) pq->total_size--;
            return ok;
        }
    }
    return false;
}

size_t pq_size(const PriorityQueue *pq) {
    return pq ? pq->total_size : 0;
}

bool pq_is_empty(const PriorityQueue *pq) {
    return pq ? (pq->total_size == 0) : true;
}
