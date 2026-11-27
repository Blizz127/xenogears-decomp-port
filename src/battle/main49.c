#include "common.h"


#ifndef XENO_PC_PORT
extern u16 D_800C324E;
extern u16 D_800C3250;
extern u16 D_800C3252;
extern u16 D_800C323A;
extern u8 g_GameState[];
extern u8 D_800D2D24[];
extern u16 D_800CCD62__rows[][184] asm("D_800CCD62");
extern u16 D_800CCDC4__rows[][184] asm("D_800CCDC4");
extern u16 D_800CEF10__rows[][42][20] asm("D_800CEF10");
extern u16 D_800CEF34__rows[][42][20] asm("D_800CEF34");
extern u16 func_80089C6C(u16 bits, u8 b);
extern u8* D_800D2D28;
extern void func_8008CCCC(u8 flag);
extern void func_8008CD28(u8 index);
extern u8 func_80085084(u16 arg0, u8 arg1, u8 arg2);
extern void func_8008AA74(u32 v);

/* func_8008CDE4.s, byte-exact: use slot `slot` of party actor `actor`
 * (the func_8008BD50 pattern for a costed entry).  The cost is the D_800CEF34
 * halfword and the target word the D_800CEF10 halfword of entry
 * slot + 0x25 in the actor's 42 x 40-byte table.  It needs the actor's
 * D_800CCDC4 halfword (0x170-byte records) to cover the cost,
 * func_80089C6C to accept the g_GameState + 0x16DA + D_800D2D24[actor] * 32
 * halfword for the slot, and none of the slot's mask bits (D_800C324E,
 * D_800C3250, D_800C3252, D_800C323A, as in func_800939CC) in the
 * actor's D_800CCD62 halfword.  Then func_8008CCCC(1) and D_800D2D28
 * +0xC6 = 1, and func_80085084(target, actor, 0): on commit the cost is
 * paid and it returns 1, else +0xC6 is cleared and func_8008CD28(actor)
 * runs.  When any check fails, func_8008AA74(0x4F).  Returns 0 unless the
 * command was committed.
 * Codegen: the record halfwords go through asm-label row views, which
 * keep the symbol in each address; with a u8 array plus offset, CSE keeps
 * the record pointer in an extra s-register across the calls. */
u8 func_8008CDE4(u8 actor, u8 slot) {
    u16 masks[4];
    u16 cost;
    u16 target;
    u8 fail;
    u8 ret;

    ret = 0;
    masks[0] = D_800C324E;
    masks[1] = D_800C3250;
    masks[2] = D_800C3252;
    masks[3] = D_800C323A;
    cost = D_800CEF34__rows[actor][slot + 0x25][0];
    target = D_800CEF10__rows[actor][slot + 0x25][0];
    fail = 1;
    if (D_800CCDC4__rows[actor][0] >= cost
        && func_80089C6C(*(u16*)(g_GameState + 0x16DA + D_800D2D24[actor] * 32), slot) != 0
        && !(D_800CCD62__rows[actor][0] & masks[slot])) {
        func_8008CCCC(1);
        D_800D2D28[0xC6] = 1;
        if (func_80085084(target, actor, 0)) {
            D_800CCDC4__rows[actor][0] -= cost;
            ret = 1;
        } else {
            D_800D2D28[0xC6] = 0;
            func_8008CD28(actor);
        }
        fail = 0;
    }
    if (fail) {
        func_8008AA74(0x4F);
    }
    return ret;
}
extern u8 D_800D3014;
extern u8* D_800C3EAC;
extern u8 D_800CCD88__rows[][0x170] asm("D_800CCD88");
extern void func_8008F8F4(u8 index, u16 x, u16 y, u16 w, u16 h, u8 animate, u8 refresh);
extern void func_800930AC(u8 a, u8* ids, u16* vals);
extern void func_80077698(void);
extern void func_800939CC(u8 a, u8 b);
extern void func_80090B90(s32 x, s32 y, s32* frame, u8* timer);
extern void func_800716D8(void);
extern void func_8007765C(void);
extern void func_80077980(void);
extern void func_8008FA60(u8 index);

