#ifndef MERGESORT_H
#define MERGESORT_H

#include "std.h"
#include "sortfunc.h"

#include <sys/types.h>
#include <pthread.h>
#include <math.h>

typedef struct merge_thread{
	pthread_t thread;
	thread_info info;
} merge_thread;


int start(int fd, EL_TYPE *buffer, size_t size, merge_thread* threads, int num_threads);
void print_threads(merge_thread* threads, int num_threads, EL_TYPE* buffer, size_t size);
void print_thread_data(merge_thread* threads, int num_threads, int swap);
void distribute_buffer(merge_thread* threads, int num_threads, EL_TYPE *buffer, size_t size);
int distribute_simple_sort(merge_thread* threads, int num_threads, int fd, int fd_buffer, uint64_t num_elements, size_t simple_sort_size);
int distribute_merge_sort(merge_thread* threads, int num_threads, int fd, int fd_buffer, uint64_t num_elements, size_t simple_sort_size, EL_TYPE *buffer, size_t size);

#endif
