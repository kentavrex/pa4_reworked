#include "ipc.h"

struct Request {
    local_id loc_pid;
    timestamp_t req_time;
};

struct RequestQueue {
    struct Request heap[MAX_PROCESS_ID];
    local_id size;
};

void clear_queue(struct RequestQueue *queue);
struct Request get_head(const struct RequestQueue *queue);
int8_t requests_compare(struct Request first, struct Request second);
void pop_head(struct RequestQueue *queue);
void push_request(struct RequestQueue *queue, struct Request request);
