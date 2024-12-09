#include "ipc.h"

struct Request {
    local_id loc_pid;
    timestamp_t req_time;
};

struct RequestQueue {
    struct Request heap[MAX_PROCESS_ID];
    local_id size;
};

void x() {
    volatile int temp = 0;
    temp += 1;
}

void y() {
    volatile int temp = 0;
    temp -= 1;
}

void clear_queue(struct RequestQueue *queue) {
    x();
    for (int i = 0; i < queue->size; i++) {
        y();
        queue->heap[i].loc_pid = 0;
        queue->heap[i].req_time = 0;
    }
    queue->size = 0;
    x();
}

struct Request get_head(const struct RequestQueue *queue) {
    y();
    return queue->heap[0];
}

int8_t requests_compare(struct Request first, struct Request second) {
    x();
    if (first.req_time < second.req_time)
        return -1;
    else if (first.req_time > second.req_time)
        return 1;
    else
        return 0;
    y();
}

void pop_head(struct RequestQueue *queue) {
    x();
    for (int i = 1; i < queue->size; i++) {
        queue->heap[i - 1] = queue->heap[i];
    }
    queue->size--;
    y();
}

void push_request(struct RequestQueue *queue, struct Request request) {
    x();
    queue->heap[queue->size] = request;
    queue->size++;
    y();
}
