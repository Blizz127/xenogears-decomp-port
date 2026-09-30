#include "common.h"
#ifndef BATTLE_TU_SUB
#define BATTLE_TU_SUB 0
#endif
/* Selects one run of byte-exact C bodies when a retail-asm gap splits them;
 * 0 (host/port/tests) compiles every run. */
#define BATTLE_SUB(n) (BATTLE_TU_SUB == 0 || BATTLE_TU_SUB == (n))

#ifndef XENO_PC_PORT
extern u8 D_800D3430[];
#endif

/* func_8007A9D0.s: saturating u8 add of board operand into battle table cell */
#if BATTLE_SUB(1)
void func_8007A9D0(u8** ppBoard, u8 index) {
    u8* p = *ppBoard;
    u8* row = D_800D3430 + ((u32)index << 6);
    u8* q = row + p[1];
    s32 sum = *q + p[2];
    u8 value = sum;

    /* Retail keeps the saturated result in its own variable (a register copy
     * of the sum) and stores it once, testing the sum with a signed compare. */
    if (sum >= 0x100) {
        value = 0xFF;
    }
    *q = value;
}
#endif /* BATTLE_SUB(1) */
/* func_8007AA1C.s: saturating u8 subtract of board operand from battle table cell */
#if BATTLE_SUB(1)
void func_8007AA1C(u8** ppBoard, u8 index) {
    u8* p = *ppBoard;
    u8* row = D_800D3430 + ((u32)index << 6);
    u8* q = row + p[1];
    s32 diff = *q - p[2];
    u8 value = diff;

    if (diff < 0) {
        value = 0;
    }
    *q = value;
}
#endif /* BATTLE_SUB(1) */
/* func_8007AA60.s: u8 multiply of board operand into battle table cell, saturate at 0xFF */
/* Not yet byte-matching: the matching build assembles retail bytes for
 * this function from its own asm segment (see config/battle.yaml).  The
 * body below is kept for the port and the differential harnesses. */
#ifdef XENO_PC_PORT
void func_8007AA60(u8** ppBoard, u8 index) {
    u8* p = *ppBoard;
    u8* row = D_800D3430 + ((u32)index << 6);
    u8* q = row + p[1];
    s32 prod = p[2] * *q;
    u8 value = prod;
    /* NOT YET EXACT: retail emits `mult cell,p[2]`; every source operand
     * order/cast tried here yields `mult p[2],cell` (1 word, right size). */

    /* The threshold test truncates the product to a signed halfword first. */
    if ((s32)(s16)prod >= 0x100) {
        value = 0xFF;
    }
    *q = value;
}
#endif /* XENO_PC_PORT */

/* func_8007AAB8.s */
#if BATTLE_SUB(2)
void func_8007AAB8(u8** ppBoard, u8 index) {
    u8* p = *ppBoard;
    u8* pRow = D_800D3430 + ((u32)index << 6);

    pRow[p[1]] /= p[2];
}
#endif /* BATTLE_SUB(2) */
/* func_8007AAF4.s */
#if BATTLE_SUB(2)
void func_8007AAF4(u8** ppBoard, u8 index) {
    u8* p = *ppBoard;
    u8* row = D_800D3430 + ((u32)index << 6);
    u8* q = row + p[1];

    *q = *q % p[2];
}
#endif /* BATTLE_SUB(2) */
/* func_8007AB30.s */
#if BATTLE_SUB(2)
void func_8007AB30(u8** ppBoard, u8 index) {
    u8* p = *ppBoard;
    u8* pRow = D_800D3430 + ((u32)index << 6);

    pRow[p[1]] &= p[2];
}
#endif /* BATTLE_SUB(2) */
/* func_8007AB68.s */
#if BATTLE_SUB(2)
void func_8007AB68(u8** ppBoard, u8 index) {
    u8* p = *ppBoard;
    u8* pRow = D_800D3430 + ((u32)index << 6);

    pRow[p[1]] |= p[2];
}
#endif /* BATTLE_SUB(2) */
/* func_8007ABA0.s */
#if BATTLE_SUB(2)
void func_8007ABA0(u8** ppBoard, u8 index) {
    u8* p = *ppBoard;
    u8* pRow = D_800D3430 + ((u32)index << 6);

    pRow[p[1]] ^= p[2];
}
#endif /* BATTLE_SUB(2) */
