#include "common.h"
#ifndef XENO_PC_PORT
#include "psyq/libgpu.h"
#endif


#ifndef XENO_PC_PORT
void func_80076D58(void *, s32, s32);
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
INCLUDE_ASM("asm/battle/nonmatchings/main6", func_80076D58);
INCLUDE_ASM("asm/battle/nonmatchings/main6", func_80076EA4);

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

INCLUDE_ASM("asm/battle/nonmatchings/main6", func_80077454);
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
extern void* bzero(unsigned char* p, int size);
extern void func_80077074(void);


/* func_80077610.s */
void func_80077610(void) {
    void* p = func_8008ABB8(0x670, 0);

    *(u32*)((u8*)D_800C3EA4 + 0xA230) = (u32)p;
    bzero(p, 0x670);
    func_80077074();
}
/* func_8007765C.s */
void func_8007765C(void) {
    func_800716D8();
    HeapFree(*(u32*)((u8*)D_800C3EA4 + 0xA230));
}
