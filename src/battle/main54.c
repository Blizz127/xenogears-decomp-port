#include "common.h"


#ifndef XENO_PC_PORT
extern u8* D_800C34B0;
s32 func_8009A1AC(s32 arg0);
#endif

/* Byte-exact under gcc 2.6.3 (BattleGcc263); the port compiles the same
 * body. */
/* func_8009A0DC.s: status code for actor idx (0x170 stride off D_800C34B0),
 * first match wins over the +0x7C / +0x80 flag words, else an HP test. */
u32 func_8009A0DC(u8 idx) {
    u8* p = D_800C34B0 + idx * 0x170;
    u32 f = *(u16*)(p + 0x7C);
    u32 g;

    if (f & 0x4000) return 8;
    if (f & 0x8000) return 1;
    if (f & 0x2000) return 2;
    g = *(u16*)(p + 0x80);
    if (g & 0x1000) return 3;
    if (g & 0x2000) return 4;
    if (f & 0x800) return 5;
    if (f & 0x1000) return 6;
    if (f & 0x2) return 3;
    if ((*(u16*)(p + 0x84) | *(u16*)(p + 0x86)) & 0x8000) return 7;
    return (*(u16*)(p + 0x4C) < (u32)(*(u16*)(p + 0x4E) >> 3)) ? 5 : 0;
}

#ifndef XENO_PC_PORT

#ifndef XENO_PC_PORT
s32 func_8009A1AC(s32 arg0) {
    u16 temp_v1_2;
    s32 var_a0;
    u16 temp_a2;
    u16 temp_v1;
    u32 temp_v0;
    u32 temp_v0_2;
    void *temp_a1;

    temp_a1 = ((arg0 & 0xFF) * 0x170) + (*(s32*)&D_800C34B0);
    temp_a2 = *(u16 *)((s8*)(temp_a1) + 0x7C);
    var_a0 = 0;
    if (temp_a2 & 0x8000) {
        return 0;
    }
    temp_v1 = *(u16 *)((s8*)(temp_a1) + 0x80);
    if (temp_v1 & 0x1000) {
        var_a0 = 0x8000;
    }
    if (temp_v1 & 0x2000) {
        var_a0 |= 0x4000;
    }
    if (temp_a2 & 0x800) {
        var_a0 |= 0x2000;
    }
    temp_v1_2 = *(u16 *)((s8*)(temp_a1) + 0x8C) | *(u16 *)((s8*)(temp_a1) + 0x8E);
    temp_v0 = temp_v1_2 & 0xF00;
    if (temp_v0 != 0) {
        var_a0 |= temp_v0 >> 3;
    }
    temp_v0_2 = temp_v1_2 & 0xF000;
    if (temp_v0_2 != 0) {
        var_a0 |= temp_v0_2 >> 3;
    }
    return var_a0 & 0xFFFF;
}
#endif

/* func_8009A258.s: byte +0x14 of entry b (0x28 stride) in actor a's 0x5F0
 * table at +0x1058, plus actor a's +0x5B, clamped to 100.  The sum is a u8
 * (retail copies it into the clamp register and masks it for the test and the
 * return), and the +0x1058 table offset is added to a * 0x5F0 before the
 * base, so it stays out of the load offset. */
u32 func_8009A258(u8 a, u8 b) {
    u8* base = D_800C34B0;
    u32 o = a * 0x5F0 + 0x1058;
    u8* t = (u8*)(o + (u32)base) + b * 0x28;
    u8* q = (u8*)(a * 0x170 + (u32)base);
    u8 v;

    v = t[0x14] + q[0x5B];
    if (v >= 0x65) {
        v = 0x64;
    }
    return v;
}
extern u8 D_800CCCE8[];
extern u8 g_GameState[];
extern u8 D_800CCE4A[];
extern u8 D_800CCD1A[];
extern s32 rand(void);

