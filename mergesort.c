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

	distribute_simple_sort(threads, num_threads, fd);

	distribute_merge_sort(threads, num_threads, fd, fd_buffer);

	return 0;
}

void print_threads(merge_thread* threads, int num_threads, EL_TYPE* buffer, size_t size){
	printf("Format: size: <size in bytes> | <size in elements>, off: <offset in bytes> | <offset in elements>\n");
	for(int i=0; i<num_threads; i++){
		printf("Thread %i:\tBlock A: size: %zu | %zu, off: %lu | %lu\n", i, threads[i].sizea*EL_SIZE, threads[i].sizea, (threads[i].blocka-buffer)*EL_SIZE, threads[i].blocka-buffer);
		printf("\t\tBlock B: size: %zu | %zu, off: %lu | %lu\n", threads[i].sizeb*EL_SIZE, threads[i].sizeb, (threads[i].blockb-buffer)*EL_SIZE, threads[i].blockb-buffer);
		printf("\t\tBlock C: size: %zu | %zu, off: %lu | %lu\n", threads[i].sizec*EL_SIZE, threads[i].sizec, (threads[i].blockc-buffer)*EL_SIZE, threads[i].blockc-buffer);
	}
	printf("Cutoff: %zu elements\n\n", (buffer+size)-(threads[num_threads-1].blockc + threads[num_threads-1].sizec));
}


void distribute_buffer(merge_thread* threads, int num_threads, EL_TYPE *buffer, size_t size){
	size_t default_size = size/num_threads;
	size_t block_size = default_size/3;
	size_t block_size_c = default_size-2*block_size;

	EL_TYPE* offset = buffer;

	for(int i=0; i<num_threads; i++){
		//block c has rest of this threads buffer
		threads[i].sizea = block_size;
		threads[i].sizeb = block_size;
		threads[i].sizec = block_size_c;
		threads[i].blocka = offset;
		threads[i].blockb = offset+block_size;
		threads[i].blockc = offset+2*block_size;

		offset += default_size;
	}

	threads[num_threads-1].sizec = (buffer+size)-threads[num_threads-1].blockc;
}


void distribute_simple_sort(merge_thread* threads, int num_threads, int fd){

}


void distribute_merge_sort(merge_thread* threads, int num_threads, int fd, int fd_buffer){
	
}