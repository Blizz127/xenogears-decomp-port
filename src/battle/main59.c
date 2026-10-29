#include "common.h"


#ifndef XENO_PC_PORT
extern u8 D_800CCD3E__rows[][0x170] asm("D_800CCD3E");
extern u16 D_800D2C94__a[] asm("D_800D2C94");

/* func_8009C134.s, byte-exact: D_800D2C94 = 1 << i for each party slot i
 * whose D_800CCD3E byte (0x170-byte records) is 3; the last match wins
 * (the main59.c port arm below does the same).  Codegen: the record byte
 * goes through an asm-label row view, which keeps the symbol in each
 * address (a u8 array plus i * 0x170 lets loop.c hoist an `la`), and
 * D_800D2C94 through an array view, whose `la` retail hoists. */
void func_8009C134(void) {
    u8 i;

    for (i = 0; i < 3; i++) {
        if (D_800CCD3E__rows[i][0] == 3) {
            D_800D2C94__a[0] = 1 << i;
        }
    }
}
extern u8* D_800C34B0;
extern u8* D_800C3E00;
extern u8 D_800C3E04;
extern u8 D_800C3E50;
extern u8* D_800C3E34;
extern u8* D_800C3D3C;
extern u8 D_800D2C34;
extern u8* D_800D2DC8;
extern u8* D_800C3DFC;
extern u8* D_800D2D6C;
extern u8 D_800D2DC4;
extern u32 D_800C3D60;
extern u32 D_800C34DC[];
extern s32 rand(void);
extern void func_80096824(void);
extern void func_8009AC48();
extern u32 func_8009DBFC(u32 a0);
extern void func_8009E788(void);
void func_8009CA90(void);
void func_8009C4B4(void);
void func_8009CB68(u8 index);
void func_8009C9C4(void);

/* The D_800C34B0 block's outcome bytes, as main51's Battle struct. */
typedef struct {
    u8 pad[0x5FA0];
    u8 f5FA0[8];
} BattleOutcomes;

/* func_8009C198.s, byte-exact: per-turn driver (the main59.c port arm
 * below does the same).  D_800C3DFC points at the active command record
 * (party actors: +0x2228 + D_800C34B0 +0x5FC1 * 0x690 + +0x5FC2 * 0x28;
 * others: +0x35D8 + +0x5FC2 * 0x28); func_8009AC48(D_800C3E04) gets
 * whatever is in $a1, as in retail; D_800D2DC4 = 0 and D_800C3D3C =
 * actor +0x5FC1's 0x170-byte record + 0x148; func_80096824 for +0xA bit
 * 0x100 with D_800C3E00 +0x56 == 1; command 1 with D_800C3E00 +0xA0 5 or
 * 0xD copies D_800D2D6C +0x1A to command +0x22.  Then for each actor
 * 0..10 (D_800C3E50) in the +0x5FAC mask: publish D_800C3E34,
 * D_800D2DC8 (+0xA4) and D_800C3D60 (+0x148), func_8009CA90, the
 * D_800C34DC[command +0x16] handler, func_8009DBFC(1 / 0) for outcome 0
 * and +0xA bits 0x800 / 0x4000, +0x5FB0 = command +2, func_8009C4B4,
 * func_8009CB68(actor).  Finally func_8009E788 for +0x56 == 4 and
 * func_8009C9C4.
 * Codegen: func_800941A4's forms (parenthesised record offsets, array
 * form flag reads, `bit = 1` at the loop start, BattleOutcomes view);
 * func_8009AC48 is declared without a prototype because retail passes
 * only $a0; the slot byte is held in a u32, as the arm does (a u8 adds
 * an andi). */
