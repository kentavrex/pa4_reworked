#include <sys/types.h>
#include <sys/wait.h>
#include <asm-generic/errno.h>
#include "util.h"
#include "common.h"
#include "pipes_manager.h"

void transfer(void *context_data, local_id initiator, local_id recipient, balance_t transfer_amount) {
}

int parse_arguments(int argc, char *argv[], int *process_count, int *use_mutex) {
    if (argc == 3 && strcmp(argv[1], "-p") == 0) {
        *process_count = atoi(argv[2]);
        if (*process_count < 1 || *process_count > 10) {
            return -1;
        }
    } else if (argc == 4) {
        if (strcmp(argv[1], "-p") == 0) {
            *process_count = atoi(argv[2]);
            if (*process_count < 1 || *process_count > 10) {
                return -1;
            }
            *use_mutex = strcmp(argv[3], "--mutexl") == 0;
        } else if (strcmp(argv[2], "-p") == 0) {
            *process_count = atoi(argv[3]);
            if (*process_count < 1 || *process_count > 10) {
                return -1;
            }
            *use_mutex = strcmp(argv[1], "--mutexl") == 0;
        }
    } else {
        return -1;
    }
    return 0;
}

void open_log_files(FILE **log_pipes, FILE **log_events) {
    *log_pipes = fopen("pipes.log", "w+");
    *log_events = fopen("events.log", "w+");
    if (!*log_pipes || !*log_events) {
        fprintf(stderr, "Error: unable to open log files.\n");
        exit(EXIT_FAILURE);
    }
}

Pipe** initialize_pipes(int process_count, FILE *log_pipes) {
    Pipe **pipes = init_pipes(process_count, log_pipes);
    if (pipes == NULL) {
        fprintf(stderr, "Error: failed to initialize pipes.\n");
        exit(EXIT_FAILURE);
    }
    return pipes;
}

void initialize_child_process(Process *child, int process_count, Pipe **pipes, int use_mutex, local_id process_id) {
    child->num_process = process_count;
    child->pipes = pipes;
    child->use_mutex = use_mutex;
    child->pid = process_id;
    child->queue_size = 0;
    child->queue = malloc(sizeof(Query) * (process_count - 1));
    if (child->queue == NULL) {
        fprintf(stderr, "Error: failed to allocate memory for process queue.\n");
        exit(EXIT_FAILURE);
    }
}

void setup_child_process(Process *child, FILE *log_pipes, FILE *log_events) {
    close_non_related_pipes(child, log_pipes);
    if (send_message(child, STARTED) != 0) {
        fprintf(stderr, "Error: failed to send STARTED message from process %d.\n", child->pid);
        exit(EXIT_FAILURE);
    }
    fprintf(stdout, log_started_fmt, get_lamport_time(), child->pid, getpid(), getppid(), 0);
    fprintf(log_events, log_started_fmt, get_lamport_time(), child->pid, getpid(), getppid(), 0);
}

void handle_startup(Process *child, FILE *log_events) {
    if (check_all_received(child, STARTED) != 0) {
        fprintf(stderr, "Error: process %d failed to receive all STARTED messages.\n", child->pid);
        exit(EXIT_FAILURE);
    }
    fprintf(stdout, log_received_all_started_fmt, get_lamport_time(), child->pid);
    fprintf(log_events, log_received_all_started_fmt, get_lamport_time(), child->pid);
}

void perform_operations(Process *child, FILE *log_events) {
    if (child->use_mutex) {
        bank_operations(child, log_events);
    } else {
        for (int iteration = 1; iteration <= child->pid * 5; iteration++) {
            char operation_log[100];
            snprintf(operation_log, sizeof(operation_log), log_loop_operation_fmt, child->pid, iteration, child->pid * 5);
            print(operation_log);
        }
    }
}

