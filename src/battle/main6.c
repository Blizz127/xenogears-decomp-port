#include "common.h"
#ifndef XENO_PC_PORT
#include "psyq/libgpu.h"
#endif


#ifndef XENO_PC_PORT
void func_80076D58(POLY_FT4* prims, u8 sel, u8 mode);
extern u32 D_800C3EA4;
void func_80077074(void);
#endif

/* Fill a POLY_FT4's corners and texture coordinates: a 13-pixel-high strip
 * (func_80076C78) or a w x h quad (func_80076CE8) at (x, y) / (u, v). */
#ifndef XENO_PC_PORT
void func_80076C78(POLY_FT4* p, s16 x, s16 y, u8 u, u8 v, u8 w) {
    p->x0 = x;
    p->y0 = y;
    p->y1 = y;
    p->x2 = x;
    p->y2 = y + 13;
    p->y3 = y + 13;
    p->u0 = u;
    p->u2 = u;
    p->x1 = x + w;
    p->x3 = x + w;
    p->v0 = v;
    p->u1 = u + w;
    p->v1 = v;
    p->v2 = v + 13;
    p->u3 = u + w;
    p->v3 = v + 13;
}

void func_80076CE8(POLY_FT4* p, s16 x, s16 y, u8 u, u8 v, s32 w, s32 h) {
    p->x0 = x;
    p->y0 = y;
    p->y1 = y;
    p->x2 = x;
    p->u0 = u;
    p->u2 = u;
    p->x1 = x + w;
    p->y2 = y + h;
    p->x3 = x + w;
    p->y3 = y + h;
    p->v0 = v;
    p->u1 = u + w;
    p->v1 = v;
    p->v2 = v + h;
    p->u3 = u + w;
    p->v3 = v + h;
}
#endif
#ifndef XENO_PC_PORT
/* func_80076D58.s: initialise the two POLY_FT4s at `prims`: SetPolyFT4,
 * mid-grey colour, SetSemiTrans(0), SetShadeTex(1), the tpage for `mode`
 * (0..3: x 0x380/0x3C0, y 0/0x100; other values leave it) and the clut
 * g_SystemPalette2 when `sel` is set, else g_SystemPalette1.  The int
 * counter gives retail's signed loop test (gcc rewrites it onto the prim
 * pointer); the clut ternary gives the pointer copy both stores go through,
 * which only gcc 2.6.x emits (2.7.2: 4 instructions off). */
extern u16 g_SystemPalette1;
extern u16 g_SystemPalette2;

void func_80076D58(POLY_FT4* prims, u8 sel, u8 mode) {
    s32 i;

    for (i = 0; i < 2; i++) {
        SetPolyFT4(&prims[i]);
        setRGB0(&prims[i], 0x80, 0x80, 0x80);
        SetSemiTrans(&prims[i], 0);
        SetShadeTex(&prims[i], 1);
        switch (mode) {
        case 0:
            prims[i].tpage = GetTPage(0, 0, 0x380, 0);
            break;
        case 1:
            prims[i].tpage = GetTPage(0, 0, 0x380, 0x100);
            break;
        case 2:
            prims[i].tpage = GetTPage(0, 0, 0x3C0, 0x100);
            break;
        case 3:
            prims[i].tpage = GetTPage(0, 0, 0x3C0, 0);
            break;
        }
        prims[i].clut = sel ? g_SystemPalette2 : g_SystemPalette1;
    }
}
/* 4-byte records at D_800D2F68; only `width` (SystemRenderStringEntry's
 * result) is backed by another reader, the other names are by offset. */
typedef struct {
    u8 width;
    u8 unk1;
    u8 unk2;
    u8 unk3;
} BattleTextRec;
extern BattleTextRec D_800D2F68[];
/* Image pointer wrapped as an aggregate element on purpose: arrays of
 * aggregates keep gcc's offset-giv addressing (lw %lo(sym)(at)) that
 * retail has; a plain void*[] turns into a pointer walk. */
typedef struct {
    void* data;
} BattleImageRef;
extern BattleImageRef D_800C3E5C[];
extern void* func_8008AC00(s32 kind);
extern void* func_800338D8(s32 id);
extern s32 SystemRenderStringEntry(void* pString, void* pWork, s32 height, s32 flag);
extern void func_800769E8(void* pRect, void* pData);
extern void HeapFree(void* p);
extern void* bzero(unsigned char* p, int size);

/* For i in 0..10: render the strings func_800338D8 returns for ids 0xA + i
 * and 0x15 + i into a fresh 0x30C-byte buffer (SystemRenderStringEntry, 0x1B,
 * flag 0 then 1), store the widths in D_800D2F68 records i and 11 + i (with
 * +2 = 0x78, +3 = 13 (i + 1), +1 = 0 / 1) and upload the buffer to
 * (0x3DE, 0x10D + 13 i) 30x13.  The buffers are freed, then the ten
 * D_800C3E5C images go to (0x3DE + 2 i, 0x100) 6x13. */
