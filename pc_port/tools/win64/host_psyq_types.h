#ifndef XENO_WINDOWS_PSX_HOST_TYPES
#define XENO_WINDOWS_PSX_HOST_TYPES
#include <sys/types.h>
#include <psx/types.h>
/* Project PsyQ runtime types would redefine the CRT's size_t/time_t.
 * The PC SDK defines all PsyQ aliases. The host CRT owns filesystem types. */
#ifndef _SYS_TYPES_H
#define _SYS_TYPES_H
#endif
#endif
