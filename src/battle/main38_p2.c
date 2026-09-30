/* Retail TU part 2 of main38.c (see the BATTLE_TU_PART note there): the
 * still-assembly run below, then that file's part-2 C bodies. */
#define BATTLE_TU_PART 2
#include "common.h"
#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/battle/nonmatchings/main38_p2", func_80086028);
INCLUDE_ASM("asm/battle/nonmatchings/main38_p2", func_800861D0);
INCLUDE_ASM("asm/battle/nonmatchings/main38_p2", func_80086B88);
INCLUDE_ASM("asm/battle/nonmatchings/main38_p2", func_80086C88);
INCLUDE_ASM("asm/battle/nonmatchings/main38_p2", func_80086F98);
INCLUDE_ASM("asm/battle/nonmatchings/main38_p2", func_800877E0);
#endif
#include "main38.c"
