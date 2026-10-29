#include "common.h"
#ifdef XENO_PC_PORT
#include "battle_guest_call.h"
#define BATTLE_PHASE_CALL0(fn, addr) BATTLE_GUEST_CALL0(addr)
#else
#define BATTLE_PHASE_CALL0(fn, addr) fn()
#endif
#ifndef XENO_PC_PORT
extern u8 D_800C3E4C;
#endif


#ifndef XENO_PC_PORT
void func_80076418();
void func_800764B4();
void func_800764EC();
#endif

void func_80076544(void) {
    switch (D_800C3E4C) {                           /* irregular */
    case 0:
        BATTLE_PHASE_CALL0(func_800764B4, 0x800764B4u);
        return;
    case 1:
        BATTLE_PHASE_CALL0(func_80076418, 0x80076418u);
        return;
    case 2:
        BATTLE_PHASE_CALL0(func_800764EC, 0x800764ECu);
        return;
    }
}

#ifndef XENO_PC_PORT
extern u8* D_800D2D28;
extern u8 D_800D3280;
extern u8 D_800C3254[];
extern u8 D_800D32A1[];
extern u8 D_800D2D24[];

/* D_800D2D28 scroll-window record (fields used by func_800765C4). */
typedef struct {
    u8 pad0[0x34];
    s32 f34;
    s32 pad38;
    s32 f3C;
    s32 pad40;
    s32 f44;
    s32 pad48;
    s32 f4C;
    s32 pad50;
    s32 f54;
    s32 pad58;
    s32 f5C;
    s32 pad60;
    s32 f64;
    s32 pad68;
    s32 f6C;
    u8 pad70[0x20];
    u8 f90[0x19];
    u8 fA9;
    u8 padAA;
    u8 fAB;
    u8 padAC[0x58];
    u16 f104;
    u16 f106;
} M3Scroll;

/* func_800765C4.s, byte-exact: open the scroll window for slot `index`.
 * The top comes from the D_800C3254 entry for D_800D3280 * 3 + index:
 * entry + 0x48 with height 0x1C, or entry + 0x44 with height 0x24 when
 * D_800D32A1[index * 8] is set and D_800D2D24[index] is not 7, offset by
 * index * 96.  Then the 0x10/0x98 bounds, the 8.8 scroll step f54 =
 * ((top - 5 - f3C) << 8) / span, the speed/state bytes, f90[index]-- and
 * fAB = 1.  The divide sits between the two +0x5C stores; its f54 load is
 * a memory read between them, so both stores survive as in retail. */
void func_800765C4(s32 index) {
    s32 slot = D_800D3280 * 3 + index;
    u16* tbl = (u16*)D_800C3254;
    u16* entry = &tbl[slot];
    M3Scroll* p = (M3Scroll*)D_800D2D28;
    s32 row = index * 96;
    u32 span;

    {
        s32 y = *entry + 0x48;

        p->f44 = 0x1C;
        p->f34 = row + y;
    }
    if (D_800D32A1[index * 8] != 0 && D_800D2D24[index] != 7) {
        s32 y = *entry + 0x44;

        p->f44 = 0x24;
        p->f34 = row + y;
    }
    {
        M3Scroll* q = (M3Scroll*)D_800D2D28;
        s32 top;
        s32 bot;

        q->f3C = 0x10;
        q->f4C = 0x98;
        top = q->f34 - 5;
        q->f54 = top - q->f3C;
        bot = q->f44 + 5;
        span = q->f4C - bot;
        q->f104 = 0x800;
        q->fA9 = 6;
        q->f5C = span;
        q->f54 = (q->f54 << 8) / span;
        q->f5C = 0x100;
        q->f64 = 0;
        q->f6C = 0;
        q->f106 = 0;
    }
    ((M3Scroll*)D_800D2D28)->fAB = 1;
    ((M3Scroll*)D_800D2D28)->f90[index]--;
}

