#include "generator.h"

void generate(int fd, int *buffer, size_t size,  int num_create){
	int written = write(fd, &num_create, 4);
	
	if(written <=0 ){
		printf("Error in generate: %s\n", strerror(errno));
		return;
	}



	int blocks = num_create/size;
	if(num_create%size != 0){
		blocks++;
	}

	int limit, offset;
	offset = 0;

	srand(time(NULL));
	for(int i=0; i<blocks; i++){
		if(num_create>size){
			limit = size;
		}else{
			limit = num_create;
		}

		for(int k=0; k<limit; k++){
			buffer[offset+k] = rand();
		}

		written = write(fd, buffer, 4*limit);

		if(written <=0 ){
			printf("Error in generate: %s\n", strerror(errno));
			return;
		}

		offset+=limit;
		num_create-=limit;
	}

}