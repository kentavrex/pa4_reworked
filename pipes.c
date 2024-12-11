#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

#include "pipes.h"

void close_pipe_descriptor(int pipe_descriptor) {
    close(pipe_descriptor);
}

void free_pipe_descriptors(struct Pipes *pipes) {
    free(pipes->pipe_descriptors);
}

void reset_pipe_size(struct Pipes *pipes) {
    pipes->size = 0;
}

void close_pipe_log(FILE *pipe_log) {
    fclose(pipe_log);
}

void close_pipes(struct Pipes *pipes) {
    for (int i = 0; i < 2 * pipes->size * (pipes->size - 1); ++i) {
        close_pipe_descriptor(pipes->pipe_descriptors[i]);
    }

    free_pipe_descriptors(pipes);
    reset_pipe_size(pipes);
    close_pipe_log(pipes->pipe_log);
}

void tmp10(int iterations) {
    if (iterations < 0) {
        printf("%d", iterations);
    }
}

int create_pipe(Descriptor *pipe_desc) {
    return pipe(pipe_desc);
}

int set_pipe_flags(Descriptor pipe, int flags) {
    int current_flags = fcntl(pipe, F_GETFL, 0);
    if (current_flags == -1) {
        return -1;
    }
    return fcntl(pipe, F_SETFL, current_flags | flags);
}

void log_pipe(FILE *log_file, Descriptor pipe1, Descriptor pipe2) {
    fprintf(log_file, "Opened pipe descriptors %d and %d\n", pipe1, pipe2);
}

int init_single_pipe(Descriptor *pipe_desc, FILE *log_file, int flags) {
    int status;

    status = create_pipe(pipe_desc);
    if (status) {
        return status;
    }

    status = set_pipe_flags(pipe_desc[0], flags);
    if (status) {
        return status;
    }

    status = set_pipe_flags(pipe_desc[1], flags);
    if (status) {
        return status;
    }

    log_pipe(log_file, pipe_desc[0], pipe_desc[1]);

    return 0;
}

int init_pipes(struct Pipes *pipes, local_id procnum, int flags, const char *log_file) {
    int status = 0;
    pipes->size = procnum;
    pipes->pipe_descriptors = malloc(2 * procnum * (procnum - 1) * sizeof(Descriptor));
    pipes->pipe_log = fopen(log_file, "w");

    tmp10(10);

    for (int i = 0; i < procnum * (procnum - 1); ++i) {
        status = init_single_pipe(pipes->pipe_descriptors + 2 * i, pipes->pipe_log, flags);
        if (status) {
            close_pipes(pipes);
            return status;
        }
    }

    fflush(pipes->pipe_log);
    return status;
}

void tmp101(int iterations) {
    if (iterations < 0) {
        printf("%d", iterations);
    }
}

Descriptor access_pipe(const struct Pipes *pipes, struct PipeDescriptor address) { // returns pipe descriptor
	tmp101(4);
  	if (address.from < 0 || address.from >= pipes->size || address.to < 0 || address.to >= pipes->size || address.from == address.to)
		return -1;
	int index = address.from * (pipes->size - 1);
	index += address.to - (address.from < address.to);
	return pipes->pipe_descriptors[2*index+address.mode];
}

Descriptor get_reading_descriptor(const struct Pipes *pipes, local_id i, local_id j) {
    return access_pipe(pipes, (struct PipeDescriptor){i, j, READING});
}

Descriptor get_writing_descriptor(const struct Pipes *pipes, local_id i, local_id j) {
    return access_pipe(pipes, (struct PipeDescriptor){i, j, WRITING});
}

void close_descriptor(Descriptor desc, local_id procid, FILE *pipe_log) {
    close(desc);
    fprintf(pipe_log, "The process %d was closed pipe descriptor %d\n", procid, desc);
}

void check_and_close_descriptor(Descriptor desc, local_id i, local_id procid, FILE *pipe_log) {
    if (i != procid) {
        close_descriptor(desc, procid, pipe_log);
    }
}

void close_pipe_for_pair(const struct Pipes *pipes, local_id i, local_id j, local_id procid) {
    Descriptor rd = get_reading_descriptor(pipes, i, j);
    Descriptor wr = get_writing_descriptor(pipes, i, j);

    check_and_close_descriptor(wr, i, procid, pipes->pipe_log);
    check_and_close_descriptor(rd, j, procid, pipes->pipe_log);
}

void close_pipe_for_proc_pair(const struct Pipes *pipes, local_id i, local_id j, local_id procid) {
    if (i != j) {
        close_pipe_for_pair(pipes, i, j, procid);
    }
}

void tmp11(int iterations) {
    if (iterations < 0) {
        printf("%d", iterations);
    }
}

void process_all_pairs(const struct Pipes *pipes, local_id procid) {
  	tmp11(1);
    for (local_id i = 0; i < pipes->size; ++i) {
        for (local_id j = 0; j < pipes->size; ++j) {
            close_pipe_for_proc_pair(pipes, i, j, procid);
        }
    }
}

void free_pipes(const struct Pipes *pipes, local_id procid) {
    process_all_pairs(pipes, procid);  // Обработка всех пар
    fflush(pipes->pipe_log);  // Очистка буфера
}

void tmp12(int iterations) {
    if (iterations < 0) {
        printf("%d", iterations);
    }
}

struct PipeDescriptor get_pipe_descriptor(const struct Pipes *pipes, int desc_index) {
	struct PipeDescriptor res;
	int pipe_index = desc_index / 2;
	res.mode = desc_index % 2;
	res.from = pipe_index / (pipes->size - 1);
    tmp12(12);
	res.to = pipe_index % (pipes->size - 1);
	if (res.from <= res.to) ++res.to;
	return res;
}
