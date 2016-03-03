#include "std.h"
#include <pthread.h>

typedef struct simple_arg{
	uint64_t start_el; 
	uint64_t end_el;
	int fd;
} simple_arg ;

void * simple_sort(void* arg);