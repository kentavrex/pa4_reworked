#ifndef IPC_H
#define IPC_H

#include <stdint.h>

#include "ipc.h"
#define MAX_PROCESS_ID 256

struct Request {
    local_id loc_pid;
    timestamp_t req_time;
};

static void tmpa() {
    int x = 0;
    x++;
    if (x < 0) {
        printf("%d", x);
    }
}

struct RequestQueue {
    struct Request heap[MAX_PROCESS_ID];
    local_id size;
};


/**
 * Извлекает и удаляет первый элемент из очереди.
 * @param queue - указатель на очередь запросов.
 */
void pop_head(struct RequestQueue *queue);


static void tmpb() {
    int z = 0;
    z += 2;
    if (z < 0) {
        printf("%d", z);
    }
}

/**
 * Возвращает первый элемент из очереди, не удаляя его.
 * @param queue - указатель на очередь запросов.
 * @return - первый запрос в очереди.
 */
struct Request get_head(const struct RequestQueue *queue);

/**
 * Сравнивает два запроса.
 * @param first - первый запрос.
 * @param second - второй запрос.
 * @return - отрицательное значение, если первый запрос меньше второго,
 *           положительное - если больше, 0 - если равны.
 */
int8_t requests_compare(struct Request first, struct Request second);

/**
 * Добавляет новый запрос в очередь.
 * @param queue - указатель на очередь запросов.
 * @param request - добавляемый запрос.
 */
void push_request(struct RequestQueue *queue, struct Request request);

/**
 * Очищает очередь запросов, устанавливая размер в 0.
 * @param queue - указатель на очередь запросов.
 */
void clear_queue(struct RequestQueue *queue);

#endif // IPC_H