/* func_8008CFB8.s, byte-exact: party actor `actor`'s skill menu.  Four
 * entries: an entry the actor can use (func_80089C6C of the
 * g_GameState + 0x16DA + D_800D2D24[actor] * 32 halfword, none of its
 * mask bits in the actor's D_800CCD62 halfword) shows id = the actor's
 * D_800CCD88 byte * 4 + slot and its D_800CEF34 cost (entry slot + 0x25);
 * others stay 0xFF / 0.  D_800D2D28 +0xCB = 0, three windows
 * (func_8008F8F4), func_800930AC(actor, ids, costs), func_80077698.
 * Then until a choice: func_800939CC(actor, sel) whenever sel changes,
 * the cursor (func_80090B90(0x8C, sel * 16 + 0x58)), a frame
 * (func_800716D8), and the D_800D3014 event: 5 cancels (0), 4 commits
 * through func_8008CDE4 (1, D_800C3EAC +0x2E6 = sel + 0x10), 1 / 3 move
 * the cursor within 0..3.  Finally func_8008CCCC(result), func_8007765C,
 * func_80077980 and func_8008FA60(0..2); returns the result.
 * Codegen: the case bodies are in retail's block order (5, 4, 1, 3; the
 * sorted 1, 3, 4, 5 order also swaps the sel and actor registers); the
 * tables go through the asm-label row views of func_8008CDE4. */
u8 func_8008CFB8(u8 actor) {
    u8 ids[4];
    u16 costs[4];
    u16 masks[4];
    s32 frame;
    u8 timer;
    s32 last;
    s32 sel;
    s32 i;
    u8 state;

    sel = 0;
    last = 0xFF;
    state = 2;
    masks[0] = D_800C324E;
    masks[1] = D_800C3250;
    masks[2] = D_800C3252;
    masks[3] = D_800C323A;
    frame = 4;
    timer = 0;
    for (i = 0; i < 4; i++) {
        ids[i] = 0xFF;
        costs[i] = 0;
    }
    for (i = 0; i < 4; i++) {
        if (func_80089C6C(*(u16*)(g_GameState + 0x16DA + D_800D2D24[actor] * 32), i)) {
            if (!(D_800CCD62__rows[actor][0] & masks[i])) {
                ids[i] = D_800CCD88__rows[actor][0] * 4 + i;
                costs[i] = D_800CEF34__rows[actor][i + 0x25][0];
            }
        }
    }
    D_800D2D28[0xCB] = 0;
    func_8008F8F4(0, 0x74, 0xA0, 0xAC, 0x38, 0, 1);
    func_8008F8F4(1, 0x84, 0x4C, 0x9C, 0x50, 0, 1);
    func_8008F8F4(2, 0x1C, 0xA0, 0x50, 0x18, 0, 1);
    func_800930AC(actor, ids, costs);
    func_80077698();
    while (state == 2) {
        if (sel != last) {
            func_800939CC(actor, sel);
            last = sel;
        }
        func_80090B90(0x8C, sel * 16 + 0x58, &frame, &timer);
        func_800716D8();
        switch (D_800D3014) {
        case 5:
            state = 0;
            break;
        case 4:
            if (func_8008CDE4(actor, sel)) {
                state = 1;
                D_800C3EAC[0x2E6] = sel + 0x10;
            }
            break;
        case 1:
            if (++sel >= 4) {
                sel = 3;
            }
            break;
        case 3:
            if (--sel < 0) {
                sel = 0;
            }
            break;
        }
    }
    func_8008CCCC(state);
    func_8007765C();
    func_80077980();
    func_8008FA60(0);
    func_8008FA60(1);
    func_8008FA60(2);
    return state;
}
extern u8* D_800C3EA4;
extern void SetSemiTrans(void* p, s32 abe);
extern void SetShadeTex(void* p, s32 tge);

/* func_8008D328.s, byte-exact: fade out the two groups of POLY_FT4 prims at
 * D_800C3EA4 + 0x641C + g * 0xFA0 (D_800D2D28 +0xD0 + g * 4 entries each,
 * every other prim from the D_800D2D28 +0xA3 buffer): SetSemiTrans(1),
 * SetShadeTex(0), tpage |= 0x20, and if its r0 byte is nonzero, r0, g0
 * and b0 = r0 - D_800C3EAC +0x2E0 * 16.  When no prim was lit,
 * D_800D2D28 +0xCB = 0.  Codegen: each access is spelled with retail's
 * association (the prim pointer as base + (g * 0xFA0 + 0x641C) + i * 40,
 * the tpage and r0 reads as base + (g * 0xFA0 + i * 40), the colour stores
 * as (base + g * 0xFA0)[i * 40 + off]).  Matches under gcc 2.6.x. */
