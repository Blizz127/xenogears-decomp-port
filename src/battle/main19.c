#include "common.h"
#include "battle_guest_abi.h"
#ifndef BATTLE_TU_SUB
#define BATTLE_TU_SUB 0
#endif
/* Selects one run of byte-exact C bodies when a retail-asm gap splits them;
 * 0 (host/port/tests) compiles every run. */
#define BATTLE_SUB(n) (BATTLE_TU_SUB == 0 || BATTLE_TU_SUB == (n))

#ifndef XENO_PC_PORT
extern u8 D_800D3430[];
#endif
#ifndef XENO_PC_PORT
extern u8 D_800D3420[];
#endif


/* func_8007AE38.s: u8 saturating add row[p[3]] = min(row[p[1]] + row[p[2]], 0xFF) */
#if BATTLE_SUB(1)
void func_8007AE38(GSLOT(u8) ppBoard, u8 index) {
    u8* p = GLOAD(u8, ppBoard);
    u8* pRow = D_800D3430 + ((u32)index << 6);
    s32 v = pRow[p[1]] + pRow[p[2]];
    u8 value = v;

    /* Retail keeps the saturated result in its own variable (a register copy of
     * the arithmetic temporary) and tests the temporary with a signed compare. */
    if (v >= 0x100) {
        value = 0xFF;
    }
    pRow[p[3]] = value;
}
#endif /* BATTLE_SUB(1) */
/* func_8007AE98.s: u8 saturating sub row[p[3]] = max(row[p[1]] - row[p[2]], 0) */
#if BATTLE_SUB(1)
void func_8007AE98(GSLOT(u8) ppBoard, u8 index) {
    u8* p = GLOAD(u8, ppBoard);
    u8* pRow = D_800D3430 + ((u32)index << 6);
    s32 v = pRow[p[1]] - pRow[p[2]];
    u8 value = v;

    if (v < 0) {
        value = 0;
    }
    pRow[p[3]] = value;
}
#endif /* BATTLE_SUB(1) */


/* func_8007AF5C.s: u8 row[p[3]] = row[p[1]] / row[p[2]]. Retail emits a bare
 * DIVU (no break 7): a zero divisor stores 0xFF (LO = 0xFFFFFFFF). */
#if BATTLE_SUB(2)
void func_8007AF5C(GSLOT(u8) ppBoard, u8 index) {
    u8* p = GLOAD(u8, ppBoard);
    u8* pRow = D_800D3430 + ((u32)index << 6);

    pRow[p[3]] = GDIVU(pRow[p[1]], pRow[p[2]]);
}
#endif /* BATTLE_SUB(2) */
/* func_8007AFAC.s: u8 remainder; bare DIVU, a zero divisor stores the
 * dividend (HI). */
#if BATTLE_SUB(2)
void func_8007AFAC(GSLOT(u8) ppBoard, u8 index) {
    u8* p = GLOAD(u8, ppBoard);
    u8* pRow = D_800D3430 + ((u32)index << 6);

    pRow[p[3]] = GREMU(pRow[p[1]], pRow[p[2]]);
}
#endif /* BATTLE_SUB(2) */
/* func_8007AFFC.s */
#if BATTLE_SUB(2)
void func_8007AFFC(GSLOT(u8) ppBoard, u8 index) {
    u8* p = GLOAD(u8, ppBoard);
    u8* row = D_800D3430 + ((u32)index << 6);

    row[p[3]] = row[p[1]] & row[p[2]];
}
#endif /* BATTLE_SUB(2) */
/* func_8007B040.s */
#if BATTLE_SUB(2)
void func_8007B040(GSLOT(u8) ppBoard, u8 index) {
    u8* p = GLOAD(u8, ppBoard);
    u8* row = D_800D3430 + ((u32)index << 6);

    row[p[3]] = row[p[1]] | row[p[2]];
}
#endif /* BATTLE_SUB(2) */
/* func_8007B084.s */
#if BATTLE_SUB(2)
void func_8007B084(GSLOT(u8) ppBoard, u8 index) {
    u8* p = GLOAD(u8, ppBoard);
    u8* row = D_800D3430 + ((u32)index << 6);

    row[p[3]] = row[p[1]] ^ row[p[2]];
}
#endif /* BATTLE_SUB(2) */
/* func_8007B0C8.s */
#if BATTLE_SUB(2)
void func_8007B0C8(GSLOT(u8) ppBoard, u8 index) {
    u8* p = GLOAD(u8, ppBoard);
    u16* row = (u16*)(D_800D3420 + ((u32)index << 6));
    s32 v = row[p[1]] + row[p[2]];

    if (v > 0xFFFF) {
        v = 0xFFFF;
    }
    row[p[3]] = (u16)v;
}
#endif /* BATTLE_SUB(2) */
/* func_8007B134.s */
#if BATTLE_SUB(2)
void func_8007B134(GSLOT(u8) ppBoard, u8 index) {
    u8* p = GLOAD(u8, ppBoard);
    u16* row = (u16*)(D_800D3420 + ((u32)index << 6));
    s32 v = row[p[1]] - row[p[2]];

    if (v < 0) {
        v = 0;
    }
    row[p[3]] = (u16)v;
}
#endif /* BATTLE_SUB(2) */
/* func_8007B198.s */
#if BATTLE_SUB(2)
void func_8007B198(GSLOT(u8) ppBoard, u8 index) {
    u8* p = GLOAD(u8, ppBoard);
    u16* row = (u16*)(D_800D3420 + ((u32)index << 6));
    /* Retail MULT keeps the low 32 bits: a product above 0x7FFFFFFF is
     * negative, skips the clamp and stores its low half. */
    s32 v = GMUL32(row[p[1]], row[p[2]]);

    if (v > 0xFFFF) {
        v = 0xFFFF;
    }
    row[p[3]] = (u16)v;
}
#endif /* BATTLE_SUB(2) */
/* func_8007B208.s: u16 quotient; bare DIVU, a zero divisor stores 0xFFFF. */
#if BATTLE_SUB(2)
void func_8007B208(GSLOT(u8) ppBoard, u8 index) {
    u8* p = GLOAD(u8, ppBoard);
    u16* row = (u16*)(D_800D3420 + ((u32)index << 6));

    row[p[3]] = GDIVU(row[p[1]], row[p[2]]);
}
#endif /* BATTLE_SUB(2) */
/* func_8007B264.s: u16 remainder; bare DIVU, a zero divisor stores the
 * dividend. */
#if BATTLE_SUB(2)
void func_8007B264(GSLOT(u8) ppBoard, u8 index) {
    u8* p = GLOAD(u8, ppBoard);
    u16* row = (u16*)(D_800D3420 + ((u32)index << 6));

    row[p[3]] = GREMU(row[p[1]], row[p[2]]);
}
#endif /* BATTLE_SUB(2) */
