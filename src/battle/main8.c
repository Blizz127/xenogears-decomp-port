#include "common.h"
#ifndef XENO_PC_PORT
#include "psyq/libgpu.h"
#endif


#ifndef XENO_PC_PORT
extern u8* D_800D2F5C;
extern void func_80026338(u8*, s32, s32*, s32*, s32*, s32*, s32*, s32*);
extern u8* D_800C3EA4;
extern u8* D_800D2D28;

#define M8_STRIP(ui, k) (((RECT*)((ui) + 0x8950))[k])
#define M8_WORD(base, off) ((s32*)((base) + (off)))
/* D_800D2F5C entry `id` into the six words at D_800C3EA4 + off. */
#define M8_FETCH(id, off)                                                     \
    func_80026338(D_800D2F5C, id, M8_WORD(D_800C3EA4, (off)),                \
                  M8_WORD(D_800C3EA4, (off) + 4), M8_WORD(D_800C3EA4, (off) + 8), \
                  M8_WORD(D_800C3EA4, (off) + 0xC), M8_WORD(D_800C3EA4, (off) + 0x10), \
                  M8_WORD(D_800C3EA4, (off) + 0x14))
#define M8_TPAGE(ui)                                                          \
    GetTPage(*(s32*)((ui) + 0xA238), 0, *(s32*)((ui) + 0xA244), *(s32*)((ui) + 0xA248))

/* func_80077990.s: set up four 0xC6 x 1 VRAM strips in the D_800C3EA4 block
 * (RECTs at +0x8950, y from the +0xA240 halfword) and StoreImage each one
 * four times into its 0x630-byte buffer (+0x8970 + k * 0x630, 0x18C-byte
 * steps).  Fill the six-word records at +0xA24C from D_800D2F5C entries
 * 0x4B, 0x50, 0x4D and 0x4E, set six RECTs at D_800D2D28 from them, and give
 * the four DR_MODEs at +0x8908 the +0xA238 texture page with texture window
 * D_800D2D28[1] (first two) or D_800D2D28[0] (last two). */