void func_8008D328(void) {
    u8 active;
    s32 g;
    s32 k;
    u8 c;

    active = 0;
    for (g = 0; g < 2; g++) {
        if (*(s32*)(D_800D2D28 + g * 4 + 0xD0) != 0) {
            for (k = 0; k < *(s32*)(D_800D2D28 + g * 4 + 0xD0) * 2; k += 2) {
                SetSemiTrans(D_800C3EA4 + (g * 0xFA0 + 0x641C) + (k + D_800D2D28[0xA3]) * 40, 1);
                SetShadeTex(D_800C3EA4 + (g * 0xFA0 + 0x641C) + (k + D_800D2D28[0xA3]) * 40, 0);
                *(u16*)(D_800C3EA4 + (g * 0xFA0 + (k + D_800D2D28[0xA3]) * 40) + 0x6432) |= 0x20;
                c = (D_800C3EA4 + (g * 0xFA0 + (k + D_800D2D28[0xA3]) * 40))[0x6420];
                if (c != 0) {
                    active = 1;
                    (D_800C3EA4 + g * 0xFA0)[(k + D_800D2D28[0xA3]) * 40 + 0x6420] = c - D_800C3EAC[0x2E0] * 16;
                    (D_800C3EA4 + g * 0xFA0)[(k + D_800D2D28[0xA3]) * 40 + 0x6421] = c - D_800C3EAC[0x2E0] * 16;
                    (D_800C3EA4 + g * 0xFA0)[(k + D_800D2D28[0xA3]) * 40 + 0x6422] = c - D_800C3EAC[0x2E0] * 16;
                }
            }
        }
    }
    if (active == 0) {
        D_800D2D28[0xCB] = 0;
    }
}
extern u8* D_800C3000[];
extern u16* D_800C2F4C[];
extern s32 D_800CCB34;
extern s32 D_800CCB34__a[] asm("D_800CCB34");
extern s32 func_80076A10(s32 glyph, u8* prims, s16 x, s16 y);

/* func_8008D598.s, byte-exact: draw the battle menu glyph groups (fade != 0:
 * func_8008D328 and return the +0xA3 byte; else lay out each group's glyph
 * list through func_80076A10 and tint the new prims; returns D_800CCB34).
 * CONSTRUCTS (counted separately):
 * - D_800CCB34 is read in three spellings (the __a alias as [0] and as *,
 *   plus the scalar), each chosen for its RTL expansion path.
 * - Duplicated `k += 4` before continue (the biv-count gate: loop.c judges
 *   the k*2 giv "not worth while", so k stays an index as in retail).
 * - The attr halfword is read twice in two spellings, so CSE keeps both
 *   loads.
 * - Pointer-sum spellings; the base temp `b` is set twice (base first,
 *   then += actor << 6) for retail's addu order.
 * - The once-set `n0 = count * 2` is copied into both inner loop counters
 *   (per the helper: sched1 then places the shift right before the jal and
 *   count stays in t0; mechanism not dump-verified by the lead). */
s32 func_8008D598(u8 actor, u8 menu, u8 fade) {
    u16 grp[2];
    u8 lst[2];
    s32 i;
    s32 j;
    s32 k;
    s32 n;
    s32 n0;
    s32 code;
    u16 attr;
    u32 c;
    u16* e;
    u8* b;

    if (fade) {
        func_8008D328();
        return D_800D2D28[0xA3];
    }
    for (i = 0; i < 2; i++) {
        grp[i] = D_800C3000[menu][i];
        lst[i] = *(D_800C3000[menu] + i + 2);
        *(s32*)(D_800D2D28 + i * 4 + 0xD0) = 0;
    }
    for (j = 0; j < 2; j++) {
        if (grp[j] == 0xFF) {
            break;
        }
        k = 0;
        while ((code = D_800C2F4C[lst[j]][k]) != 0xFFFF) {
            if (code >= 0x4000) {
                *(D_800C3EAC + (code & 3) + 0x2C0) = *(D_800C3EAC + (actor << 6) + (code & 0xFF));
                code = *(D_800C3EAC + (actor << 6) + (code & 0xFF));
                if (code == 0xFF) {
                    k += 4;
                    continue;
                }
            }
            n0 = *(s32*)(D_800D2D28 + grp[j] * 4 + 0xD0) * 2;
            *(s32*)(D_800D2D28 + grp[j] * 4 + 0xD0) +=
                func_80076A10(code, D_800C3EA4 + (grp[j] * 0xFA0 + 0x641C) +
                                        *(s32*)(D_800D2D28 + grp[j] * 4 + 0xD0) * 0x50,
                              D_800C2F4C[lst[j]][k + 1], D_800C2F4C[lst[j]][k + 2]);
            e = D_800C2F4C[lst[j]] + k;
            attr = e[3];
            c = (attr & 0xFF) >> 1;
            if (D_800C2F4C[lst[j]][k + 3] >= 0x2000) {
                c = 0x10;
                b = D_800C3EAC;
                b += actor << 6;
                if (*(u16*)(b + (attr & 0xF) * 2 + 0x1C) == 0) {
                    c = (attr & 0xF0) >> 1;
                }
            }
            if (c != 0) {
                for (n = n0; n < *(s32*)(D_800D2D28 + grp[j] * 4 + 0xD0) * 2; n += 2) {
                    SetSemiTrans(D_800C3EA4 + (grp[j] * 0xFA0 + 0x641C) + (n + D_800CCB34__a[0]) * 40, 1);
                    SetShadeTex(D_800C3EA4 + (grp[j] * 0xFA0 + 0x641C) + (n + D_800CCB34__a[0]) * 40, 0);
                    (D_800C3EA4 + grp[j] * 0xFA0)[(n + D_800CCB34) * 40 + 0x6420] = c;
                    (D_800C3EA4 + grp[j] * 0xFA0)[(n + D_800CCB34) * 40 + 0x6421] = c;
                    (D_800C3EA4 + grp[j] * 0xFA0)[(n + D_800CCB34) * 40 + 0x6422] = c;
                    *(u16*)(D_800C3EA4 + (grp[j] * 0xFA0 + (n + *D_800CCB34__a) * 40) + 0x6432) |= 0x20;
                }
            } else {
                for (n = n0; n < *(s32*)(D_800D2D28 + grp[j] * 4 + 0xD0) * 2; n += 2) {
                    (D_800C3EA4 + grp[j] * 0xFA0)[(n + D_800CCB34) * 40 + 0x6420] = 0x80;
                    (D_800C3EA4 + grp[j] * 0xFA0)[(n + D_800CCB34) * 40 + 0x6421] = 0x80;
                    (D_800C3EA4 + grp[j] * 0xFA0)[(n + D_800CCB34) * 40 + 0x6422] = 0x80;
                }
            }
            k += 4;
        }
    }
    return D_800CCB34;
}

