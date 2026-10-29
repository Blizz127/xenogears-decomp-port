#include "common.h"


#ifndef XENO_PC_PORT
extern u8* D_800C3DFC;
extern u8* D_800C3E34;
extern u8* D_800D2D6C;
extern u8* D_800C3E00;
extern u8* D_800D2DC8;
extern u8* D_800C34B0;
extern u8 D_800C3E50;
extern u8 g_GameState[];
extern int rand(void);

/* func_8009D3A0.s, byte-exact: hit/miss roll for the current action
 * (D_800C3DFC) by the attacker (D_800D2D6C) against the target
 * (D_800C3E34).  Returns 1 (hit), 2 or 3 (miss) from their status flags,
 * the hit value (D_800C3E00 +0x5E, plus the attacker's signed +0x9F unless
 * its g_GameState +0x22B6 byte is set), the evade value (target +0x5F plus
 * half of D_800D2DC8's signed +0x9F), the actor record's +0x15A bits and
 * rand() % 100 (the main61.c port arm below does the same). */
u32 func_8009D3A0(void) {
    u16 flags;
    s16 hit;
    s16 evade;
    s8 d;
    u8 v;
    s16 diff;
    s16 bonus;
    s16 crit;
    u16 st;
    u32 off;

    crit = 0;
    bonus = 0;
    if ((*(u16*)(D_800C3DFC + 0xA) & 0x200) && (*(u16*)(D_800C3E34 + 0x34) & 8)) {
        return 3;
    }
    flags = *(u16*)(D_800C3DFC + 0xA);
    if (flags & 0x1000) {
        return 3;
    }
    if ((*(u16*)(D_800C3E34 + 0x84) | *(u16*)(D_800C3E34 + 0x86)) & 0x100) {
        return 3;
    }
    if (*(u16*)(D_800C3E34 + 0x7C) & 0x2000) {
        return 1;
    }
    if (*(u16*)(D_800C3E34 + 0x80) & 0x1000) {
        return 1;
    }
    if (flags & 0x8000) {
        return 1;
    }
    evade = D_800C3E34[0x5F];
    v = g_GameState[0x22B6 + D_800D2D6C[4]];
    hit = D_800C3E00[0x5E];
    if (v == 0) {
        hit += (s8)D_800D2D6C[0x9F];
    }
    d = D_800D2DC8[0x9F];
    if (d != 0) {
        evade += d / 2;
    }
    if (D_800C3E00[0x56] == 4) {
        if ((D_800C3DFC[0x10] & 0x80) && v == 0) {
            return 3;
        }
        if ((D_800C3DFC[0x10] & 0x20) && g_GameState[0x22B6 + D_800D2D6C[7]] == 0) {
            return 3;
        }
    }
    if ((*(u16*)(D_800D2D6C + 0x80) | *(u16*)(D_800D2D6C + 0x82)) & 0x800) {
        hit += hit / 2;
    }
    if (*(u16*)(D_800D2D6C + 0x7C) & 0x10) {
        crit = 0x3C;
    }
    if (*(u16*)(D_800C3DFC + 0xA) & 0x1000) {
        return 3;
    }
    off = D_800C3E50 * 0x170;
    if ((D_800C34B0 + off)[0x15A] & 0x80) {
        if ((*(u16*)(D_800D2DC8 + 0x80) | *(u16*)(D_800D2DC8 + 0x82)) & 0x400) {
            evade += evade / 2;
        }
        if (*(u16*)(D_800D2DC8 + 0x7C) & 0x10) {
            bonus = 0x1E;
        }
        if (*(u16*)(D_800D2DC8 + 0x7C) & 0xC00) {
            return 1;
        }
    } else {
        hit /= 2;
        if (*(u16*)(D_800C3E34 + 0x7C) & 0x2000) {
            return 1;
        }
        if (*(u16*)(D_800C3E34 + 0x80) & 0x1000) {
            return 1;
        }
        if ((*(u16*)(D_800C3E34 + 0x84) | *(u16*)(D_800C3E34 + 0x86)) & 0x800) {
            evade += evade / 2;
        }
    }
    off = D_800C3E50 * 0x170;
    if ((D_800C34B0 + off)[0x15A] & 1) {
        if (rand() % 100 < 0x5F) {
            return 2;
        }
        return 1;
    }
    diff = hit + (s8)D_800C3DFC[0x15] - evade;
    st = *(u16*)(D_800C3E34 + 0x84) | *(u16*)(D_800C3E34 + 0x86);
    if (st & 0x20) {
        if ((s16)(rand() % 100 - diff) >= 0x32) {
            return 3;
        }
        return 1;
    }
    if (st & 0x40) {
        if ((s16)(rand() % 100 - diff) >= 0x32) {
            return 2;
        }
        return 1;
    }
    if ((s16)(rand() % 100 - diff) >= (s16)(0x55 - crit + bonus)) {
        return 3;
    }
    if ((s16)(rand() % 100 - diff) >= (s16)(0x55 - crit + bonus)) {
        return 2;
    }
    return 1;
}
extern u8* D_800C3DFC;
extern u8* D_800D2D6C;
extern u16 func_80096FBC(void);