void func_80077990(void) {
    u8* ui = D_800C3EA4;
    u8* buf0 = ui + 0x8970;
    u8* buf1 = ui + 0x8FA0;
    u8* buf2 = ui + 0x95D0;
    u8* buf3 = ui + 0x9C00;
    RECT* r;
    u8* p;

    M8_STRIP(ui, 0).x = 1;
    M8_STRIP(ui, 0).y = *(u16*)(ui + 0xA240) - 1;
    M8_STRIP(ui, 0).w = 0xC6;
    M8_STRIP(ui, 0).h = 1;
    M8_STRIP(ui, 1).x = 1;
    M8_STRIP(ui, 1).y = *(u16*)(ui + 0xA240);
    M8_STRIP(ui, 1).w = 0xC6;
    M8_STRIP(ui, 1).h = 1;
    M8_STRIP(ui, 2).x = 1;
    M8_STRIP(ui, 2).y = *(u16*)(ui + 0xA240) - 2;
    M8_STRIP(ui, 2).w = 0xC6;
    M8_STRIP(ui, 2).h = 1;
    M8_STRIP(ui, 3).x = 1;
    M8_STRIP(ui, 3).y = *(u16*)(ui + 0xA240) - 3;
    M8_STRIP(ui, 3).w = 0xC6;
    M8_STRIP(ui, 3).h = 1;
    StoreImage(&M8_STRIP(D_800C3EA4, 0), (u_long*)buf0);
    StoreImage(&M8_STRIP(D_800C3EA4, 1), (u_long*)buf1);
    StoreImage(&M8_STRIP(D_800C3EA4, 2), (u_long*)buf2);
    StoreImage(&M8_STRIP(D_800C3EA4, 3), (u_long*)buf3);
    StoreImage(&M8_STRIP(D_800C3EA4, 0), (u_long*)(buf0 + 0x18C));
    StoreImage(&M8_STRIP(D_800C3EA4, 1), (u_long*)(buf1 + 0x18C));
    StoreImage(&M8_STRIP(D_800C3EA4, 2), (u_long*)(buf2 + 0x18C));
    StoreImage(&M8_STRIP(D_800C3EA4, 3), (u_long*)(buf3 + 0x18C));
    StoreImage(&M8_STRIP(D_800C3EA4, 0), (u_long*)(buf0 + 0x318));
    StoreImage(&M8_STRIP(D_800C3EA4, 1), (u_long*)(buf1 + 0x318));
    StoreImage(&M8_STRIP(D_800C3EA4, 2), (u_long*)(buf2 + 0x318));
    StoreImage(&M8_STRIP(D_800C3EA4, 3), (u_long*)(buf3 + 0x318));
    StoreImage(&M8_STRIP(D_800C3EA4, 0), (u_long*)(buf0 + 0x4A4));
    StoreImage(&M8_STRIP(D_800C3EA4, 1), (u_long*)(buf1 + 0x4A4));
    StoreImage(&M8_STRIP(D_800C3EA4, 2), (u_long*)(buf2 + 0x4A4));
    StoreImage(&M8_STRIP(D_800C3EA4, 3), (u_long*)(buf3 + 0x4A4));
    M8_FETCH(0x4B, 0xA24C);
    M8_FETCH(0x50, 0xA264);
    M8_FETCH(0x4D, 0xA27C);
    M8_FETCH(0x4E, 0xA294);
    r = (RECT*)D_800D2D28;
    p = D_800C3EA4;
    r[0].x = r[0].y = 0;
    r[0].w = r[0].h = 0x100;
    *(s32*)(p + 0xA25C) = 0x3C0;
    *(s32*)(p + 0xA274) = 0x3C8;
    *(s32*)(p + 0xA260) = 0x34;
    *(s32*)(p + 0xA278) = 0x34;
    r[1].x = (*(u16*)(p + 0xA244) & 0x3F) * 2;
    r[1].y = *(u16*)(p + 0xA248);
    r[1].w = 0x100;
    r[1].h = 0x100;
    r[2].x = (*(u16*)(p + 0xA25C) & 0x3F) * 2;
    r[2].y = *(u16*)(p + 0xA260);
    r[2].w = 8;
    r[2].h = 0x10;
    r[3].x = (*(u16*)(p + 0xA274) & 0x3F) * 2;
    r[3].y = *(u16*)(p + 0xA278);
    r[3].w = 8;
    r[3].h = 0x10;
    r[4].x = (*(u16*)(p + 0xA28C) & 0x3F) * 2 + 0xE;
    r[4].y = *(u16*)(p + 0xA290);
    r[4].w = 0x10;
    r[4].h = 8;
    r[5].x = (*(u16*)(p + 0xA2A4) & 0x3F) * 2 + 0xE;
    r[5].y = *(u16*)(p + 0xA2A8);
    r[5].w = 0x10;
    r[5].h = 8;
    SetDrawMode((DR_MODE*)(D_800C3EA4 + 0x8908), 0, 0, M8_TPAGE(D_800C3EA4),
                (RECT*)(D_800D2D28 + 8));
    SetDrawMode((DR_MODE*)(D_800C3EA4 + 0x8914), 0, 0, M8_TPAGE(D_800C3EA4),
                (RECT*)(D_800D2D28 + 8));
    SetDrawMode((DR_MODE*)(D_800C3EA4 + 0x8920), 0, 0, M8_TPAGE(D_800C3EA4), (RECT*)D_800D2D28);
    SetDrawMode((DR_MODE*)(D_800C3EA4 + 0x892C), 0, 0, M8_TPAGE(D_800C3EA4), (RECT*)D_800D2D28);
}
#undef M8_STRIP
#undef M8_WORD
#undef M8_FETCH
#undef M8_TPAGE
extern u16 g_SystemPalette1;
extern u16 g_SystemPalette2;

/* func_800780A8.s: set up the two POLY_FT4 at the start of p -- grey
 * (0x80) colour, opaque, unshaded, the system palette picked by n & 1 (also
 * kept at p[0x5C]) and the 0x3C0 texture page at y = n / 2 * 13 -- and clear
 * p[0x5D].  Retail hoists nothing out of the loop and compares the pointers
 * signed, which the goto loop reproduces. */
