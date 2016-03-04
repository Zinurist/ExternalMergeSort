#include "std.h"
#include <pthread.h>

typedef struct thread_info{
	EL_TYPE* blocka;//start, included
	EL_TYPE* blockb;//end of blocka
	EL_TYPE* blockc;//end of blockb
	size_t sizea, sizeb, sizec;
	void* data;
} thread_info ;

typedef struct simple_arg{
	uint64_t start_el; 
	uint64_t end_el;
	int fd;
} simple_arg ;

void * simple_sort(void* arg);