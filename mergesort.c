#include "mergesort.h"

int start(int fd, EL_TYPE *buffer, size_t size, merge_thread* threads, int num_threads){

	//Init mutex
	if(pthread_mutex_init(&file_lock, NULL) != 0){
        printf("Error when initializing mutex: %s\n",strerror(errno));
        return 55;//schwifty-fyve
    }

	//--- Allocate buffer file ---
	uint64_t num_elements;
	if(read(fd, &num_elements, sizeof(uint64_t)) < sizeof(uint64_t)){
		printf("Error when reading number of elements (start): %s\n",strerror(errno));
		return 9;
	}
	printf("%llu elements to sort\n",(long long unsigned int)num_elements);

	//create buffer file
	char tmp[16];
	strncpy(tmp, "mergesortXXXXXX", sizeof(tmp));
	int fd_buffer = -1;
	fd_buffer = mkstemp(tmp);
	unlink(tmp);//->file gets deleted when closed
	//int fd_buffer = open("test2.txt", O_RDWR|O_CREAT);;
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
	//2. sort SIMPLE_SORT_NUM elements using quicksort
	//3. begin merge
	//4. use buffer, copy left data in buffer to original file

	distribute_buffer(threads, num_threads, buffer, size);
	print_threads(threads, num_threads, buffer, size);

	//maximal/optimal block size for simple (quick) sort
	size_t simple_sort_size = threads[0].info.end - threads[0].info.blocka;

	fprintf(stderr,"Starting simple sort...\n");

	if(distribute_simple_sort(threads, num_threads, fd, fd_buffer, num_elements, simple_sort_size)){
		close(fd_buffer);
		return 14;
	}

	fprintf(stderr,"Starting merge sort...\n");

	if(distribute_merge_sort(threads, num_threads, fd, fd_buffer, num_elements, simple_sort_size, buffer, size)){
		close(fd_buffer);
		return 15;
	}

	close(fd_buffer);
	return 0;
}

void print_threads(merge_thread* threads, int num_threads, EL_TYPE* buffer, size_t size){
	size_t sizea, sizeb, sizec;
	fprintf(stderr,"\nFormat: size: <size in bytes> | <size in elements>, off: <offset in bytes> | <offset in elements>\n");
	for(int i=0; i<num_threads; i++){
		sizea = threads[i].info.blockb-threads[i].info.blocka;
		sizeb = threads[i].info.blockc-threads[i].info.blockb;
		sizec = threads[i].info.end   -threads[i].info.blockc;
		fprintf(stderr,"Thread %i:\tBlock A: size: %zu | %zu, off: %lu | %lu\n", i, sizea*EL_SIZE, sizea, (threads[i].info.blocka-buffer)*EL_SIZE, threads[i].info.blocka-buffer);
		fprintf(stderr,"\t\tBlock B: size: %zu | %zu, off: %lu | %lu\n", sizeb*EL_SIZE, sizeb, (threads[i].info.blockb-buffer)*EL_SIZE, threads[i].info.blockb-buffer);
		fprintf(stderr,"\t\tBlock C: size: %zu | %zu, off: %lu | %lu\n", sizec*EL_SIZE, sizec, (threads[i].info.blockc-buffer)*EL_SIZE, threads[i].info.blockc-buffer);
	}
	fprintf(stderr,"\n");

}

void print_thread_data(merge_thread* threads, int num_threads, int swap){
	if(swap) fprintf(stderr,"\ntemporary -> original\n");
	else fprintf(stderr,"\noriginal -> temporary\n");

	for(int i=0; i<num_threads; i++){
		fprintf(stderr,"Thread %i: ",i);
		
		fprintf(stderr,"\tstart A: %lu, start B: %lu, end: %lu\n", threads[i].info.data.start_from_a, threads[i].info.data.start_from_b, threads[i].info.data.end_from);
		fprintf(stderr,"\t\tto: %lu\n", threads[i].info.data.start_to);
		fprintf(stderr,"\t\tblock_size: %lu, pairs: %lu\n", threads[i].info.data.block_size, threads[i].info.data.pairs);

	}
	fprintf(stderr,"\n");
}


void distribute_buffer(merge_thread* threads, int num_threads, EL_TYPE *buffer, size_t size){
	size_t default_size = size/num_threads;
	//distribution: 1:1:2, since block a/b process half the data of block c
	size_t block_size = default_size/4;
	size_t block_size_c = default_size-2*block_size;

	EL_TYPE* offset = buffer;

	for(int i=0; i<num_threads; i++){
		//block c has rest of this threads buffer

		threads[i].info.blocka = offset;
		threads[i].info.blockb = offset+block_size;
		threads[i].info.blockc = offset+2*block_size;
		threads[i].info.end = offset+2*block_size+block_size_c;

		offset += default_size;
	}

	//old from before commit 3c210e84b1a2dcf1e446181f88a3284f27df5955
	//threads[num_threads-1].info.sizec = (buffer+size)-threads[num_threads-1].info.blockc;
	threads[num_threads-1].info.end = buffer+size;
}