/* func_8009A2D4.s, byte-exact: refresh actor idx's status block at
 * D_800CCCE8 + 0x5F24 from its 0x170-byte record (stats at +0xA4, status
 * at +0x148) and its 0x690-byte entry at +0x2228, and tick the status (the
 * main54.c port arm below does the same).  pct is the share
 * (+0x64 - +0x60) / (+0x64 / 10), at least 1, scaled by +0x54 + 5, zeroed
 * or capped by the flag checks; while the status is 4 its D_800CCE4A
 * timer counts down, and status 3 starts it (3, or 6 with D_800CCD1A
 * 0x40) when g_GameState +0x22B6 has 0x4000 and rand() % 100 < pct.  Retail increments pct uninitialised
 * when +0x64 == +0x60, as written here. */
void func_8009A2D4(u8 idx) {
    u8* rec;
    u8* stats;
    u8* st;
    u8* src;
    u8* dst;
    u8 i;
    u8 pct;
    u32 hi;
    u32 lo;

    rec = D_800CCCE8 + idx * 0x170;
    stats = D_800CCCE8 + 0xA4 + idx * 0x170;
    st = D_800CCCE8 + 0x148 + idx * 0x170;
    src = D_800CCCE8 + 0x2228 + idx * 0x690;
    dst = D_800CCCE8 + 0x5F24;
    for (i = 0; i < 15; i++) {
        ((u16*)dst)[i] = *(u16*)(src + 0x24);
        src += 0x28;
    }
    if (stats[0x57] != 0) {
        *(u16*)(dst + 0x26) = stats[0x57] * *(u16*)dst;
    } else {
        *(u16*)(dst + 0x26) = 0x1E;
    }
    switch (st[0]) {
    case 1:
    case 2:
    case 3:
        *(u16*)(dst + 0x26) += st[0] * 20;
        break;
    case 4:
        *(u16*)(dst + 0x26) *= 10;
        break;
    }
    *(u16*)(dst + 0x1E) = stats[0x3C] * stats[0x3F];
    *(u16*)(dst + 0x1E) += stats[0x12];
    if (rec[0x56] == 4) {
        *(u16*)(dst + 0x1E) += stats[0x22];
    }
    dst[0x29] = stats[0x98];
    dst[0x2A] = stats[0x99];
    hi = *(u32*)(stats + 0x64);
    lo = *(u32*)(stats + 0x60);
    if (hi != lo) {
        pct = (hi - lo) / (hi / 10);
        if (pct == 0) {
            pct++;
        }
    } else {
        pct++;
    }
    pct = pct * (rec[0x54] + 5);
    if (!(*(u16*)(g_GameState + 0x22B6) & 0x4000)) {
        pct = 0;
    }
    if (rec[0x62] < 0x32) {
        pct = 0;
    }
    if (rec[0xA0] == 3) {
        pct = 0;
    }
    if (rec[0xA0] == 0xF) {
        pct = 0x63;
    }
    if (pct >= 0x64) {
        pct = 0x63;
    }
    *(u16*)(dst + 0x2E) = pct;
    *(u16*)(dst + 0x24) = 0;
    if (*(u16*)(stats + 0x7C) & 0x100) {
        *(u16*)(dst + 0x24) = 0x8000;
    }
    if (*(u16*)(stats + 0x7C) & 0x200) {
        *(u16*)(dst + 0x24) |= 0x4000;
    }
    if (*(u16*)(stats + 0x7C) & 0x80) {
        *(u16*)(dst + 0x24) |= 0x2000;
    }
    if (*(u16*)(stats + 0x7C) & 0x10) {
        *(u16*)(dst + 0x24) |= 0x1000;
    }
    if (*(u16*)(stats + 0x38) < (*(u16*)(stats + 0x3A) >> 3)) {
        *(u16*)(dst + 0x24) |= 0x800;
    } else {
        *(u16*)(dst + 0x24) &= 0xF7FF;
    }
    if (*(u16*)(stats + 0x38) == 0) {
        *(u16*)(stats + 0x80) &= 0x7FFF;
        *(u16*)(rec + 0x84) &= 0x7FFF;
    }
    if (*(u16*)(stats + 0x80) & 0x8000) {
        dst[0x2C] = 1;
    } else {
        dst[0x2C] = 0;
    }
    if (st[0] == 4) {
        u32 off;

        dst[0x28] = 4;
        off = idx * 0x170;
        if (--D_800CCE4A[off] == 0) {
            *(u16*)(stats + 0x80) &= 0xBFFF;
            st[0] = 0;
            rec[0x54] = 0;
        }
    } else {
        dst[0x28] = st[0];
        if (st[0] == 3 && (*(u16*)(g_GameState + 0x22B6) & 0x4000)) {
            if (rand() % 100 < pct) {
                u32 off;

                *(u16*)(stats + 0x80) |= 0x4000;
                off = idx * 0x170;
                D_800CCE4A[off] = 3;
                if (*(u16*)(D_800CCD1A + off) & 0x40) {
                    D_800CCE4A[off] = 6;
                }
                st[0]++;
            }
        }
    }
}
#endif