extern u32 D_800D2E38[];
extern u8 D_800C3E4C;
extern s32 D_800CCB34;
extern s32 func_80076A10(s32 glyph, u8* prims, s16 x, s16 y);
extern void func_80076B00(u8* prim);

/* Window frame corners: restart the window block's glyph count (+0x5A0)
 * and lay the four 8x8 corner glyphs (one set per D_800C3E4C mode) at the
 * corners of (x, y, w, h) through func_80076A10, each taking the next
 * 0x50-byte prim pair; then func_80076B00 on the current buffer's prim of
 * the four corner pairs. */
void func_8008DC34(u32 kind, u16 x, u16 y, u16 w, u16 h) {
    u8* blk;
    u8 glyph[4];
    s32 i;

    blk = (u8*)D_800D2E38[kind & 0xFF];
    if (D_800C3E4C != 0) {
        glyph[0] = 0x4A;
        glyph[1] = 0x4C;
        glyph[2] = 0x4F;
        glyph[3] = 0x51;
    } else {
        glyph[0] = 0xF0;
        glyph[1] = 0xF2;
        glyph[2] = 0xF5;
        glyph[3] = 0xF7;
    }
    *(s32*)(blk + 0x5A0) = 0;
    *(s32*)(blk + 0x5A0) += func_80076A10(glyph[0], blk + *(s32*)(blk + 0x5A0) * 0x50, x, y);
    *(s32*)(blk + 0x5A0) += func_80076A10(glyph[1], blk + *(s32*)(blk + 0x5A0) * 0x50, x + w - 8, y);
    *(s32*)(blk + 0x5A0) += func_80076A10(glyph[2], blk + *(s32*)(blk + 0x5A0) * 0x50, x, y + h - 8);
    *(s32*)(blk + 0x5A0) += func_80076A10(glyph[3], blk + *(s32*)(blk + 0x5A0) * 0x50, x + w - 8, y + h - 8);
    for (i = 0; i < 4; i++) {
        func_80076B00(blk + (i * 2 + D_800CCB34) * 0x28);
    }
}
extern s32 D_800CCB34__a[] asm("D_800CCB34");
extern u8* D_800C3EA4__a[] asm("D_800C3EA4");

/* Current-buffer prim field at block offset `off` (prims are 0x28 apart per
 * buffer), and a byte of the battle UI state (frame texture cells). */
#define M49_PRIM(off) (blk + D_800CCB34__a[0] * 0x28 + (off))
#define M49_UI(off) (D_800C3EA4__a[0][off])

/* Window top edge: two POLY_FT4 (block +0x140 / +0x190, per buffer) span
 * the 16-pixel band between the corners, half the inner width each, with
 * the frame texture's 8x16 edge cell; then func_80076B00 on both. */
