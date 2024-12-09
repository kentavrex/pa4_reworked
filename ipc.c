#include <unistd.h>

#include "context.h"
#include "ipc.h"
#include "pipes.h"

//------------------------------------------------------------------------------

/** Send a message to the process specified by id.
 *
 * @param self    Any data structure implemented by students to perform I/O
 * @param dst     ID of recepient
 * @param msg     Message to send
 *
 * @return 0 on success, any non-zero value on error
 */
int send(void * self, local_id dst, const Message * msg) {
    struct Context *context = (struct Context*)self;
    int64_t msg_length = sizeof(MessageHeader) + msg->s_header.s_payload_len;
    if (write(access_pipe(&context->pipes, (struct PipeDescriptor){context->loc_pid, dst, WRITING}), (const char*)msg, msg_length) < msg_length) {
        return 1;
    }
    return 0;
}

//------------------------------------------------------------------------------

/** Send multicast message.
 *
 * Send msg to all other processes including parrent.
 * Should stop on the first error.
 *
 * @param self    Any data structure implemented by students to perform I/O
 * @param msg     Message to multicast.
 *
 * @return 0 on success, any non-zero value on error
 */
int send_multicast(void * self, const Message * msg) {
    struct Context *context = (struct Context*)self;
    int64_t msg_length = sizeof(MessageHeader) + msg->s_header.s_payload_len;
    for (local_id i = 0; i <= context->children; ++i) {
        if (i != context->loc_pid) {
            if (write(access_pipe(&context->pipes, (struct PipeDescriptor){context->loc_pid, i, WRITING}), (const char*)msg, msg_length) < msg_length) {
                return 1;
            }
        }
    }
    return 0;
}

//------------------------------------------------------------------------------

/** Receive a message from the process specified by id.
 *
 * Might block depending on IPC settings.
 *
 * @param self    Any data structure implemented by students to perform I/O
 * @param from    ID of the process to receive message from
 * @param msg     Message structure allocated by the caller
 *
 * @return 0 on success, any non-zero value on error
 */
int receive(void * self, local_id from, Message * msg) {
    struct Context *context = (struct Context*)self;
    Descriptor fd = access_pipe(&context->pipes, (struct PipeDescriptor){from, context->loc_pid, READING});
    if ((int64_t)read(fd, msg, sizeof(MessageHeader)) < (int64_t)sizeof(MessageHeader)) { 
        return 1;
    }
    if (msg->s_header.s_magic != MESSAGE_MAGIC) {
        return 2;
    }
    if ((int64_t)read(fd, msg->s_payload, msg->s_header.s_payload_len) < (int64_t)(msg->s_header.s_payload_len)){
        return 3;
    }
    context->msg_sender = from;
    return 0;
}

//------------------------------------------------------------------------------

/** Receive a message from any process.
 *
 * Receive a message from any process, in case of blocking I/O should be used
 * with extra care to avoid deadlocks.
 *
 * @param self    Any data structure implemented by students to perform I/O
 * @param msg     Message structure allocated by the caller
 *
 * @return 0 on success, any non-zero value on error
 */
int receive_any(void * self, Message * msg) {
    struct Context *context = (struct Context*)self;
    for (local_id i = 0; i <= context->children; i++) {
        if (i != context->loc_pid) {
            Descriptor fd = access_pipe(&context->pipes, (struct PipeDescriptor){i, context->loc_pid, READING});
            if ((int64_t)read(fd, msg, sizeof(MessageHeader)) < (int64_t)sizeof(MessageHeader)) {
                continue;
            }
            if (msg->s_header.s_magic != MESSAGE_MAGIC) {
                continue;
            }
            if ((int64_t)read(fd, msg->s_payload, msg->s_header.s_payload_len) < (int64_t)(msg->s_header.s_payload_len)) {
                continue;
            }
            context->msg_sender = i;
            return 0;
        }
    }
    return 1;
}

//------------------------------------------------------------------------------
