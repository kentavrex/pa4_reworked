#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include "pipes.h"

static void close_pipe_pair(Descriptor *pipe_descriptors, int index) {
    close(pipe_descriptors[2 * index]);
    close(pipe_descriptors[2 * index + 1]);
}

static void set_pipe_flags(Descriptor *pipe_descriptors, int index, int flags) {
    fcntl(pipe_descriptors[2 * index], F_SETFL, fcntl(pipe_descriptors[2 * index], F_GETFL, 0) | flags);
    fcntl(pipe_descriptors[2 * index + 1], F_SETFL, fcntl(pipe_descriptors[2 * index + 1], F_GETFL, 0) | flags);
}

static void log_pipe_creation(FILE *log_file, Descriptor *pipe_descriptors, int index) {
    fprintf(log_file, "Opened pipe descriptors %d and %d\n", pipe_descriptors[2 * index], pipe_descriptors[2 * index + 1]);
}

static int create_pipe_pair(Descriptor *pipe_descriptors, int index) {
    int status = pipe(pipe_descriptors + 2 * index);
    if (status) return status;
    set_pipe_flags(pipe_descriptors, index, O_NONBLOCK);
    return 0;
}

static void close_single_pipe(Descriptor *pipe_descriptors, int index) {
    close(pipe_descriptors[2 * index]);
    close(pipe_descriptors[2 * index + 1]);
}

static void close_all_pipes(struct Pipes *pipes) {
    for (int i = 0; i < 2 * pipes->size * (pipes->size - 1); i++) {
        close_single_pipe(pipes->pipe_descriptors, i);
    }
}

static void free_resources(struct Pipes *pipes) {
    free(pipes->pipe_descriptors);
    fclose(pipes->pipe_log);
}

void close_pipes(struct Pipes *pipes) {
    close_all_pipes(pipes);
    pipes->size = 0;
    free_resources(pipes);
}

static void allocate_pipe_descriptors(struct Pipes *pipes, local_id process_num) {
    pipes->pipe_descriptors = malloc(2 * process_num * (process_num - 1) * sizeof(Descriptor));
}

static void open_pipe_log(struct Pipes *pipes, const char *log_file) {
    pipes->pipe_log = fopen(log_file, "w");
}

static int create_and_setup_pipe(struct Pipes *pipes, int index) {
    int status = create_pipe_pair(pipes->pipe_descriptors, index);
    if (status) {
        return status;
    }
    log_pipe_creation(pipes->pipe_log, pipes->pipe_descriptors, index);
    return 0;
}

int init_pipes(struct Pipes *pipes, local_id process_num, int flags, const char *log_file) {
    int status = 0;
    pipes->size = process_num;

    allocate_pipe_descriptors(pipes, process_num);
    open_pipe_log(pipes, log_file);

    status = setup_all_pipes(pipes, process_num);
    if (status != 0) {
        close_pipes(pipes);
    }

    fflush(pipes->pipe_log);
    return status;
}

int setup_all_pipes(struct Pipes *pipes, int process_num) {
    for (int i = 0; i < process_num * (process_num - 1); i++) {
        int status = create_and_setup_pipe(pipes, i);
        if (status != 0) {
            return status;
        }
    }
    return 0;
}

Descriptor access_pipe(const struct Pipes *pipes, struct PipeDescriptor address) {
    if (address.from < 0 || address.from >= pipes->size || address.to < 0 || address.to >= pipes->size || address.from == address.to) {
        return -1;
    }
    int index = calculate_pipe_index(pipes, address);
    return pipes->pipe_descriptors[2 * index + address.mode];
}

int calculate_pipe_index(const struct Pipes *pipes, struct PipeDescriptor address) {
    int index = address.from * (pipes->size - 1);
    index += address.to - (address.from < address.to);
    return index;
}

sstatic void close_process_pipe(FILE *pipe_log, const struct Pipes *pipes, local_id process_id, local_id i, local_id j, PipeMode mode) {
    Descriptor pipe = access_pipe(pipes, (struct PipeDescriptor){i, j, mode});
    if ((mode == WRITING && i != process_id) || (mode == READING && j != process_id)) {
        close(pipe);
        log_pipe_closure(pipe_log, process_id, pipe);
    }
}

static void log_pipe_closure(FILE *pipe_log, local_id process_id, Descriptor pipe) {
    fprintf(pipe_log, "Process %d closed pipe descriptor %d\n", process_id, pipe);
}

void close_process_pipes(FILE *pipe_log, const struct Pipes *pipes, local_id process_id, local_id i, local_id j) {
    close_process_pipe(pipe_log, pipes, process_id, i, j, READING);
    close_process_pipe(pipe_log, pipes, process_id, i, j, WRITING);
}

void close_pipes_for_process(FILE *pipe_log, const struct Pipes *pipes, local_id process_id) {
    for (local_id i = 0; i < pipes->size; i++) {
        for (local_id j = 0; j < pipes->size; j++) {
            if (i != j) {
                close_process_pipes(pipe_log, pipes, process_id, i, j);
            }
        }
    }
}

void free_pipes(const struct Pipes *pipes, local_id process_id) {
    close_pipes_for_process(pipes->pipe_log, pipes, process_id);
    flush_pipe_log(pipes->pipe_log);
}

void flush_pipe_log(FILE *pipe_log) {
    fflush(pipe_log);
}


struct PipeDescriptor get_pipe_descriptor(const struct Pipes *pipes, int descriptor_index) {
    struct PipeDescriptor res;
    res.to = (descriptor_index / 2) % (pipes->size - 1);
    res.mode = descriptor_index % 2;
    res.from = (descriptor_index / 2) / (pipes->size - 1);
    if (res.from <= res.to) {
      ++res.to;
    }
    return res;
}
