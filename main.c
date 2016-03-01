#include "main.h"


int main(int argc, char *argv[]){
	
	if(argc < 2 || (strcmp(argv[1], "-h")==0) || (strcmp(argv[1], "--help")==0) || argc&1 || argc>8){
		printf("Usage: mergesort <file> [options]\n");
		printf("Options:\n");
		printf("\t -c num : Fill file with num random values. Used for testing.\n");
		printf("\t -s num : Allocate num MB, used as buffer.\n");
		printf("\t -sb num : Allocate num bytes, used as buffer.\n");
		printf("\t -skb num : Allocate num KB, used as buffer.\n");
		printf("\t -sgb num : Allocate num GB, used as buffer.\n");
		printf("\t -t num : Create max num threads to sort/create.\n");
		return -1;
	}

	char *buffer;
	char *file = argv[1];
	size_t size = 1024*1024*DEFAULT_SIZE;
	int num_threads = DEFAULT_THREADS;
	int num_create = -1;

	for (int i=2; i<argc; i++){
		if(strcmp(argv[i], "-s") == 0){
			i++;
			size = 1024*1024*atoi(argv[i]);
		}else if(strcmp(argv[i], "-sgb") == 0){
			i++;
			size = 1024*1024*1024*atoi(argv[i]);
		}else if(strcmp(argv[i], "-skb") == 0){
			i++;
			size = 1024*atoi(argv[i]);
		}else if(strcmp(argv[i], "-sb") == 0){
			i++;
			size = atoi(argv[i]);
		}else if(strcmp(argv[i], "-t") == 0){
			i++;
			num_threads = atoi(argv[i]);
		}else if(strcmp(argv[i], "-c") == 0){
			i++;
			num_create = atoi(argv[i]);
			if(num_create<=0){
				printf("Error: Number in create option must be positive!\n");
				return 1;
			}
		}

	}
	//making sure, it's correctly aligned
	size = (size/EL_SIZE)*EL_SIZE;


	//allocate buffer
	buffer = malloc(size);
	if(!buffer){
		printf("Error: Couldn't allocate enough memory!\n");
		return 2;
	}


	//open file
	int fd = -1;
	if(num_create>0){
		fd = open(file, O_RDWR|O_CREAT);
	}else{
		fd = open(file, O_RDWR);
	}
	if(fd < 0){
		printf("Error when reading file: %s\n",strerror(errno));
		return 3;
	}


	//start routines
	if(num_create>0){
		generate(fd, (int*)buffer, size/EL_SIZE, num_create);
	}else{
		//get size of file/number of elements
		struct stat st;
		stat(file, &st);
		size_t file_size = st.st_size;

		//create buffer file
		char * tmp = "mergesortXXXXXX";
		int fd_buffer = -1;
		fd_buffer = mkstemp(tmp);
		unlink(tmp);//->file gets deleted when closed
		if(fd_buffer < 0){
			printf("Error when creating buffer file: %s\n",strerror(errno));
			close(fd);
			return 4;
		}
		//make sure, enough space is available
		if(ftruncate(fd_buffer, file_size/2 + EL_SIZE)){
			printf("Error when truncating buffer file: %s\n",strerror(errno));
			close(fd);
			close(fd_buffer);
			return 5;
		}

		start(fd, fd_buffer, file_size, (int*)buffer, size/EL_SIZE, num_threads);

		close(fd_buffer);
	}

	close(fd);
	return 0;
}
