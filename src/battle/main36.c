#include "common.h"
/* Retail TU split: gcc 2.7.2 cc1 emits every file-scope INCLUDE_ASM block
 * before the first compiled body, so a mixed TU whose retail layout has a C
 * function ahead of an assembly function cannot be one object.  Each
 * contiguous [asm run][C run] of the retail layout is therefore its own
 * matching TU: src/battle/main36_p<N>.c defines BATTLE_TU_PART=<N> and
 * includes this file.  Host/port builds compile this file whole (part 0). */
#ifndef BATTLE_TU_PART
#define BATTLE_TU_PART 0
#endif
#define BATTLE_PART(n) (BATTLE_TU_PART == 0 || BATTLE_TU_PART == (n))


#ifndef XENO_PC_PORT
extern u8* D_800C3EAC;
#endif
#ifndef XENO_PC_PORT
extern u8 D_800C3FE8[];
#endif

#if BATTLE_PART(1)
/* func_80085388.s: 11 passes (i = 0..10) clearing two u16 slots and
 * setting two byte slots to 0xFF around D_800C3FE8, strided by
 * *(D_800C3EAC + 0x2DA) * 72. Incoming args are all clobbered. */
/* Not yet byte-matching: the matching build assembles retail bytes for
 * this function from its own asm segment (see config/battle.yaml).  The
 * body below is kept for the port and the differential harnesses. */
#ifdef XENO_PC_PORT
void func_80085388(void) {
    s32 i = 0;
    u8* base = D_800C3FE8;
    do {
        u8 v = *(u8*)(D_800C3EAC + 0x2DA);
        *(u16*)(base + (s32)v * 72 + (i << 1)) = 0;
        v = *(u8*)(D_800C3EAC + 0x2DA);
        *(base + 0x18 + (s32)v * 72 + i) = 0xFF;
        v = *(u8*)(D_800C3EAC + 0x2DA);
        *(u16*)((i << 1) + (s32)v * 72 + base + 0x24) = 0;
        v = *(u8*)(D_800C3EAC + 0x2DA);
        *(base + 0x3C + (s32)v * 72 + i) = 0xFF;
        i++;
    } while (i < 11);
}
#endif /* XENO_PC_PORT */
#endif /* BATTLE_PART(1) */
#ifndef XENO_PC_PORT
extern u8 D_800C3FE8[];
#endif
#ifndef XENO_PC_PORT
extern u8 D_800D2C54[];
#endif
#ifndef XENO_PC_PORT
extern u8 D_800D2C88[];
#endif
#ifndef XENO_PC_PORT
extern u8 D_800D2D70[];
#endif
#ifndef XENO_PC_PORT
extern u8 D_800D2D5C[];
#endif
#ifndef XENO_PC_PORT
extern u8 D_800D2D67;
#endif

#if BATTLE_PART(1)
/* func_80085454.s: copy D_800D2C54 (u16 every 4 bytes) and D_800D2C88
 * (bytes) into the D_800C3FE8 table picked by (arg0 & 0xFF) * 72, while
 * folding each D_800D2D70 halfword with its source halfword according to
 * the (D_800D2C88 byte, D_800D2D5C byte) pair, and mirroring the updated
 * pair into the table. 11 passes (a3 = 5C..66); incoming a1 is unused.
 * The OUTER dispatch is a switch -- retail builds an ordered comparison tree
 * (slti against 3), which an if/else-if chain over the same constants does not
 * produce -- while the inner w dispatch stays an if/else-if chain; making both
 * switches overshoots retail by 32 bytes. */
/* Not yet byte-matching: the matching build assembles retail bytes for
 * this function from its own asm segment (see config/battle.yaml).  The
 * body below is kept for the port and the differential harnesses. */
