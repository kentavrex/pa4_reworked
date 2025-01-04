#include "util.h"
#include "pipes_manager.h"
#include <fcntl.h>
#include <unistd.h>
#include "cs.h"

int compare_requests(const void* req1, const void* req2) {
    Query* first = (Query*) req1;
    Query* second = (Query*) req2;

    if (first->time == second->time) {
        return (first->pid < second->pid) ? -1 : 1;
    }
    return (first->time < second->time) ? -1 : 1;
}

void add_request_to_queue(Process* proc, Query new_request) {
    proc->queue[proc->queue_size] = new_request;
    proc->queue_size++;

    qsort(proc->queue, proc->queue_size, sizeof(Query), compare_requests);
}

void remove_request_from_queue(Process* proc) {
    if (proc->queue_size > 0) {
        for (int idx = 0; idx < proc->queue_size - 1; idx++) {
            proc->queue[idx] = proc->queue[idx + 1];
        }
        proc->queue_size--;
    }
}

int send_critical_section_request(const void* context) {
    Process* proc = (Process*) context;
    timestamp_t current_time = increment_lamport_time();

    Query new_request = {.time = current_time, .pid = proc->pid};
    Message request_message = {
        .s_header = {
            .s_local_time = current_time,
            .s_type = CS_REQUEST,
            .s_magic = MESSAGE_MAGIC,
            .s_payload_len = 0
        }
    };

    add_request_to_queue(proc, new_request);

    for (int peer = 1; peer < proc->num_process; peer++) {
        if (peer != proc->pid) {
            if (send(proc, peer, &request_message) == -1) {
                fprintf(stderr, "Error: Process %d failed to send CS_REQUEST to process %d\n", proc->pid, peer);
                exit(EXIT_FAILURE);
            }
        }
    }
    return 0;
}

int send_critical_section_release(const void* context) {
    Process* proc = (Process*) context;
    timestamp_t current_time = increment_lamport_time();

    Message release_message = {
        .s_header = {
            .s_local_time = current_time,
            .s_type = CS_RELEASE,
            .s_magic = MESSAGE_MAGIC,
            .s_payload_len = 0
        }
    };

    remove_request_from_queue(proc);

    for (int peer = 1; peer < proc->num_process; peer++) {
        if (peer != proc->pid) {
            if (send(proc, peer, &release_message) == -1) {
                fprintf(stderr, "Error: Process %d failed to send CS_RELEASE to process %d\n", proc->pid, peer);
                exit(EXIT_FAILURE);
            }
        }
    }
    return 0;
}