void func_80076EA4(void) {
    BattleImageRef buf[11];
    RECT r;
    s32 i;

    for (i = 0; i < 11; i++) {
        buf[i].data = func_8008AC00(0x1B);
        bzero(buf[i].data, 0x30C);
        D_800D2F68[i].width = SystemRenderStringEntry(func_800338D8(i + 0xA), buf[i].data, 0x1B, 0);
        D_800D2F68[i + 11].width = SystemRenderStringEntry(func_800338D8(i + 0x15), buf[i].data, 0x1B, 1);
        D_800D2F68[i].unk2 = D_800D2F68[i + 11].unk2 = 0x78;
        D_800D2F68[i].unk3 = D_800D2F68[i + 11].unk3 = i * 0xD + 0xD;
        D_800D2F68[i].unk1 = 0;
        D_800D2F68[i + 11].unk1 = 1;
        r.y = i * 0xD + 0x10D;
        r.x = 0x3DE;
        r.w = 0x1E;
        r.h = 0xD;
        func_800769E8(&r, buf[i].data);
    }
    for (i = 0; i < 11; i++) {
        HeapFree(buf[i].data);
    }
    for (i = 0; i < 10; i++) {
        r.x = 0x3DE + i * 2;
        r.y = 0x100;
        r.w = 6;
        r.h = 0xD;
        func_800769E8(&r, D_800C3E5C[i].data);
    }
}

#ifndef XENO_PC_PORT
void func_80077074(void) {
    *(s8 *)((s8*)(*(void **)((s8*)((*(void **)&D_800C3EA4)) + 0xA230)) + 0x669) = 0;
    *(s8 *)((s8*)(*(void **)((s8*)((*(void **)&D_800C3EA4)) + 0xA230)) + 0x66B) = 0;
    *(s8 *)((s8*)(*(void **)((s8*)((*(void **)&D_800C3EA4)) + 0xA230)) + 0x66D) = 0;
    func_80076D58(*(void **)((s8*)((*(void **)&D_800C3EA4)) + 0xA230), 0, 1);
    func_80076D58(*(void **)((s8*)((*(void **)&D_800C3EA4)) + 0xA230) + 0x50, 0, 1);
    func_80076D58(*(void **)((s8*)((*(void **)&D_800C3EA4)) + 0xA230) + 0xA0, 0, 2);
    func_80076D58(*(void **)((s8*)((*(void **)&D_800C3EA4)) + 0xA230) + 0xF0, 0, 2);
    func_80076D58(*(void **)((s8*)((*(void **)&D_800C3EA4)) + 0xA230) + 0x140, 0, 1);
    func_80076D58(*(void **)((s8*)((*(void **)&D_800C3EA4)) + 0xA230) + 0x190, 0, 2);
    func_80076D58(*(void **)((s8*)((*(void **)&D_800C3EA4)) + 0xA230) + 0x1E0, 0, 2);
    func_80076D58(*(void **)((s8*)((*(void **)&D_800C3EA4)) + 0xA230) + 0x230, 0, 2);
    func_80076D58(*(void **)((s8*)((*(void **)&D_800C3EA4)) + 0xA230) + 0x280, 0, 3);
    func_80076D58(*(void **)((s8*)((*(void **)&D_800C3EA4)) + 0xA230) + 0x2D0, 1, 3);
    func_80076D58(*(void **)((s8*)((*(void **)&D_800C3EA4)) + 0xA230) + 0x320, 0, 2);
    func_80076D58(*(void **)((s8*)((*(void **)&D_800C3EA4)) + 0xA230) + 0x370, 1, 2);
    func_80076D58(*(void **)((s8*)((*(void **)&D_800C3EA4)) + 0xA230) + 0x3C0, 0, 2);
    func_80076D58(*(void **)((s8*)((*(void **)&D_800C3EA4)) + 0xA230) + 0x410, 0, 2);
    func_80076D58(*(void **)((s8*)((*(void **)&D_800C3EA4)) + 0xA230) + 0x460, 0, 2);
    func_80076D58(*(void **)((s8*)((*(void **)&D_800C3EA4)) + 0xA230) + 0x4B0, 0, 2);
    func_80076D58(*(void **)((s8*)((*(void **)&D_800C3EA4)) + 0xA230) + 0x500, 0, 2);
    func_80076D58(*(void **)((s8*)((*(void **)&D_800C3EA4)) + 0xA230) + 0x550, 0, 2);
}
#endif

