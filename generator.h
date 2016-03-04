#ifndef GENERATOR_H
#define GENERATOR_H

#include "std.h"

#include <time.h>

int generate(int fd, EL_TYPE *buffer, size_t size,  uint64_t num_create);

#endif