void func_800780A8(u8* p, s32 n) {
    u8 grey = 0x80;
    s32 half = n / 2;
    POLY_FT4* prim;

    prim = (POLY_FT4*)p;
loop:
    SetPolyFT4(prim);
    prim->r0 = grey;
    prim->g0 = grey;
    prim->b0 = grey;
    SetSemiTrans(prim, 0);
    SetShadeTex(prim, 1);
    p[0x5C] = n & 1;
    prim->clut = p[0x5C] ? g_SystemPalette2 : g_SystemPalette1;
    prim->tpage = GetTPage(0, 0, 0x3C0, half * 13);
    prim++;
    if ((s32)prim < (s32)(p + 0x50)) {
        goto loop;
    }
    p[0x5D] = 0;
}
/* Battle gauge strip record (0x60 bytes): two POLY_FT4 set up by
 * func_800780A8, the strip's VRAM rect and its 0x39-byte buffer. */
typedef struct {
    u8 prims[0x50];
    RECT rect;     /* 0x50 */
    void* image;   /* 0x58 */
    u8 pad5C[4];
} M8Gauge;
extern M8Gauge D_800D36C8[];
extern void func_8008F8F4(u8 index, u16 x, u16 y, u16 w, u16 h, u8 animate, u8 refresh);
extern void* func_8008AC00(s32 size);

/* func_8007819C.s: open gauge windows 5 and 4, then set up the eight strip
 * records in pairs: each pair shares one func_8008AC00 buffer and one rect
 * (x 0x3C0, y = pair * 13, 0x3C x 0xD).  The buffer goes through a temp:
 * the chained `a = b = f()` form makes loop.c pick different givs. */
void func_8007819C(void) {
    s32 i;
    RECT* r;
    void* buf;

    func_8008F8F4(5, 8, 0x2A, 0x70, 0x12, 0, 0);
    D_800D2D28[0xB5] = 0;
    func_8008F8F4(4, 0x20, 0xC8, 0xF4, 0x12, 0, 0);
    D_800D2D28[0xB4] = 0;
    for (i = 0; i < 8; i += 2) {
        buf = func_8008AC00(0x39);
        D_800D36C8[i].image = buf;
        D_800D36C8[i + 1].image = buf;
        r = &D_800D36C8[i].rect;
        r->x = 0x3C0;
        r->y = i / 2 * 13;
        r->w = 0x3C;
        r->h = 0xD;
        D_800D36C8[i + 1].rect = *r;
        func_800780A8((u8*)&D_800D36C8[i], i);
        func_800780A8((u8*)&D_800D36C8[i + 1], i + 1);
    }
}
extern u8 D_800D2D24[];
extern u8 D_800D3294;

/* func_80078310.s: for each i in 0..2 whose D_800D2D24[i] is not 0x7F, open
 * the TIM at base + kind * 0x460 (kind 0xB for i 1 and 2 while D_800D3294 is
 * set), fetch the six words of D_800D2F5C entry arg1 + i, place the CLUT at
 * words 2/3 and the pixels at words 4/5 (x shifted by i * 6), and upload both.
 * The words sit in one flat array, as retail's per-word offset registers
 * show. */
void func_80078310(u8* base, u8 arg1) {
    TIM_IMAGE tim;
    s32 slot[18];
    s32 i;
    u8 kind;

    for (i = 0; i < 3; i++) {
        if (D_800D2D24[i] != 0x7F) {
            kind = D_800D2D24[i];
            if (D_800D3294 != 0 && (i == 1 || i == 2)) {
                kind = 0xB;
            }
            OpenTIM((u_long*)(base + kind * 0x460));
            ReadTIM(&tim);
            func_80026338(D_800D2F5C, arg1 + i, &slot[i * 6 + 0], &slot[i * 6 + 1],
                          &slot[i * 6 + 2], &slot[i * 6 + 3], &slot[i * 6 + 4],
                          &slot[i * 6 + 5]);
            tim.crect->x = slot[i * 6 + 2];
            tim.crect->y = slot[i * 6 + 3];
            tim.prect->x = slot[i * 6 + 4] + i * 6;
            tim.prect->y = slot[i * 6 + 5];
            LoadImage(tim.crect, tim.caddr);
            LoadImage(tim.prect, tim.paddr);
            DrawSync(0);
        }
    }
}
extern u8 D_800D2DCC[];
extern u8 D_800D2DE4[];
extern u8 D_800D2DF0[];
extern u16 func_80098AF8(s32 idx, s32 flag);

