/* Retail TU part 2 of mainc114.c (see the BATTLE_TU_PART note there): the
 * still-assembly run below, then that file's part-2 C bodies. */
#define BATTLE_TU_PART 2
#include "common.h"
#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/battle/nonmatchings/mainc114_p2", func_800BB9D4);
INCLUDE_ASM("asm/battle/nonmatchings/mainc114_p2", func_800BBAB8);
INCLUDE_ASM("asm/battle/nonmatchings/mainc114_p2", func_800BBEE0);
INCLUDE_ASM("asm/battle/nonmatchings/mainc114_p2", func_800BC018);
INCLUDE_ASM("asm/battle/nonmatchings/mainc114_p2", func_800BC158);
#endif
#include "mainc114.c"
