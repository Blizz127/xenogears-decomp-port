/* Native owner of the PSX BIOS memory-card file API used by game code.
 *
 * Retail reaches the memory card through the libapi file calls on "buXY:"
 * paths (open/read/write/close/erase/format/rename, firstfile/nextfile).  In
 * the native link the plain names open/read/write/close/rename would bind to
 * the host C library -- with PSX flag values -- so game TUs that talk to the
 * card include this header with XENO_MEMCARD_REMAP defined, which routes those
 * calls to the PcPortCard_* functions.  firstfile/nextfile/erase/format have
 * no host-libc meaning and keep their PSX names.
 *
 * Semantics follow the PSX BIOS (see memcard_port.c for the details and the
 * raw 128 KiB card-image format the backend stores). */
#ifndef XENO_MEMCARD_PORT_H
#define XENO_MEMCARD_PORT_H

struct DIRENTRY;

int PcPortCard_open(const char* name, unsigned int mode);
int PcPortCard_read(int fd, void* buffer, int length);
int PcPortCard_write(int fd, const void* buffer, int length);
int PcPortCard_close(int fd);
int PcPortCard_rename(const char* from, const char* to);
int erase(char* name);
int format(char* name);
struct DIRENTRY* firstfile(char* name, struct DIRENTRY* entry);
struct DIRENTRY* nextfile(struct DIRENTRY* entry);

#ifdef XENO_MEMCARD_REMAP
#define open(name, mode) PcPortCard_open((name), (mode))
#define read(fd, buffer, length) PcPortCard_read((fd), (buffer), (length))
#define write(fd, buffer, length) PcPortCard_write((fd), (buffer), (length))
#define close(fd) PcPortCard_close((fd))
#define rename(from, to) PcPortCard_rename((from), (to))
#endif

#endif
