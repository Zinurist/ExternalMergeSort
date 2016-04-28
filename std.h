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


#define EL_TYPE unsigned int //needs to be comparable using > < >= <=
#define EL_SIZE (sizeof(EL_TYPE))
#define SIMPLE_SORT_NUM 8 //needs to be >0

#endif