void send_done_and_wait(Process *child, FILE *log_pipes, FILE *log_events) {
    if (send_message(child, DONE) != 0) {
        fprintf(stderr, "Error: failed to send DONE message from process %d.\n", child->pid);
        exit(EXIT_FAILURE);
    }
    fprintf(stdout, log_done_fmt, get_lamport_time(), child->pid, 0);
    fprintf(log_events, log_done_fmt, get_lamport_time(), child->pid, 0);
    if (check_all_received(child, DONE) != 0) {
        fprintf(stderr, "Error: process %d failed to receive all DONE messages.\n", child->pid);
        exit(EXIT_FAILURE);
    }
    fprintf(stdout, log_received_all_done_fmt, get_lamport_time(), child->pid);
    fprintf(log_events, log_received_all_done_fmt, get_lamport_time(), child->pid);
}

void close_pipes(Process *child, FILE *log_pipes) {
    close_outcoming_pipes(child, log_pipes);
    close_incoming_pipes(child, log_pipes);
}

void initialize_parent_process(Process *parent, Pipe **pipes, int process_count) {
    parent->num_process = process_count;
    parent->pipes = pipes;
    parent->pid = PARENT_ID;
}

void handle_parent_process(Process *parent, FILE *log_pipes, FILE *log_events) {
    close_non_related_pipes(parent, log_pipes);
    fprintf(stdout, log_started_fmt, get_lamport_time(), PARENT_ID, getpid(), getppid(), 0);
    fprintf(log_events, log_started_fmt, get_lamport_time(), PARENT_ID, getpid(), getppid(), 0);
    if (check_all_received(parent, STARTED) != 0) {
        fprintf(stderr, "Error: parent process failed to receive all STARTED messages.\n");
        exit(EXIT_FAILURE);
    }
    fprintf(stdout, log_received_all_started_fmt, get_lamport_time(), PARENT_ID);
    fprintf(log_events, log_received_all_started_fmt, get_lamport_time(), PARENT_ID);
    if (check_all_received(parent, DONE) != 0) {
        fprintf(stderr, "Error: parent process failed to receive all DONE messages.\n");
        exit(EXIT_FAILURE);
    }
    fprintf(stdout, log_received_all_done_fmt, get_lamport_time(), PARENT_ID);
    fprintf(log_events, log_received_all_done_fmt, get_lamport_time(), PARENT_ID);
    fprintf(stdout, log_done_fmt, get_lamport_time(), PARENT_ID, 0);
    fprintf(log_events, log_done_fmt, get_lamport_time(), PARENT_ID, 0);
    close_incoming_pipes(parent, log_pipes);
    close_outcoming_pipes(parent, log_pipes);
}

void wait_for_children() {
    while (wait(NULL) > 0);
}

int main(int argc, char *argv[]) {
    int process_count = 0;
    int use_mutex = 0;

    if (parse_arguments(argc, argv, &process_count, &use_mutex) != 0) {
        fprintf(stderr, "Usage: %s -p X [--mutexl]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    process_count++;

    FILE *log_pipes = NULL;
    FILE *log_events = NULL;
    open_log_files(&log_pipes, &log_events);

    Pipe **pipes = initialize_pipes(process_count, log_pipes);

    for (local_id process_id = 1; process_id < process_count; process_id++) {
        pid_t child_pid = fork();
        if (child_pid < 0) {
            fprintf(stderr, "Error: fork failed.\n");
            exit(EXIT_FAILURE);
        }
        if (child_pid == 0) {
            Process child;
            initialize_child_process(&child, process_count, pipes, use_mutex, process_id);

            printf("Child process initialized with ID: %d\n", child.pid);

            setup_child_process(&child, log_pipes, log_events);
            handle_startup(&child, log_events);
            perform_operations(&child, log_events);
            send_done_and_wait(&child, log_pipes, log_events);
            close_pipes(&child, log_pipes);
            exit(EXIT_SUCCESS);
        }
    }

    Process parent;
    initialize_parent_process(&parent, pipes, process_count);

    printf("Parent process initialized with ID: %d\n", parent.pid);

    handle_parent_process(&parent, log_pipes, log_events);
    wait_for_children();

    fclose(log_pipes);
    fclose(log_events);

    return 0;
}