u16 func_8009D948(void) {
    u16 a0 = func_80096FBC();
    u8 m;
    s32 a1;

    if (*(u16*)(D_800C3DFC + 0xA) & 0x100) {
        m = D_800D2D6C[0x9E];
        if ((*(u16*)(D_800D2D6C + 0x7C) & 0x100) || m == 0) {
            m = 4;
        }
        a1 = m * a0 / 4;
        a0 = a1;
        if ((*(u16*)(D_800D2D6C + 0x80) | *(u16*)(D_800D2D6C + 0x82)) & 0x40) {
            a0 = a1 + (a0 >> 1);
        }
    }
    return a0;
}

extern u16 func_80097610(void);
extern u8* D_800D2DC8;
extern u8* D_800C34B0;
extern u8 D_800C3E50;

/* func_8009DA04.s, byte-exact: defence value from func_80097610.  When
 * D_800C3DFC+0xA bit 8 is set: scale the D_800D2DC8+0x72 term by
 * (100 - D_800D2DC8[0x99]) / 100 (signed) when both are nonzero, add it for
 * a +0x15A bit 7 actor D_800C3E50, and add half again when D_800D2DC8+0x80
 * or +0x82 has bit 5; otherwise scale the value itself by that factor.
 * gcc 2.6.x code.  Codegen: the call result and the +0x72 term are u16
 * (masked at each use, as retail does); the two D_800D2DC8 reads of the
 * first branch go through their own block-local pointers; the actor record
 * sum adds D_800C34B0 loaded into a local first and goes through its own
 * pointer p (base first, as in main56's func_8009AB38; indexing the sum
 * directly comes out offset first). */
u16 func_8009DA04(void) {
    u16 a1 = func_80097610();
    u8 a2;

    if (*(u16*)(D_800C3DFC + 0xA) & 0x100) {
        u8* d = D_800D2DC8;
        u8* base;
        u8* d2;
        u8* p;
        u16 a0;

        a2 = d[0x99];
        a0 = *(u16*)(d + 0x72);
        if (a2 != 0 && a0 != 0) {
            a0 = a0 * (100 - a2) / 100;
        }
        base = D_800C34B0;
        p = base + D_800C3E50 * 0x170;
        if (p[0x15A] & 0x80) {
            a1 += a0;
        }
        d2 = D_800D2DC8;
        if ((*(u16*)(d2 + 0x80) | *(u16*)(d2 + 0x82)) & 0x20) {
            a1 += a1 >> 1;
        }
    } else {
        a2 = D_800D2DC8[0x99];
        if (a2 != 0) {
            a1 = a1 * (100 - a2) / 100;
        }
    }
    return a1;
}
extern u8* D_800D2DC8;

/* Scale a0 by the target's resistance byte (D_800D2DC8 + 0x88 + index of
 * the highest set bit of D_800C3DFC's +8 mask): unchanged for 0, 0 for 20
 * and above, else a0 * (20 - r) / 20 (signed).  As in retail, idx is not
 * initialised: with an empty mask the not-found path reads whatever the
 * register holds (the port arm uses 0).  The (u32) cast on the index is
 * what retail's addressing needs (found by a permuter run). */
