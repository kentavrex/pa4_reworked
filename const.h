#ifndef CONST_H
#define CONST_H

#include <stdint.h>
#include "banking.h"

static const int ERR = 1;
static const int OK = 0;

typedef struct {
    int fd[2];
} Pipe;

static const short WRITE = 1;

typedef struct {
    timestamp_t time;
    local_id pid;
} Query;

static const short READ = 0;

typedef struct {
    long num_process;
    Pipe** pipes;
    int8_t pid;
    int use_mutex;
    Query* queue;
    int queue_size;
} Process;

#endif
