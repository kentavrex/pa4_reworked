#ifndef CS_H
#define CS_H


#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <asm-generic/errno.h>

#include "pa2345.h"
#include "const.h"

void create_request_message(Message* message, timestamp_t curr_time);

int sort_requests(const void* left, const void* right);

void send_message_to_peers(Process* handler, Message* message, int exclude_pid);

void enqueue_request(Process* handler, Query request);

void dequeue_request(Process* handler);

void create_release_message(Message* message, timestamp_t curr_time);

int initiate_cs_request(const void* context);

int finalize_cs_release(const void* context);

#endif
