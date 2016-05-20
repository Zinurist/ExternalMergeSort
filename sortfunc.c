#include "sortfunc.h"

pthread_mutex_t file_lock;

void * simple_sort(void * arg){

	thread_info* info = (thread_info*) arg;

	//assert: (end_el-start_el) % SIMPLE_SORT_NUM = 0, unless its the last thread

	size_t size = info->end - info->blocka;
	size = (size/SIMPLE_SORT_NUM)*SIMPLE_SORT_NUM; //pairs of SIMPLE_SORT_NUM elements are sorted

	//TODO store this part in data.block_size/pairs during distribution of simplesort
	uint64_t num_elements = (info->data.end_from - info->data.start_from_a)/EL_SIZE;

	//num of elements that fit into pairs
	uint64_t num_el_fit = (num_elements/SIMPLE_SORT_NUM)*SIMPLE_SORT_NUM;


	uint64_t runs = (num_el_fit/size)+1;
	//TODO

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


int get(int fd, EL_TYPE* block, EL_TYPE* value, EL_TYPE** src, EL_TYPE** limit_block, size_t limit, uint64_t* offset, uint64_t offset_end){
	if((*src) >= (*limit_block)){
		if((*offset) >= offset_end){
			return 1;
		}

		//read
		if(limit > offset_end - (*offset)) limit = offset_end - (*offset);

		pthread_mutex_lock(&file_lock);
		lseek(fd, (*offset), SEEK_SET);
		if(read(fd, block, limit) < limit){
			printf("E\n");
			//apparently this happens sometimes
			//printf("Error in merge sort while reading from file: %s\n", strerror(errno));
			//exit(39);	
		}
		pthread_mutex_unlock(&file_lock);

		*limit_block = block+(limit/EL_SIZE);
		*offset += limit;
		*src = block;

	}
	
	*value = **src;
	(*src)++;
	return 0;
}

void set(int fd, EL_TYPE* block, EL_TYPE value, EL_TYPE** dest, EL_TYPE* limit_block, uint64_t* offset){
	if((*dest) >= limit_block){
		//write
		*offset += flush_rest(fd, block, (*dest), (*offset));
		*dest = block;
	}
	
	**dest = value;
	(*dest)++;

}

size_t flush_rest(int fd, EL_TYPE* block, EL_TYPE* end, uint64_t offset){

	size_t limit = (end - block)*EL_SIZE;

	pthread_mutex_lock(&file_lock);
	lseek(fd, offset, SEEK_SET);
	if(write(fd, block, limit) < limit){
		printf("Error in merge sort while writing to file: %s\n", strerror(errno));
		exit(41);
	}
	pthread_mutex_unlock(&file_lock);


	return limit;
}


void * merge_sort(void* arg){

	thread_info* info = (thread_info*) arg;

	//uint64_t el_per_merge = info->data.block_size*2;
	uint64_t offset_from_a = info->data.start_from_a;
	uint64_t offset_from_b = info->data.start_from_b;
	uint64_t offset_end_a;
	uint64_t offset_end_b;
	uint64_t offset_to = info->data.start_to;
	int reached = 1;
	int last = 0;
	//first time both will be loaded
	//after that: last=1 if a written, last=-1 if b written

	EL_TYPE a,b;
	EL_TYPE* cur_a;
	EL_TYPE* cur_b;
	EL_TYPE* cur_c;

	size_t limit_a, limit_b;
	EL_TYPE* limit_blocka;
	EL_TYPE* limit_blockb;

	limit_a = (info->blockb - info->blocka)*EL_SIZE;
	limit_b = (info->blockc - info->blockb)*EL_SIZE;

	for(uint64_t i = 0; i<info->data.pairs; ){
		reached = 1;
		last = 0;
		cur_c = info->blockc;
		cur_a = 0;
		cur_b = 0;
		limit_blocka = 0;
		limit_blockb = 0;

		offset_end_a = offset_from_a+info->data.block_size;
		offset_end_b = offset_from_b+info->data.block_size;
		//last pair, might have to end earlier!
		if(++i == info->data.pairs){
			if(info->data.end_from <= offset_end_a){
				offset_end_a = info->data.end_from;
				offset_end_b = offset_from_b;
				reached = 3;
			}else /*if(info->data.end_from < offset_end_b)*/{
				offset_end_b = info->data.end_from;
			}
		}

		//first load, after that only one of them is loaded;
		get(info->data.fd_from, info->blocka, &a, &cur_a, &limit_blocka, limit_a, &offset_from_a, offset_end_a);
		get(info->data.fd_from, info->blockb, &b, &cur_b, &limit_blockb, limit_b, &offset_from_b, offset_end_b);

		while(reached){
			switch(reached){
				case 0: break;
				case 1://normal
					if(last >=1 && get(info->data.fd_from, info->blocka, &a, &cur_a, &limit_blocka, limit_a, &offset_from_a, offset_end_a)){
						reached = 2;
						break;
					}
					if(last <=-1 && get(info->data.fd_from, info->blockb, &b, &cur_b, &limit_blockb, limit_b, &offset_from_b, offset_end_b)){
						reached = 3;
						break;
					} 
					if(a<b){
						//printf("set %x, not %x, %d\n", a,b,reached);
						set(info->data.fd_to, info->blockc, a, &cur_c, info->end, &offset_to);
						last = 1;
					}else{
						//printf("set %x, not %x, %d\n", b, a,reached);
						set(info->data.fd_to, info->blockc, b, &cur_c, info->end, &offset_to);
						last = -1;
					}
					break;
				case 2://a exhausted
					set(info->data.fd_to, info->blockc, b, &cur_c, info->end, &offset_to);

					if(get(info->data.fd_from, info->blockb, &b, &cur_b, &limit_blockb, limit_b, &offset_from_b, offset_end_b)){
						reached = 0;
						break;
					}
					break;
				case 3://b exhausted
					set(info->data.fd_to, info->blockc, a, &cur_c, info->end, &offset_to);

					if(get(info->data.fd_from, info->blocka, &a, &cur_a, &limit_blocka, limit_a, &offset_from_a, offset_end_a)){
						reached = 0;
						break;
					}
					break;
			}

		}

		offset_to += flush_rest(info->data.fd_to, info->blockc, cur_c, offset_to);

		//skip the other block
		offset_from_a += info->data.block_size;
		offset_from_b += info->data.block_size;

	}



	return NULL;
}
/*
void * merge_sort(void* arg){

	thread_info* info = (thread_info*) arg;

	uint64_t el_per_merge = info->data.block_size*2;
	uint64_t offset_from_a = info->data.start_from_a;
	uint64_t offset_from_b = info->data.start_from_b;
	uint64_t offset_end_a = offset_from_b;
	uint64_t offset_end_b = info->data.start_from_b + info->data.block_size*EL_SIZE;
	uint64_t offset_to = info->data.start_to;
	//a/b need to be loaded for the first time->set pointer to the end/limits to 0
	EL_TYPE* current_el_a = (EL_TYPE*)1;
	EL_TYPE* current_el_b = (EL_TYPE*)1;
	EL_TYPE* current_el_c = info->blockc;
	//points to end of blocka (smaller than startb, if not filled)
	EL_TYPE* limit_blocka = 0;
	EL_TYPE* limit_blockb = 0;
	//all in bytes
	uint64_t limit_a, limit_b, limit_c, limit_min_a, limit_min_b;
	int reached;
	//0 - normal
	//1 - a has finished (b should be copied over)
	//2 - b has finished
	//TODO make more efficient: instead of continuing with loop, directly copy rest of a/b

	limit_min_a = (info->blockb - info->blocka)*EL_SIZE;
	if(info->data.block_size < limit_min_a) limit_min_a = info->data.block_size;
	limit_min_b = (info->blockc - info->blockb)*EL_SIZE;
	if(info->data.block_size < limit_min_b) limit_min_b = info->data.block_size;

	for(uint64_t i = 0; i<info->data.pairs; i++){
		limit_c = 0;
		reached = 0;

		if(offset_end_a > info->data.end_from){
			offset_end_a = info->data.end_from;
			offset_end_b = info->data.end_from;
		}else if(offset_end_b > info->data.end_from){
			offset_end_b = info->data.end_from;
		}

		for(uint64_t el = 0; el < el_per_merge; el++){
			//TODO more efficient: dont check every round (maybe using nested loops for write c?)
			//read a
			if(current_el_a >= limit_blocka){
				limit_a = offset_end_a - offset_from_a;
				if(limit_min_a < limit_a) limit_a = limit_min_a;
				if(offset_end_a <= offset_from_a){
					if(reached == 2) break;
					reached = 1;
				}else{
					pthread_mutex_lock(&file_lock);
					lseek(info->data.fd_from, offset_from_a, SEEK_SET);
					if(read(info->data.fd_from, info->blocka, limit_a) < limit_a){
						//printf("Error in merge sort while reading from file (a): %s\n", strerror(errno));
						//exit(39);	
					}
					pthread_mutex_unlock(&file_lock);
					current_el_a = info->blocka;
					limit_blocka = current_el_a + limit_a/EL_SIZE;
					offset_from_a += limit_a;
				}
			}

			//read b
			if(current_el_b >= limit_blockb){
				limit_b = offset_end_b - offset_from_b;				
				if(limit_min_b < limit_b) limit_b = limit_min_b;
				if(offset_end_b <= offset_from_b){
					if(reached == 1) break;
					reached = 2;
				}else{
					pthread_mutex_lock(&file_lock);
					lseek(info->data.fd_from, offset_from_b, SEEK_SET);
					if(read(info->data.fd_from, info->blockb, limit_b) < limit_b){
						//printf("Error in merge sort while reading from file (b): %s\n", strerror(errno));
						//exit(40);	
					}
					pthread_mutex_unlock(&file_lock);
					current_el_b = info->blockb;
					limit_blockb = current_el_b + limit_b/EL_SIZE;
					offset_from_b += limit_b;
				}
			}

			//write c
			if(current_el_c >= info->end){
				pthread_mutex_lock(&file_lock);
				lseek(info->data.fd_to, offset_to, SEEK_SET);
				if(write(info->data.fd_to, info->blockc, limit_c) < limit_c){
					printf("Error in merge sort while writing to file: %s\n", strerror(errno));
					exit(41);
				}
				pthread_mutex_unlock(&file_lock);
				offset_to += limit_c;
			}

			//actual merging:
			switch(reached){
				case 0:
					if(*current_el_a < *current_el_b){
						*current_el_c = *current_el_a;
						current_el_a++;
					}else{
						*current_el_c = *current_el_b;
						current_el_b++;
					}
					break;
				case 1:
					*current_el_c = *current_el_b;
					current_el_b++;
					break;
				case 2:
					*current_el_c = *current_el_a;
					current_el_a++;
					break;
			}
			

			current_el_c++;
			limit_c += EL_SIZE;

		}

		//write rest of c
		pthread_mutex_lock(&file_lock);
		lseek(info->data.fd_to, offset_to, SEEK_SET);
		if(write(info->data.fd_to, info->blockc, limit_c) < limit_c){
			printf("Error in merge sort while writing to file: %s\n", strerror(errno));
			exit(42);
		}
		pthread_mutex_unlock(&file_lock);
		offset_to += limit_c;


		//skip block-offsets to next pair, 1 block already skipped by adding limit to offset above
		offset_from_a += info->data.block_size*EL_SIZE;
		offset_from_b += info->data.block_size*EL_SIZE;
		offset_end_b += info->data.block_size*EL_SIZE*2;
		offset_end_b += info->data.block_size*EL_SIZE*2;

	}



	return NULL;
}
*/



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