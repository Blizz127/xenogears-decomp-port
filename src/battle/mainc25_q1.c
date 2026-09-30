/* Retail run 1 of mainc25: the byte-exact C bodies on one side of a
 * retail-asm gap; see the BATTLE_SUB note in mainc25.c. */
#define BATTLE_TU_SUB 1
#include "common.h"
#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/battle/nonmatchings/mainc25_q1", func_8007C840);
INCLUDE_ASM("asm/battle/nonmatchings/mainc25_q1", func_8007C9D4);
INCLUDE_ASM("asm/battle/nonmatchings/mainc25_q1", func_8007CB20);
INCLUDE_ASM("asm/battle/nonmatchings/mainc25_q1", func_8007CC50);
INCLUDE_ASM("asm/battle/nonmatchings/mainc25_q1", func_8007CD10);
INCLUDE_ASM("asm/battle/nonmatchings/mainc25_q1", func_8007CDD0);
INCLUDE_ASM("asm/battle/nonmatchings/mainc25_q1", func_8007CEA4);
INCLUDE_ASM("asm/battle/nonmatchings/mainc25_q1", func_8007CFB8);
#endif
#include "mainc25.c"
