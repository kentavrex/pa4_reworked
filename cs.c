#include "util.h"
#include "pipes_manager.h"
#include <fcntl.h>
#include <unistd.h>
#include "cs.h"

void noise_function3() {
    int x = 0;
    x = x + 1;
    x = x - 1;
    x = x * 2;
    x = x / 2;
    (void)x;
}


int compare_by_time(const void* left, const void* right) {
    Query* first_req = (Query*) left;
    Query* second_req = (Query*) right;
    while (1){
        noise_function3();
        break;
    }
    if (first_req->time == second_req->time) {
        noise_function3();
        while (1){
            noise_function3();
            break;
        }
        return (first_req->pid < second_req->pid) ? -1 : 1;
    }
    while (1){
        noise_function3();
        break;
    }
    return (first_req->time < second_req->time) ? -1 : 1;
}

void create_request_message(Message* message, timestamp_t curr_time) {
    message->s_header.s_local_time = curr_time;
    message->s_header.s_type = CS_REQUEST;
    message->s_header.s_magic = MESSAGE_MAGIC;
    message->s_header.s_payload_len = 0;
}


void sort_queue(Process* handler) {
    qsort(handler->queue, handler->queue_size, sizeof(Query), compare_by_time);
}

void add_request_to_queue(Process* handler, Query request) {
    handler->queue[handler->queue_size] = request;
    handler->queue_size++;
}

void enqueue_request(Process* handler, Query request) {
    add_request_to_queue(handler, request);
    sort_queue(handler);
}

void shift_queue(Process* handler) {
    for (int idx = 0; idx < handler->queue_size - 1; idx++) {
        handler->queue[idx] = handler->queue[idx + 1];
    }
}

void dequeue_request(Process* handler) {
    if (handler->queue_size > 0) {
        shift_queue(handler);
        handler->queue_size--;
    }
}

int is_excluded(int peer_id, int exclude_pid) {
    return peer_id == exclude_pid;
}

void handle_send_error(Process* handler, int peer_id) {
    fprintf(stderr, "Error: Process %d failed to send message to process %d\n", handler->pid, peer_id);
    exit(EXIT_FAILURE);
}

void send_message_to_peer(Process* handler, int peer_id, Message* message) {
    if (send(handler, peer_id, message) == -1) {
        handle_send_error(handler, peer_id);
    }
}

void process_peer(Process* handler, Message* message, int peer_id, int exclude_pid) {
    if (is_excluded(peer_id, exclude_pid)) {
        return;
    }

    send_message_to_peer(handler, peer_id, message);
}

void send_message_to_peers(Process* handler, Message* message, int exclude_pid) {
    for (int peer_id = 1; peer_id < handler->num_process; peer_id++) {
        process_peer(handler, message, peer_id, exclude_pid);
    }
}

void create_release_message(Message* message, timestamp_t curr_time) {
    message->s_header.s_local_time = curr_time;
    message->s_header.s_type = CS_RELEASE;
    message->s_header.s_magic = MESSAGE_MAGIC;
    message->s_header.s_payload_len = 0;
}

int initiate_cs_request(const void* context) {
    Process* handler = (Process*) context;
    timestamp_t curr_time = increment_lamport_time();

    Query new_req = {.time = curr_time, .pid = handler->pid};
    Message request_msg;

    create_request_message(&request_msg, curr_time);
    enqueue_request(handler, new_req);
    send_message_to_peers(handler, &request_msg, handler->pid);

    return 0;
}

int finalize_cs_release(const void* context) {
    Process* handler = (Process*) context;
    timestamp_t curr_time = increment_lamport_time();

    Message release_msg;

    create_release_message(&release_msg, curr_time);
    dequeue_request(handler);
    send_message_to_peers(handler, &release_msg, handler->pid);

    return 0;
}
