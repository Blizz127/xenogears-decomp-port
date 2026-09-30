/* Retail run 1 of main35_p3: the byte-exact C bodies on one side of a
 * retail-asm gap; see the BATTLE_SUB note in main35.c. */
#define BATTLE_TU_SUB 1
#include "common.h"
#define BATTLE_TU_PART 3
#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/battle/nonmatchings/main35_p3_q1", func_80084548);
#endif
#include "main35.c"
