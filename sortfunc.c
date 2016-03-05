#include "sortfunc.h"

pthread_mutex_t file_lock;

void * simple_sort(void * arg){


	thread_info* info = (thread_info*) arg;
	simple_arg* bounds = (simple_arg*)info->data;

	//assert: (end_el-start_el) % SIMPLE_SORT_NUM = 0, unless its the last thread
	//printf("%i, %lu, %lu\n", bounds->fd, bounds->start_el, bounds->end_el);

	uint64_t size = info->end - info->blocka;
	size = (size/SIMPLE_SORT_NUM)*SIMPLE_SORT_NUM; //pairs of SIMPLE_SORT_NUM elements are sorted

	uint64_t num_elements = bounds->end_el - bounds->start_el;

	if(num_elements%SIMPLE_SORT_NUM != 0){
		//TODO enable sort of the last few elements
	}
	num_elements = (num_elements/SIMPLE_SORT_NUM)*SIMPLE_SORT_NUM;//TODO remove this


	uint64_t runs = (num_elements/size)+1;
	//add sizeof(uint64_t), because of number of elements at beginning of file
	uint64_t offset = bounds->start_el*EL_SIZE+sizeof(uint64_t);
	uint64_t limit = 0;
	EL_TYPE* current_el;

	for(int i=0; i<runs; i++){
		if(num_elements >= size){
			limit = size;
		}else{
			limit = num_elements;
			if(limit == 0) break;
		}

		//---read from file---
		pthread_mutex_lock(&file_lock);

		lseek(bounds->fd, offset, SEEK_SET);
		if(read(bounds->fd, info->blocka, limit*EL_SIZE) < limit*EL_SIZE){
			printf("Error in simple sort while reading from file: %s\n", strerror(errno));
			exit(20);
		}
		current_el = info->blocka;

		pthread_mutex_unlock(&file_lock);
		//--------------------


		//---sorting---
		for(int k=0; k<limit; k+=SIMPLE_SORT_NUM){
			quick_sort(current_el, SIMPLE_SORT_NUM);
			//printf("after: %x, %x, %x, %x\n", current_el[0],current_el[1],current_el[2],current_el[3]);
			current_el += SIMPLE_SORT_NUM;
		}
		//-------------



		//---write to file---
		pthread_mutex_lock(&file_lock);

		lseek(bounds->fd, offset, SEEK_SET);
		if(write(bounds->fd, info->blocka, limit*EL_SIZE) < limit*EL_SIZE){
			printf("Error in simple sort while writing to file: %s\n", strerror(errno));
			exit(21);
		}

		pthread_mutex_unlock(&file_lock);
		//-------------------

		offset += limit*EL_SIZE;
		num_elements -= limit;
	}


	free(bounds);
	return NULL;
}


void quick_sort(EL_TYPE* buffer, size_t size){
	if(size <=1){
		return;
	}

	EL_TYPE pivot = buffer[0];
	EL_TYPE tmp;
	EL_TYPE *left, *right;
	left = buffer+1;
	right = buffer+size-1;

	while(1){
		while(*right >= pivot && left<right) right--;
		while(*left < pivot && left<right) left++;

		if(left == right){
			//done
			if(*left<pivot){
				buffer[0] = *left;
				*left = pivot;
				quick_sort(buffer, left-buffer);
				quick_sort(left+1, (buffer+size)-left-1);
			}else{
				quick_sort(left, size-1);
			}
			return;
		}else{
			tmp = *left;
			*left = *right;
			*right = tmp;
		}


	}



}