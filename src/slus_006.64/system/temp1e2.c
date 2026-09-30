/* Retail TU 0x80025224..0x80025C04, split out of system/temp1e.c (see the note at its top).
 * Its source stays in temp1e.c, selected with TEMP1E_PART_2; the port builds it from
 * temp1e.c, so this TU is empty there. */
/* Never compiled: splat decides which per-function .s files to emit from the
 * INCLUDE_ASM uses it finds in this file; the real ones live in the
 * TEMP1E_PART_2 part of temp1e.c. Keep this list in sync with them. */
#if 0
INCLUDE_ASM(TEMP1E_ASM, func_80025258);
INCLUDE_ASM(TEMP1E_ASM, func_8002541C);
INCLUDE_ASM(TEMP1E_ASM, func_80025544);
INCLUDE_ASM(TEMP1E_ASM, func_80025718);
INCLUDE_ASM(TEMP1E_ASM, func_800257F0);
INCLUDE_ASM(TEMP1E_ASM, func_80025A88);
#endif

#ifndef XENO_PC_PORT
#define TEMP1E_PART_2
#include "temp1e.c"
#endif
