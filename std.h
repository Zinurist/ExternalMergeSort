#ifndef STD_H
#define STD_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <inttypes.h>

//needs to be comparable using > < >= <=
#define EL_TYPE unsigned short 
#define EL_SIZE (sizeof(EL_TYPE))
//needs to be >0
#define SIMPLE_SORT_NUM 8 

#endif
