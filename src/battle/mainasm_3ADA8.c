/* battle mainasm_3ADA8: the retail-asm run at file offset 0x3ADA8, in retail order.
 * Functions still listed as INCLUDE_ASM are not byte-matching C yet. The whole
 * file is retail-build only; any port body for these functions lives in the
 * port's own TU. */
#include "common.h"

#ifndef XENO_PC_PORT
/* func_800AA898.s, byte-exact when this TU is built with the BattleNoSched1
 * preset (gears.toml: the Default gcc 2.7.2 settings plus
 * -fno-schedule-insns, applied to mainasm_3ADA8 only).  Under plain -O2 the
 * same body is 6/39.
 * Initialise the record at p: a2 into +0x8, a3 into +0x14, -1 into the
 * +0x3C / +0x98 / +0x3A / +0x1E ids, 0xFF into +0x5C, 0x6B into +0x39, 1 into
 * +0x8E, and clear the other listed fields.  `list` is unused.  The port
 * runs its own body (main72.c).
 * Codegen: retail materialises the -1 once, at the top, and reuses it for
 * the three s16 stores (`s16 m = -1`).  With sched1 on, the once-set m gets
 * the birthing boost and sinks to its first use; with sched1 off the li
 * stays where retail has it.
 * This is an observed match, not proof of the original build settings: the
 * file may instead have had a source shape not yet found that gives the
 * same code under plain -O2.  sched1-off changes 63% of the other matched
 * battle functions, so no other TU may use this preset. */
void func_800AA898(u8* p, s32* list, u8* a2, s32 a3) {
    s16 m = -1;

    *(u16*)(p + 0x3C) = 0xFFFF;
    *(p + 0x5C) = 0xFF;
    *(p + 0x39) = 0x6B;
    *(u8**)(p + 0x8) = a2;
    *(u32*)(p + 0xC) = 0;
    *(u32*)(p + 0x10) = 0;
    *(u32*)(p + 0x14) = a3;
    *(u32*)(p + 0x18) = 0;
    *(p + 0x2B) = 0;
    *(s16*)(p + 0x98) = m;
    *(u16*)(p + 0x58) = 0;
    *(p + 0x35) = 0;
    *(p + 0x37) = 0;
    *(p + 0x38) = 0;
    *(s16*)(p + 0x3A) = m;
    *(u16*)(p + 0x70) = 0;
    *(u16*)(p + 0x72) = 0;
    *(u16*)(p + 0x74) = 0;
    *(u16*)(p + 0x76) = 0;
    *(u16*)(p + 0x78) = 0;
    *(u16*)(p + 0x7A) = 0;
    *(u16*)(p + 0x7C) = 0;
    *(u16*)(p + 0x7E) = 0;
    *(u16*)(p + 0x80) = 0;
    *(u16*)(p + 0x82) = 0;
    *(u16*)(p + 0x84) = 0;
    *(u16*)(p + 0x86) = 0;
    *(u16*)(p + 0x88) = 0;
    *(u16*)(p + 0x8A) = 0;
    *(u16*)(p + 0x8C) = 0;
    *(u16*)(p + 0x8E) = 1;
    *(p + 0x36) = 0;
    *(s16*)(p + 0x1E) = m;
}

#endif
