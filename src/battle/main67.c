#include "common.h"


#ifndef XENO_PC_PORT
extern void SetPolyFT4(void* p);
extern void SetSemiTrans(void* p, int abe);
extern u_short GetClut(int x, int y);
extern u_short GetTPage(int tp, int abr, int x, int y);

/* Initialise all (count + 1) 124-byte entries of the list at p (+0 entries,
 * +4 s16 count): +0x16 = -1, +0x1E = 0, and for both double-buffered
 * POLY_FT4s at +0x2C/+0x54: SetPolyFT4, semi-trans on, CLUT (0,0x1CD),
 * tpage (0,1,0x380,0) and fixed u/v corners. */
void func_800A2D5C(u8* p) {
    u8* e = *(u8**)p;
    s32 i;
    s32 j;

    for (i = 0; i < *(s16*)(p + 4) + 1; e += 0x7C, i++) {
        *(s16*)(e + 0x16) = -1;
        *(u16*)(e + 0x1E) = 0;
        for (j = 0; j < 2; j++) {
            u8* prim = e + 0x2C + j * 0x28;
            u8* q = e + j * 0x28;
            u16 tpage;

            SetPolyFT4(prim);
            SetSemiTrans(prim, 1);
            *(u16*)(q + 0x3A) = GetClut(0, 0x1CD);
            tpage = GetTPage(0, 1, 0x380, 0);
            q[0x48] = 0xF;
            *(u16*)(q + 0x42) = tpage;
            q[0x38] = 0;
            q[0x39] = 0xC1;
            q[0x40] = 0;
            q[0x41] = 0xC1;
            q[0x49] = 0xC1;
            q[0x50] = 0xF;
            q[0x51] = 0xC1;
        }
    }
}

extern void SetSemiTrans(void* p, int abe);

/* Allocate the entry at the list cursor (+6) when it is below the count (+4)
 * and free (+0x16 == -1): advance the cursor past the following used
 * entries, set semi-trans on both prims and return it.  Otherwise return the
 * entry at index count (the spare slot). */
u8* func_800A2E88(u8* list, s16 semi) {
    s16 cur = *(s16*)(list + 6);
    u8* e;

    if (cur < *(s16*)(list + 4)) {
        e = (u8*)(cur * 0x7C + *(u32*)list);
        if (*(s16*)(e + 0x16) == -1) {
            for (*(s16*)(list + 6) = cur + 1; *(s16*)(list + 6) < *(s16*)(list + 4); (*(s16*)(list + 6))++) {
                if (*(s16*)(*(u8**)list + *(s16*)(list + 6) * 0x7C + 0x16) == -1) {
                    break;
                }
            }
            SetSemiTrans(e + 0x2C, semi);
            SetSemiTrans(e + 0x54, semi);
            return e;
        }
    }
    return (u8*)(*(s16*)(list + 4) * 0x7C + *(u32*)list);
}

INCLUDE_ASM("asm/battle/nonmatchings/main67", func_800A2F94);
INCLUDE_ASM("asm/battle/nonmatchings/main67", func_800A2FD8);
INCLUDE_ASM("asm/battle/nonmatchings/main67", func_800A32D8);
#endif


/* func_800A3484.s */
void func_800A3484(s16* p) {
    *p = -1;
}


/* ---- Port bodies (fo/bat3 gfx helper).  Not byte-matching: the matching
 * build keeps the retail bytes from the INCLUDE_ASM block above; these are
 * compiled only for the port and the differential harness. ---- */
#ifdef XENO_PC_PORT
#include "psyq/libgte.h"
#include "psyq/libgpu.h"
#include <psx/inline_c.h>

extern s32 D_80050100;

/* A 32-bit pointer slot read from guest memory, resolved the way the battle
 * runtime resolves it (resolve_memory/translate_argument): KSEG0/KSEG1 is
 * guest RAM, 0x1F800xxx is the scratchpad, anything else is already a
 * (sub-4 GiB, -no-pie) host address stored by native port code. */
static inline u8* gfx67_h(u32 v) {
    if ((v & 0xFF800000u) == 0x80000000u || (v & 0xFF800000u) == 0xA0000000u) {
        return (u8*)PSX_ADDR(v);
    }
    if ((v & 0xFFFFF000u) == 0x1F800000u) {
        return g_PsxScratchpad + (v & 0xFFF);
    }
    return (u8*)(uintptr_t)v;
}
/* Host pointer -> the 32-bit value retail would hold for it. */
static inline u32 gfx67_g(const void* p) {
    return PsxMemory_GuestAddr(p);
}
/* R3000 DIV: retail traps (break 7) on a zero divisor; the host must not
 * fault, so it yields the LO the hardware leaves. */
static inline s32 gfx67_div(s32 a, s32 b) {
    if (b == 0) {
        return (a >= 0) ? -1 : 1;
    }
    if (b == -1 && a == (s32)0x80000000) {
        return a;
    }
    return a / b;
}