void func_8008DE04(u32 kind, u16 x, u16 y, u16 w) {
    u8* blk;
    s32 i;

    blk = (u8*)D_800D2E38[kind & 0xFF];
    *(s16*)M49_PRIM(0x148) = x + 8;
    *(s16*)M49_PRIM(0x14A) = y - 8;
    *(s16*)M49_PRIM(0x150) = x + 8 + (w - 16) / 2;
    *(s16*)M49_PRIM(0x152) = y - 8;
    *(s16*)M49_PRIM(0x158) = x + 8;
    *(s16*)M49_PRIM(0x15A) = y + 8;
    *(s16*)M49_PRIM(0x160) = x + 8 + (w - 16) / 2;
    *(s16*)M49_PRIM(0x162) = y + 8;
    *(s16*)M49_PRIM(0x198) = x + 8 + (w - 16) / 2;
    *(s16*)M49_PRIM(0x19A) = y - 8;
    *(s16*)M49_PRIM(0x1A0) = x + 8 + (w - 16) / 2 + (w - 16) / 2;
    *(s16*)M49_PRIM(0x1A2) = y - 8;
    *(s16*)M49_PRIM(0x1A8) = x + 8 + (w - 16) / 2;
    *(s16*)M49_PRIM(0x1AA) = y + 8;
    *(s16*)M49_PRIM(0x1B0) = x + 8 + (w - 16) / 2 + (w - 16) / 2;
    *(s16*)M49_PRIM(0x1B2) = y + 8;
    *M49_PRIM(0x14C) = (M49_UI(0xA25C) & 0x3F) * 2;
    *M49_PRIM(0x14D) = M49_UI(0xA260);
    *M49_PRIM(0x154) = (M49_UI(0xA25C) & 0x3F) * 2 + 7;
    *M49_PRIM(0x155) = M49_UI(0xA260);
    *M49_PRIM(0x15C) = (M49_UI(0xA25C) & 0x3F) * 2;
    *M49_PRIM(0x15D) = M49_UI(0xA260) + 0x10;
    *M49_PRIM(0x164) = (M49_UI(0xA25C) & 0x3F) * 2 + 7;
    *M49_PRIM(0x165) = M49_UI(0xA260) + 0x10;
    *M49_PRIM(0x19C) = (M49_UI(0xA25C) & 0x3F) * 2;
    *M49_PRIM(0x19D) = M49_UI(0xA260);
    *M49_PRIM(0x1A4) = (M49_UI(0xA25C) & 0x3F) * 2 + 7;
    *M49_PRIM(0x1A5) = M49_UI(0xA260);
    *M49_PRIM(0x1AC) = (M49_UI(0xA25C) & 0x3F) * 2;
    *M49_PRIM(0x1AD) = M49_UI(0xA260) + 0x10;
    *M49_PRIM(0x1B4) = (M49_UI(0xA25C) & 0x3F) * 2 + 7;
    *M49_PRIM(0x1B5) = M49_UI(0xA260) + 0x10;
    for (i = 0; i < 2; i++) {
        func_80076B00(blk + ((i * 2 + D_800CCB34__a[0]) * 0x28 + 0x140));
    }
}
/* Halfword prim stores go through a one-field struct.  With them the stack
 * load of `h` lands at its first use, as in retail; with raw `*(s16*)`
 * stores it stays at the function entry (func_8008E430, func_8008F0A8). */
typedef struct {
    s16 v;
} M49Half;
#define M49_P16(off) (((M49Half*)M49_PRIM(off))->v)

/* func_8008E430.s: the window's bottom edge -- two POLY_FT4 (block +0x1E0 /
 * +0x230) along the y + h side, half the inner width each, textured from
 * the frame's bottom cell -- then func_80076B00 on the current buffer's two.
 * `h` is read as a full word here; func_8008F6E4 passes it as u16. */
