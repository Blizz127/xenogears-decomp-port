#include "common.h"


#ifndef XENO_PC_PORT
#include "psyq/libgpu.h"
#include "psyq/libgte.h"
extern VECTOR D_800C35C4;

typedef struct {
    s16 unk0;
    s16 unk2;
    s16 unk4;
    s16 unk6;
    s16 unk8;
    s16 unkA;
    s16 unkC;
    s16 unkE;
    VECTOR unk10;
    u8 unk20[4];
    POLY_FT3 prims[2];
    s32 unk64;
    VECTOR unk68;
    s32 unk78;
} B7424Rec; /* 0x7C */

/* func_800B7424.s, byte-exact: lay out the 2 x 14 x 20 grid of 0x7C-byte
 * effect records at p (side offsets 0x3C / +0x87A0, rows 0x9B0 apart).
 * Each record gets a start position, a random spin/drift vector
 * (RotMatrixZYX + ApplyMatrixLV of D_800C35C4 with a random -z), a
 * lifetime at +0x78 and two textured POLY_FT3s at +0x24/+0x44 cutting a
 * 16x16 cell of the 0x2C0 page.  Retail returns p in v0.
 * CONSTRUCTS (counted separately):
 * - The prim and column loops are goto loops.  Without loop notes, the
 *   loop-depth ref weights give k retail's s6 and poff/v1/u1 s3/s4/s5.
 * - `ff` stands in for loop.c's hoisted and spilled 0xFF (setRGB0),
 *   recreating retail's reload copy.
 * Inherited from the parked draft: the `base` macro, the explicit
 * c16/r16/cu/poff/u0/u1/v1 temporaries, `d` assigned inside the first
 * +0x64 store (retail stores +0x64 twice), and the B7424Rec view. */
u32 func_800B7424(u32 p) {
#define base ((u8*)p)
    VECTOR sq;
    SVECTOR r;
    MATRIX m;
    s32 radius;
    s32 row;
    s32 side;
    s32 r16;
    s32 ya;
    s32 yb;
    s32 cu;
    s32 roff;
    s32 y0;
    s32 y1;
    s32 off;
    s32 c16;
    s32 col;
    B7424Rec* e;
    s32 d;
    s32 a;
    s32 ang;
    s32 k;
    s32 poff;
    POLY_FT3* prim;
    s32 u0;
    s32 u1;
    s32 v1;
    u8 ff;

    ff = 0xFF;
    *(s32*)(base + 0x38) = 0;
    radius = SquareRoot0(0x9500) << 10;
    for (side = 0, off = 0x3C; side != 2; off += 0x87A0, side++) {
        for (row = 0, roff = 0, y0 = -0xCA0, y1 = -0xD60; row != 0xE;
             roff += 0x9B0, y0 += 0x200, y1 += 0x200, row++) {
            r16 = row << 4;
            ya = y1;
            yb = y0;
            col = 0;
            c16 = 0;
        next_col:
            {
                e = (B7424Rec*)(base + off + roff + col * 0x7C);
                e->unk0 = 0;
                e->unk2 = 0;
                e->unk4 = 0;
                if (side == 0) {
                    e->unk10.vx = (c16 - 0x9B) << 5;
                    e->unk10.vy = ya;
                    e->unk10.vz = 0x4000;
                } else {
                    e->unk10.vx = (c16 - 0x95) << 5;
                    e->unk10.vy = yb;
                    e->unk10.vz = 0x4000;
                }
                D_800C35C4.vz = 0xFE0C0000;
                D_800C35C4.vz += -(rand() % 1000) << 16;
                Square0(&e->unk10, &sq);
                e->unk64 = (radius / 32 - (d = SquareRoot0(sq.vx + sq.vy))) / 2048;
                e->unk64 = d / 1024;
                a = ratan2(e->unk10.vy, e->unk10.vx);
                ang = a + (0x600 + rand() % 1024);
                d = (d << 11) / radius;
                d -= 0x20 - rand() % 64;
                r.vx = 0;
                r.vy = d;
                r.vz = ang;
                RotMatrixZYX(&r, &m);
                ApplyMatrixLV(&m, &D_800C35C4, &e->unk68);
                k = 0;
                e->unk78 = 0x70800 - ((rand() % 1600) << 8);
                u0 = c16 & 0x3F;
                u1 = u0 + 0x10;
                v1 = r16 + 0x10;
                cu = c16;
                poff = 0x24;
                e->unk8 = (rand() & 0xFF) - 0x7F;
                e->unkA = (rand() & 0xFF) - 0x7F;
                e->unkC = (rand() & 0x1FF) - 0xFF;
            next_prim:
                    prim = (POLY_FT3*)((u8*)e + poff);
                    SetPolyFT3(prim);
                    SetShadeTex(prim, 0);
                    setRGB0(prim, ff, ff, ff);
                    prim->code &= 0xFD;
                    prim->tpage = GetTPage(2, 1, cu + 0x2C0, 0x100);
                    if (side == 0) {
                        setUV3(prim, u0, r16, u1, r16, u0, v1);
                    } else {
                        setUV3(prim, u1, r16, u1, v1, u0, v1);
                    }
                    k++;
                    poff += 0x20;
                    if (k != 2) {
                        goto next_prim;
                    }
            }
            col++;
            c16 += 0x10;
            if (col != 0x14) {
                goto next_col;
            }
        }
    }
    return p;
#undef base
}

