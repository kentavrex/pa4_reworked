#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "common.h"
#include "context.h"
#include "ipc.h"
#include "pa2345.h"
#include "pipes.h"
#include "func.h"

timestamp_t lamport_time = 0;

int create_pipes_phase(struct Context *context) {
    if (init_pipes(&context->pipes, context->children + 1, O_NONBLOCK, pipes_log)) {
        fputs("Parent: failed to create pipes\n", stderr);
        return 2;
    }
    return 0;
}

void setup_events_log(struct Context *context) {
    create_events_log_file();
    context->events = fopen(events_log, "a");
}

int create_child_process(struct Context *context, local_id i) {
    pid_t pid = fork();
    if (pid == 0) {
        context->loc_pid = i;
        return 0;
    }
    if (pid < 0) {
        fprintf(stderr, "Parent: failed to create child process %d\n", i);
        close_pipes(&context->pipes);
        fclose(context->events);
        return 3;
    }
    return 1;
}

void parent_phase(struct Context *context) {
    parent_func(*context);
}

void child_phase(struct Context *context) {
    int res = child_func(*context);
    if (res) exit(res);
}

void free_pipes_and_exit(struct Context *context) {
    free_pipes(&context->pipes, context->loc_pid);
    close_pipes(&context->pipes);
    fclose(context->events);
}

int main(int argc, char *argv[]) {
    struct Context context;
    if (parse_args(argc, argv, &context)) {
        return 1;
    }

    int res = create_pipes_phase(&context);
    if (res) return res;

    setup_events_log(&context);

    for (local_id i = 1; i <= context.children; i++) {
        res = create_child_process(&context, i);
        if (res == 0) break;
    }

    free_pipes_and_exit(&context);

    if (context.loc_pid == PARENT_ID) {
        parent_phase(&context);
    } else {
        child_phase(&context);
    }

    return 0;
}