/* func_800A2D5C.s: initialise all (count + 1) 124-byte entries of the list
 * at p (+0 entries, +4 s16 count): +0x16 = -1, +0x1E = 0, and for both
 * double-buffered POLY_FT4s at +0x2C/+0x54: SetPolyFT4, semi-trans on,
 * CLUT (0,0x1CD), tpage (0,1,0x380,0) and fixed u/v corners. */
void func_800A2D5C(u8* p) {
    u8* e = gfx67_h(*(u32*)(p + 0));
    s32 i;

    for (i = 0; i < *(s16*)(p + 4) + 1; i++, e += 0x7C) {
        s32 j;

        *(s16*)(e + 0x16) = -1;
        *(u16*)(e + 0x1E) = 0;
        for (j = 0; j < 2; j++) {
            u8* prim = e + 0x2C + j * 0x28;
            u8* q = e + j * 0x28;
            u16 tpage;

            SetPolyFT4((POLY_FT4*)prim);
            SetSemiTrans(prim, 1);
            *(u16*)(q + 0x3A) = GetClut(0, 0x1CD);
            tpage = GetTPage(0, 1, 0x380, 0);
            q[0x48] = 0xF;
            *(u16*)(q + 0x42) = tpage;
            q[0x38] = 0;
            q[0x39] = 0xC1;
            q[0x40] = 0;
            q[0x41] = 0xC1;
            q[0x49] = 0xC1;
            q[0x50] = 0xF;
            q[0x51] = 0xC1;
        }
    }
}

/* func_800A2E88.s: allocate the entry at the list cursor (+6) when it is
 * below the count (+4) and free (+0x16 == -1): advance the cursor past the
 * following used entries, set semi-trans on both prims and return it.
 * Otherwise return the entry at index count (the spare slot). */
u8* func_800A2E88(u8* list, s32 semi) {
    s16 cur = *(s16*)(list + 6);

    if (cur < *(s16*)(list + 4)) {
        u8* e = gfx67_h(*(u32*)(list + 0)) + cur * 0x7C;

        if (*(s16*)(e + 0x16) == -1) {
            s16 n;

            *(s16*)(list + 6) = cur + 1;
            n = *(s16*)(list + 4);
            if ((s16)(cur + 1) < n) {
                u8* base = gfx67_h(*(u32*)(list + 0));

                while (1) {
                    s16 c = *(s16*)(list + 6);

                    if (*(s16*)(base + c * 0x7C + 0x16) == -1) {
                        break;
                    }
                    *(s16*)(list + 6) = c + 1;
                    if (!((s16)(c + 1) < n)) {
                        break;
                    }
                }
            }
            SetSemiTrans(e + 0x2C, (s16)semi);
            SetSemiTrans(e + 0x54, (s16)semi);
            return e;
        }
    }
    return gfx67_h(*(u32*)(list + 0)) + *(s16*)(list + 4) * 0x7C;
}

/* func_800A2F94.s: release entry e of the list: its index ((e - base) / 124)
 * lowers the cursor (+6) when smaller, +0x16 = -1; returns the index. */
u32 func_800A2F94(u8* list, u8* e) {
    u32 d = (gfx67_g(e) - *(u32*)(list + 0)) >> 2;
    u32 idx = (u32)(((u64)d * 0x21084211u) >> 32) >> 2;

    if (!(*(s16*)(list + 6) < (s32)idx)) {
        *(u16*)(list + 6) = (u16)idx;
    }
    *(s16*)(e + 0x16) = -1;
    return idx;
}

/* func_800A2FD8.s: draw and age every live entry of the list: an expired
 * entry (+0x16 >= +0x1E) is released through func_800A2F94; otherwise the
 * colour (+0x20/+0x22/+0x24 >> 6) goes into buffer `buf`'s POLY_FT4, whose
 * corners are either copied as screen xy (+0xE == 0) or projected with
 * RTPT/RTPS through m, and the prim is linked into ot (at slot 0, or at
 * SZ3 >> 2 >> D_80050100).  Then age += step and colour -= velocity*step. */
