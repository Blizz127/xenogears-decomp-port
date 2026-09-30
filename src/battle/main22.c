#include "common.h"
#ifndef BATTLE_TU_SUB
#define BATTLE_TU_SUB 0
#endif
/* Selects one run of byte-exact C bodies when a retail-asm gap splits them;
 * 0 (host/port/tests) compiles every run. */
#define BATTLE_SUB(n) (BATTLE_TU_SUB == 0 || BATTLE_TU_SUB == (n))

#ifndef XENO_PC_PORT
extern u8 D_800D3420[];
extern u8 D_800D3410[];
extern u8 D_800D343F;
extern u8 D_800D342E;
#endif

#ifndef XENO_PC_PORT
static __inline__ u16* BattleCopyRow16(s32 idx) {
    register u16* base asm("$4");
    u32 offset = (idx & 0xFF) << 6;
    base = (u16*)D_800D3420;
    return (u16*)((u8*)base + offset);
}
static __inline__ u32* BattleCopyRow32(s32 idx) {
    register u32* base asm("$4");
    u32 offset = (idx & 0xFF) << 6;
    base = (u32*)D_800D3410;
    return (u32*)((u8*)base + offset);
}
#else
static __inline__ u16* BattleCopyRow16(s32 idx) {
    return (u16*)(D_800D3420 + ((idx & 0xFF) << 6));
}
static __inline__ u32* BattleCopyRow32(s32 idx) {
    return (u32*)(D_800D3410 + ((idx & 0xFF) << 6));
}
#endif


/* func_8007B98C.s: row copy u16: T[p[2]] = T[p[1]] in D_800D3420 row. */
#if BATTLE_SUB(1)
void func_8007B98C(u8 **pp, s32 idx) {
    u8 *p = *pp;
    u16 *t = BattleCopyRow16(idx);

    t[p[2]] = t[p[1]];
}
#endif /* BATTLE_SUB(1) */

/* func_8007B9C8.s: row copy u32: T[p[2]] = T[p[1]] in D_800D3410 row. */
#if BATTLE_SUB(1)
void func_8007B9C8(u8 **pp, s32 idx) {
    u8 *p = *pp;
    u32 *t = BattleCopyRow32(idx);

    t[p[2]] = t[p[1]];
}
#endif /* BATTLE_SUB(1) */







#ifndef XENO_PC_PORT
extern u8 D_800D343F;
#endif
#ifndef XENO_PC_PORT
extern u8 D_800D342E;
#endif


/* func_8007BA88.s */
#if BATTLE_SUB(2)
void func_8007BA88(u8 index) {
    s32 i = 0xF;
    u8* p = (u8*)((u32)&D_800D343F + ((u32)index << 6));

    for (; i >= 0; i--) {
        *p-- = 0;
    }
}
#endif /* BATTLE_SUB(2) */
/* func_8007BAB8.s */
#if BATTLE_SUB(2)
void func_8007BAB8(u8 index) {
    s32 i = 7;
    u16* p = (u16*)((u32)&D_800D342E + ((u32)index << 6));

    for (; i >= 0; i--) {
        *p-- = 0;
    }
}
#endif /* BATTLE_SUB(2) */
