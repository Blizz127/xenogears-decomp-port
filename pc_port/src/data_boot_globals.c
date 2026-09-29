/* data_boot_globals.c -- writable movie/archive globals.
 *
 * These are BSS objects in the retail executable.  Keeping them as typed
 * objects in a port-only TU prevents the undefined-symbol stub generator from
 * emitting executable placeholders for globals that StStartRead and MovieMain
 * write during normal boot.
 */

#include "common.h"

s32 D_8005A470;
u16 D_80062514;
