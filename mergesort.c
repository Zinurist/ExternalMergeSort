#include "mergesort.h"

void start(int fd, int fd_buffer, size_t file_size, EL_TYPE *buffer, size_t size, int num_threads){

	int num_elements;
	read(fd, &num_elements, 4);
	printf("%i\n",num_elements);

	read(fd, buffer, 4*num_elements);

	for(int i=0; i<num_elements; i++){
		printf("%i\n", buffer[i]);
	}

}