extern u8 g_GameState[];


/* func_8009A7B8.s */
u32 func_8009A7B8(u8 index) {
    return *(u8*)(g_GameState + 0x271 + ((u32)index * 0xA4));
}


/* ---- Port bodies (fo/bat3), compiled only for the port and the
 * differential harness.  The retail build compiles func_8009A258 and
 * func_8009A2D4 from their byte-exact C above. ---- */
#ifndef XENO_PC_PORT
extern u8* D_800C34B0;
#endif
#ifndef XENO_PC_PORT
extern u8 D_800CCCE8[];
#endif
#ifndef XENO_PC_PORT
extern u8 D_800CCE4A[];
#endif
#ifndef XENO_PC_PORT
extern u8 D_800CCD1A[];
#endif
#ifdef XENO_PC_PORT
extern int rand(void);

/* func_8009A258.s: byte +0x14 of entry b (0x28 stride) in actor a's 0x5F0
 * table at +0x1058, plus actor a's +0x5B, clamped to 100 on the low byte. */
u32 func_8009A258(u8 a, u8 b) {
    u8* base = D_800C34B0;
    u32 v = base[a * 0x5F0 + 0x1058 + b * 0x28 + 0x14] + base[a * 0x170 + 0x5B];

    if ((v & 0xFF) >= 0x65) {
        v = 0x64;
    }
    return v & 0xFF;
}

/* R3000 divu: a zero divisor leaves LO = 0xFFFFFFFF (no trap). */
static u32 bat3_divu(u32 a, u32 b) {
    return (b == 0) ? 0xFFFFFFFFu : a / b;
}

/* func_8009A2D4.s: refresh the per-actor status block at D_800CCCE8+0x5F24
 * for actor idx, and tick the +0x148 counter state (rand()-driven when 3).
 * Retail leaves $s1 unset on the +0x60 == +0x64 path and increments the
 * caller's value; the host uses 0 + 1 there (the only value it can know). */
