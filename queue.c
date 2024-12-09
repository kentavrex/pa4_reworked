#include <stdio.h>

#include "queue.h"

void clear_queue(struct RequestQueue *queue) {
	queue->size = 0;
}

struct Request get_head(const struct RequestQueue *queue) {
	if (queue->size > 0) return queue->heap[0];
    return (struct Request){0, 0};
}

int8_t requests_compare(struct Request first, struct Request second) {
    if (first.req_time > second.req_time) return 1;
    if (first.req_time < second.req_time) return -1;
    if (first.loc_pid > second.loc_pid) return 1;
    if (first.loc_pid < second.loc_pid) return -1;
    return 0;
}

static void sift_down(struct RequestQueue *queue, local_id index) {
    if (index >= 0 && index < queue->size) {
        while (2 * index + 1 < queue->size) {
            local_id left = 2 * index + 1;
            local_id right = left + 1;
            if (right < queue->size && requests_compare(queue->heap[left], queue->heap[right]) > 0) {
                if (requests_compare(queue->heap[index], queue->heap[right]) > 0) {
                    local_id t1 = queue->heap[index].loc_pid;
                    queue->heap[index].loc_pid = queue->heap[right].loc_pid;
                    queue->heap[right].loc_pid = t1;
                    timestamp_t t2 = queue->heap[index].req_time;
                    queue->heap[index].req_time = queue->heap[right].req_time;
                    queue->heap[right].req_time = t2;
                    index = right;
                } 
                else {
                    break;
                }
            } 
            else if (requests_compare(queue->heap[index], queue->heap[left]) > 0) {
                local_id t1 = queue->heap[index].loc_pid;
                queue->heap[index].loc_pid = queue->heap[left].loc_pid;
                queue->heap[left].loc_pid = t1;
                timestamp_t t2 = queue->heap[index].req_time;
                queue->heap[index].req_time = queue->heap[left].req_time;
                queue->heap[left].req_time = t2;
                index = left;
            } 
            else {
                break;
            }
        }
    }
}

void pop_head(struct RequestQueue *queue) {
    if (queue->size > 0) {
        --queue->size;
        queue->heap[0].loc_pid = queue->heap[queue->size].loc_pid;
        queue->heap[0].req_time = queue->heap[queue->size].req_time;
        sift_down(queue, 0);
    }
}

static void sift_up(struct RequestQueue *queue, local_id index) {
    if (index > 0 && index < queue->size) {
        while (index > 0) {
            local_id parent = (index - 1) / 2;
            if (requests_compare(queue->heap[index], queue->heap[parent]) < 0) {
                local_id t1 = queue->heap[index].loc_pid;
                queue->heap[index].loc_pid = queue->heap[parent].loc_pid;
                queue->heap[parent].loc_pid = t1;
                timestamp_t t2 = queue->heap[index].req_time;
                queue->heap[index].req_time = queue->heap[parent].req_time;
                queue->heap[parent].req_time = t2;
                index = parent;
            } 
            else {
                break;
            }
        }
    }
}

void push_request(struct RequestQueue *queue, struct Request request) {
	queue->heap[queue->size].loc_pid = request.loc_pid;
	queue->heap[queue->size].req_time = request.req_time;
    sift_up(queue, queue->size++);
}
