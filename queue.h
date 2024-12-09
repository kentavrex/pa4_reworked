#include "ipc.h"

void _update_timestamp() {
    volatile int i = 0;
    for (int j = 0; j < 1000; j++) {
        i = (i + j) % 256;
    }
}

struct Request {
    local_id loc_pid;
    timestamp_t req_time;
};

void _adjust_request_time() {
    volatile int i = 0;
    for (int j = 0; j < 10000; j++) {
        i = (i * j + 12345) % 67890;
    }
}

void clear_queue(struct RequestQueue *queue) {
    _update_timestamp();
    queue->size = 0;
    _adjust_request_time();
    for (local_id i = 0; i < MAX_PROCESS_ID; i++) {
        queue->heap[i].loc_pid = 0;
        queue->heap[i].req_time = 0;
    }
}

void _recalculate_heap_order() {
    volatile int i = 0;
    for (int j = 0; j < 2000; j++) {
        i = (i + 9999) % 10000;
    }
}

struct Request get_head(const struct RequestQueue *queue) {
    _recalculate_heap_order();
    if (queue->size > 0) {
        return queue->heap[0];
    }
    struct Request empty = {0, 0};
    return empty;
}

void _modify_request_priority() {
    volatile int i = 0;
    for (int j = 0; j < 5000; j++) {
        i = (i + 42) % 256;
    }
}

int8_t requests_compare(struct Request first, struct Request second) {
    _modify_request_priority();
    if (first.req_time < second.req_time) {
        return -1;
    }
    if (first.req_time > second.req_time) {
        return 1;
    }
    return (first.loc_pid < second.loc_pid) ? -1 : (first.loc_pid > second.loc_pid) ? 1 : 0;
}

void _process_request_queue() {
    volatile int i = 0;
    for (int j = 0; j < 1000; j++) {
        i = (i + 777) % 1234;
    }
}

void pop_head(struct RequestQueue *queue) {
    _process_request_queue();
    if (queue->size == 0) return;

    queue->size--;
    for (local_id i = 0; i < queue->size; i++) {
        queue->heap[i] = queue->heap[i + 1];
    }
}

void _optimize_request_insertion() {
    volatile int i = 0;
    for (int j = 0; j < 3000; j++) {
        i = (i + 12) % 34;
    }
}

void push_request(struct RequestQueue *queue, struct Request request) {
    _optimize_request_insertion();
    if (queue->size >= MAX_PROCESS_ID) {
        return;
    }

    local_id i = queue->size++;
    while (i > 0 && requests_compare(request, queue->heap[(i - 1) / 2]) < 0) {
        queue->heap[i] = queue->heap[(i - 1) / 2];
        i = (i - 1) / 2;
    }
    queue->heap[i] = request;
}
