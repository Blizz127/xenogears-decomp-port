#include "common.h"
#ifndef BATTLE_TU_SUB
#define BATTLE_TU_SUB 0
#endif
/* Selects one run of byte-exact C bodies when a retail-asm gap splits them;
 * 0 (host/port/tests) compiles every run. */
#define BATTLE_SUB(n) (BATTLE_TU_SUB == 0 || BATTLE_TU_SUB == (n))

#ifndef XENO_PC_PORT
extern u8 D_800D3410[];
#define BATTLE_U32_ROW(idx) ({ \
    register u32 *base asm("$6"); \
    u32 offset = ((idx) & 0xFF) << 6; \
    base = (u32 *)D_800D3410; \
    (u32 *)((u8 *)base + offset); \
})
#else
#define BATTLE_U32_ROW(idx) ((u32 *)(D_800D3410 + (((idx) & 0xFF) << 6)))
#endif


/* func_8007E6A0.s: row[(idx&0xFF)] table add: T[p[3]] = T[p[1]] + T[p[2]]. */
#if BATTLE_SUB(1)
void func_8007E6A0(u8 **pp, s32 idx) {
    u8 *p = *pp;
    u32 *t = BATTLE_U32_ROW(idx);

    t[p[3]] = t[p[1]] + t[p[2]];
}
#endif /* BATTLE_SUB(1) */

/* func_8007E6F0.s: row[(idx&0xFF)] table sub: T[p[3]] = T[p[1]] - T[p[2]]. */
#if BATTLE_SUB(1)
void func_8007E6F0(u8 **pp, s32 idx) {
    u8 *p = *pp;
    u32 *t = BATTLE_U32_ROW(idx);

    t[p[3]] = t[p[1]] - t[p[2]];
}
#endif /* BATTLE_SUB(1) */

#undef BATTLE_U32_ROW

/* func_8007E740.s: row[(idx&0xFF)] table mul: T[p[1]] = T[p[1]] * p[2]. */
/* Not yet byte-matching: the matching build assembles retail bytes for
 * this function from its own asm segment (see config/battle.yaml).  The
 * body below is kept for the port and the differential harnesses. */
#ifdef XENO_PC_PORT
void func_8007E740(u8 **pp, s32 idx) {
    u8 *p = *pp;
    u32 *t = (u32 *)(D_800D3410 + ((idx & 0xFF) << 6));
    t[p[1]] = t[p[1]] * p[2];
}
#endif /* XENO_PC_PORT */

/* func_8007E780.s */
/* MATCHED C, byte-exact with the TU's standard gcc-2.7.2-psx + maspsx
 * (verified instruction-for-instruction against the retail slice at file
 * offset 0xEC90, 64 B: only the %hi/%lo of D_800D3410 are relocations).
 * The `u8 index` parameter is what makes cc1 emit the leading
 * `andi $a1,$a1,0xFF`; the row is `D_800D3410 + ((u32)index << 6)` and the
 * divide targets `pRow[p[1]]`. */
#if BATTLE_SUB(2)
void func_8007E780(u8** ppBoard, u8 index) {
    u8* p = *ppBoard;
    u32* pRow = (u32*)(D_800D3410 + ((u32)index << 6));

    pRow[p[1]] /= p[2];
}
#endif /* BATTLE_SUB(2) */


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
extern void LoadImage(void* pRect, void* pData);
extern void DrawSync(s32 mode);


/* func_8007E7C0.s */
#if BATTLE_SUB(2)
void func_8007E7C0(u8** ppBoard) {
    u8* p = *ppBoard;
    u32 i1 = p[1];
    u32 i2 = p[2];
    u8* pBase = D_800D3278;

    *(u16*)((i1 << 1) + (u32)pBase + 0x394) = (u16)i2;
}
#endif /* BATTLE_SUB(2) */
