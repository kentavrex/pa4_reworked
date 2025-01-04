#include "util.h"
#include "pipes_manager.h"
#include "cs.h"
#include <fcntl.h>
#include <unistd.h>


static timestamp_t lamport_time = 0;


timestamp_t get_lamport_time(void) {
    return lamport_time;
}


timestamp_t increment_lamport_time(void) {
    lamport_time += 1;
    return lamport_time;
}


void update_lamport_time(timestamp_t received_time) {
    if (received_time > lamport_time) {
        lamport_time = received_time;
    }
    lamport_time += 1; 
}


void handle_done(Process *proc, FILE *log_file, int *has_sent_done) {
    send_message(proc, DONE);
    printf(log_done_fmt, get_lamport_time(), proc->pid, 0);
    fprintf(log_file, log_done_fmt, get_lamport_time(), proc->pid, 0);
    *has_sent_done = 1;
}


void handle_cs_request(Process *proc, int *has_sent_request) {
    initiate_cs_request(proc);
    *has_sent_request = 1;
}


void handle_critical_section(Process *proc, int *operation_counter, int *has_sent_request, int *reply_count) {
    char log_message[100];
    snprintf(log_message, sizeof(log_message), log_loop_operation_fmt, proc->pid, *operation_counter, proc->pid * 5);
    print(log_message);
    (*operation_counter)++;
    finalize_cs_release(proc);
    *has_sent_request = 0;
    *reply_count = 0;
}


void handle_request(Process *proc, local_id src_id, const Message *incoming_msg) {
    Query new_request = {.pid = src_id, .time = incoming_msg->s_header.s_local_time};
    enqueue_request(proc, new_request);

    Message reply_msg = {
        .s_header = {
            .s_magic = MESSAGE_MAGIC,
            .s_local_time = increment_lamport_time(),
            .s_type = CS_REPLY,
            .s_payload_len = 0
        }
    };

    if (send(proc, new_request.pid, &reply_msg) != 0) {
        fprintf(stderr, "Error: Unable to send CS_REPLY from process %d to process %d\n", proc->pid, src_id);
        exit(EXIT_FAILURE);
    }
}


void handle_reply(int *reply_count) {
    (*reply_count)++;
}


void handle_release(Process *proc) {
    dequeue_request(proc);
}


void handle_done2(int *completed_processes) {
    (*completed_processes)++;
}


void process_message(Process *proc, local_id src_id, Message *incoming_msg, int *reply_count, int *completed_processes) {
    update_lamport_time(incoming_msg->s_header.s_local_time);

    switch (incoming_msg->s_header.s_type) {
        case CS_REQUEST:
            handle_request(proc, src_id, incoming_msg);
        break;
        case CS_REPLY:
            handle_reply(reply_count);
        break;
        case CS_RELEASE:
            handle_release(proc);
        break;
        case DONE:
            handle_done2(completed_processes);
        break;
        default:
            fprintf(stderr, "Warning: Received unknown message type from process %d\n", src_id);
        break;
    }
}


void process_incoming_message(Process *proc, int *reply_count, int *completed_processes) {
    Message incoming_msg;
    for (local_id src_id = 0; src_id < proc->num_process; src_id++) {
        if (src_id != proc->pid && !receive(proc, src_id, &incoming_msg)) {
            process_message(proc, src_id, &incoming_msg, reply_count, completed_processes);
        }
    }
}


void check_all_done(Process *proc, FILE *log_file, int *completed_processes, int *has_sent_done, int operation_counter) {
    if (*has_sent_done && *completed_processes == proc->num_process - 2 && operation_counter > proc->pid * 5) {
        printf(log_received_all_done_fmt, get_lamport_time(), proc->pid);
        fprintf(log_file, log_received_all_done_fmt, get_lamport_time(), proc->pid);
        exit(0);
    }
}

void send_done_if_needed(Process *proc, FILE *log_file, int operation_counter, int *has_sent_done, int *has_sent_request) {
    if (!(*has_sent_done) && operation_counter > proc->pid * 5) {
        handle_done(proc, log_file, has_sent_done);
        *has_sent_request = 0;
    }
}

void send_request_if_needed(Process *proc, int operation_counter, int *has_sent_request) {
    if (operation_counter <= proc->pid * 5 && !(*has_sent_request)) {
        handle_cs_request(proc, has_sent_request);
    }
}

void execute_critical_section_if_ready(Process *proc, int *operation_counter, int *has_sent_request, int *reply_count) {
    if (*has_sent_request && proc->queue[0].pid == proc->pid && *reply_count == proc->num_process - 2) {
        handle_critical_section(proc, operation_counter, has_sent_request, reply_count);
    }
}