void func_8009C198(void) {
    u32 bit;
    u32 slot;
    u16 f;

    if (D_800C3E04 < 3) {
        D_800C3DFC = D_800C34B0 + (D_800C34B0[0x5FC1] * 0x690 + 0x2228) + D_800C34B0[0x5FC2] * 0x28;
    } else {
        D_800C3DFC = D_800C34B0 + (D_800C34B0[0x5FC2] * 0x28 + 0x35D8);
    }
    func_8009AC48(D_800C3E04);
    slot = D_800C34B0[0x5FC1];
    D_800D2DC4 = 0;
    D_800C3D3C = D_800C34B0 + slot * 0x170 + 0x148;
    if ((((u16*)D_800C3DFC)[5] & 0x100) && D_800C3E00[0x56] == 1) {
        func_80096824();
    }
    if ((D_800C3E00[0xA0] == 5 || D_800C3E00[0xA0] == 0xD) && D_800C34B0[0x5FC2] == 1) {
        D_800C3DFC[0x22] = D_800D2D6C[0x1A];
    }
    bit = 1;
    for (D_800C3E50 = 0; D_800C3E50 < 11; D_800C3E50++, bit <<= 1) {
        if (bit & *(u16*)(D_800C34B0 + 0x5FAC)) {
            D_800C3E34 = D_800C34B0 + D_800C3E50 * 0x170;
            D_800D2DC8 = D_800C3E34 + 0xA4;
            D_800C3D60 = (u32)(D_800C3E34 + 0x148);
            func_8009CA90();
            ((void (*)(void))D_800C34DC[D_800C3DFC[0x16]])();
            if (((BattleOutcomes*)D_800C34B0)->f5FA0[D_800C3E50] == 0) {
                f = ((u16*)D_800C3DFC)[5];
                if (f & 0x800) {
                    func_8009DBFC(1);
                } else if (f & 0x4000) {
                    func_8009DBFC(0);
                }
            }
            *(u16*)(D_800C34B0 + 0x5FB0) = *(u16*)(D_800C3DFC + 2);
            func_8009C4B4();
            func_8009CB68(D_800C3E50);
        }
    }
    if (D_800C3E00[0x56] == 4) {
        func_8009E788();
    }
    func_8009C9C4();
}


/* func_8009C4B4.s, byte-exact: per-slot bookkeeping after an action (the
 * main59.c port arm below does the same).  For a party actor
 * (D_800C3E04 < 3) the D_800C34B0 +0x5FC2 command index drives:
 * commands 0-2 bump the D_800C3D3C counter byte, capped at its second
 * byte; 3-5 / 6-8 / 9-11 set it to D_800D2C34 - 1 / - 2 / - 3; 3-11 add
 * index / 3 to D_800C3E00 +0x54, and 5 / 8 / 11 add 1 / 2 / 3 more.
 * Then, for actor D_800C3E50: outcome 0 with D_800C3E34 +0x80 bit 0x2000
 * clears that bit and D_800D2DC8 +0x7C bit 0x1000 on rand() % 100 < 80;
 * outcome 10 with D_800D2DC8 +0x7E bit 0x80 zeroes the +0x5F6C result
 * word; outcome 0 gets the +0x32 bit 0x80 halve/add-half roll (60, or 80
 * when +0x56 is 0) and the +0x32 bit 0x20 copy to D_800C3E04, as in
 * func_800946F4; +0x8A bit 0x200 zeroes an outcome-1 result word.
 * Codegen: the outcome bytes go through a struct view (BattleOutcomes,
 * main51's Battle layout), which loads D_800C34B0 before the index as
 * retail does; the counter cap compares the incremented value held in
 * `v`.  Matches only under gcc 2.6.x (D_800D2C34 - 1 is addiu 0xFF). */
