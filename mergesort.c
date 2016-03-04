#include "mergesort.h"

int start(int fd, EL_TYPE *buffer, size_t size, int num_threads){

	//--- Allocate threads ---
	merge_thread* threads;
	threads = malloc(sizeof(merge_thread) * num_threads);
	if(threads == NULL){
		printf("Error when allocating threads: %s\n",strerror(errno));
		return 8;
	}
	//------------------------

	//--- Allocate buffer file ---
	uint64_t num_elements;
	read(fd, &num_elements, sizeof(uint64_t));
	printf("%llu elements to sort\n",(long long unsigned int)num_elements);

	//create buffer file
	char tmp[16];
	strncpy(tmp, "mergesortXXXXXX", sizeof(tmp));
	int fd_buffer = -1;
	fd_buffer = mkstemp(tmp);
	unlink(tmp);//->file gets deleted when closed
	if(fd_buffer < 0){
		printf("Error when creating buffer file: %s\n",strerror(errno));
		return 4;
	}
	//make sure, enough space is available
	if(ftruncate(fd_buffer, (num_elements/2+1)*EL_SIZE)){
		printf("Error when truncating buffer file: %s\n",strerror(errno));
		close(fd_buffer);
		return 5;
	}
	//----------------------------

	//1. divide into regions for threads
	//2. sort 4 elements normally
	//3. begin merge
	//4. use buffer, copy left data in buffer to original file

	distribute_buffer(threads, num_threads, buffer, size);
	print_threads(threads, num_threads, buffer, size);

	printf("Starting simple sort...\n");

	if(distribute_simple_sort(threads, num_threads, fd, num_elements)){
		close(fd_buffer);
		return 14;
	}

	if(distribute_merge_sort(threads, num_threads, fd, fd_buffer, num_elements)){
		close(fd_buffer);
		return 15;
	}

	return 0;
}

void print_threads(merge_thread* threads, int num_threads, EL_TYPE* buffer, size_t size){
	size_t sizea, sizeb, sizec;
	printf("Format: size: <size in bytes> | <size in elements>, off: <offset in bytes> | <offset in elements>\n");
	for(int i=0; i<num_threads; i++){
		sizea = threads[i].info.blockb-threads[i].info.blocka;
		sizeb = threads[i].info.blockc-threads[i].info.blockb;
		sizec = threads[i].info.end   -threads[i].info.blockc;
		printf("Thread %i:\tBlock A: size: %zu | %zu, off: %lu | %lu\n", i, sizea*EL_SIZE, sizea, (threads[i].info.blocka-buffer)*EL_SIZE, threads[i].info.blocka-buffer);
		printf("\t\tBlock B: size: %zu | %zu, off: %lu | %lu\n", sizeb*EL_SIZE, sizeb, (threads[i].info.blockb-buffer)*EL_SIZE, threads[i].info.blockb-buffer);
		printf("\t\tBlock C: size: %zu | %zu, off: %lu | %lu\n", sizec*EL_SIZE, sizec, (threads[i].info.blockc-buffer)*EL_SIZE, threads[i].info.blockc-buffer);
	}
	//printf("Cutoff: %zu elements\n\n", (buffer+size)-(threads[num_threads-1].info.blockc + threads[num_threads-1].info.sizec));
}


void distribute_buffer(merge_thread* threads, int num_threads, EL_TYPE *buffer, size_t size){
	size_t default_size = size/num_threads;
	size_t block_size = default_size/3;
	size_t block_size_c = default_size-2*block_size;

	EL_TYPE* offset = buffer;

	for(int i=0; i<num_threads; i++){
		//block c has rest of this threads buffer
		//threads[i].info.sizea = block_size;
		//threads[i].info.sizeb = block_size;
		//threads[i].info.sizec = block_size_c;
		threads[i].info.blocka = offset;
		threads[i].info.blockb = offset+block_size;
		threads[i].info.blockc = offset+2*block_size;
		threads[i].info.end = offset+2*block_size+block_size_c;

		offset += default_size;
	}

	//threads[num_threads-1].info.sizec = (buffer+size)-threads[num_threads-1].info.blockc;
	threads[num_threads-1].info.end = buffer+size;
}


int distribute_simple_sort(merge_thread* threads, int num_threads, int fd, uint64_t num_elements){

	//buffer_size for arguments
	size_t buffer_size = sizeof(int)+2*sizeof(uint64_t);//for 1 thread

	//calculating distribution of elements on threads
	uint64_t pairs = num_elements/4;//simple sort sorts 4 elements
	uint64_t el_per_thread = (pairs/num_threads)*4;//last thread has a few more, if not divisble
	uint64_t el_rest = num_elements-(el_per_thread*num_threads);

	//used when writing to thread buffer
	uint64_t start_el = 0;
	uint64_t end_el = 0;
	uint64_t* buffer;

	//create threads
	int err;
	for(int i=0; i<num_threads; i++){
		start_el = end_el;
		end_el = start_el+el_per_thread;
		if(i == num_threads-1){
			end_el += el_rest;
		}

		buffer = malloc(buffer_size);
		if(buffer == NULL){
			printf("Error when allocating thread buffer (simple sort): %s\n", strerror(errno));
			return 1;
		}
		buffer[0] = start_el;
		buffer[1] = end_el;
		*((int*)(buffer+2)) = fd;

		threads[i].info.data = (void*)buffer;
		err = pthread_create(&threads[i].thread, NULL, &simple_sort, &threads[i].info);

		if(err != 0){
			printf("Error when creating thread %i (simple sort): %s\n", i, strerror(err));
			return 1;
		}
	}

	//join threads
	for(int i=0; i<num_threads; i++){
		err = pthread_join(threads[i].thread, NULL);

		if(err != 0){
			printf("Error when joining thread %i (simple sort): %s\n", i, strerror(err));
			return 1;
		}
	}

	return 0;
}


int distribute_merge_sort(merge_thread* threads, int num_threads, int fd, int fd_buffer, uint64_t num_elements){

	return 0;
}