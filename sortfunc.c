#include "sortfunc.h"

pthread_mutex_t file;

void * simple_sort(void * arg){


	thread_info* info = (thread_info*) arg;
	simple_arg* bounds = (simple_arg*)info->data;

	//assert: (end_el-start_el) % 4 = 0, unless its the last thread
	//printf("%i, %lu, %lu\n", info->fd, info->start_el, info->end_el);



	pthread_mutex_lock(&file);

	//add sizeof(uint64_t), because of number of elements at beginning of file
	lseek(bounds->fd, bounds->start_el*EL_SIZE+sizeof(uint64_t), SEEK_SET);

	pthread_mutex_unlock(&file);


	free(bounds);
	return NULL;
}