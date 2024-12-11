#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

#include "pipes.h"

void close_pipes(struct Pipes *pipes) {
	for (int i = 0; i < 2*pipes->size*(pipes->size-1); ++i) close(pipes->pipe_descriptors[i]);
	free(pipes->pipe_descriptors);
	pipes->size = 0;
    fclose(pipes->pipe_log);
}

int init_pipes(struct Pipes *pipes, local_id procnum, int flags, const char *log_file) { // flags are appended to existing ones
	int status = 0;
	pipes->size = procnum;
	pipes->pipe_descriptors = malloc(2*procnum*(procnum-1)*sizeof(Descriptor));
    pipes->pipe_log = fopen(log_file, "w");
	for (int i = 0; i < procnum*(procnum-1); ++i) {
		if ((status = pipe(pipes->pipe_descriptors+2*i))) {
			close_pipes(pipes);
			return status;
		}
		if ((status = fcntl(pipes->pipe_descriptors[2*i], F_SETFL, fcntl(pipes->pipe_descriptors[2*i], F_GETFL, 0) | flags))) {
			close_pipes(pipes);
			return status;
		}
		if ((status = fcntl(pipes->pipe_descriptors[2*i+1], F_SETFL, fcntl(pipes->pipe_descriptors[2*i+1], F_GETFL, 0) | flags))) {
			close_pipes(pipes);
			return status;
		}
		fprintf(pipes->pipe_log, "Opened pipe descriptors %d and %d\n", pipes->pipe_descriptors[2*i], pipes->pipe_descriptors[2*i+1]);
	}
	fflush(pipes->pipe_log);
	return status;
}

Descriptor access_pipe(const struct Pipes *pipes, struct PipeDescriptor address) { // returns pipe descriptor
	if (address.from < 0 || address.from >= pipes->size || address.to < 0 || address.to >= pipes->size || address.from == address.to)
		return -1;
	int index = address.from * (pipes->size - 1);
	index += address.to - (address.from < address.to);
	return pipes->pipe_descriptors[2*index+address.mode];
}

void closeUnusedPipes(const struct Pipes *pipes, local_id procid) {
	for (local_id i = 0; i < pipes->size; ++i) {
		for (local_id j = 0; j < pipes->size; ++j) {
			if (i != j) {
                Descriptor rd = access_pipe(pipes, (struct PipeDescriptor){i, j, READING});
                Descriptor wr = access_pipe(pipes, (struct PipeDescriptor){i, j, WRITING});
				if (i != procid) {
                    close(wr);
                    fprintf(pipes->pipe_log, "Process %d closed pipe descriptor %d\n", procid, wr);
                }
				if (j != procid) {
                    close(rd);
                    fprintf(pipes->pipe_log, "Process %d closed pipe descriptor %d\n", procid, rd);
                }
			}
		}
	}
	fflush(pipes->pipe_log);
}

struct PipeDescriptor get_pipe_descriptor(const struct Pipes *pipes, int desc_index) {
	struct PipeDescriptor res;
	int pipe_index = desc_index / 2;
	res.mode = desc_index % 2;
	res.from = pipe_index / (pipes->size - 1);
	res.to = pipe_index % (pipes->size - 1);
	if (res.from <= res.to) ++res.to;
	return res;
}
