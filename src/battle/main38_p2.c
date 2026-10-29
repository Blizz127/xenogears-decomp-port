/* Retail TU part 2 of main38.c (see the BATTLE_TU_PART note there): the
 * still-assembly run below, then that file's part-2 C bodies. */
#define BATTLE_TU_PART 2
#include "common.h"
#ifndef XENO_PC_PORT
extern u8 D_800D2D24[];
extern u8* D_800D2DB4;
extern s32 D_800CCB34;
extern void func_80076D58(u8* prims, u8 sel, u8 mode);
extern void* func_80033784(s32 table, s32 id);
extern s32 SystemRenderStringEntry(void* str, u32 buf, s32 h, s32 flag);
typedef struct {
    s16 x, y, w, h;
} BattleRect;
extern void LoadImage(BattleRect* rect, u32 buf);
extern void func_80076C78(u8* p, u16 x, u16 y, u8 u, u8 v, u8 w);

/* func_80086028.s, byte-exact: draw entry `id` of character
 * D_800D2D24[actor] in slot `slot` and return slot + 1.  The slot's two
 * POLY_FT4s at D_800D2DB4 + 0x5550 + slot * 80 are initialised
 * (func_80076D58(prims, slot % 2, 3)); the func_80033784 string is
 * rendered into *buf (SystemRenderStringEntry, height 0x1B, slot % 2) and
 * uploaded to the 30 x 13 VRAM rect at (0x3C0 + slot / 2 * 30, 0x1A);
 * then func_80076C78 sets the D_800CCB34 buffer's prim to x = (row + flag
 * + 1) * 16 + 0x50 + slot * 4, y = 0xC8 - slot * 16, u = slot / 2 * 120,
 * v = 0x1A and the rendered width, and the D_800D2DB4 +0x5D80 count goes
 * up by one.
 * Codegen: the prim offsets are parenthesised (offset first, then the
 * base); `flag + 1` goes through a local, so fold does not reassociate it
 * into (row + 1) + flag; `return ++slot` gives slot the extra references
 * that rank it above `odd` in local-alloc (s3 vs s4).  Matches only under
 * gcc 2.6.x (2.7.2: diff 8/106). */
s32 func_80086028(u8 actor, s32 slot, s32 row, u8 id, u32* buf, u8 flag) {
    BattleRect rect;
    s32 half = slot / 2;
    s32 odd = slot % 2;
    s32 w;
    s32 n;

    func_80076D58(D_800D2DB4 + (slot * 80 + 0x5550), odd, 3);
    w = SystemRenderStringEntry(func_80033784(D_800D2D24[actor], id), *buf, 0x1B, odd);
    rect.x = half * 30 + 0x3C0;
    rect.y = 0x1A;
    rect.w = 0x1E;
    rect.h = 0xD;
    LoadImage(&rect, *buf);
    n = flag + 1;
    func_80076C78(D_800D2DB4 + ((slot * 2 + D_800CCB34) * 40 + 0x5550),
                  (row + n) * 16 + 0x50 + slot * 4, 0xC8 - slot * 16, half * 120, 0x1A, w);
    D_800D2DB4[0x5D80]++;
    return ++slot;
}
/* func_800861D0.s, byte-exact: add combo input `index` - 4 to actor
 * `actor`'s input list in the command menu context D_800C3EAC (+0x2CC,
 * count +0x2D6; with +0x2E4 set the new tail reads 0xFF during the lookup)
 * and look the list up among the 13 seven-byte D_800C3160 sequences
 * (combo = first full match, 13 for none).  The D_800D2DB4 prim counts
 * +0x5D80/+0x5D81 are cleared and one glyph per input is drawn at x = 0x50
 * + i * 16 (0 -> 0x5D, 2 -> 0x5E, 3 -> 0x5F; any other value reuses the
 * previous glyph, which retail leaves unset on the first input).  Entries
 * come from D_800C31AC[the actor's D_800CCD3E id] and must pass
 * func_80089C6C (g_GameState +0x16C0 mask row D_800D2D24[actor]):
 * - combos 0-12 offer entry `combo` (while actor row +0x20 is clear): glyph
 *   7 after the inputs (with +0x2E4 set the slot steps back instead), then
 *   n = func_80086028(..., 0) and shown = 1.  With +0x2E4 set and shown,
 *   +0x2DC = entry + 8, the D_800CCB34 low byte goes to +0x5D90/+0x5D8F,
 *   D_800D2D28[0xA8] = 1, func_800716D8 runs, the three D_800C3A70 text
 *   buffers are freed and that is all.
 * - Otherwise combos 0-5 and 8 offer entry 13 + combo / 19 (glyphs 8 and 7,
 *   actor +0x1C and +0x20 clear) and combos 0, 1 and 3 offer entries 20,
 *   21 and 22 (glyphs 9 and 7, +0x1E and +0x20 clear), both through
 *   func_80086028(..., 1); then the D_800CCB34 bytes, D_800D2D28[0xA8] = 1,
 *   func_800716D8 and the frees.
 * Codegen: one variable serves as the match flag and then the combo, one
 * as the inner index and then the fallthrough step count, as retail's
 * registers show; the lookup walks a ctx copy (retail reloads D_800C3EAC
 * after it); the second glyph of a pair is at (i + 1) * 16 (a shared
 * i * 16 would stay live across the call and push actor out of fp); the
 * D_800CCD3E id is read through a [][0x170] row view (a flat
 * [actor * 0x170] index gets its whole address kept in a register); shown
 * = 1 after the call (sched2 hoists it among the arguments).  Both jump
 * tables (jtbl_800702B8, jtbl_800702F0) are this TU's .rodata. */
