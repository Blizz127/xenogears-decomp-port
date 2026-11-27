#ifndef XENO_RENDERER_LIBSN_H
#define XENO_RENDERER_LIBSN_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* The renderer's host file API carries a FILE pointer in its handle. */
int PCinit(void);
uintptr_t PCopen(char *path, int flags, int permissions);
int PCclose(uintptr_t handle);
int PCread(uintptr_t handle, char *destination, int length);
int PCwrite(uintptr_t handle, char *source, int length);
int PClseek(uintptr_t handle, int offset, int origin);
int PCcreat(char *path, int permissions);

#ifdef __cplusplus
}
#endif
#endif
