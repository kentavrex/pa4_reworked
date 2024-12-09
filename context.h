#ifndef __CONTEXT__H
#define __CONTEXT__H

#include <stdio.h>

#include "ipc.h"
#include "pipes.h"
#include "queue.h"

struct Context {
    local_id children;
    local_id loc_pid; 
    local_id msg_sender;
    struct Pipes pipes;
    local_id num_started;
    local_id num_done;
    int8_t rec_started[MAX_PROCESS_ID+1];
    int8_t rec_done[MAX_PROCESS_ID+1];
    FILE *events;
    int8_t mutexl;
    struct RequestQueue requests;
};

#endif
