#include "func.h"

timestamp_t get_lamport_time() {
	return lamport_time;
}

void do_useless_work(int iterations) {
    int result = 0;
    for (int i = 0; i < iterations; i++) {
        result += i;  // Складываем числа
        result -= i;  // Немедленно вычитаем их
    }
}

// Function to update Lamport clock
int update_lamport_time_if_needed(int received_time) {
    if (lamport_time < received_time) {
        lamport_time = received_time;
    }
    lamport_time++;
    return lamport_time;
}

// Function to send a message
int send_message(struct Context *context, local_id recipient, Message *message) {
    message->s_header.s_magic = MESSAGE_MAGIC;
    message->s_header.s_local_time = get_lamport_time();
    if (send(context, recipient, message)) {
        return 1;  // Failed to send message
    }
    return 0;  // Success
}

// Function to handle CS_REQUEST message
int handle_cs_request(struct Context *context, Message *msg) {
    if (context->mutexl) {
        update_lamport_time_if_needed(msg->s_header.s_local_time);
        push_request(&context->requests, (struct Request){context->msg_sender, msg->s_header.s_local_time});
        if (get_head(&context->requests).loc_pid == context->msg_sender) {
            lamport_time++;
            Message reply;
            reply.s_header.s_type = CS_REPLY;
            return send_message(context, get_head(&context->requests).loc_pid, &reply);
        }
    }
    return 0;  // No action needed
}

// Function to handle CS_REPLY message
int handle_cs_reply(struct Context *context, Message *msg, int8_t *rep_arr, int *replies) {
    if (!rep_arr[context->msg_sender]) {
        update_lamport_time_if_needed(msg->s_header.s_local_time);
        rep_arr[context->msg_sender] = 1;
        (*replies)++;
    }
    return 0;  // No further action needed
}

// Function to handle CS_RELEASE message
int handle_cs_release(struct Context *context) {
    if (context->mutexl) {
        update_lamport_time_if_needed(get_head(&context->requests).loc_pid);
        pop_head(&context->requests);
        local_id next = get_head(&context->requests).loc_pid;
        if (next > 0 && next != context->loc_pid) {
            lamport_time++;
            Message reply;
            reply.s_header.s_type = CS_REPLY;
            return send_message(context, next, &reply);
        }
    }
    return 0;
}

// Function to handle DONE message
int handle_done(struct Context *context, Message *msg, int8_t *rep_arr, int *replies) {
    if (context->num_done < context->children) {
        if (!context->rec_done[context->msg_sender]) {
            update_lamport_time_if_needed(msg->s_header.s_local_time);
            context->rec_done[context->msg_sender] = 1;
            context->num_done++;
            if (context->num_done == context->children) {
                printf(log_received_all_done_fmt, get_lamport_time(), context->loc_pid);
                fprintf(context->events, log_received_all_done_fmt, get_lamport_time(), context->loc_pid);
            }
            if (!rep_arr[context->msg_sender]) {
                rep_arr[context->msg_sender] = 1;
                (*replies)++;
            }
        }
    }
    return 0;  // No further action needed
}

// Main function for requesting the critical section
int request_cs(const void * self) {
    struct Context *context = (struct Context*)self;
    lamport_time++;
    push_request(&context->requests, (struct Request){context->loc_pid, get_lamport_time()});

    Message request;
    request.s_header.s_type = CS_REQUEST;
    request.s_header.s_payload_len = 0;

    if (send_message(context, context->loc_pid, &request)) {
        return 1;  // Failed to send CS_REQUEST message
    }

    local_id replies = 0;
    int8_t rep_arr[MAX_PROCESS_ID + 1] = {0};

    // Ensure the local process is counted as replying
    if (!rep_arr[context->loc_pid]) {
        replies++;
        rep_arr[context->loc_pid] = 1;
    }

    while (replies < context->children) {
        Message msg;
        while (receive_any(context, &msg)) {}

        switch (msg.s_header.s_type) {
            case CS_REQUEST:
                if (handle_cs_request(context, &msg)) {
                    return 2;  // Failed to send CS_REPLY
                }
                break;
            case CS_REPLY:
                if (handle_cs_reply(context, &msg, rep_arr, &replies)) {
                    return 3;  // Failed to handle CS_REPLY
                }
                break;
            case CS_RELEASE:
                if (handle_cs_release(context)) {
                    return 4;  // Failed to handle CS_RELEASE
                }
                break;
            case DONE:
                if (handle_done(context, &msg, rep_arr, &replies)) {
                    return 5;  // Failed to handle DONE message
                }
                break;
            default:
                break;  // Unknown message type
        }
    }
    return 0;  // Success
}


