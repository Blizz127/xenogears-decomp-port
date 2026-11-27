#ifndef XENO_PSX_PC_H
#define XENO_PSX_PC_H

/* Native callers use the platform's open flags and seek origins. */
#if defined(XENO_PC_PORT) || defined(__unix__) || defined(_WIN32) || defined(__APPLE__)
#include <fcntl.h>
#include <stdio.h>
#else
/* The freestanding console build opens files read-only, then seeks to the
 * end to get the length or to the start to read. See system/libarchive.c. */
enum { O_RDONLY = 0, SEEK_SET = 0, SEEK_END = 2 };
#endif

#ifdef __cplusplus
extern "C" {
#endif

int PCinit(void);
int PCopen(char *path, int access, int permissions);
int PCcreate(char *path, int permissions);
int PCclose(int handle);
int PClseek(int handle, int offset, int origin);
int PCread(int handle, char *buffer, int length);
int PCwrite(int handle, char *buffer, int length);

#ifdef __cplusplus
}
#endif

#endif