void func_8009C4B4(void) {
    s32 lim;
    u8 v;

    if (D_800C3E04 < 3) {
        if (D_800C34B0[0x5FC2] < 3) {
            v = D_800C3D3C[0] + 1;
            D_800C3D3C[0] = v;
            if (D_800C3D3C[1] < v) {
                D_800C3D3C[0]--;
            }
        }
        if (D_800C34B0[0x5FC2] >= 3 && D_800C34B0[0x5FC2] < 6) {
            D_800C3D3C[0] = D_800D2C34 - 1;
        }
        if (D_800C34B0[0x5FC2] >= 6 && D_800C34B0[0x5FC2] < 9) {
            D_800C3D3C[0] = D_800D2C34 - 2;
        }
        if (D_800C34B0[0x5FC2] >= 9 && D_800C34B0[0x5FC2] < 12) {
            D_800C3D3C[0] = D_800D2C34 - 3;
        }
        if (D_800C34B0[0x5FC2] >= 3 && D_800C34B0[0x5FC2] < 12) {
            D_800C3E00[0x54] += D_800C34B0[0x5FC2] / 3;
        }
        if (D_800C34B0[0x5FC2] == 5) {
            D_800C3E00[0x54] += 1;
        }
        if (D_800C34B0[0x5FC2] == 8) {
            D_800C3E00[0x54] += 2;
        }
        if (D_800C34B0[0x5FC2] == 11) {
            D_800C3E00[0x54] += 3;
        }
    }
    if (((BattleOutcomes*)D_800C34B0)->f5FA0[D_800C3E50] == 0 && (*(u16*)(D_800C3E34 + 0x80) & 0x2000)) {
        if (rand() % 100 < 80) {
            u8* e = D_800C3E34;
            u8* d = D_800D2DC8;

            *(u16*)(e + 0x80) &= 0xDFFF;
            *(u16*)(d + 0x7C) &= 0xEFFF;
        }
    }
    {
        u8* b = D_800C34B0;
        u32 i = D_800C3E50;

        if ((b + i)[0x5FA0] == 10 && (*(u16*)(D_800D2DC8 + 0x7E) & 0x80)) {
            *(u32*)(b + i * 4 + 0x5F6C) = 0;
        }
    }
    if (((BattleOutcomes*)D_800C34B0)->f5FA0[D_800C3E50] == 0) {
        if (*(u16*)(D_800C3E34 + 0x32) & 0x80) {
            lim = 60;
            if (D_800C3E34[0x56] == 0) {
                lim = 80;
            }
            if (rand() % 100 < lim) {
                *(u32*)(D_800C34B0 + D_800C3E50 * 4 + 0x5F6C) >>= 1;
            } else {
                *(u32*)(D_800C34B0 + D_800C3E50 * 4 + 0x5F6C) += *(u32*)(D_800C34B0 + D_800C3E50 * 4 + 0x5F6C) >> 1;
            }
        }
        if (*(u16*)(D_800C3E34 + 0x32) & 0x20) {
            ((BattleOutcomes*)D_800C34B0)->f5FA0[D_800C3E04] = 0;
            *(u32*)(D_800C34B0 + D_800C3E04 * 4 + 0x5F6C) = *(u32*)(D_800C34B0 + D_800C3E50 * 4 + 0x5F6C);
        }
    }
    if (*(u16*)(D_800C3E34 + 0x8A) & 0x200) {
        u8* b = D_800C34B0;
        u32 i = D_800C3E50;

        if ((b + i)[0x5FA0] == 1) {
            *(u32*)(b + i * 4 + 0x5F6C) = 0;
        }
    }
}
extern u8* D_800C34B0;
extern u8* D_800C3DFC;
extern u8* D_800C3E00;
extern u8* D_800D2D6C;

/* Latch the action record bytes D_800C3DFC +0x20..+0x23 into D_800C34B0
 * +0x5FBC..+0x5FBF and +0x5FC2 into +0x5FC0; when the +0x5FBE byte has no
 * low six bits, OR in (D_800D2D6C +0x84 | D_800C3E00 +0x8A) >> 12.  The
 * shifted bits are computed before the test (retail does the OR before the
 * branch and the shift in its delay slot). */
void func_8009C9C4(void) {
    u32 a0 = *(u16*)(D_800D2D6C + 0x84);
    u32 a1 = *(u16*)(D_800C3E00 + 0x8A);
    u32 v;
    u32 bits;

    D_800C34B0[0x5FBC] = D_800C3DFC[0x20];
    D_800C34B0[0x5FBD] = D_800C3DFC[0x21];
    D_800C34B0[0x5FBE] = D_800C3DFC[0x22];
    D_800C34B0[0x5FBF] = D_800C3DFC[0x23];
    D_800C34B0[0x5FC0] = D_800C34B0[0x5FC2];
    bits = (a0 | a1) >> 12;
    v = D_800C34B0[0x5FBE];
    if ((v & 0x3F) == 0) {
        D_800C34B0[0x5FBE] = bits | v;
    }
}
#endif
#ifndef XENO_PC_PORT
extern u8* D_800C34B0;
#endif
#ifndef XENO_PC_PORT
extern u8* D_800C3DFC;
#endif
#ifndef XENO_PC_PORT
extern u8 D_800C3E50;
#endif
#ifdef XENO_PC_PORT
/* Port body: D_800C3DFC is a guest-RAM alias here, so the cursor word is
 * advanced through PSX_ADDR; the matching build compiles the retail C below. */
/* func_8009CA90.s: advance the D_800C3DFC cursor by 0x78 when the +0x5FC2 mode
 * is 0xC..0xE, or by 0x258 when it is 0..2; either way only while the active
 * actor's +0x15A flag has bit7 clear.  D_800C34B0 is loaded once. */
