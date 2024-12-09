#include "ipc.h"

struct Request {
    local_id loc_pid;
    timestamp_t req_time;
};

struct RequestQueue {
    struct Request heap[MAX_PROCESS_ID];
    local_id size;
};

void clear_queue(struct RequestQueue *queue) {
    queue->size = 0;
}

struct Request get_head(const struct RequestQueue *queue) {
    if (queue->size > 0) {
        return queue->heap[0];
    }
    struct Request empty_request = {0};
    return empty_request;
}

int8_t requests_compare(struct Request first, struct Request second) {
    if (first.req_time < second.req_time) {
        return -1;
    } else if (first.req_time > second.req_time) {
        return 1;
    }
    return 0;
}

void pop_head(struct RequestQueue *queue) {
    if (queue->size > 0) {
        for (local_id i = 0; i < queue->size - 1; ++i) {
            queue->heap[i] = queue->heap[i + 1];
        }
        queue->size--;
    }
}

void push_request(struct RequestQueue *queue, struct Request request) {
    if (queue->size < MAX_PROCESS_ID) {
        queue->heap[queue->size] = request;
        queue->size++;
    }
}

void sort_queue(struct RequestQueue *queue) {
    for (local_id i = 0; i < queue->size - 1; ++i) {
        for (local_id j = i + 1; j < queue->size; ++j) {
            if (requests_compare(queue->heap[i], queue->heap[j]) > 0) {
                struct Request temp = queue->heap[i];
                queue->heap[i] = queue->heap[j];
                queue->heap[j] = temp;
            }
        }
    }
}
