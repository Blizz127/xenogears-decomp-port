#include "common.h"


/* func_800AA79C.s */
#ifndef XENO_PC_PORT
extern u32 D_800D3368[];
#define BATTLE_79C_SLOT(index) D_800D3368[index]
#define BATTLE_79C_BYTE(address) ((u8*)(address))
#else
#define BATTLE_79C_SLOT(index) (*(u32*)PSX_ADDR(0x800D3368 + (index) * 4))
#define BATTLE_79C_BYTE(address) ((u8*)PSX_ADDR(address))
#endif
void func_800AA79C(u32 arg0, u32 arg1) {
    u32 a;
    u32 b;
    /* Host seams translate wrapping table slots and guest record addresses;
     * both builds share the retail flag stores and pointer swap. */
    *(BATTLE_79C_BYTE(BATTLE_79C_SLOT(arg0) + 0x34)) = 0;
    *(BATTLE_79C_BYTE(BATTLE_79C_SLOT(arg1) + 0x34)) = 1;
    a = BATTLE_79C_SLOT(arg0);
    b = BATTLE_79C_SLOT(arg1);
    BATTLE_79C_SLOT(arg0) = b;
    BATTLE_79C_SLOT(arg1) = a;
}
#undef BATTLE_79C_SLOT
#undef BATTLE_79C_BYTE


/* func_800AA7DC.s */
#ifndef XENO_PC_PORT
extern u8 D_800C402F[];
#endif
s32 func_800AA7DC(u32 index) {
    u32 match = 0xF7;
    u32 off = index * 0x48;
    u8 v;

    do {
#ifdef XENO_PC_PORT
        /* The walk is unbounded: it runs until it finds a byte that is not
         * 0xF7. Retail's `lbu` wraps inside the 2 MB of RAM, and the
         * interpreter masks every access the same way, but `D_800C402F[off]`
         * on the host indexes straight off the end of the RAM buffer and
         * faults. Mask per access so the host wraps exactly as the hardware
         * and the interpreter do. */
        v = *(u8*)PSX_ADDR(0x800C402F + off);
#else
        v = D_800C402F[off];
#endif
        off += 0x48;
    } while (v == match);
    if (v == 0xFF) {
        v = 0xFE;
    }
    return (s32)v;
}