#ifdef XENO_PC_PORT
void func_80085454(s32 arg0) {
    u32 s = (u32)arg0 & 0xFFu;
    u32 t5 = ((s << 3) + s) << 3;
    u8* base = D_800C3FE8 + t5;
    u16* d0 = (u16*)base;
    u8* d18 = base + 0x18;
    u16* d24 = (u16*)(base + 0x24);
    u8* d3c = base + 0x3C;
    u8* a3 = D_800D2D5C;
    s32 k = 0;
    do {
        u16 T = *(u16*)(D_800D2C54 + (s32)(k * 4));
        u16* a2k = (u16*)(D_800D2D70 + (s32)(k * 2));
        u8* a3k = D_800D2D5C + k;
        u8 v1;
        d0[k] = T;
        v1 = *(D_800D2C88 + k);
        d18[k] = v1;
        /* v1 == 1, or v1 >= 3 with v1 != 5, skips the fold. The w == 2
         * arm differs per v1 group (E554 adds, E51C takes |.|), so the
         * dispatch is duplicated like the asm blocks. */
        switch (v1) {
        case 2:
            {
                u8 w = *a3k;

            if (w == 2) {
                *a2k = (u16)(*a2k + T);
            } else if (w == 0 || w == 5) {
                s32 d = (s32)(s16)T - (s32)(s16)(*a2k);

                *a2k = (u16)(d < 0 ? -d : d);
                /* E594 (d < 0) stores no a3 byte; E5A0 (d >= 0) stores 2. */
                if (d >= 0) {
                    *a3k = 2;
                }
            } else {
                *a2k = T;
                *a3k = 2;
            }
            }
            break;
        case 0:
        case 5:
            {
                u8 w = *a3k;

            if (w == 0 || w == 5) {
                *a2k = (u16)(*a2k + T);
            } else if (w == 2) {
                s32 d = (s32)(s16)T - (s32)(s16)(*a2k);

                *a2k = (u16)(d < 0 ? -d : d);
                /* E594 (d < 0) stores no a3 byte; fall-through stores 0. */
                if (d >= 0) {
                    *a3k = 0;
                }
            } else {
                *a2k = T;
                *a3k = v1;
            }
            }
            break;
        }
        d24[k] = *a2k;
        d3c[k] = *a3k;
        k++;
        a3++;
    } while (a3 < &D_800D2D67);
}
#endif /* XENO_PC_PORT */

#endif /* BATTLE_PART(1) */
#ifndef XENO_PC_PORT
extern u8 D_800D2DCC[];
#endif
#ifndef XENO_PC_PORT
extern u8 D_800CCD34[];
#endif
#ifndef XENO_PC_PORT
extern u8 D_800CCD36[];
#endif
#ifndef XENO_PC_PORT
extern u8 D_800CCD64[];
#endif
#ifndef XENO_PC_PORT
extern u8 D_800C4000[];
#endif
#ifndef XENO_PC_PORT
extern u8 D_800CCD38[];
#endif
#ifndef XENO_PC_PORT
extern u8 D_800CCD3A[];
#endif
#ifndef XENO_PC_PORT
extern u8 D_800CCDC4[];
#endif
#ifndef XENO_PC_PORT
extern u8 D_800CCDC6[];
#endif
#ifndef XENO_PC_PORT
extern u8 D_800CCDEC[];
#endif
#ifndef XENO_PC_PORT
extern u8 D_800CCDF0[];
#endif
#ifndef XENO_PC_PORT
extern u8 D_800CCE08[];
#endif
#ifndef XENO_PC_PORT
extern u8 D_800C2050;
#endif
#ifndef XENO_PC_PORT
extern u16 D_800C48E8;
#endif
extern u16 func_80089C08(u8 idx);
extern void func_800883AC(u8 a0);