u32 func_8009DB54(u32 a0) {
    u32 mask = *(u16*)(D_800C3DFC + 0x8);
    u32 i;
    u32 idx;
    u32 r;

    for (i = 0; (i & 0xFF) < 0x10; i++) {
        if (mask & (0x8000 >> i)) {
            idx = i;
            break;
        }
    }
    r = D_800D2DC8[(u32)((idx & 0xFF) + 0x88)];
    if (r != 0) {
        if (r < 0x14) {
            a0 = (s32)(a0 * (0x14 - r)) / 20;
        } else {
            a0 = 0;
        }
    }
    return a0;
}
extern void func_8009E868(u8 kind, u16 mask);

/* The battle state at D_800C34B0: per-actor words at +0x5F6C and bytes at
 * +0x5FA0. */
typedef struct {
    u8 pad[0x5F6C];
    u32 x5F6C[13];
    u8 x5FA0[8];
} DBFCState;

/* func_8009DBFC.s, byte-exact: apply the status effect of the current
 * action to the target (D_800D2DC8) when the actor record's +0x15A bit 7
 * is set and rand() % 100 is within the signed chance: chance, kind, mask
 * and duration come from the attacker (D_800D2D6C +0x13/+0x14/+0x10,
 * duration 5) when fromAttacker is set, else from the action
 * (D_800C3DFC +0x1C/+0x1D/+0x1E/+0x11).  Kinds 0/1 first try to cancel
 * an opposing status (returning 1 when one is cleared), then set the
 * status bits and per-status durations in the actor record; kinds 3,
 * 0xA-0xF and 0x10 update the target's other status words, D_800C34B0's
 * per-actor fields or the +0x99 counter (kind 0xB then reports as kind 0
 * with mask 0x40).  Ends with func_8009E868(kind, mask) and returns 1;
 * returns 0 when the roll fails or the status is blocked (the main61.c
 * port arm below does the same). */
