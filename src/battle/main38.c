#include "common.h"
/* Retail TU split: gcc 2.7.2 cc1 emits every file-scope INCLUDE_ASM block
 * before the first compiled body, so a mixed TU whose retail layout has a C
 * function ahead of an assembly function cannot be one object.  Each
 * contiguous [asm run][C run] of the retail layout is therefore its own
 * matching TU: src/battle/main38_p<N>.c defines BATTLE_TU_PART=<N> and
 * includes this file.  Host/port builds compile this file whole (part 0). */
#ifndef BATTLE_TU_PART
#define BATTLE_TU_PART 0
#endif
#define BATTLE_PART(n) (BATTLE_TU_PART == 0 || BATTLE_TU_PART == (n))


#ifndef XENO_PC_PORT
extern u8* D_800C3EAC;
#endif
#ifndef XENO_PC_PORT
extern u32 D_800C3160[];
#endif
#ifndef XENO_PC_PORT
extern u8 D_800CCD3E[];
#endif
#ifndef XENO_PC_PORT
extern u8 D_800D2D24[];
#endif
#ifndef XENO_PC_PORT
extern u32 D_800C31AC[];
#endif
extern u8 g_GameState[];
extern u32 func_80089C6C(u32 a0, u8 a1);

#if BATTLE_PART(1)
/* func_80085EB4.s: if *(D_800C3EAC + 0x2D6) is nonzero and
 * (a0 & 0xFF) == 4, scan the 13 D_800C3160 pointer rows for the first
 * whose 7 bytes match D_800C3EAC[0x2CC..0x2D2]; on a match at row a3,
 * run (12 - a3) addiu steps (jtbl_80070280 falls through into the
 * F9C tail), then: v1 = a1 & 0xFF; v0 = D_800C31AC[D_800CCD3E[v1*368]]
 * + 0xC - (12 - a3); call func_80089C6C(*(g_GameState + 0x16C0 +
 * D_800D2D24[v1]*32), *(u8*)v0); return 1 iff its low half is nonzero.
 * No match (or a failed gate) returns 0. Retail v0 residue is the
 * return, used by the caller. */
/* Not yet byte-matching: the matching build assembles retail bytes for
 * this function from its own asm segment (see config/battle.yaml).  The
 * body below is kept for the port and the differential harnesses. */
#ifdef XENO_PC_PORT
u32 func_80085EB4(s32 a0, s32 a1, u8* a2, s32 a3) {
    u8* eac = D_800C3EAC;
    u32 s0 = 0;

    if (eac[0x2D6] != 0 && (a0 & 0xFF) == 4) {
        u32* rows = D_800C3160;
        s32 match;
        a3 = 0;
        a0 = 0;
        do {
            a2 = (u8*)rows[0];
            do {
                match = 0;
                if (eac[a0 + 0x2CC] != *a2) {
                    break;
                }
                match = 1;
                a0++;
                a2++;
            } while (a0 < 7);
            a0 = 0;
            if (match) {
                break;
            }
            a3++;
            rows++;
        } while (a3 < 13);
        switch (a3) {
        case 0: a0++;
        case 1: a0++;
        case 2: a0++;
        case 3: a0++;
        case 4: a0++;
        case 5: a0++;
        case 6: a0++;
        case 7: a0++;
        case 8: a0++;
        case 9: a0++;
        case 10: a0++;
        case 11: a0++;
        case 12:
            break;
        default:
            return s0;
        }
        if ((func_80089C6C(*(u16*)(g_GameState + 0x16C0 + (u32)D_800D2D24[(a1 & 0xFF)] * 32),
                           *(u8*)(D_800C31AC[D_800CCD3E[(a1 & 0xFF) * 368]] + (12 - a0))) & 0xFFFF) != 0) {
            s0 = 1;
        }
    }
    return s0;
}
#endif /* XENO_PC_PORT */
#endif /* BATTLE_PART(1) */
#if BATTLE_PART(2)
#endif /* BATTLE_PART(2) */


#ifndef XENO_PC_PORT
extern u8 D_800C402F[];
#endif
#ifndef XENO_PC_PORT
extern u8 D_800C400B[];
#endif
#ifndef XENO_PC_PORT
extern u8 D_800C3FFE[];
#endif
extern u16 func_80089C08(u8 idx);


#if BATTLE_PART(2)
/* func_800879A8.s */
void func_800879A8(u8 v, u8 idx) {
    s32 i = 0;
    u8 ff = 0xFF;
    s32 off = 0;

    for (; i < 0x20; i++) {
        D_800C402F[off] = ff;
        D_800C400B[off] = v;
        *(u16*)(D_800C3FFE + off) = func_80089C08(idx & 0xFF);
        off += 0x48;
    }
}
#endif /* BATTLE_PART(2) */