/* Refresh the 11 slot words: +0x16 and +0 halfwords of D_800D2DF0 get
 * func_80098AF8(i, 0) for a live slot (D_800D2DCC[i]) and 0xFF otherwise
 * (one chained store per branch, as retail duplicates them); clear
 * D_800D2DE4[i], the +0x2C halfword and out[i]. */
void func_80078508(void* pBuffer) {
    u8* out = (u8*)pBuffer;
    s32 i;

    for (i = 0; i < 11; i++) {
        if (D_800D2DCC[i] != 0) {
            *(u16*)(D_800D2DF0 + i * 2) = *(u16*)(D_800D2DF0 + 0x16 + i * 2) = func_80098AF8(i, 0);
        } else {
            *(u16*)(D_800D2DF0 + i * 2) = *(u16*)(D_800D2DF0 + 0x16 + i * 2) = 0xFF;
        }
        D_800D2DE4[i] = 0;
        *(u16*)(D_800D2DF0 + 0x2C + i * 2) = 0;
        out[i] = 0;
    }
}
extern u8* D_800C3EAC;
extern u8 D_800D2E5C__e[][8] asm("D_800D2E5C");
extern u8 D_800C400B[];
extern u8 D_800C4022[];

/* func_800785D4.s, byte-exact: append a 72-byte entry at the D_800C3EAC
 * +0x2DA cursor (D_800C400B = a, D_800C4022 = the halfword at bytes +4/+5
 * of the 8-byte D_800D2E5C entry b), then bump the cursor.
 * Codegen: the halfword is read through an 8-byte entry view of
 * D_800D2E5C.  Its byte addresses are symbol + offset CONSTs with no
 * symbolic term for sched's alias test, so both loads stay ahead of the
 * D_800C400B byte store as in retail; plain D_800D2E60/D_800D2E61 reads
 * sink below it.  The high byte is read first, and `p` reloads D_800C3EAC
 * after the byte store as in func_8007887C.  Matches under gcc 2.6.3
 * (main8's preset), 2.6.0 and 2.7.2. */
void func_800785D4(u8 a, u8 b) {
    u16 word = (D_800D2E5C__e[b][5] << 8) | D_800D2E5C__e[b][4];
    u8* p;

    D_800C400B[D_800C3EAC[0x2DA] * 72] = a;
    p = D_800C3EAC;
    *(u16*)(D_800C4022 + p[0x2DA] * 72) = word;
    p[0x2DA] = p[0x2DA] + 1;
}
/* func_80078658.s, byte-exact: while fewer than nine strips are in use,
 * render string D_800D2E5F[a * 8] of the D_800C3DDC table into strip
 * D_800C3E8C.  Strips are 96-byte records at D_800D3718 (rect at +0, work
 * word at +8, width byte at +0xE).  Upload the strip (func_800769E8), then
 * append a 0xFA entry with colour b and the strip index, and advance both
 * the strip count and the entry cursor.  Codegen: the work word is read
 * through a 96-byte record view of D_800D3720.  With
 * *(void**)(D_800D3720 + idx * 96) the folded address temp stays a dead
 * pseudo with a stack slot (frame 0x20, retail 0x18).  The tail reads the
 * count into its own n and computes the entry pointer before the count
 * store, as retail orders it (decomp-permuter hint, natural form). */
extern u8* D_800C3DDC;
extern u8 D_800C3E8C;
extern u8* D_800C3EAC;
extern u8 D_800C400B[];
extern u8 D_800C402F[];
extern u8 D_800C4022[];
extern u8 D_800D2E5F[];
extern u8 D_800D3718[];
extern u8 D_800D3726[];
extern u8* GetStringEntry(u8* table, u8 id);
extern s32 SystemRenderStringEntry(void* pString, void* pWork, s32 height, s32 flag);
extern void func_800769E8(u8* rect, void* work);
typedef struct { void* work; u8 pad[92]; } Strip96;
extern Strip96 D_800D3720__s[] asm("D_800D3720");