void func_8009CA90(void) {
    u8* p = D_800C34B0;

    /* D_800C3DFC is pointer-typed in the guest-RAM alias, so the cursor word
     * itself has to be advanced through PSX_ADDR; assigning to the alias is not
     * even an lvalue. Retail adds to the stored guest address, same as here. */
    if ((u32)(p[0x5FC2] - 0xC) < 3) {
        if (((p + D_800C3E50 * 368)[0x15A] & 0x80) == 0) {
            *(u32 *)PSX_ADDR(0x800C3DFC) += 0x78;
            return;
        }
    }
    if (p[0x5FC2] < 3) {
        if (((p + D_800C3E50 * 368)[0x15A] & 0x80) == 0) {
            *(u32 *)PSX_ADDR(0x800C3DFC) += 0x258;
        }
    }
}
#else
void func_8009CA90(void) {
    u8 *p = D_800C34B0;
    u8 *flags;

    if ((u32)(p[0x5FC2] - 0xC) < 3) {
        flags = &(p + D_800C3E50 * 0x170)[0x15A];
        if ((*flags & 0x80) == 0) {
            D_800C3DFC += 0x78;
            return;
        }
    }
    if (p[0x5FC2] < 3) {
        flags = &(p + D_800C3E50 * 0x170)[0x15A];
        if ((*flags & 0x80) == 0) {
            D_800C3DFC += 0x258;
        }
    }
}
#endif
#ifndef XENO_PC_PORT
extern u8 D_800CCD8C[];
#endif


/* func_8009CB68.s */
void func_8009CB68(u8 index) {
    u32 off = ((u32)index & 0xFF) * 368;
    u8* base = D_800CCD8C;
    u16* p = (u16*)((u32)(unsigned int)base + off);
    u8* q = base;
    u16* r;
    u16 v;

    q -= 0xA4;
    r = (u16*)((u32)(unsigned int)q + off);
    if ((p[0x40] & 0x200) != 0) {
        v = r[0x42] | 0x20;
    } else {
        v = r[0x42] & 0xFFDF;
    }
    r[0x42] = v;
}


/* ---- Port bodies (fo/bat3).  Not byte-matching: the matching build
 * compiles func_8009C134, func_8009C198 and func_8009C4B4 from the
 * byte-exact C above; these are compiled only for the port and the
 * differential harness. ---- */
#ifndef XENO_PC_PORT
extern u8* D_800C3E00;
#endif
#ifndef XENO_PC_PORT
extern u8* D_800C3E34;
#endif
#ifndef XENO_PC_PORT
extern u8* D_800D2DC8;
#endif
#ifndef XENO_PC_PORT
extern u8* D_800D2D6C;
#endif
#ifndef XENO_PC_PORT
extern u8* D_800C3D3C;
#endif
#ifndef XENO_PC_PORT
extern u8 D_800C3E04;
#endif
#ifndef XENO_PC_PORT
extern u8 D_800D2DC4;
#endif
#ifndef XENO_PC_PORT
extern u8 D_800D2C34;
#endif
#ifndef XENO_PC_PORT
extern u16 D_800D2C94;
#endif
#ifndef XENO_PC_PORT
extern u32 D_800C3D60;
#endif
#ifndef XENO_PC_PORT
extern u8 D_800CCD3E[];
#endif
#ifndef XENO_PC_PORT
extern u32 D_800C34DC[];
#endif
#ifdef XENO_PC_PORT
extern int rand(void);
extern void func_80096824(void);
extern void func_8009AC48(u8 index, u8 flag);
extern u32 func_8009DBFC(u32 a0);
extern void func_8009E788(void);
/* Runs a guest (retail MIPS) function through the battle interpreter; see
 * pc_port/src/battle_mips_runtime_internal.h. */
extern int PcPort_BattleMipsCallGuest(unsigned int target,
                                      const unsigned int* args,
                                      unsigned argc, unsigned int* result);
void func_8009C4B4(void);
void func_8009C9C4(void);

/* The guest word at a pointer global's own address (e.g. the value of
 * D_800C34B0 as a PSX address), for storing freshly computed pointers. */
#define BAT3_GW(addr) (*(u32*)PSX_ADDR(addr))

/* func_8009C134.s: D_800D2C94 = 1 << i for every i in 0..2 whose actor
 * record (0x170 stride) has +0x56 (D_800CCD3E) == 3; the last match wins. */