u32 func_8009DBFC(u8 fromAttacker) {
    u8* rec;
    u8* rec2;
    u8 chance;
    u8 kind;
    u16 mask;
    u8 dur;
    u32 off;

    rec = D_800C34B0 + D_800C3E50 * 0x170;
    rec2 = rec;
    if (fromAttacker) {
        chance = D_800D2D6C[0x13];
        kind = D_800D2D6C[0x14];
        mask = *(u16*)(D_800D2D6C + 0x10);
        dur = 5;
    } else {
        chance = D_800C3DFC[0x1C];
        kind = D_800C3DFC[0x1D];
        mask = *(u16*)(D_800C3DFC + 0x1E);
        dur = D_800C3DFC[0x11];
    }
    off = D_800C3E50 * 0x170;
    if (!((D_800C34B0 + off)[0x15A] & 0x80)) {
        return 0;
    }
    if ((s8)chance < rand() % 100) {
        return 0;
    }
    switch (kind) {
    case 0:
        if (mask & 0x20) {
            u8* d = D_800D2DC8;
            u16 f = *(u16*)(d + 0x80);
            if (f & 0x8000) {
                u8* e = D_800C3E34;

                *(u16*)(d + 0x80) = f & 0x7FFF;
                *(u16*)(e + 0x84) &= 0x7FFF;
                return 1;
            }
        }
        break;
    case 1:
        if (mask & 0xA) {
            u8* d = D_800D2DC8;
            u16 f = *(u16*)(d + 0x80);
            if (f & 0x5) {
                *(u16*)(d + 0x80) = f & 0xFFFA;
                return 1;
            }
        } else if (mask & 0x5) {
            u8* d = D_800D2DC8;
            u16 f = *(u16*)(d + 0x80);
            if (f & 0xA) {
                *(u16*)(d + 0x80) = f & 0xFFF5;
                return 1;
            }
        }
        break;
    }
    if (kind == 0) {
        u8* d = D_800D2DC8;

        if (mask & *(u16*)(d + 0x7E)) {
            return 0;
        }
        switch (mask) {
        case 0x400:
            rec[0x15C] = dur;
            *(u16*)(D_800D2DC8 + 0x7C) &= 0xFBFF;
            *(u16*)(rec2 + 0x7C) |= 0x2000;
            break;
        case 0x1000:
            *(u16*)(d + 0x7C) &= 0xEFFF;
            *(u16*)(rec2 + 0x80) |= 0x2000;
            break;
        case 0x200:
            rec[0x15D] = dur;
            break;
        case 0x100:
            rec[0x15E] = dur;
            break;
        case 0x80:
            rec[0x15F] = dur;
            break;
        case 0x20:
            rec[0x160] = dur;
            *(u16*)(rec2 + 0x7C) |= 0x1000;
            break;
        case 0x10:
            rec[0x161] = dur;
            break;
        }
        *(u16*)(D_800D2DC8 + 0x7C) |= mask;
    }
    if (kind == 1) {
        *(u16*)(D_800D2DC8 + 0x80) |= mask;
        switch (mask) {
        case 0x1000:
            rec[0x166] = dur;
            break;
        case 0x40:
            rec[0x167] = dur;
            break;
        case 0x20:
            rec[0x168] = dur;
            break;
        }
    }
    if (kind == 3) {
        if (mask & 0xF000) {
            u8* d = D_800D2DC8;

            if (*(u16*)(d + 0x86) & 0xF000) {
                return 0;
            }
            *(u16*)(d + 0x84) = mask | (*(u16*)(d + 0x84) & 0xFFF);
            rec[0x163] = dur;
        }
        if (mask & 0xF00) {
            u8* d = D_800D2DC8;

            if (*(u16*)(d + 0x86) & 0xF00) {
                return 0;
            }
            *(u16*)(d + 0x84) = mask | (*(u16*)(d + 0x84) & 0xF0FF);
            rec[0x164] = dur;
        }
    }
    if (kind == 0xA) {
        *(u16*)(D_800D2DC8 + 0x84) &= ~mask;
    }
    if (kind == 0xB) {
        u8* d = D_800D2DC8;

        if (!(*(u16*)(d + 0x7E) & 0x40)) {
            d[0x99] += mask;
            kind = 0;
            if (D_800D2DC8[0x99] >= 0x64) {
                D_800D2DC8[0x99] = 0x63;
            }
            mask = 0x40;
        }
    }
    if (kind == 0xC) {
        ((DBFCState*)D_800C34B0)->x5FA0[D_800C3E50] = 0;
        ((DBFCState*)D_800C34B0)->x5F6C[D_800C3E50] = *(u32*)(D_800D2DC8 + 0x60) / mask;
    }
    if (kind == 0xD) {
        ((DBFCState*)D_800C34B0)->x5FA0[D_800C3E50] = 0;
        ((DBFCState*)D_800C34B0)->x5F6C[D_800C3E50] = *(u32*)(D_800D2DC8 + 0x60) - 1;
    }
    if (kind == 0xE) {
        u8* d = D_800D2DC8;

        *(u16*)(d + 0x80) = 0;
        *(u16*)(d + 0x84) = 0;
        if (mask == 1) {
            *(u16*)(d + 0x82) = 0;
            *(u16*)(d + 0x86) = 0;
        }
    }
    if (kind == 0x10) {
        u8* d = D_800D2DC8;
        u8* e = D_800C3E34;

        *(u16*)(d + 0x7C) |= 1;
        *(u16*)(e + 0x7C) |= 0x80;
    }
    if (kind == 0xF) {
        u8* d = D_800D2DC8;
        u8* e = D_800C3E34;

        *(u16*)(d + 0x7C) &= 0xFFFE;
        *(u16*)(e + 0x7C) &= 0xFF7F;
    }
    func_8009E868(kind, mask);
    return 1;
}
#endif


#ifndef XENO_PC_PORT
extern u8* D_800D2D28;
#endif
#ifndef XENO_PC_PORT
extern u8* D_800D2DC8;
#endif


