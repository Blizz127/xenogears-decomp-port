/* Retail TU 0x80022FC4..0x80024F64 (sprite/animation tasks), the second of
 * the TUs that were merged into system/temp1.c: it was built with the
 * gcc-2.7.2-cdk compiler (AnimScriptTick and func_800233A4 match only with
 * it, func_80022CAC only without). Its source stays in temp1.c, selected with
 * TEMP1_PART_A2; the port builds it from temp1.c, so this TU is empty there. */
/* Never compiled: splat decides which per-function .s files to emit from the
 * INCLUDE_ASM uses it finds in this file, and the real ones live in the
 * TEMP1_PART_A2 half of temp1.c. Keep this list in sync with them. */
#if 0
INCLUDE_ASM(TEMP1_ASM, func_80023124);
INCLUDE_ASM(TEMP1_ASM, func_80023170);
INCLUDE_ASM(TEMP1_ASM, func_80023290);
INCLUDE_ASM(TEMP1_ASM, func_80023468);
INCLUDE_ASM(TEMP1_ASM, func_800234AC);
INCLUDE_ASM(TEMP1_ASM, func_80023538);
INCLUDE_ASM(TEMP1_ASM, func_80023804);
INCLUDE_ASM(TEMP1_ASM, func_80023A48);
INCLUDE_ASM(TEMP1_ASM, func_80023B84);
INCLUDE_ASM(TEMP1_ASM, func_80023FD8);
INCLUDE_ASM(TEMP1_ASM, func_8002435C);
INCLUDE_ASM(TEMP1_ASM, func_800245D8);
INCLUDE_ASM(TEMP1_ASM, func_80024730);
INCLUDE_ASM(TEMP1_ASM, func_800248D4);
#endif

#ifndef XENO_PC_PORT
#define TEMP1_PART_A2
#include "temp1.c"
#endif
