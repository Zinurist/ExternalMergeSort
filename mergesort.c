#include "mergesort.h"

int start(int fd, EL_TYPE *buffer, size_t size, int num_threads){

	
	int num_elements;
	read(fd, &num_elements, 4);
	printf("%i elements to sort\n",num_elements);

	//create buffer file
	char * tmp = "mergesortXXXXXX";
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




	read(fd, buffer, 4*num_elements);

	for(int i=0; i<num_elements; i++){
		printf("%i\n", buffer[i]);
	}


	return 0;
}
