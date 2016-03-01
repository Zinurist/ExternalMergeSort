#include "generator.h"

int generate(int fd, EL_TYPE *buffer, size_t size,  uint64_t num_create){
	int written = 0;
	written = write(fd, &num_create, sizeof(uint64_t));

	if(written <=0 ){
		printf("Error in generate: %s\n", strerror(errno));
		return 6;
	}



	uint64_t blocks = num_create/size;
	if(num_create%size != 0){
		blocks++;
	}

	uint64_t limit, offset;
	offset = 0;

	srand(time(NULL));
	for(uint64_t i=0; i<blocks; i++){
		if(num_create>size){
			limit = size;
		}else{
			limit = num_create;
		}

		for(uint64_t k=0; k<limit; k++){
			buffer[offset+k] = rand();
		}

		written = write(fd, buffer, EL_SIZE*limit);

		if(written <=0 ){
			printf("Error in generate: %s\n", strerror(errno));
			return 7;
		}

		offset+=limit;
		num_create-=limit;
	}

	return 0;
}