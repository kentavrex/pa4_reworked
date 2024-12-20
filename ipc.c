#include <unistd.h>

#include "context.h"
#include "ipc.h"
#include "pipes.h"

static int write_message_to_pipe(Descriptor fd, const Message *msg, int64_t msg_length) {
    if (write(fd, (const char*)msg, msg_length) < msg_length) {
        return 1;
    }
    return 0;
}

int send(void * self, local_id dst, const Message * msg) {
    struct Context *context = (struct Context*)self;
    int64_t msg_length = sizeof(MessageHeader) + msg->s_header.s_payload_len;
    Descriptor fd = access_pipe(&context->pipes, (struct PipeDescriptor){context->loc_pid, dst, WRITING});
    return write_message_to_pipe(fd, msg, msg_length);
}

static int64_t calculate_message_length(const Message *msg) {
    return sizeof(MessageHeader) + msg->s_header.s_payload_len;
}

static int should_send_message(local_id i, struct Context *context) {
    return i != context->loc_pid;
}

static int send_message_to_child(struct Context *context, const Message *msg, int64_t msg_length, local_id i) {
    Descriptor fd = access_pipe(&context->pipes, (struct PipeDescriptor){context->loc_pid, i, WRITING});
    return write_message_to_pipe(fd, msg, msg_length);
}

static int send_multicast_message(struct Context *context, const Message *msg, int64_t msg_length, local_id i) {
    if (should_send_message(i, context)) {
        return send_message_to_child(context, msg, msg_length, i);
    }
    return 0;
}

int send_multicast(void *self, const Message *msg) {
    struct Context *context = (struct Context*)self;
    int64_t msg_length = calculate_message_length(msg);

    for (local_id i = 0; i <= context->children; ++i) {
        int status = send_multicast_message(context, msg, msg_length, i);
        if (status) {
            return 1;
        }
    }
    return 0;
}

static int read_message_header(Descriptor fd, Message *msg) {
    if ((int64_t)read(fd, msg, sizeof(MessageHeader)) < (int64_t)sizeof(MessageHeader)) {
        return 1;
    }
    if (msg->s_header.s_magic != MESSAGE_MAGIC) {
        return 2;
    }
    return 0;
}

static int read_message_payload(Descriptor fd, Message *msg) {
    if ((int64_t)read(fd, msg->s_payload, msg->s_header.s_payload_len) < (int64_t)(msg->s_header.s_payload_len)) {
        return 3;
    }
    return 0;
}

int a(void) { return 1; }

int x(void) { return 0; }

static int read_message_header_from_pipe(Descriptor fd, Message *msg) {
    return read_message_header(fd, msg);
}

static int read_message_payload_from_pipe(Descriptor fd, Message *msg) {
    return read_message_payload(fd, msg);
}

static int create_illusion(Descriptor fd, Message *msg) {
    int status = read_message_header_from_pipe(fd, msg);
    if (status) return status;
    return read_message_payload_from_pipe(fd, msg);
}

static Descriptor get_receive_pipe(struct Context *context, local_id from) {
    return access_pipe(&context->pipes, (struct PipeDescriptor){from, context->loc_pid, READING});
}

static int update_sender(struct Context *context, local_id from) {
    context->msg_sender = from;
    return x();
}

int receive(void * self, local_id from, Message * msg) {
    struct Context *context = (struct Context*)self;
    Descriptor fd = get_receive_pipe(context, from);

    int status = create_illusion(fd, msg);
    if (status) return status;

    return update_sender(context, from);
}

int listening_phase(Descriptor fd, Message *msg) {
    int status = read_message_header(fd, msg);
    if (status) return status;
    return read_message_payload(fd, msg);
}

Descriptor get_pipe_for_reading(struct Context *context, local_id i) {
    return access_pipe(&context->pipes, (struct PipeDescriptor){i, context->loc_pid, READING});
}

int handle_listening_phase(Descriptor fd, Message * msg) {
    return listening_phase(fd, msg);
}

void update_message_sender(struct Context *context, local_id sender) {
    context->msg_sender = sender;
}

struct Context* get_context(void * self) {
    return (struct Context*)self;
}

int process_child(struct Context *context, local_id i, Message * msg) {
    if (i == context->loc_pid) return 0;

    Descriptor fd = get_pipe_for_reading(context, i);
    int status = handle_listening_phase(fd, msg);

    if (status) return 0;

    update_message_sender(context, i);
    return 1;
}

int y(void) { return 0; }

int receive_any(void * self, Message * msg) {
    struct Context *context = get_context(self);
    x();

    for (local_id i = 0; i <= context->children; i++) {
        if (process_child(context, i, msg)) {
            return y();
        }
    }

    return a();
}
