#ifndef __FUNC__H
#define __FUNC__H

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

extern timestamp_t lamport_time;

timestamp_t get_lamport_time();
int parse_args(int argc, char * argv[], struct Context * context);
void create_events_log_file();
void parent_func(struct Context context);
int child_func(struct Context context);

#endif