void func_80077364(POLY_FT4* prims, u8 index) {
    s32 i;
    s32 off;

    for (i = 0; i < 4; i++) {
        POLY_FT4* p = &prims[i];

        SetPolyFT4(p);
        SetShadeTex(p, 1);
        setRGB0(p, 0xFF, 0xFF, 0xFF);
        off = index * 0x18;
        p->tpage = GetTPage(*(s32*)(D_800C3EA4 + off + 0xA238), 0,
                            *(s32*)(D_800C3EA4 + off + 0xA244),
                            *(s32*)(D_800C3EA4 + off + 0xA248));
        p->clut = GetClut(*(s32*)(D_800C3EA4 + off + 0xA23C),
                          *(s32*)(D_800C3EA4 + off + 0xA240));
    }
}

extern u32 D_800D2E38[];
extern u8 D_800594D4[];
extern s32 D_800595A0;
extern u8* D_800D2D28;

/* Prepare battle window `index`'s prims: for both buffers, the background
 * POLY_G4 (block +0x3C0) gets the D_800594D4 colour on all four corners and
 * semi-transparency, and its DR_MODE (block +0x408) the tpage of the frame
 * texture (UI state +0xA25C/+0xA260, abr D_800595A0); then func_80077364
 * sets up the four edge prim pairs (+0x140 .. +0x320). */
void func_80077454(u8 index) {
    u8* blk;
    u8 i;

    blk = (u8*)D_800D2E38[index];
    for (i = 0; i < 2; i++) {
        SetPolyG4((POLY_G4*)((i * 0x24 + 0x3C0) + blk));
        (blk + i * 0x24)[0x3C4] = D_800594D4[0];
        (blk + i * 0x24)[0x3C5] = D_800594D4[1];
        (blk + i * 0x24)[0x3C6] = D_800594D4[2];
        (blk + i * 0x24)[0x3CC] = D_800594D4[0];
        (blk + i * 0x24)[0x3CD] = D_800594D4[1];
        (blk + i * 0x24)[0x3CE] = D_800594D4[2];
        (blk + i * 0x24)[0x3D4] = D_800594D4[0];
        (blk + i * 0x24)[0x3D5] = D_800594D4[1];
        (blk + i * 0x24)[0x3D6] = D_800594D4[2];
        (blk + i * 0x24)[0x3DC] = D_800594D4[0];
        (blk + i * 0x24)[0x3DD] = D_800594D4[1];
        (blk + i * 0x24)[0x3DE] = D_800594D4[2];
        SetSemiTrans((POLY_G4*)((i * 0x24 + 0x3C0) + blk), 1);
        SetDrawMode((DR_MODE*)((i * 0xC + 0x408) + blk), 0, 0,
                    GetTPage(0, D_800595A0, *(s32*)((u8*)D_800C3EA4 + 0xA25C), *(s32*)((u8*)D_800C3EA4 + 0xA260)),
                    (RECT*)D_800D2D28);
    }
    func_80077364((POLY_FT4*)(blk + 0x140), 1);
    func_80077364((POLY_FT4*)(blk + 0x1E0), 2);
    func_80077364((POLY_FT4*)(blk + 0x280), 3);
    func_80077364((POLY_FT4*)(blk + 0x320), 4);
}
#endif


#ifndef XENO_PC_PORT
extern u8* D_800D2D28;
#endif
#ifndef XENO_PC_PORT
extern u8 D_800D3420[];
#endif
#ifndef XENO_PC_PORT
extern u8 D_800D3430[];
#endif
#ifndef XENO_PC_PORT
extern u32 D_800D3410[];
#endif
extern u16 D_8005A3A0[];
#ifndef XENO_PC_PORT
extern u8 D_800D366C;
#endif
#ifndef XENO_PC_PORT
extern u8* D_800C3EAC;
#endif
extern u32 func_80089C08(u32 v);
extern void func_800BC404(u32 v);
extern void func_800BCD98(u32 v);
extern void func_80085454(s32);
extern void func_80085618(s32);
extern void func_8008AA40(u8 v);
#ifndef XENO_PC_PORT
extern u32 D_800C3EA4;
#endif
extern void* func_8008ABB8(s32 size, s32 flag);
#ifndef XENO_PC_PORT
extern void* bzero(unsigned char* p, int size);
#endif
extern void func_80077074(void);


/* func_80077610.s */
#ifndef XENO_PC_PORT /* port: not adopted; stays out of the port TU */
void func_80077610(void) {
    void* p = func_8008ABB8(0x670, 0);

    *(u32*)((u8*)D_800C3EA4 + 0xA230) = (u32)p;
    bzero(p, 0x670);
    func_80077074();
}
#endif
/* func_8007765C.s */
#ifndef XENO_PC_PORT /* port: not adopted; stays out of the port TU */
void func_8007765C(void) {
    func_800716D8();
    HeapFree(*(u32*)((u8*)D_800C3EA4 + 0xA230));
}
#endif
