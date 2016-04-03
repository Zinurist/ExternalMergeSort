#ifndef SORTFUNC_H
#define SORTFUNC_H

#include "std.h"

#include <pthread.h>

extern pthread_mutex_t file_lock;

typedef struct thread_info{
	EL_TYPE* blocka;//start, included
	EL_TYPE* blockb;//end of blocka
	EL_TYPE* blockc;//end of blockb
	EL_TYPE* end;
	void* data;
} thread_info ;

typedef struct simple_arg{
	uint64_t start_el; 
	uint64_t end_el;
	int fd;
} simple_arg ;

typedef struct merge_phase{
	uint64_t start_el_a;
	uint64_t end_el_a;
	uint64_t start_el_b;
	uint64_t end_el_b;
	uint64_t start_buffer;
	uint64_t end_buffer;
	int fd;
	int fd_buffer;
} merge_phase;

void * simple_sort(void* arg);
void quick_sort(EL_TYPE* buffer, size_t size);
void * merge_sort(void* arg);

int is_sorted(int fd, EL_TYPE *buffer, size_t size);

#endif