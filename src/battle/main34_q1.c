/* Retail run 1 of main34: the byte-exact C bodies on one side of a
 * retail-asm gap; see the BATTLE_SUB note in main34.c. */
#define BATTLE_TU_SUB 1
#include "common.h"
#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/battle/nonmatchings/main34_q1", func_80080160);
#endif
#include "main34.c"