typedef struct {
    u32 buf;
} M38TextBuf;
extern M38TextBuf D_800C3A70[];
extern u8* D_800C3EAC;
extern u8 D_800D2D24[];
extern u8 g_GameState[];
extern u32 D_800C3160[];
extern u32 D_800C31AC[];
extern u8 D_800CCD3E__r[][0x170] asm("D_800CCD3E");
extern u8* D_800D2D28;
extern s32 D_800CCB34__a[] asm("D_800CCB34");
extern u32 func_80089C6C(u32 a0, u8 a1);
extern void* func_8008AC00(s32 size);
extern s32 func_80076A10(s32 index, u8* polys, s16 x, s16 y);
extern void func_800716D8(void);

void func_800861D0(u8 index, u8 actor) {
    s32 n = 0;
    s32 shown = 0;
    s32 j;
    s32 i;
    s32 step;
    s32 combo;
    u8 glyph;
    u8* seq;
    u8* ctx;

    for (j = 0; j < 3; j++) {
        D_800C3A70[j].buf = (u32)func_8008AC00(0x1E);
    }
    (D_800C3EAC + D_800C3EAC[0x2D6])[0x2CC] = index - 4;
    D_800C3EAC[0x2D6]++;
    if (D_800C3EAC[0x2E4] != 0) {
        (D_800C3EAC + D_800C3EAC[0x2D6])[0x2CB] = 0xFF;
    }
    ctx = D_800C3EAC;
    for (i = 0; i < 13; i++) {
        seq = (u8*)D_800C3160[i];
        for (step = 0; step < 7; step++) {
            combo = 0;
            if ((ctx + step)[0x2CC] != seq[step]) {
                break;
            }
            combo = 1;
        }
        if (combo) {
            break;
        }
    }
    combo = i;
    if (D_800C3EAC[0x2E4] != 0) {
        (D_800C3EAC + D_800C3EAC[0x2D6])[0x2CB] = index - 4;
    }
    D_800D2DB4[0x5D81] = 0;
    D_800D2DB4[0x5D80] = 0;
    for (i = 0; i < D_800C3EAC[0x2D6]; i++) {
        switch ((D_800C3EAC + i)[0x2CC]) {
        case 0:
            glyph = 0x5D;
            break;
        case 2:
            glyph = 0x5E;
            break;
        case 3:
            glyph = 0x5F;
            break;
        }
        D_800D2DB4[0x5D81] += func_80076A10(glyph, D_800D2DB4 + (D_800D2DB4[0x5D81] * 0x50 + 0x5640),
                                            i * 16 + 0x50, 0xD0 - n * 16);
    }
    step = 0;
    switch (combo) {
    case 0:
        step++;
    case 1:
        step++;
    case 2:
        step++;
    case 3:
        step++;
    case 4:
        step++;
    case 5:
        step++;
    case 6:
        step++;
    case 7:
        step++;
    case 8:
        step++;
    case 9:
        step++;
    case 10:
        step++;
    case 11:
        step++;
    case 12:
        if ((u16)func_80089C6C(*(u16*)(g_GameState + 0x16C0 + D_800D2D24[actor] * 32),
                               *(u8*)(D_800C31AC[D_800CCD3E__r[actor][0]] + (12 - step))) != 0 &&
            *(u16*)(D_800C3EAC + (actor << 6) + 0x20) == 0) {
            if (D_800C3EAC[0x2E4] == 0) {
                D_800D2DB4[0x5D81] += func_80076A10(7, D_800D2DB4 + (D_800D2DB4[0x5D81] * 0x50 + 0x5640),
                                                    i * 16 + 0x50, 0xD0 - n * 16);
            } else {
                i--;
            }
            n = func_80086028(actor, n, i, *(u8*)(D_800C31AC[D_800CCD3E__r[actor][0]] + (12 - step)),
                              &D_800C3A70[n / 2].buf, 0);
            shown = 1;
        }
        break;
    }
    if (D_800C3EAC[0x2E4] != 0 && shown != 0) {
        D_800D2DB4[0x5D90] = *(u8*)D_800CCB34__a;
        D_800C3EAC[0x2DC] = *(u8*)(D_800C31AC[D_800CCD3E__r[actor][0]] + (12 - step)) + 8;
        D_800D2DB4[0x5D8F] = *(u8*)D_800CCB34__a;
        D_800D2D28[0xA8] = 1;
        func_800716D8();
        for (j = 0; j < 3; j++) {
            HeapFree((void*)D_800C3A70[j].buf);
        }
        return;
    }
    step = 0;
    switch (combo) {
    case 0:
        step++;
    case 1:
        step++;
    case 2:
        step++;
    case 3:
        step++;
    case 4:
        step++;
    case 5:
        step++;
    case 8:
        if ((u16)func_80089C6C(*(u16*)(g_GameState + 0x16C0 + D_800D2D24[actor] * 32),
                               *(u8*)(D_800C31AC[D_800CCD3E__r[actor][0]] + (0x13 - step))) != 0 &&
            *(u16*)(D_800C3EAC + (actor << 6) + 0x1C) == 0 && *(u16*)(D_800C3EAC + (actor << 6) + 0x20) == 0) {
            s16 y = 0xD0 - n * 16;

            D_800D2DB4[0x5D81] += func_80076A10(8, D_800D2DB4 + (D_800D2DB4[0x5D81] * 0x50 + 0x5640),
                                                i * 16 + 0x54, y);
            D_800D2DB4[0x5D81] += func_80076A10(7, D_800D2DB4 + (D_800D2DB4[0x5D81] * 0x50 + 0x5640),
                                                (i + 1) * 16 + 0x54, y);
            n = func_80086028(actor, n, i, *(u8*)(D_800C31AC[D_800CCD3E__r[actor][0]] + (0x13 - step)),
                              &D_800C3A70[n / 2].buf, 1);
        }
        break;
    }
    step = 0;
    switch (combo) {
    case 0:
        step++;
    case 1:
        step++;
    case 3:
        if ((u16)func_80089C6C(*(u16*)(g_GameState + 0x16C0 + D_800D2D24[actor] * 32),
                               *(u8*)(D_800C31AC[D_800CCD3E__r[actor][0]] + (0x16 - step))) != 0 &&
            *(u16*)(D_800C3EAC + (actor << 6) + 0x1E) == 0 && *(u16*)(D_800C3EAC + (actor << 6) + 0x20) == 0) {
            s16 y = 0xD0 - n * 16;

            D_800D2DB4[0x5D81] += func_80076A10(9, D_800D2DB4 + (D_800D2DB4[0x5D81] * 0x50 + 0x5640),
                                                i * 16 + 0x58, y);
            D_800D2DB4[0x5D81] += func_80076A10(7, D_800D2DB4 + (D_800D2DB4[0x5D81] * 0x50 + 0x5640),
                                                (i + 1) * 16 + 0x58, y);
            func_80086028(actor, n, i, *(u8*)(D_800C31AC[D_800CCD3E__r[actor][0]] + (0x16 - step)),
                          &D_800C3A70[n / 2].buf, 1);
        }
        break;
    }
    D_800D2DB4[0x5D90] = *(u8*)D_800CCB34__a;
    D_800D2DB4[0x5D8F] = *(u8*)D_800CCB34__a;
    D_800D2D28[0xA8] = 1;
    func_800716D8();
    for (j = 0; j < 3; j++) {
        HeapFree((void*)D_800C3A70[j].buf);
    }
}
extern u8* D_800C3EAC;
extern u8 D_800D2C34[];
extern u8 D_800D2D24[];
extern u8 D_800C34CC[];
extern u8 g_GameState[];
extern u32 func_80089C6C(u32 a0, u8 a1);

