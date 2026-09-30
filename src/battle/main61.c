#include "common.h"


#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/battle/nonmatchings/main61", func_8009D3A0);
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

INCLUDE_ASM("asm/battle/nonmatchings/main61", func_8009DA04);
INCLUDE_ASM("asm/battle/nonmatchings/main61", func_8009DB54);
INCLUDE_ASM("asm/battle/nonmatchings/main61", func_8009DBFC);
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


/* ---- Port bodies (fo/bat3).  Not byte-matching: the matching build keeps
 * the retail bytes from the INCLUDE_ASM block above; these are compiled only
 * for the port and the differential harness. ---- */
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
