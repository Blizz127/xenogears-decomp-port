#ifndef XENO_PORT_COMPAT_H
#define XENO_PORT_COMPAT_H

/*
 * Linux/gcc portability shim for PsyCross (which is Windows-first).
 *
 * PsyCross maps `_stricmp` to `strcasecmp`, but its own include/psx/strings.h
 * shadows the system <strings.h> on the include path, so the POSIX declarations
 * go missing. We force-include this (by absolute path) so it cannot be shadowed.
 */

#ifdef __cplusplus
extern "C" {
#endif

int strcasecmp(const char *a, const char *b);
int strncasecmp(const char *a, const char *b, unsigned long n);

#ifdef __cplusplus
}
#endif

#endif /* XENO_PORT_COMPAT_H */