void func_80078658(u8 a, u8 b) {
    u32 strip;
    u8* p;
    u16* dst;
    u32 n;

    if (D_800C3E8C >= 9) {
        return;
    }
    D_800D3726[D_800C3E8C * 96] = SystemRenderStringEntry(GetStringEntry(D_800C3DDC, D_800D2E5F[(u32)a << 3]),
                                                          D_800D3720__s[D_800C3E8C].work, 0x39,
                                                          D_800C3E8C & 1);
    strip = D_800C3E8C;
    func_800769E8(D_800D3718 + strip * 96, D_800D3720__s[strip].work);
    D_800C400B[D_800C3EAC[0x2DA] * 72] = b;
    D_800C402F[D_800C3EAC[0x2DA] * 72] = 0xFA;
    p = D_800C3EAC;
    n = D_800C3E8C;
    dst = (u16*)(D_800C4022 + p[0x2DA] * 72);
    D_800C3E8C = n + 1;
    *dst = (u16)n;
    p[0x2DA] = p[0x2DA] + 1;
}
#endif
#ifndef XENO_PC_PORT
extern u8* D_800C3EAC;
extern u8 D_800C400B[];
extern u8 D_800C402F[];
extern u8 D_800C4022[];
#endif
/* Byte-exact under gcc 2.6.3 (BattleGcc263); the port compiles the same
 * body (shaped like the matched func_8007887C). */
/* func_800787E0.s: append a 0xF7 entry -- colour b, halfword a -- at the
 * D_800C3EAC+0x2DA cursor and bump the cursor. */
void func_800787E0(u8 a, u8 b) {
    u8* p;

    D_800C400B[D_800C3EAC[0x2DA] * 72] = b;
    D_800C402F[D_800C3EAC[0x2DA] * 72] = 0xF7;
    p = D_800C3EAC;
    *(u16*)(D_800C4022 + p[0x2DA] * 72) = a;
    p[0x2DA] = p[0x2DA] + 1;
}
#ifndef XENO_PC_PORT
extern u8 D_800C3E8C;
#endif
#ifndef XENO_PC_PORT
extern u8 D_800D2DCC[];
#endif
#ifndef XENO_PC_PORT
extern u8 D_800D2DE4[];
#endif
#ifndef XENO_PC_PORT
extern u8 D_800D2DF0[];
#endif
#ifndef XENO_PC_PORT
extern u8 D_800D2E60[];
#endif
#ifndef XENO_PC_PORT
extern u8 D_800D2E61[];
#endif
#ifndef XENO_PC_PORT
extern u8* D_800C3DDC;
#endif
#ifndef XENO_PC_PORT
extern u8 D_800D3718[];
#endif
#ifndef XENO_PC_PORT
extern u8 D_800D3720[];
#endif
#ifndef XENO_PC_PORT
extern u8 D_800D3726[];
#endif
#ifndef XENO_PC_PORT
extern u8* D_800C3EAC;
#endif
#ifndef XENO_PC_PORT
extern u8 D_800C400B[];
#endif
#ifndef XENO_PC_PORT
extern u8 D_800C402F[];
#endif
#ifndef XENO_PC_PORT
extern u8 D_800C4022[];
#endif
/* func_8007887C.s: append one 72-byte entry at the D_800C3EAC+0x2DA cursor --
 * colour a0, marker 0xF8 and the halfword D_800C3E8C - 1 -- then bump the
 * cursor.  Skipped entirely while D_800C3E8C is 0. */
void func_8007887C(u8 a0) {
    if (D_800C3E8C != 0) {
        u8* p;

        D_800C400B[D_800C3EAC[0x2DA] * 72] = a0;
        D_800C402F[D_800C3EAC[0x2DA] * 72] = 0xF8;
        p = D_800C3EAC;
        *(u16*)(D_800C4022 + p[0x2DA] * 72) = D_800C3E8C - 1;
        p[0x2DA] = p[0x2DA] + 1;
    }
}


#ifndef XENO_PC_PORT
extern u8 D_800D2E5F[];
#endif
extern void func_80078658(u8 a, u8 b);
extern void func_800787E0(u8 a, u8 b);
extern void func_8007887C(u8 a);