void func_8008E430(u32 kind, u16 x, u16 y, u16 w, s32 h) {
    u8* blk;
    s32 i;

    blk = (u8*)D_800D2E38[kind & 0xFF];
    M49_P16(0x1E8) = x + 8;
    M49_P16(0x1EA) = y + h - 8;
    M49_P16(0x1F0) = x + 8 + (w - 16) / 2;
    M49_P16(0x1F2) = y + h - 8;
    M49_P16(0x1F8) = x + 8;
    M49_P16(0x1FA) = y + h + 8;
    M49_P16(0x200) = x + 8 + (w - 16) / 2;
    M49_P16(0x202) = y + h + 8;
    M49_P16(0x238) = x + 8 + (w - 16) / 2;
    M49_P16(0x23A) = y + h - 8;
    M49_P16(0x240) = x + 8 + (w - 16) / 2 + (w - 16) / 2;
    M49_P16(0x242) = y + h - 8;
    M49_P16(0x248) = x + 8 + (w - 16) / 2;
    M49_P16(0x24A) = y + h + 8;
    M49_P16(0x250) = x + 8 + (w - 16) / 2 + (w - 16) / 2;
    M49_P16(0x252) = y + h + 8;
    *M49_PRIM(0x1EC) = (M49_UI(0xA274) & 0x3F) * 2 - 8;
    *M49_PRIM(0x1ED) = M49_UI(0xA278);
    *M49_PRIM(0x1F4) = (M49_UI(0xA274) & 0x3F) * 2 - 1;
    *M49_PRIM(0x1F5) = M49_UI(0xA278);
    *M49_PRIM(0x1FC) = (M49_UI(0xA274) & 0x3F) * 2 - 8;
    *M49_PRIM(0x1FD) = M49_UI(0xA278) + 0x10;
    *M49_PRIM(0x204) = (M49_UI(0xA274) & 0x3F) * 2 - 1;
    *M49_PRIM(0x205) = M49_UI(0xA278) + 0x10;
    *M49_PRIM(0x23C) = (M49_UI(0xA274) & 0x3F) * 2 - 8;
    *M49_PRIM(0x23D) = M49_UI(0xA278);
    *M49_PRIM(0x244) = (M49_UI(0xA274) & 0x3F) * 2 - 1;
    *M49_PRIM(0x245) = M49_UI(0xA278);
    *M49_PRIM(0x24C) = (M49_UI(0xA274) & 0x3F) * 2 - 8;
    *M49_PRIM(0x24D) = M49_UI(0xA278) + 0x10;
    *M49_PRIM(0x254) = (M49_UI(0xA274) & 0x3F) * 2 - 1;
    *M49_PRIM(0x255) = M49_UI(0xA278) + 0x10;
    for (i = 0; i < 2; i++) {
        func_80076B00(blk + ((i * 2 + D_800CCB34__a[0]) * 0x28 + 0x1E0));
    }
}
/* Window left edge: two POLY_FT4 (block +0x280 / +0x2D0) span the 16-pixel
 * band between the corners, half the inner height each, with the frame
 * texture's left cell; then func_80076B00 on both. */
void func_8008EA70(u32 kind, u16 x, u16 y, u16 h) {
    u8* blk;
    s32 i;

    blk = (u8*)D_800D2E38[kind & 0xFF];
    *(s16*)M49_PRIM(0x288) = x - 8;
    *(s16*)M49_PRIM(0x28A) = y + 8;
    *(s16*)M49_PRIM(0x290) = x + 8;
    *(s16*)M49_PRIM(0x292) = y + 8;
    *(s16*)M49_PRIM(0x298) = x - 8;
    *(s16*)M49_PRIM(0x29A) = y + 8 + (h - 16) / 2;
    *(s16*)M49_PRIM(0x2A0) = x + 8;
    *(s16*)M49_PRIM(0x2A2) = y + 8 + (h - 16) / 2;
    *(s16*)M49_PRIM(0x2D8) = x - 8;
    *(s16*)M49_PRIM(0x2DA) = y + 8 + (h - 16) / 2;
    *(s16*)M49_PRIM(0x2E0) = x + 8;
    *(s16*)M49_PRIM(0x2E2) = y + 8 + (h - 16) / 2;
    *(s16*)M49_PRIM(0x2E8) = x - 8;
    *(s16*)M49_PRIM(0x2EA) = y + 8 + (h - 16) / 2 + (h - 16) / 2;
    *(s16*)M49_PRIM(0x2F0) = x + 8;
    *(s16*)M49_PRIM(0x2F2) = y + 8 + (h - 16) / 2 + (h - 16) / 2;
    *M49_PRIM(0x28C) = (M49_UI(0xA28C) & 0x3F) * 2 + 0xE;
    *M49_PRIM(0x28D) = M49_UI(0xA290);
    *M49_PRIM(0x294) = (M49_UI(0xA28C) & 0x3F) * 2 + 0x1E;
    *M49_PRIM(0x295) = M49_UI(0xA290);
    *M49_PRIM(0x29C) = (M49_UI(0xA28C) & 0x3F) * 2 + 0xE;
    *M49_PRIM(0x29D) = M49_UI(0xA290) + 7;
    *M49_PRIM(0x2A4) = (M49_UI(0xA28C) & 0x3F) * 2 + 0x1E;
    *M49_PRIM(0x2A5) = M49_UI(0xA290) + 7;
    *M49_PRIM(0x2DC) = (M49_UI(0xA28C) & 0x3F) * 2 + 0xE;
    *M49_PRIM(0x2DD) = M49_UI(0xA290);
    *M49_PRIM(0x2E4) = (M49_UI(0xA28C) & 0x3F) * 2 + 0x1E;
    *M49_PRIM(0x2E5) = M49_UI(0xA290);
    *M49_PRIM(0x2EC) = (M49_UI(0xA28C) & 0x3F) * 2 + 0xE;
    *M49_PRIM(0x2ED) = M49_UI(0xA290) + 7;
    *M49_PRIM(0x2F4) = (M49_UI(0xA28C) & 0x3F) * 2 + 0x1E;
    *M49_PRIM(0x2F5) = M49_UI(0xA290) + 7;
    for (i = 0; i < 2; i++) {
        func_80076B00(blk + ((i * 2 + D_800CCB34__a[0]) * 0x28 + 0x280));
    }
}
/* func_8008F0A8.s: the window's right edge -- two POLY_FT4 (block +0x320 /
 * +0x370) down the x + w side, half the inner height each, textured from the
 * frame's right cell -- then func_80076B00 on the current buffer's two. */
