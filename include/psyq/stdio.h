#ifndef XENO_PSX_STDIO_H
#define XENO_PSX_STDIO_H

/* Console entry points used by the matching CD diagnostics and implemented
 * by src/slus_006.64/psyq/libc2/puts.c and the resident BIOS wrappers. */
void putchar(char ch);
void puts(char *text);
int printf(const char *format, ...);
int write(int handle, const void *data, int bytes);

#endif
