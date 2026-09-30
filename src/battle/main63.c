#include "common.h"


#ifndef XENO_PC_PORT
extern u8* D_800D2DC8;
#endif
#ifndef XENO_PC_PORT
extern u8* D_800C3DFC;
#endif
#ifndef XENO_PC_PORT
extern u8 D_800C3E50;
#endif
#ifndef XENO_PC_PORT
extern u8* D_800C34B0;
#endif
#ifdef XENO_PC_PORT
/* Coexistence: decompiled and logic-faithful, but not byte-exact yet, so the
 * matching build assembles retail bytes below and the port uses this body.
 * Every instruction is reproduced except the destination register of the
 * `mfhi` (retail $v1, every candidate shape $a1) -- see the fifty-third batch
 * note in docs/ai_context/ACTIVE_HANDOFF.md. */
/* func_8009E410.s: slot D_800C3E50 of the D_800C34B0 block: byte +0x5FA0 = 0xA,
 * word +0x5F6C = (*(u16*)(D_800D2DC8 + 0x3A) * D_800C3DFC[0x11]) / 20.
 * Identical to func_8009E48C apart from the opcode byte. */
void func_8009E410(void) {
    s32 q = (*(u16*)(D_800D2DC8 + 0x3A) * D_800C3DFC[0x11]) / 20;

    (D_800C34B0 + D_800C3E50)[0x5FA0] = 0xA;
    ((s32*)(D_800C34B0 + 0x5F6C))[D_800C3E50] = q;
}
#else
INCLUDE_ASM("asm/battle/nonmatchings/main63", func_8009E410);
#endif
#ifdef XENO_PC_PORT
/* Coexistence: decompiled and logic-faithful, but not byte-exact yet, so the
 * matching build assembles retail bytes below and the port uses this body. */
/* func_8009E48C.s: slot D_800C3E50 of the D_800C34B0 block: byte +0x5FA0 = 0xB,
 * word +0x5F6C = (*(u16*)(D_800D2DC8 + 0x3A) * D_800C3DFC[0x11]) / 20. */
void func_8009E48C(void) {
    s32 q = (*(u16*)(D_800D2DC8 + 0x3A) * D_800C3DFC[0x11]) / 20;

    (D_800C34B0 + D_800C3E50)[0x5FA0] = 0xB;
    ((s32*)(D_800C34B0 + 0x5F6C))[D_800C3E50] = q;
}
#else
INCLUDE_ASM("asm/battle/nonmatchings/main63", func_8009E48C);
#endif
#ifndef XENO_PC_PORT
extern u8* D_800D2DC8;
#endif
#ifndef XENO_PC_PORT
extern u8* D_800C3E34;
#endif


/* func_8009E508.s */
void func_8009E508(void) {
    u8* p = D_800D2DC8;
    u8* q = D_800C3E34;

    *(u16*)(p + 0x7C) &= 0xF80B;
    *(u16*)(q + 0x7A) &= 0xFFDF;
}