#if BATTLE_PART(1)
/* func_80085618.s: for s2 in 0..10, gate on D_800D2DCC[s2] and bit15 of
 * D_800CCD64[s2*0x170] (bit15 set: OR func_80089C08(s2) into D_800C48E8,
 * no mark); else dispatch on D_800C4000[72*(a0&0xFF)+s2] (< 12) through
 * six row cases over the D_800CCxx tables (jump table jtbl_80070250).
 * Row MIN stores (case 0 diff > 0) and most other arms land on the mark
 * tail (*(D_800C3EAC + s2 + 0x2EB) = 1); cases 1/9, 3, 4, 6 and d >= 12
 * land on the no-mark tails. Only case 0's diff <= 0 path calls
 * func_800883AC(s2) (for s2 >= 3). The s4 stores address absolute
 * retail scratch (s4+0x8F3C etc., resolved in the test). Retail leaves
 * residue in v0, ignored by the only caller. */
/* Not yet byte-matching: the matching build assembles retail bytes for
 * this function from its own asm segment (see config/battle.yaml).  The
 * body below is kept for the port and the differential harnesses. */
#ifdef XENO_PC_PORT
void func_80085618(s32 a0) {
    u32 A = (u32)a0 & 0xFFu;
    u32 fp = ((A << 3) + A) << 3;
    u8* s7 = D_800C3FE8;
    u8* s3 = s7 + fp;
    u8* s6 = D_800CCDC4;
    u8* s4 = s7 - 0x138;
    u8* s5 = s7 - 0x130;
    u32 s0 = 0;
    u32 s2;

    for (s2 = 0; s2 < 11; s2++, s0 += 0x170, s6 += 0x170, s4 += 0x170,
         s5 += 0x1C, s3 += 2) {
        /* TAIL_SB1 mark; set on every path that falls in (not the two
         * direct shared-tail jumps). */
        int mark = 0;
        if (D_800D2DCC[s2] != 0) {
            u16 h = *(u16*)(D_800CCD64 + s0);
            if ((h & 0x8000u) != 0) {
                /* OR-acc path jumps straight to the no-mark tail. */
                D_800C48E8 = (u16)(D_800C48E8 | func_80089C08((u8)s2));
            } else {
                u8 d = D_800C4000[fp + s2];
                if (d < 12) {
                    switch (d) {
                    case 0:
                    case 5:
                    case 7:
                    case 8: {
                        /* row MIN/DIFF store with s16 compare */
                        if (*s5 != 0) {
                            u16 y = *(u16*)s3;
                            u32 x = *(u32*)(D_800CCDEC + s0);
                            u32 diff = x - y;
                            if ((s32)diff > 0) {
                                /* MIN store jumps to the mark tail: no call. */
                                *(u32*)(D_800CCDEC + s0) = diff;
                                mark = 1;
                                break;
                            } else {
                                *(u32*)(D_800CCDEC + s0) = 0;
                                D_800C48E8 = (u16)(D_800C48E8 |
                                                   func_80089C08((u8)s2));
                                *(u16*)(D_800CCE08 + s0) = (u16)(*(u16*)(D_800CCE08 + s0) | 0x8000u);
                                *(u16*)(D_800CCD64 + s0) = (u16)(h | 0x8000u);
                            }
                        } else {
                            s16 x = *(s16*)(D_800CCD34 + s0);
                            s16 y = *(s16*)s3;
                            s32 diff = (s32)x - (s32)y;
                            if (diff > 0) {
                                /* MIN store jumps to the mark tail: no call. */
                                *(u16*)(D_800CCD34 + s0) = (u16)diff;
                                mark = 1;
                                break;
                            } else {
                                *(u16*)(D_800CCD34 + s0) = 0;
                                D_800C48E8 = (u16)(D_800C48E8 |
                                                   func_80089C08((u8)s2));
                                *(u16*)(D_800CCD64 + s0) = (u16)(h | 0x8000u);
                            }
                        }
                        if (s2 >= 3) {
                            func_800883AC((u8)s2);
                        }
                        mark = 1;
                        break;
                    }
                    case 1:
                    case 9: {
                        s16 x = *(s16*)(D_800CCD38 + s0);
                        s16 y = *(s16*)s3;
                        s32 diff = (s32)x - (s32)y;
                        if (diff > 0) {
                            *(u16*)(D_800CCD38 + s0) = (u16)diff;
                        } else {
                            *(u16*)(D_800CCD38 + s0) = 0;
                        }
                        break;
                    }
                    case 2: {
                        /* both exits land on the mark tail. */
                        if (*s5 != 0 && D_800C2050 == 0) {
                            u16 y = *(u16*)s3;
                            u32 x = *(u32*)(D_800CCDEC + s0);
                            u32 v = x + y;
                            *(u32*)(s4 + 0x8F3C) = v;
                            if (*(u32*)(D_800CCDF0 + s0) < v) {
                                *(u32*)(D_800CCDEC + s0) = *(u32*)(D_800CCDF0 + s0);
                            }
                        } else {
                            u16 x = *(u16*)(D_800CCD34 + s0);
                            u16 y = *(u16*)s3;
                            u16 v = (u16)(x + y);
                            *(u16*)(s4 + 0x8E84) = v;
                            if (*(u16*)(D_800CCD36 + s0) < v) {
                                *(u16*)(D_800CCD34 + s0) = *(u16*)(D_800CCD36 + s0);
                            }
                        }
                        mark = 1;
                        break;
                    }
                    case 3: {
                        if (*s5 == 0 || D_800C2050 != 0) {
                            u16 x = *(u16*)(D_800CCD38 + s0);
                            u16 y = *(u16*)s3;
                            u16 v = (u16)(x + y);
                            *(u16*)(s4 + 0x8E88) = v;
                            if (*(u16*)(D_800CCD3A + s0) >= v) {
                                break;
                            }
                            *(u16*)(D_800CCD38 + s0) = *(u16*)(D_800CCD3A + s0);
                        }
                        break;
                    }
                    case 10: {
                        u16 y = *(u16*)s6;
                        u16 x = *(u16*)s3;
                        u32 diff = (u32)y - (u32)x;
                        if ((s32)diff > 0) {
                            *(u16*)(D_800CCDC4 + s0) = (u16)diff;
                        } else {
                            *(u16*)(D_800CCDC4 + s0) = 0;
                        }
                        mark = 1;
                        break;
                    }
                    case 11: {
                        /* both exits land on the mark tail. */
                        u16 v = (u16)(*(u16*)s6 + *(u16*)s3);
                        *(u16*)(s4 + 0x8F14) = v;
                        if (*(u16*)(D_800CCDC6 + s0) < v) {
                            *(u16*)(D_800CCDC4 + s0) = *(u16*)(D_800CCDC6 + s0);
                        }
                        mark = 1;
                        break;
                    }
                    default:
                        break;
                    }
                }
            }
        }
        if (mark) {
            D_800C3EAC[s2 + 0x2EB] = 1;
        }
    }
}
#endif /* XENO_PC_PORT */
#endif /* BATTLE_PART(1) */
#ifndef XENO_PC_PORT
extern u8 D_800CCD68[];
#endif
#ifndef XENO_PC_PORT
extern u8 D_800CCD36[];
#endif
#ifndef XENO_PC_PORT
extern u8 D_800CCD34[];
#endif
#ifndef XENO_PC_PORT
extern u8 D_800CCD74[];
#endif