void bank_operations(Process *proc, FILE *log_file) {
    int completed_processes = 0;
    int operation_counter = 1;
    int has_sent_request = 0;
    int has_sent_done = 0;
    int reply_count = 0;

    while (1) {
        // Проверяем, все ли процессы завершили работу
        check_all_done(proc, log_file, &completed_processes, &has_sent_done, operation_counter);

        // Отправляем сообщение DONE, если выполнение завершено
        send_done_if_needed(proc, log_file, operation_counter, &has_sent_done, &has_sent_request);

        // Отправляем запрос на вход в критическую секцию, если нужно
        send_request_if_needed(proc, operation_counter, &has_sent_request);

        // Выполняем критическую секцию, если готовы
        execute_critical_section_if_ready(proc, &operation_counter, &has_sent_request, &reply_count);

        // Обрабатываем входящие сообщения
        process_incoming_message(proc, &reply_count, &completed_processes);
    }
}


void close_non_related_pipes(Process* pipes, FILE* pipe_file_ptr) {
    int n = pipes->num_process;

    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {

            if (i != j) {
                if (i != pipes->pid && j != pipes->pid) {

                    close(pipes->pipes[i][j].fd[READ]);
                    close(pipes->pipes[i][j].fd[WRITE]);
                    fprintf(pipe_file_ptr, "Closed full pipe from %d to %d, write fd: %d, read fd: %d.\n",
                            i, j, pipes->pipes[i][j].fd[WRITE], pipes->pipes[i][j].fd[READ]);
                } 

                else if (i == pipes->pid && j != pipes->pid) {
                    close(pipes->pipes[i][j].fd[READ]);
                    fprintf(pipe_file_ptr, "Closed read end from %d to %d, read fd: %d.\n",
                            i, j, pipes->pipes[i][j].fd[READ]);
                }

                else if (j == pipes->pid && i != pipes->pid) {
                    close(pipes->pipes[i][j].fd[WRITE]);
                    fprintf(pipe_file_ptr, "Closed write end from %d to %d, write fd: %d.\n",
                            i, j, pipes->pipes[i][j].fd[WRITE]);
                }
            }
        }
    }
}


void close_outcoming_pipes(Process* processes, FILE* pipe_file_ptr) {
    int pid = processes->pid;

    for (int target = 0; target < processes->num_process; target++) {
        if (target == pid) continue;

        
        close(processes->pipes[pid][target].fd[READ]);
        close(processes->pipes[pid][target].fd[WRITE]);

        
        fprintf(pipe_file_ptr, "Closed outgoing pipe from %d to %d, write fd: %d, read fd: %d.\n",
                pid, target, processes->pipes[pid][target].fd[WRITE], processes->pipes[pid][target].fd[READ]);
    }
}

void close_incoming_pipes(Process* processes, FILE* pipe_file_ptr) {
    int pid = processes->pid;

    for (int source = 0; source < processes->num_process; source++) {
        if (source == pid) continue;


        close(processes->pipes[source][pid].fd[READ]);
        close(processes->pipes[source][pid].fd[WRITE]);


        fprintf(pipe_file_ptr, "Closed incoming pipe from %d to %d, write fd: %d, read fd: %d.\n",
                source, pid, processes->pipes[source][pid].fd[WRITE], processes->pipes[source][pid].fd[READ]);
    }
}

int prepare_message(Message *msg, timestamp_t current_time, Process *proc, MessageType msg_type, const char *log_fmt) {
    if (!msg || !proc || !log_fmt) {
        fprintf(stderr, "[ERROR] Null pointer passed to prepare_message.\n");
        return -1;
    }

    msg->s_header.s_local_time = current_time;
    msg->s_header.s_magic = MESSAGE_MAGIC;
    msg->s_header.s_type = msg_type;

    int payload_size = snprintf(msg->s_payload, sizeof(msg->s_payload), log_fmt,
                                current_time, proc->pid, getpid(), getppid(), 0);
    if (payload_size < 0) {
        fprintf(stderr, "[ERROR] Failed to format message payload.\n");
        return -1;
    }

    msg->s_header.s_payload_len = payload_size;
    return 0;
}

int multicast_message(Process *proc, Message *msg) {
    if (!proc || !msg) {
        fprintf(stderr, "[ERROR] Null pointer passed to multicast_message.\n");
        return -1;
    }

    increment_lamport_time();
    if (send_multicast(proc, msg) != 0) {
        fprintf(stderr, "[ERROR] Failed to multicast message from process %d.\n", proc->pid);
        return -1;
    }

    return 0;
}

