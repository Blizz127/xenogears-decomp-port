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
#ifndef XENO_PC_PORT
extern u8 D_800D2CE0[];
#endif
#ifndef XENO_PC_PORT
extern u8 D_800D2CB0[];
#endif


/* func_8007D0CC.s: row[p[1]] = D_800D2CB0[i] for first i<48 with
 * D_800D2CE0[i] == p[2] (0 if none). */
/* Not yet byte-matching: the matching build assembles retail bytes for
 * this function from its own asm segment (see config/battle.yaml).  The
 * body below is kept for the port and the differential harnesses. */
#ifdef XENO_PC_PORT
void func_8007D0CC(u8 **pp, s32 idx) {
    u8 *p = *pp;
    u8 *row = D_800D3430 + ((idx & 0xFF) << 6);
    s32 i;

    row[p[1]] = 0;
    for (i = 0; i < 0x30; i++) {
        if (D_800D2CE0[i] == p[2]) {
            row[p[1]] = D_800D2CB0[i];
            return;
        }
    }
}
#endif /* XENO_PC_PORT */


#ifndef XENO_PC_PORT
extern u8 D_800D3420[];
#endif
#ifndef XENO_PC_PORT
extern u8 D_800D3410[];
#endif
extern u8 g_GameState[];


/* func_8007D148.s */
#if BATTLE_SUB(2)
void func_8007D148(u8** ppBoard, u8* dst, u8 idx, u8 off) {
    u8* p = *ppBoard;
    u16* row = (u16*)(D_800D3420 + (((u32)idx & 0xFF) << 6));
    u16 v = row[p[2]];
    u32 o = ((u32)off & 0xFF) << 3;

    dst[o + p[1]] = (u8)v;
    dst += o + (*ppBoard)[1];
    dst[1] = (u8)(v >> 8);
}
#endif /* BATTLE_SUB(2) */
/* func_8007D1A8.s */
/* Not yet byte-matching: the matching build assembles retail bytes for
 * this function from its own asm segment (see config/battle.yaml).  The
 * body below is kept for the port and the differential harnesses. */
#ifdef XENO_PC_PORT
void func_8007D1A8(u8** ppBoard, u8 index) {
    u8* p = *ppBoard;
    u8* pRow = D_800D3410 + ((u32)index << 6);

    *(u32*)((p[1] << 2) + (u32)pRow) = *(u32*)(g_GameState + 0x1924);
}
#endif /* XENO_PC_PORT */
