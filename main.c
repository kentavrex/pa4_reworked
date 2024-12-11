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

int create_pipes_and_log(struct Context *context) {
    if (init_pipes(&context->pipes, context->children + 1, O_NONBLOCK, pipes_log)) {
        fputs("Parent: failed to create pipes\n", stderr);
        return 2;
    }

    create_events_log_file();
    context->events = fopen(events_log, "a");
    if (context->events == NULL) {
        fputs("Parent: failed to create event log\n", stderr);
        return 2;
    }

    return 0;
}

int fork_children(struct Context *context) {
    for (local_id i = 1; i <= context->children; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            context->loc_pid = i;
            free_pipes(&context->pipes, i);
            return 0;
        }
        if (pid < 0) {
            fprintf(stderr, "Parent: failed to create child process %d\n", i);
            close_pipes(&context->pipes);
            fclose(context->events);
            return 3;
        }
        context->loc_pid = PARENT_ID;
        free_pipes(&context->pipes, PARENT_ID);
    }

    return 0;
}

void cleanup_and_close(struct Context *context) {
    free_pipes(&context->pipes, context->loc_pid);
    close_pipes(&context->pipes);
    fclose(context->events);
}

int main(int argc, char * argv[]) {
    struct Context context;
    if (parse_args(argc, argv, &context)) {
        return 1;
    }

    int res = create_pipes_and_log(&context);
    if (res != 0) {
        return res;
    }

    res = fork_children(&context);
    if (res != 0) {
        return res;
    }

    if (context.loc_pid == PARENT_ID) {
        parent_func(context);
    } else {
        res = child_func(context);
        if (res) {
            return res;
        }
    }

    cleanup_and_close(&context);
    return 0;
}
