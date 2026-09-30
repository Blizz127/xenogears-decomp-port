/* Retail TU misc2 of menu.bin (0x801E3ECC..0x801E8070).  Its source lives in misc.c, part 2;
 * the port builds it from misc.c, so this TU is empty there. */
/* Never compiled: splat decides which per-function .s files to emit from the
 * INCLUDE_ASM uses it finds in this file, and the real ones live in the
 * MENU_PART == 2 half of misc.c.  Keep this list in sync with them. */
#if 0
INCLUDE_ASM("../asm/menu/nonmatchings/main/misc2", func_801E41C0);
INCLUDE_ASM("../asm/menu/nonmatchings/main/misc2", func_801E433C);
INCLUDE_ASM("../asm/menu/nonmatchings/main/misc2", func_801E4754);
INCLUDE_ASM("../asm/menu/nonmatchings/main/misc2", func_801E4998);
INCLUDE_ASM("../asm/menu/nonmatchings/main/misc2", func_801E4A28);
INCLUDE_ASM("../asm/menu/nonmatchings/main/misc2", func_801E4D10);
INCLUDE_ASM("../asm/menu/nonmatchings/main/misc2", func_801E5178);
INCLUDE_ASM("../asm/menu/nonmatchings/main/misc2", func_801E53CC);
INCLUDE_ASM("../asm/menu/nonmatchings/main/misc2", func_801E56E8);
INCLUDE_ASM("../asm/menu/nonmatchings/main/misc2", func_801E5924);
INCLUDE_ASM("../asm/menu/nonmatchings/main/misc2", func_801E5B88);
INCLUDE_ASM("../asm/menu/nonmatchings/main/misc2", func_801E5E4C);
INCLUDE_ASM("../asm/menu/nonmatchings/main/misc2", func_801E61B0);
INCLUDE_ASM("../asm/menu/nonmatchings/main/misc2", func_801E6668);
INCLUDE_ASM("../asm/menu/nonmatchings/main/misc2", func_801E68AC);
INCLUDE_ASM("../asm/menu/nonmatchings/main/misc2", func_801E6B70);
INCLUDE_ASM("../asm/menu/nonmatchings/main/misc2", func_801E6CFC);
INCLUDE_ASM("../asm/menu/nonmatchings/main/misc2", func_801E6F5C);
INCLUDE_ASM("../asm/menu/nonmatchings/main/misc2", func_801E71B4);
INCLUDE_ASM("../asm/menu/nonmatchings/main/misc2", func_801E733C);
INCLUDE_ASM("../asm/menu/nonmatchings/main/misc2", func_801E78C8);
INCLUDE_ASM("../asm/menu/nonmatchings/main/misc2", func_801E7C50);
INCLUDE_ASM("../asm/menu/nonmatchings/main/misc2", func_801E7E68);
#endif

#ifndef XENO_PC_PORT
#define MENU_PART 2
#include "misc.c"
#endif