void func_800A2FD8(u8* list, MATRIX* m, s32 step, u32* ot, s32 buf) {
    u8* e;
    s32 i;

    SetRotMatrix(m);
    SetTransMatrix(m);
    e = gfx67_h(*(u32*)(list + 0));
    for (i = 0; i < *(s16*)(list + 4); i++, e += 0x7C) {
        s16 age = *(s16*)(e + 0x16);
        u32 off;
        u8* q;
        u8* prim;
        u32* slot;
        s32 vx, vy, vz;

        if (age == -1) {
            continue;
        }
        if (!(age < *(s16*)(e + 0x1E))) {
            func_800A2F94(list, e);
            continue;
        }
        off = buf * 0x28;
        q = e + off;
        prim = e + off + 0x2C;
        q[0x30] = *(u16*)(e + 0x20) >> 6;
        q[0x31] = *(u16*)(e + 0x22) >> 6;
        q[0x32] = *(u16*)(e + 0x24) >> 6;
        if (*(s16*)(e + 0xE) == 0) {
            *(u16*)(q + 0x34) = *(u16*)(e + 0x0);
            *(u16*)(q + 0x36) = *(u16*)(e + 0x2);
            *(u16*)(q + 0x3C) = *(u16*)(e + 0x8);
            *(u16*)(q + 0x3E) = *(u16*)(e + 0xA);
            *(u16*)(q + 0x44) = *(u16*)(e + 0x10);
            *(u16*)(q + 0x46) = *(u16*)(e + 0x12);
            *(u16*)(q + 0x4C) = *(u16*)(e + 0x18);
            *(u16*)(q + 0x4E) = *(u16*)(e + 0x1A);
            slot = &ot[0];
        } else {
            s32 z;

            MTC2(*(u32*)(e + 0x0), 0);
            MTC2(*(u32*)(e + 0x4), 1);
            MTC2(*(u32*)(e + 0x8), 2);
            MTC2(*(u32*)(e + 0xC), 3);
            MTC2(*(u32*)(e + 0x10), 4);
            MTC2(*(u32*)(e + 0x14), 5);
            gte_rtpt();
            *(u32*)(prim + 0x8) = MFC2(12);
            *(u32*)(prim + 0x10) = MFC2(13);
            *(u32*)(prim + 0x18) = MFC2(14);
            z = (s32)MFC2(19) >> 2;
            z = z >> (D_80050100 & 31);
            MTC2(*(u32*)(e + 0x18), 0);
            MTC2(*(u32*)(e + 0x1C), 1);
            gte_rtps();
            *(u32*)(prim + 0x20) = MFC2(14);
            slot = &ot[z];
        }
        *(u32*)(q + 0x2C) = (*(u32*)(q + 0x2C) & 0xFF000000u) | (*slot & 0x00FFFFFFu);
        *slot = (*slot & 0xFF000000u) | (gfx67_g(prim) & 0x00FFFFFFu);
        vx = *(s16*)(e + 0x26) * step;
        vy = *(s16*)(e + 0x28) * step;
        vz = *(s16*)(e + 0x2A) * step;
        *(u16*)(e + 0x16) = *(u16*)(e + 0x16) + step;
        *(u16*)(e + 0x22) = *(u16*)(e + 0x22) - vy;
        *(u16*)(e + 0x20) = *(u16*)(e + 0x20) - vx;
        *(u16*)(e + 0x24) = *(u16*)(e + 0x24) - vz;
    }
}

/* func_800A32D8.s: fill a 0x70-byte trail/particle record (NULL e is a
 * no-op that leaves arg18 in v0).  Colour start (arg6..8 << 6) and the
 * per-step fade ((start - (end << 6)) / steps) with steps = (s16)arg5
 * (retail traps on 0; see gfx67_div).  +0x60 = min((s16)arg4, 7). */
u32 func_800A32D8(u8* e, u32 a1, u32 a2, u32 a3, u32 arg4, u32 arg5, u32 arg6,
                  u32 arg7, u32 arg8, u32 arg9, u32 arg10, u32 arg11,
                  u32 arg12, u32 arg13, u32 arg14, u32 arg15, u32 arg16,
                  u32 arg17, u32 arg18) {
    s32 d;
    s32 r, g, b;

    if (e == NULL) {
        return arg18 & 0xFFFF;
    }
    e[3] = (u8)arg18;
    *(s16*)(e + 0x5E) = -1;
    *(u16*)(e + 0x0) = (u16)a2;
    e[2] = (u8)a3;
    *(u32*)(e + 0x4) = a1;
    *(u16*)(e + 0xC) = (u16)arg12;
    *(u16*)(e + 0xE) = (u16)arg13;
    *(u16*)(e + 0x10) = (u16)arg14;
    *(u16*)(e + 0x14) = (u16)arg15;
    *(u16*)(e + 0x16) = (u16)arg16;
    *(u16*)(e + 0x18) = (u16)arg17;
    *(u16*)(e + 0x5C) = 0;
    if ((s16)arg4 < 7) {
        *(u16*)(e + 0x60) = (u16)arg4;
    } else {
        *(u16*)(e + 0x60) = 7;
    }
    d = (s16)arg5;
    *(u16*)(e + 0x64) = (u16)((arg6 & 0xFF) << 6);
    r = gfx67_div(*(s16*)(e + 0x64) - (s32)((arg9 & 0xFF) << 6), d);
    *(u16*)(e + 0x66) = (u16)((arg7 & 0xFF) << 6);
    g = gfx67_div(*(s16*)(e + 0x66) - (s32)((arg10 & 0xFF) << 6), d);
    *(u16*)(e + 0x68) = (u16)((arg8 & 0xFF) << 6);
    b = gfx67_div(*(s16*)(e + 0x68) - (s32)((arg11 & 0xFF) << 6), d);
    *(u16*)(e + 0x62) = (u16)arg5;
    *(u32*)(e + 0x8) = 0;
    *(u16*)(e + 0x6A) = (u16)r;
    *(u16*)(e + 0x6C) = (u16)g;
    *(u16*)(e + 0x6E) = (u16)b;
    return 0;
}
#endif /* XENO_PC_PORT */
