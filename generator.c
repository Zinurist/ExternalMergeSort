#include "generator.h"

int generate(int fd, EL_TYPE *buffer, size_t size,  uint64_t num_create){

	if(write(fd, &num_create, sizeof(uint64_t)) < sizeof(uint64_t)){
		printf("Error in generate: %s\n", strerror(errno));
		return 6;
	}



	uint64_t runs = (num_create/size)+1;

	uint64_t limit;

	srand(time(NULL));
	for(uint64_t i=0; i<runs; i++){
		if(num_create >= size){
			limit = size;
		}else{
			limit = num_create;
			if(limit == 0) break;
		}

		for(uint64_t k=0; k<limit; k++){
			buffer[k] = rand();
		}

		if(write(fd, buffer, EL_SIZE*limit) < EL_SIZE*limit){
			printf("Error in generate: %s\n", strerror(errno));
			return 7;
		}

		num_create -= limit;
	}

	return 0;
}