/* func_8009E268.s */
void func_8009E268(void) {
    D_800D2DC8[0x99] = 0;
}


/* ---- Port bodies (fo/bat3).  The retail build compiles the byte-exact C
 * above for func_8009D3A0, func_8009DA04 and func_8009DBFC.  These are
 * compiled only for the port and the differential harness. ---- */
#ifndef XENO_PC_PORT
extern u8* D_800C34B0;
#endif
#ifndef XENO_PC_PORT
extern u8* D_800C3DFC;
#endif
#ifndef XENO_PC_PORT
extern u8* D_800C3E00;
#endif
#ifndef XENO_PC_PORT
extern u8* D_800C3E34;
#endif
#ifndef XENO_PC_PORT
extern u8* D_800D2D6C;
#endif
#ifndef XENO_PC_PORT
extern u8 D_800C3E50;
#endif
extern u8 g_GameState[];
#ifdef XENO_PC_PORT
extern int rand(void);
extern u32 func_80096FBC(void);
extern u32 func_80097610(void);
extern void func_8009E868(u8 kind, u16 mask);

/* rand() % 100 as retail computes it (signed). */
static s32 bat3_rand100(void) {
    return (s32)rand() % 100;
}

/* (s16)x / 2 the way retail rounds it: add the sign bit, then shift. */
static u32 bat3_half16(u32 x) {
    s32 v = (s16)x;
    s32 sign = (x >> 15) & 1;

    return (u32)((v + sign) >> 1);
}

/* func_8009D3A0.s: hit/miss roll for the current action against the target
 * slot.  Returns 1 (hit), 2 (critical/second outcome) or 3 (miss), from the
 * status flags of the attacker (D_800D2D6C) and target (D_800C3E34), the
 * action record (D_800C3DFC) and rand().
 * Two halvings use a logical shift of the 32-bit sum where the others use an
 * arithmetic one; they differ only in bit 31, which the later (s16) casts
 * drop, but the host keeps retail's forms anyway. */