int prepare_and_send_message(Process *proc, MessageType msg_type, timestamp_t current_time, const char *format) {
    Message msg;
    if (prepare_message(&msg, current_time, proc, msg_type, format) != 0) {
        return -1;
    }
    if (multicast_message(proc, &msg) != 0) {
        return -1;
    }
    return 0;
}

int handle_message_type(MessageType msg_type, timestamp_t current_time, Process *proc) {
    switch (msg_type) {
        case STARTED:
            return prepare_and_send_message(proc, msg_type, current_time, log_started_fmt);

        case DONE:
            return prepare_and_send_message(proc, msg_type, current_time, log_done_fmt);

        default:
            fprintf(stderr, "[ERROR] Unsupported message type: %d\n", msg_type);
        return -1;
    }
}

int send_message(Process *proc, MessageType msg_type) {
    if (!proc) {
        fprintf(stderr, "[ERROR] Process pointer is NULL.\n");
        return -1;
    }

    if (msg_type < STARTED || msg_type > BALANCE_HISTORY) {
        fprintf(stderr, "[ERROR] Invalid message type: %d\n", msg_type);
        return -1;
    }

    timestamp_t current_time = increment_lamport_time();

    return handle_message_type(msg_type, current_time, proc);
}

int receive_message(Process* process, int i, Message* msg) {
    while (receive(process, i, msg)) {}
    return msg->s_header.s_type;
}

void process_message2(Process* process, Message* msg, int* count, MessageType type) {
    if (msg->s_header.s_type == type) {
        update_lamport_time(msg->s_header.s_local_time);
        (*count)++;
        printf("Process %d readed %d messages with type %s\n",
            process->pid, *count, type == 0 ? "STARTED" : "DONE");
    }
}

int check_received_for_pid(Process* process, MessageType type, int* count) {
    for (int i = 1; i < process->num_process; i++) {
        if (i != process->pid) {
            Message msg;
            int msg_type = receive_message(process, i, &msg);
            if (msg_type == type) {
                process_message2(process, &msg, count, type);
            }
        }
    }
    return *count;
}

int check_all_received(Process* process, MessageType type) {
    int count = 0;
    count = check_received_for_pid(process, type, &count);

    if ((process->pid != 0 && count == process->num_process - 2) ||
        (process->pid == 0 && count == process->num_process - 1)) {
        return 0;
        }
    return -1;
}

static int set_pipe_nonblocking(int pipe_fd[2]) {
    int flags_read = fcntl(pipe_fd[READ], F_GETFL);
    int flags_write = fcntl(pipe_fd[WRITE], F_GETFL);

    if (flags_read == -1 || flags_write == -1) {
        perror("Error retrieving flags for pipe");
        return ERR;
    }

    if (fcntl(pipe_fd[READ], F_SETFL, flags_read | O_NONBLOCK) == -1) {
        perror("Failed to set non-blocking mode for read end of pipe");
        return ERR;
    }

    if (fcntl(pipe_fd[WRITE], F_SETFL, flags_write | O_NONBLOCK) == -1) {
        perror("Failed to set non-blocking mode for write end of pipe");
        return ERR;
    }

    return OK;
}

static void log_pipe_initialization(FILE* log_fp, int src, int dest, int fd_write, int fd_read) {
    fprintf(log_fp, "Pipe initialized: from process %d to process %d (write: %d, read: %d)\n",
            src, dest, fd_write, fd_read);
}

static int create_pipe(Pipe* pipe_obj) {
    if (pipe(pipe_obj->fd) != 0) {
        perror("Pipe creation failed");
        return ERR;
    }

    return set_pipe_nonblocking(pipe_obj->fd);
}

static void initialize_pipes_for_processes(Pipe** pipes, int process_count, FILE* log_fp) {
    for (int src = 0; src < process_count; src++) {
        for (int dest = 0; dest < process_count; dest++) {
            if (src == dest) {
                continue;
            }

            if (create_pipe(&pipes[src][dest]) != OK) {
                exit(EXIT_FAILURE);
            }

            log_pipe_initialization(log_fp, src, dest, pipes[src][dest].fd[WRITE], pipes[src][dest].fd[READ]);
        }
    }
}

Pipe** init_pipes(int process_count, FILE* log_fp) {
    Pipe** pipes = (Pipe**) malloc(process_count * sizeof(Pipe*));
    for (int i = 0; i < process_count; i++) {
        pipes[i] = (Pipe*) malloc(process_count * sizeof(Pipe));
    }

    initialize_pipes_for_processes(pipes, process_count, log_fp);
    return pipes;
}