#include "psyq/libgpu.h"
#include "battle/spad_stack.h"
typedef struct {
    DRAWENV draw;
    DISPENV disp;
    u32 ot[0x1000];
} B7870Buf; /* 0x4070 */
typedef struct {
    u8 pad0[0xB70];
    B7870Buf db[2];
    B7870Buf* cur;
    u32* ot;
    u8 pad8C58[0x2C];
    s32 idx;
} B7870Glob;
extern u8 D_800C3EB0__b[] asm("D_800C3EB0");
static inline B7870Glob* B7870_Glob(void) {
    return (B7870Glob*)D_800C3EB0__b;
}
extern u32 D_800C3CB4__p[] asm("D_800C3CB4");
extern void WorkListsReset(void);
extern s32 ArchiveDataSync(void);
extern void ArchiveCdDataSync(s32 mode);
extern void GameCheckAndHandleSoftReset(void);
extern void func_8001BB0C(void);
extern void func_801E5840(u8 a0);
extern u8* func_800B73EC(void);
extern void func_800B6F0C__t(u8* task) asm("func_800B6F0C");
extern void func_800B7160(u8* task);
extern void func_800B7330(u8* p);
extern s32 func_80021AD8(s32 color, s32 value);
extern void Vsync(s32 mode);

/* Screen transition: copy the 320x224 front buffer to VRAM (0x2C0, 0x100)
 * with every pixel's STP bit set, flip and clear the double buffer once,
 * give both draw environments a white background, then run the transition
 * task (func_800B73EC) every frame (Vsync(2)) while the background colour
 * of db[idx] (idx at +0x8C84) fades by 0xC a step and the loader advances
 * (func_801E5840 for states 1, 3 and 4, func_8001BB0C for state 2), until
 * 0x56 frames have passed and the loader reaches state 5. */
