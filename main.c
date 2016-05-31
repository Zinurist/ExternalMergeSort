#include "main.h"

int main(int argc, char *argv[]){
	
	if(argc < 2 || (strcmp(argv[1], "-h")==0) || (strcmp(argv[1], "--help")==0) || argc>8){
		printf("Usage: mergesort <file> [options]\n");
		printf("Options:\n");
		printf("\t -r num : Fill file with num random values. Used for testing.\n");
		printf("\t -c : Check if file is sorted. Used for testing.\n");
		printf("\t -s num : Allocate num MB, used as buffer.\n");
		printf("\t -sb num : Allocate num bytes, used as buffer.\n");
		printf("\t -skb num : Allocate num KB, used as buffer.\n");
		printf("\t -sgb num : Allocate num GB, used as buffer.\n");
		printf("\t -t num : Create max num threads to sort/create.\n");
		printf("-r overrules -c. -t is ignored if using -r/c.");
		return -1;
	}

	char *buffer;
	char *file = argv[1];
	uint64_t size = 1024*1024*DEFAULT_SIZE;
	int num_threads = DEFAULT_THREADS;
	uint64_t num_create = 0;
	int check_sorted = 0;
	char ans;//used when asking for continue

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
			check_sorted = 1;
		}else if(strcmp(argv[i], "-r") == 0){
			i++;
			num_create = strtoll(argv[i], NULL, 10);
			if(num_create<=0){
				printf("Error: Number in create option must be positive!\n");
				return 1;
			}
		}

	}
	//making sure, it's correctly aligned
	size = (size/EL_SIZE)*EL_SIZE;
	if(size < EL_SIZE*num_threads*3*SIMPLE_SORT_SIZE_MIN){//each threads gets 3 blocks of at least SIMPLE_SORT_NUM elements
		printf("Buffer not big enough, aborting.\n");
		return 11;
	}


	//--- Allocate buffer ---
	buffer = malloc(size);
	if(!buffer){
		printf("Error: Couldn't allocate enough memory!\n");
		return 2;
	}
	//-----------------------

	//--- Allocate threads ---
	merge_thread* threads = NULL;
	//checking for multithreading here,
	if( !(num_create>0 || check_sorted) ){
		threads = malloc(sizeof(merge_thread) * num_threads);
		if(!threads){
			printf("Error when allocating threads: %s\n",strerror(errno));
			return 8;
		}
	}
	//------------------------

	#ifdef _POSIX_MEMLOCK 
	//lock buffer memory
	if( (mlock(buffer, size)) & (mlock(threads,size)) ){
		printf("Couldn't lock memory (%s). Pages might get swapped out. Still continue? (y/n, default: y)\n",strerror(errno));
		ans = 'y';
		scanf("%c", &ans);
		if(ans != 'y' && ans != 'Y'){
			printf("Aborting.\n");
			return 0;
		}
	}
	#endif

	//open file
	int fd = -1;
	if(num_create>0){
		if(num_create >= (1024*1024*1024)/EL_SIZE){
			ans = 'n';
			printf("This will create a file of %.2f Gbyte, continue? (y/n, default: n) ", 
				EL_SIZE*num_create/(1024.0*1024.0*1024.0));
			scanf("%c", &ans);
			if(ans != 'y' && ans != 'Y'){
				printf("Aborting.\n");
				return 0;
			}
		}

		fd = open(file, O_RDWR|O_CREAT);
	}else{
		fd = open(file, O_RDWR);
	}
	if(fd < 0){
		printf("Error when reading file: %s\n",strerror(errno));
		return 3;
	}


	int err_code = 0;
	//start routines
	if(check_sorted){
		err_code = is_sorted(fd, (EL_TYPE*)buffer, size/EL_SIZE);
	}else if(num_create>0){
		err_code = generate(fd, (EL_TYPE*)buffer, size/EL_SIZE, num_create);
	}else{
		err_code = start(fd, (EL_TYPE*)buffer, size/EL_SIZE, threads, num_threads);
	}

	close(fd);

	printf("All done!\n");
	return err_code;
}