void func_8009C134(void) {
    u32 i;

    for (i = 0; i < 3; i++) {
        if (D_800CCD3E[i * 0x170] == 3) {
            D_800D2C94 = 1 << i;
        }
    }
}

/* func_8009C198.s: per-turn driver.  Points D_800C3DFC at the active action
 * record, runs func_8009AC48, then for each of the 11 slots whose bit is set
 * in the +0x5FAC mask: publishes the slot pointers, dispatches the slot's
 * handler through the D_800C34DC table, and post-processes.
 *
 * func_8009AC48 takes (index, flag) but retail passes whatever is in $a1:
 * on the D_800C3E04 < 3 path that is the low byte of the D_800C34B0 word
 * (loaded just before), otherwise the caller's $a1.  The second parameter
 * exists only to carry that incoming $a1; it is declared as a pointer so a
 * guest address arriving through the bridge can be turned back into the
 * exact guest value (PsxMemory_GuestAddr is the identity for non-RAM
 * values such as the 1 left by func_8009AC48). */
void func_8009C198(u32 unused, u8* a1_reg) {
    u32 s0;
    u8 a1 = (u8)PsxMemory_GuestAddr(a1_reg);

    (void)unused;
    if (D_800C3E04 < 3) {
        u8* b = D_800C34B0;

        a1 = (u8)BAT3_GW(0x800C34B0);
        BAT3_GW(0x800C3DFC) = BAT3_GW(0x800C34B0) + b[0x5FC1] * 0x690 +
                              0x2228 + b[0x5FC2] * 0x28;
    } else {
        BAT3_GW(0x800C3DFC) =
            BAT3_GW(0x800C34B0) + D_800C34B0[0x5FC2] * 0x28 + 0x35D8;
    }
    func_8009AC48(D_800C3E04, a1);
    {
        u32 slot = D_800C34B0[0x5FC1];

        D_800D2DC4 = 0;
        BAT3_GW(0x800C3D3C) = BAT3_GW(0x800C34B0) + slot * 0x170 + 0x148;
    }
    if ((*(u16*)(D_800C3DFC + 0xA) & 0x100) && D_800C3E00[0x56] == 1) {
        func_80096824();
    }
    s0 = 1;
    if (D_800C3E00[0xA0] == 5 || D_800C3E00[0xA0] == 0xD) {
        if (D_800C34B0[0x5FC2] == 1) {
            D_800C3DFC[0x22] = D_800D2D6C[0x1A];
        }
    }
    D_800C3E50 = 0;
    do {
        if (s0 & *(u16*)(D_800C34B0 + 0x5FAC)) {
            u32 g = BAT3_GW(0x800C34B0) + D_800C3E50 * 0x170;

            BAT3_GW(0x800C3E34) = g;
            BAT3_GW(0x800D2DC8) = g + 0xA4;
            D_800C3D60 = g + 0x148;
            func_8009CA90();
            PcPort_BattleMipsCallGuest(D_800C34DC[D_800C3DFC[0x16]], 0, 0, 0);
            if ((D_800C34B0 + D_800C3E50)[0x5FA0] == 0) {
                u32 f = *(u16*)(D_800C3DFC + 0xA);

                if (f & 0x800) {
                    func_8009DBFC(1);
                } else if (f & 0x4000) {
                    func_8009DBFC(0);
                }
            }
            *(u16*)(D_800C34B0 + 0x5FB0) = *(u16*)(D_800C3DFC + 2);
            func_8009C4B4();
            func_8009CB68(D_800C3E50);
        }
        D_800C3E50 = D_800C3E50 + 1;
        s0 <<= 1;
    } while (D_800C3E50 < 0xB);
    if (D_800C3E00[0x56] == 4) {
        func_8009E788();
    }
    func_8009C9C4();
}

/* rand() % 100 as retail computes it (signed). */
static s32 bat3_rand100(void) {
    return (s32)rand() % 100;
}

/* func_8009C4B4.s: per-slot bookkeeping after an action: counter/charge
 * updates on the acting record, status recovery rolls, and the +0x5F6C
 * damage word adjustments. */
