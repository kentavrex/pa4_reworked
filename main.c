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

int main(int argc, char * argv[]) {
    struct Context context;
    if(parse_args(argc, argv, &context)){
        return 1;
    }

    if (init_pipes(&context.pipes, context.children+1, O_NONBLOCK, pipes_log)) {
        fputs("Parent: failed to create pipes\n", stderr);
        return 2;
    }
    
    create_events_log_file();
    context.events = fopen(events_log, "a");

    for (local_id i = 1; i <= context.children; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            context.loc_pid = i;
            break;
        }
        if (pid < 0) {
            fprintf(stderr, "Parent: failed to create child process %d\n", i);
            close_pipes(&context.pipes);
            fclose(context.events);
            return 3;
        }
        context.loc_pid = PARENT_ID;
    }
    
    free_pipes(&context.pipes, context.loc_pid);

    if (context.loc_pid == PARENT_ID) {
        parent_func(context);
    }
    else {
        int res = child_func(context);
        if(res){
            return res;
        }
    }
    close_pipes(&context.pipes);
    fclose(context.events);
    return 0;
}