void func_8008F0A8(u32 kind, u16 x, u16 y, u16 w, u16 h) {
    u8* blk;
    s32 i;

    blk = (u8*)D_800D2E38[kind & 0xFF];
    M49_P16(0x328) = x + w - 8;
    M49_P16(0x32A) = y + 8;
    M49_P16(0x330) = x + w + 8;
    M49_P16(0x332) = y + 8;
    M49_P16(0x338) = x + w - 8;
    M49_P16(0x33A) = y + 8 + (h - 16) / 2;
    M49_P16(0x340) = x + w + 8;
    M49_P16(0x342) = y + 8 + (h - 16) / 2;
    M49_P16(0x378) = x + w - 8;
    M49_P16(0x37A) = y + 8 + (h - 16) / 2;
    M49_P16(0x380) = x + w + 8;
    M49_P16(0x382) = y + 8 + (h - 16) / 2;
    M49_P16(0x388) = x + w - 8;
    M49_P16(0x38A) = y + 8 + (h - 16) / 2 + (h - 16) / 2;
    M49_P16(0x390) = x + w + 8;
    M49_P16(0x392) = y + 8 + (h - 16) / 2 + (h - 16) / 2;
    *M49_PRIM(0x32C) = (M49_UI(0xA2A4) & 0x3F) * 2 + 0xE;
    *M49_PRIM(0x32D) = M49_UI(0xA2A8);
    *M49_PRIM(0x334) = (M49_UI(0xA2A4) & 0x3F) * 2 + 0x1E;
    *M49_PRIM(0x335) = M49_UI(0xA2A8);
    *M49_PRIM(0x33C) = (M49_UI(0xA2A4) & 0x3F) * 2 + 0xE;
    *M49_PRIM(0x33D) = M49_UI(0xA2A8) + 7;
    *M49_PRIM(0x344) = (M49_UI(0xA2A4) & 0x3F) * 2 + 0x1E;
    *M49_PRIM(0x345) = M49_UI(0xA2A8) + 7;
    *M49_PRIM(0x37C) = (M49_UI(0xA2A4) & 0x3F) * 2 + 0xE;
    *M49_PRIM(0x37D) = M49_UI(0xA2A8);
    *M49_PRIM(0x384) = (M49_UI(0xA2A4) & 0x3F) * 2 + 0x1E;
    *M49_PRIM(0x385) = M49_UI(0xA2A8);
    *M49_PRIM(0x38C) = (M49_UI(0xA2A4) & 0x3F) * 2 + 0xE;
    *M49_PRIM(0x38D) = M49_UI(0xA2A8) + 7;
    *M49_PRIM(0x394) = (M49_UI(0xA2A4) & 0x3F) * 2 + 0x1E;
    *M49_PRIM(0x395) = M49_UI(0xA2A8) + 7;
    for (i = 0; i < 2; i++) {
        func_80076B00(blk + ((i * 2 + D_800CCB34__a[0]) * 0x28 + 0x320));
    }
}
extern u8* D_800D2D28;
extern u32 D_800D2E38[];
void func_8008DC34(u32 kind, u16 x, u16 y, u16 w, u16 h);
void func_8008DE04(u32 kind, u16 x, u16 y, u16 w);
/* func_8008E430 itself reads h as a full word; this caller passes it as u16. */
extern void func_8008E430__h16(u32 kind, u16 x, u16 y, u16 w, u16 h) asm("func_8008E430");
void func_8008EA70(u32 kind, u16 x, u16 y, u16 h);
void func_8008F0A8(u32 kind, u16 x, u16 y, u16 w, u16 h);

/* Draw battle window `kind` at (x, y), w x h: with its D_800D2D28 +0xB0
 * flag dropped, set the corners of the current buffer's background quad
 * (POLY_G4 at block +0x3C0, 0x24 per buffer), rebuild the frame parts
 * (func_8008DC34 .. func_8008F0A8), record the buffer at +0x5A4 and raise
 * the flag again.  The buffer index is re-read for every corner. */