void func_8009A2D4(u8 idx) {
    u8* base = D_800CCCE8;
    u32 off = idx * 0x170;
    u8* a3 = base + off;
    u8* s0 = base + 0xA4 + off;
    u8* s2 = base + 0x148 + off;
    u8* a2 = base + 0x2228 + idx * 0x690;
    u8* a1 = base + 0x5F24;
    u32 i;
    u32 v;
    u32 t;
    u32 s1 = 0;

    for (i = 0; i < 15; i++) {
        *(u16*)(a1 + i * 2) = *(u16*)(a2 + 0x24 + i * 0x28);
    }
    if (s0[0x57] != 0) {
        *(u16*)(a1 + 0x26) = s0[0x57] * *(u16*)(a1 + 0);
    } else {
        *(u16*)(a1 + 0x26) = 0x1E;
    }
    v = s2[0];
    if (v != 0) {
        if (v < 4) {
            *(u16*)(a1 + 0x26) = *(u16*)(a1 + 0x26) + v * 20;
        } else if (v == 4) {
            *(u16*)(a1 + 0x26) = *(u16*)(a1 + 0x26) * 10;
        }
    }
    t = s0[0x3C] * s0[0x3F];
    *(u16*)(a1 + 0x1E) = t;
    t = s0[0x12] + t;
    *(u16*)(a1 + 0x1E) = t;
    if (a3[0x56] == 4) {
        *(u16*)(a1 + 0x1E) = s0[0x22] + t;
    }
    a1[0x29] = s0[0x98];
    a1[0x2A] = s0[0x99];
    {
        u32 hi = *(u32*)(s0 + 0x64);
        u32 lo = *(u32*)(s0 + 0x60);

        if (hi != lo) {
            u32 q = bat3_divu(hi - lo, hi / 10);

            s1 = q;
            if ((q & 0xFF) == 0) {
                s1 = q + 1;
            }
        } else {
            s1 = s1 + 1;
        }
    }
    s1 = (s1 & 0xFF) * (a3[0x54] + 5);
    if (!(*(u16*)(g_GameState + 0x22B6) & 0x4000)) {
        s1 = 0;
    }
    if (a3[0x62] < 0x32) {
        s1 = 0;
    }
    v = a3[0xA0];
    if (v == 3) {
        s1 = 0;
    }
    if (v == 0xF) {
        s1 = 0x63;
    }
    if ((s1 & 0xFF) >= 0x64) {
        s1 = 0x63;
    }
    *(u16*)(a1 + 0x2E) = s1 & 0xFF;
    *(u16*)(a1 + 0x24) = 0;
    if (*(u16*)(s0 + 0x7C) & 0x100) {
        *(u16*)(a1 + 0x24) = 0x8000;
    }
    if (*(u16*)(s0 + 0x7C) & 0x200) {
        *(u16*)(a1 + 0x24) |= 0x4000;
    }
    if (*(u16*)(s0 + 0x7C) & 0x80) {
        *(u16*)(a1 + 0x24) |= 0x2000;
    }
    if (*(u16*)(s0 + 0x7C) & 0x10) {
        *(u16*)(a1 + 0x24) |= 0x1000;
    }
    if (*(u16*)(s0 + 0x38) < (u32)(*(u16*)(s0 + 0x3A) >> 3)) {
        *(u16*)(a1 + 0x24) |= 0x800;
    } else {
        *(u16*)(a1 + 0x24) &= 0xF7FF;
    }
    if (*(u16*)(s0 + 0x38) == 0) {
        *(u16*)(s0 + 0x80) &= 0x7FFF;
        *(u16*)(a3 + 0x84) &= 0x7FFF;
    }
    a1[0x2C] = (*(u16*)(s0 + 0x80) & 0x8000) ? 1 : 0;
    if (s2[0] == 4) {
        u8 c;

        a1[0x28] = 4;
        c = D_800CCE4A[off] - 1;
        D_800CCE4A[off] = c;
        if (c == 0) {
            *(u16*)(s0 + 0x80) &= 0xBFFF;
            s2[0] = 0;
            a3[0x54] = 0;
        }
    } else {
        a1[0x28] = s2[0];
        if (s2[0] == 3 && (*(u16*)(g_GameState + 0x22B6) & 0x4000)) {
            if ((s32)(rand() % 100) < (s32)(s1 & 0xFF)) {
                *(u16*)(s0 + 0x80) |= 0x4000;
                D_800CCE4A[off] = 3;
                if (*(u16*)(D_800CCD1A + off) & 0x40) {
                    D_800CCE4A[off] = 6;
                }
                s2[0]++;
            }
        }
    }
}
#endif /* XENO_PC_PORT */