/* func_8007893C.s */
void func_8007893C(u8 a, u8 b) {
    if (*(u8*)(D_800D2E5F + ((u32)a << 3)) == 0) {
        return;
    }
    func_80078658(a & 0xFF, b & 0xFF);
    func_800787E0(0x1E, b & 0xFF);
    func_8007887C(b & 0xFF);
}


#ifdef XENO_PC_PORT
/* Port-side bodies transcribed from the retail listings; the matching build
 * still assembles the INCLUDE_ASM bytes above. */
#include "battle_guest_call.h"
extern void* GetStringEntry(void* pTable, s32 index);
extern s32 SystemRenderStringEntry(void* pString, void* pWork, s32 height, s32 flag);
extern void func_800769E8(void* pRect, void* pData);

/* func_80078508.s: reset the eleven per-slot records.  Live slots (non-zero
 * D_800D2DCC byte) take their current value from func_80098AF8(slot, 0) into
 * both halfword rows at D_800D2DF0 and +0x16; empty slots get 0xFF there.
 * Every slot then clears D_800D2DE4[i], the +0x2C halfword row and the
 * caller's byte buffer. */
void func_80078508(void* pBuffer) {
    u8* out = (u8*)pBuffer;
    s32 i;

    for (i = 0; i < 11; i++) {
        u16 v;

        if (D_800D2DCC[i] != 0) {
            /* func_80098AF8 (main53) is matched C but not adopted on the host
             * yet: run the retail bytes. */
            v = (u16)BATTLE_GUEST_CALL(0x80098AF8u, (u32)i, 0u);
        } else {
            v = 0xFF;
        }
        *(u16*)(D_800D2DF0 + 0x16 + i * 2) = v;
        *(u16*)(D_800D2DF0 + i * 2) = v;
        D_800D2DE4[i] = 0;
        *(u16*)(D_800D2DF0 + 0x2C + i * 2) = 0;
        out[i] = 0;
    }
}

/* func_800785D4.s: append a 72-byte entry at the D_800C3EAC+0x2DA cursor --
 * colour a0 and the D_800D2E60/D_800D2E61 halfword of row b -- then bump the
 * cursor.  Unlike its siblings there is no marker byte. */
void func_800785D4(u8 a, u8 b) {
    u32 row = (u32)b << 3;
    u16 word = D_800D2E60[row] | (D_800D2E61[row] << 8);

    D_800C400B[D_800C3EAC[0x2DA] * 72] = a;
    *(u16*)(D_800C4022 + D_800C3EAC[0x2DA] * 72) = word;
    D_800C3EAC[0x2DA] = D_800C3EAC[0x2DA] + 1;
}

/* func_80078658.s: while fewer than nine strips are in use, render string
 * D_800D2E5F[a*8] of the D_800C3DDC table into strip D_800C3E8C (96-byte
 * records at D_800D3718: rect at +0, work word at +8, width byte at +0xE),
 * upload it, then append a 0xFA entry with colour b and the strip index and
 * advance both the strip count and the entry cursor. */
void func_80078658(u8 a, u8 b) {
    u32 strip;
    u8* p;

    if (D_800C3E8C >= 9) {
        return;
    }
    {
        void* entry = GetStringEntry(D_800C3DDC, D_800D2E5F[(u32)a << 3]);
        s32 width;

        strip = D_800C3E8C;
        width = SystemRenderStringEntry(
            entry, PSX_ADDR(*(u32*)(D_800D3720 + strip * 96)), 0x39, strip & 1);
        D_800D3726[D_800C3E8C * 96] = (u8)width;
    }
    strip = D_800C3E8C;
    func_800769E8(D_800D3718 + strip * 96,
                  PSX_ADDR(*(u32*)(D_800D3720 + strip * 96)));
    D_800C400B[D_800C3EAC[0x2DA] * 72] = b;
    D_800C402F[D_800C3EAC[0x2DA] * 72] = 0xFA;
    p = D_800C3EAC;
    strip = D_800C3E8C;
    D_800C3E8C = strip + 1;
    *(u16*)(D_800C4022 + p[0x2DA] * 72) = (u16)strip;
    p[0x2DA] = p[0x2DA] + 1;
}

#endif