void func_8009C4B4(void) {
    if (D_800C3E04 < 3) {
        u32 m;

        if (D_800C34B0[0x5FC2] < 3) {
            u8 v = D_800C3D3C[0] + 1;

            D_800C3D3C[0] = v;
            if (D_800C3D3C[1] < v) {
                D_800C3D3C[0] = D_800C3D3C[0] - 1;
            }
        }
        if ((u32)(D_800C34B0[0x5FC2] - 3) < 3) {
            D_800C3D3C[0] = D_800D2C34 - 1;
        }
        if ((u32)(D_800C34B0[0x5FC2] - 6) < 3) {
            D_800C3D3C[0] = D_800D2C34 - 2;
        }
        if ((u32)(D_800C34B0[0x5FC2] - 9) < 3) {
            D_800C3D3C[0] = D_800D2C34 - 3;
        }
        if ((u32)(D_800C34B0[0x5FC2] - 3) < 9) {
            m = D_800C34B0[0x5FC2];
            D_800C3E00[0x54] = D_800C3E00[0x54] + m / 3;
        }
        if (D_800C34B0[0x5FC2] == 5) {
            D_800C3E00[0x54] = D_800C3E00[0x54] + 1;
        }
        if (D_800C34B0[0x5FC2] == 8) {
            D_800C3E00[0x54] = D_800C3E00[0x54] + 2;
        }
        if (D_800C34B0[0x5FC2] == 0xB) {
            D_800C3E00[0x54] = D_800C3E00[0x54] + 3;
        }
    }
    if ((D_800C34B0 + D_800C3E50)[0x5FA0] == 0 &&
        (*(u16*)(D_800C3E34 + 0x80) & 0x2000)) {
        if (bat3_rand100() < 0x50) {
            u8* e = D_800C3E34;
            u8* d = D_800D2DC8;

            *(u16*)(e + 0x80) &= 0xDFFF;
            *(u16*)(d + 0x7C) &= 0xEFFF;
        }
    }
    {
        u8* b = D_800C34B0;
        u32 i = D_800C3E50;

        if ((b + i)[0x5FA0] == 0xA && (*(u16*)(D_800D2DC8 + 0x7E) & 0x80)) {
            *(u32*)(b + i * 4 + 0x5F6C) = 0;
        }
    }
    if ((D_800C34B0 + D_800C3E50)[0x5FA0] == 0) {
        u8* e = D_800C3E34;

        if (*(u16*)(e + 0x32) & 0x80) {
            s32 lim = (e[0x56] != 0) ? 0x3C : 0x50;

            if (bat3_rand100() < lim) {
                u32* w = (u32*)(D_800C34B0 + D_800C3E50 * 4 + 0x5F6C);

                *w = *w >> 1;
            } else {
                u32* w = (u32*)(D_800C34B0 + D_800C3E50 * 4 + 0x5F6C);

                *w = (*w >> 1) + *w;
            }
        }
        if (*(u16*)(D_800C3E34 + 0x32) & 0x20) {
            (D_800C34B0 + D_800C3E04)[0x5FA0] = 0;
            {
                u32 src = D_800C3E50;
                u32 dst = D_800C3E04;
                u8* b = D_800C34B0;

                *(u32*)(b + dst * 4 + 0x5F6C) = *(u32*)(b + src * 4 + 0x5F6C);
            }
        }
    }
    if (*(u16*)(D_800C3E34 + 0x8A) & 0x200) {
        u8* b = D_800C34B0;
        u32 i = D_800C3E50;

        if ((b + i)[0x5FA0] == 1) {
            *(u32*)(b + i * 4 + 0x5F6C) = 0;
        }
    }
}

/* func_8009C9C4.s: latch the action record's +0x20..+0x23 bytes and the
 * +0x5FC2 mode into the +0x5FBC..+0x5FC0 block; when +0x5FBE's low six
 * bits are clear, OR in the top nibble of the two actors' status words. */
void func_8009C9C4(void) {
    u32 a0 = *(u16*)(D_800D2D6C + 0x84);
    u32 a1 = *(u16*)(D_800C3E00 + 0x8A);
    u32 v;

    D_800C34B0[0x5FBC] = D_800C3DFC[0x20];
    D_800C34B0[0x5FBD] = D_800C3DFC[0x21];
    D_800C34B0[0x5FBE] = D_800C3DFC[0x22];
    D_800C34B0[0x5FBF] = D_800C3DFC[0x23];
    D_800C34B0[0x5FC0] = D_800C34B0[0x5FC2];
    v = D_800C34B0[0x5FBE];
    if ((v & 0x3F) == 0) {
        D_800C34B0[0x5FBE] = ((a0 | a1) >> 12) | v;
    }
}
#endif /* XENO_PC_PORT */
