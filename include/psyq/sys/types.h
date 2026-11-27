#ifndef XENO_PSX_SYS_TYPES_H
#define XENO_PSX_SYS_TYPES_H

/* Game structures use these BSD integer names. u_long follows unsigned long
 * in the port (see FIELD_CTX_OT3); both are four bytes on the console. */
#ifndef XENO_WINDOWS_PSX_HOST_TYPES
typedef unsigned char u_char;
typedef unsigned short u_short;
typedef unsigned int u_int;
typedef unsigned long u_long;
typedef unsigned int uint;
typedef unsigned long ulong;
#endif

#ifdef __SIZE_TYPE__
typedef __SIZE_TYPE__ size_t;
#else
/* The matching build preprocesses with -undef -nostdinc (MIPS o32). */
typedef unsigned int size_t;
#endif

#endif