void func_800B7870(void) {
    RECT rect;
    u16* buf;
    u16* q;
    s32 i;
    s32 wait;
    s32 state;
    s32 n;
    u8* p;
    B7870Buf* next;
    B7870Buf* first;
    B7870Glob* g;
    B7870Glob* h;
    B7870Glob* f;
    B7870Buf* db0;

    wait = 0x56;
    WorkListsReset();
    state = 1;
    n = 0;
    buf = HeapAlloc(0x30000, 1);
    q = buf;
    rect.x = 0;
    rect.y = 0;
    rect.w = 0x140;
    rect.h = 0xE0;
    StoreImage(&rect, (u_long*)buf);
    DrawSync(0);
    for (i = 0; i != 0x14000; i++) {
        *q++ |= 0x8000;
    }
    rect.x = 0x2C0;
    rect.y = 0x100;
    rect.w = 0x140;
    rect.h = 0xE0;
    LoadImage(&rect, (u_long*)buf);
    DrawSync(0);
    HeapFree(buf);
    g = B7870_Glob();
    first = &g->db[0];
    /* db0 is copied inside the compare, after the g->cur load, as retail */
    if (g->cur == (db0 = first)) {
        first = &g->db[1];
    }
    g->cur = first;
    g->ot = first->ot;
    ClearOTagR((u_long*)first->ot, 0x1000);
    g->idx = 0;
    g->cur = db0;
    g->db[0].draw.isbg = 1;
    g->db[1].draw.isbg = 1;
    g->db[0].draw.r0 = 0xFF;
    g->db[1].draw.r0 = 0xFF;
    g->db[0].draw.g0 = 0xFF;
    g->db[1].draw.g0 = 0xFF;
    g->db[0].draw.b0 = 0xFF;
    g->db[1].draw.b0 = 0xFF;
    p = func_800B73EC();
    while (wait != 0 || state != 5) {
        if (wait > 0) {
            wait--;
        }
        h = B7870_Glob();
        next = &h->db[0];
        if (h->cur == next) {
            next = &h->db[1];
        }
        h->cur = next;
        h->ot = next->ot;
        ClearOTagR((u_long*)next->ot, 0x1000);
        h->idx = 1 - h->idx;
        D_800C3CB4__p[0] = (u32)h->ot;
        if (ArchiveDataSync() == 0) {
            switch (state) {
            case 0:
                break;
            case 2:
                func_8001BB0C();
                state++;
                break;
            case 1:
            case 3:
            case 4:
                func_801E5840(n);
                n++;
                state++;
                break;
            }
        }
        GameCheckAndHandleSoftReset();
        BATTLE_SPAD_STACK_SET(0x1F8003FC);
        func_800B6F0C__t(p);
        func_800B7160(p);
        BATTLE_SPAD_STACK_RESET();
        DrawSync(0);
        Vsync(2);
        f = B7870_Glob();
        f->db[f->idx].draw.r0 = func_80021AD8(f->db[f->idx].draw.r0, -0xC);
        f->db[f->idx].draw.g0 = func_80021AD8(f->db[f->idx].draw.g0, -0xC);
        f->db[f->idx].draw.b0 = func_80021AD8(f->db[f->idx].draw.b0, -0xC);
        PutDispEnv(&f->cur->disp);
        PutDrawEnv(&f->cur->draw);
        DrawOTag((u_long*)&f->cur->ot[0xFFF]);
    }
    func_800B7330(p);
    SetDispMask(0);
    ArchiveCdDataSync(0);
    func_801E5840(3);
}
#endif


extern u8* TimerWorkListAllocateTask(u32 owner, u32 size);
extern void TimerWorkListSetTaskCallback(void* pTask, void* callback);
extern u32 func_800B57E4(u8* s);
extern void func_800B5B3C(u8* task);
extern void func_800B5854(void);
extern void func_800BF73C(void);
extern void func_800B5CC0(void);
extern void func_800BDC14(void);
extern void func_800BDF1C(void);
extern void func_8001E148(u32 v);
extern void func_800C08CC(u32 a0, void* a1, void* a2);
extern void func_800B51B0(void);
extern void func_800245D8(u32 a0, u32 a1);
#ifndef XENO_PC_PORT
extern u8 D_800C3EB0[];
#endif
extern u32 WorkListsAddTasks(u32 a0, u32 a1, void* a2, void* a3, void* a4);
extern u32 func_800B7424(u32 p);
extern void func_800B7364(void);
extern void func_800B6F0C(void);
extern void func_800B7134(void);
extern void WorkListSetTaskCallback(void* pTask, void* callback);
extern void D_80025A88(void);
#ifndef XENO_PC_PORT
extern u8 D_800D3420[];
#endif
#ifndef XENO_PC_PORT
extern u32 D_800C3CE8[];
#endif
extern void WorkListTaskSetOnFreeCallback(void* pTask, void* callback);
extern void func_800B3358(u8* p);
extern void func_800B3588(u8* p);
#ifndef XENO_PC_PORT
extern u32 D_800C3548[];
#endif


/* func_800B7C28.s */
#ifndef XENO_PC_PORT
extern u8 D_800D2FDC[];
#endif
void func_800B7C28(void) {
    D_800D2FDC[0] = 0;
}
