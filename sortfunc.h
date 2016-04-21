#ifndef SORTFUNC_H
#define SORTFUNC_H

#include "std.h"

#include <pthread.h>

extern pthread_mutex_t file_lock;

typedef struct sort_data{
	uint64_t start_from_a;
	uint64_t start_from_b;
	uint64_t end_from;
	uint64_t start_to;
	uint64_t block_size;
	uint64_t pairs;
	int fd_from;
	int fd_to;
} sort_data;

typedef struct thread_info{
	EL_TYPE* blocka;//start, included
	EL_TYPE* blockb;//end of blocka
	EL_TYPE* blockc;//end of blockb
	EL_TYPE* end;
	sort_data data;
} thread_info;



void * simple_sort(void* arg);
void quick_sort(EL_TYPE* buffer, size_t size);
void * merge_sort(void* arg);

int is_sorted(int fd, EL_TYPE *buffer, size_t size);

#endif