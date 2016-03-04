#ifndef MERGESORT_H
#define MERGESORT_H

#include "std.h"
#include "sortfunc.h"

#include <sys/types.h>
#include <pthread.h>

#define EL_TYPE int
#define EL_SIZE (sizeof(EL_TYPE))

typedef struct merge_thread{
	pthread_t thread;
	thread_info info;
} merge_thread;


int start(int fd, EL_TYPE *buffer, size_t size, int num_threads);
void print_threads(merge_thread* threads, int num_threads, EL_TYPE* buffer, size_t size);
void distribute_buffer(merge_thread* threads, int num_threads, EL_TYPE *buffer, size_t size);
int distribute_simple_sort(merge_thread* threads, int num_threads, int fd, uint64_t num_elements);
int distribute_merge_sort(merge_thread* threads, int num_threads, int fd, int fd_buffer, uint64_t num_elements);

#endif
