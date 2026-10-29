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

#ifdef XENO_PC_PORT /* h-frontier lifted bodies */
/* Instruction-faithful port bodies (native-only frontier), generated by
 *   tools/scripts/battle_lift_asm.py asm/battle/matchings/<tu>/<fn>.s --args N [--void] --jal-service
 * (include/battle_lift.h; every call goes through the interpreter's own
 * jal path, so each callee gets the binding an interpreted jal gets).  The
 * matched C above stays the matching build's. */
#include "battle_lift.h"

/* func_8008F8F4.s (91 instructions; --args 7 --void), frontier step2.  Callees through the jal service: bzero, func_800716D8, func_80077454, func_8008ABB8, func_8008F6E4. */
void func_8008F8F4(u32 arg0, u32 arg1, u32 arg2, u32 arg3, u32 arg4, u32 arg5, u32 arg6) {
    static const char FN[] = "func_8008F8F4";
    u32 r_at = 0, r_v0 = 0, r_v1 = 0, r_a0 = 0, r_a1 = 0, r_a2 = 0, r_a3 = 0, r_t0 = 0, r_t1 = 0, r_t2 = 0, r_t3 = 0, r_t4 = 0, r_t5 = 0, r_t6 = 0, r_t7 = 0, r_s0 = 0, r_s1 = 0, r_s2 = 0, r_s3 = 0, r_s4 = 0, r_s5 = 0, r_s6 = 0, r_s7 = 0, r_t8 = 0, r_t9 = 0, r_k0 = 0, r_k1 = 0, r_gp = 0, r_sp = 0, r_fp = 0, r_ra = 0;
    u32 hi = 0, lo = 0, bl_saved;
    r_sp = bl_enter(FN, 0x40u, &bl_saved);
    r_a0 = arg0;
    r_a1 = arg1;
    r_a2 = arg2;
    r_a3 = arg3;
    bl_sw(r_sp + 0x10u, arg4);
    bl_sw(r_sp + 0x14u, arg5);
    bl_sw(r_sp + 0x18u, arg6);
    r_v0 = 0x800D0000u; /* 8008F8F4 lui $v0, %hi(D_800D2D28) */
    r_v0 = bl_lw(r_v0 + 0x2D28u); /* 8008F8F8 lw $v0, %lo(D_800D2D28)($v0) */
    r_sp = r_sp + 0xFFFFFFC0u; /* 8008F8FC addiu $sp, $sp, -0x40 */
    bl_sw(r_sp + 0x24u, r_s3); /* 8008F900 sw $s3, 0x24($sp) */
    r_s3 = bl_lhu(r_sp + 0x50u); /* 8008F904 lhu $s3, 0x50($sp) */
    bl_sw(r_sp + 0x2Cu, r_s5); /* 8008F908 sw $s5, 0x2C($sp) */
    r_s5 = bl_lbu(r_sp + 0x54u); /* 8008F90C lbu $s5, 0x54($sp) */
    bl_sw(r_sp + 0x20u, r_s2); /* 8008F910 sw $s2, 0x20($sp) */
    r_s2 = r_a0 + 0u; /* 8008F914 addu $s2, $a0, $zero */
    bl_sw(r_sp + 0x28u, r_s4); /* 8008F918 sw $s4, 0x28($sp) */
    r_s4 = r_a1 + 0u; /* 8008F91C addu $s4, $a1, $zero */
    bl_sw(r_sp + 0x30u, r_s6); /* 8008F920 sw $s6, 0x30($sp) */
    r_s6 = r_a2 + 0u; /* 8008F924 addu $s6, $a2, $zero */
    bl_sw(r_sp + 0x34u, r_s7); /* 8008F928 sw $s7, 0x34($sp) */
    bl_sw(r_sp + 0x1Cu, r_s1); /* 8008F92C sw $s1, 0x1C($sp) */
    r_s1 = r_s2 & 0xFFu; /* 8008F930 andi $s1, $s2, 0xFF */
    bl_sw(r_sp + 0x3Cu, r_ra); /* 8008F934 sw $ra, 0x3C($sp) */
    bl_sw(r_sp + 0x38u, r_fp); /* 8008F938 sw $fp, 0x38($sp) */
    bl_sw(r_sp + 0x18u, r_s0); /* 8008F93C sw $s0, 0x18($sp) */
    r_v0 = r_v0 + r_s1; /* 8008F940 addu $v0, $v0, $s1 */
    r_v0 = bl_lbu(r_v0 + 0xB0u); /* 8008F944 lbu $v0, 0xB0($v0) */
    r_fp = bl_lbu(r_sp + 0x58u); /* 8008F948 lbu $fp, 0x58($sp) */
    /* 8008F94C bnez $v0, .L8008F9A8 */
    { int c_ = (r_v0 != 0); r_s7 = r_a3 + 0u; if (c_) goto L_8008F9A8; }
    r_a0 = 0u | 0x5A8u; /* 8008F954 ori $a0, $zero, 0x5A8 */
    /* 8008F958 jal func_8008ABB8 */
    r_a1 = 0u + 0u;
    bl_jal(FN, 0x8008ABB8u, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* func_8008ABB8 */
    r_a0 = r_v0 + 0u; /* 8008F960 addu $a0, $v0, $zero */
    r_s0 = r_s1 << 2; /* 8008F964 sll $s0, $s1, 2 */
    r_at = 0x800D0000u; /* 8008F968 lui $at, %hi(D_800D2E38) */
    r_at = r_at + r_s0; /* 8008F96C addu $at, $at, $s0 */
    bl_sw(r_at + 0x2E38u, r_a0); /* 8008F970 sw $a0, %lo(D_800D2E38)($at) */
    /* 8008F974 jal bzero */
    r_a1 = 0u | 0x5A8u;
    bl_jal(FN, 0x8003F8E8u, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* bzero */
    r_a0 = 0u | 0xEu; /* 8008F97C ori $a0, $zero, 0xE */
    /* 8008F980 jal func_8008ABB8 */
    r_a1 = 0u + 0u;
    bl_jal(FN, 0x8008ABB8u, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* func_8008ABB8 */
    r_a0 = r_v0 + 0u; /* 8008F988 addu $a0, $v0, $zero */
    r_at = 0x800D0000u; /* 8008F98C lui $at, %hi(D_800D2D90) */
    r_at = r_at + r_s0; /* 8008F990 addu $at, $at, $s0 */
    bl_sw(r_at + 0x2D90u, r_a0); /* 8008F994 sw $a0, %lo(D_800D2D90)($at) */
    /* 8008F998 jal bzero */
    r_a1 = 0u | 0xEu;
    bl_jal(FN, 0x8003F8E8u, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* bzero */
    /* 8008F9A0 jal func_80077454 */
    r_a0 = r_s1 + 0u;
    bl_jal(FN, 0x80077454u, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* func_80077454 */
L_8008F9A8:
    /* 8008F9A8 beqz $s5, .L8008FA04 */
    { int c_ = (r_s5 == 0); r_v0 = r_s1 << 2; if (c_) goto L_8008FA04; }
    r_at = 0x800D0000u; /* 8008F9B0 lui $at, %hi(D_800D2D90) */
    r_at = r_at + r_v0; /* 8008F9B4 addu $at, $at, $v0 */
    r_v0 = bl_lw(r_at + 0x2D90u); /* 8008F9B8 lw $v0, %lo(D_800D2D90)($at) */
    ; /* 8008F9BC nop  */
    bl_sb(r_v0 + 0xCu, r_s2); /* 8008F9C0 sb $s2, 0xC($v0) */
    r_v1 = 0x800D0000u; /* 8008F9C4 lui $v1, %hi(D_800D2D28) */
    r_v1 = bl_lw(r_v1 + 0x2D28u); /* 8008F9C8 lw $v1, %lo(D_800D2D28)($v1) */
    bl_sh(r_v0, r_s4); /* 8008F9CC sh $s4, 0x0($v0) */
    bl_sh(r_v0 + 0x2u, r_s6); /* 8008F9D0 sh $s6, 0x2($v0) */
    bl_sh(r_v0 + 0x4u, r_s7); /* 8008F9D4 sh $s7, 0x4($v0) */
    bl_sh(r_v0 + 0x6u, r_s3); /* 8008F9D8 sh $s3, 0x6($v0) */
    bl_sh(r_v0 + 0x8u, 0u); /* 8008F9DC sh $zero, 0x8($v0) */
    bl_sh(r_v0 + 0xAu, 0u); /* 8008F9E0 sh $zero, 0xA($v0) */
    r_v1 = r_v1 + r_s1; /* 8008F9E4 addu $v1, $v1, $s1 */
    bl_sb(r_v1 + 0xBFu, 0u); /* 8008F9E8 sb $zero, 0xBF($v1) */
    r_v0 = 0x800D0000u; /* 8008F9EC lui $v0, %hi(D_800D2D28) */
    r_v0 = bl_lw(r_v0 + 0x2D28u); /* 8008F9F0 lw $v0, %lo(D_800D2D28)($v0) */
    r_v1 = 0u | 0x1u; /* 8008F9F4 ori $v1, $zero, 0x1 */
    r_v0 = r_v0 + r_s1; /* 8008F9F8 addu $v0, $v0, $s1 */
    /* 8008F9FC j .L8008FA2C */
    bl_sb(r_v0 + 0xB8u, r_v1); goto L_8008FA2C;
L_8008FA04:
    bl_sw(r_sp + 0x10u, r_s3); /* 8008FA04 sw $s3, 0x10($sp) */
    r_a0 = r_s1 + 0u; /* 8008FA08 addu $a0, $s1, $zero */
    r_a1 = r_s4 & 0xFFFFu; /* 8008FA0C andi $a1, $s4, 0xFFFF */
    r_a2 = r_s6 & 0xFFFFu; /* 8008FA10 andi $a2, $s6, 0xFFFF */
    /* 8008FA14 jal func_8008F6E4 */
    r_a3 = r_s7 & 0xFFFFu;
    bl_jal(FN, 0x8008F6E4u, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* func_8008F6E4 */
    /* 8008FA1C beqz $fp, .L8008FA2C */
    { int c_ = (r_fp == 0); ; if (c_) goto L_8008FA2C; }
    /* 8008FA24 jal func_800716D8 */
    ;
    bl_jal(FN, 0x800716D8u, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* func_800716D8 */
L_8008FA2C:
    r_ra = bl_lw(r_sp + 0x3Cu); /* 8008FA2C lw $ra, 0x3C($sp) */
    r_fp = bl_lw(r_sp + 0x38u); /* 8008FA30 lw $fp, 0x38($sp) */
    r_s7 = bl_lw(r_sp + 0x34u); /* 8008FA34 lw $s7, 0x34($sp) */
    r_s6 = bl_lw(r_sp + 0x30u); /* 8008FA38 lw $s6, 0x30($sp) */
    r_s5 = bl_lw(r_sp + 0x2Cu); /* 8008FA3C lw $s5, 0x2C($sp) */
    r_s4 = bl_lw(r_sp + 0x28u); /* 8008FA40 lw $s4, 0x28($sp) */
    r_s3 = bl_lw(r_sp + 0x24u); /* 8008FA44 lw $s3, 0x24($sp) */
    r_s2 = bl_lw(r_sp + 0x20u); /* 8008FA48 lw $s2, 0x20($sp) */
    r_s1 = bl_lw(r_sp + 0x1Cu); /* 8008FA4C lw $s1, 0x1C($sp) */
    r_s0 = bl_lw(r_sp + 0x18u); /* 8008FA50 lw $s0, 0x18($sp) */
    r_sp = r_sp + 0x40u; /* 8008FA54 addiu $sp, $sp, 0x40 */
    /* 8008FA58 jr $ra */
    ; goto bl_exit;
bl_exit:
    bl_leave(bl_saved);
    (void)hi; (void)lo; (void)r_at; (void)r_v1; (void)r_k0; (void)r_k1; (void)r_gp; (void)r_ra;
    (void)r_v0;
}

/* func_8008F6E4.s (132 instructions; --args 5 --void), frontier step2.  Callees through the jal service: func_8008DC34, func_8008DE04, func_8008E430, func_8008EA70, func_8008F0A8. */
void func_8008F6E4(u32 arg0, u32 arg1, u32 arg2, u32 arg3, u32 arg4) {
    static const char FN[] = "func_8008F6E4";
    u32 r_at = 0, r_v0 = 0, r_v1 = 0, r_a0 = 0, r_a1 = 0, r_a2 = 0, r_a3 = 0, r_t0 = 0, r_t1 = 0, r_t2 = 0, r_t3 = 0, r_t4 = 0, r_t5 = 0, r_t6 = 0, r_t7 = 0, r_s0 = 0, r_s1 = 0, r_s2 = 0, r_s3 = 0, r_s4 = 0, r_s5 = 0, r_s6 = 0, r_s7 = 0, r_t8 = 0, r_t9 = 0, r_k0 = 0, r_k1 = 0, r_gp = 0, r_sp = 0, r_fp = 0, r_ra = 0;
    u32 hi = 0, lo = 0, bl_saved;
    r_sp = bl_enter(FN, 0x38u, &bl_saved);
    r_a0 = arg0;
    r_a1 = arg1;
    r_a2 = arg2;
    r_a3 = arg3;
    bl_sw(r_sp + 0x10u, arg4);
    r_sp = r_sp + 0xFFFFFFC8u; /* 8008F6E4 addiu $sp, $sp, -0x38 */
    r_t0 = r_a1 + 0u; /* 8008F6E8 addu $t0, $a1, $zero */
    r_t1 = r_a2 + 0u; /* 8008F6EC addu $t1, $a2, $zero */
    bl_sw(r_sp + 0x2Cu, r_s5); /* 8008F6F0 sw $s5, 0x2C($sp) */
    r_s5 = r_a3 + 0u; /* 8008F6F4 addu $s5, $a3, $zero */
    bl_sw(r_sp + 0x24u, r_s3); /* 8008F6F8 sw $s3, 0x24($sp) */
    r_s3 = r_a0 & 0xFFu; /* 8008F6FC andi $s3, $a0, 0xFF */
    r_v1 = r_s3 << 2; /* 8008F700 sll $v1, $s3, 2 */
    bl_sw(r_sp + 0x1Cu, r_s1); /* 8008F704 sw $s1, 0x1C($sp) */
    r_s1 = r_t0 + 0u; /* 8008F708 addu $s1, $t0, $zero */
    bl_sw(r_sp + 0x18u, r_s0); /* 8008F70C sw $s0, 0x18($sp) */
    r_s0 = r_t1 + 0u; /* 8008F710 addu $s0, $t1, $zero */
    r_t0 = r_t0 + r_s5; /* 8008F714 addu $t0, $t0, $s5 */
    bl_sw(r_sp + 0x28u, r_s4); /* 8008F718 sw $s4, 0x28($sp) */
    r_s4 = bl_lw(r_sp + 0x48u); /* 8008F71C lw $s4, 0x48($sp) */
    r_v0 = 0x800D0000u; /* 8008F720 lui $v0, %hi(D_800D2D28) */
    r_v0 = bl_lw(r_v0 + 0x2D28u); /* 8008F724 lw $v0, %lo(D_800D2D28)($v0) */
    r_a0 = r_s3 + 0u; /* 8008F728 addu $a0, $s3, $zero */
    bl_sw(r_sp + 0x30u, r_ra); /* 8008F72C sw $ra, 0x30($sp) */
    bl_sw(r_sp + 0x20u, r_s2); /* 8008F730 sw $s2, 0x20($sp) */
    r_at = 0x800D0000u; /* 8008F734 lui $at, %hi(D_800D2E38) */
    r_at = r_at + r_v1; /* 8008F738 addu $at, $at, $v1 */
    r_s2 = bl_lw(r_at + 0x2E38u); /* 8008F73C lw $s2, %lo(D_800D2E38)($at) */
    r_v0 = r_v0 + r_s3; /* 8008F740 addu $v0, $v0, $s3 */
    bl_sb(r_v0 + 0xB0u, 0u); /* 8008F744 sb $zero, 0xB0($v0) */
    r_v1 = 0x800D0000u; /* 8008F748 lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 8008F74C lw $v1, %lo(D_800CCB34)($v1) */
    r_s5 = r_s5 & 0xFFFFu; /* 8008F750 andi $s5, $s5, 0xFFFF */
    r_v0 = r_v1 << 3; /* 8008F754 sll $v0, $v1, 3 */
    r_v0 = r_v0 + r_v1; /* 8008F758 addu $v0, $v0, $v1 */
    r_v0 = r_v0 << 2; /* 8008F75C sll $v0, $v0, 2 */
    r_v0 = r_v0 + r_s2; /* 8008F760 addu $v0, $v0, $s2 */
    bl_sh(r_v0 + 0x3C8u, r_s1); /* 8008F764 sh $s1, 0x3C8($v0) */
    r_v1 = 0x800D0000u; /* 8008F768 lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 8008F76C lw $v1, %lo(D_800CCB34)($v1) */
    r_a3 = r_s5 + 0u; /* 8008F770 addu $a3, $s5, $zero */
    r_v0 = r_v1 << 3; /* 8008F774 sll $v0, $v1, 3 */
    r_v0 = r_v0 + r_v1; /* 8008F778 addu $v0, $v0, $v1 */
    r_v0 = r_v0 << 2; /* 8008F77C sll $v0, $v0, 2 */
    r_v0 = r_v0 + r_s2; /* 8008F780 addu $v0, $v0, $s2 */
    bl_sh(r_v0 + 0x3CAu, r_s0); /* 8008F784 sh $s0, 0x3CA($v0) */
    r_v1 = 0x800D0000u; /* 8008F788 lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 8008F78C lw $v1, %lo(D_800CCB34)($v1) */
    r_t1 = r_t1 + r_s4; /* 8008F790 addu $t1, $t1, $s4 */
    r_v0 = r_v1 << 3; /* 8008F794 sll $v0, $v1, 3 */
    r_v0 = r_v0 + r_v1; /* 8008F798 addu $v0, $v0, $v1 */
    r_v0 = r_v0 << 2; /* 8008F79C sll $v0, $v0, 2 */
    r_v0 = r_v0 + r_s2; /* 8008F7A0 addu $v0, $v0, $s2 */
    bl_sh(r_v0 + 0x3D0u, r_t0); /* 8008F7A4 sh $t0, 0x3D0($v0) */
    r_v1 = 0x800D0000u; /* 8008F7A8 lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 8008F7AC lw $v1, %lo(D_800CCB34)($v1) */
    r_s4 = r_s4 & 0xFFFFu; /* 8008F7B0 andi $s4, $s4, 0xFFFF */
    r_v0 = r_v1 << 3; /* 8008F7B4 sll $v0, $v1, 3 */
    r_v0 = r_v0 + r_v1; /* 8008F7B8 addu $v0, $v0, $v1 */
    r_v0 = r_v0 << 2; /* 8008F7BC sll $v0, $v0, 2 */
    r_v0 = r_v0 + r_s2; /* 8008F7C0 addu $v0, $v0, $s2 */
    bl_sh(r_v0 + 0x3D2u, r_s0); /* 8008F7C4 sh $s0, 0x3D2($v0) */
    r_v1 = 0x800D0000u; /* 8008F7C8 lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 8008F7CC lw $v1, %lo(D_800CCB34)($v1) */
    r_s0 = r_s0 & 0xFFFFu; /* 8008F7D0 andi $s0, $s0, 0xFFFF */
    r_v0 = r_v1 << 3; /* 8008F7D4 sll $v0, $v1, 3 */
    r_v0 = r_v0 + r_v1; /* 8008F7D8 addu $v0, $v0, $v1 */
    r_v0 = r_v0 << 2; /* 8008F7DC sll $v0, $v0, 2 */
    r_v0 = r_v0 + r_s2; /* 8008F7E0 addu $v0, $v0, $s2 */
    bl_sh(r_v0 + 0x3D8u, r_s1); /* 8008F7E4 sh $s1, 0x3D8($v0) */
    r_v1 = 0x800D0000u; /* 8008F7E8 lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 8008F7EC lw $v1, %lo(D_800CCB34)($v1) */
    r_a2 = r_s0 + 0u; /* 8008F7F0 addu $a2, $s0, $zero */
    r_v0 = r_v1 << 3; /* 8008F7F4 sll $v0, $v1, 3 */
    r_v0 = r_v0 + r_v1; /* 8008F7F8 addu $v0, $v0, $v1 */
    r_v0 = r_v0 << 2; /* 8008F7FC sll $v0, $v0, 2 */
    r_v0 = r_v0 + r_s2; /* 8008F800 addu $v0, $v0, $s2 */
    bl_sh(r_v0 + 0x3DAu, r_t1); /* 8008F804 sh $t1, 0x3DA($v0) */
    r_v1 = 0x800D0000u; /* 8008F808 lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 8008F80C lw $v1, %lo(D_800CCB34)($v1) */
    r_s1 = r_s1 & 0xFFFFu; /* 8008F810 andi $s1, $s1, 0xFFFF */
    r_v0 = r_v1 << 3; /* 8008F814 sll $v0, $v1, 3 */
    r_v0 = r_v0 + r_v1; /* 8008F818 addu $v0, $v0, $v1 */
    r_v0 = r_v0 << 2; /* 8008F81C sll $v0, $v0, 2 */
    r_v0 = r_v0 + r_s2; /* 8008F820 addu $v0, $v0, $s2 */
    bl_sh(r_v0 + 0x3E0u, r_t0); /* 8008F824 sh $t0, 0x3E0($v0) */
    r_v1 = 0x800D0000u; /* 8008F828 lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 8008F82C lw $v1, %lo(D_800CCB34)($v1) */
    r_a1 = r_s1 + 0u; /* 8008F830 addu $a1, $s1, $zero */
    r_v0 = r_v1 << 3; /* 8008F834 sll $v0, $v1, 3 */
    r_v0 = r_v0 + r_v1; /* 8008F838 addu $v0, $v0, $v1 */
    r_v0 = r_v0 << 2; /* 8008F83C sll $v0, $v0, 2 */
    r_v0 = r_v0 + r_s2; /* 8008F840 addu $v0, $v0, $s2 */
    bl_sh(r_v0 + 0x3E2u, r_t1); /* 8008F844 sh $t1, 0x3E2($v0) */
    /* 8008F848 jal func_8008DC34 */
    bl_sw(r_sp + 0x10u, r_s4);
    bl_jal(FN, 0x8008DC34u, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* func_8008DC34 */
    r_a0 = r_s3 + 0u; /* 8008F850 addu $a0, $s3, $zero */
    r_a1 = r_s1 + 0u; /* 8008F854 addu $a1, $s1, $zero */
    r_a2 = r_s0 + 0u; /* 8008F858 addu $a2, $s0, $zero */
    /* 8008F85C jal func_8008DE04 */
    r_a3 = r_s5 + 0u;
    bl_jal(FN, 0x8008DE04u, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* func_8008DE04 */
    r_a0 = r_s3 + 0u; /* 8008F864 addu $a0, $s3, $zero */
    r_a1 = r_s1 + 0u; /* 8008F868 addu $a1, $s1, $zero */
    r_a2 = r_s0 + 0u; /* 8008F86C addu $a2, $s0, $zero */
    r_a3 = r_s5 + 0u; /* 8008F870 addu $a3, $s5, $zero */
    /* 8008F874 jal func_8008E430 */
    bl_sw(r_sp + 0x10u, r_s4);
    bl_jal(FN, 0x8008E430u, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* func_8008E430 */
    r_a0 = r_s3 + 0u; /* 8008F87C addu $a0, $s3, $zero */
    r_a1 = r_s1 + 0u; /* 8008F880 addu $a1, $s1, $zero */
    r_a2 = r_s0 + 0u; /* 8008F884 addu $a2, $s0, $zero */
    /* 8008F888 jal func_8008EA70 */
    r_a3 = r_s4 + 0u;
    bl_jal(FN, 0x8008EA70u, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* func_8008EA70 */
    r_a0 = r_s3 + 0u; /* 8008F890 addu $a0, $s3, $zero */
    r_a1 = r_s1 + 0u; /* 8008F894 addu $a1, $s1, $zero */
    r_a2 = r_s0 + 0u; /* 8008F898 addu $a2, $s0, $zero */
    r_a3 = r_s5 + 0u; /* 8008F89C addu $a3, $s5, $zero */
    /* 8008F8A0 jal func_8008F0A8 */
    bl_sw(r_sp + 0x10u, r_s4);
    bl_jal(FN, 0x8008F0A8u, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* func_8008F0A8 */
    r_v0 = 0x800D0000u; /* 8008F8A8 lui $v0, %hi(D_800CCB34) */
    r_v0 = bl_lbu(r_v0 + 0xFFFFCB34u); /* 8008F8AC lbu $v0, %lo(D_800CCB34)($v0) */
    ; /* 8008F8B0 nop  */
    bl_sb(r_s2 + 0x5A4u, r_v0); /* 8008F8B4 sb $v0, 0x5A4($s2) */
    r_v0 = 0x800D0000u; /* 8008F8B8 lui $v0, %hi(D_800D2D28) */
    r_v0 = bl_lw(r_v0 + 0x2D28u); /* 8008F8BC lw $v0, %lo(D_800D2D28)($v0) */
    r_v1 = 0u | 0x1u; /* 8008F8C0 ori $v1, $zero, 0x1 */
    r_v0 = r_v0 + r_s3; /* 8008F8C4 addu $v0, $v0, $s3 */
    bl_sb(r_v0 + 0xB0u, r_v1); /* 8008F8C8 sb $v1, 0xB0($v0) */
    r_ra = bl_lw(r_sp + 0x30u); /* 8008F8CC lw $ra, 0x30($sp) */
    r_s5 = bl_lw(r_sp + 0x2Cu); /* 8008F8D0 lw $s5, 0x2C($sp) */
    r_s4 = bl_lw(r_sp + 0x28u); /* 8008F8D4 lw $s4, 0x28($sp) */
    r_s3 = bl_lw(r_sp + 0x24u); /* 8008F8D8 lw $s3, 0x24($sp) */
    r_s2 = bl_lw(r_sp + 0x20u); /* 8008F8DC lw $s2, 0x20($sp) */
    r_s1 = bl_lw(r_sp + 0x1Cu); /* 8008F8E0 lw $s1, 0x1C($sp) */
    r_s0 = bl_lw(r_sp + 0x18u); /* 8008F8E4 lw $s0, 0x18($sp) */
    r_sp = r_sp + 0x38u; /* 8008F8E8 addiu $sp, $sp, 0x38 */
    /* 8008F8EC jr $ra */
    ; goto bl_exit;
bl_exit:
    bl_leave(bl_saved);
    (void)hi; (void)lo; (void)r_at; (void)r_v1; (void)r_k0; (void)r_k1; (void)r_gp; (void)r_ra;
    (void)r_v0;
}

/* func_8008DC34.s (116 instructions; --args 5 --void), frontier step2.  Callees through the jal service: func_80076A10, func_80076B00. */
void func_8008DC34(u32 arg0, u32 arg1, u32 arg2, u32 arg3, u32 arg4) {
    static const char FN[] = "func_8008DC34";
    u32 r_at = 0, r_v0 = 0, r_v1 = 0, r_a0 = 0, r_a1 = 0, r_a2 = 0, r_a3 = 0, r_t0 = 0, r_t1 = 0, r_t2 = 0, r_t3 = 0, r_t4 = 0, r_t5 = 0, r_t6 = 0, r_t7 = 0, r_s0 = 0, r_s1 = 0, r_s2 = 0, r_s3 = 0, r_s4 = 0, r_s5 = 0, r_s6 = 0, r_s7 = 0, r_t8 = 0, r_t9 = 0, r_k0 = 0, r_k1 = 0, r_gp = 0, r_sp = 0, r_fp = 0, r_ra = 0;
    u32 hi = 0, lo = 0, bl_saved;
    r_sp = bl_enter(FN, 0x38u, &bl_saved);
    r_a0 = arg0;
    r_a1 = arg1;
    r_a2 = arg2;
    r_a3 = arg3;
    bl_sw(r_sp + 0x10u, arg4);
    r_sp = r_sp + 0xFFFFFFC8u; /* 8008DC34 addiu $sp, $sp, -0x38 */
    bl_sw(r_sp + 0x1Cu, r_s1); /* 8008DC38 sw $s1, 0x1C($sp) */
    r_s1 = r_a1 + 0u; /* 8008DC3C addu $s1, $a1, $zero */
    bl_sw(r_sp + 0x28u, r_s4); /* 8008DC40 sw $s4, 0x28($sp) */
    r_s4 = r_a2 + 0u; /* 8008DC44 addu $s4, $a2, $zero */
    bl_sw(r_sp + 0x2Cu, r_s5); /* 8008DC48 sw $s5, 0x2C($sp) */
    r_a0 = r_a0 & 0xFFu; /* 8008DC4C andi $a0, $a0, 0xFF */
    bl_sw(r_sp + 0x30u, r_s6); /* 8008DC50 sw $s6, 0x30($sp) */
    r_s6 = bl_lhu(r_sp + 0x48u); /* 8008DC54 lhu $s6, 0x48($sp) */
    r_v0 = 0x800C0000u; /* 8008DC58 lui $v0, %hi(D_800C3E4C) */
    r_v0 = bl_lbu(r_v0 + 0x3E4Cu); /* 8008DC5C lbu $v0, %lo(D_800C3E4C)($v0) */
    r_a0 = r_a0 << 2; /* 8008DC60 sll $a0, $a0, 2 */
    bl_sw(r_sp + 0x34u, r_ra); /* 8008DC64 sw $ra, 0x34($sp) */
    bl_sw(r_sp + 0x24u, r_s3); /* 8008DC68 sw $s3, 0x24($sp) */
    bl_sw(r_sp + 0x20u, r_s2); /* 8008DC6C sw $s2, 0x20($sp) */
    bl_sw(r_sp + 0x18u, r_s0); /* 8008DC70 sw $s0, 0x18($sp) */
    r_at = 0x800D0000u; /* 8008DC74 lui $at, %hi(D_800D2E38) */
    r_at = r_at + r_a0; /* 8008DC78 addu $at, $at, $a0 */
    r_s3 = bl_lw(r_at + 0x2E38u); /* 8008DC7C lw $s3, %lo(D_800D2E38)($at) */
    /* 8008DC80 beqz $v0, .L8008DCA8 */
    { int c_ = (r_v0 == 0); r_s5 = r_a3 + 0u; if (c_) goto L_8008DCA8; }
    r_v0 = 0u | 0x4Au; /* 8008DC88 ori $v0, $zero, 0x4A */
    bl_sb(r_sp + 0x10u, r_v0); /* 8008DC8C sb $v0, 0x10($sp) */
    r_v0 = 0u | 0x4Cu; /* 8008DC90 ori $v0, $zero, 0x4C */
    bl_sb(r_sp + 0x11u, r_v0); /* 8008DC94 sb $v0, 0x11($sp) */
    r_v0 = 0u | 0x4Fu; /* 8008DC98 ori $v0, $zero, 0x4F */
    bl_sb(r_sp + 0x12u, r_v0); /* 8008DC9C sb $v0, 0x12($sp) */
    /* 8008DCA0 j .L8008DCC4 */
    r_v0 = 0u | 0x51u; goto L_8008DCC4;
L_8008DCA8:
    r_v0 = 0u | 0xF0u; /* 8008DCA8 ori $v0, $zero, 0xF0 */
    bl_sb(r_sp + 0x10u, r_v0); /* 8008DCAC sb $v0, 0x10($sp) */
    r_v0 = 0u | 0xF2u; /* 8008DCB0 ori $v0, $zero, 0xF2 */
    bl_sb(r_sp + 0x11u, r_v0); /* 8008DCB4 sb $v0, 0x11($sp) */
    r_v0 = 0u | 0xF5u; /* 8008DCB8 ori $v0, $zero, 0xF5 */
    bl_sb(r_sp + 0x12u, r_v0); /* 8008DCBC sb $v0, 0x12($sp) */
    r_v0 = 0u | 0xF7u; /* 8008DCC0 ori $v0, $zero, 0xF7 */
L_8008DCC4:
    bl_sb(r_sp + 0x13u, r_v0); /* 8008DCC4 sb $v0, 0x13($sp) */
    r_a1 = r_s3 + 0u; /* 8008DCC8 addu $a1, $s3, $zero */
    r_s2 = r_s1 << 16; /* 8008DCCC sll $s2, $s1, 16 */
    r_s2 = BL_SRA(r_s2, 16); /* 8008DCD0 sra $s2, $s2, 16 */
    r_a2 = r_s2 + 0u; /* 8008DCD4 addu $a2, $s2, $zero */
    r_s0 = r_s4 << 16; /* 8008DCD8 sll $s0, $s4, 16 */
    r_s0 = BL_SRA(r_s0, 16); /* 8008DCDC sra $s0, $s0, 16 */
    bl_sw(r_s3 + 0x5A0u, 0u); /* 8008DCE0 sw $zero, 0x5A0($s3) */
    r_a0 = bl_lbu(r_sp + 0x10u); /* 8008DCE4 lbu $a0, 0x10($sp) */
    /* 8008DCE8 jal func_80076A10 */
    r_a3 = r_s0 + 0u;
    bl_jal(FN, 0x80076A10u, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* func_80076A10 */
    r_s1 = r_s1 + r_s5; /* 8008DCF0 addu $s1, $s1, $s5 */
    r_s1 = r_s1 + 0xFFFFFFF8u; /* 8008DCF4 addiu $s1, $s1, -0x8 */
    r_s1 = r_s1 << 16; /* 8008DCF8 sll $s1, $s1, 16 */
    r_s1 = BL_SRA(r_s1, 16); /* 8008DCFC sra $s1, $s1, 16 */
    r_a2 = r_s1 + 0u; /* 8008DD00 addu $a2, $s1, $zero */
    r_v1 = bl_lw(r_s3 + 0x5A0u); /* 8008DD04 lw $v1, 0x5A0($s3) */
    r_a3 = r_s0 + 0u; /* 8008DD08 addu $a3, $s0, $zero */
    r_v0 = r_v0 + r_v1; /* 8008DD0C addu $v0, $v0, $v1 */
    r_a1 = r_v0 << 2; /* 8008DD10 sll $a1, $v0, 2 */
    r_a1 = r_a1 + r_v0; /* 8008DD14 addu $a1, $a1, $v0 */
    r_a1 = r_a1 << 4; /* 8008DD18 sll $a1, $a1, 4 */
    bl_sw(r_s3 + 0x5A0u, r_v0); /* 8008DD1C sw $v0, 0x5A0($s3) */
    r_a0 = bl_lbu(r_sp + 0x11u); /* 8008DD20 lbu $a0, 0x11($sp) */
    /* 8008DD24 jal func_80076A10 */
    r_a1 = r_s3 + r_a1;
    bl_jal(FN, 0x80076A10u, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* func_80076A10 */
    r_a2 = r_s2 + 0u; /* 8008DD2C addu $a2, $s2, $zero */
    r_s0 = r_s4 + r_s6; /* 8008DD30 addu $s0, $s4, $s6 */
    r_s0 = r_s0 + 0xFFFFFFF8u; /* 8008DD34 addiu $s0, $s0, -0x8 */
    r_s0 = r_s0 << 16; /* 8008DD38 sll $s0, $s0, 16 */
    r_s0 = BL_SRA(r_s0, 16); /* 8008DD3C sra $s0, $s0, 16 */
    r_v1 = bl_lw(r_s3 + 0x5A0u); /* 8008DD40 lw $v1, 0x5A0($s3) */
    r_a3 = r_s0 + 0u; /* 8008DD44 addu $a3, $s0, $zero */
    r_v0 = r_v0 + r_v1; /* 8008DD48 addu $v0, $v0, $v1 */
    r_a1 = r_v0 << 2; /* 8008DD4C sll $a1, $v0, 2 */
    r_a1 = r_a1 + r_v0; /* 8008DD50 addu $a1, $a1, $v0 */
    r_a1 = r_a1 << 4; /* 8008DD54 sll $a1, $a1, 4 */
    bl_sw(r_s3 + 0x5A0u, r_v0); /* 8008DD58 sw $v0, 0x5A0($s3) */
    r_a0 = bl_lbu(r_sp + 0x12u); /* 8008DD5C lbu $a0, 0x12($sp) */
    /* 8008DD60 jal func_80076A10 */
    r_a1 = r_s3 + r_a1;
    bl_jal(FN, 0x80076A10u, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* func_80076A10 */
    r_a2 = r_s1 + 0u; /* 8008DD68 addu $a2, $s1, $zero */
    r_v1 = bl_lw(r_s3 + 0x5A0u); /* 8008DD6C lw $v1, 0x5A0($s3) */
    r_a3 = r_s0 + 0u; /* 8008DD70 addu $a3, $s0, $zero */
    r_v0 = r_v0 + r_v1; /* 8008DD74 addu $v0, $v0, $v1 */
    r_a1 = r_v0 << 2; /* 8008DD78 sll $a1, $v0, 2 */
    r_a1 = r_a1 + r_v0; /* 8008DD7C addu $a1, $a1, $v0 */
    r_a1 = r_a1 << 4; /* 8008DD80 sll $a1, $a1, 4 */
    bl_sw(r_s3 + 0x5A0u, r_v0); /* 8008DD84 sw $v0, 0x5A0($s3) */
    r_a0 = bl_lbu(r_sp + 0x13u); /* 8008DD88 lbu $a0, 0x13($sp) */
    /* 8008DD8C jal func_80076A10 */
    r_a1 = r_s3 + r_a1;
    bl_jal(FN, 0x80076A10u, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* func_80076A10 */
    r_v1 = bl_lw(r_s3 + 0x5A0u); /* 8008DD94 lw $v1, 0x5A0($s3) */
    r_s0 = 0u + 0u; /* 8008DD98 addu $s0, $zero, $zero */
    r_v0 = r_v0 + r_v1; /* 8008DD9C addu $v0, $v0, $v1 */
    bl_sw(r_s3 + 0x5A0u, r_v0); /* 8008DDA0 sw $v0, 0x5A0($s3) */
    r_v0 = r_s0 << 1; /* 8008DDA4 sll $v0, $s0, 1 */
L_8008DDA8:
    r_v1 = 0x800D0000u; /* 8008DDA8 lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 8008DDAC lw $v1, %lo(D_800CCB34)($v1) */
    r_s0 = r_s0 + 0x1u; /* 8008DDB0 addiu $s0, $s0, 0x1 */
    r_v0 = r_v0 + r_v1; /* 8008DDB4 addu $v0, $v0, $v1 */
    r_a0 = r_v0 << 2; /* 8008DDB8 sll $a0, $v0, 2 */
    r_a0 = r_a0 + r_v0; /* 8008DDBC addu $a0, $a0, $v0 */
    r_a0 = r_a0 << 3; /* 8008DDC0 sll $a0, $a0, 3 */
    /* 8008DDC4 jal func_80076B00 */
    r_a0 = r_s3 + r_a0;
    bl_jal(FN, 0x80076B00u, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* func_80076B00 */
    r_v0 = (u32)((s32)r_s0 < 4); /* 8008DDCC slti $v0, $s0, 0x4 */
    /* 8008DDD0 bnez $v0, .L8008DDA8 */
    { int c_ = (r_v0 != 0); r_v0 = r_s0 << 1; if (c_) goto L_8008DDA8; }
    r_ra = bl_lw(r_sp + 0x34u); /* 8008DDD8 lw $ra, 0x34($sp) */
    r_s6 = bl_lw(r_sp + 0x30u); /* 8008DDDC lw $s6, 0x30($sp) */
    r_s5 = bl_lw(r_sp + 0x2Cu); /* 8008DDE0 lw $s5, 0x2C($sp) */
    r_s4 = bl_lw(r_sp + 0x28u); /* 8008DDE4 lw $s4, 0x28($sp) */
    r_s3 = bl_lw(r_sp + 0x24u); /* 8008DDE8 lw $s3, 0x24($sp) */
    r_s2 = bl_lw(r_sp + 0x20u); /* 8008DDEC lw $s2, 0x20($sp) */
    r_s1 = bl_lw(r_sp + 0x1Cu); /* 8008DDF0 lw $s1, 0x1C($sp) */
    r_s0 = bl_lw(r_sp + 0x18u); /* 8008DDF4 lw $s0, 0x18($sp) */
    r_sp = r_sp + 0x38u; /* 8008DDF8 addiu $sp, $sp, 0x38 */
    /* 8008DDFC jr $ra */
    ; goto bl_exit;
bl_exit:
    bl_leave(bl_saved);
    (void)hi; (void)lo; (void)r_at; (void)r_v1; (void)r_k0; (void)r_k1; (void)r_gp; (void)r_ra;
    (void)r_v0;
}

/* func_8008DE04.s (395 instructions; --args 4 --void), frontier step2.  Callees through the jal service: func_80076B00. */
void func_8008DE04(u32 arg0, u32 arg1, u32 arg2, u32 arg3) {
    static const char FN[] = "func_8008DE04";
    u32 r_at = 0, r_v0 = 0, r_v1 = 0, r_a0 = 0, r_a1 = 0, r_a2 = 0, r_a3 = 0, r_t0 = 0, r_t1 = 0, r_t2 = 0, r_t3 = 0, r_t4 = 0, r_t5 = 0, r_t6 = 0, r_t7 = 0, r_s0 = 0, r_s1 = 0, r_s2 = 0, r_s3 = 0, r_s4 = 0, r_s5 = 0, r_s6 = 0, r_s7 = 0, r_t8 = 0, r_t9 = 0, r_k0 = 0, r_k1 = 0, r_gp = 0, r_sp = 0, r_fp = 0, r_ra = 0;
    u32 hi = 0, lo = 0, bl_saved;
    r_sp = bl_enter(FN, 0x20u, &bl_saved);
    r_a0 = arg0;
    r_a1 = arg1;
    r_a2 = arg2;
    r_a3 = arg3;
    r_sp = r_sp + 0xFFFFFFE0u; /* 8008DE04 addiu $sp, $sp, -0x20 */
    r_a0 = r_a0 & 0xFFu; /* 8008DE08 andi $a0, $a0, 0xFF */
    r_a0 = r_a0 << 2; /* 8008DE0C sll $a0, $a0, 2 */
    r_a1 = r_a1 + 0x8u; /* 8008DE10 addiu $a1, $a1, 0x8 */
    bl_sw(r_sp + 0x18u, r_ra); /* 8008DE14 sw $ra, 0x18($sp) */
    bl_sw(r_sp + 0x14u, r_s1); /* 8008DE18 sw $s1, 0x14($sp) */
    bl_sw(r_sp + 0x10u, r_s0); /* 8008DE1C sw $s0, 0x10($sp) */
    r_at = 0x800D0000u; /* 8008DE20 lui $at, %hi(D_800D2E38) */
    r_at = r_at + r_a0; /* 8008DE24 addu $at, $at, $a0 */
    r_s1 = bl_lw(r_at + 0x2E38u); /* 8008DE28 lw $s1, %lo(D_800D2E38)($at) */
    r_a0 = 0u | 0xFFF8u; /* 8008DE2C ori $a0, $zero, 0xFFF8 */
    r_v1 = 0x800D0000u; /* 8008DE30 lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 8008DE34 lw $v1, %lo(D_800CCB34)($v1) */
    r_a0 = r_a2 + r_a0; /* 8008DE38 addu $a0, $a2, $a0 */
    r_v0 = r_v1 << 2; /* 8008DE3C sll $v0, $v1, 2 */
    r_v0 = r_v0 + r_v1; /* 8008DE40 addu $v0, $v0, $v1 */
    r_v0 = r_v0 << 3; /* 8008DE44 sll $v0, $v0, 3 */
    r_v0 = r_v0 + r_s1; /* 8008DE48 addu $v0, $v0, $s1 */
    bl_sh(r_v0 + 0x148u, r_a1); /* 8008DE4C sh $a1, 0x148($v0) */
    r_v1 = 0x800D0000u; /* 8008DE50 lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 8008DE54 lw $v1, %lo(D_800CCB34)($v1) */
    r_a3 = r_a3 & 0xFFFFu; /* 8008DE58 andi $a3, $a3, 0xFFFF */
    r_v0 = r_v1 << 2; /* 8008DE5C sll $v0, $v1, 2 */
    r_v0 = r_v0 + r_v1; /* 8008DE60 addu $v0, $v0, $v1 */
    r_v0 = r_v0 << 3; /* 8008DE64 sll $v0, $v0, 3 */
    r_v0 = r_v0 + r_s1; /* 8008DE68 addu $v0, $v0, $s1 */
    bl_sh(r_v0 + 0x14Au, r_a0); /* 8008DE6C sh $a0, 0x14A($v0) */
    r_v1 = 0x800D0000u; /* 8008DE70 lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 8008DE74 lw $v1, %lo(D_800CCB34)($v1) */
    r_a3 = r_a3 + 0xFFFFFFF0u; /* 8008DE78 addiu $a3, $a3, -0x10 */
    r_v0 = r_v1 << 2; /* 8008DE7C sll $v0, $v1, 2 */
    r_v0 = r_v0 + r_v1; /* 8008DE80 addu $v0, $v0, $v1 */
    r_v0 = r_v0 << 3; /* 8008DE84 sll $v0, $v0, 3 */
    r_v0 = r_v0 + r_s1; /* 8008DE88 addu $v0, $v0, $s1 */
    r_v1 = r_a3 >> 31; /* 8008DE8C srl $v1, $a3, 31 */
    r_a3 = r_a3 + r_v1; /* 8008DE90 addu $a3, $a3, $v1 */
    r_a3 = BL_SRA(r_a3, 1); /* 8008DE94 sra $a3, $a3, 1 */
    r_t0 = r_a1 + r_a3; /* 8008DE98 addu $t0, $a1, $a3 */
    bl_sh(r_v0 + 0x150u, r_t0); /* 8008DE9C sh $t0, 0x150($v0) */
    r_v1 = 0x800D0000u; /* 8008DEA0 lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 8008DEA4 lw $v1, %lo(D_800CCB34)($v1) */
    ; /* 8008DEA8 nop  */
    r_v0 = r_v1 << 2; /* 8008DEAC sll $v0, $v1, 2 */
    r_v0 = r_v0 + r_v1; /* 8008DEB0 addu $v0, $v0, $v1 */
    r_v0 = r_v0 << 3; /* 8008DEB4 sll $v0, $v0, 3 */
    r_v0 = r_v0 + r_s1; /* 8008DEB8 addu $v0, $v0, $s1 */
    bl_sh(r_v0 + 0x152u, r_a0); /* 8008DEBC sh $a0, 0x152($v0) */
    r_v1 = 0x800D0000u; /* 8008DEC0 lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 8008DEC4 lw $v1, %lo(D_800CCB34)($v1) */
    ; /* 8008DEC8 nop  */
    r_v0 = r_v1 << 2; /* 8008DECC sll $v0, $v1, 2 */
    r_v0 = r_v0 + r_v1; /* 8008DED0 addu $v0, $v0, $v1 */
    r_v0 = r_v0 << 3; /* 8008DED4 sll $v0, $v0, 3 */
    r_v0 = r_v0 + r_s1; /* 8008DED8 addu $v0, $v0, $s1 */
    bl_sh(r_v0 + 0x158u, r_a1); /* 8008DEDC sh $a1, 0x158($v0) */
    r_v1 = 0x800D0000u; /* 8008DEE0 lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 8008DEE4 lw $v1, %lo(D_800CCB34)($v1) */
    r_a2 = r_a2 + 0x8u; /* 8008DEE8 addiu $a2, $a2, 0x8 */
    r_v0 = r_v1 << 2; /* 8008DEEC sll $v0, $v1, 2 */
    r_v0 = r_v0 + r_v1; /* 8008DEF0 addu $v0, $v0, $v1 */
    r_v0 = r_v0 << 3; /* 8008DEF4 sll $v0, $v0, 3 */
    r_v0 = r_v0 + r_s1; /* 8008DEF8 addu $v0, $v0, $s1 */
    bl_sh(r_v0 + 0x15Au, r_a2); /* 8008DEFC sh $a2, 0x15A($v0) */
    r_v1 = 0x800D0000u; /* 8008DF00 lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 8008DF04 lw $v1, %lo(D_800CCB34)($v1) */
    ; /* 8008DF08 nop  */
    r_v0 = r_v1 << 2; /* 8008DF0C sll $v0, $v1, 2 */
    r_v0 = r_v0 + r_v1; /* 8008DF10 addu $v0, $v0, $v1 */
    r_v0 = r_v0 << 3; /* 8008DF14 sll $v0, $v0, 3 */
    r_v0 = r_v0 + r_s1; /* 8008DF18 addu $v0, $v0, $s1 */
    bl_sh(r_v0 + 0x160u, r_t0); /* 8008DF1C sh $t0, 0x160($v0) */
    r_v1 = 0x800D0000u; /* 8008DF20 lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 8008DF24 lw $v1, %lo(D_800CCB34)($v1) */
    ; /* 8008DF28 nop  */
    r_v0 = r_v1 << 2; /* 8008DF2C sll $v0, $v1, 2 */
    r_v0 = r_v0 + r_v1; /* 8008DF30 addu $v0, $v0, $v1 */
    r_v0 = r_v0 << 3; /* 8008DF34 sll $v0, $v0, 3 */
    r_v0 = r_v0 + r_s1; /* 8008DF38 addu $v0, $v0, $s1 */
    bl_sh(r_v0 + 0x162u, r_a2); /* 8008DF3C sh $a2, 0x162($v0) */
    r_v1 = 0x800D0000u; /* 8008DF40 lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 8008DF44 lw $v1, %lo(D_800CCB34)($v1) */
    ; /* 8008DF48 nop  */
    r_v0 = r_v1 << 2; /* 8008DF4C sll $v0, $v1, 2 */
    r_v0 = r_v0 + r_v1; /* 8008DF50 addu $v0, $v0, $v1 */
    r_v0 = r_v0 << 3; /* 8008DF54 sll $v0, $v0, 3 */
    r_v0 = r_v0 + r_s1; /* 8008DF58 addu $v0, $v0, $s1 */
    bl_sh(r_v0 + 0x198u, r_t0); /* 8008DF5C sh $t0, 0x198($v0) */
    r_v1 = 0x800D0000u; /* 8008DF60 lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 8008DF64 lw $v1, %lo(D_800CCB34)($v1) */
    ; /* 8008DF68 nop  */
    r_v0 = r_v1 << 2; /* 8008DF6C sll $v0, $v1, 2 */
    r_v0 = r_v0 + r_v1; /* 8008DF70 addu $v0, $v0, $v1 */
    r_v0 = r_v0 << 3; /* 8008DF74 sll $v0, $v0, 3 */
    r_v0 = r_v0 + r_s1; /* 8008DF78 addu $v0, $v0, $s1 */
    bl_sh(r_v0 + 0x19Au, r_a0); /* 8008DF7C sh $a0, 0x19A($v0) */
    r_v1 = 0x800D0000u; /* 8008DF80 lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 8008DF84 lw $v1, %lo(D_800CCB34)($v1) */
    r_a3 = r_t0 + r_a3; /* 8008DF88 addu $a3, $t0, $a3 */
    r_v0 = r_v1 << 2; /* 8008DF8C sll $v0, $v1, 2 */
    r_v0 = r_v0 + r_v1; /* 8008DF90 addu $v0, $v0, $v1 */
    r_v0 = r_v0 << 3; /* 8008DF94 sll $v0, $v0, 3 */
    r_v0 = r_v0 + r_s1; /* 8008DF98 addu $v0, $v0, $s1 */
    bl_sh(r_v0 + 0x1A0u, r_a3); /* 8008DF9C sh $a3, 0x1A0($v0) */
    r_v1 = 0x800D0000u; /* 8008DFA0 lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 8008DFA4 lw $v1, %lo(D_800CCB34)($v1) */
    ; /* 8008DFA8 nop  */
    r_v0 = r_v1 << 2; /* 8008DFAC sll $v0, $v1, 2 */
    r_v0 = r_v0 + r_v1; /* 8008DFB0 addu $v0, $v0, $v1 */
    r_v0 = r_v0 << 3; /* 8008DFB4 sll $v0, $v0, 3 */
    r_v0 = r_v0 + r_s1; /* 8008DFB8 addu $v0, $v0, $s1 */
    bl_sh(r_v0 + 0x1A2u, r_a0); /* 8008DFBC sh $a0, 0x1A2($v0) */
    r_v1 = 0x800D0000u; /* 8008DFC0 lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 8008DFC4 lw $v1, %lo(D_800CCB34)($v1) */
    ; /* 8008DFC8 nop  */
    r_v0 = r_v1 << 2; /* 8008DFCC sll $v0, $v1, 2 */
    r_v0 = r_v0 + r_v1; /* 8008DFD0 addu $v0, $v0, $v1 */
    r_v0 = r_v0 << 3; /* 8008DFD4 sll $v0, $v0, 3 */
    r_v0 = r_v0 + r_s1; /* 8008DFD8 addu $v0, $v0, $s1 */
    bl_sh(r_v0 + 0x1A8u, r_t0); /* 8008DFDC sh $t0, 0x1A8($v0) */
    r_v1 = 0x800D0000u; /* 8008DFE0 lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 8008DFE4 lw $v1, %lo(D_800CCB34)($v1) */
    ; /* 8008DFE8 nop  */
    r_v0 = r_v1 << 2; /* 8008DFEC sll $v0, $v1, 2 */
    r_v0 = r_v0 + r_v1; /* 8008DFF0 addu $v0, $v0, $v1 */
    r_v0 = r_v0 << 3; /* 8008DFF4 sll $v0, $v0, 3 */
    r_v0 = r_v0 + r_s1; /* 8008DFF8 addu $v0, $v0, $s1 */
    bl_sh(r_v0 + 0x1AAu, r_a2); /* 8008DFFC sh $a2, 0x1AA($v0) */
    r_v1 = 0x800D0000u; /* 8008E000 lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 8008E004 lw $v1, %lo(D_800CCB34)($v1) */
    ; /* 8008E008 nop  */
    r_v0 = r_v1 << 2; /* 8008E00C sll $v0, $v1, 2 */
    r_v0 = r_v0 + r_v1; /* 8008E010 addu $v0, $v0, $v1 */
    r_v0 = r_v0 << 3; /* 8008E014 sll $v0, $v0, 3 */
    r_v0 = r_v0 + r_s1; /* 8008E018 addu $v0, $v0, $s1 */
    bl_sh(r_v0 + 0x1B0u, r_a3); /* 8008E01C sh $a3, 0x1B0($v0) */
    r_v1 = 0x800D0000u; /* 8008E020 lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 8008E024 lw $v1, %lo(D_800CCB34)($v1) */
    ; /* 8008E028 nop  */
    r_v0 = r_v1 << 2; /* 8008E02C sll $v0, $v1, 2 */
    r_v0 = r_v0 + r_v1; /* 8008E030 addu $v0, $v0, $v1 */
    r_v0 = r_v0 << 3; /* 8008E034 sll $v0, $v0, 3 */
    r_v0 = r_v0 + r_s1; /* 8008E038 addu $v0, $v0, $s1 */
    bl_sh(r_v0 + 0x1B2u, r_a2); /* 8008E03C sh $a2, 0x1B2($v0) */
    r_v1 = 0x800D0000u; /* 8008E040 lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 8008E044 lw $v1, %lo(D_800CCB34)($v1) */
    ; /* 8008E048 nop  */
    r_v0 = r_v1 << 2; /* 8008E04C sll $v0, $v1, 2 */
    r_v0 = r_v0 + r_v1; /* 8008E050 addu $v0, $v0, $v1 */
    r_v1 = 0x800C0000u; /* 8008E054 lui $v1, %hi(D_800C3EA4) */
    r_v1 = bl_lw(r_v1 + 0x3EA4u); /* 8008E058 lw $v1, %lo(D_800C3EA4)($v1) */
    r_v0 = r_v0 << 3; /* 8008E05C sll $v0, $v0, 3 */
    r_at = 0x10000u; /* 8008E060 lui $at, (0x10000 >> 16) */
    r_at = r_v1 + r_at; /* 8008E064 addu $at, $v1, $at */
    r_v1 = bl_lbu(r_at + 0xFFFFA25Cu); /* 8008E068 lbu $v1, -0x5DA4($at) */
    r_v0 = r_v0 + r_s1; /* 8008E06C addu $v0, $v0, $s1 */
    r_v1 = r_v1 & 0x3Fu; /* 8008E070 andi $v1, $v1, 0x3F */
    r_v1 = r_v1 << 1; /* 8008E074 sll $v1, $v1, 1 */
    bl_sb(r_v0 + 0x14Cu, r_v1); /* 8008E078 sb $v1, 0x14C($v0) */
    r_v1 = 0x800D0000u; /* 8008E07C lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 8008E080 lw $v1, %lo(D_800CCB34)($v1) */
    ; /* 8008E084 nop  */
    r_v0 = r_v1 << 2; /* 8008E088 sll $v0, $v1, 2 */
    r_v0 = r_v0 + r_v1; /* 8008E08C addu $v0, $v0, $v1 */
    r_v1 = 0x800C0000u; /* 8008E090 lui $v1, %hi(D_800C3EA4) */
    r_v1 = bl_lw(r_v1 + 0x3EA4u); /* 8008E094 lw $v1, %lo(D_800C3EA4)($v1) */
    r_v0 = r_v0 << 3; /* 8008E098 sll $v0, $v0, 3 */
    r_at = 0x10000u; /* 8008E09C lui $at, (0x10000 >> 16) */
    r_at = r_v1 + r_at; /* 8008E0A0 addu $at, $v1, $at */
    r_v1 = bl_lbu(r_at + 0xFFFFA260u); /* 8008E0A4 lbu $v1, -0x5DA0($at) */
    r_v0 = r_v0 + r_s1; /* 8008E0A8 addu $v0, $v0, $s1 */
    bl_sb(r_v0 + 0x14Du, r_v1); /* 8008E0AC sb $v1, 0x14D($v0) */
    r_v0 = 0x800D0000u; /* 8008E0B0 lui $v0, %hi(D_800CCB34) */
    r_v0 = bl_lw(r_v0 + 0xFFFFCB34u); /* 8008E0B4 lw $v0, %lo(D_800CCB34)($v0) */
    ; /* 8008E0B8 nop  */
    r_v1 = r_v0 << 2; /* 8008E0BC sll $v1, $v0, 2 */
    r_v1 = r_v1 + r_v0; /* 8008E0C0 addu $v1, $v1, $v0 */
    r_v0 = 0x800C0000u; /* 8008E0C4 lui $v0, %hi(D_800C3EA4) */
    r_v0 = bl_lw(r_v0 + 0x3EA4u); /* 8008E0C8 lw $v0, %lo(D_800C3EA4)($v0) */
    r_v1 = r_v1 << 3; /* 8008E0CC sll $v1, $v1, 3 */
    r_at = 0x10000u; /* 8008E0D0 lui $at, (0x10000 >> 16) */
    r_at = r_v0 + r_at; /* 8008E0D4 addu $at, $v0, $at */
    r_v0 = bl_lbu(r_at + 0xFFFFA25Cu); /* 8008E0D8 lbu $v0, -0x5DA4($at) */
    r_v1 = r_v1 + r_s1; /* 8008E0DC addu $v1, $v1, $s1 */
    r_v0 = r_v0 & 0x3Fu; /* 8008E0E0 andi $v0, $v0, 0x3F */
    r_v0 = r_v0 << 1; /* 8008E0E4 sll $v0, $v0, 1 */
    r_v0 = r_v0 + 0x7u; /* 8008E0E8 addiu $v0, $v0, 0x7 */
    bl_sb(r_v1 + 0x154u, r_v0); /* 8008E0EC sb $v0, 0x154($v1) */
    r_v1 = 0x800D0000u; /* 8008E0F0 lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 8008E0F4 lw $v1, %lo(D_800CCB34)($v1) */
    ; /* 8008E0F8 nop  */
    r_v0 = r_v1 << 2; /* 8008E0FC sll $v0, $v1, 2 */
    r_v0 = r_v0 + r_v1; /* 8008E100 addu $v0, $v0, $v1 */
    r_v1 = 0x800C0000u; /* 8008E104 lui $v1, %hi(D_800C3EA4) */
    r_v1 = bl_lw(r_v1 + 0x3EA4u); /* 8008E108 lw $v1, %lo(D_800C3EA4)($v1) */
    r_v0 = r_v0 << 3; /* 8008E10C sll $v0, $v0, 3 */
    r_at = 0x10000u; /* 8008E110 lui $at, (0x10000 >> 16) */
    r_at = r_v1 + r_at; /* 8008E114 addu $at, $v1, $at */
    r_v1 = bl_lbu(r_at + 0xFFFFA260u); /* 8008E118 lbu $v1, -0x5DA0($at) */
    r_v0 = r_v0 + r_s1; /* 8008E11C addu $v0, $v0, $s1 */
    bl_sb(r_v0 + 0x155u, r_v1); /* 8008E120 sb $v1, 0x155($v0) */
    r_v1 = 0x800D0000u; /* 8008E124 lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 8008E128 lw $v1, %lo(D_800CCB34)($v1) */
    ; /* 8008E12C nop  */
    r_v0 = r_v1 << 2; /* 8008E130 sll $v0, $v1, 2 */
    r_v0 = r_v0 + r_v1; /* 8008E134 addu $v0, $v0, $v1 */
    r_v1 = 0x800C0000u; /* 8008E138 lui $v1, %hi(D_800C3EA4) */
    r_v1 = bl_lw(r_v1 + 0x3EA4u); /* 8008E13C lw $v1, %lo(D_800C3EA4)($v1) */
    r_v0 = r_v0 << 3; /* 8008E140 sll $v0, $v0, 3 */
    r_at = 0x10000u; /* 8008E144 lui $at, (0x10000 >> 16) */
    r_at = r_v1 + r_at; /* 8008E148 addu $at, $v1, $at */
    r_v1 = bl_lbu(r_at + 0xFFFFA25Cu); /* 8008E14C lbu $v1, -0x5DA4($at) */
    r_v0 = r_v0 + r_s1; /* 8008E150 addu $v0, $v0, $s1 */
    r_v1 = r_v1 & 0x3Fu; /* 8008E154 andi $v1, $v1, 0x3F */
    r_v1 = r_v1 << 1; /* 8008E158 sll $v1, $v1, 1 */
    bl_sb(r_v0 + 0x15Cu, r_v1); /* 8008E15C sb $v1, 0x15C($v0) */
    r_v1 = 0x800D0000u; /* 8008E160 lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 8008E164 lw $v1, %lo(D_800CCB34)($v1) */
    ; /* 8008E168 nop  */
    r_v0 = r_v1 << 2; /* 8008E16C sll $v0, $v1, 2 */
    r_v0 = r_v0 + r_v1; /* 8008E170 addu $v0, $v0, $v1 */
    r_v1 = 0x800C0000u; /* 8008E174 lui $v1, %hi(D_800C3EA4) */
    r_v1 = bl_lw(r_v1 + 0x3EA4u); /* 8008E178 lw $v1, %lo(D_800C3EA4)($v1) */
    r_v0 = r_v0 << 3; /* 8008E17C sll $v0, $v0, 3 */
    r_at = 0x10000u; /* 8008E180 lui $at, (0x10000 >> 16) */
    r_at = r_v1 + r_at; /* 8008E184 addu $at, $v1, $at */
    r_v1 = bl_lbu(r_at + 0xFFFFA260u); /* 8008E188 lbu $v1, -0x5DA0($at) */
    r_v0 = r_v0 + r_s1; /* 8008E18C addu $v0, $v0, $s1 */
    r_v1 = r_v1 + 0x10u; /* 8008E190 addiu $v1, $v1, 0x10 */
    bl_sb(r_v0 + 0x15Du, r_v1); /* 8008E194 sb $v1, 0x15D($v0) */
    r_v0 = 0x800D0000u; /* 8008E198 lui $v0, %hi(D_800CCB34) */
    r_v0 = bl_lw(r_v0 + 0xFFFFCB34u); /* 8008E19C lw $v0, %lo(D_800CCB34)($v0) */
    ; /* 8008E1A0 nop  */
    r_v1 = r_v0 << 2; /* 8008E1A4 sll $v1, $v0, 2 */
    r_v1 = r_v1 + r_v0; /* 8008E1A8 addu $v1, $v1, $v0 */
    r_v0 = 0x800C0000u; /* 8008E1AC lui $v0, %hi(D_800C3EA4) */
    r_v0 = bl_lw(r_v0 + 0x3EA4u); /* 8008E1B0 lw $v0, %lo(D_800C3EA4)($v0) */
    r_v1 = r_v1 << 3; /* 8008E1B4 sll $v1, $v1, 3 */
    r_at = 0x10000u; /* 8008E1B8 lui $at, (0x10000 >> 16) */
    r_at = r_v0 + r_at; /* 8008E1BC addu $at, $v0, $at */
    r_v0 = bl_lbu(r_at + 0xFFFFA25Cu); /* 8008E1C0 lbu $v0, -0x5DA4($at) */
    r_v1 = r_v1 + r_s1; /* 8008E1C4 addu $v1, $v1, $s1 */
    r_v0 = r_v0 & 0x3Fu; /* 8008E1C8 andi $v0, $v0, 0x3F */
    r_v0 = r_v0 << 1; /* 8008E1CC sll $v0, $v0, 1 */
    r_v0 = r_v0 + 0x7u; /* 8008E1D0 addiu $v0, $v0, 0x7 */
    bl_sb(r_v1 + 0x164u, r_v0); /* 8008E1D4 sb $v0, 0x164($v1) */
    r_v1 = 0x800D0000u; /* 8008E1D8 lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 8008E1DC lw $v1, %lo(D_800CCB34)($v1) */
    ; /* 8008E1E0 nop  */
    r_v0 = r_v1 << 2; /* 8008E1E4 sll $v0, $v1, 2 */
    r_v0 = r_v0 + r_v1; /* 8008E1E8 addu $v0, $v0, $v1 */
    r_v1 = 0x800C0000u; /* 8008E1EC lui $v1, %hi(D_800C3EA4) */
    r_v1 = bl_lw(r_v1 + 0x3EA4u); /* 8008E1F0 lw $v1, %lo(D_800C3EA4)($v1) */
    r_v0 = r_v0 << 3; /* 8008E1F4 sll $v0, $v0, 3 */
    r_at = 0x10000u; /* 8008E1F8 lui $at, (0x10000 >> 16) */
    r_at = r_v1 + r_at; /* 8008E1FC addu $at, $v1, $at */
    r_v1 = bl_lbu(r_at + 0xFFFFA260u); /* 8008E200 lbu $v1, -0x5DA0($at) */
    r_v0 = r_v0 + r_s1; /* 8008E204 addu $v0, $v0, $s1 */
    r_v1 = r_v1 + 0x10u; /* 8008E208 addiu $v1, $v1, 0x10 */
    bl_sb(r_v0 + 0x165u, r_v1); /* 8008E20C sb $v1, 0x165($v0) */
    r_v1 = 0x800D0000u; /* 8008E210 lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 8008E214 lw $v1, %lo(D_800CCB34)($v1) */
    ; /* 8008E218 nop  */
    r_v0 = r_v1 << 2; /* 8008E21C sll $v0, $v1, 2 */
    r_v0 = r_v0 + r_v1; /* 8008E220 addu $v0, $v0, $v1 */
    r_v1 = 0x800C0000u; /* 8008E224 lui $v1, %hi(D_800C3EA4) */
    r_v1 = bl_lw(r_v1 + 0x3EA4u); /* 8008E228 lw $v1, %lo(D_800C3EA4)($v1) */
    r_v0 = r_v0 << 3; /* 8008E22C sll $v0, $v0, 3 */
    r_at = 0x10000u; /* 8008E230 lui $at, (0x10000 >> 16) */
    r_at = r_v1 + r_at; /* 8008E234 addu $at, $v1, $at */
    r_v1 = bl_lbu(r_at + 0xFFFFA25Cu); /* 8008E238 lbu $v1, -0x5DA4($at) */
    r_v0 = r_v0 + r_s1; /* 8008E23C addu $v0, $v0, $s1 */
    r_v1 = r_v1 & 0x3Fu; /* 8008E240 andi $v1, $v1, 0x3F */
    r_v1 = r_v1 << 1; /* 8008E244 sll $v1, $v1, 1 */
    bl_sb(r_v0 + 0x19Cu, r_v1); /* 8008E248 sb $v1, 0x19C($v0) */
    r_v1 = 0x800D0000u; /* 8008E24C lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 8008E250 lw $v1, %lo(D_800CCB34)($v1) */
    ; /* 8008E254 nop  */
    r_v0 = r_v1 << 2; /* 8008E258 sll $v0, $v1, 2 */
    r_v0 = r_v0 + r_v1; /* 8008E25C addu $v0, $v0, $v1 */
    r_v1 = 0x800C0000u; /* 8008E260 lui $v1, %hi(D_800C3EA4) */
    r_v1 = bl_lw(r_v1 + 0x3EA4u); /* 8008E264 lw $v1, %lo(D_800C3EA4)($v1) */
    r_v0 = r_v0 << 3; /* 8008E268 sll $v0, $v0, 3 */
    r_at = 0x10000u; /* 8008E26C lui $at, (0x10000 >> 16) */
    r_at = r_v1 + r_at; /* 8008E270 addu $at, $v1, $at */
    r_v1 = bl_lbu(r_at + 0xFFFFA260u); /* 8008E274 lbu $v1, -0x5DA0($at) */
    r_v0 = r_v0 + r_s1; /* 8008E278 addu $v0, $v0, $s1 */
    bl_sb(r_v0 + 0x19Du, r_v1); /* 8008E27C sb $v1, 0x19D($v0) */
    r_v0 = 0x800D0000u; /* 8008E280 lui $v0, %hi(D_800CCB34) */
    r_v0 = bl_lw(r_v0 + 0xFFFFCB34u); /* 8008E284 lw $v0, %lo(D_800CCB34)($v0) */
    ; /* 8008E288 nop  */
    r_v1 = r_v0 << 2; /* 8008E28C sll $v1, $v0, 2 */
    r_v1 = r_v1 + r_v0; /* 8008E290 addu $v1, $v1, $v0 */
    r_v0 = 0x800C0000u; /* 8008E294 lui $v0, %hi(D_800C3EA4) */
    r_v0 = bl_lw(r_v0 + 0x3EA4u); /* 8008E298 lw $v0, %lo(D_800C3EA4)($v0) */
    r_v1 = r_v1 << 3; /* 8008E29C sll $v1, $v1, 3 */
    r_at = 0x10000u; /* 8008E2A0 lui $at, (0x10000 >> 16) */
    r_at = r_v0 + r_at; /* 8008E2A4 addu $at, $v0, $at */
    r_v0 = bl_lbu(r_at + 0xFFFFA25Cu); /* 8008E2A8 lbu $v0, -0x5DA4($at) */
    r_v1 = r_v1 + r_s1; /* 8008E2AC addu $v1, $v1, $s1 */
    r_v0 = r_v0 & 0x3Fu; /* 8008E2B0 andi $v0, $v0, 0x3F */
    r_v0 = r_v0 << 1; /* 8008E2B4 sll $v0, $v0, 1 */
    r_v0 = r_v0 + 0x7u; /* 8008E2B8 addiu $v0, $v0, 0x7 */
    bl_sb(r_v1 + 0x1A4u, r_v0); /* 8008E2BC sb $v0, 0x1A4($v1) */
    r_v1 = 0x800D0000u; /* 8008E2C0 lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 8008E2C4 lw $v1, %lo(D_800CCB34)($v1) */
    ; /* 8008E2C8 nop  */
    r_v0 = r_v1 << 2; /* 8008E2CC sll $v0, $v1, 2 */
    r_v0 = r_v0 + r_v1; /* 8008E2D0 addu $v0, $v0, $v1 */
    r_v1 = 0x800C0000u; /* 8008E2D4 lui $v1, %hi(D_800C3EA4) */
    r_v1 = bl_lw(r_v1 + 0x3EA4u); /* 8008E2D8 lw $v1, %lo(D_800C3EA4)($v1) */
    r_v0 = r_v0 << 3; /* 8008E2DC sll $v0, $v0, 3 */
    r_at = 0x10000u; /* 8008E2E0 lui $at, (0x10000 >> 16) */
    r_at = r_v1 + r_at; /* 8008E2E4 addu $at, $v1, $at */
    r_v1 = bl_lbu(r_at + 0xFFFFA260u); /* 8008E2E8 lbu $v1, -0x5DA0($at) */
    r_v0 = r_v0 + r_s1; /* 8008E2EC addu $v0, $v0, $s1 */
    bl_sb(r_v0 + 0x1A5u, r_v1); /* 8008E2F0 sb $v1, 0x1A5($v0) */
    r_v1 = 0x800D0000u; /* 8008E2F4 lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 8008E2F8 lw $v1, %lo(D_800CCB34)($v1) */
    ; /* 8008E2FC nop  */
    r_v0 = r_v1 << 2; /* 8008E300 sll $v0, $v1, 2 */
    r_v0 = r_v0 + r_v1; /* 8008E304 addu $v0, $v0, $v1 */
    r_v1 = 0x800C0000u; /* 8008E308 lui $v1, %hi(D_800C3EA4) */
    r_v1 = bl_lw(r_v1 + 0x3EA4u); /* 8008E30C lw $v1, %lo(D_800C3EA4)($v1) */
    r_v0 = r_v0 << 3; /* 8008E310 sll $v0, $v0, 3 */
    r_at = 0x10000u; /* 8008E314 lui $at, (0x10000 >> 16) */
    r_at = r_v1 + r_at; /* 8008E318 addu $at, $v1, $at */
    r_v1 = bl_lbu(r_at + 0xFFFFA25Cu); /* 8008E31C lbu $v1, -0x5DA4($at) */
    r_v0 = r_v0 + r_s1; /* 8008E320 addu $v0, $v0, $s1 */
    r_v1 = r_v1 & 0x3Fu; /* 8008E324 andi $v1, $v1, 0x3F */
    r_v1 = r_v1 << 1; /* 8008E328 sll $v1, $v1, 1 */
    bl_sb(r_v0 + 0x1ACu, r_v1); /* 8008E32C sb $v1, 0x1AC($v0) */
    r_v1 = 0x800D0000u; /* 8008E330 lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 8008E334 lw $v1, %lo(D_800CCB34)($v1) */
    ; /* 8008E338 nop  */
    r_v0 = r_v1 << 2; /* 8008E33C sll $v0, $v1, 2 */
    r_v0 = r_v0 + r_v1; /* 8008E340 addu $v0, $v0, $v1 */
    r_v1 = 0x800C0000u; /* 8008E344 lui $v1, %hi(D_800C3EA4) */
    r_v1 = bl_lw(r_v1 + 0x3EA4u); /* 8008E348 lw $v1, %lo(D_800C3EA4)($v1) */
    r_v0 = r_v0 << 3; /* 8008E34C sll $v0, $v0, 3 */
    r_at = 0x10000u; /* 8008E350 lui $at, (0x10000 >> 16) */
    r_at = r_v1 + r_at; /* 8008E354 addu $at, $v1, $at */
    r_v1 = bl_lbu(r_at + 0xFFFFA260u); /* 8008E358 lbu $v1, -0x5DA0($at) */
    r_v0 = r_v0 + r_s1; /* 8008E35C addu $v0, $v0, $s1 */
    r_v1 = r_v1 + 0x10u; /* 8008E360 addiu $v1, $v1, 0x10 */
    bl_sb(r_v0 + 0x1ADu, r_v1); /* 8008E364 sb $v1, 0x1AD($v0) */
    r_v0 = 0x800D0000u; /* 8008E368 lui $v0, %hi(D_800CCB34) */
    r_v0 = bl_lw(r_v0 + 0xFFFFCB34u); /* 8008E36C lw $v0, %lo(D_800CCB34)($v0) */
    ; /* 8008E370 nop  */
    r_v1 = r_v0 << 2; /* 8008E374 sll $v1, $v0, 2 */
    r_v1 = r_v1 + r_v0; /* 8008E378 addu $v1, $v1, $v0 */
    r_v0 = 0x800C0000u; /* 8008E37C lui $v0, %hi(D_800C3EA4) */
    r_v0 = bl_lw(r_v0 + 0x3EA4u); /* 8008E380 lw $v0, %lo(D_800C3EA4)($v0) */
    r_v1 = r_v1 << 3; /* 8008E384 sll $v1, $v1, 3 */
    r_at = 0x10000u; /* 8008E388 lui $at, (0x10000 >> 16) */
    r_at = r_v0 + r_at; /* 8008E38C addu $at, $v0, $at */
    r_v0 = bl_lbu(r_at + 0xFFFFA25Cu); /* 8008E390 lbu $v0, -0x5DA4($at) */
    r_v1 = r_v1 + r_s1; /* 8008E394 addu $v1, $v1, $s1 */
    r_v0 = r_v0 & 0x3Fu; /* 8008E398 andi $v0, $v0, 0x3F */
    r_v0 = r_v0 << 1; /* 8008E39C sll $v0, $v0, 1 */
    r_v0 = r_v0 + 0x7u; /* 8008E3A0 addiu $v0, $v0, 0x7 */
    bl_sb(r_v1 + 0x1B4u, r_v0); /* 8008E3A4 sb $v0, 0x1B4($v1) */
    r_v1 = 0x800D0000u; /* 8008E3A8 lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 8008E3AC lw $v1, %lo(D_800CCB34)($v1) */
    r_s0 = 0u + 0u; /* 8008E3B0 addu $s0, $zero, $zero */
    r_v0 = r_v1 << 2; /* 8008E3B4 sll $v0, $v1, 2 */
    r_v0 = r_v0 + r_v1; /* 8008E3B8 addu $v0, $v0, $v1 */
    r_v1 = 0x800C0000u; /* 8008E3BC lui $v1, %hi(D_800C3EA4) */
    r_v1 = bl_lw(r_v1 + 0x3EA4u); /* 8008E3C0 lw $v1, %lo(D_800C3EA4)($v1) */
    r_v0 = r_v0 << 3; /* 8008E3C4 sll $v0, $v0, 3 */
    r_at = 0x10000u; /* 8008E3C8 lui $at, (0x10000 >> 16) */
    r_at = r_v1 + r_at; /* 8008E3CC addu $at, $v1, $at */
    r_v1 = bl_lbu(r_at + 0xFFFFA260u); /* 8008E3D0 lbu $v1, -0x5DA0($at) */
    r_v0 = r_v0 + r_s1; /* 8008E3D4 addu $v0, $v0, $s1 */
    r_v1 = r_v1 + 0x10u; /* 8008E3D8 addiu $v1, $v1, 0x10 */
    bl_sb(r_v0 + 0x1B5u, r_v1); /* 8008E3DC sb $v1, 0x1B5($v0) */
    r_v0 = r_s0 << 1; /* 8008E3E0 sll $v0, $s0, 1 */
L_8008E3E4:
    r_v1 = 0x800D0000u; /* 8008E3E4 lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 8008E3E8 lw $v1, %lo(D_800CCB34)($v1) */
    r_s0 = r_s0 + 0x1u; /* 8008E3EC addiu $s0, $s0, 0x1 */
    r_v0 = r_v0 + r_v1; /* 8008E3F0 addu $v0, $v0, $v1 */
    r_a0 = r_v0 << 2; /* 8008E3F4 sll $a0, $v0, 2 */
    r_a0 = r_a0 + r_v0; /* 8008E3F8 addu $a0, $a0, $v0 */
    r_a0 = r_a0 << 3; /* 8008E3FC sll $a0, $a0, 3 */
    r_a0 = r_a0 + 0x140u; /* 8008E400 addiu $a0, $a0, 0x140 */
    /* 8008E404 jal func_80076B00 */
    r_a0 = r_s1 + r_a0;
    bl_jal(FN, 0x80076B00u, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* func_80076B00 */
    r_v0 = (u32)((s32)r_s0 < 2); /* 8008E40C slti $v0, $s0, 0x2 */
    /* 8008E410 bnez $v0, .L8008E3E4 */
    { int c_ = (r_v0 != 0); r_v0 = r_s0 << 1; if (c_) goto L_8008E3E4; }
    r_ra = bl_lw(r_sp + 0x18u); /* 8008E418 lw $ra, 0x18($sp) */
    r_s1 = bl_lw(r_sp + 0x14u); /* 8008E41C lw $s1, 0x14($sp) */
    r_s0 = bl_lw(r_sp + 0x10u); /* 8008E420 lw $s0, 0x10($sp) */
    r_sp = r_sp + 0x20u; /* 8008E424 addiu $sp, $sp, 0x20 */
    /* 8008E428 jr $ra */
    ; goto bl_exit;
bl_exit:
    bl_leave(bl_saved);
    (void)hi; (void)lo; (void)r_at; (void)r_v1; (void)r_k0; (void)r_k1; (void)r_gp; (void)r_ra;
    (void)r_v0;
}

/* func_8008E430.s (400 instructions; --args 5 --void), frontier step2.  Callees through the jal service: func_80076B00. */
void func_8008E430(u32 arg0, u32 arg1, u32 arg2, u32 arg3, u32 arg4) {
    static const char FN[] = "func_8008E430";
    u32 r_at = 0, r_v0 = 0, r_v1 = 0, r_a0 = 0, r_a1 = 0, r_a2 = 0, r_a3 = 0, r_t0 = 0, r_t1 = 0, r_t2 = 0, r_t3 = 0, r_t4 = 0, r_t5 = 0, r_t6 = 0, r_t7 = 0, r_s0 = 0, r_s1 = 0, r_s2 = 0, r_s3 = 0, r_s4 = 0, r_s5 = 0, r_s6 = 0, r_s7 = 0, r_t8 = 0, r_t9 = 0, r_k0 = 0, r_k1 = 0, r_gp = 0, r_sp = 0, r_fp = 0, r_ra = 0;
    u32 hi = 0, lo = 0, bl_saved;
    r_sp = bl_enter(FN, 0x20u, &bl_saved);
    r_a0 = arg0;
    r_a1 = arg1;
    r_a2 = arg2;
    r_a3 = arg3;
    bl_sw(r_sp + 0x10u, arg4);
    r_sp = r_sp + 0xFFFFFFE0u; /* 8008E430 addiu $sp, $sp, -0x20 */
    r_a0 = r_a0 & 0xFFu; /* 8008E434 andi $a0, $a0, 0xFF */
    r_a0 = r_a0 << 2; /* 8008E438 sll $a0, $a0, 2 */
    r_v1 = 0x800D0000u; /* 8008E43C lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 8008E440 lw $v1, %lo(D_800CCB34)($v1) */
    r_a1 = r_a1 + 0x8u; /* 8008E444 addiu $a1, $a1, 0x8 */
    bl_sw(r_sp + 0x18u, r_ra); /* 8008E448 sw $ra, 0x18($sp) */
    bl_sw(r_sp + 0x14u, r_s1); /* 8008E44C sw $s1, 0x14($sp) */
    bl_sw(r_sp + 0x10u, r_s0); /* 8008E450 sw $s0, 0x10($sp) */
    r_at = 0x800D0000u; /* 8008E454 lui $at, %hi(D_800D2E38) */
    r_at = r_at + r_a0; /* 8008E458 addu $at, $at, $a0 */
    r_s1 = bl_lw(r_at + 0x2E38u); /* 8008E45C lw $s1, %lo(D_800D2E38)($at) */
    r_v0 = r_v1 << 2; /* 8008E460 sll $v0, $v1, 2 */
    r_v0 = r_v0 + r_v1; /* 8008E464 addu $v0, $v0, $v1 */
    r_v0 = r_v0 << 3; /* 8008E468 sll $v0, $v0, 3 */
    r_v0 = r_v0 + r_s1; /* 8008E46C addu $v0, $v0, $s1 */
    bl_sh(r_v0 + 0x1E8u, r_a1); /* 8008E470 sh $a1, 0x1E8($v0) */
    r_v1 = 0x800D0000u; /* 8008E474 lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 8008E478 lw $v1, %lo(D_800CCB34)($v1) */
    r_a3 = r_a3 & 0xFFFFu; /* 8008E47C andi $a3, $a3, 0xFFFF */
    r_v0 = r_v1 << 2; /* 8008E480 sll $v0, $v1, 2 */
    r_v0 = r_v0 + r_v1; /* 8008E484 addu $v0, $v0, $v1 */
    r_v0 = r_v0 << 3; /* 8008E488 sll $v0, $v0, 3 */
    r_v1 = bl_lw(r_sp + 0x30u); /* 8008E48C lw $v1, 0x30($sp) */
    r_v0 = r_v0 + r_s1; /* 8008E490 addu $v0, $v0, $s1 */
    r_a2 = r_a2 + r_v1; /* 8008E494 addu $a2, $a2, $v1 */
    r_t0 = r_a2 + 0xFFFFFFF8u; /* 8008E498 addiu $t0, $a2, -0x8 */
    bl_sh(r_v0 + 0x1EAu, r_t0); /* 8008E49C sh $t0, 0x1EA($v0) */
    r_v1 = 0x800D0000u; /* 8008E4A0 lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 8008E4A4 lw $v1, %lo(D_800CCB34)($v1) */
    r_a3 = r_a3 + 0xFFFFFFF0u; /* 8008E4A8 addiu $a3, $a3, -0x10 */
    r_v0 = r_v1 << 2; /* 8008E4AC sll $v0, $v1, 2 */
    r_v0 = r_v0 + r_v1; /* 8008E4B0 addu $v0, $v0, $v1 */
    r_v0 = r_v0 << 3; /* 8008E4B4 sll $v0, $v0, 3 */
    r_v0 = r_v0 + r_s1; /* 8008E4B8 addu $v0, $v0, $s1 */
    r_v1 = r_a3 >> 31; /* 8008E4BC srl $v1, $a3, 31 */
    r_a3 = r_a3 + r_v1; /* 8008E4C0 addu $a3, $a3, $v1 */
    r_a3 = BL_SRA(r_a3, 1); /* 8008E4C4 sra $a3, $a3, 1 */
    r_a0 = r_a1 + r_a3; /* 8008E4C8 addu $a0, $a1, $a3 */
    bl_sh(r_v0 + 0x1F0u, r_a0); /* 8008E4CC sh $a0, 0x1F0($v0) */
    r_v1 = 0x800D0000u; /* 8008E4D0 lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 8008E4D4 lw $v1, %lo(D_800CCB34)($v1) */
    ; /* 8008E4D8 nop  */
    r_v0 = r_v1 << 2; /* 8008E4DC sll $v0, $v1, 2 */
    r_v0 = r_v0 + r_v1; /* 8008E4E0 addu $v0, $v0, $v1 */
    r_v0 = r_v0 << 3; /* 8008E4E4 sll $v0, $v0, 3 */
    r_v0 = r_v0 + r_s1; /* 8008E4E8 addu $v0, $v0, $s1 */
    bl_sh(r_v0 + 0x1F2u, r_t0); /* 8008E4EC sh $t0, 0x1F2($v0) */
    r_v1 = 0x800D0000u; /* 8008E4F0 lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 8008E4F4 lw $v1, %lo(D_800CCB34)($v1) */
    ; /* 8008E4F8 nop  */
    r_v0 = r_v1 << 2; /* 8008E4FC sll $v0, $v1, 2 */
    r_v0 = r_v0 + r_v1; /* 8008E500 addu $v0, $v0, $v1 */
    r_v0 = r_v0 << 3; /* 8008E504 sll $v0, $v0, 3 */
    r_v0 = r_v0 + r_s1; /* 8008E508 addu $v0, $v0, $s1 */
    bl_sh(r_v0 + 0x1F8u, r_a1); /* 8008E50C sh $a1, 0x1F8($v0) */
    r_v1 = 0x800D0000u; /* 8008E510 lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 8008E514 lw $v1, %lo(D_800CCB34)($v1) */
    r_a2 = r_a2 + 0x8u; /* 8008E518 addiu $a2, $a2, 0x8 */
    r_v0 = r_v1 << 2; /* 8008E51C sll $v0, $v1, 2 */
    r_v0 = r_v0 + r_v1; /* 8008E520 addu $v0, $v0, $v1 */
    r_v0 = r_v0 << 3; /* 8008E524 sll $v0, $v0, 3 */
    r_v0 = r_v0 + r_s1; /* 8008E528 addu $v0, $v0, $s1 */
    bl_sh(r_v0 + 0x1FAu, r_a2); /* 8008E52C sh $a2, 0x1FA($v0) */
    r_v1 = 0x800D0000u; /* 8008E530 lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 8008E534 lw $v1, %lo(D_800CCB34)($v1) */
    ; /* 8008E538 nop  */
    r_v0 = r_v1 << 2; /* 8008E53C sll $v0, $v1, 2 */
    r_v0 = r_v0 + r_v1; /* 8008E540 addu $v0, $v0, $v1 */
    r_v0 = r_v0 << 3; /* 8008E544 sll $v0, $v0, 3 */
    r_v0 = r_v0 + r_s1; /* 8008E548 addu $v0, $v0, $s1 */
    bl_sh(r_v0 + 0x200u, r_a0); /* 8008E54C sh $a0, 0x200($v0) */
    r_v1 = 0x800D0000u; /* 8008E550 lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 8008E554 lw $v1, %lo(D_800CCB34)($v1) */
    ; /* 8008E558 nop  */
    r_v0 = r_v1 << 2; /* 8008E55C sll $v0, $v1, 2 */
    r_v0 = r_v0 + r_v1; /* 8008E560 addu $v0, $v0, $v1 */
    r_v0 = r_v0 << 3; /* 8008E564 sll $v0, $v0, 3 */
    r_v0 = r_v0 + r_s1; /* 8008E568 addu $v0, $v0, $s1 */
    bl_sh(r_v0 + 0x202u, r_a2); /* 8008E56C sh $a2, 0x202($v0) */
    r_v1 = 0x800D0000u; /* 8008E570 lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 8008E574 lw $v1, %lo(D_800CCB34)($v1) */
    ; /* 8008E578 nop  */
    r_v0 = r_v1 << 2; /* 8008E57C sll $v0, $v1, 2 */
    r_v0 = r_v0 + r_v1; /* 8008E580 addu $v0, $v0, $v1 */
    r_v0 = r_v0 << 3; /* 8008E584 sll $v0, $v0, 3 */
    r_v0 = r_v0 + r_s1; /* 8008E588 addu $v0, $v0, $s1 */
    bl_sh(r_v0 + 0x238u, r_a0); /* 8008E58C sh $a0, 0x238($v0) */
    r_v1 = 0x800D0000u; /* 8008E590 lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 8008E594 lw $v1, %lo(D_800CCB34)($v1) */
    ; /* 8008E598 nop  */
    r_v0 = r_v1 << 2; /* 8008E59C sll $v0, $v1, 2 */
    r_v0 = r_v0 + r_v1; /* 8008E5A0 addu $v0, $v0, $v1 */
    r_v0 = r_v0 << 3; /* 8008E5A4 sll $v0, $v0, 3 */
    r_v0 = r_v0 + r_s1; /* 8008E5A8 addu $v0, $v0, $s1 */
    bl_sh(r_v0 + 0x23Au, r_t0); /* 8008E5AC sh $t0, 0x23A($v0) */
    r_v1 = 0x800D0000u; /* 8008E5B0 lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 8008E5B4 lw $v1, %lo(D_800CCB34)($v1) */
    r_a3 = r_a0 + r_a3; /* 8008E5B8 addu $a3, $a0, $a3 */
    r_v0 = r_v1 << 2; /* 8008E5BC sll $v0, $v1, 2 */
    r_v0 = r_v0 + r_v1; /* 8008E5C0 addu $v0, $v0, $v1 */
    r_v0 = r_v0 << 3; /* 8008E5C4 sll $v0, $v0, 3 */
    r_v0 = r_v0 + r_s1; /* 8008E5C8 addu $v0, $v0, $s1 */
    bl_sh(r_v0 + 0x240u, r_a3); /* 8008E5CC sh $a3, 0x240($v0) */
    r_v1 = 0x800D0000u; /* 8008E5D0 lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 8008E5D4 lw $v1, %lo(D_800CCB34)($v1) */
    ; /* 8008E5D8 nop  */
    r_v0 = r_v1 << 2; /* 8008E5DC sll $v0, $v1, 2 */
    r_v0 = r_v0 + r_v1; /* 8008E5E0 addu $v0, $v0, $v1 */
    r_v0 = r_v0 << 3; /* 8008E5E4 sll $v0, $v0, 3 */
    r_v0 = r_v0 + r_s1; /* 8008E5E8 addu $v0, $v0, $s1 */
    bl_sh(r_v0 + 0x242u, r_t0); /* 8008E5EC sh $t0, 0x242($v0) */
    r_v1 = 0x800D0000u; /* 8008E5F0 lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 8008E5F4 lw $v1, %lo(D_800CCB34)($v1) */
    ; /* 8008E5F8 nop  */
    r_v0 = r_v1 << 2; /* 8008E5FC sll $v0, $v1, 2 */
    r_v0 = r_v0 + r_v1; /* 8008E600 addu $v0, $v0, $v1 */
    r_v0 = r_v0 << 3; /* 8008E604 sll $v0, $v0, 3 */
    r_v0 = r_v0 + r_s1; /* 8008E608 addu $v0, $v0, $s1 */
    bl_sh(r_v0 + 0x248u, r_a0); /* 8008E60C sh $a0, 0x248($v0) */
    r_v1 = 0x800D0000u; /* 8008E610 lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 8008E614 lw $v1, %lo(D_800CCB34)($v1) */
    ; /* 8008E618 nop  */
    r_v0 = r_v1 << 2; /* 8008E61C sll $v0, $v1, 2 */
    r_v0 = r_v0 + r_v1; /* 8008E620 addu $v0, $v0, $v1 */
    r_v0 = r_v0 << 3; /* 8008E624 sll $v0, $v0, 3 */
    r_v0 = r_v0 + r_s1; /* 8008E628 addu $v0, $v0, $s1 */
    bl_sh(r_v0 + 0x24Au, r_a2); /* 8008E62C sh $a2, 0x24A($v0) */
    r_v1 = 0x800D0000u; /* 8008E630 lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 8008E634 lw $v1, %lo(D_800CCB34)($v1) */
    ; /* 8008E638 nop  */
    r_v0 = r_v1 << 2; /* 8008E63C sll $v0, $v1, 2 */
    r_v0 = r_v0 + r_v1; /* 8008E640 addu $v0, $v0, $v1 */
    r_v0 = r_v0 << 3; /* 8008E644 sll $v0, $v0, 3 */
    r_v0 = r_v0 + r_s1; /* 8008E648 addu $v0, $v0, $s1 */
    bl_sh(r_v0 + 0x250u, r_a3); /* 8008E64C sh $a3, 0x250($v0) */
    r_v1 = 0x800D0000u; /* 8008E650 lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 8008E654 lw $v1, %lo(D_800CCB34)($v1) */
    ; /* 8008E658 nop  */
    r_v0 = r_v1 << 2; /* 8008E65C sll $v0, $v1, 2 */
    r_v0 = r_v0 + r_v1; /* 8008E660 addu $v0, $v0, $v1 */
    r_v0 = r_v0 << 3; /* 8008E664 sll $v0, $v0, 3 */
    r_v0 = r_v0 + r_s1; /* 8008E668 addu $v0, $v0, $s1 */
    bl_sh(r_v0 + 0x252u, r_a2); /* 8008E66C sh $a2, 0x252($v0) */
    r_v0 = 0x800D0000u; /* 8008E670 lui $v0, %hi(D_800CCB34) */
    r_v0 = bl_lw(r_v0 + 0xFFFFCB34u); /* 8008E674 lw $v0, %lo(D_800CCB34)($v0) */
    ; /* 8008E678 nop  */
    r_v1 = r_v0 << 2; /* 8008E67C sll $v1, $v0, 2 */
    r_v1 = r_v1 + r_v0; /* 8008E680 addu $v1, $v1, $v0 */
    r_v0 = 0x800C0000u; /* 8008E684 lui $v0, %hi(D_800C3EA4) */
    r_v0 = bl_lw(r_v0 + 0x3EA4u); /* 8008E688 lw $v0, %lo(D_800C3EA4)($v0) */
    r_v1 = r_v1 << 3; /* 8008E68C sll $v1, $v1, 3 */
    r_at = 0x10000u; /* 8008E690 lui $at, (0x10000 >> 16) */
    r_at = r_v0 + r_at; /* 8008E694 addu $at, $v0, $at */
    r_v0 = bl_lbu(r_at + 0xFFFFA274u); /* 8008E698 lbu $v0, -0x5D8C($at) */
    r_v1 = r_v1 + r_s1; /* 8008E69C addu $v1, $v1, $s1 */
    r_v0 = r_v0 & 0x3Fu; /* 8008E6A0 andi $v0, $v0, 0x3F */
    r_v0 = r_v0 << 1; /* 8008E6A4 sll $v0, $v0, 1 */
    r_v0 = r_v0 + 0xFFFFFFF8u; /* 8008E6A8 addiu $v0, $v0, -0x8 */
    bl_sb(r_v1 + 0x1ECu, r_v0); /* 8008E6AC sb $v0, 0x1EC($v1) */
    r_v1 = 0x800D0000u; /* 8008E6B0 lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 8008E6B4 lw $v1, %lo(D_800CCB34)($v1) */
    ; /* 8008E6B8 nop  */
    r_v0 = r_v1 << 2; /* 8008E6BC sll $v0, $v1, 2 */
    r_v0 = r_v0 + r_v1; /* 8008E6C0 addu $v0, $v0, $v1 */
    r_v1 = 0x800C0000u; /* 8008E6C4 lui $v1, %hi(D_800C3EA4) */
    r_v1 = bl_lw(r_v1 + 0x3EA4u); /* 8008E6C8 lw $v1, %lo(D_800C3EA4)($v1) */
    r_v0 = r_v0 << 3; /* 8008E6CC sll $v0, $v0, 3 */
    r_at = 0x10000u; /* 8008E6D0 lui $at, (0x10000 >> 16) */
    r_at = r_v1 + r_at; /* 8008E6D4 addu $at, $v1, $at */
    r_v1 = bl_lbu(r_at + 0xFFFFA278u); /* 8008E6D8 lbu $v1, -0x5D88($at) */
    r_v0 = r_v0 + r_s1; /* 8008E6DC addu $v0, $v0, $s1 */
    bl_sb(r_v0 + 0x1EDu, r_v1); /* 8008E6E0 sb $v1, 0x1ED($v0) */
    r_v0 = 0x800D0000u; /* 8008E6E4 lui $v0, %hi(D_800CCB34) */
    r_v0 = bl_lw(r_v0 + 0xFFFFCB34u); /* 8008E6E8 lw $v0, %lo(D_800CCB34)($v0) */
    ; /* 8008E6EC nop  */
    r_v1 = r_v0 << 2; /* 8008E6F0 sll $v1, $v0, 2 */
    r_v1 = r_v1 + r_v0; /* 8008E6F4 addu $v1, $v1, $v0 */
    r_v0 = 0x800C0000u; /* 8008E6F8 lui $v0, %hi(D_800C3EA4) */
    r_v0 = bl_lw(r_v0 + 0x3EA4u); /* 8008E6FC lw $v0, %lo(D_800C3EA4)($v0) */
    r_v1 = r_v1 << 3; /* 8008E700 sll $v1, $v1, 3 */
    r_at = 0x10000u; /* 8008E704 lui $at, (0x10000 >> 16) */
    r_at = r_v0 + r_at; /* 8008E708 addu $at, $v0, $at */
    r_v0 = bl_lbu(r_at + 0xFFFFA274u); /* 8008E70C lbu $v0, -0x5D8C($at) */
    r_v1 = r_v1 + r_s1; /* 8008E710 addu $v1, $v1, $s1 */
    r_v0 = r_v0 & 0x3Fu; /* 8008E714 andi $v0, $v0, 0x3F */
    r_v0 = r_v0 << 1; /* 8008E718 sll $v0, $v0, 1 */
    r_v0 = r_v0 + 0xFFFFFFFFu; /* 8008E71C addiu $v0, $v0, -0x1 */
    bl_sb(r_v1 + 0x1F4u, r_v0); /* 8008E720 sb $v0, 0x1F4($v1) */
    r_v1 = 0x800D0000u; /* 8008E724 lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 8008E728 lw $v1, %lo(D_800CCB34)($v1) */
    ; /* 8008E72C nop  */
    r_v0 = r_v1 << 2; /* 8008E730 sll $v0, $v1, 2 */
    r_v0 = r_v0 + r_v1; /* 8008E734 addu $v0, $v0, $v1 */
    r_v1 = 0x800C0000u; /* 8008E738 lui $v1, %hi(D_800C3EA4) */
    r_v1 = bl_lw(r_v1 + 0x3EA4u); /* 8008E73C lw $v1, %lo(D_800C3EA4)($v1) */
    r_v0 = r_v0 << 3; /* 8008E740 sll $v0, $v0, 3 */
    r_at = 0x10000u; /* 8008E744 lui $at, (0x10000 >> 16) */
    r_at = r_v1 + r_at; /* 8008E748 addu $at, $v1, $at */
    r_v1 = bl_lbu(r_at + 0xFFFFA278u); /* 8008E74C lbu $v1, -0x5D88($at) */
    r_v0 = r_v0 + r_s1; /* 8008E750 addu $v0, $v0, $s1 */
    bl_sb(r_v0 + 0x1F5u, r_v1); /* 8008E754 sb $v1, 0x1F5($v0) */
    r_v0 = 0x800D0000u; /* 8008E758 lui $v0, %hi(D_800CCB34) */
    r_v0 = bl_lw(r_v0 + 0xFFFFCB34u); /* 8008E75C lw $v0, %lo(D_800CCB34)($v0) */
    ; /* 8008E760 nop  */
    r_v1 = r_v0 << 2; /* 8008E764 sll $v1, $v0, 2 */
    r_v1 = r_v1 + r_v0; /* 8008E768 addu $v1, $v1, $v0 */
    r_v0 = 0x800C0000u; /* 8008E76C lui $v0, %hi(D_800C3EA4) */
    r_v0 = bl_lw(r_v0 + 0x3EA4u); /* 8008E770 lw $v0, %lo(D_800C3EA4)($v0) */
    r_v1 = r_v1 << 3; /* 8008E774 sll $v1, $v1, 3 */
    r_at = 0x10000u; /* 8008E778 lui $at, (0x10000 >> 16) */
    r_at = r_v0 + r_at; /* 8008E77C addu $at, $v0, $at */
    r_v0 = bl_lbu(r_at + 0xFFFFA274u); /* 8008E780 lbu $v0, -0x5D8C($at) */
    r_v1 = r_v1 + r_s1; /* 8008E784 addu $v1, $v1, $s1 */
    r_v0 = r_v0 & 0x3Fu; /* 8008E788 andi $v0, $v0, 0x3F */
    r_v0 = r_v0 << 1; /* 8008E78C sll $v0, $v0, 1 */
    r_v0 = r_v0 + 0xFFFFFFF8u; /* 8008E790 addiu $v0, $v0, -0x8 */
    bl_sb(r_v1 + 0x1FCu, r_v0); /* 8008E794 sb $v0, 0x1FC($v1) */
    r_v1 = 0x800D0000u; /* 8008E798 lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 8008E79C lw $v1, %lo(D_800CCB34)($v1) */
    ; /* 8008E7A0 nop  */
    r_v0 = r_v1 << 2; /* 8008E7A4 sll $v0, $v1, 2 */
    r_v0 = r_v0 + r_v1; /* 8008E7A8 addu $v0, $v0, $v1 */
    r_v1 = 0x800C0000u; /* 8008E7AC lui $v1, %hi(D_800C3EA4) */
    r_v1 = bl_lw(r_v1 + 0x3EA4u); /* 8008E7B0 lw $v1, %lo(D_800C3EA4)($v1) */
    r_v0 = r_v0 << 3; /* 8008E7B4 sll $v0, $v0, 3 */
    r_at = 0x10000u; /* 8008E7B8 lui $at, (0x10000 >> 16) */
    r_at = r_v1 + r_at; /* 8008E7BC addu $at, $v1, $at */
    r_v1 = bl_lbu(r_at + 0xFFFFA278u); /* 8008E7C0 lbu $v1, -0x5D88($at) */
    r_v0 = r_v0 + r_s1; /* 8008E7C4 addu $v0, $v0, $s1 */
    r_v1 = r_v1 + 0x10u; /* 8008E7C8 addiu $v1, $v1, 0x10 */
    bl_sb(r_v0 + 0x1FDu, r_v1); /* 8008E7CC sb $v1, 0x1FD($v0) */
    r_v0 = 0x800D0000u; /* 8008E7D0 lui $v0, %hi(D_800CCB34) */
    r_v0 = bl_lw(r_v0 + 0xFFFFCB34u); /* 8008E7D4 lw $v0, %lo(D_800CCB34)($v0) */
    ; /* 8008E7D8 nop  */
    r_v1 = r_v0 << 2; /* 8008E7DC sll $v1, $v0, 2 */
    r_v1 = r_v1 + r_v0; /* 8008E7E0 addu $v1, $v1, $v0 */
    r_v0 = 0x800C0000u; /* 8008E7E4 lui $v0, %hi(D_800C3EA4) */
    r_v0 = bl_lw(r_v0 + 0x3EA4u); /* 8008E7E8 lw $v0, %lo(D_800C3EA4)($v0) */
    r_v1 = r_v1 << 3; /* 8008E7EC sll $v1, $v1, 3 */
    r_at = 0x10000u; /* 8008E7F0 lui $at, (0x10000 >> 16) */
    r_at = r_v0 + r_at; /* 8008E7F4 addu $at, $v0, $at */
    r_v0 = bl_lbu(r_at + 0xFFFFA274u); /* 8008E7F8 lbu $v0, -0x5D8C($at) */
    r_v1 = r_v1 + r_s1; /* 8008E7FC addu $v1, $v1, $s1 */
    r_v0 = r_v0 & 0x3Fu; /* 8008E800 andi $v0, $v0, 0x3F */
    r_v0 = r_v0 << 1; /* 8008E804 sll $v0, $v0, 1 */
    r_v0 = r_v0 + 0xFFFFFFFFu; /* 8008E808 addiu $v0, $v0, -0x1 */
    bl_sb(r_v1 + 0x204u, r_v0); /* 8008E80C sb $v0, 0x204($v1) */
    r_v1 = 0x800D0000u; /* 8008E810 lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 8008E814 lw $v1, %lo(D_800CCB34)($v1) */
    ; /* 8008E818 nop  */
    r_v0 = r_v1 << 2; /* 8008E81C sll $v0, $v1, 2 */
    r_v0 = r_v0 + r_v1; /* 8008E820 addu $v0, $v0, $v1 */
    r_v1 = 0x800C0000u; /* 8008E824 lui $v1, %hi(D_800C3EA4) */
    r_v1 = bl_lw(r_v1 + 0x3EA4u); /* 8008E828 lw $v1, %lo(D_800C3EA4)($v1) */
    r_v0 = r_v0 << 3; /* 8008E82C sll $v0, $v0, 3 */
    r_at = 0x10000u; /* 8008E830 lui $at, (0x10000 >> 16) */
    r_at = r_v1 + r_at; /* 8008E834 addu $at, $v1, $at */
    r_v1 = bl_lbu(r_at + 0xFFFFA278u); /* 8008E838 lbu $v1, -0x5D88($at) */
    r_v0 = r_v0 + r_s1; /* 8008E83C addu $v0, $v0, $s1 */
    r_v1 = r_v1 + 0x10u; /* 8008E840 addiu $v1, $v1, 0x10 */
    bl_sb(r_v0 + 0x205u, r_v1); /* 8008E844 sb $v1, 0x205($v0) */
    r_v0 = 0x800D0000u; /* 8008E848 lui $v0, %hi(D_800CCB34) */
    r_v0 = bl_lw(r_v0 + 0xFFFFCB34u); /* 8008E84C lw $v0, %lo(D_800CCB34)($v0) */
    ; /* 8008E850 nop  */
    r_v1 = r_v0 << 2; /* 8008E854 sll $v1, $v0, 2 */
    r_v1 = r_v1 + r_v0; /* 8008E858 addu $v1, $v1, $v0 */
    r_v0 = 0x800C0000u; /* 8008E85C lui $v0, %hi(D_800C3EA4) */
    r_v0 = bl_lw(r_v0 + 0x3EA4u); /* 8008E860 lw $v0, %lo(D_800C3EA4)($v0) */
    r_v1 = r_v1 << 3; /* 8008E864 sll $v1, $v1, 3 */
    r_at = 0x10000u; /* 8008E868 lui $at, (0x10000 >> 16) */
    r_at = r_v0 + r_at; /* 8008E86C addu $at, $v0, $at */
    r_v0 = bl_lbu(r_at + 0xFFFFA274u); /* 8008E870 lbu $v0, -0x5D8C($at) */
    r_v1 = r_v1 + r_s1; /* 8008E874 addu $v1, $v1, $s1 */
    r_v0 = r_v0 & 0x3Fu; /* 8008E878 andi $v0, $v0, 0x3F */
    r_v0 = r_v0 << 1; /* 8008E87C sll $v0, $v0, 1 */
    r_v0 = r_v0 + 0xFFFFFFF8u; /* 8008E880 addiu $v0, $v0, -0x8 */
    bl_sb(r_v1 + 0x23Cu, r_v0); /* 8008E884 sb $v0, 0x23C($v1) */
    r_v1 = 0x800D0000u; /* 8008E888 lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 8008E88C lw $v1, %lo(D_800CCB34)($v1) */
    ; /* 8008E890 nop  */
    r_v0 = r_v1 << 2; /* 8008E894 sll $v0, $v1, 2 */
    r_v0 = r_v0 + r_v1; /* 8008E898 addu $v0, $v0, $v1 */
    r_v1 = 0x800C0000u; /* 8008E89C lui $v1, %hi(D_800C3EA4) */
    r_v1 = bl_lw(r_v1 + 0x3EA4u); /* 8008E8A0 lw $v1, %lo(D_800C3EA4)($v1) */
    r_v0 = r_v0 << 3; /* 8008E8A4 sll $v0, $v0, 3 */
    r_at = 0x10000u; /* 8008E8A8 lui $at, (0x10000 >> 16) */
    r_at = r_v1 + r_at; /* 8008E8AC addu $at, $v1, $at */
    r_v1 = bl_lbu(r_at + 0xFFFFA278u); /* 8008E8B0 lbu $v1, -0x5D88($at) */
    r_v0 = r_v0 + r_s1; /* 8008E8B4 addu $v0, $v0, $s1 */
    bl_sb(r_v0 + 0x23Du, r_v1); /* 8008E8B8 sb $v1, 0x23D($v0) */
    r_v0 = 0x800D0000u; /* 8008E8BC lui $v0, %hi(D_800CCB34) */
    r_v0 = bl_lw(r_v0 + 0xFFFFCB34u); /* 8008E8C0 lw $v0, %lo(D_800CCB34)($v0) */
    ; /* 8008E8C4 nop  */
    r_v1 = r_v0 << 2; /* 8008E8C8 sll $v1, $v0, 2 */
    r_v1 = r_v1 + r_v0; /* 8008E8CC addu $v1, $v1, $v0 */
    r_v0 = 0x800C0000u; /* 8008E8D0 lui $v0, %hi(D_800C3EA4) */
    r_v0 = bl_lw(r_v0 + 0x3EA4u); /* 8008E8D4 lw $v0, %lo(D_800C3EA4)($v0) */
    r_v1 = r_v1 << 3; /* 8008E8D8 sll $v1, $v1, 3 */
    r_at = 0x10000u; /* 8008E8DC lui $at, (0x10000 >> 16) */
    r_at = r_v0 + r_at; /* 8008E8E0 addu $at, $v0, $at */
    r_v0 = bl_lbu(r_at + 0xFFFFA274u); /* 8008E8E4 lbu $v0, -0x5D8C($at) */
    r_v1 = r_v1 + r_s1; /* 8008E8E8 addu $v1, $v1, $s1 */
    r_v0 = r_v0 & 0x3Fu; /* 8008E8EC andi $v0, $v0, 0x3F */
    r_v0 = r_v0 << 1; /* 8008E8F0 sll $v0, $v0, 1 */
    r_v0 = r_v0 + 0xFFFFFFFFu; /* 8008E8F4 addiu $v0, $v0, -0x1 */
    bl_sb(r_v1 + 0x244u, r_v0); /* 8008E8F8 sb $v0, 0x244($v1) */
    r_v1 = 0x800D0000u; /* 8008E8FC lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 8008E900 lw $v1, %lo(D_800CCB34)($v1) */
    ; /* 8008E904 nop  */
    r_v0 = r_v1 << 2; /* 8008E908 sll $v0, $v1, 2 */
    r_v0 = r_v0 + r_v1; /* 8008E90C addu $v0, $v0, $v1 */
    r_v1 = 0x800C0000u; /* 8008E910 lui $v1, %hi(D_800C3EA4) */
    r_v1 = bl_lw(r_v1 + 0x3EA4u); /* 8008E914 lw $v1, %lo(D_800C3EA4)($v1) */
    r_v0 = r_v0 << 3; /* 8008E918 sll $v0, $v0, 3 */
    r_at = 0x10000u; /* 8008E91C lui $at, (0x10000 >> 16) */
    r_at = r_v1 + r_at; /* 8008E920 addu $at, $v1, $at */
    r_v1 = bl_lbu(r_at + 0xFFFFA278u); /* 8008E924 lbu $v1, -0x5D88($at) */
    r_v0 = r_v0 + r_s1; /* 8008E928 addu $v0, $v0, $s1 */
    bl_sb(r_v0 + 0x245u, r_v1); /* 8008E92C sb $v1, 0x245($v0) */
    r_v0 = 0x800D0000u; /* 8008E930 lui $v0, %hi(D_800CCB34) */
    r_v0 = bl_lw(r_v0 + 0xFFFFCB34u); /* 8008E934 lw $v0, %lo(D_800CCB34)($v0) */
    ; /* 8008E938 nop  */
    r_v1 = r_v0 << 2; /* 8008E93C sll $v1, $v0, 2 */
    r_v1 = r_v1 + r_v0; /* 8008E940 addu $v1, $v1, $v0 */
    r_v0 = 0x800C0000u; /* 8008E944 lui $v0, %hi(D_800C3EA4) */
    r_v0 = bl_lw(r_v0 + 0x3EA4u); /* 8008E948 lw $v0, %lo(D_800C3EA4)($v0) */
    r_v1 = r_v1 << 3; /* 8008E94C sll $v1, $v1, 3 */
    r_at = 0x10000u; /* 8008E950 lui $at, (0x10000 >> 16) */
    r_at = r_v0 + r_at; /* 8008E954 addu $at, $v0, $at */
    r_v0 = bl_lbu(r_at + 0xFFFFA274u); /* 8008E958 lbu $v0, -0x5D8C($at) */
    r_v1 = r_v1 + r_s1; /* 8008E95C addu $v1, $v1, $s1 */
    r_v0 = r_v0 & 0x3Fu; /* 8008E960 andi $v0, $v0, 0x3F */
    r_v0 = r_v0 << 1; /* 8008E964 sll $v0, $v0, 1 */
    r_v0 = r_v0 + 0xFFFFFFF8u; /* 8008E968 addiu $v0, $v0, -0x8 */
    bl_sb(r_v1 + 0x24Cu, r_v0); /* 8008E96C sb $v0, 0x24C($v1) */
    r_v1 = 0x800D0000u; /* 8008E970 lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 8008E974 lw $v1, %lo(D_800CCB34)($v1) */
    ; /* 8008E978 nop  */
    r_v0 = r_v1 << 2; /* 8008E97C sll $v0, $v1, 2 */
    r_v0 = r_v0 + r_v1; /* 8008E980 addu $v0, $v0, $v1 */
    r_v1 = 0x800C0000u; /* 8008E984 lui $v1, %hi(D_800C3EA4) */
    r_v1 = bl_lw(r_v1 + 0x3EA4u); /* 8008E988 lw $v1, %lo(D_800C3EA4)($v1) */
    r_v0 = r_v0 << 3; /* 8008E98C sll $v0, $v0, 3 */
    r_at = 0x10000u; /* 8008E990 lui $at, (0x10000 >> 16) */
    r_at = r_v1 + r_at; /* 8008E994 addu $at, $v1, $at */
    r_v1 = bl_lbu(r_at + 0xFFFFA278u); /* 8008E998 lbu $v1, -0x5D88($at) */
    r_v0 = r_v0 + r_s1; /* 8008E99C addu $v0, $v0, $s1 */
    r_v1 = r_v1 + 0x10u; /* 8008E9A0 addiu $v1, $v1, 0x10 */
    bl_sb(r_v0 + 0x24Du, r_v1); /* 8008E9A4 sb $v1, 0x24D($v0) */
    r_v0 = 0x800D0000u; /* 8008E9A8 lui $v0, %hi(D_800CCB34) */
    r_v0 = bl_lw(r_v0 + 0xFFFFCB34u); /* 8008E9AC lw $v0, %lo(D_800CCB34)($v0) */
    ; /* 8008E9B0 nop  */
    r_v1 = r_v0 << 2; /* 8008E9B4 sll $v1, $v0, 2 */
    r_v1 = r_v1 + r_v0; /* 8008E9B8 addu $v1, $v1, $v0 */
    r_v0 = 0x800C0000u; /* 8008E9BC lui $v0, %hi(D_800C3EA4) */
    r_v0 = bl_lw(r_v0 + 0x3EA4u); /* 8008E9C0 lw $v0, %lo(D_800C3EA4)($v0) */
    r_v1 = r_v1 << 3; /* 8008E9C4 sll $v1, $v1, 3 */
    r_at = 0x10000u; /* 8008E9C8 lui $at, (0x10000 >> 16) */
    r_at = r_v0 + r_at; /* 8008E9CC addu $at, $v0, $at */
    r_v0 = bl_lbu(r_at + 0xFFFFA274u); /* 8008E9D0 lbu $v0, -0x5D8C($at) */
    r_v1 = r_v1 + r_s1; /* 8008E9D4 addu $v1, $v1, $s1 */
    r_v0 = r_v0 & 0x3Fu; /* 8008E9D8 andi $v0, $v0, 0x3F */
    r_v0 = r_v0 << 1; /* 8008E9DC sll $v0, $v0, 1 */
    r_v0 = r_v0 + 0xFFFFFFFFu; /* 8008E9E0 addiu $v0, $v0, -0x1 */
    bl_sb(r_v1 + 0x254u, r_v0); /* 8008E9E4 sb $v0, 0x254($v1) */
    r_v1 = 0x800D0000u; /* 8008E9E8 lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 8008E9EC lw $v1, %lo(D_800CCB34)($v1) */
    r_s0 = 0u + 0u; /* 8008E9F0 addu $s0, $zero, $zero */
    r_v0 = r_v1 << 2; /* 8008E9F4 sll $v0, $v1, 2 */
    r_v0 = r_v0 + r_v1; /* 8008E9F8 addu $v0, $v0, $v1 */
    r_v1 = 0x800C0000u; /* 8008E9FC lui $v1, %hi(D_800C3EA4) */
    r_v1 = bl_lw(r_v1 + 0x3EA4u); /* 8008EA00 lw $v1, %lo(D_800C3EA4)($v1) */
    r_v0 = r_v0 << 3; /* 8008EA04 sll $v0, $v0, 3 */
    r_at = 0x10000u; /* 8008EA08 lui $at, (0x10000 >> 16) */
    r_at = r_v1 + r_at; /* 8008EA0C addu $at, $v1, $at */
    r_v1 = bl_lbu(r_at + 0xFFFFA278u); /* 8008EA10 lbu $v1, -0x5D88($at) */
    r_v0 = r_v0 + r_s1; /* 8008EA14 addu $v0, $v0, $s1 */
    r_v1 = r_v1 + 0x10u; /* 8008EA18 addiu $v1, $v1, 0x10 */
    bl_sb(r_v0 + 0x255u, r_v1); /* 8008EA1C sb $v1, 0x255($v0) */
    r_v0 = r_s0 << 1; /* 8008EA20 sll $v0, $s0, 1 */
L_8008EA24:
    r_v1 = 0x800D0000u; /* 8008EA24 lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 8008EA28 lw $v1, %lo(D_800CCB34)($v1) */
    r_s0 = r_s0 + 0x1u; /* 8008EA2C addiu $s0, $s0, 0x1 */
    r_v0 = r_v0 + r_v1; /* 8008EA30 addu $v0, $v0, $v1 */
    r_a0 = r_v0 << 2; /* 8008EA34 sll $a0, $v0, 2 */
    r_a0 = r_a0 + r_v0; /* 8008EA38 addu $a0, $a0, $v0 */
    r_a0 = r_a0 << 3; /* 8008EA3C sll $a0, $a0, 3 */
    r_a0 = r_a0 + 0x1E0u; /* 8008EA40 addiu $a0, $a0, 0x1E0 */
    /* 8008EA44 jal func_80076B00 */
    r_a0 = r_s1 + r_a0;
    bl_jal(FN, 0x80076B00u, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* func_80076B00 */
    r_v0 = (u32)((s32)r_s0 < 2); /* 8008EA4C slti $v0, $s0, 0x2 */
    /* 8008EA50 bnez $v0, .L8008EA24 */
    { int c_ = (r_v0 != 0); r_v0 = r_s0 << 1; if (c_) goto L_8008EA24; }
    r_ra = bl_lw(r_sp + 0x18u); /* 8008EA58 lw $ra, 0x18($sp) */
    r_s1 = bl_lw(r_sp + 0x14u); /* 8008EA5C lw $s1, 0x14($sp) */
    r_s0 = bl_lw(r_sp + 0x10u); /* 8008EA60 lw $s0, 0x10($sp) */
    r_sp = r_sp + 0x20u; /* 8008EA64 addiu $sp, $sp, 0x20 */
    /* 8008EA68 jr $ra */
    ; goto bl_exit;
bl_exit:
    bl_leave(bl_saved);
    (void)hi; (void)lo; (void)r_at; (void)r_v1; (void)r_k0; (void)r_k1; (void)r_gp; (void)r_ra;
    (void)r_v0;
}

/* func_8008EA70.s (398 instructions; --args 4 --void), frontier step2.  Callees through the jal service: func_80076B00. */
void func_8008EA70(u32 arg0, u32 arg1, u32 arg2, u32 arg3) {
    static const char FN[] = "func_8008EA70";
    u32 r_at = 0, r_v0 = 0, r_v1 = 0, r_a0 = 0, r_a1 = 0, r_a2 = 0, r_a3 = 0, r_t0 = 0, r_t1 = 0, r_t2 = 0, r_t3 = 0, r_t4 = 0, r_t5 = 0, r_t6 = 0, r_t7 = 0, r_s0 = 0, r_s1 = 0, r_s2 = 0, r_s3 = 0, r_s4 = 0, r_s5 = 0, r_s6 = 0, r_s7 = 0, r_t8 = 0, r_t9 = 0, r_k0 = 0, r_k1 = 0, r_gp = 0, r_sp = 0, r_fp = 0, r_ra = 0;
    u32 hi = 0, lo = 0, bl_saved;
    r_sp = bl_enter(FN, 0x20u, &bl_saved);
    r_a0 = arg0;
    r_a1 = arg1;
    r_a2 = arg2;
    r_a3 = arg3;
    r_sp = r_sp + 0xFFFFFFE0u; /* 8008EA70 addiu $sp, $sp, -0x20 */
    r_a0 = r_a0 & 0xFFu; /* 8008EA74 andi $a0, $a0, 0xFF */
    r_a0 = r_a0 << 2; /* 8008EA78 sll $a0, $a0, 2 */
    bl_sw(r_sp + 0x18u, r_ra); /* 8008EA7C sw $ra, 0x18($sp) */
    bl_sw(r_sp + 0x14u, r_s1); /* 8008EA80 sw $s1, 0x14($sp) */
    bl_sw(r_sp + 0x10u, r_s0); /* 8008EA84 sw $s0, 0x10($sp) */
    r_at = 0x800D0000u; /* 8008EA88 lui $at, %hi(D_800D2E38) */
    r_at = r_at + r_a0; /* 8008EA8C addu $at, $at, $a0 */
    r_s1 = bl_lw(r_at + 0x2E38u); /* 8008EA90 lw $s1, %lo(D_800D2E38)($at) */
    r_a0 = 0u | 0xFFF8u; /* 8008EA94 ori $a0, $zero, 0xFFF8 */
    r_v1 = 0x800D0000u; /* 8008EA98 lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 8008EA9C lw $v1, %lo(D_800CCB34)($v1) */
    r_a0 = r_a1 + r_a0; /* 8008EAA0 addu $a0, $a1, $a0 */
    r_v0 = r_v1 << 2; /* 8008EAA4 sll $v0, $v1, 2 */
    r_v0 = r_v0 + r_v1; /* 8008EAA8 addu $v0, $v0, $v1 */
    r_v0 = r_v0 << 3; /* 8008EAAC sll $v0, $v0, 3 */
    r_v0 = r_v0 + r_s1; /* 8008EAB0 addu $v0, $v0, $s1 */
    bl_sh(r_v0 + 0x288u, r_a0); /* 8008EAB4 sh $a0, 0x288($v0) */
    r_v1 = 0x800D0000u; /* 8008EAB8 lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 8008EABC lw $v1, %lo(D_800CCB34)($v1) */
    r_a2 = r_a2 + 0x8u; /* 8008EAC0 addiu $a2, $a2, 0x8 */
    r_v0 = r_v1 << 2; /* 8008EAC4 sll $v0, $v1, 2 */
    r_v0 = r_v0 + r_v1; /* 8008EAC8 addu $v0, $v0, $v1 */
    r_v0 = r_v0 << 3; /* 8008EACC sll $v0, $v0, 3 */
    r_v0 = r_v0 + r_s1; /* 8008EAD0 addu $v0, $v0, $s1 */
    bl_sh(r_v0 + 0x28Au, r_a2); /* 8008EAD4 sh $a2, 0x28A($v0) */
    r_v1 = 0x800D0000u; /* 8008EAD8 lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 8008EADC lw $v1, %lo(D_800CCB34)($v1) */
    r_a1 = r_a1 + 0x8u; /* 8008EAE0 addiu $a1, $a1, 0x8 */
    r_v0 = r_v1 << 2; /* 8008EAE4 sll $v0, $v1, 2 */
    r_v0 = r_v0 + r_v1; /* 8008EAE8 addu $v0, $v0, $v1 */
    r_v0 = r_v0 << 3; /* 8008EAEC sll $v0, $v0, 3 */
    r_v0 = r_v0 + r_s1; /* 8008EAF0 addu $v0, $v0, $s1 */
    bl_sh(r_v0 + 0x290u, r_a1); /* 8008EAF4 sh $a1, 0x290($v0) */
    r_v1 = 0x800D0000u; /* 8008EAF8 lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 8008EAFC lw $v1, %lo(D_800CCB34)($v1) */
    ; /* 8008EB00 nop  */
    r_v0 = r_v1 << 2; /* 8008EB04 sll $v0, $v1, 2 */
    r_v0 = r_v0 + r_v1; /* 8008EB08 addu $v0, $v0, $v1 */
    r_v0 = r_v0 << 3; /* 8008EB0C sll $v0, $v0, 3 */
    r_v0 = r_v0 + r_s1; /* 8008EB10 addu $v0, $v0, $s1 */
    bl_sh(r_v0 + 0x292u, r_a2); /* 8008EB14 sh $a2, 0x292($v0) */
    r_v1 = 0x800D0000u; /* 8008EB18 lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 8008EB1C lw $v1, %lo(D_800CCB34)($v1) */
    r_a3 = r_a3 & 0xFFFFu; /* 8008EB20 andi $a3, $a3, 0xFFFF */
    r_v0 = r_v1 << 2; /* 8008EB24 sll $v0, $v1, 2 */
    r_v0 = r_v0 + r_v1; /* 8008EB28 addu $v0, $v0, $v1 */
    r_v0 = r_v0 << 3; /* 8008EB2C sll $v0, $v0, 3 */
    r_v0 = r_v0 + r_s1; /* 8008EB30 addu $v0, $v0, $s1 */
    bl_sh(r_v0 + 0x298u, r_a0); /* 8008EB34 sh $a0, 0x298($v0) */
    r_v1 = 0x800D0000u; /* 8008EB38 lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 8008EB3C lw $v1, %lo(D_800CCB34)($v1) */
    r_a3 = r_a3 + 0xFFFFFFF0u; /* 8008EB40 addiu $a3, $a3, -0x10 */
    r_v0 = r_v1 << 2; /* 8008EB44 sll $v0, $v1, 2 */
    r_v0 = r_v0 + r_v1; /* 8008EB48 addu $v0, $v0, $v1 */
    r_v0 = r_v0 << 3; /* 8008EB4C sll $v0, $v0, 3 */
    r_v0 = r_v0 + r_s1; /* 8008EB50 addu $v0, $v0, $s1 */
    r_v1 = r_a3 >> 31; /* 8008EB54 srl $v1, $a3, 31 */
    r_a3 = r_a3 + r_v1; /* 8008EB58 addu $a3, $a3, $v1 */
    r_a3 = BL_SRA(r_a3, 1); /* 8008EB5C sra $a3, $a3, 1 */
    r_a2 = r_a2 + r_a3; /* 8008EB60 addu $a2, $a2, $a3 */
    bl_sh(r_v0 + 0x29Au, r_a2); /* 8008EB64 sh $a2, 0x29A($v0) */
    r_v1 = 0x800D0000u; /* 8008EB68 lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 8008EB6C lw $v1, %lo(D_800CCB34)($v1) */
    ; /* 8008EB70 nop  */
    r_v0 = r_v1 << 2; /* 8008EB74 sll $v0, $v1, 2 */
    r_v0 = r_v0 + r_v1; /* 8008EB78 addu $v0, $v0, $v1 */
    r_v0 = r_v0 << 3; /* 8008EB7C sll $v0, $v0, 3 */
    r_v0 = r_v0 + r_s1; /* 8008EB80 addu $v0, $v0, $s1 */
    bl_sh(r_v0 + 0x2A0u, r_a1); /* 8008EB84 sh $a1, 0x2A0($v0) */
    r_v1 = 0x800D0000u; /* 8008EB88 lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 8008EB8C lw $v1, %lo(D_800CCB34)($v1) */
    ; /* 8008EB90 nop  */
    r_v0 = r_v1 << 2; /* 8008EB94 sll $v0, $v1, 2 */
    r_v0 = r_v0 + r_v1; /* 8008EB98 addu $v0, $v0, $v1 */
    r_v0 = r_v0 << 3; /* 8008EB9C sll $v0, $v0, 3 */
    r_v0 = r_v0 + r_s1; /* 8008EBA0 addu $v0, $v0, $s1 */
    bl_sh(r_v0 + 0x2A2u, r_a2); /* 8008EBA4 sh $a2, 0x2A2($v0) */
    r_v1 = 0x800D0000u; /* 8008EBA8 lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 8008EBAC lw $v1, %lo(D_800CCB34)($v1) */
    ; /* 8008EBB0 nop  */
    r_v0 = r_v1 << 2; /* 8008EBB4 sll $v0, $v1, 2 */
    r_v0 = r_v0 + r_v1; /* 8008EBB8 addu $v0, $v0, $v1 */
    r_v0 = r_v0 << 3; /* 8008EBBC sll $v0, $v0, 3 */
    r_v0 = r_v0 + r_s1; /* 8008EBC0 addu $v0, $v0, $s1 */
    bl_sh(r_v0 + 0x2D8u, r_a0); /* 8008EBC4 sh $a0, 0x2D8($v0) */
    r_v1 = 0x800D0000u; /* 8008EBC8 lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 8008EBCC lw $v1, %lo(D_800CCB34)($v1) */
    ; /* 8008EBD0 nop  */
    r_v0 = r_v1 << 2; /* 8008EBD4 sll $v0, $v1, 2 */
    r_v0 = r_v0 + r_v1; /* 8008EBD8 addu $v0, $v0, $v1 */
    r_v0 = r_v0 << 3; /* 8008EBDC sll $v0, $v0, 3 */
    r_v0 = r_v0 + r_s1; /* 8008EBE0 addu $v0, $v0, $s1 */
    bl_sh(r_v0 + 0x2DAu, r_a2); /* 8008EBE4 sh $a2, 0x2DA($v0) */
    r_v1 = 0x800D0000u; /* 8008EBE8 lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 8008EBEC lw $v1, %lo(D_800CCB34)($v1) */
    ; /* 8008EBF0 nop  */
    r_v0 = r_v1 << 2; /* 8008EBF4 sll $v0, $v1, 2 */
    r_v0 = r_v0 + r_v1; /* 8008EBF8 addu $v0, $v0, $v1 */
    r_v0 = r_v0 << 3; /* 8008EBFC sll $v0, $v0, 3 */
    r_v0 = r_v0 + r_s1; /* 8008EC00 addu $v0, $v0, $s1 */
    bl_sh(r_v0 + 0x2E0u, r_a1); /* 8008EC04 sh $a1, 0x2E0($v0) */
    r_v1 = 0x800D0000u; /* 8008EC08 lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 8008EC0C lw $v1, %lo(D_800CCB34)($v1) */
    ; /* 8008EC10 nop  */
    r_v0 = r_v1 << 2; /* 8008EC14 sll $v0, $v1, 2 */
    r_v0 = r_v0 + r_v1; /* 8008EC18 addu $v0, $v0, $v1 */
    r_v0 = r_v0 << 3; /* 8008EC1C sll $v0, $v0, 3 */
    r_v0 = r_v0 + r_s1; /* 8008EC20 addu $v0, $v0, $s1 */
    bl_sh(r_v0 + 0x2E2u, r_a2); /* 8008EC24 sh $a2, 0x2E2($v0) */
    r_v1 = 0x800D0000u; /* 8008EC28 lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 8008EC2C lw $v1, %lo(D_800CCB34)($v1) */
    ; /* 8008EC30 nop  */
    r_v0 = r_v1 << 2; /* 8008EC34 sll $v0, $v1, 2 */
    r_v0 = r_v0 + r_v1; /* 8008EC38 addu $v0, $v0, $v1 */
    r_v0 = r_v0 << 3; /* 8008EC3C sll $v0, $v0, 3 */
    r_v0 = r_v0 + r_s1; /* 8008EC40 addu $v0, $v0, $s1 */
    bl_sh(r_v0 + 0x2E8u, r_a0); /* 8008EC44 sh $a0, 0x2E8($v0) */
    r_v1 = 0x800D0000u; /* 8008EC48 lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 8008EC4C lw $v1, %lo(D_800CCB34)($v1) */
    r_a2 = r_a2 + r_a3; /* 8008EC50 addu $a2, $a2, $a3 */
    r_v0 = r_v1 << 2; /* 8008EC54 sll $v0, $v1, 2 */
    r_v0 = r_v0 + r_v1; /* 8008EC58 addu $v0, $v0, $v1 */
    r_v0 = r_v0 << 3; /* 8008EC5C sll $v0, $v0, 3 */
    r_v0 = r_v0 + r_s1; /* 8008EC60 addu $v0, $v0, $s1 */
    bl_sh(r_v0 + 0x2EAu, r_a2); /* 8008EC64 sh $a2, 0x2EA($v0) */
    r_v1 = 0x800D0000u; /* 8008EC68 lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 8008EC6C lw $v1, %lo(D_800CCB34)($v1) */
    ; /* 8008EC70 nop  */
    r_v0 = r_v1 << 2; /* 8008EC74 sll $v0, $v1, 2 */
    r_v0 = r_v0 + r_v1; /* 8008EC78 addu $v0, $v0, $v1 */
    r_v0 = r_v0 << 3; /* 8008EC7C sll $v0, $v0, 3 */
    r_v0 = r_v0 + r_s1; /* 8008EC80 addu $v0, $v0, $s1 */
    bl_sh(r_v0 + 0x2F0u, r_a1); /* 8008EC84 sh $a1, 0x2F0($v0) */
    r_v1 = 0x800D0000u; /* 8008EC88 lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 8008EC8C lw $v1, %lo(D_800CCB34)($v1) */
    ; /* 8008EC90 nop  */
    r_v0 = r_v1 << 2; /* 8008EC94 sll $v0, $v1, 2 */
    r_v0 = r_v0 + r_v1; /* 8008EC98 addu $v0, $v0, $v1 */
    r_v0 = r_v0 << 3; /* 8008EC9C sll $v0, $v0, 3 */
    r_v0 = r_v0 + r_s1; /* 8008ECA0 addu $v0, $v0, $s1 */
    bl_sh(r_v0 + 0x2F2u, r_a2); /* 8008ECA4 sh $a2, 0x2F2($v0) */
    r_v0 = 0x800D0000u; /* 8008ECA8 lui $v0, %hi(D_800CCB34) */
    r_v0 = bl_lw(r_v0 + 0xFFFFCB34u); /* 8008ECAC lw $v0, %lo(D_800CCB34)($v0) */
    ; /* 8008ECB0 nop  */
    r_v1 = r_v0 << 2; /* 8008ECB4 sll $v1, $v0, 2 */
    r_v1 = r_v1 + r_v0; /* 8008ECB8 addu $v1, $v1, $v0 */
    r_v0 = 0x800C0000u; /* 8008ECBC lui $v0, %hi(D_800C3EA4) */
    r_v0 = bl_lw(r_v0 + 0x3EA4u); /* 8008ECC0 lw $v0, %lo(D_800C3EA4)($v0) */
    r_v1 = r_v1 << 3; /* 8008ECC4 sll $v1, $v1, 3 */
    r_at = 0x10000u; /* 8008ECC8 lui $at, (0x10000 >> 16) */
    r_at = r_v0 + r_at; /* 8008ECCC addu $at, $v0, $at */
    r_v0 = bl_lbu(r_at + 0xFFFFA28Cu); /* 8008ECD0 lbu $v0, -0x5D74($at) */
    r_v1 = r_v1 + r_s1; /* 8008ECD4 addu $v1, $v1, $s1 */
    r_v0 = r_v0 & 0x3Fu; /* 8008ECD8 andi $v0, $v0, 0x3F */
    r_v0 = r_v0 << 1; /* 8008ECDC sll $v0, $v0, 1 */
    r_v0 = r_v0 + 0xEu; /* 8008ECE0 addiu $v0, $v0, 0xE */
    bl_sb(r_v1 + 0x28Cu, r_v0); /* 8008ECE4 sb $v0, 0x28C($v1) */
    r_v1 = 0x800D0000u; /* 8008ECE8 lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 8008ECEC lw $v1, %lo(D_800CCB34)($v1) */
    ; /* 8008ECF0 nop  */
    r_v0 = r_v1 << 2; /* 8008ECF4 sll $v0, $v1, 2 */
    r_v0 = r_v0 + r_v1; /* 8008ECF8 addu $v0, $v0, $v1 */
    r_v1 = 0x800C0000u; /* 8008ECFC lui $v1, %hi(D_800C3EA4) */
    r_v1 = bl_lw(r_v1 + 0x3EA4u); /* 8008ED00 lw $v1, %lo(D_800C3EA4)($v1) */
    r_v0 = r_v0 << 3; /* 8008ED04 sll $v0, $v0, 3 */
    r_at = 0x10000u; /* 8008ED08 lui $at, (0x10000 >> 16) */
    r_at = r_v1 + r_at; /* 8008ED0C addu $at, $v1, $at */
    r_v1 = bl_lbu(r_at + 0xFFFFA290u); /* 8008ED10 lbu $v1, -0x5D70($at) */
    r_v0 = r_v0 + r_s1; /* 8008ED14 addu $v0, $v0, $s1 */
    bl_sb(r_v0 + 0x28Du, r_v1); /* 8008ED18 sb $v1, 0x28D($v0) */
    r_v0 = 0x800D0000u; /* 8008ED1C lui $v0, %hi(D_800CCB34) */
    r_v0 = bl_lw(r_v0 + 0xFFFFCB34u); /* 8008ED20 lw $v0, %lo(D_800CCB34)($v0) */
    ; /* 8008ED24 nop  */
    r_v1 = r_v0 << 2; /* 8008ED28 sll $v1, $v0, 2 */
    r_v1 = r_v1 + r_v0; /* 8008ED2C addu $v1, $v1, $v0 */
    r_v0 = 0x800C0000u; /* 8008ED30 lui $v0, %hi(D_800C3EA4) */
    r_v0 = bl_lw(r_v0 + 0x3EA4u); /* 8008ED34 lw $v0, %lo(D_800C3EA4)($v0) */
    r_v1 = r_v1 << 3; /* 8008ED38 sll $v1, $v1, 3 */
    r_at = 0x10000u; /* 8008ED3C lui $at, (0x10000 >> 16) */
    r_at = r_v0 + r_at; /* 8008ED40 addu $at, $v0, $at */
    r_v0 = bl_lbu(r_at + 0xFFFFA28Cu); /* 8008ED44 lbu $v0, -0x5D74($at) */
    r_v1 = r_v1 + r_s1; /* 8008ED48 addu $v1, $v1, $s1 */
    r_v0 = r_v0 & 0x3Fu; /* 8008ED4C andi $v0, $v0, 0x3F */
    r_v0 = r_v0 << 1; /* 8008ED50 sll $v0, $v0, 1 */
    r_v0 = r_v0 + 0x1Eu; /* 8008ED54 addiu $v0, $v0, 0x1E */
    bl_sb(r_v1 + 0x294u, r_v0); /* 8008ED58 sb $v0, 0x294($v1) */
    r_v1 = 0x800D0000u; /* 8008ED5C lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 8008ED60 lw $v1, %lo(D_800CCB34)($v1) */
    ; /* 8008ED64 nop  */
    r_v0 = r_v1 << 2; /* 8008ED68 sll $v0, $v1, 2 */
    r_v0 = r_v0 + r_v1; /* 8008ED6C addu $v0, $v0, $v1 */
    r_v1 = 0x800C0000u; /* 8008ED70 lui $v1, %hi(D_800C3EA4) */
    r_v1 = bl_lw(r_v1 + 0x3EA4u); /* 8008ED74 lw $v1, %lo(D_800C3EA4)($v1) */
    r_v0 = r_v0 << 3; /* 8008ED78 sll $v0, $v0, 3 */
    r_at = 0x10000u; /* 8008ED7C lui $at, (0x10000 >> 16) */
    r_at = r_v1 + r_at; /* 8008ED80 addu $at, $v1, $at */
    r_v1 = bl_lbu(r_at + 0xFFFFA290u); /* 8008ED84 lbu $v1, -0x5D70($at) */
    r_v0 = r_v0 + r_s1; /* 8008ED88 addu $v0, $v0, $s1 */
    bl_sb(r_v0 + 0x295u, r_v1); /* 8008ED8C sb $v1, 0x295($v0) */
    r_v0 = 0x800D0000u; /* 8008ED90 lui $v0, %hi(D_800CCB34) */
    r_v0 = bl_lw(r_v0 + 0xFFFFCB34u); /* 8008ED94 lw $v0, %lo(D_800CCB34)($v0) */
    ; /* 8008ED98 nop  */
    r_v1 = r_v0 << 2; /* 8008ED9C sll $v1, $v0, 2 */
    r_v1 = r_v1 + r_v0; /* 8008EDA0 addu $v1, $v1, $v0 */
    r_v0 = 0x800C0000u; /* 8008EDA4 lui $v0, %hi(D_800C3EA4) */
    r_v0 = bl_lw(r_v0 + 0x3EA4u); /* 8008EDA8 lw $v0, %lo(D_800C3EA4)($v0) */
    r_v1 = r_v1 << 3; /* 8008EDAC sll $v1, $v1, 3 */
    r_at = 0x10000u; /* 8008EDB0 lui $at, (0x10000 >> 16) */
    r_at = r_v0 + r_at; /* 8008EDB4 addu $at, $v0, $at */
    r_v0 = bl_lbu(r_at + 0xFFFFA28Cu); /* 8008EDB8 lbu $v0, -0x5D74($at) */
    r_v1 = r_v1 + r_s1; /* 8008EDBC addu $v1, $v1, $s1 */
    r_v0 = r_v0 & 0x3Fu; /* 8008EDC0 andi $v0, $v0, 0x3F */
    r_v0 = r_v0 << 1; /* 8008EDC4 sll $v0, $v0, 1 */
    r_v0 = r_v0 + 0xEu; /* 8008EDC8 addiu $v0, $v0, 0xE */
    bl_sb(r_v1 + 0x29Cu, r_v0); /* 8008EDCC sb $v0, 0x29C($v1) */
    r_v1 = 0x800D0000u; /* 8008EDD0 lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 8008EDD4 lw $v1, %lo(D_800CCB34)($v1) */
    ; /* 8008EDD8 nop  */
    r_v0 = r_v1 << 2; /* 8008EDDC sll $v0, $v1, 2 */
    r_v0 = r_v0 + r_v1; /* 8008EDE0 addu $v0, $v0, $v1 */
    r_v1 = 0x800C0000u; /* 8008EDE4 lui $v1, %hi(D_800C3EA4) */
    r_v1 = bl_lw(r_v1 + 0x3EA4u); /* 8008EDE8 lw $v1, %lo(D_800C3EA4)($v1) */
    r_v0 = r_v0 << 3; /* 8008EDEC sll $v0, $v0, 3 */
    r_at = 0x10000u; /* 8008EDF0 lui $at, (0x10000 >> 16) */
    r_at = r_v1 + r_at; /* 8008EDF4 addu $at, $v1, $at */
    r_v1 = bl_lbu(r_at + 0xFFFFA290u); /* 8008EDF8 lbu $v1, -0x5D70($at) */
    r_v0 = r_v0 + r_s1; /* 8008EDFC addu $v0, $v0, $s1 */
    r_v1 = r_v1 + 0x7u; /* 8008EE00 addiu $v1, $v1, 0x7 */
    bl_sb(r_v0 + 0x29Du, r_v1); /* 8008EE04 sb $v1, 0x29D($v0) */
    r_v0 = 0x800D0000u; /* 8008EE08 lui $v0, %hi(D_800CCB34) */
    r_v0 = bl_lw(r_v0 + 0xFFFFCB34u); /* 8008EE0C lw $v0, %lo(D_800CCB34)($v0) */
    ; /* 8008EE10 nop  */
    r_v1 = r_v0 << 2; /* 8008EE14 sll $v1, $v0, 2 */
    r_v1 = r_v1 + r_v0; /* 8008EE18 addu $v1, $v1, $v0 */
    r_v0 = 0x800C0000u; /* 8008EE1C lui $v0, %hi(D_800C3EA4) */
    r_v0 = bl_lw(r_v0 + 0x3EA4u); /* 8008EE20 lw $v0, %lo(D_800C3EA4)($v0) */
    r_v1 = r_v1 << 3; /* 8008EE24 sll $v1, $v1, 3 */
    r_at = 0x10000u; /* 8008EE28 lui $at, (0x10000 >> 16) */
    r_at = r_v0 + r_at; /* 8008EE2C addu $at, $v0, $at */
    r_v0 = bl_lbu(r_at + 0xFFFFA28Cu); /* 8008EE30 lbu $v0, -0x5D74($at) */
    r_v1 = r_v1 + r_s1; /* 8008EE34 addu $v1, $v1, $s1 */
    r_v0 = r_v0 & 0x3Fu; /* 8008EE38 andi $v0, $v0, 0x3F */
    r_v0 = r_v0 << 1; /* 8008EE3C sll $v0, $v0, 1 */
    r_v0 = r_v0 + 0x1Eu; /* 8008EE40 addiu $v0, $v0, 0x1E */
    bl_sb(r_v1 + 0x2A4u, r_v0); /* 8008EE44 sb $v0, 0x2A4($v1) */
    r_v1 = 0x800D0000u; /* 8008EE48 lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 8008EE4C lw $v1, %lo(D_800CCB34)($v1) */
    ; /* 8008EE50 nop  */
    r_v0 = r_v1 << 2; /* 8008EE54 sll $v0, $v1, 2 */
    r_v0 = r_v0 + r_v1; /* 8008EE58 addu $v0, $v0, $v1 */
    r_v1 = 0x800C0000u; /* 8008EE5C lui $v1, %hi(D_800C3EA4) */
    r_v1 = bl_lw(r_v1 + 0x3EA4u); /* 8008EE60 lw $v1, %lo(D_800C3EA4)($v1) */
    r_v0 = r_v0 << 3; /* 8008EE64 sll $v0, $v0, 3 */
    r_at = 0x10000u; /* 8008EE68 lui $at, (0x10000 >> 16) */
    r_at = r_v1 + r_at; /* 8008EE6C addu $at, $v1, $at */
    r_v1 = bl_lbu(r_at + 0xFFFFA290u); /* 8008EE70 lbu $v1, -0x5D70($at) */
    r_v0 = r_v0 + r_s1; /* 8008EE74 addu $v0, $v0, $s1 */
    r_v1 = r_v1 + 0x7u; /* 8008EE78 addiu $v1, $v1, 0x7 */
    bl_sb(r_v0 + 0x2A5u, r_v1); /* 8008EE7C sb $v1, 0x2A5($v0) */
    r_v0 = 0x800D0000u; /* 8008EE80 lui $v0, %hi(D_800CCB34) */
    r_v0 = bl_lw(r_v0 + 0xFFFFCB34u); /* 8008EE84 lw $v0, %lo(D_800CCB34)($v0) */
    ; /* 8008EE88 nop  */
    r_v1 = r_v0 << 2; /* 8008EE8C sll $v1, $v0, 2 */
    r_v1 = r_v1 + r_v0; /* 8008EE90 addu $v1, $v1, $v0 */
    r_v0 = 0x800C0000u; /* 8008EE94 lui $v0, %hi(D_800C3EA4) */
    r_v0 = bl_lw(r_v0 + 0x3EA4u); /* 8008EE98 lw $v0, %lo(D_800C3EA4)($v0) */
    r_v1 = r_v1 << 3; /* 8008EE9C sll $v1, $v1, 3 */
    r_at = 0x10000u; /* 8008EEA0 lui $at, (0x10000 >> 16) */
    r_at = r_v0 + r_at; /* 8008EEA4 addu $at, $v0, $at */
    r_v0 = bl_lbu(r_at + 0xFFFFA28Cu); /* 8008EEA8 lbu $v0, -0x5D74($at) */
    r_v1 = r_v1 + r_s1; /* 8008EEAC addu $v1, $v1, $s1 */
    r_v0 = r_v0 & 0x3Fu; /* 8008EEB0 andi $v0, $v0, 0x3F */
    r_v0 = r_v0 << 1; /* 8008EEB4 sll $v0, $v0, 1 */
    r_v0 = r_v0 + 0xEu; /* 8008EEB8 addiu $v0, $v0, 0xE */
    bl_sb(r_v1 + 0x2DCu, r_v0); /* 8008EEBC sb $v0, 0x2DC($v1) */
    r_v1 = 0x800D0000u; /* 8008EEC0 lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 8008EEC4 lw $v1, %lo(D_800CCB34)($v1) */
    ; /* 8008EEC8 nop  */
    r_v0 = r_v1 << 2; /* 8008EECC sll $v0, $v1, 2 */
    r_v0 = r_v0 + r_v1; /* 8008EED0 addu $v0, $v0, $v1 */
    r_v1 = 0x800C0000u; /* 8008EED4 lui $v1, %hi(D_800C3EA4) */
    r_v1 = bl_lw(r_v1 + 0x3EA4u); /* 8008EED8 lw $v1, %lo(D_800C3EA4)($v1) */
    r_v0 = r_v0 << 3; /* 8008EEDC sll $v0, $v0, 3 */
    r_at = 0x10000u; /* 8008EEE0 lui $at, (0x10000 >> 16) */
    r_at = r_v1 + r_at; /* 8008EEE4 addu $at, $v1, $at */
    r_v1 = bl_lbu(r_at + 0xFFFFA290u); /* 8008EEE8 lbu $v1, -0x5D70($at) */
    r_v0 = r_v0 + r_s1; /* 8008EEEC addu $v0, $v0, $s1 */
    bl_sb(r_v0 + 0x2DDu, r_v1); /* 8008EEF0 sb $v1, 0x2DD($v0) */
    r_v0 = 0x800D0000u; /* 8008EEF4 lui $v0, %hi(D_800CCB34) */
    r_v0 = bl_lw(r_v0 + 0xFFFFCB34u); /* 8008EEF8 lw $v0, %lo(D_800CCB34)($v0) */
    ; /* 8008EEFC nop  */
    r_v1 = r_v0 << 2; /* 8008EF00 sll $v1, $v0, 2 */
    r_v1 = r_v1 + r_v0; /* 8008EF04 addu $v1, $v1, $v0 */
    r_v0 = 0x800C0000u; /* 8008EF08 lui $v0, %hi(D_800C3EA4) */
    r_v0 = bl_lw(r_v0 + 0x3EA4u); /* 8008EF0C lw $v0, %lo(D_800C3EA4)($v0) */
    r_v1 = r_v1 << 3; /* 8008EF10 sll $v1, $v1, 3 */
    r_at = 0x10000u; /* 8008EF14 lui $at, (0x10000 >> 16) */
    r_at = r_v0 + r_at; /* 8008EF18 addu $at, $v0, $at */
    r_v0 = bl_lbu(r_at + 0xFFFFA28Cu); /* 8008EF1C lbu $v0, -0x5D74($at) */
    r_v1 = r_v1 + r_s1; /* 8008EF20 addu $v1, $v1, $s1 */
    r_v0 = r_v0 & 0x3Fu; /* 8008EF24 andi $v0, $v0, 0x3F */
    r_v0 = r_v0 << 1; /* 8008EF28 sll $v0, $v0, 1 */
    r_v0 = r_v0 + 0x1Eu; /* 8008EF2C addiu $v0, $v0, 0x1E */
    bl_sb(r_v1 + 0x2E4u, r_v0); /* 8008EF30 sb $v0, 0x2E4($v1) */
    r_v1 = 0x800D0000u; /* 8008EF34 lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 8008EF38 lw $v1, %lo(D_800CCB34)($v1) */
    ; /* 8008EF3C nop  */
    r_v0 = r_v1 << 2; /* 8008EF40 sll $v0, $v1, 2 */
    r_v0 = r_v0 + r_v1; /* 8008EF44 addu $v0, $v0, $v1 */
    r_v1 = 0x800C0000u; /* 8008EF48 lui $v1, %hi(D_800C3EA4) */
    r_v1 = bl_lw(r_v1 + 0x3EA4u); /* 8008EF4C lw $v1, %lo(D_800C3EA4)($v1) */
    r_v0 = r_v0 << 3; /* 8008EF50 sll $v0, $v0, 3 */
    r_at = 0x10000u; /* 8008EF54 lui $at, (0x10000 >> 16) */
    r_at = r_v1 + r_at; /* 8008EF58 addu $at, $v1, $at */
    r_v1 = bl_lbu(r_at + 0xFFFFA290u); /* 8008EF5C lbu $v1, -0x5D70($at) */
    r_v0 = r_v0 + r_s1; /* 8008EF60 addu $v0, $v0, $s1 */
    bl_sb(r_v0 + 0x2E5u, r_v1); /* 8008EF64 sb $v1, 0x2E5($v0) */
    r_v0 = 0x800D0000u; /* 8008EF68 lui $v0, %hi(D_800CCB34) */
    r_v0 = bl_lw(r_v0 + 0xFFFFCB34u); /* 8008EF6C lw $v0, %lo(D_800CCB34)($v0) */
    ; /* 8008EF70 nop  */
    r_v1 = r_v0 << 2; /* 8008EF74 sll $v1, $v0, 2 */
    r_v1 = r_v1 + r_v0; /* 8008EF78 addu $v1, $v1, $v0 */
    r_v0 = 0x800C0000u; /* 8008EF7C lui $v0, %hi(D_800C3EA4) */
    r_v0 = bl_lw(r_v0 + 0x3EA4u); /* 8008EF80 lw $v0, %lo(D_800C3EA4)($v0) */
    r_v1 = r_v1 << 3; /* 8008EF84 sll $v1, $v1, 3 */
    r_at = 0x10000u; /* 8008EF88 lui $at, (0x10000 >> 16) */
    r_at = r_v0 + r_at; /* 8008EF8C addu $at, $v0, $at */
    r_v0 = bl_lbu(r_at + 0xFFFFA28Cu); /* 8008EF90 lbu $v0, -0x5D74($at) */
    r_v1 = r_v1 + r_s1; /* 8008EF94 addu $v1, $v1, $s1 */
    r_v0 = r_v0 & 0x3Fu; /* 8008EF98 andi $v0, $v0, 0x3F */
    r_v0 = r_v0 << 1; /* 8008EF9C sll $v0, $v0, 1 */
    r_v0 = r_v0 + 0xEu; /* 8008EFA0 addiu $v0, $v0, 0xE */
    bl_sb(r_v1 + 0x2ECu, r_v0); /* 8008EFA4 sb $v0, 0x2EC($v1) */
    r_v1 = 0x800D0000u; /* 8008EFA8 lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 8008EFAC lw $v1, %lo(D_800CCB34)($v1) */
    ; /* 8008EFB0 nop  */
    r_v0 = r_v1 << 2; /* 8008EFB4 sll $v0, $v1, 2 */
    r_v0 = r_v0 + r_v1; /* 8008EFB8 addu $v0, $v0, $v1 */
    r_v1 = 0x800C0000u; /* 8008EFBC lui $v1, %hi(D_800C3EA4) */
    r_v1 = bl_lw(r_v1 + 0x3EA4u); /* 8008EFC0 lw $v1, %lo(D_800C3EA4)($v1) */
    r_v0 = r_v0 << 3; /* 8008EFC4 sll $v0, $v0, 3 */
    r_at = 0x10000u; /* 8008EFC8 lui $at, (0x10000 >> 16) */
    r_at = r_v1 + r_at; /* 8008EFCC addu $at, $v1, $at */
    r_v1 = bl_lbu(r_at + 0xFFFFA290u); /* 8008EFD0 lbu $v1, -0x5D70($at) */
    r_v0 = r_v0 + r_s1; /* 8008EFD4 addu $v0, $v0, $s1 */
    r_v1 = r_v1 + 0x7u; /* 8008EFD8 addiu $v1, $v1, 0x7 */
    bl_sb(r_v0 + 0x2EDu, r_v1); /* 8008EFDC sb $v1, 0x2ED($v0) */
    r_v0 = 0x800D0000u; /* 8008EFE0 lui $v0, %hi(D_800CCB34) */
    r_v0 = bl_lw(r_v0 + 0xFFFFCB34u); /* 8008EFE4 lw $v0, %lo(D_800CCB34)($v0) */
    ; /* 8008EFE8 nop  */
    r_v1 = r_v0 << 2; /* 8008EFEC sll $v1, $v0, 2 */
    r_v1 = r_v1 + r_v0; /* 8008EFF0 addu $v1, $v1, $v0 */
    r_v0 = 0x800C0000u; /* 8008EFF4 lui $v0, %hi(D_800C3EA4) */
    r_v0 = bl_lw(r_v0 + 0x3EA4u); /* 8008EFF8 lw $v0, %lo(D_800C3EA4)($v0) */
    r_v1 = r_v1 << 3; /* 8008EFFC sll $v1, $v1, 3 */
    r_at = 0x10000u; /* 8008F000 lui $at, (0x10000 >> 16) */
    r_at = r_v0 + r_at; /* 8008F004 addu $at, $v0, $at */
    r_v0 = bl_lbu(r_at + 0xFFFFA28Cu); /* 8008F008 lbu $v0, -0x5D74($at) */
    r_v1 = r_v1 + r_s1; /* 8008F00C addu $v1, $v1, $s1 */
    r_v0 = r_v0 & 0x3Fu; /* 8008F010 andi $v0, $v0, 0x3F */
    r_v0 = r_v0 << 1; /* 8008F014 sll $v0, $v0, 1 */
    r_v0 = r_v0 + 0x1Eu; /* 8008F018 addiu $v0, $v0, 0x1E */
    bl_sb(r_v1 + 0x2F4u, r_v0); /* 8008F01C sb $v0, 0x2F4($v1) */
    r_v1 = 0x800D0000u; /* 8008F020 lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 8008F024 lw $v1, %lo(D_800CCB34)($v1) */
    r_s0 = 0u + 0u; /* 8008F028 addu $s0, $zero, $zero */
    r_v0 = r_v1 << 2; /* 8008F02C sll $v0, $v1, 2 */
    r_v0 = r_v0 + r_v1; /* 8008F030 addu $v0, $v0, $v1 */
    r_v1 = 0x800C0000u; /* 8008F034 lui $v1, %hi(D_800C3EA4) */
    r_v1 = bl_lw(r_v1 + 0x3EA4u); /* 8008F038 lw $v1, %lo(D_800C3EA4)($v1) */
    r_v0 = r_v0 << 3; /* 8008F03C sll $v0, $v0, 3 */
    r_at = 0x10000u; /* 8008F040 lui $at, (0x10000 >> 16) */
    r_at = r_v1 + r_at; /* 8008F044 addu $at, $v1, $at */
    r_v1 = bl_lbu(r_at + 0xFFFFA290u); /* 8008F048 lbu $v1, -0x5D70($at) */
    r_v0 = r_v0 + r_s1; /* 8008F04C addu $v0, $v0, $s1 */
    r_v1 = r_v1 + 0x7u; /* 8008F050 addiu $v1, $v1, 0x7 */
    bl_sb(r_v0 + 0x2F5u, r_v1); /* 8008F054 sb $v1, 0x2F5($v0) */
    r_v0 = r_s0 << 1; /* 8008F058 sll $v0, $s0, 1 */
L_8008F05C:
    r_v1 = 0x800D0000u; /* 8008F05C lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 8008F060 lw $v1, %lo(D_800CCB34)($v1) */
    r_s0 = r_s0 + 0x1u; /* 8008F064 addiu $s0, $s0, 0x1 */
    r_v0 = r_v0 + r_v1; /* 8008F068 addu $v0, $v0, $v1 */
    r_a0 = r_v0 << 2; /* 8008F06C sll $a0, $v0, 2 */
    r_a0 = r_a0 + r_v0; /* 8008F070 addu $a0, $a0, $v0 */
    r_a0 = r_a0 << 3; /* 8008F074 sll $a0, $a0, 3 */
    r_a0 = r_a0 + 0x280u; /* 8008F078 addiu $a0, $a0, 0x280 */
    /* 8008F07C jal func_80076B00 */
    r_a0 = r_s1 + r_a0;
    bl_jal(FN, 0x80076B00u, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* func_80076B00 */
    r_v0 = (u32)((s32)r_s0 < 2); /* 8008F084 slti $v0, $s0, 0x2 */
    /* 8008F088 bnez $v0, .L8008F05C */
    { int c_ = (r_v0 != 0); r_v0 = r_s0 << 1; if (c_) goto L_8008F05C; }
    r_ra = bl_lw(r_sp + 0x18u); /* 8008F090 lw $ra, 0x18($sp) */
    r_s1 = bl_lw(r_sp + 0x14u); /* 8008F094 lw $s1, 0x14($sp) */
    r_s0 = bl_lw(r_sp + 0x10u); /* 8008F098 lw $s0, 0x10($sp) */
    r_sp = r_sp + 0x20u; /* 8008F09C addiu $sp, $sp, 0x20 */
    /* 8008F0A0 jr $ra */
    ; goto bl_exit;
bl_exit:
    bl_leave(bl_saved);
    (void)hi; (void)lo; (void)r_at; (void)r_v1; (void)r_k0; (void)r_k1; (void)r_gp; (void)r_ra;
    (void)r_v0;
}

/* func_8008F0A8.s (399 instructions; --args 5 --void), frontier step2.  Callees through the jal service: func_80076B00. */
void func_8008F0A8(u32 arg0, u32 arg1, u32 arg2, u32 arg3, u32 arg4) {
    static const char FN[] = "func_8008F0A8";
    u32 r_at = 0, r_v0 = 0, r_v1 = 0, r_a0 = 0, r_a1 = 0, r_a2 = 0, r_a3 = 0, r_t0 = 0, r_t1 = 0, r_t2 = 0, r_t3 = 0, r_t4 = 0, r_t5 = 0, r_t6 = 0, r_t7 = 0, r_s0 = 0, r_s1 = 0, r_s2 = 0, r_s3 = 0, r_s4 = 0, r_s5 = 0, r_s6 = 0, r_s7 = 0, r_t8 = 0, r_t9 = 0, r_k0 = 0, r_k1 = 0, r_gp = 0, r_sp = 0, r_fp = 0, r_ra = 0;
    u32 hi = 0, lo = 0, bl_saved;
    r_sp = bl_enter(FN, 0x20u, &bl_saved);
    r_a0 = arg0;
    r_a1 = arg1;
    r_a2 = arg2;
    r_a3 = arg3;
    bl_sw(r_sp + 0x10u, arg4);
    r_sp = r_sp + 0xFFFFFFE0u; /* 8008F0A8 addiu $sp, $sp, -0x20 */
    r_a0 = r_a0 & 0xFFu; /* 8008F0AC andi $a0, $a0, 0xFF */
    r_a0 = r_a0 << 2; /* 8008F0B0 sll $a0, $a0, 2 */
    r_a1 = r_a1 + r_a3; /* 8008F0B4 addu $a1, $a1, $a3 */
    r_v1 = 0x800D0000u; /* 8008F0B8 lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 8008F0BC lw $v1, %lo(D_800CCB34)($v1) */
    r_a3 = r_a1 + 0xFFFFFFF8u; /* 8008F0C0 addiu $a3, $a1, -0x8 */
    bl_sw(r_sp + 0x18u, r_ra); /* 8008F0C4 sw $ra, 0x18($sp) */
    bl_sw(r_sp + 0x14u, r_s1); /* 8008F0C8 sw $s1, 0x14($sp) */
    bl_sw(r_sp + 0x10u, r_s0); /* 8008F0CC sw $s0, 0x10($sp) */
    r_at = 0x800D0000u; /* 8008F0D0 lui $at, %hi(D_800D2E38) */
    r_at = r_at + r_a0; /* 8008F0D4 addu $at, $at, $a0 */
    r_s1 = bl_lw(r_at + 0x2E38u); /* 8008F0D8 lw $s1, %lo(D_800D2E38)($at) */
    r_v0 = r_v1 << 2; /* 8008F0DC sll $v0, $v1, 2 */
    r_v0 = r_v0 + r_v1; /* 8008F0E0 addu $v0, $v0, $v1 */
    r_v0 = r_v0 << 3; /* 8008F0E4 sll $v0, $v0, 3 */
    r_v0 = r_v0 + r_s1; /* 8008F0E8 addu $v0, $v0, $s1 */
    bl_sh(r_v0 + 0x328u, r_a3); /* 8008F0EC sh $a3, 0x328($v0) */
    r_v1 = 0x800D0000u; /* 8008F0F0 lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 8008F0F4 lw $v1, %lo(D_800CCB34)($v1) */
    r_a2 = r_a2 + 0x8u; /* 8008F0F8 addiu $a2, $a2, 0x8 */
    r_v0 = r_v1 << 2; /* 8008F0FC sll $v0, $v1, 2 */
    r_v0 = r_v0 + r_v1; /* 8008F100 addu $v0, $v0, $v1 */
    r_v0 = r_v0 << 3; /* 8008F104 sll $v0, $v0, 3 */
    r_v0 = r_v0 + r_s1; /* 8008F108 addu $v0, $v0, $s1 */
    bl_sh(r_v0 + 0x32Au, r_a2); /* 8008F10C sh $a2, 0x32A($v0) */
    r_v1 = 0x800D0000u; /* 8008F110 lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 8008F114 lw $v1, %lo(D_800CCB34)($v1) */
    r_a1 = r_a1 + 0x8u; /* 8008F118 addiu $a1, $a1, 0x8 */
    r_v0 = r_v1 << 2; /* 8008F11C sll $v0, $v1, 2 */
    r_v0 = r_v0 + r_v1; /* 8008F120 addu $v0, $v0, $v1 */
    r_v0 = r_v0 << 3; /* 8008F124 sll $v0, $v0, 3 */
    r_v0 = r_v0 + r_s1; /* 8008F128 addu $v0, $v0, $s1 */
    bl_sh(r_v0 + 0x330u, r_a1); /* 8008F12C sh $a1, 0x330($v0) */
    r_v1 = 0x800D0000u; /* 8008F130 lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 8008F134 lw $v1, %lo(D_800CCB34)($v1) */
    ; /* 8008F138 nop  */
    r_v0 = r_v1 << 2; /* 8008F13C sll $v0, $v1, 2 */
    r_v0 = r_v0 + r_v1; /* 8008F140 addu $v0, $v0, $v1 */
    r_v0 = r_v0 << 3; /* 8008F144 sll $v0, $v0, 3 */
    r_v0 = r_v0 + r_s1; /* 8008F148 addu $v0, $v0, $s1 */
    bl_sh(r_v0 + 0x332u, r_a2); /* 8008F14C sh $a2, 0x332($v0) */
    r_v1 = 0x800D0000u; /* 8008F150 lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 8008F154 lw $v1, %lo(D_800CCB34)($v1) */
    ; /* 8008F158 nop  */
    r_v0 = r_v1 << 2; /* 8008F15C sll $v0, $v1, 2 */
    r_v0 = r_v0 + r_v1; /* 8008F160 addu $v0, $v0, $v1 */
    r_v0 = r_v0 << 3; /* 8008F164 sll $v0, $v0, 3 */
    r_v0 = r_v0 + r_s1; /* 8008F168 addu $v0, $v0, $s1 */
    bl_sh(r_v0 + 0x338u, r_a3); /* 8008F16C sh $a3, 0x338($v0) */
    r_v1 = 0x800D0000u; /* 8008F170 lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 8008F174 lw $v1, %lo(D_800CCB34)($v1) */
    r_a0 = bl_lhu(r_sp + 0x30u); /* 8008F178 lhu $a0, 0x30($sp) */
    r_v0 = r_v1 << 2; /* 8008F17C sll $v0, $v1, 2 */
    r_v0 = r_v0 + r_v1; /* 8008F180 addu $v0, $v0, $v1 */
    r_v0 = r_v0 << 3; /* 8008F184 sll $v0, $v0, 3 */
    r_v0 = r_v0 + r_s1; /* 8008F188 addu $v0, $v0, $s1 */
    r_a0 = r_a0 + 0xFFFFFFF0u; /* 8008F18C addiu $a0, $a0, -0x10 */
    r_v1 = r_a0 >> 31; /* 8008F190 srl $v1, $a0, 31 */
    r_a0 = r_a0 + r_v1; /* 8008F194 addu $a0, $a0, $v1 */
    r_a0 = BL_SRA(r_a0, 1); /* 8008F198 sra $a0, $a0, 1 */
    r_a2 = r_a2 + r_a0; /* 8008F19C addu $a2, $a2, $a0 */
    bl_sh(r_v0 + 0x33Au, r_a2); /* 8008F1A0 sh $a2, 0x33A($v0) */
    r_v1 = 0x800D0000u; /* 8008F1A4 lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 8008F1A8 lw $v1, %lo(D_800CCB34)($v1) */
    ; /* 8008F1AC nop  */
    r_v0 = r_v1 << 2; /* 8008F1B0 sll $v0, $v1, 2 */
    r_v0 = r_v0 + r_v1; /* 8008F1B4 addu $v0, $v0, $v1 */
    r_v0 = r_v0 << 3; /* 8008F1B8 sll $v0, $v0, 3 */
    r_v0 = r_v0 + r_s1; /* 8008F1BC addu $v0, $v0, $s1 */
    bl_sh(r_v0 + 0x340u, r_a1); /* 8008F1C0 sh $a1, 0x340($v0) */
    r_v1 = 0x800D0000u; /* 8008F1C4 lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 8008F1C8 lw $v1, %lo(D_800CCB34)($v1) */
    ; /* 8008F1CC nop  */
    r_v0 = r_v1 << 2; /* 8008F1D0 sll $v0, $v1, 2 */
    r_v0 = r_v0 + r_v1; /* 8008F1D4 addu $v0, $v0, $v1 */
    r_v0 = r_v0 << 3; /* 8008F1D8 sll $v0, $v0, 3 */
    r_v0 = r_v0 + r_s1; /* 8008F1DC addu $v0, $v0, $s1 */
    bl_sh(r_v0 + 0x342u, r_a2); /* 8008F1E0 sh $a2, 0x342($v0) */
    r_v1 = 0x800D0000u; /* 8008F1E4 lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 8008F1E8 lw $v1, %lo(D_800CCB34)($v1) */
    ; /* 8008F1EC nop  */
    r_v0 = r_v1 << 2; /* 8008F1F0 sll $v0, $v1, 2 */
    r_v0 = r_v0 + r_v1; /* 8008F1F4 addu $v0, $v0, $v1 */
    r_v0 = r_v0 << 3; /* 8008F1F8 sll $v0, $v0, 3 */
    r_v0 = r_v0 + r_s1; /* 8008F1FC addu $v0, $v0, $s1 */
    bl_sh(r_v0 + 0x378u, r_a3); /* 8008F200 sh $a3, 0x378($v0) */
    r_v1 = 0x800D0000u; /* 8008F204 lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 8008F208 lw $v1, %lo(D_800CCB34)($v1) */
    ; /* 8008F20C nop  */
    r_v0 = r_v1 << 2; /* 8008F210 sll $v0, $v1, 2 */
    r_v0 = r_v0 + r_v1; /* 8008F214 addu $v0, $v0, $v1 */
    r_v0 = r_v0 << 3; /* 8008F218 sll $v0, $v0, 3 */
    r_v0 = r_v0 + r_s1; /* 8008F21C addu $v0, $v0, $s1 */
    bl_sh(r_v0 + 0x37Au, r_a2); /* 8008F220 sh $a2, 0x37A($v0) */
    r_v1 = 0x800D0000u; /* 8008F224 lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 8008F228 lw $v1, %lo(D_800CCB34)($v1) */
    ; /* 8008F22C nop  */
    r_v0 = r_v1 << 2; /* 8008F230 sll $v0, $v1, 2 */
    r_v0 = r_v0 + r_v1; /* 8008F234 addu $v0, $v0, $v1 */
    r_v0 = r_v0 << 3; /* 8008F238 sll $v0, $v0, 3 */
    r_v0 = r_v0 + r_s1; /* 8008F23C addu $v0, $v0, $s1 */
    bl_sh(r_v0 + 0x380u, r_a1); /* 8008F240 sh $a1, 0x380($v0) */
    r_v1 = 0x800D0000u; /* 8008F244 lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 8008F248 lw $v1, %lo(D_800CCB34)($v1) */
    ; /* 8008F24C nop  */
    r_v0 = r_v1 << 2; /* 8008F250 sll $v0, $v1, 2 */
    r_v0 = r_v0 + r_v1; /* 8008F254 addu $v0, $v0, $v1 */
    r_v0 = r_v0 << 3; /* 8008F258 sll $v0, $v0, 3 */
    r_v0 = r_v0 + r_s1; /* 8008F25C addu $v0, $v0, $s1 */
    bl_sh(r_v0 + 0x382u, r_a2); /* 8008F260 sh $a2, 0x382($v0) */
    r_v1 = 0x800D0000u; /* 8008F264 lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 8008F268 lw $v1, %lo(D_800CCB34)($v1) */
    ; /* 8008F26C nop  */
    r_v0 = r_v1 << 2; /* 8008F270 sll $v0, $v1, 2 */
    r_v0 = r_v0 + r_v1; /* 8008F274 addu $v0, $v0, $v1 */
    r_v0 = r_v0 << 3; /* 8008F278 sll $v0, $v0, 3 */
    r_v0 = r_v0 + r_s1; /* 8008F27C addu $v0, $v0, $s1 */
    bl_sh(r_v0 + 0x388u, r_a3); /* 8008F280 sh $a3, 0x388($v0) */
    r_v1 = 0x800D0000u; /* 8008F284 lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 8008F288 lw $v1, %lo(D_800CCB34)($v1) */
    r_a2 = r_a2 + r_a0; /* 8008F28C addu $a2, $a2, $a0 */
    r_v0 = r_v1 << 2; /* 8008F290 sll $v0, $v1, 2 */
    r_v0 = r_v0 + r_v1; /* 8008F294 addu $v0, $v0, $v1 */
    r_v0 = r_v0 << 3; /* 8008F298 sll $v0, $v0, 3 */
    r_v0 = r_v0 + r_s1; /* 8008F29C addu $v0, $v0, $s1 */
    bl_sh(r_v0 + 0x38Au, r_a2); /* 8008F2A0 sh $a2, 0x38A($v0) */
    r_v1 = 0x800D0000u; /* 8008F2A4 lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 8008F2A8 lw $v1, %lo(D_800CCB34)($v1) */
    ; /* 8008F2AC nop  */
    r_v0 = r_v1 << 2; /* 8008F2B0 sll $v0, $v1, 2 */
    r_v0 = r_v0 + r_v1; /* 8008F2B4 addu $v0, $v0, $v1 */
    r_v0 = r_v0 << 3; /* 8008F2B8 sll $v0, $v0, 3 */
    r_v0 = r_v0 + r_s1; /* 8008F2BC addu $v0, $v0, $s1 */
    bl_sh(r_v0 + 0x390u, r_a1); /* 8008F2C0 sh $a1, 0x390($v0) */
    r_v1 = 0x800D0000u; /* 8008F2C4 lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 8008F2C8 lw $v1, %lo(D_800CCB34)($v1) */
    ; /* 8008F2CC nop  */
    r_v0 = r_v1 << 2; /* 8008F2D0 sll $v0, $v1, 2 */
    r_v0 = r_v0 + r_v1; /* 8008F2D4 addu $v0, $v0, $v1 */
    r_v0 = r_v0 << 3; /* 8008F2D8 sll $v0, $v0, 3 */
    r_v0 = r_v0 + r_s1; /* 8008F2DC addu $v0, $v0, $s1 */
    bl_sh(r_v0 + 0x392u, r_a2); /* 8008F2E0 sh $a2, 0x392($v0) */
    r_v0 = 0x800D0000u; /* 8008F2E4 lui $v0, %hi(D_800CCB34) */
    r_v0 = bl_lw(r_v0 + 0xFFFFCB34u); /* 8008F2E8 lw $v0, %lo(D_800CCB34)($v0) */
    ; /* 8008F2EC nop  */
    r_v1 = r_v0 << 2; /* 8008F2F0 sll $v1, $v0, 2 */
    r_v1 = r_v1 + r_v0; /* 8008F2F4 addu $v1, $v1, $v0 */
    r_v0 = 0x800C0000u; /* 8008F2F8 lui $v0, %hi(D_800C3EA4) */
    r_v0 = bl_lw(r_v0 + 0x3EA4u); /* 8008F2FC lw $v0, %lo(D_800C3EA4)($v0) */
    r_v1 = r_v1 << 3; /* 8008F300 sll $v1, $v1, 3 */
    r_at = 0x10000u; /* 8008F304 lui $at, (0x10000 >> 16) */
    r_at = r_v0 + r_at; /* 8008F308 addu $at, $v0, $at */
    r_v0 = bl_lbu(r_at + 0xFFFFA2A4u); /* 8008F30C lbu $v0, -0x5D5C($at) */
    r_v1 = r_v1 + r_s1; /* 8008F310 addu $v1, $v1, $s1 */
    r_v0 = r_v0 & 0x3Fu; /* 8008F314 andi $v0, $v0, 0x3F */
    r_v0 = r_v0 << 1; /* 8008F318 sll $v0, $v0, 1 */
    r_v0 = r_v0 + 0xEu; /* 8008F31C addiu $v0, $v0, 0xE */
    bl_sb(r_v1 + 0x32Cu, r_v0); /* 8008F320 sb $v0, 0x32C($v1) */
    r_v1 = 0x800D0000u; /* 8008F324 lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 8008F328 lw $v1, %lo(D_800CCB34)($v1) */
    ; /* 8008F32C nop  */
    r_v0 = r_v1 << 2; /* 8008F330 sll $v0, $v1, 2 */
    r_v0 = r_v0 + r_v1; /* 8008F334 addu $v0, $v0, $v1 */
    r_v1 = 0x800C0000u; /* 8008F338 lui $v1, %hi(D_800C3EA4) */
    r_v1 = bl_lw(r_v1 + 0x3EA4u); /* 8008F33C lw $v1, %lo(D_800C3EA4)($v1) */
    r_v0 = r_v0 << 3; /* 8008F340 sll $v0, $v0, 3 */
    r_at = 0x10000u; /* 8008F344 lui $at, (0x10000 >> 16) */
    r_at = r_v1 + r_at; /* 8008F348 addu $at, $v1, $at */
    r_v1 = bl_lbu(r_at + 0xFFFFA2A8u); /* 8008F34C lbu $v1, -0x5D58($at) */
    r_v0 = r_v0 + r_s1; /* 8008F350 addu $v0, $v0, $s1 */
    bl_sb(r_v0 + 0x32Du, r_v1); /* 8008F354 sb $v1, 0x32D($v0) */
    r_v0 = 0x800D0000u; /* 8008F358 lui $v0, %hi(D_800CCB34) */
    r_v0 = bl_lw(r_v0 + 0xFFFFCB34u); /* 8008F35C lw $v0, %lo(D_800CCB34)($v0) */
    ; /* 8008F360 nop  */
    r_v1 = r_v0 << 2; /* 8008F364 sll $v1, $v0, 2 */
    r_v1 = r_v1 + r_v0; /* 8008F368 addu $v1, $v1, $v0 */
    r_v0 = 0x800C0000u; /* 8008F36C lui $v0, %hi(D_800C3EA4) */
    r_v0 = bl_lw(r_v0 + 0x3EA4u); /* 8008F370 lw $v0, %lo(D_800C3EA4)($v0) */
    r_v1 = r_v1 << 3; /* 8008F374 sll $v1, $v1, 3 */
    r_at = 0x10000u; /* 8008F378 lui $at, (0x10000 >> 16) */
    r_at = r_v0 + r_at; /* 8008F37C addu $at, $v0, $at */
    r_v0 = bl_lbu(r_at + 0xFFFFA2A4u); /* 8008F380 lbu $v0, -0x5D5C($at) */
    r_v1 = r_v1 + r_s1; /* 8008F384 addu $v1, $v1, $s1 */
    r_v0 = r_v0 & 0x3Fu; /* 8008F388 andi $v0, $v0, 0x3F */
    r_v0 = r_v0 << 1; /* 8008F38C sll $v0, $v0, 1 */
    r_v0 = r_v0 + 0x1Eu; /* 8008F390 addiu $v0, $v0, 0x1E */
    bl_sb(r_v1 + 0x334u, r_v0); /* 8008F394 sb $v0, 0x334($v1) */
    r_v1 = 0x800D0000u; /* 8008F398 lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 8008F39C lw $v1, %lo(D_800CCB34)($v1) */
    ; /* 8008F3A0 nop  */
    r_v0 = r_v1 << 2; /* 8008F3A4 sll $v0, $v1, 2 */
    r_v0 = r_v0 + r_v1; /* 8008F3A8 addu $v0, $v0, $v1 */
    r_v1 = 0x800C0000u; /* 8008F3AC lui $v1, %hi(D_800C3EA4) */
    r_v1 = bl_lw(r_v1 + 0x3EA4u); /* 8008F3B0 lw $v1, %lo(D_800C3EA4)($v1) */
    r_v0 = r_v0 << 3; /* 8008F3B4 sll $v0, $v0, 3 */
    r_at = 0x10000u; /* 8008F3B8 lui $at, (0x10000 >> 16) */
    r_at = r_v1 + r_at; /* 8008F3BC addu $at, $v1, $at */
    r_v1 = bl_lbu(r_at + 0xFFFFA2A8u); /* 8008F3C0 lbu $v1, -0x5D58($at) */
    r_v0 = r_v0 + r_s1; /* 8008F3C4 addu $v0, $v0, $s1 */
    bl_sb(r_v0 + 0x335u, r_v1); /* 8008F3C8 sb $v1, 0x335($v0) */
    r_v0 = 0x800D0000u; /* 8008F3CC lui $v0, %hi(D_800CCB34) */
    r_v0 = bl_lw(r_v0 + 0xFFFFCB34u); /* 8008F3D0 lw $v0, %lo(D_800CCB34)($v0) */
    ; /* 8008F3D4 nop  */
    r_v1 = r_v0 << 2; /* 8008F3D8 sll $v1, $v0, 2 */
    r_v1 = r_v1 + r_v0; /* 8008F3DC addu $v1, $v1, $v0 */
    r_v0 = 0x800C0000u; /* 8008F3E0 lui $v0, %hi(D_800C3EA4) */
    r_v0 = bl_lw(r_v0 + 0x3EA4u); /* 8008F3E4 lw $v0, %lo(D_800C3EA4)($v0) */
    r_v1 = r_v1 << 3; /* 8008F3E8 sll $v1, $v1, 3 */
    r_at = 0x10000u; /* 8008F3EC lui $at, (0x10000 >> 16) */
    r_at = r_v0 + r_at; /* 8008F3F0 addu $at, $v0, $at */
    r_v0 = bl_lbu(r_at + 0xFFFFA2A4u); /* 8008F3F4 lbu $v0, -0x5D5C($at) */
    r_v1 = r_v1 + r_s1; /* 8008F3F8 addu $v1, $v1, $s1 */
    r_v0 = r_v0 & 0x3Fu; /* 8008F3FC andi $v0, $v0, 0x3F */
    r_v0 = r_v0 << 1; /* 8008F400 sll $v0, $v0, 1 */
    r_v0 = r_v0 + 0xEu; /* 8008F404 addiu $v0, $v0, 0xE */
    bl_sb(r_v1 + 0x33Cu, r_v0); /* 8008F408 sb $v0, 0x33C($v1) */
    r_v1 = 0x800D0000u; /* 8008F40C lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 8008F410 lw $v1, %lo(D_800CCB34)($v1) */
    ; /* 8008F414 nop  */
    r_v0 = r_v1 << 2; /* 8008F418 sll $v0, $v1, 2 */
    r_v0 = r_v0 + r_v1; /* 8008F41C addu $v0, $v0, $v1 */
    r_v1 = 0x800C0000u; /* 8008F420 lui $v1, %hi(D_800C3EA4) */
    r_v1 = bl_lw(r_v1 + 0x3EA4u); /* 8008F424 lw $v1, %lo(D_800C3EA4)($v1) */
    r_v0 = r_v0 << 3; /* 8008F428 sll $v0, $v0, 3 */
    r_at = 0x10000u; /* 8008F42C lui $at, (0x10000 >> 16) */
    r_at = r_v1 + r_at; /* 8008F430 addu $at, $v1, $at */
    r_v1 = bl_lbu(r_at + 0xFFFFA2A8u); /* 8008F434 lbu $v1, -0x5D58($at) */
    r_v0 = r_v0 + r_s1; /* 8008F438 addu $v0, $v0, $s1 */
    r_v1 = r_v1 + 0x7u; /* 8008F43C addiu $v1, $v1, 0x7 */
    bl_sb(r_v0 + 0x33Du, r_v1); /* 8008F440 sb $v1, 0x33D($v0) */
    r_v0 = 0x800D0000u; /* 8008F444 lui $v0, %hi(D_800CCB34) */
    r_v0 = bl_lw(r_v0 + 0xFFFFCB34u); /* 8008F448 lw $v0, %lo(D_800CCB34)($v0) */
    ; /* 8008F44C nop  */
    r_v1 = r_v0 << 2; /* 8008F450 sll $v1, $v0, 2 */
    r_v1 = r_v1 + r_v0; /* 8008F454 addu $v1, $v1, $v0 */
    r_v0 = 0x800C0000u; /* 8008F458 lui $v0, %hi(D_800C3EA4) */
    r_v0 = bl_lw(r_v0 + 0x3EA4u); /* 8008F45C lw $v0, %lo(D_800C3EA4)($v0) */
    r_v1 = r_v1 << 3; /* 8008F460 sll $v1, $v1, 3 */
    r_at = 0x10000u; /* 8008F464 lui $at, (0x10000 >> 16) */
    r_at = r_v0 + r_at; /* 8008F468 addu $at, $v0, $at */
    r_v0 = bl_lbu(r_at + 0xFFFFA2A4u); /* 8008F46C lbu $v0, -0x5D5C($at) */
    r_v1 = r_v1 + r_s1; /* 8008F470 addu $v1, $v1, $s1 */
    r_v0 = r_v0 & 0x3Fu; /* 8008F474 andi $v0, $v0, 0x3F */
    r_v0 = r_v0 << 1; /* 8008F478 sll $v0, $v0, 1 */
    r_v0 = r_v0 + 0x1Eu; /* 8008F47C addiu $v0, $v0, 0x1E */
    bl_sb(r_v1 + 0x344u, r_v0); /* 8008F480 sb $v0, 0x344($v1) */
    r_v1 = 0x800D0000u; /* 8008F484 lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 8008F488 lw $v1, %lo(D_800CCB34)($v1) */
    ; /* 8008F48C nop  */
    r_v0 = r_v1 << 2; /* 8008F490 sll $v0, $v1, 2 */
    r_v0 = r_v0 + r_v1; /* 8008F494 addu $v0, $v0, $v1 */
    r_v1 = 0x800C0000u; /* 8008F498 lui $v1, %hi(D_800C3EA4) */
    r_v1 = bl_lw(r_v1 + 0x3EA4u); /* 8008F49C lw $v1, %lo(D_800C3EA4)($v1) */
    r_v0 = r_v0 << 3; /* 8008F4A0 sll $v0, $v0, 3 */
    r_at = 0x10000u; /* 8008F4A4 lui $at, (0x10000 >> 16) */
    r_at = r_v1 + r_at; /* 8008F4A8 addu $at, $v1, $at */
    r_v1 = bl_lbu(r_at + 0xFFFFA2A8u); /* 8008F4AC lbu $v1, -0x5D58($at) */
    r_v0 = r_v0 + r_s1; /* 8008F4B0 addu $v0, $v0, $s1 */
    r_v1 = r_v1 + 0x7u; /* 8008F4B4 addiu $v1, $v1, 0x7 */
    bl_sb(r_v0 + 0x345u, r_v1); /* 8008F4B8 sb $v1, 0x345($v0) */
    r_v0 = 0x800D0000u; /* 8008F4BC lui $v0, %hi(D_800CCB34) */
    r_v0 = bl_lw(r_v0 + 0xFFFFCB34u); /* 8008F4C0 lw $v0, %lo(D_800CCB34)($v0) */
    ; /* 8008F4C4 nop  */
    r_v1 = r_v0 << 2; /* 8008F4C8 sll $v1, $v0, 2 */
    r_v1 = r_v1 + r_v0; /* 8008F4CC addu $v1, $v1, $v0 */
    r_v0 = 0x800C0000u; /* 8008F4D0 lui $v0, %hi(D_800C3EA4) */
    r_v0 = bl_lw(r_v0 + 0x3EA4u); /* 8008F4D4 lw $v0, %lo(D_800C3EA4)($v0) */
    r_v1 = r_v1 << 3; /* 8008F4D8 sll $v1, $v1, 3 */
    r_at = 0x10000u; /* 8008F4DC lui $at, (0x10000 >> 16) */
    r_at = r_v0 + r_at; /* 8008F4E0 addu $at, $v0, $at */
    r_v0 = bl_lbu(r_at + 0xFFFFA2A4u); /* 8008F4E4 lbu $v0, -0x5D5C($at) */
    r_v1 = r_v1 + r_s1; /* 8008F4E8 addu $v1, $v1, $s1 */
    r_v0 = r_v0 & 0x3Fu; /* 8008F4EC andi $v0, $v0, 0x3F */
    r_v0 = r_v0 << 1; /* 8008F4F0 sll $v0, $v0, 1 */
    r_v0 = r_v0 + 0xEu; /* 8008F4F4 addiu $v0, $v0, 0xE */
    bl_sb(r_v1 + 0x37Cu, r_v0); /* 8008F4F8 sb $v0, 0x37C($v1) */
    r_v1 = 0x800D0000u; /* 8008F4FC lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 8008F500 lw $v1, %lo(D_800CCB34)($v1) */
    ; /* 8008F504 nop  */
    r_v0 = r_v1 << 2; /* 8008F508 sll $v0, $v1, 2 */
    r_v0 = r_v0 + r_v1; /* 8008F50C addu $v0, $v0, $v1 */
    r_v1 = 0x800C0000u; /* 8008F510 lui $v1, %hi(D_800C3EA4) */
    r_v1 = bl_lw(r_v1 + 0x3EA4u); /* 8008F514 lw $v1, %lo(D_800C3EA4)($v1) */
    r_v0 = r_v0 << 3; /* 8008F518 sll $v0, $v0, 3 */
    r_at = 0x10000u; /* 8008F51C lui $at, (0x10000 >> 16) */
    r_at = r_v1 + r_at; /* 8008F520 addu $at, $v1, $at */
    r_v1 = bl_lbu(r_at + 0xFFFFA2A8u); /* 8008F524 lbu $v1, -0x5D58($at) */
    r_v0 = r_v0 + r_s1; /* 8008F528 addu $v0, $v0, $s1 */
    bl_sb(r_v0 + 0x37Du, r_v1); /* 8008F52C sb $v1, 0x37D($v0) */
    r_v0 = 0x800D0000u; /* 8008F530 lui $v0, %hi(D_800CCB34) */
    r_v0 = bl_lw(r_v0 + 0xFFFFCB34u); /* 8008F534 lw $v0, %lo(D_800CCB34)($v0) */
    ; /* 8008F538 nop  */
    r_v1 = r_v0 << 2; /* 8008F53C sll $v1, $v0, 2 */
    r_v1 = r_v1 + r_v0; /* 8008F540 addu $v1, $v1, $v0 */
    r_v0 = 0x800C0000u; /* 8008F544 lui $v0, %hi(D_800C3EA4) */
    r_v0 = bl_lw(r_v0 + 0x3EA4u); /* 8008F548 lw $v0, %lo(D_800C3EA4)($v0) */
    r_v1 = r_v1 << 3; /* 8008F54C sll $v1, $v1, 3 */
    r_at = 0x10000u; /* 8008F550 lui $at, (0x10000 >> 16) */
    r_at = r_v0 + r_at; /* 8008F554 addu $at, $v0, $at */
    r_v0 = bl_lbu(r_at + 0xFFFFA2A4u); /* 8008F558 lbu $v0, -0x5D5C($at) */
    r_v1 = r_v1 + r_s1; /* 8008F55C addu $v1, $v1, $s1 */
    r_v0 = r_v0 & 0x3Fu; /* 8008F560 andi $v0, $v0, 0x3F */
    r_v0 = r_v0 << 1; /* 8008F564 sll $v0, $v0, 1 */
    r_v0 = r_v0 + 0x1Eu; /* 8008F568 addiu $v0, $v0, 0x1E */
    bl_sb(r_v1 + 0x384u, r_v0); /* 8008F56C sb $v0, 0x384($v1) */
    r_v1 = 0x800D0000u; /* 8008F570 lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 8008F574 lw $v1, %lo(D_800CCB34)($v1) */
    ; /* 8008F578 nop  */
    r_v0 = r_v1 << 2; /* 8008F57C sll $v0, $v1, 2 */
    r_v0 = r_v0 + r_v1; /* 8008F580 addu $v0, $v0, $v1 */
    r_v1 = 0x800C0000u; /* 8008F584 lui $v1, %hi(D_800C3EA4) */
    r_v1 = bl_lw(r_v1 + 0x3EA4u); /* 8008F588 lw $v1, %lo(D_800C3EA4)($v1) */
    r_v0 = r_v0 << 3; /* 8008F58C sll $v0, $v0, 3 */
    r_at = 0x10000u; /* 8008F590 lui $at, (0x10000 >> 16) */
    r_at = r_v1 + r_at; /* 8008F594 addu $at, $v1, $at */
    r_v1 = bl_lbu(r_at + 0xFFFFA2A8u); /* 8008F598 lbu $v1, -0x5D58($at) */
    r_v0 = r_v0 + r_s1; /* 8008F59C addu $v0, $v0, $s1 */
    bl_sb(r_v0 + 0x385u, r_v1); /* 8008F5A0 sb $v1, 0x385($v0) */
    r_v0 = 0x800D0000u; /* 8008F5A4 lui $v0, %hi(D_800CCB34) */
    r_v0 = bl_lw(r_v0 + 0xFFFFCB34u); /* 8008F5A8 lw $v0, %lo(D_800CCB34)($v0) */
    ; /* 8008F5AC nop  */
    r_v1 = r_v0 << 2; /* 8008F5B0 sll $v1, $v0, 2 */
    r_v1 = r_v1 + r_v0; /* 8008F5B4 addu $v1, $v1, $v0 */
    r_v0 = 0x800C0000u; /* 8008F5B8 lui $v0, %hi(D_800C3EA4) */
    r_v0 = bl_lw(r_v0 + 0x3EA4u); /* 8008F5BC lw $v0, %lo(D_800C3EA4)($v0) */
    r_v1 = r_v1 << 3; /* 8008F5C0 sll $v1, $v1, 3 */
    r_at = 0x10000u; /* 8008F5C4 lui $at, (0x10000 >> 16) */
    r_at = r_v0 + r_at; /* 8008F5C8 addu $at, $v0, $at */
    r_v0 = bl_lbu(r_at + 0xFFFFA2A4u); /* 8008F5CC lbu $v0, -0x5D5C($at) */
    r_v1 = r_v1 + r_s1; /* 8008F5D0 addu $v1, $v1, $s1 */
    r_v0 = r_v0 & 0x3Fu; /* 8008F5D4 andi $v0, $v0, 0x3F */
    r_v0 = r_v0 << 1; /* 8008F5D8 sll $v0, $v0, 1 */
    r_v0 = r_v0 + 0xEu; /* 8008F5DC addiu $v0, $v0, 0xE */
    bl_sb(r_v1 + 0x38Cu, r_v0); /* 8008F5E0 sb $v0, 0x38C($v1) */
    r_v1 = 0x800D0000u; /* 8008F5E4 lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 8008F5E8 lw $v1, %lo(D_800CCB34)($v1) */
    ; /* 8008F5EC nop  */
    r_v0 = r_v1 << 2; /* 8008F5F0 sll $v0, $v1, 2 */
    r_v0 = r_v0 + r_v1; /* 8008F5F4 addu $v0, $v0, $v1 */
    r_v1 = 0x800C0000u; /* 8008F5F8 lui $v1, %hi(D_800C3EA4) */
    r_v1 = bl_lw(r_v1 + 0x3EA4u); /* 8008F5FC lw $v1, %lo(D_800C3EA4)($v1) */
    r_v0 = r_v0 << 3; /* 8008F600 sll $v0, $v0, 3 */
    r_at = 0x10000u; /* 8008F604 lui $at, (0x10000 >> 16) */
    r_at = r_v1 + r_at; /* 8008F608 addu $at, $v1, $at */
    r_v1 = bl_lbu(r_at + 0xFFFFA2A8u); /* 8008F60C lbu $v1, -0x5D58($at) */
    r_v0 = r_v0 + r_s1; /* 8008F610 addu $v0, $v0, $s1 */
    r_v1 = r_v1 + 0x7u; /* 8008F614 addiu $v1, $v1, 0x7 */
    bl_sb(r_v0 + 0x38Du, r_v1); /* 8008F618 sb $v1, 0x38D($v0) */
    r_v0 = 0x800D0000u; /* 8008F61C lui $v0, %hi(D_800CCB34) */
    r_v0 = bl_lw(r_v0 + 0xFFFFCB34u); /* 8008F620 lw $v0, %lo(D_800CCB34)($v0) */
    ; /* 8008F624 nop  */
    r_v1 = r_v0 << 2; /* 8008F628 sll $v1, $v0, 2 */
    r_v1 = r_v1 + r_v0; /* 8008F62C addu $v1, $v1, $v0 */
    r_v0 = 0x800C0000u; /* 8008F630 lui $v0, %hi(D_800C3EA4) */
    r_v0 = bl_lw(r_v0 + 0x3EA4u); /* 8008F634 lw $v0, %lo(D_800C3EA4)($v0) */
    r_v1 = r_v1 << 3; /* 8008F638 sll $v1, $v1, 3 */
    r_at = 0x10000u; /* 8008F63C lui $at, (0x10000 >> 16) */
    r_at = r_v0 + r_at; /* 8008F640 addu $at, $v0, $at */
    r_v0 = bl_lbu(r_at + 0xFFFFA2A4u); /* 8008F644 lbu $v0, -0x5D5C($at) */
    r_v1 = r_v1 + r_s1; /* 8008F648 addu $v1, $v1, $s1 */
    r_v0 = r_v0 & 0x3Fu; /* 8008F64C andi $v0, $v0, 0x3F */
    r_v0 = r_v0 << 1; /* 8008F650 sll $v0, $v0, 1 */
    r_v0 = r_v0 + 0x1Eu; /* 8008F654 addiu $v0, $v0, 0x1E */
    bl_sb(r_v1 + 0x394u, r_v0); /* 8008F658 sb $v0, 0x394($v1) */
    r_v1 = 0x800D0000u; /* 8008F65C lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 8008F660 lw $v1, %lo(D_800CCB34)($v1) */
    r_s0 = 0u + 0u; /* 8008F664 addu $s0, $zero, $zero */
    r_v0 = r_v1 << 2; /* 8008F668 sll $v0, $v1, 2 */
    r_v0 = r_v0 + r_v1; /* 8008F66C addu $v0, $v0, $v1 */
    r_v1 = 0x800C0000u; /* 8008F670 lui $v1, %hi(D_800C3EA4) */
    r_v1 = bl_lw(r_v1 + 0x3EA4u); /* 8008F674 lw $v1, %lo(D_800C3EA4)($v1) */
    r_v0 = r_v0 << 3; /* 8008F678 sll $v0, $v0, 3 */
    r_at = 0x10000u; /* 8008F67C lui $at, (0x10000 >> 16) */
    r_at = r_v1 + r_at; /* 8008F680 addu $at, $v1, $at */
    r_v1 = bl_lbu(r_at + 0xFFFFA2A8u); /* 8008F684 lbu $v1, -0x5D58($at) */
    r_v0 = r_v0 + r_s1; /* 8008F688 addu $v0, $v0, $s1 */
    r_v1 = r_v1 + 0x7u; /* 8008F68C addiu $v1, $v1, 0x7 */
    bl_sb(r_v0 + 0x395u, r_v1); /* 8008F690 sb $v1, 0x395($v0) */
    r_v0 = r_s0 << 1; /* 8008F694 sll $v0, $s0, 1 */
L_8008F698:
    r_v1 = 0x800D0000u; /* 8008F698 lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 8008F69C lw $v1, %lo(D_800CCB34)($v1) */
    r_s0 = r_s0 + 0x1u; /* 8008F6A0 addiu $s0, $s0, 0x1 */
    r_v0 = r_v0 + r_v1; /* 8008F6A4 addu $v0, $v0, $v1 */
    r_a0 = r_v0 << 2; /* 8008F6A8 sll $a0, $v0, 2 */
    r_a0 = r_a0 + r_v0; /* 8008F6AC addu $a0, $a0, $v0 */
    r_a0 = r_a0 << 3; /* 8008F6B0 sll $a0, $a0, 3 */
    r_a0 = r_a0 + 0x320u; /* 8008F6B4 addiu $a0, $a0, 0x320 */
    /* 8008F6B8 jal func_80076B00 */
    r_a0 = r_s1 + r_a0;
    bl_jal(FN, 0x80076B00u, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* func_80076B00 */
    r_v0 = (u32)((s32)r_s0 < 2); /* 8008F6C0 slti $v0, $s0, 0x2 */
    /* 8008F6C4 bnez $v0, .L8008F698 */
    { int c_ = (r_v0 != 0); r_v0 = r_s0 << 1; if (c_) goto L_8008F698; }
    r_ra = bl_lw(r_sp + 0x18u); /* 8008F6CC lw $ra, 0x18($sp) */
    r_s1 = bl_lw(r_sp + 0x14u); /* 8008F6D0 lw $s1, 0x14($sp) */
    r_s0 = bl_lw(r_sp + 0x10u); /* 8008F6D4 lw $s0, 0x10($sp) */
    r_sp = r_sp + 0x20u; /* 8008F6D8 addiu $sp, $sp, 0x20 */
    /* 8008F6DC jr $ra */
    ; goto bl_exit;
bl_exit:
    bl_leave(bl_saved);
    (void)hi; (void)lo; (void)r_at; (void)r_v1; (void)r_k0; (void)r_k1; (void)r_gp; (void)r_ra;
    (void)r_v0;
}
#endif /* XENO_PC_PORT h-frontier */