u32 func_8009D3A0(void) {
    u8* dfc = D_800C3DFC;
    u8* tgt = D_800C3E34;
    u8* atk;
    u8* ent;
    u32 flags;
    u32 a1;
    u32 a2;
    u32 a3;
    s32 s0 = 0;
    s32 s1;
    s32 s3 = 0;
    s32 r;

    if ((*(u16*)(dfc + 0xA) & 0x200) && (*(u16*)(tgt + 0x34) & 0x8)) {
        return 3;
    }
    dfc = D_800C3DFC;
    flags = *(u16*)(dfc + 0xA);
    if (flags & 0x1000) {
        return 3;
    }
    tgt = D_800C3E34;
    if ((*(u16*)(tgt + 0x84) | *(u16*)(tgt + 0x86)) & 0x100) {
        return 3;
    }
    if (*(u16*)(tgt + 0x7C) & 0x2000) {
        return 1;
    }
    if (*(u16*)(tgt + 0x80) & 0x1000) {
        return 1;
    }
    if (flags & 0x8000) {
        return 1;
    }
    atk = D_800D2D6C;
    a1 = tgt[0x5F];
    ent = D_800C3E00;
    a3 = g_GameState[0x22B6 + atk[4]];
    a2 = ent[0x5E];
    if (a3 == 0) {
        a2 = (s32)(s8)atk[0x9F] + a2;
    }
    {
        s32 v = (s8)D_800D2DC8[0x9F];

        if (v != 0) {
            /* sign bit added, then a LOGICAL shift (srl) */
            a1 = (((u32)v + (v < 0 ? 1u : 0u)) >> 1) + a1;
        }
    }
    if (ent[0x56] == 4) {
        if ((dfc[0x10] & 0x80) && a3 == 0) {
            return 3;
        }
        if (D_800C3DFC[0x10] & 0x20) {
            if (g_GameState[0x22B6 + D_800D2D6C[7]] == 0) {
                return 3;
            }
        }
    }
    atk = D_800D2D6C;
    if ((*(u16*)(atk + 0x80) | *(u16*)(atk + 0x82)) & 0x800) {
        a2 = a2 + bat3_half16(a2);
    }
    if (*(u16*)(atk + 0x7C) & 0x10) {
        s3 = 0x3C;
    }
    if (*(u16*)(D_800C3DFC + 0xA) & 0x1000) {
        return 3;
    }
    if ((D_800C34B0 + D_800C3E50 * 0x170)[0x15A] & 0x80) {
        u8* d = D_800D2DC8;
        u32 v;

        if ((*(u16*)(d + 0x80) | *(u16*)(d + 0x82)) & 0x400) {
            a1 = a1 + bat3_half16(a1);
        }
        v = *(u16*)(d + 0x7C);
        if (v & 0x10) {
            s0 = 0x1E;
        }
        if (v & 0xC00) {
            return 1;
        }
    } else {
        {
            s32 v = (s16)a2;

            /* sign bit added, then a LOGICAL shift (srl) */
            a2 = ((u32)v + ((a2 >> 15) & 1)) >> 1;
        }
        tgt = D_800C3E34;
        if (*(u16*)(tgt + 0x7C) & 0x2000) {
            return 1;
        }
        if (*(u16*)(tgt + 0x80) & 0x1000) {
            return 1;
        }
        if ((*(u16*)(tgt + 0x84) | *(u16*)(tgt + 0x86)) & 0x800) {
            a1 = a1 + bat3_half16(a1);
        }
    }
    if ((D_800C34B0 + D_800C3E50 * 0x170)[0x15A] & 0x1) {
        return (bat3_rand100() < 0x5F) ? 2 : 1;
    }
    {
        u32 st;

        s1 = (s32)(a2 + (u32)(s32)(s8)D_800C3DFC[0x15] - a1);
        tgt = D_800C3E34;
        st = *(u16*)(tgt + 0x84) | *(u16*)(tgt + 0x86);
        if (st & 0x20) {
            r = bat3_rand100();
            return ((s16)(r - s1) < 0x32) ? 1 : 3;
        }
        if (st & 0x40) {
            r = bat3_rand100();
            return ((s16)(r - s1) < 0x32) ? 1 : 2;
        }
    }
    r = bat3_rand100();
    s0 = (s16)(s0 + 0x55 - s3);
    if (!((s16)(r - s1) < s0)) {
        return 3;
    }
    r = bat3_rand100();
    return ((s16)(r - s1) < s0) ? 1 : 2;
}

/* func_8009D948.s: attack power from func_80096FBC, scaled by 4 (or the
 * attacker's +0x9E multiplier) / 4 for flagged actions, +50% with status
 * bit 0x40. */
u16 func_8009D948(void) {
    u32 a0 = func_80096FBC();

    if (*(u16*)(D_800C3DFC + 0xA) & 0x100) {
        u8* atk = D_800D2D6C;
        s32 m = atk[0x9E];
        s32 prod;
        u32 a1;

        if ((*(u16*)(atk + 0x7C) & 0x100) || m == 0) {
            m = 4;
        }
        prod = (s32)((u32)m * (a0 & 0xFFFF));
        if (prod < 0) {
            prod += 3;
        }
        a1 = (u32)(prod >> 2);
        atk = D_800D2D6C;
        a0 = a1;
        if ((*(u16*)(atk + 0x80) | *(u16*)(atk + 0x82)) & 0x40) {
            a0 = a1 + ((a1 & 0xFFFF) >> 1);
        }
    }
    return a0 & 0xFFFF;
}

/* func_8009DA04.s: defence value from func_80097610, reduced by the
 * target's +0x99 percentage; flagged actions add the +0x72 value for
 * player slots and +50% with status bit 0x20. */
