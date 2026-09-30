#include "common.h"


#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/battle/nonmatchings/main62", func_8009E278);
INCLUDE_ASM("asm/battle/nonmatchings/main62", func_8009E2EC);
#endif
#ifndef XENO_PC_PORT
extern u8* D_800C3E00;
extern u8* D_800C3DFC;
extern u8 D_800C3E50;
extern u8* D_800C34B0;
#endif
/* func_8009E364.s: slot D_800C3E50 of the D_800C34B0 block: byte +0x5FA0 = 2,
 * word +0x5F6C = D_800C3E00[0x5B] * D_800C3DFC[0x11]. */
void func_8009E364(void) {
    u32 prod;

    prod = D_800C3E00[0x5B] * D_800C3DFC[0x11];
    (D_800C34B0 + D_800C3E50)[0x5FA0] = 2;
    ((u32*)(D_800C34B0 + 0x5F6C))[D_800C3E50] = prod;
}
#ifndef XENO_PC_PORT
extern u32 D_800C3D60;
#endif
#ifndef XENO_PC_PORT
extern u8 D_800C3E50;
#endif
#ifndef XENO_PC_PORT
extern u8 D_800CCE4A[];
#endif

#ifdef XENO_PC_PORT
#define BATTLE_C3D60_BYTE D_800C3D60_P
#else
#define BATTLE_C3D60_BYTE ((u8*)D_800C3D60)
#endif

/* func_8009E3C8.s */
void func_8009E3C8(void) {
    *BATTLE_C3D60_BYTE = 4;
    *(u8*)(D_800CCE4A + (u32)D_800C3E50 * 0x170) = 3;
}
#undef BATTLE_C3D60_BYTE


/* ---- Port bodies (fo/bat3).  Not byte-matching: the matching build keeps
 * the retail bytes from the INCLUDE_ASM block above. ---- */
#ifndef XENO_PC_PORT
extern u8* D_800D2DC8;
#endif
#ifdef XENO_PC_PORT
/* func_8009E278.s: current actor (D_800C3E50): state 2 at +0x5FA0 and
 * +0x5F6C word = D_800D2DC8+0x4F x D_800D2DC8+0x64 / 10 (computed first). */
void func_8009E278(void) {
    u32 prod = D_800D2DC8[0x4F] * *(u32*)(D_800D2DC8 + 0x64);

    D_800C34B0[D_800C3E50 + 0x5FA0] = 2;
    *(u32*)(D_800C34B0 + D_800C3E50 * 4 + 0x5F6C) = prod / 10;
}

/* func_8009E2EC.s: as func_8009E278 with D_800C3DFC+0x11 and / 20. */
void func_8009E2EC(void) {
    u32 prod = D_800C3DFC[0x11] * *(u32*)(D_800D2DC8 + 0x64);

    D_800C34B0[D_800C3E50 + 0x5FA0] = 2;
    *(u32*)(D_800C34B0 + D_800C3E50 * 4 + 0x5F6C) = prod / 20;
}
#endif /* XENO_PC_PORT */