/* func_80086B88.s, byte-exact: 1 when entry `index` may be used in the
 * current command context (D_800C3EAC).  With +0x2D6 clear it always may.
 * Otherwise its byte of D_800C34CC (at index + (+0x2CC + 1) * 3, or
 * index + 12 when D_800D2C34 is 4) must be allowed by func_80089C6C in
 * the g_GameState +0x16C4 mask of character D_800D2D24[slot], and unless
 * +0x2CC is 0xFF or D_800D2C34 is 4, D_800D2C34 must exceed +0x2CC. */
s32 func_80086B88(u8 index, u8 slot) {
    u8 i = index + (D_800C3EAC[0x2CC] + 1) * 3;
    s32 ok = 1;

    if (D_800C3EAC[0x2D6] != 0) {
        if (D_800D2C34[0] == 4) {
            i = index + 12;
        }
        if ((u16)func_80089C6C(*(u16*)(g_GameState + 0x16C4 + D_800D2D24[slot] * 32), D_800C34CC[i]) == 0) {
            ok = 0;
        } else if (D_800C3EAC[0x2CC] != 0xFF && D_800D2C34[0] != 4 && D_800D2C34[0] < D_800C3EAC[0x2CC] + 1) {
            ok = 0;
        }
    }
    return ok;
}
/* func_80086C88.s, byte-exact under gcc 2.6.3 (main38_p2 is BattleGcc263):
 * the glyph-row sibling of func_80086028.  Draw entry `id` of the actor
 * record D_800CCD88[actor * 0x170] in slot `slot` and return slot + 1.
 * The name string (func_800339C8 of the record byte and D_800C34CC[id]) is
 * rendered into *buf and uploaded to the 30 x 13 VRAM rect at
 * (0x3C0 + slot / 2 * 30, 0x1A), and the D_800CCB34 buffer's +0x5550 prim
 * is placed as in func_80086028 (x = slot * 4 + (row + 1) * 16 + 0x86).
 * The +0x5C80 prims show up to four glyphs: after
 * func_8008AAA0(D_800CCB34 table +0x60D8 [id]), each of the four bytes at
 * D_800C3CF9 that is not 0xFF has its D_800C3E5C word uploaded to a 6 x 13
 * rect at x = slot * 8 + 0x3DE + 2 * n; that prim gets x = ... + 0xEA,
 * u = slot * 32 + 0x78, v = 0 and width n * 8.  The D_800D2DB4 counts at
 * +0x5D80 and +0x5D82 go up by one.
 * Codegen: the locals are one struct {rect; glyphs[4]} (retail addresses
 * the glyph rects from the frame base); D_800CCB34 goes through a struct
 * view (one `la` base for the buf word and the +0x60D8 table);
 * `return ++slot`, as in func_80086028, gives slot the extra references
 * that rank it above the D_800C3CF9 walker in global-alloc (s7 vs fp);
 * the glyph stores are spelled `(loc.glyphs + n)->` (`loc.glyphs[n].`
 * swaps v0/v1 in the store address); `x += 2` follows the call.
 * CONSTRUCTS (counted separately): the call's rect argument is built
 * set-twice (`g = loc.glyphs; g += n;`) and the glyph word is read through
 * a pointer temp (`q = &D_800C3E5C[p[i]]; func_800769E8(g, *q);`); folding
 * either into the call grows the function (792-808 vs 784 bytes). */