int release_cs(const void * self) {
    struct Context *context = (struct Context*)self;
    pop_head(&context->requests);
    lamport_time++;
    Message release;
    release.s_header.s_magic = MESSAGE_MAGIC;
    release.s_header.s_type = CS_RELEASE;
    release.s_header.s_payload_len = 0;
    release.s_header.s_local_time = get_lamport_time();
    if (send_multicast(context, &release)) {
        //fprintf(stderr, "Child %d: failed to send CS_RELEASE message\n", context->loc_pid);
        return 1;
    }
    local_id next = get_head(&context->requests).loc_pid;
    if (next > 0) {
        lamport_time++;
        Message reply;
        reply.s_header.s_magic = MESSAGE_MAGIC;
        reply.s_header.s_type = CS_REPLY;
        reply.s_header.s_payload_len = 0;
        reply.s_header.s_local_time = get_lamport_time();
        if (send(context, next, &reply)) {
            //fprintf(stderr, "Child %d: failed to seNd CS_REPLY message\n", context->loc_pid);
            return 2;
        }
    }
    return 0;
}

int parse_args(int argc, char * argv[], struct Context * context){
    if (argc < 3) {
        fprintf(stderr, "Usage: %s [--mutexl] -p N [--mutexl]\n", argv[0]);
        return 1;
    }
    
    (*context).mutexl = 0;
    int8_t rp = 0;
    for (int i = 1; i < argc; i++) {
        if (rp) {
            (*context).children = atoi(argv[i]);
            rp = 0;
        }
        if (strcmp(argv[i], "--mutexl") == 0) {
            (*context).mutexl = 1;
        }
        else if (strcmp(argv[i], "-p") == 0) {
            rp = 1;
        }
    }

    return 0;
}

void create_events_log_file(){
    FILE *evt = fopen(events_log, "w");
    fclose(evt);
}


void parent_func(struct Context context){
    for (local_id i = 1; i <= context.children; i++) context.rec_started[i] = 0;
    context.num_started = 0;
    for (local_id i = 1; i <= context.children; i++) context.rec_done[i] = 0;
    context.num_done = 0;
    while (context.num_started < context.children || context.num_done < context.children) {
        Message msg;
        while (receive_any(&context, &msg)) {}
        switch (msg.s_header.s_type) {
            case STARTED:
                if (context.num_started < context.children) {
                    if (!context.rec_started[context.msg_sender]) {
                        if (lamport_time < msg.s_header.s_local_time) lamport_time = msg.s_header.s_local_time;
                        lamport_time++;
                        context.rec_started[context.msg_sender] = 1;
                        context.num_started++;
                        if (context.num_started == context.children) {
                            printf(log_received_all_started_fmt, get_lamport_time(), context.loc_pid);
                            fprintf(context.events, log_received_all_started_fmt, get_lamport_time(), context.loc_pid);
                        }
                    }
                }
                break;
            case DONE:
                if (context.num_done < context.children) {
                    if (!context.rec_done[context.msg_sender]) {
                        if (lamport_time < msg.s_header.s_local_time) lamport_time = msg.s_header.s_local_time;
                        lamport_time++;
                        context.rec_done[context.msg_sender] = 1;
                        ++context.num_done;
                        if (context.num_done == context.children) {
                            printf(log_received_all_done_fmt, get_lamport_time(), context.loc_pid);
                            fprintf(context.events, log_received_all_done_fmt, get_lamport_time(), context.loc_pid);
                        }
                    }
                }
                break;
            default: break;
        }
        fflush(context.events);
    }
    for (local_id i = 0; i < context.children; i++) wait(NULL);
}

// Функция для отправки STARTED сообщения
int send_started_message(struct Context *context) {
    lamport_time++;
    Message started;
    started.s_header.s_magic = MESSAGE_MAGIC;
    started.s_header.s_type = STARTED;
    started.s_header.s_local_time = get_lamport_time();
    sprintf(started.s_payload, log_started_fmt, get_lamport_time(), context->loc_pid, getpid(), getppid(), 0);
    started.s_header.s_payload_len = strlen(started.s_payload);
    puts(started.s_payload);
    fputs(started.s_payload, context->events);

    if (send_multicast(context, &started)) {
        fprintf(stderr, "Child %d: failed to send STARTED message\n", context->loc_pid);
        close_pipes(&context->pipes);
        fclose(context->events);
        return 4;
    }
    return 0;  // Сообщение успешно отправлено
}

// Функция для обработки STARTED сообщений
int handle_started_message(struct Context *context, Message *msg) {
    if (context->num_started < context->children) {
        if (!context->rec_started[context->msg_sender]) {
            update_lamport_time_if_needed(msg->s_header.s_local_time);
            context->rec_started[context->msg_sender] = 1;
            context->num_started++;
            if (context->num_started == context->children) {
                printf(log_received_all_started_fmt, get_lamport_time(), context->loc_pid);
                fprintf(context->events, log_received_all_started_fmt, get_lamport_time(), context->loc_pid);
                return 1;  // Все процессы начали
            }
        }
    }
    return 0;  // Не все процессы начали
}