u16 func_8009DA04(void) {
    u32 a1 = func_80097610();

    if (*(u16*)(D_800C3DFC + 0xA) & 0x100) {
        u8* d = D_800D2DC8;
        u32 a2 = d[0x99];
        u32 a0 = *(u16*)(d + 0x72);

        if (a2 != 0 && a0 != 0) {
            a0 = (u32)((s32)(a0 * (100 - a2)) / 100);
        }
        if ((D_800C34B0 + D_800C3E50 * 0x170)[0x15A] & 0x80) {
            a1 = a1 + a0;
        }
        d = D_800D2DC8;
        if ((*(u16*)(d + 0x80) | *(u16*)(d + 0x82)) & 0x20) {
            a1 = a1 + ((a1 & 0xFFFF) >> 1);
        }
    } else {
        u32 a0 = D_800D2DC8[0x99];

        if (a0 != 0) {
            a1 = (u32)((s32)((a1 & 0xFFFF) * (100 - a0)) / 100);
        }
    }
    return a1 & 0xFFFF;
}

/* func_8009DB54.s: scale a0 by the target's resistance for the highest
 * set element bit of the action's +0x8 mask: 0 keeps a0, 1..19 gives
 * a0 * (20 - r) / 20, 20+ gives 0.  With no bit set retail indexes with
 * the caller's $v1 (uninitialised); the host uses 0. */
u32 func_8009DB54(u32 a0) {
    u32 mask = *(u16*)(D_800C3DFC + 0x8);
    u32 i;
    u32 idx = 0;
    u32 r;

    for (i = 0; (i & 0xFF) < 0x10; i++) {
        if (mask & (0x8000 >> i)) {
            idx = i;
            break;
        }
    }
    r = D_800D2DC8[(idx & 0xFF) + 0x88];
    if (r == 0) {
        return a0;
    }
    if (r < 0x14) {
        return (u32)((s32)(a0 * (0x14 - r)) / 20);
    }
    return 0;
}

/* R3000 divu: a zero divisor leaves LO = 0xFFFFFFFF (no trap). */
static u32 bat3_divu(u32 a, u32 b) {
    return (b == 0) ? 0xFFFFFFFFu : a / b;
}

/* func_8009DBFC.s: apply the status effect of the current action (from the
 * attacker record when a0 != 0, else from the action record): a rand()
 * roll against the effect's chance, then per effect kind (+0x14/+0x1D)
 * set/clear the target's status words and durations.  Returns 1 when the
 * effect landed, 0 otherwise. */