INCLUDE_ASM("asm/battle/nonmatchings/main3", func_80076710);
#endif

#ifndef XENO_PC_PORT
extern u32 D_800D2D40;
#endif
#ifndef XENO_PC_PORT
extern u32 D_800D2D48;
#endif
#ifndef XENO_PC_PORT
extern u8 D_800C3BCC[];
#endif
extern u16 D_8005A3A0[];
#ifndef XENO_PC_PORT
extern u8 D_800D2E62[];
#endif
#ifndef XENO_PC_PORT
extern u16 D_800D39E0;
#endif
#ifndef XENO_PC_PORT
extern u8* D_800D3278;
#endif
#ifndef XENO_PC_PORT
extern u8* D_800D2D28;
#endif
#ifndef XENO_PC_PORT
extern u8 D_800C3E4C;
#endif
#ifndef XENO_PC_PORT
extern u8 D_800D3280;
#endif
#ifndef XENO_PC_PORT
extern u8 D_800C3254[];
#endif
#ifndef XENO_PC_PORT
extern u8 D_800D32A1[];
#endif
#ifndef XENO_PC_PORT
extern u8 D_800D2D24[];
#endif
extern void LoadImage(void* pRect, void* pData);
extern void DrawSync(s32 mode);


/* func_800769E8.s */
void func_800769E8(void* pRect, void* pData) {
    LoadImage(pRect, pData);
    DrawSync(0);
}


#ifdef XENO_PC_PORT
/* Port-side bodies transcribed from the retail listings; the matching build
 * still assembles the INCLUDE_ASM bytes above. */
/* func_800765C4.s: set up the D_800D2D28 scroll window for actor `index`
 * from the D_800C3254 layout table (row D_800D3280), then consume one of the
 * actor's D_800D2D28+0x90 counters.  `index` arrives unmasked, so every
 * address is formed in 32-bit guest arithmetic exactly as retail does. */
void func_800765C4(u32 index) {
    u32 p = BATTLE_G32(0x800D2D28u);
    u32 entry = 0x800C3254u + (((u32)D_800D3280 * 3 + index) << 1);
    u32 row = index * 96;
    u32 span;
    u32 step;

    BATTLE_G32(p + 0x44) = 0x1C;
    BATTLE_G32(p + 0x34) = row + (BATTLE_G16(entry) + 0x48);
    if (BATTLE_G8(0x800D32A1u + (index << 3)) != 0 &&
        BATTLE_G8(0x800D2D24u + index) != 7) {
        BATTLE_G32(p + 0x44) = 0x24;
        BATTLE_G32(p + 0x34) = row + (BATTLE_G16(entry) + 0x44);
    }
    p = BATTLE_G32(0x800D2D28u);
    BATTLE_G32(p + 0x3C) = 0x10;
    BATTLE_G32(p + 0x4C) = 0x98;
    BATTLE_G32(p + 0x54) = BATTLE_G32(p + 0x34) - 5 - BATTLE_G32(p + 0x3C);
    span = BATTLE_G32(p + 0x4C) - (BATTLE_G32(p + 0x44) + 5);
    step = BattleDivu(BATTLE_G32(p + 0x54) << 8, span);
    BATTLE_G16(p + 0x104) = 0x800;
    BATTLE_G8(p + 0xA9) = 6;
    BATTLE_G32(p + 0x5C) = span;
    BATTLE_G32(p + 0x5C) = 0x100;
    BATTLE_G32(p + 0x64) = 0;
    BATTLE_G32(p + 0x6C) = 0;
    BATTLE_G16(p + 0x106) = 0;
    BATTLE_G32(p + 0x54) = step;
    p = BATTLE_G32(0x800D2D28u);
    BATTLE_G8(p + 0xAB) = 1;
    p = BATTLE_G32(0x800D2D28u);
    BATTLE_G8(p + 0x90 + index) = BATTLE_G8(p + 0x90 + index) - 1;
}
#endif
