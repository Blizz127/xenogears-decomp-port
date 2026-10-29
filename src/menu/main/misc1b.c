/* Retail TU misc1b of menu.bin (0x801DBE54..0x801E3ECC, .rodata 0xFC..0x22C).
 * Its source lives in misc.c, part 1b; the port builds it from misc.c, so
 * this TU is empty there.  It exists because cc1 aligns every C jump table
 * to 8 within its TU: the seven input-dispatch tables of this range
 * (jtbl_801C50FC .. jtbl_801C5204, owned by func_801DBE54, DDF24, E05D0,
 * E23CC, E2BE4, E3088 and E36D4) sit at 4 mod 8, so they only land at their
 * retail addresses when this TU's .rodata starts at 0x801C50FC. */
/* Never compiled: splat decides which per-function .s files to emit from the
 * INCLUDE_ASM uses it finds in this file, and the real ones live in the
 * MENU_PART == 4 half of misc.c.  Keep this list in sync with them. */
#if 0
INCLUDE_ASM("../asm/menu/nonmatchings/main/misc1b", func_801DC3D8);
INCLUDE_ASM("../asm/menu/nonmatchings/main/misc1b", func_801DCE60);
INCLUDE_ASM("../asm/menu/nonmatchings/main/misc1b", func_801DD5E8);
INCLUDE_ASM("../asm/menu/nonmatchings/main/misc1b", func_801DE5CC);
INCLUDE_ASM("../asm/menu/nonmatchings/main/misc1b", func_801DF0D4);
INCLUDE_ASM("../asm/menu/nonmatchings/main/misc1b", func_801DF5D0);
INCLUDE_ASM("../asm/menu/nonmatchings/main/misc1b", func_801DF890);
INCLUDE_ASM("../asm/menu/nonmatchings/main/misc1b", func_801DFB68);
INCLUDE_ASM("../asm/menu/nonmatchings/main/misc1b", func_801DFF5C);
INCLUDE_ASM("../asm/menu/nonmatchings/main/misc1b", func_801E05D0);
INCLUDE_ASM("../asm/menu/nonmatchings/main/misc1b", func_801E1418);
INCLUDE_ASM("../asm/menu/nonmatchings/main/misc1b", func_801E31C0);
INCLUDE_ASM("../asm/menu/nonmatchings/main/misc1b", func_801E3C2C);
#endif

#ifndef XENO_PC_PORT
#define MENU_PART 4
#include "misc.c"
#endif