typedef struct {
    s32 buf;
    u8 pad4[0x60D4];
    u16 tbl[1];
} B86Ctx;
extern B86Ctx D_800CCB34__s asm("D_800CCB34");
extern u8 D_800CCD88[];
extern u8 D_800C3CF9[];
extern u32 D_800C3E5C[];
extern void* func_800339C8(u8 a, u8 b);
extern void func_800769E8(BattleRect* r, u32 glyph);
extern void func_8008AAA0(u32 a0);

s32 func_80086C88(u8 actor, s32 slot, s32 row, u8 id, u32* buf) {
    struct {
        BattleRect rect;
        BattleRect glyphs[4];
    } loc;
    s32 half = slot / 2;
    s32 odd = slot % 2;
    s32 w;
    s32 n = 0;
    s32 x;
    s32 i;
    u8* p = D_800C3CF9;
    BattleRect* g;
    u32* q;

    func_80076D58(D_800D2DB4 + (slot * 80 + 0x5550), odd, 3);
    w = SystemRenderStringEntry(func_800339C8(D_800CCD88[actor * 0x170], D_800C34CC[id]), *buf, 0x1B, odd);
    loc.rect.x = half * 30 + 0x3C0;
    loc.rect.y = 0x1A;
    loc.rect.w = 0x1E;
    loc.rect.h = 0xD;
    LoadImage(&loc.rect, *buf);
    func_80076C78(D_800D2DB4 + ((slot * 2 + D_800CCB34__s.buf) * 40 + 0x5550),
                  slot * 4 + (row + 1) * 16 + 0x86, 0xC8 - slot * 16, half * 120, 0x1A, w);
    func_80076D58(D_800D2DB4 + (slot * 80 + 0x5C80), 0, 3);
    func_8008AAA0(D_800CCB34__s.tbl[id]);
    x = slot * 8 + 0x3DE;
    for (i = 0; i < 4; i++) {
        if (p[i] != 0xFF) {
            (loc.glyphs + n)->x = x;
            (loc.glyphs + n)->y = 0;
            (loc.glyphs + n)->w = 6;
            (loc.glyphs + n)->h = 0xD;
            g = loc.glyphs;
            g += n;
            q = &D_800C3E5C[p[i]];
            func_800769E8(g, *q);
            x += 2;
            n++;
        }
    }
    func_80076C78(D_800D2DB4 + ((slot * 2 + D_800CCB34__s.buf) * 40 + 0x5C80),
                  slot * 4 + (row + 1) * 16 + 0xEA, 0xC8 - slot * 16, slot * 32 + 0x78, 0, n * 8);
    D_800D2DB4[0x5D80]++;
    D_800D2DB4[0x5D82]++;
    return ++slot;
}

