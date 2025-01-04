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

/*
int compare_requests(const void* req1, const void* req2);

void add_request_to_queue(Process* proc, Query new_request);

void remove_request_from_queue(Process* proc);

int send_critical_section_request(const void* context);

int send_critical_section_release(const void* context);
*/
int compare_requests(const void* a, const void* b);

void add_request_to_queue(Process* process, Query query);

void remove_request_from_queue(Process* process);

int send_critical_section_request (const void* self);

int send_critical_section_release (const void* self);

#endif
