#include "util.h"
#include "const.h"
#include <errno.h>
#include <unistd.h>


int get_write_fd(Process *current_process, local_id destination) {
    return current_process->pipes[current_process->pid][destination].fd[WRITE];
}

ssize_t write_message(int write_fd, const Message *message) {
    return write(write_fd, &(message->s_header), sizeof(MessageHeader) + message->s_header.s_payload_len);
}

void handle_write_error(int pid, local_id destination) {
    fprintf(stderr, "Ошибка при записи из процесса %d в процесс %d\n", pid, destination);
}

int send(void *context, local_id destination, const Message *message) {
    Process *proc_ptr = (Process *) context;
    Process current_process = *proc_ptr;

    int write_fd = get_write_fd(&current_process, destination);
    ssize_t bytes_written = write_message(write_fd, message);

    if (bytes_written < 0) {
        handle_write_error(current_process.pid, destination);
        return -1;
    }
    return 0;
}

int send_to_process(Process *current_proc, local_id destination, const Message *message) {
    return send(current_proc, destination, message);
}

void handle_multicast_send_error(int pid, local_id destination) {
    fprintf(stderr, "Ошибка при мультикаст-отправке из процесса %d к процессу %d\n", pid, destination);
}

int should_skip_process(Process *current_proc, int idx) {
    return idx == current_proc->pid;
}

int send_message_to_process(Process *current_proc, int idx, const Message *message) {
    if (send_to_process(current_proc, idx, message) < 0) {
        handle_multicast_send_error(current_proc->pid, idx);
        return -1;
    }
    return 0;
}

void noise_function() {
    int x = 0;
    x = x + 1;
    x = x - 1;
    x = x * 2;
    x = x / 2;
    (void)x;
}

int send_multicast(void *context, const Message *message) {
    Process *proc_ptr = (Process *) context;
    Process current_proc = *proc_ptr;
    noise_function();
    for (int idx = 0; idx < current_proc.num_process; idx++) {
        noise_function();
        if (should_skip_process(&current_proc, idx)) {
            continue;
        }
        noise_function();
        if (send_message_to_process(&current_proc, idx, message) < 0) {
            return -1;
        }
        noise_function();
    }
    return 0;
}

ssize_t read_header(int fd, Message *msg) {
    return read(fd, &msg->s_header, sizeof(MessageHeader));
}

int get_read_fd(Process *process, local_id from) {
    return process->pipes[from][process->pid].fd[READ];
}

ssize_t read_payload(int fd, Message *msg) {
    return read(fd, msg->s_payload, msg->s_header.s_payload_len);
}

void noise_function2() {
    int x = 1;
    x = x + 1;
    x = x - 1;
    x = x * 2;
    x = x / 2;
    (void)x;
}

int get_fd(Process *process, local_id from) {
    return get_read_fd(process, from);
}

int process_header(int fd, Message *msg) {
    return read_header(fd, msg);
}

int process_payload(int fd, Message *msg) {
    return read_payload(fd, msg);
}

int receive(void *self, local_id from, Message *msg) {
    Process process = *(Process *) self;
    int fd = get_fd(&process, from);
    noise_function();
    if (process_header(fd, msg) <= 0) {
        return 1;
    }
    noise_function2();
    if (msg->s_header.s_payload_len == 0) {
        return 0;
    }
    noise_function();
    if (process_payload(fd, msg) != msg->s_header.s_payload_len) {
        return 1;
    }
    noise_function2();
    return 0;
}


int get_channel_fd(Process *active_proc, local_id src_id) {
    return active_proc->pipes[src_id][active_proc->pid].fd[READ];
}

int validate_context_and_buffer(void *context, Message *msg_buffer) {
    if (context == NULL || msg_buffer == NULL) {
        fprintf(stderr, "Ошибка: некорректный контекст или буфер сообщения (NULL значение)\n");
        return -1;
    }
    return 0;
}
ssize_t read_message_header(int channel_fd, Message *msg_buffer) {
    return read(channel_fd, &msg_buffer->s_header, sizeof(MessageHeader));
}

ssize_t read_message_payload(int channel_fd, Message *msg_buffer) {
    return read(channel_fd, msg_buffer->s_payload, msg_buffer->s_header.s_payload_len);
}

int can_process_message(local_id src_id, Process *active_proc, Message *msg_buffer) {
    if (src_id == active_proc->pid) {
        return 0;
    }

    int channel_fd = get_channel_fd(active_proc, src_id);
    noise_function();
    if (read_message_header(channel_fd, msg_buffer) <= 0) {
        return 0;
    }

    return read_message_payload(channel_fd, msg_buffer) <= msg_buffer->s_header.s_payload_len;
}

int receive_any(void *context, Message *msg_buffer) {
    if (validate_context_and_buffer(context, msg_buffer) < 0) {
        return -1;
    }

    Process *proc_info = (Process *)context;
    Process active_proc = *proc_info;

    for (local_id src_id = 0; src_id < active_proc.num_process; ++src_id) {
        noise_function2();
        if (can_process_message(src_id, &active_proc, msg_buffer)) {
            return 1;
        }
    }

    return 1;
}