/* func_80086F98.s, byte-exact: confirm command `index` for the battle
 * character in slot `actor` of the command menu context D_800C3EAC; the
 * caller func_80083948 ignores the result at one call site and stores
 * (result == 0) in +0x2E4 at the other.
 * - With bit 7 of the actor's D_800CCE42 byte (0x170-byte records) clear
 *   and D_800D2C34 != 4: +0x2DC = D_800C34CC[index], return 1.
 * - Unless (D_800D2C34 != 0 and index < D_800D2C34) or +0x2E7 is set:
 *   +0x2DC = index, return 1.
 * - Otherwise three 0x1E text buffers (func_8008AC00) go into D_800C3A70,
 *   index is appended to the +0x2CC list (count +0x2D6; with +0x2E4 set the
 *   new tail byte is written 0xFF then index, and sel = index + (+0x2CC +
 *   1) * 3), the D_800D2DB4 prim counts +0x5D80..+0x5D82 are cleared and
 *   one glyph per listed entry is drawn at x = 0x80 + i * 16 (0 -> 0x5E,
 *   1 -> 0x5F, 2 -> 0x5D, 0xFF skipped; any other byte reuses the previous
 *   glyph, which retail leaves unset on the first entry).  With D_800D2C34
 *   == 4 sel is index + 12 (12 for index 0xFF).  When func_80089C6C allows
 *   D_800C34CC[sel] for the actor (g_GameState +0x16C4 mask), ret = 1 and
 *   func_80086C88 lists entry sel: directly with +0x2E4 set, else after
 *   glyph 8 and only while actor +0x1C is zero.  With +0x2E4 set and ret =
 *   1: ret = 0 and +0x2DC = D_800C34CC[sel] (sel = index + 12 when +0x2CC
 *   is 0xFF).  Otherwise +0x2DC = index and entries (index + 1) * 3 + 1 /
 *   + 2 (13 / 14 in mode 4) are offered with glyphs 9 / 7 (ret = 1 each)
 *   when allowed and actor +0x1E / +0x20 is zero.  Both exits copy the
 *   D_800CCB34 low byte into +0x5D8F..+0x5D91, set D_800D2D28[0xA8] = 1,
 *   run func_800716D8, free the three buffers and return ret.
 * Codegen: `(actor << 6)` keeps retail's base-first addu; D_800C3A70 is a
 * struct array so the loops keep a scaled index with symbol addressing; the
 * mode-4 sel is an if/else (the single-if form lets reorg steal the actor
 * mask into the first branch's delay slot); the first follow-up sets ret = 1
 * once after its inner if/else (both func_80086C88 calls then cross-jump and
 * reorg moves ret = 1 into the shared jal delay slot), the other two set it
 * after their calls (sched2 hoists it into the lbu delay). */