void func_8008F6E4(u8 kind, u16 x, u16 y, u16 w, s32 h) {
    u32 i = kind;
    u8* blk = (u8*)D_800D2E38[i];

    *(D_800D2D28 + i + 0xB0) = 0;
    *(s16*)(blk + D_800CCB34__a[0] * 0x24 + 0x3C8) = x;
    *(s16*)(blk + D_800CCB34__a[0] * 0x24 + 0x3CA) = y;
    *(s16*)(blk + D_800CCB34__a[0] * 0x24 + 0x3D0) = x + w;
    *(s16*)(blk + D_800CCB34__a[0] * 0x24 + 0x3D2) = y;
    *(s16*)(blk + D_800CCB34__a[0] * 0x24 + 0x3D8) = x;
    *(s16*)(blk + D_800CCB34__a[0] * 0x24 + 0x3DA) = y + h;
    *(s16*)(blk + D_800CCB34__a[0] * 0x24 + 0x3E0) = x + w;
    *(s16*)(blk + D_800CCB34__a[0] * 0x24 + 0x3E2) = y + h;
    func_8008DC34(i, x, y, w, h);
    func_8008DE04(i, x, y, w);
    func_8008E430__h16(i, x, y, w, h);
    func_8008EA70(i, x, y, h);
    func_8008F0A8(i, x, y, w, h);
    blk[0x5A4] = *(u8*)D_800CCB34__a;
    *(D_800D2D28 + i + 0xB0) = 1;
}
extern u8* D_800D2D28;
extern u32 D_800D2E38[];
/* Battle window rectangle (D_800D2D90), as main50's open animation reads
 * it. */
typedef struct {
    u16 x;
    u16 y;
    u16 w;
    u16 h;
    u16 curW;
    u16 curH;
    u8 kind;
} BattleWindowRect;
extern BattleWindowRect* D_800D2D90__p[] asm("D_800D2D90");
extern void* func_8008ABB8(s32 size, s32 mode);
extern void bzero(void* p, s32 n);
extern void func_80077454(u32 index);
extern void func_8008F6E4(u8 kind, u16 x, u16 y, u16 w, s32 h);
extern void func_800716D8(void);

/* Open battle window `index` at (x, y) with size w x h.  On first use
 * (D_800D2D28 +0xB0 + index clear) allocate and clear its 0x5A8-byte
 * D_800D2E38 block and 0xE-byte rectangle, then run func_80077454.
 * `animate` stores the rectangle with a zero current size and marks the
 * window shown but not yet open (+0xB8 / +0xBF), for func_8008FAD8 to grow;
 * otherwise it is drawn at full size now (func_8008F6E4), followed by
 * func_800716D8 when `refresh` is set. */
void func_8008F8F4(u8 index, u16 x, u16 y, u16 w, u16 h, u8 animate, u8 refresh) {
    u32 i = index;
    void* p;
    BattleWindowRect* r;

    if (*(D_800D2D28 + i + 0xB0) == 0) {
        p = func_8008ABB8(0x5A8, 0);
        D_800D2E38[i] = (u32)p;
        bzero(p, 0x5A8);
        p = func_8008ABB8(0xE, 0);
        D_800D2D90__p[i] = p;
        bzero(p, 0xE);
        func_80077454(i);
    }
    if (animate != 0) {
        r = D_800D2D90__p[i];
        r->kind = index;
        r->x = x;
        r->y = y;
        r->w = w;
        r->h = h;
        r->curW = 0;
        r->curH = 0;
        *(D_800D2D28 + i + 0xBF) = 0;
        *(D_800D2D28 + i + 0xB8) = 1;
    } else {
        func_8008F6E4(i, x, y, w, h);
        if (refresh != 0) {
            func_800716D8();
        }
    }
}
#undef M49_PRIM
#undef M49_UI
#undef M49_P16
#endif


#ifndef XENO_PC_PORT
extern u8* D_800D2D28;
#endif
#ifndef XENO_PC_PORT
extern u32 D_800D2E38[];
#endif
#ifndef XENO_PC_PORT
extern u32 D_800D2D90[];
#endif
extern void func_800716D8(void);
extern void HeapFree(u32 p);


/* func_8008FA60.s */
void func_8008FA60(u8 index) {
    u32 i = (u32)index & 0xFF;
    u8* p = D_800D2D28;

    *(u8*)((int)(unsigned int)p + i + 0xB0) = 0;
    p = D_800D2D28;
    *(u8*)((int)(unsigned int)p + i + 0xB8) = 0;
    func_800716D8();
    HeapFree(D_800D2E38[i]);
    HeapFree(D_800D2D90[i]);
}