// Функция для выполнения операций в цикле
int perform_operations(struct Context *context) {
    for (int16_t i = 1; i <= context->loc_pid * 5; i++) {
        char log[50];
        sprintf(log, log_loop_operation_fmt, context->loc_pid, i, context->loc_pid * 5);
        if (context->mutexl) {
            int status = request_cs(context);
            if (status) {
                fprintf(stderr, "Child %d: request_cs() resulted %d\n", context->loc_pid, status);
                return 100;  // Ошибка при запросе критической секции
            }
        }
        print(log);
        if (context->mutexl) {
            int status = release_cs(context);
            if (status) {
                fprintf(stderr, "Child %d: release_cs() resulted %d\n", context->loc_pid, status);
                return 101;  // Ошибка при освобождении критической секции
            }
        }
    }
    return 0;  // Операции успешно выполнены
}

// Функция для отправки DONE сообщения
int send_done_message(struct Context *context) {
    lamport_time++;
    Message done;
    done.s_header.s_magic = MESSAGE_MAGIC;
    done.s_header.s_type = DONE;
    sprintf(done.s_payload, log_done_fmt, get_lamport_time(), context->loc_pid, 0);
    done.s_header.s_payload_len = strlen(done.s_payload);
    done.s_header.s_local_time = get_lamport_time();
    puts(done.s_payload);
    fputs(done.s_payload, context->events);

    if (send_multicast(context, &done)) {
        fprintf(stderr, "Child %d: failed to send DONE message\n", context->loc_pid);
        close_pipes(&context->pipes);
        fclose(context->events);
        return 5;
    }
    context->rec_done[context->loc_pid] = 1;
    context->num_done++;
    if (context->num_done == context->children) {
        printf(log_received_all_done_fmt, get_lamport_time(), context->loc_pid);
        fprintf(context->events, log_received_all_done_fmt, get_lamport_time(), context->loc_pid);
    }
    return 0;  // DONE сообщение отправлено успешно
}

// Функция для обработки CS_REQUEST сообщений
int handle_cs_request2(struct Context *context, Message *msg) {
    if (context->mutexl) {
        update_lamport_time_if_needed(msg->s_header.s_local_time);
        lamport_time++;
        push_request(&context->requests, (struct Request){context->msg_sender, msg->s_header.s_local_time});
        if (get_head(&context->requests).loc_pid == context->msg_sender) {
            lamport_time++;
            Message reply;
            reply.s_header.s_magic = MESSAGE_MAGIC;
            reply.s_header.s_type = CS_REPLY;
            reply.s_header.s_payload_len = 0;
            reply.s_header.s_local_time = get_lamport_time();
            return send(&context, get_head(&context->requests).loc_pid, &reply);
        }
    }
    return 0;  // Обработано успешно
}

// Функция для обработки CS_RELEASE сообщений
int handle_cs_release(struct Context *context) {
    if (context->mutexl) {
        update_lamport_time_if_needed(get_lamport_time());
        lamport_time++;
        pop_head(&context->requests);
        local_id next = get_head(&context->requests).loc_pid;
        if (next > 0 && next != context->loc_pid) {
            lamport_time++;
            Message reply;
            reply.s_header.s_magic = MESSAGE_MAGIC;
            reply.s_header.s_type = CS_REPLY;
            reply.s_header.s_payload_len = 0;
            reply.s_header.s_local_time = get_lamport_time();
            return send(&context, next, &reply);
        }
    }
    return 0;  // Обработано успешно
}

// Основная функция с рефакторингом
int child_func(struct Context context) {
    clear_queue(&context.requests);

    // Шаг 1: Отправка STARTED сообщения
    if (send_started_message(&context)) return 4;

    // Шаг 2: Инициализация состояний
    for (local_id i = 1; i <= context.children; i++) context.rec_started[i] = (i == context.loc_pid);
    context.num_started = 1;
    for (local_id i = 1; i <= context.children; i++) context.rec_done[i] = 0;
    context.num_done = 0;

    // Основной цикл
    int8_t active = 1;
    while (active || context.num_done < context.children) {
        Message msg;
        while (receive_any(&context, &msg)) {}

        switch (msg.s_header.s_type) {
            case STARTED:
                if (handle_started_message(&context, &msg)) {
                    // Все процессы начали, продолжаем с операций
                    if (perform_operations(&context)) return 100;
                    if (send_done_message(&context)) return 5;
                    active = 0;  // Заканчиваем
                }
                break;
            case CS_REQUEST:
                if (handle_cs_request2(&context, &msg)) return 6;
                break;
            case CS_RELEASE:
                if (handle_cs_release(&context)) return 7;
                break;
            case DONE:
                if (context.num_done < context.children) {
                    if (!context.rec_done[context.msg_sender]) {
                        update_lamport_time_if_needed(msg.s_header.s_local_time);
                        context.rec_done[context.msg_sender] = 1;
                        context.num_done++;
                        if (context.num_done == context.children) {
                            printf(log_received_all_done_fmt, get_lamport_time(), context.loc_pid);
                            fprintf(context.events, log_received_all_done_fmt, get_lamport_time(), context.loc_pid);
                        }
                    }
                }
                break;
            default: break;
        }
        fflush(context.events);
    }
    return 0;
}
