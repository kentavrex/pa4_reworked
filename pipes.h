#ifndef __PIPES__H
#define __PIPES__H

#include <stdio.h>

#include "ipc.h"

typedef int Descriptor;
typedef int8_t Mode;

static const Mode READING = 0;
static const Mode WRITING = 1;

struct PipeDescriptor {
	local_id from;
	local_id to;
	Mode mode;
};

struct Pipes {
	local_id size;
	Descriptor *pipe_descriptors;
	FILE *pipe_log;
};

void close_pipes(struct Pipes *pipes);
int init_pipes(struct Pipes *pipes, local_id process_num, int flags, const char *log_file);
Descriptor access_pipe(const struct Pipes *pipes, struct PipeDescriptor address);
void free_pipes(const struct Pipes *pipes, local_id process_id);
struct PipeDescriptor get_pipe_descriptor(const struct Pipes *pipes, int descriptor_index);

#endif