#if BATTLE_PART(1)
/* func_80085AC4.s: t = a0 & 0xFF, off = 368*t: copy the halfword
 * D_800CCD36[off] to D_800CCD34[off], then zero D_800CCD68 and D_800CCD74
 * with two countdown loops (i = 2 and i = 4, stepping -2, pointer -4).
 * NOTE: those loops underflow the bases by up to 8 bytes at t = 0; retail
 * genuinely writes there, so the C does too. Retail recomputes the 368-byte
 * stride for the second loop rather than reusing the first offset.
 * void: retail leaves residue in v0, ignored by callers. */
/* Not yet byte-matching: the matching build assembles retail bytes for
 * this function from its own asm segment (see config/battle.yaml).  The
 * body below is kept for the port and the differential harnesses. */
#ifdef XENO_PC_PORT
void func_80085AC4(u8 a0) {
    u32 t;
    u32 off;
    u32 off2;
    s32 i;
    u16* p;

    t = (u32)a0 & 0xFFu;
    off = ((((t << 1) + t) << 3) - t) << 4;
    *(u16*)(D_800CCD34 + off) = *(u16*)(D_800CCD36 + off);
    p = (u16*)(D_800CCD68 + off);
    for (i = 2; i >= 0; i -= 2) {
        *p = 0;
        p = (u16*)((u8*)p - 4);
    }
    t = (u32)a0 & 0xFFu;
    off2 = ((((t << 1) + t) << 3) - t) << 4;
    p = (u16*)(D_800CCD74 + off2);
    for (i = 4; i >= 0; i -= 2) {
        *p = 0;
        p = (u16*)((u8*)p - 4);
    }
}
#endif /* XENO_PC_PORT */

