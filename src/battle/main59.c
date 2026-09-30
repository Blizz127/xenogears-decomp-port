#include "common.h"


#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/battle/nonmatchings/main59", func_8009C134);
INCLUDE_ASM("asm/battle/nonmatchings/main59", func_8009C198);
INCLUDE_ASM("asm/battle/nonmatchings/main59", func_8009C4B4);
INCLUDE_ASM("asm/battle/nonmatchings/main59", func_8009C9C4);
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


/* ---- Port bodies (fo/bat3).  Not byte-matching: the matching build keeps
 * the retail bytes from the INCLUDE_ASM block above; these are compiled only
 * for the port and the differential harness. ---- */
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
