#include "common.h"


#ifndef XENO_PC_PORT
extern s32 D_800C3D58[];
extern s32 D_80059198[];
extern s32 D_80050100[];
extern u8 D_800C3EB0[];
extern void ClearOTagR(void* ot, s32 n);

/* The 28-byte party/enemy slot records at the head of D_800C3EB0. */
typedef struct {
    u8 unk0[6];
    u8 state;
    u8 unk7;
    u8 alive;
    u8 unk9[0x13];
} BattleSlot1C;

/* Count the live enemy slots (3..10: state < 0x11 and alive) into
 * D_800C3D58, derive D_80059198 = max(count / 2 - 1, 0), park it at
 * D_800C3EB0 + 0x8DAC (clearing D_80059198), set D_80050100 = 2, then flip
 * the draw buffer pointer (+0x8C50) between +0xB70 and +0x4BE0, clear the
 * chosen buffer's OT and copy buffer A's +0x18 colour bytes into B. */
void func_800B88C4(void) {
    s32 i;
    u8* base;
    u8* q;
    u8* buf;
    s32 t;

    i = 3;
    D_800C3D58[0] = 0;
    for (; i != 11; i++) {
        if (((BattleSlot1C*)D_800C3EB0)[i].state < 0x11 && ((BattleSlot1C*)D_800C3EB0)[i].alive != 0) {
            D_800C3D58[0]++;
        }
    }
    D_80059198[0] = D_800C3D58[0] / 2 - 1;
    if (D_80059198[0] < 0) {
        D_80059198[0] = 0;
    }
    base = (u8*)(u32)D_800C3EB0;
    q = base + 0x8000;
    buf = base + 0xB70;
    t = D_80059198[0];
    D_80059198[0] = 0;
    D_80050100[0] = 2;
    *(s32*)(q + 0xDAC) = t;
    if (*(u8**)(q + 0xC50) == buf) {
        buf = base + 0x4BE0;
    }
    *(u8**)(q + 0xC50) = buf;
    *(u8**)(q + 0xC54) = buf + 0x70;
    ClearOTagR(buf + 0x70, 0x1000);
    *(s32*)(q + 0xC84) = 1;
    *(u8**)(q + 0xC50) = base + 0x4BE0;
    q[0xDA8] = 0;
    base[0x4BF8] = base[0xB88];
    base[0x4BF9] = base[0xB89];
    base[0x4BFA] = base[0xB8A];
    base[0x4BFB] = base[0xB8B];
}
#endif


#ifndef XENO_PC_PORT
extern u8* D_800D2D28;
#endif
#ifndef XENO_PC_PORT
extern u8* D_800D2DC8;
#endif


/* func_800B89F4.s */
void func_800B89F4(void) {
}