#endif /* BATTLE_PART(1) */
extern u32 func_8009ADA0(u8 idx, u32 *out, u32 a2);
extern void func_80085388(void);
extern void func_80085618(s32 a0);
extern void func_800BE538(u8 a0, u32 a1, u32 a2, u32 a3);

#if BATTLE_PART(2)
/* This candidate has not been proven matching. The test adapter supplies
 * the incoming a2 residue explicitly; retail keeps its original ABI. */
#ifdef REMU_HOST_TEST
void func_80085B58(s32 a0, u32 a2save) {
    u32 s4 = (u32)a0;
    u32 s0 = s4 & 0xFFu;
    u32 w[3] = { 0u, 0u, 0u };
    if (func_8009ADA0((u8)s0, w, a2save) == 0) {
        return;
    }
    {
        u32 s2 = s0;
        u16 *s3 = (u16*)(D_800C3FE8 + 2u * s2);
        u32 *wp = w;
        u32 s1;
        for (s1 = 0; s1 < 3; s1++, wp++) {
            u32 v = *wp;
            if (v != 0) {
                u8 *p = D_800C3EAC;
                *(p + 0x2DAu) = 0;
                func_80085388();
                D_800C4000[s2] = (u8)(s1 + 8u);
                *s3 = (u16)v;
                /* $a0 = 0x2DA byte (just zeroed; jal delay is nop). */
                func_80085618((s32)*(D_800C3EAC + 0x2DAu));
            }
        }
        func_800BE538((u8)s4, w[0], w[1], w[2]);
    }
}
#endif
#endif /* BATTLE_PART(2) */


#ifndef XENO_PC_PORT
extern u16 D_800D39DC;
#endif
#ifndef XENO_PC_PORT
extern u16 D_800C48E8;
#endif
#ifndef XENO_PC_PORT
extern u16 D_800D2C94;
#endif
#ifndef XENO_PC_PORT
extern u16 D_800D2C96;
#endif
extern void func_80098C6C(u32 v);
#ifndef XENO_PC_PORT
extern u8* D_800D2D28;
#endif
extern void func_80085454(s32);
extern void func_80085618(s32);


#if BATTLE_PART(2)
/* func_80085C48.s */
void func_80085C48(u32 a0, u32 a1, u32 a2) {
    u16 v = D_800D39DC;

    D_800C48E8 = 0;
    D_800D2C94 = (u16)a1;
    D_800D2C96 = v;
    func_80098C6C(a2 & 0xFFFF);
}
/* func_80085C88.s */
void func_80085C88(u8 index) {
    func_80085454(index);
    func_80085618(index);
    D_800D2D28[0xAD] = 0;
}
#endif /* BATTLE_PART(2) */