extern u8 D_800CCE42[];
extern u8* D_800D2D28;
extern s32 D_800CCB34__a[] asm("D_800CCB34");
extern void* func_8008AC00(s32 size);
extern s32 func_80076A10(s32 index, u8* polys, s16 x, s16 y);
extern s32 func_80086C88(u8 actor, s32 slot, s32 row, u8 id, u32* buf);
extern void func_800716D8(void);

s32 func_80086F98(u8 index, u8 actor) {
    u8 sel = (index + 1) * 3;
    s32 n = 0;
    s32 ret = 0;
    u8 id;
    u8 glyph;
    s32 i;
    s32 j;

    if (!(D_800CCE42[(D_800C3EAC + (actor << 6))[0x3C] * 0x170] & 0x80) && D_800D2C34[0] != 4) {
        D_800C3EAC[0x2DC] = D_800C34CC[index];
        return 1;
    }
    if ((D_800D2C34[0] != 0 && D_800D2C34[0] - 1 >= index) || D_800C3EAC[0x2E7] != 0) {
        for (j = 0; j < 3; j++) {
            D_800C3A70[j].buf = (u32)func_8008AC00(0x1E);
        }
        (D_800C3EAC + D_800C3EAC[0x2D6])[0x2CC] = index;
        D_800C3EAC[0x2D6]++;
        if (D_800C3EAC[0x2E4] != 0) {
            (D_800C3EAC + D_800C3EAC[0x2D6])[0x2CB] = 0xFF;
            (D_800C3EAC + D_800C3EAC[0x2D6])[0x2CB] = index;
            sel = index + (D_800C3EAC[0x2CC] + 1) * 3;
        }
        D_800D2DB4[0x5D81] = 0;
        D_800D2DB4[0x5D80] = 0;
        D_800D2DB4[0x5D82] = 0;
        for (i = 0; i < D_800C3EAC[0x2D6]; i++) {
            if ((D_800C3EAC + i)[0x2CC] != 0xFF) {
                switch ((D_800C3EAC + i)[0x2CC]) {
                case 0:
                    glyph = 0x5E;
                    break;
                case 1:
                    glyph = 0x5F;
                    break;
                case 2:
                    glyph = 0x5D;
                    break;
                }
                D_800D2DB4[0x5D81] += func_80076A10(glyph, D_800D2DB4 + (D_800D2DB4[0x5D81] * 0x50 + 0x5640),
                                                    i * 16 + 0x80, 0xD0 - n * 16);
            }
        }
        if (D_800D2C34[0] == 4) {
            if (index == 0xFF) {
                sel = 12;
            } else {
                sel = index + 12;
            }
        }
        id = sel;
        if ((u16)func_80089C6C(*(u16*)(g_GameState + 0x16C4 + D_800D2D24[actor] * 32), D_800C34CC[id]) != 0) {
            if (D_800C3EAC[0x2E4] == 0) {
                if (*(u16*)(D_800C3EAC + (actor << 6) + 0x1C) == 0) {
                    D_800D2DB4[0x5D81] += func_80076A10(8, D_800D2DB4 + (D_800D2DB4[0x5D81] * 0x50 + 0x5640),
                                                        i * 16 + 0x80, 0xD0 - n * 16);
                    n = func_80086C88(actor, n, i - 1, id, &D_800C3A70[n / 2].buf);
                }
            } else {
                n = func_80086C88(actor, n, i - 1, id, &D_800C3A70[n / 2].buf);
            }
            ret = 1;
        }
        if (D_800C3EAC[0x2E4] != 0 && ret != 0) {
            ret = 0;
            if (D_800C3EAC[0x2CC] == 0xFF) {
                sel = index + 12;
            }
            D_800D2DB4[0x5D90] = *(u8*)D_800CCB34__a;
            D_800C3EAC[0x2DC] = D_800C34CC[sel];
            D_800D2DB4[0x5D8F] = *(u8*)D_800CCB34__a;
            D_800D2DB4[0x5D91] = *(u8*)D_800CCB34__a;
            D_800D2D28[0xA8] = 1;
            func_800716D8();
            for (j = 0; j < 3; j++) {
                HeapFree((void*)D_800C3A70[j].buf);
            }
        } else {
            D_800C3EAC[0x2DC] = index;
            if (D_800D2C34[0] != 4) {
                id = (index + 1) * 3 + 1;
            } else {
                id = 13;
            }
            if ((u16)func_80089C6C(*(u16*)(g_GameState + 0x16C4 + D_800D2D24[actor] * 32), D_800C34CC[id]) != 0 &&
                *(u16*)(D_800C3EAC + (actor << 6) + 0x1E) == 0) {
                D_800D2DB4[0x5D81] += func_80076A10(9, D_800D2DB4 + (D_800D2DB4[0x5D81] * 0x50 + 0x5640),
                                                    i * 16 + 0x84, 0xD0 - n * 16);
                n = func_80086C88(actor, n, i, id, &D_800C3A70[n / 2].buf);
                ret = 1;
            }
            if (D_800D2C34[0] != 4) {
                id = (index + 1) * 3 + 2;
            } else {
                id = 14;
            }
            if ((u16)func_80089C6C(*(u16*)(g_GameState + 0x16C4 + D_800D2D24[actor] * 32), D_800C34CC[id]) != 0 &&
                *(u16*)(D_800C3EAC + (actor << 6) + 0x20) == 0) {
                D_800D2DB4[0x5D81] += func_80076A10(7, D_800D2DB4 + (D_800D2DB4[0x5D81] * 0x50 + 0x5640),
                                                    i * 16 + 0x88, 0xD0 - n * 16);
                func_80086C88(actor, n, i, id, &D_800C3A70[n / 2].buf);
                ret = 1;
            }
            D_800D2DB4[0x5D90] = *(u8*)D_800CCB34__a;
            D_800D2DB4[0x5D8F] = *(u8*)D_800CCB34__a;
            D_800D2DB4[0x5D91] = *(u8*)D_800CCB34__a;
            D_800D2D28[0xA8] = 1;
            func_800716D8();
            for (j = 0; j < 3; j++) {
                HeapFree((void*)D_800C3A70[j].buf);
            }
        }
    } else {
        D_800C3EAC[0x2DC] = index;
        ret = 1;
    }
    return ret;
}
/* func_800877E0.s, byte-exact: fill the nine 6-byte entries after
 * D_800C48E8 for actor a against b.  All are set to 0xFFFF/0xFFFF; entry 0
 * takes a's +0xA/+0xC halfwords (28-byte rows at D_800C3EB4) with flag 0.
 * Entries 1..7 come from the D_800D3364 table: the byte grid at +0x140,
 * indexed by the +0 bytes of a's and b's rows (64 bytes per a value, 8 per
 * b value), gives per slot either 0xFF (stop) or a byte whose low 3 bits
 * pick a 32-byte table entry (its two halfwords) and whose bit 7 is the
 * entry's flag.  Returns 1 if D_800D2D24[a] is 4, else 0.
 * Codegen:
 * - The grid address is rebuilt for each of the three reads, and each
 *   read reloads a's and b's row bytes after the previous entry store, as
 *   in retail.  The entry stores go through the D_800C48E8 block view;
 *   their relocations name D_800C48E8 + 4.. where splat says
 *   D_800C48EC/EE/F0, and the bytes are the same.
 * - Each address is built as `x = (s32)(t + <a's byte> * 64);` then
 *   `row = (u8*)(<b's byte> * 8 + x + 0x140);`.  With a's part as its own
 *   statement a's byte loads first, and with b's term first in the int
 *   sum the sum lands in b's register, as in retail.  As one expression,
 *   the int form loads b's byte first and the pointer form puts the sum
 *   in a's register. */
