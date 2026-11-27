#ifndef XENO_PSX_STDARG_H
#define XENO_PSX_STDARG_H

#ifdef XENO_PC_PORT
#include <stdarg.h>
#else
/* MIPS o32, little endian. GCC homes the variadic argument registers in
 * the caller's stack frame. __builtin_next_arg locates the first slot.
 * See the System V MIPS ABI supplement, "Function Calling Sequence", and
 * GCC Internals, "Implementing the Varargs Macros". */
typedef unsigned char *va_list;

/* Taking the address keeps the last named argument in its home slot under
 * GCC 2.7, as required for the PSX matching build. */
#define va_start(args, last) ((void)&(last), (void)((args) = __builtin_next_arg(last)))
#define va_end(args) ((void)0)

/* Slots occupy whole words. A doubleword-aligned value may skip one slot. */
#define XENO_VA_SLOT_BYTES(type) ((sizeof(type) + 3U) & ~3U)
#define XENO_VA_ALIGNED(args, type) \
    (__alignof__(type) > 4 ? (unsigned char *)(((unsigned long)(args) + 7UL) & ~7UL) : (args))
#define va_arg(args, type) \
    (*(type *)(((args) = XENO_VA_ALIGNED(args, type) + XENO_VA_SLOT_BYTES(type)) \
               - XENO_VA_SLOT_BYTES(type)))
#endif

#endif
