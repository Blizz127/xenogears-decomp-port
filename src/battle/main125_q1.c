/* Retail run 1 of main125: the byte-exact C bodies on one side of a
 * retail-asm gap; see the BATTLE_SUB note in main125.c. */
#define BATTLE_TU_SUB 1
#include "common.h"
#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/battle/nonmatchings/main125_q1", func_800BEE2C);
#endif
#include "main125.c"