typedef struct {
    u16 a;
    u16 b;
    u8 flag;
    u8 pad;
} BattleEntry6;
typedef struct {
    u16 a;
    u16 b;
    u8 pad[28];
} BattleTblEntry;
typedef struct {
    BattleTblEntry e[8];
    u8 pad[0x40];
    u8 grid[0][8][8];
} BattleTbl;
typedef struct {
    u16 mask;
    u16 pad;
    BattleEntry6 e[9];
} BattleEntryBlock;
extern BattleEntryBlock D_800C48E8__blk asm("D_800C48E8");
extern u8 D_800C3EB4__rows[][28] asm("D_800C3EB4");
extern u16 D_800C3EBE__rows[][14] asm("D_800C3EBE");
extern u16 D_800C3EC0__rows[][14] asm("D_800C3EC0");
extern u8* D_800D3364;
extern u8 D_800D2D24[];

s32 func_800877E0(u8 a, u8 b) {
    s32 ret = 0;
    s32 i;
    u8* t;
    u8* row;
    s32 x;

    for (i = 0; i < 9; i++) {
        D_800C48E8__blk.e[i].a = 0xFFFF;
        D_800C48E8__blk.e[i].b = 0xFFFF;
    }
    D_800C48E8__blk.e[0].a = D_800C3EBE__rows[a][0];
    D_800C48E8__blk.e[0].b = D_800C3EC0__rows[a][0];
    D_800C48E8__blk.e[0].flag = 0;
    for (i = 1; i < 8; i++) {
        t = D_800D3364;
        x = (s32)(t + D_800C3EB4__rows[a][0] * 64);
        row = (u8*)(D_800C3EB4__rows[b][0] * 8 + x + 0x140);
        if (row[i] == 0xFF) {
            break;
        }
        D_800C48E8__blk.e[i].a = ((BattleTbl*)t)->e[row[i] & 7].a;
        x = (s32)(t + D_800C3EB4__rows[a][0] * 64);
        row = (u8*)(D_800C3EB4__rows[b][0] * 8 + x + 0x140);
        D_800C48E8__blk.e[i].b = ((BattleTbl*)t)->e[row[i] & 7].b;
        x = (s32)(t + D_800C3EB4__rows[a][0] * 64);
        row = (u8*)(D_800C3EB4__rows[b][0] * 8 + x + 0x140);
        D_800C48E8__blk.e[i].flag = row[i] & 0x80;
    }
    if (D_800D2D24[a] == 4) {
        ret = 1;
    }
    return ret;
}
#endif
#include "main38.c"
