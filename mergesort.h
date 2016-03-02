#include "std.h"

#include <sys/types.h>
#include <pthread.h>

typedef struct merge_thread{
	pthread_t thread;
	EL_TYPE* blocka;//start, included
	EL_TYPE* blockb;//end of blocka
	EL_TYPE* blockc;//end of blockb
	size_t sizea, sizeb, sizec;
} merge_thread;




int start(int fd, EL_TYPE *buffer, size_t size, int num_threads);
void distribute_buffer(merge_thread* threads, int num_threads, EL_TYPE *buffer, size_t size);
void print_threads(merge_thread* threads, int num_threads, EL_TYPE* buffer);
int simple_sort(int fd, uint64_t start_el, uint64_t end_el);


