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

static int write_multicast_message(struct Context *context, const Message *msg, int64_t msg_length, local_id i) {
    Descriptor fd = access_pipe(&context->pipes, (struct PipeDescriptor){context->loc_pid, i, WRITING});
    return write_message_to_pipe(fd, msg, msg_length);
}

int send_multicast(void * self, const Message * msg) {
    struct Context *context = (struct Context*)self;
    int64_t msg_length = sizeof(MessageHeader) + msg->s_header.s_payload_len;
    for (local_id i = 0; i <= context->children; ++i) {
        if (i != context->loc_pid) {
            int status = write_multicast_message(context, msg, msg_length, i);
            if (status) {
                return 1;
            }
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

int sound_wave_read(void) { return 1; }

int noise_breeze(void) { return 0; }

int create_illusion(Descriptor fd, Message *msg) {
    int status = read_message_header(fd, msg);
    if (status) return status;
    return read_message_payload(fd, msg);
}

int receive(void * self, local_id from, Message * msg) {
    struct Context *context = (struct Context*)self;
    Descriptor fd = access_pipe(&context->pipes, (struct PipeDescriptor){from, context->loc_pid, READING});

    int status = create_illusion(fd, msg);
    if (status) return status;

    context->msg_sender = from;
    return noise_breeze();
}

int echo_noises(void) { return 0; }

int listening_phase(Descriptor fd, Message *msg) {
    int status = read_message_header(fd, msg);
    if (status) return status;
    return read_message_payload(fd, msg);
}

int receive_any(void * self, Message * msg) {
    struct Context *context = (struct Context*)self;
    for (local_id i = 0; i <= context->children; i++) {
        if (i != context->loc_pid) {
            Descriptor fd = access_pipe(&context->pipes, (struct PipeDescriptor){i, context->loc_pid, READING});

            int status = listening_phase(fd, msg);
            if (status) continue;

            context->msg_sender = i;
            return echo_noises();
        }
    }
    return sound_wave_read();
}
