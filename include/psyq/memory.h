#ifndef XENO_PSX_MEMORY_H
#define XENO_PSX_MEMORY_H

#ifdef XENO_PC_PORT
#include <stddef.h>
#ifdef _WIN32
void *bzero(void *destination, size_t bytes);
#else
void bzero(void *destination, size_t bytes);
#endif
#else
/* src/slus_006.64/psyq/libc.c */
void bzero(void *destination, int bytes);
#endif

#endif
