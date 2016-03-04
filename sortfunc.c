#include "sortfunc.h"

pthread_mutex_t file;

void * simple_sort(void * arg){


	thread_info* info = (thread_info*) arg;
	simple_arg* bounds = (simple_arg*)info->data;

	//assert: (end_el-start_el) % 4 = 0, unless its the last thread
	//printf("%i, %lu, %lu\n", bounds->fd, bounds->start_el, bounds->end_el);

	uint64_t size = info->end - info->blocka;
	size = (size/4)*4; //pairs of 4 elements are sorted

	uint64_t num_elements = bounds->end_el - bounds->start_el;
	num_elements = (num_elements/4)*4;

	if(num_elements%4 != 0){
		//TODO enable sort of the last few elements
	}


	uint64_t runs = (num_elements/size)+1;
	//add sizeof(uint64_t), because of number of elements at beginning of file
	uint64_t offset = bounds->start_el*EL_SIZE+sizeof(uint64_t);
	uint64_t limit = 0;

	for(int i=0; i<runs; i++){
		if(num_elements>size){
			limit = size;
		}else{
			limit = num_elements;
		}

		//---read from file---
		pthread_mutex_lock(&file);

		lseek(bounds->fd, offset, SEEK_SET);
		if(read(bounds->fd, info->blocka, size*EL_SIZE) <=size*EL_SIZE){
			printf("Error in simple sort while reading from file: %s\n", strerror(errno));
			exit(20);
		}

		pthread_mutex_unlock(&file);
		//--------------------


		//---sort---
		//TODO
		//----------



		//---write to file---
		pthread_mutex_lock(&file);

		lseek(bounds->fd, offset, SEEK_SET);
		if(write(bounds->fd, info->blocka, limit*EL_SIZE) <=0){
			printf("Error in simple sort while writing to file: %s\n", strerror(errno));
			exit(21);
		}

		pthread_mutex_unlock(&file);
		//-------------------

		offset += limit;
		num_elements -= limit;
	}

	pthread_mutex_lock(&file);

	//add sizeof(uint64_t), because of number of elements at beginning of file
	lseek(bounds->fd, bounds->start_el*EL_SIZE+sizeof(uint64_t), SEEK_SET);

	pthread_mutex_unlock(&file);


	free(bounds);
	return NULL;
}