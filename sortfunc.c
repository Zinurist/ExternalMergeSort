#include "sortfunc.h"

pthread_mutex_t file_lock;

void * simple_sort(void * arg){

	thread_info* info = (thread_info*) arg;

	//assert: (end_el-start_el) % SIMPLE_SORT_NUM = 0, unless its the last thread

	size_t size = info->end - info->blocka;
	size = (size/SIMPLE_SORT_NUM)*SIMPLE_SORT_NUM; //pairs of SIMPLE_SORT_NUM elements are sorted

	uint64_t num_elements = (info->data.end_from - info->data.start_from_a)/EL_SIZE;

	//num of elements that fit into pairs
	uint64_t num_el_fit = (num_elements/SIMPLE_SORT_NUM)*SIMPLE_SORT_NUM;


	uint64_t runs = (num_el_fit/size)+1;
	//add sizeof(uint64_t), because of number of elements at beginning of file
	uint64_t offset_from = info->data.start_from_a;
	uint64_t offset_to = info->data.start_to;
	uint64_t limit = 0;
	EL_TYPE* current_el;

	for(uint64_t i=0; i<runs; i++){
		if(num_elements >= size){
			limit = size;
		}else{
			limit = num_elements;
			if(limit == 0) break;
		}

		//---read from file---
		pthread_mutex_lock(&file_lock);

		lseek(info->data.fd_from, offset_from, SEEK_SET);
		if(read(info->data.fd_from, info->blocka, limit*EL_SIZE) < limit*EL_SIZE){
			printf("Error in simple sort while reading from file: %s\n", strerror(errno));
			exit(20);
		}
		current_el = info->blocka;

		pthread_mutex_unlock(&file_lock);
		//--------------------


		//---sorting---
		int num_to_sort;
		for(int k=limit; k>0; k -= num_to_sort){
			//k is the amount of elements in the buffer, that still need to be sorted
			//instead of count from 0 to limit, count the other way -> no subtracting in the if-clause below needed

			//if+else should be faster than modulo
			//num_to_sort = limit % SIMPLE_SORT_NUM;
			if(k < SIMPLE_SORT_NUM)	num_to_sort = k;
			else 					num_to_sort = SIMPLE_SORT_NUM;
			quick_sort(current_el, num_to_sort);
			//printf("after: %x, %x, %x, %x\n", current_el[0],current_el[1],current_el[2],current_el[3]);
			current_el += num_to_sort;
		}
		//-------------



		//---write to file---
		pthread_mutex_lock(&file_lock);

		lseek(info->data.fd_to, offset_to, SEEK_SET);
		if(write(info->data.fd_to, info->blocka, limit*EL_SIZE) < limit*EL_SIZE){
			printf("Error in simple sort while writing to file: %s\n", strerror(errno));
			exit(21);
		}

		pthread_mutex_unlock(&file_lock);
		//-------------------

		offset_from += limit*EL_SIZE;
		offset_to += limit*EL_SIZE;
		num_elements -= limit;
	}


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




void * merge_sort(void* arg){

	//thread_info* info = (thread_info*) arg;


	return NULL;
}




int is_sorted(int fd, EL_TYPE *buffer, size_t size){

	uint64_t num_elements;
	if(read(fd, &num_elements, sizeof(uint64_t)) < sizeof(uint64_t)){
		printf("Error when reading number of elements (is_sorted): %s\n",strerror(errno));
		return 61;
	}

	uint64_t runs = (num_elements/size)+1;

	uint64_t limit;

	for(uint64_t i=0; i<runs; i++){
		if(num_elements >= size){
			limit = size;
		}else{
			limit = num_elements;
			if(limit == 0) break;
		}

		if(read(fd, buffer, EL_SIZE*limit) < EL_SIZE*limit){
			printf("Error in is_sorted while reading: %s\n", strerror(errno));
			return 7;
		}

		for(uint64_t k = 1; k<limit; k++){
			if(buffer[k-1] > buffer[k]){
				printf("Not sorted! (at %zu)\n", i*size+k);
				return 0;
			}
		}

		num_elements -= limit;
	}

	printf("Sorted!\n");
	return 0;
}