int distribute_simple_sort(merge_thread* threads, int num_threads, int fd, int fd_buffer, uint64_t num_elements, size_t simple_sort_size){

	//calculating distribution of elements on threads
	uint64_t pairs = num_elements/simple_sort_size;//simple sort sorts simple_sort_size elements
	uint64_t el_per_thread = (pairs/num_threads)*simple_sort_size;//last thread has a few more, if not divisible
	uint64_t el_rest = num_elements-(el_per_thread*num_threads);
	//it's possible that the last thread does (num_threads-1) more simple sorts than the other threads!
	//doesn't matter because of small simple_sort_size

	//calculating number of merge phases, needed to determine correct file buffer
	//ld(pairs) = depth of merge tree
	if(num_elements%simple_sort_size !=0) pairs++;
	int even = ceil(log(pairs)/log(2));
	even = even%2;

	//used when writing to thread buffer
	uint64_t start_el = 0;
	uint64_t end_el = 0;

	//create threads
	int err;
	for(int i=0; i<num_threads; i++){
		start_el = end_el;
		end_el = start_el+el_per_thread;
		if(i == num_threads-1){
			end_el += el_rest;
		}

		threads[i].info.data.block_size = simple_sort_size;
		threads[i].info.data.start_from_a = start_el*EL_SIZE+sizeof(uint64_t);
		threads[i].info.data.end_from = end_el*EL_SIZE+sizeof(uint64_t);
		threads[i].info.data.fd_from = fd;
		if(even){
			threads[i].info.data.start_to = start_el*EL_SIZE;
			threads[i].info.data.fd_to = fd_buffer;
		}else{
			threads[i].info.data.start_to = start_el*EL_SIZE+sizeof(uint64_t);
			threads[i].info.data.fd_to = fd;
		}

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


int distribute_merge_sort(merge_thread* threads, int num_threads, int fd, int fd_buffer, uint64_t num_elements, size_t simple_sort_size, EL_TYPE *buffer, size_t size){
	
	uint64_t block_size = simple_sort_size;
	uint64_t pairs, pairs_per_thread, start, end;

	int num_runs = ceil(log(num_elements/(double)simple_sort_size)/log(2));
	fprintf(stderr,"%i merge run(s)\n", (num_runs>0?num_runs:0));

	int swap = num_runs%2;

	//runs
	int err;
	for(int run=0; run<num_runs; run++){
		pairs = (num_elements)/(block_size*2);
		if(num_elements%(block_size*2) != 0) pairs++;

		if(pairs < num_threads){
			num_threads = pairs;
			//redistribute buffer
			distribute_buffer(threads, num_threads, buffer, size);
			fprintf(stderr, "\nRedistributing buffer...");
		}
		fprintf(stderr,"\nStarting run %i: %lu elements/block, %i thread(s), %lu pairs\n", run, block_size, num_threads, pairs);

		end = 0;
		//create threads
		for(int i=0; i<num_threads; i++){

			//pairs per thread calculated for each thread!
			pairs_per_thread = pairs/(num_threads-i);
			if(pairs%(num_threads-i) != 0) pairs_per_thread++;

			//create merge data
			start = end;
			if(i==num_threads-1){
				end = num_elements;//rest of elements
				pairs_per_thread = pairs;//rest of pairs
			}else{
				end = start + pairs_per_thread*2*block_size;
			}

			//calculate rest pairs to distribute
			pairs -= pairs_per_thread;
			//printf("thread %i: start=%lu, end=%lu\n", i, start, end);

			threads[i].info.data.block_size = block_size;
			threads[i].info.data.pairs = pairs_per_thread;
			if(swap){
				//fd_buffer->fd
				threads[i].info.data.start_from_a = start*EL_SIZE;
				threads[i].info.data.start_from_b = threads[i].info.data.start_from_a + block_size*EL_SIZE;
				threads[i].info.data.end_from = end*EL_SIZE;
				threads[i].info.data.start_to = start*EL_SIZE + sizeof(uint64_t);
				
				threads[i].info.data.fd_from = fd_buffer;
				threads[i].info.data.fd_to = fd;
				
			}else{
				//fd->fd_buffer
				threads[i].info.data.start_from_a = start*EL_SIZE + sizeof(uint64_t);
				threads[i].info.data.start_from_b = threads[i].info.data.start_from_a + block_size*EL_SIZE;
				threads[i].info.data.end_from = end*EL_SIZE + sizeof(uint64_t);
				threads[i].info.data.start_to = start*EL_SIZE;
				
				threads[i].info.data.fd_from = fd;
				threads[i].info.data.fd_to = fd_buffer;
				
			}

			err = pthread_create(&threads[i].thread, NULL, &merge_sort, &threads[i].info);

			if(err != 0){
				printf("Error when creating thread %i (merge phase %i): %s\n", i, run, strerror(err));
				return 60;
			}
		}

		print_thread_data(threads, num_threads, swap);

		//join threads
		for(int i=0; i<num_threads; i++){
			err = pthread_join(threads[i].thread, NULL);

			if(err != 0){
				printf("Error when joining thread %i (merge phase %i): %s\n", i, run, strerror(err));
				return 61;
			}
		}

		swap = !swap;
		block_size += block_size;
	}

	return 0;
}