u32 func_8009DBFC(u32 a0) {
    u8* s3;
    u8* s4;
    s32 s0;
    u32 s5;
    u32 s1;
    u32 s2;
    u32 a1;

    s3 = D_800C34B0 + D_800C3E50 * 0x170;
    s4 = s3;
    if (a0 & 0xFF) {
        u8* p = D_800D2D6C;

        s0 = p[0x13];
        s5 = p[0x14];
        s1 = *(u16*)(p + 0x10);
        s2 = 5;
    } else {
        u8* p = D_800C3DFC;

        s0 = p[0x1C];
        s5 = p[0x1D];
        s1 = *(u16*)(p + 0x1E);
        s2 = p[0x11];
    }
    if (!((D_800C34B0 + D_800C3E50 * 0x170)[0x15A] & 0x80)) {
        return 0;
    }
    s0 = (s8)s0;
    if (s0 < bat3_rand100()) {
        return 0;
    }
    if (s5 == 0) {
        if (s1 & 0x20) {
            u8* d = D_800D2DC8;
            u32 f = *(u16*)(d + 0x80);

            if (f & 0x8000) {
                u8* e = D_800C3E34;

                *(u16*)(d + 0x80) = f & 0x7FFF;
                *(u16*)(e + 0x84) &= 0x7FFF;
                return 1;
            }
        }
    } else if (s5 == 1) {
        if (s1 & 0xA) {
            u8* d = D_800D2DC8;
            u32 f = *(u16*)(d + 0x80);

            if (f & 0x5) {
                *(u16*)(d + 0x80) = f & 0xFFFA;
                return 1;
            }
        } else if (s1 & 0x5) {
            u8* d = D_800D2DC8;
            u32 f = *(u16*)(d + 0x80);

            if (f & 0xA) {
                *(u16*)(d + 0x80) = f & 0xFFF5;
                return 1;
            }
        }
    }
    if (s5 == 0) {
        u8* d = D_800D2DC8;

        if (s1 & *(u16*)(d + 0x7E)) {
            return 0;
        }
        switch (s1) {
        case 0x400:
            s3[0x15C] = s2;
            *(u16*)(D_800D2DC8 + 0x7C) &= 0xFBFF;
            *(u16*)(s4 + 0x7C) |= 0x2000;
            break;
        case 0x1000:
            *(u16*)(d + 0x7C) &= 0xEFFF;
            *(u16*)(s4 + 0x80) |= 0x2000;
            break;
        case 0x200:
            s3[0x15D] = s2;
            break;
        case 0x100:
            s3[0x15E] = s2;
            break;
        case 0x80:
            s3[0x15F] = s2;
            break;
        case 0x20:
            s3[0x160] = s2;
            *(u16*)(s4 + 0x7C) |= 0x1000;
            break;
        case 0x10:
            s3[0x161] = s2;
            break;
        }
        *(u16*)(D_800D2DC8 + 0x7C) |= s1;
    }
    if (s5 == 1) {
        *(u16*)(D_800D2DC8 + 0x80) |= s1;
        switch (s1) {
        case 0x40:
            s3[0x167] = s2;
            break;
        case 0x20:
            s3[0x168] = s2;
            break;
        case 0x1000:
            s3[0x166] = s2;
            break;
        }
    }
    if (s5 == 3) {
        if (s1 & 0xF000) {
            u8* d = D_800D2DC8;

            if (*(u16*)(d + 0x86) & 0xF000) {
                return 0;
            }
            *(u16*)(d + 0x84) = s1 | (*(u16*)(d + 0x84) & 0xFFF);
            s3[0x163] = s2;
        }
        if (s1 & 0xF00) {
            u8* d = D_800D2DC8;

            if (*(u16*)(d + 0x86) & 0xF00) {
                return 0;
            }
            *(u16*)(d + 0x84) = s1 | (*(u16*)(d + 0x84) & 0xF0FF);
            s3[0x164] = s2;
        }
    }
    a1 = s5;
    if (a1 == 0xA) {
        *(u16*)(D_800D2DC8 + 0x84) &= ~s1;
    }
    if (a1 == 0xB) {
        u8* d = D_800D2DC8;

        if (!(*(u16*)(d + 0x7E) & 0x40)) {
            d[0x99] = s1 + d[0x99];
            d = D_800D2DC8;
            s5 = 0;
            if (d[0x99] >= 0x64) {
                d[0x99] = 0x63;
            }
            s1 = 0x40;
        }
    }
    a1 = s5;
    if (a1 == 0xC) {
        u32 q;

        (D_800C34B0 + D_800C3E50)[0x5FA0] = 0;
        q = bat3_divu(*(u32*)(D_800D2DC8 + 0x60), s1);
        *(u32*)(D_800C34B0 + D_800C3E50 * 4 + 0x5F6C) = q;
    }
    if (a1 == 0xD) {
        (D_800C34B0 + D_800C3E50)[0x5FA0] = 0;
        *(u32*)(D_800C34B0 + D_800C3E50 * 4 + 0x5F6C) =
            *(u32*)(D_800D2DC8 + 0x60) - 1;
    }
    if (a1 == 0xE) {
        u8* d = D_800D2DC8;

        *(u16*)(d + 0x80) = 0;
        *(u16*)(d + 0x84) = 0;
        if ((s1 & 0xFFFF) == 1) {
            *(u16*)(d + 0x82) = 0;
            *(u16*)(d + 0x86) = 0;
        }
    }
    a1 = s5;
    if (a1 == 0x10) {
        u8* d = D_800D2DC8;
        u8* e = D_800C3E34;

        *(u16*)(d + 0x7C) |= 0x1;
        *(u16*)(e + 0x7C) |= 0x80;
    }
    if (a1 == 0xF) {
        u8* d = D_800D2DC8;
        u8* e = D_800C3E34;

        *(u16*)(d + 0x7C) &= 0xFFFE;
        *(u16*)(e + 0x7C) &= 0xFF7F;
    }
    func_8009E868(a1, s1);
    return 1;
}
#endif /* XENO_PC_PORT */
