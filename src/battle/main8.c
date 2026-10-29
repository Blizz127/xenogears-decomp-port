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

#ifdef XENO_PC_PORT
/* h-lift: instruction-faithful port bodies of functions the battle still
 * interpreted, generated from the retail listings by
 * tools/scripts/battle_lift_asm.py --jal-service (one statement per MIPS
 * instruction; include/battle_lift.h has the register, memory, frame and
 * call contract; every call goes through the interpreter's own jal path)
 * and committed as generated.  The matched C above stays the matching
 * build's.  Regenerate with
 *   tools/scripts/battle_lift_asm.py asm/battle/matchings/<tu>/<fn>.s --args N [--void] --jal-service
 */
#include "battle_lift.h"

/* func_800780A8.s (61 instructions, frame 0x28; lifted with --args 2 --void).
 * Callees through the jal service: GetTPage, SetPolyFT4, SetSemiTrans, SetShadeTex. */
void func_800780A8(u32 arg0, u32 arg1) {
    static const char FN[] = "func_800780A8";
    u32 r_at = 0, r_v0 = 0, r_v1 = 0, r_a0 = 0, r_a1 = 0, r_a2 = 0, r_a3 = 0, r_t0 = 0, r_t1 = 0, r_t2 = 0, r_t3 = 0, r_t4 = 0, r_t5 = 0, r_t6 = 0, r_t7 = 0, r_s0 = 0, r_s1 = 0, r_s2 = 0, r_s3 = 0, r_s4 = 0, r_s5 = 0, r_s6 = 0, r_s7 = 0, r_t8 = 0, r_t9 = 0, r_k0 = 0, r_k1 = 0, r_gp = 0, r_sp = 0, r_fp = 0, r_ra = 0;
    u32 hi = 0, lo = 0, bl_saved;
    r_sp = bl_enter(FN, 0x28u, &bl_saved);
    r_a0 = arg0;
    r_a1 = arg1;
    r_sp = r_sp + 0xFFFFFFD8u; /* 800780A8 addiu $sp, $sp, -0x28 */
    bl_sw(r_sp + 0x14u, r_s1); /* 800780AC sw $s1, 0x14($sp) */
    r_s1 = r_a0 + 0u; /* 800780B0 addu $s1, $a0, $zero */
    bl_sw(r_sp + 0x1Cu, r_s3); /* 800780B4 sw $s3, 0x1C($sp) */
    r_s3 = r_a1 + 0u; /* 800780B8 addu $s3, $a1, $zero */
    bl_sw(r_sp + 0x20u, r_s4); /* 800780BC sw $s4, 0x20($sp) */
    r_s4 = 0u | 0x80u; /* 800780C0 ori $s4, $zero, 0x80 */
    r_v0 = r_s3 >> 31; /* 800780C4 srl $v0, $s3, 31 */
    r_v0 = r_s3 + r_v0; /* 800780C8 addu $v0, $s3, $v0 */
    bl_sw(r_sp + 0x18u, r_s2); /* 800780CC sw $s2, 0x18($sp) */
    r_s2 = BL_SRA(r_v0, 1); /* 800780D0 sra $s2, $v0, 1 */
    bl_sw(r_sp + 0x10u, r_s0); /* 800780D4 sw $s0, 0x10($sp) */
    r_s0 = r_s1 + 0u; /* 800780D8 addu $s0, $s1, $zero */
    bl_sw(r_sp + 0x24u, r_ra); /* 800780DC sw $ra, 0x24($sp) */
L_800780E0:
    /* 800780E0 jal SetPolyFT4 */
    r_a0 = r_s0 + 0u;
    bl_jal(FN, 0x80043CB0u, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* SetPolyFT4 */
    r_a0 = r_s0 + 0u; /* 800780E8 addu $a0, $s0, $zero */
    r_a1 = 0u + 0u; /* 800780EC addu $a1, $zero, $zero */
    bl_sb(r_s0 + 0x4u, r_s4); /* 800780F0 sb $s4, 0x4($s0) */
    bl_sb(r_s0 + 0x5u, r_s4); /* 800780F4 sb $s4, 0x5($s0) */
    /* 800780F8 jal SetSemiTrans */
    bl_sb(r_s0 + 0x6u, r_s4);
    bl_jal(FN, 0x80043BFCu, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* SetSemiTrans */
    r_a0 = r_s0 + 0u; /* 80078100 addu $a0, $s0, $zero */
    /* 80078104 jal SetShadeTex */
    r_a1 = 0u | 0x1u;
    bl_jal(FN, 0x80043C24u, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* SetShadeTex */
    r_v0 = r_s3 & 0x1u; /* 8007810C andi $v0, $s3, 0x1 */
    bl_sb(r_s1 + 0x5Cu, r_v0); /* 80078110 sb $v0, 0x5C($s1) */
    r_v0 = bl_lbu(r_s1 + 0x5Cu); /* 80078114 lbu $v0, 0x5C($s1) */
    ; /* 80078118 nop  */
    /* 8007811C beqz $v0, .L80078134 */
    { int c_ = (r_v0 == 0); r_a0 = 0u + 0u; if (c_) goto L_80078134; }
    r_v0 = 0x80060000u; /* 80078124 lui $v0, %hi(g_SystemPalette2) */
    r_v0 = bl_lhu(r_v0 + 0xFFFF9414u); /* 80078128 lhu $v0, %lo(g_SystemPalette2)($v0) */
    /* 8007812C j .L8007813C */
    ; goto L_8007813C;
L_80078134:
    r_v0 = 0x80060000u; /* 80078134 lui $v0, %hi(g_SystemPalette1) */
    r_v0 = bl_lhu(r_v0 + 0xFFFF95D4u); /* 80078138 lhu $v0, %lo(g_SystemPalette1)($v0) */
L_8007813C:
    r_a1 = 0u + 0u; /* 8007813C addu $a1, $zero, $zero */
    r_a3 = r_s2 << 1; /* 80078140 sll $a3, $s2, 1 */
    r_a3 = r_a3 + r_s2; /* 80078144 addu $a3, $a3, $s2 */
    r_a3 = r_a3 << 2; /* 80078148 sll $a3, $a3, 2 */
    r_a2 = 0u | 0x3C0u; /* 8007814C ori $a2, $zero, 0x3C0 */
    r_a3 = r_a3 + r_s2; /* 80078150 addu $a3, $a3, $s2 */
    /* 80078154 jal GetTPage */
    bl_sh(r_s0 + 0xEu, r_v0);
    bl_jal(FN, 0x80043A1Cu, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* GetTPage */
    bl_sh(r_s0 + 0x16u, r_v0); /* 8007815C sh $v0, 0x16($s0) */
    r_s0 = r_s0 + 0x28u; /* 80078160 addiu $s0, $s0, 0x28 */
    r_v0 = r_s1 + 0x50u; /* 80078164 addiu $v0, $s1, 0x50 */
    r_v0 = (u32)((s32)r_s0 < (s32)r_v0); /* 80078168 slt $v0, $s0, $v0 */
    /* 8007816C bnez $v0, .L800780E0 */
    { int c_ = (r_v0 != 0); ; if (c_) goto L_800780E0; }
    bl_sb(r_s1 + 0x5Du, 0u); /* 80078174 sb $zero, 0x5D($s1) */
    r_ra = bl_lw(r_sp + 0x24u); /* 80078178 lw $ra, 0x24($sp) */
    r_s4 = bl_lw(r_sp + 0x20u); /* 8007817C lw $s4, 0x20($sp) */
    r_s3 = bl_lw(r_sp + 0x1Cu); /* 80078180 lw $s3, 0x1C($sp) */
    r_s2 = bl_lw(r_sp + 0x18u); /* 80078184 lw $s2, 0x18($sp) */
    r_s1 = bl_lw(r_sp + 0x14u); /* 80078188 lw $s1, 0x14($sp) */
    r_s0 = bl_lw(r_sp + 0x10u); /* 8007818C lw $s0, 0x10($sp) */
    r_sp = r_sp + 0x28u; /* 80078190 addiu $sp, $sp, 0x28 */
    /* 80078194 jr $ra */
    ; goto bl_exit;
bl_exit:
    bl_leave(bl_saved);
    (void)hi; (void)lo; (void)r_at; (void)r_v1; (void)r_k0; (void)r_k1; (void)r_gp; (void)r_ra;
    (void)r_v0;
}
#endif /* XENO_PC_PORT */

#ifdef XENO_PC_PORT /* h-frontier lifted bodies */
/* Instruction-faithful port bodies (native-only frontier), generated by
 *   tools/scripts/battle_lift_asm.py asm/battle/matchings/<tu>/<fn>.s --args N [--void] --jal-service
 * (include/battle_lift.h; every call goes through the interpreter's own
 * jal path, so each callee gets the binding an interpreted jal gets).  The
 * matched C above stays the matching build's. */
#include "battle_lift.h"

/* func_8007819C.s (93 instructions; --args 0 --void), frontier step2.  Callees through the jal service: func_800780A8, func_8008AC00, func_8008F8F4. */
void func_8007819C(void) {
    static const char FN[] = "func_8007819C";
    u32 r_at = 0, r_v0 = 0, r_v1 = 0, r_a0 = 0, r_a1 = 0, r_a2 = 0, r_a3 = 0, r_t0 = 0, r_t1 = 0, r_t2 = 0, r_t3 = 0, r_t4 = 0, r_t5 = 0, r_t6 = 0, r_t7 = 0, r_s0 = 0, r_s1 = 0, r_s2 = 0, r_s3 = 0, r_s4 = 0, r_s5 = 0, r_s6 = 0, r_s7 = 0, r_t8 = 0, r_t9 = 0, r_k0 = 0, r_k1 = 0, r_gp = 0, r_sp = 0, r_fp = 0, r_ra = 0;
    u32 hi = 0, lo = 0, bl_saved;
    r_sp = bl_enter(FN, 0x40u, &bl_saved);
    r_sp = r_sp + 0xFFFFFFC0u; /* 8007819C addiu $sp, $sp, -0x40 */
    r_a0 = 0u | 0x5u; /* 800781A0 ori $a0, $zero, 0x5 */
    r_a1 = 0u | 0x8u; /* 800781A4 ori $a1, $zero, 0x8 */
    r_a2 = 0u | 0x2Au; /* 800781A8 ori $a2, $zero, 0x2A */
    r_a3 = 0u | 0x70u; /* 800781AC ori $a3, $zero, 0x70 */
    bl_sw(r_sp + 0x20u, r_s0); /* 800781B0 sw $s0, 0x20($sp) */
    r_s0 = 0u | 0x12u; /* 800781B4 ori $s0, $zero, 0x12 */
    bl_sw(r_sp + 0x38u, r_ra); /* 800781B8 sw $ra, 0x38($sp) */
    bl_sw(r_sp + 0x34u, r_s5); /* 800781BC sw $s5, 0x34($sp) */
    bl_sw(r_sp + 0x30u, r_s4); /* 800781C0 sw $s4, 0x30($sp) */
    bl_sw(r_sp + 0x2Cu, r_s3); /* 800781C4 sw $s3, 0x2C($sp) */
    bl_sw(r_sp + 0x28u, r_s2); /* 800781C8 sw $s2, 0x28($sp) */
    bl_sw(r_sp + 0x24u, r_s1); /* 800781CC sw $s1, 0x24($sp) */
    bl_sw(r_sp + 0x10u, r_s0); /* 800781D0 sw $s0, 0x10($sp) */
    bl_sw(r_sp + 0x14u, 0u); /* 800781D4 sw $zero, 0x14($sp) */
    /* 800781D8 jal func_8008F8F4 */
    bl_sw(r_sp + 0x18u, 0u);
    bl_jal(FN, 0x8008F8F4u, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* func_8008F8F4 */
    r_a0 = 0u | 0x4u; /* 800781E0 ori $a0, $zero, 0x4 */
    r_a1 = 0u | 0x20u; /* 800781E4 ori $a1, $zero, 0x20 */
    r_a2 = 0u | 0xC8u; /* 800781E8 ori $a2, $zero, 0xC8 */
    r_a3 = 0u | 0xF4u; /* 800781EC ori $a3, $zero, 0xF4 */
    r_s2 = 0u + 0u; /* 800781F0 addu $s2, $zero, $zero */
    r_s5 = 0x800D0000u; /* 800781F4 lui $s5, %hi(D_800D3718) */
    r_s5 = r_s5 + 0x3718u; /* 800781F8 addiu $s5, $s5, %lo(D_800D3718) */
    r_s4 = r_s5 + 0x10u; /* 800781FC addiu $s4, $s5, 0x10 */
    r_s1 = r_s5 + 0u; /* 80078200 addu $s1, $s5, $zero */
    r_v0 = 0x800D0000u; /* 80078204 lui $v0, %hi(D_800D2D28) */
    r_v0 = bl_lw(r_v0 + 0x2D28u); /* 80078208 lw $v0, %lo(D_800D2D28)($v0) */
    r_s3 = 0u | 0x60u; /* 8007820C ori $s3, $zero, 0x60 */
    bl_sb(r_v0 + 0xB5u, 0u); /* 80078210 sb $zero, 0xB5($v0) */
    bl_sw(r_sp + 0x10u, r_s0); /* 80078214 sw $s0, 0x10($sp) */
    bl_sw(r_sp + 0x14u, 0u); /* 80078218 sw $zero, 0x14($sp) */
    /* 8007821C jal func_8008F8F4 */
    bl_sw(r_sp + 0x18u, 0u);
    bl_jal(FN, 0x8008F8F4u, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* func_8008F8F4 */
    r_v0 = 0x800D0000u; /* 80078224 lui $v0, %hi(D_800D2D28) */
    r_v0 = bl_lw(r_v0 + 0x2D28u); /* 80078228 lw $v0, %lo(D_800D2D28)($v0) */
    r_s0 = 0u + 0u; /* 8007822C addu $s0, $zero, $zero */
    bl_sb(r_v0 + 0xB4u, 0u); /* 80078230 sb $zero, 0xB4($v0) */
L_80078234:
    /* 80078234 jal func_8008AC00 */
    r_a0 = 0u | 0x39u;
    bl_jal(FN, 0x8008AC00u, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* func_8008AC00 */
    r_at = 0x800D0000u; /* 8007823C lui $at, %hi(D_800D36C0) */
    r_at = r_at + r_s3; /* 80078240 addu $at, $at, $s3 */
    bl_sw(r_at + 0x36C0u, r_v0); /* 80078244 sw $v0, %lo(D_800D36C0)($at) */
    r_at = 0x800D0000u; /* 80078248 lui $at, %hi(D_800D3720) */
    r_at = r_at + r_s3; /* 8007824C addu $at, $at, $s3 */
    bl_sw(r_at + 0x3720u, r_v0); /* 80078250 sw $v0, %lo(D_800D3720)($at) */
    r_v0 = 0u | 0x3C0u; /* 80078254 ori $v0, $zero, 0x3C0 */
    r_v1 = r_s2 >> 31; /* 80078258 srl $v1, $s2, 31 */
    r_v1 = r_s2 + r_v1; /* 8007825C addu $v1, $s2, $v1 */
    r_v1 = BL_SRA(r_v1, 1); /* 80078260 sra $v1, $v1, 1 */
    bl_sh(r_s1, r_v0); /* 80078264 sh $v0, 0x0($s1) */
    r_v0 = r_v1 << 1; /* 80078268 sll $v0, $v1, 1 */
    r_v0 = r_v0 + r_v1; /* 8007826C addu $v0, $v0, $v1 */
    r_v0 = r_v0 << 2; /* 80078270 sll $v0, $v0, 2 */
    r_v0 = r_v0 + r_v1; /* 80078274 addu $v0, $v0, $v1 */
    bl_sh(r_s1 + 0x2u, r_v0); /* 80078278 sh $v0, 0x2($s1) */
    r_v0 = 0u | 0x3Cu; /* 8007827C ori $v0, $zero, 0x3C */
    bl_sh(r_s1 + 0x4u, r_v0); /* 80078280 sh $v0, 0x4($s1) */
    r_v0 = 0u | 0xDu; /* 80078284 ori $v0, $zero, 0xD */
    bl_sh(r_s1 + 0x6u, r_v0); /* 80078288 sh $v0, 0x6($s1) */
    r_v0 = r_s3 + r_s5; /* 8007828C addu $v0, $s3, $s5 */
    r_v1 = bl_lwl(r_s1 + 0x3u, r_v1); /* 80078290 lwl $v1, 0x3($s1) */
    r_v1 = bl_lwr(r_s1, r_v1); /* 80078294 lwr $v1, 0x0($s1) */
    r_a0 = bl_lwl(r_s1 + 0x7u, r_a0); /* 80078298 lwl $a0, 0x7($s1) */
    r_a0 = bl_lwr(r_s1 + 0x4u, r_a0); /* 8007829C lwr $a0, 0x4($s1) */
    bl_swl(r_v0 + 0x3u, r_v1); /* 800782A0 swl $v1, 0x3($v0) */
    bl_swr(r_v0, r_v1); /* 800782A4 swr $v1, 0x0($v0) */
    bl_swl(r_v0 + 0x7u, r_a0); /* 800782A8 swl $a0, 0x7($v0) */
    bl_swr(r_v0 + 0x4u, r_a0); /* 800782AC swr $a0, 0x4($v0) */
    r_a0 = r_s5 + 0xFFFFFFB0u; /* 800782B0 addiu $a0, $s5, -0x50 */
    r_a0 = r_s0 + r_a0; /* 800782B4 addu $a0, $s0, $a0 */
    /* 800782B8 jal func_800780A8 */
    r_a1 = r_s2 + 0u;
    bl_jal(FN, 0x800780A8u, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* func_800780A8 */
    r_a0 = r_s4 + 0u; /* 800782C0 addu $a0, $s4, $zero */
    /* 800782C4 jal func_800780A8 */
    r_a1 = r_s2 + 0x1u;
    bl_jal(FN, 0x800780A8u, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* func_800780A8 */
    r_s4 = r_s4 + 0xC0u; /* 800782CC addiu $s4, $s4, 0xC0 */
    r_s1 = r_s1 + 0xC0u; /* 800782D0 addiu $s1, $s1, 0xC0 */
    r_s3 = r_s3 + 0xC0u; /* 800782D4 addiu $s3, $s3, 0xC0 */
    r_s2 = r_s2 + 0x2u; /* 800782D8 addiu $s2, $s2, 0x2 */
    r_v0 = (u32)((s32)r_s2 < 8); /* 800782DC slti $v0, $s2, 0x8 */
    /* 800782E0 bnez $v0, .L80078234 */
    { int c_ = (r_v0 != 0); r_s0 = r_s0 + 0xC0u; if (c_) goto L_80078234; }
    r_ra = bl_lw(r_sp + 0x38u); /* 800782E8 lw $ra, 0x38($sp) */
    r_s5 = bl_lw(r_sp + 0x34u); /* 800782EC lw $s5, 0x34($sp) */
    r_s4 = bl_lw(r_sp + 0x30u); /* 800782F0 lw $s4, 0x30($sp) */
    r_s3 = bl_lw(r_sp + 0x2Cu); /* 800782F4 lw $s3, 0x2C($sp) */
    r_s2 = bl_lw(r_sp + 0x28u); /* 800782F8 lw $s2, 0x28($sp) */
    r_s1 = bl_lw(r_sp + 0x24u); /* 800782FC lw $s1, 0x24($sp) */
    r_s0 = bl_lw(r_sp + 0x20u); /* 80078300 lw $s0, 0x20($sp) */
    r_sp = r_sp + 0x40u; /* 80078304 addiu $sp, $sp, 0x40 */
    /* 80078308 jr $ra */
    ; goto bl_exit;
bl_exit:
    bl_leave(bl_saved);
    (void)hi; (void)lo; (void)r_at; (void)r_v1; (void)r_k0; (void)r_k1; (void)r_gp; (void)r_ra;
    (void)r_v0;
}

/* func_800780A8: adopted from the h-lift block above (321613d2), whose body is
 * identical to this step's lift and to a fresh --jal-service lift. */
#endif /* XENO_PC_PORT h-frontier */
