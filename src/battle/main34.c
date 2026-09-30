#include "common.h"
#ifndef BATTLE_TU_SUB
#define BATTLE_TU_SUB 0
#endif
/* Selects one run of byte-exact C bodies when a retail-asm gap splits them;
 * 0 (host/port/tests) compiles every run. */
#define BATTLE_SUB(n) (BATTLE_TU_SUB == 0 || BATTLE_TU_SUB == (n))


#ifndef XENO_PC_PORT
extern u8* D_800C3EAC;
#endif
#ifndef XENO_PC_PORT
extern u8 D_800C402F[];
#endif
#ifndef XENO_PC_PORT
extern u8 D_800C400B[];
#endif
#ifndef XENO_PC_PORT
extern u8 D_800D366C;
#endif
#ifndef XENO_PC_PORT
extern u8* D_800D3278;
#endif
#ifndef XENO_PC_PORT
extern u8 D_800C204C;
#endif
extern void func_8007FCE8(void);
extern void func_8007FDEC(void);
extern void func_800800E8(u8 index);
extern void func_8007FB70(u8 index);


/* func_80080B64.s */
#if BATTLE_SUB(2)
void func_80080B64(u8 v) {
    u32 idx = (u32)(*(u8*)(D_800C3EAC + 0x2DA));

    D_800C402F[idx * 72] = 0xFE;
    idx = (u32)(*(u8*)(D_800C3EAC + 0x2DA));
    D_800C400B[idx * 72] = v;
    D_800D366C = 0;
}
#endif /* BATTLE_SUB(2) */
/* func_80080BD0.s */
#if BATTLE_SUB(2)
void func_80080BD0(void) {
    D_800C3EAC[0x2DB] = 1;
    if (D_800C3EAC[0x2D3] < 3 && D_800C204C == 0) {
        func_8007FCE8();
        func_8007FDEC();
        func_800800E8(D_800C3EAC[0x2D3]);
        func_8007FB70(D_800C3EAC[0x2D3]);
    }
}
#endif /* BATTLE_SUB(2) */
/* func_80080C6C.s */
#if BATTLE_SUB(2)
void func_80080C6C(u8 index) {
    u32 off = ((u32)index & 0xFF) * 0x38;
    u8* pBase = D_800D3278;

    *(u8*)(pBase + off + 0x34) = 1;
}
#endif /* BATTLE_SUB(2) */
