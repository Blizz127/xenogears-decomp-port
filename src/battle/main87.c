#include "common.h"


#ifndef XENO_PC_PORT
#include "psyq/libgte.h"
extern u8* g_GfxCurWorkBuffer__a[] asm("g_GfxCurWorkBuffer");
extern u8* g_GfxCurWorkBufferEnd__a[] asm("g_GfxCurWorkBufferEnd");
extern u32* g_GfxCurOT__a[] asm("g_GfxCurOT");
extern MATRIX D_8004FBB8;
extern MATRIX D_800C3574;
extern s32 D_80050100;
extern void AddPrim(void* ot, void* p);

/* func_800B5DF4.s: work-list draw callback for the object at task+4 (skipped
 * while obj->0x34 != 0).  Projects obj's position (16.16 at +0/+4/+8) with
 * D_800C3574 (obj->0x3F bit 0) or D_8004FBB8 as rotation+translation, stores
 * the OT depth in obj->0x2E, projects the position minus (obj->0xC/0x10/0x14
 * >> (obj->0x36 + 8)) as the second vertex, and links a LINE_F2-sized
 * packet (word obj->0x28) plus an optional E1 draw-mode packet.  Each branch
 * makes its own SetRotMatrix/SetTransMatrix calls (retail's cross-jumped
 * tail with the matrix in $s0), and the packet pointer is a copy of the
 * tested work pointer, as retail keeps them in $a1 and $s2. */
void func_800B5DF4(u8* task) {
    u8* obj = *(u8**)(task + 4);
    u8* work;
    u8* line;
    SVECTOR sv;
    long flag;
    s32 depth;
    u32 shift;
    u32 mode;
    u8* pkt;
    u32* ot;

    if (*(u16*)(obj + 0x34) != 0) {
        return;
    }
    work = g_GfxCurWorkBuffer__a[0];
    if (work + 0x10 >= g_GfxCurWorkBufferEnd__a[0]) {
        return;
    }
    sv.vx = *(s32*)(obj + 0) >> 16;
    sv.vy = *(s32*)(obj + 4) >> 16;
    sv.vz = *(s32*)(obj + 8) >> 16;
    g_GfxCurWorkBuffer__a[0] = work + 0x10;
    line = work;
    if (obj[0x3F] & 1) {
        SetRotMatrix(&D_800C3574);
        SetTransMatrix(&D_800C3574);
    } else {
        SetRotMatrix(&D_8004FBB8);
        SetTransMatrix(&D_8004FBB8);
    }
    depth = RotTransPers(&sv, (long*)(line + 8), &flag, &flag) >> D_80050100;
    shift = *(u16*)(obj + 0x36) + 8;
    *(u16*)(obj + 0x2E) = depth;
    sv.vx -= *(s32*)(obj + 0xC) >> shift;
    sv.vy -= *(s32*)(obj + 0x10) >> shift;
    sv.vz -= *(s32*)(obj + 0x14) >> shift;
    RotTransPers(&sv, (long*)(line + 0xC), &flag, &flag);
    line[3] = 3;
    *(u32*)(line + 4) = *(u32*)(obj + 0x28);
    AddPrim(g_GfxCurOT__a[0] + depth, line);
    pkt = g_GfxCurWorkBuffer__a[0];
    if (pkt + 8 >= g_GfxCurWorkBufferEnd__a[0]) {
        return;
    }
    mode = obj[0x3C] >> 5;
    if (mode == 0) {
        return;
    }
    g_GfxCurWorkBuffer__a[0] = pkt + 8;
    pkt[3] = 1;
    ot = g_GfxCurOT__a[0] + depth;
    *(u32*)(pkt + 4) = 0xE1000000 | (((mode - 1) & 3) << 5);
    AddPrim(ot, pkt);
}
extern void WorkListSetTaskCallback(void* pTask, void* callback);
extern void func_800B5DF4(u8* task);

void func_800B5FBC(u8* obj) {
    *(s16*)(obj + 0x34) = 1;
    WorkListSetTaskCallback(*(u8**)(obj + 0x6C) + 0x1C, func_800B5DF4);
    *(s8*)(obj + 0x2B) = 0x40;
}

#include "psyq/libgte.h"
extern u8* g_GfxCurWorkBuffer__a[] asm("g_GfxCurWorkBuffer");
extern u8* g_GfxCurWorkBufferEnd__a[] asm("g_GfxCurWorkBufferEnd");
extern u32* g_GfxCurOT__a[] asm("g_GfxCurOT");
extern MATRIX D_8004FBB8;
extern s32 D_80050100;
extern void AddPrim(void* ot, void* p);

/* func_800B6004.s: work-list draw callback: like func_800B5DF4, but always
 * projecting through D_8004FBB8 and taking the second vertex from the
 * linked object at obj->0x70.  Retail re-stores obj's position into sv
 * before overwriting it with the linked object's; positions are 16.16, so
 * the integer parts are read as `word >> 16` (retail's lh).  The second
 * packet has its own pointer and takes its OT entry before its draw-mode
 * word, as in retail. */
void func_800B6004(u8* task) {
    u8* obj = *(u8**)(task + 4);
    u8* work;
    u8* other;
    SVECTOR sv;
    long flag;
    s32 depth;
    u32 mode;
    u8* pkt;
    u32* ot;

    if (*(u16*)(obj + 0x34) != 0) {
        return;
    }
    work = g_GfxCurWorkBuffer__a[0];
    other = *(u8**)(obj + 0x70);
    if (work + 0x10 >= g_GfxCurWorkBufferEnd__a[0]) {
        return;
    }
    sv.vx = *(s32*)(obj + 0) >> 16;
    sv.vy = *(s32*)(obj + 4) >> 16;
    sv.vz = *(s32*)(obj + 8) >> 16;
    g_GfxCurWorkBuffer__a[0] = work + 0x10;
    SetRotMatrix(&D_8004FBB8);
    SetTransMatrix(&D_8004FBB8);
    depth = RotTransPers(&sv, (long*)(work + 8), &flag, &flag) >> D_80050100;
    *(u16*)(obj + 0x2E) = depth;
    sv.vx = *(s32*)(obj + 0) >> 16;
    sv.vy = *(s32*)(obj + 4) >> 16;
    sv.vz = *(s32*)(obj + 8) >> 16;
    sv.vx = *(s32*)(other + 0) >> 16;
    sv.vy = *(s32*)(other + 4) >> 16;
    sv.vz = *(s32*)(other + 8) >> 16;
    RotTransPers(&sv, (long*)(work + 0xC), &flag, &flag);
    work[3] = 3;
    *(u32*)(work + 4) = *(u32*)(obj + 0x28);
    AddPrim(g_GfxCurOT__a[0] + depth, work);
    pkt = g_GfxCurWorkBuffer__a[0];
    if (pkt + 8 >= g_GfxCurWorkBufferEnd__a[0]) {
        return;
    }
    mode = obj[0x3C] >> 5;
    if (mode == 0) {
        return;
    }
    g_GfxCurWorkBuffer__a[0] = pkt + 8;
    pkt[3] = 1;
    ot = g_GfxCurOT__a[0] + depth;
    *(u32*)(pkt + 4) = 0xE1000000 | (((mode - 1) & 3) << 5);
    AddPrim(ot, pkt);
}
extern void WorkListSetTaskCallback(void* pTask, void* callback);
extern void func_800B6004(u8* task);

void func_800B61B0(u8* obj) {
    *(s16*)(obj + 0x34) = 1;
    WorkListSetTaskCallback(*(u8**)(obj + 0x6C) + 0x1C, func_800B6004);
    *(s8*)(obj + 0x2B) = 0x40;
}

extern void* HeapAlloc(u32 size, u32 flags);
extern u32 HeapFree(void* p);
extern void func_800A96B4(u8 a0);
extern s16 D_800D36BC[];

/* clear D_800D36BC and run func_800A96B4(p[0]) on a HeapAlloc(0x4000, 1)
 * stack (caller $sp saved at stack + 0x3E00). */
void func_800B61F8(u32 unused, u8* p) {
    u8* stack = HeapAlloc(0x4000, 1);

    __asm__ volatile(
        "\taddu  $t0, %0, $zero\n"
        "\tsw    $sp, 0($t0)\n"
        "\taddiu $t0, $t0, -4\n"
        "\taddu  $sp, $t0, $zero\n"
        :
        : "r"(stack + 0x3E00)
        : "t0", "memory");
    D_800D36BC[0] = 0;
    func_800A96B4(p[0]);
    __asm__ volatile(
        "\taddiu $sp, $sp, 4\n"
        "\tlw    $sp, 0($sp)\n"
        :
        :
        : "memory");
    HeapFree(stack);
}

extern void* HeapAlloc(u32 size, u32 flags);
extern u32 HeapFree(void* p);
extern void func_800A9FF0(u32 a0);

void func_800B626C(void) {
    u8* stack = HeapAlloc(0x4000, 1);

    __asm__ volatile(
        "\taddu  $t0, %0, $zero\n"
        "\tsw    $sp, 0($t0)\n"
        "\taddiu $t0, $t0, -4\n"
        "\taddu  $sp, $t0, $zero\n"
        :
        : "r"(stack + 0x3E00)
        : "t0", "memory");
    func_800A9FF0(0xB);
    __asm__ volatile(
        "\taddiu $sp, $sp, 4\n"
        "\tlw    $sp, 0($sp)\n"
        :
        :
        : "memory");
    HeapFree(stack);
}

extern void func_800A979C(s32 idx, s16 x, s16 y, s16 z, s16 w);
extern void func_800BEE2C(u32 a0, u32 a1, u32 a2);
extern u8* D_800C3E1C__a[] asm("D_800C3E1C");
extern u16 D_800D3634__a[] asm("D_800D3634");

/* func_800B62C8.s: on a HeapAlloc(0x4000, 1) stack (the $sp switch of
 * func_800B626C), call func_800BEE2C(0xB, mask, p[0]).  Mode 2 first runs
 * func_800A979C(0xB, 0x300, 0x100, 0, 0x1DB) and takes the mask as one bit
 * selected by the current record's (D_800C3E1C) +0xAC low two bits and +0xA8
 * top two bits; otherwise the mask is D_800D3634.  Each branch makes its own
 * func_800BEE2C call (retail's shared call tail), and the two bit fields go
 * through temps so +0xA8 is loaded first while +0xAC stays the first `or`
 * operand. */
void func_800B62C8(u32 unused, u8* p) {
    u8* stack = HeapAlloc(0x4000, 1);
    u32 hi;
    u32 lo;

    __asm__ volatile(
        "\taddu  $t0, %0, $zero\n"
        "\tsw    $sp, 0($t0)\n"
        "\taddiu $t0, $t0, -4\n"
        "\taddu  $sp, $t0, $zero\n"
        :
        : "r"(stack + 0x3E00)
        : "t0", "memory");
    if (p[0] == 2) {
        func_800A979C(0xB, 0x300, 0x100, 0, 0x1DB);
        hi = *(u32*)(D_800C3E1C__a[0] + 0xA8) >> 30;
        lo = (*(u32*)(D_800C3E1C__a[0] + 0xAC) & 3) << 2;
        func_800BEE2C(0xB, 1 << (lo | hi), p[0]);
    } else {
        func_800BEE2C(0xB, D_800D3634__a[0], p[0]);
    }
    __asm__ volatile(
        "\taddiu $sp, $sp, 4\n"
        "\tlw    $sp, 0($sp)\n"
        :
        :
        : "memory");
    HeapFree(stack);
}
#endif


extern void func_800B3CD4(u32 a0, u32 a1, u32 a2, s32 a3,
                           s32 a4, s32 a5);


/* func_800B639C.s */
void func_800B639C(u32 a0, u8* p) {
    u32 n = (u32)((*(s8*)(p + 1) << 8) | p[0]);

    p += n;
    func_800B3CD4(p[5], p[3], p[4], *(s8*)(p + 0), *(s8*)(p + 1),
                  *(s8*)(p + 2));
}

#ifdef XENO_PC_PORT
#include "battle_host_guest.h"
#include "psyq/libgte.h"
#include "psyq/libgpu.h"
#include "guest_prim_link.h"
extern void* HeapAlloc(u32 size, u32 flags);
extern u32 HeapFree(void* p);
extern void WorkListSetTaskCallback(void* pTask, void* callback);
extern void* g_GfxCurWorkBuffer;
extern void* g_GfxCurWorkBufferEnd;
extern u_long* g_GfxCurOT;
extern MATRIX D_8004FBB8;
extern s32 D_80050100;
extern void func_800A9FF0(u32 v);
extern void func_800A96B4(u32 index);
extern void func_800A979C(u32 a0, u32 a1, u32 a2, u32 a3, u32 a4);
extern void func_800BEE2C(u32 a0, u32 a1, u32 a2);

/* Retail RotTransPers stores 32-bit SXY/P/FLAG words; the host wrapper takes
 * longs, so project into temporaries and store four bytes, like the
 * battle-runtime bridge does. */
static s32 bat87_rot_trans_pers(SVECTOR* v, u8* sxy) {
    int xy = 0;
    long p = 0;
    long flag = 0;
    s32 otz = RotTransPers(v, &xy, &p, &flag);

    *(u32*)sxy = (u32)xy;
    return otz;
}

/* func_800B5DF4.s: work-list draw callback for the object at task+4 (skipped
 * while obj->0x34 != 0).  Projects obj's position (+2/+6/+A) with
 * D_800C3574 (obj->0x3F bit 0) or D_8004FBB8 as rotation+translation, stores
 * the OT depth in obj->0x2E, projects position minus (obj->0xC/0x10/0x14 >>
 * (obj->0x36 + 8)) as the second vertex, and links a LINE_F2-sized packet
 * (code/colour word obj->0x28) plus an optional E1 draw-mode packet. */
void func_800B5DF4(u8* task) {
    u32 obj = BG_U32(task + 4);
    u8* work;
    u8* ot;
    SVECTOR sv;
    MATRIX* m;
    s32 depth;
    u32 shift;
    u32 mode;

    if (BG_U16(obj + 0x34) != 0) {
        return;
    }
    work = (u8*)g_GfxCurWorkBuffer;
    if (!((uintptr_t)(work + 0x10) < (uintptr_t)g_GfxCurWorkBufferEnd)) {
        return;
    }
    sv.vx = BG_S16(obj + 0x2);
    sv.vy = BG_S16(obj + 0x6);
    g_GfxCurWorkBuffer = work + 0x10;
    sv.vz = BG_S16(obj + 0xA);
    sv.pad = 0;
    if ((BG_U8(obj + 0x3F) & 1) != 0) {
        m = (MATRIX*)BG_PTR(0x800C3574); /* D_800C3574 */
    } else {
        m = &D_8004FBB8;
    }
    SetRotMatrix(m);
    SetTransMatrix(m);
    depth = bat87_rot_trans_pers(&sv, work + 0x8);
    shift = (u32)BG_U16(obj + 0x36) + 8;
    depth = depth >> ((u32)D_80050100 & 31);
    BG_U16(obj + 0x2E) = (u16)depth;
    sv.vx = (s16)((u32)(u16)sv.vx - (u32)(BG_S32(obj + 0xC) >> (shift & 31)));
    sv.vy = (s16)((u32)(u16)sv.vy - (u32)(BG_S32(obj + 0x10) >> (shift & 31)));
    sv.vz = (s16)((u32)(u16)sv.vz - (u32)(BG_S32(obj + 0x14) >> (shift & 31)));
    bat87_rot_trans_pers(&sv, work + 0xC);
    work[3] = 3;
    *(u32*)(work + 4) = BG_U32(obj + 0x28);
    ot = (u8*)g_GfxCurOT + (intptr_t)(s32)((u32)depth << 2);
    PcPort_AddPrimDomainAware(ot, work);

    work = (u8*)g_GfxCurWorkBuffer;
    if (!((uintptr_t)(work + 8) < (uintptr_t)g_GfxCurWorkBufferEnd)) {
        return;
    }
    mode = BG_U8(obj + 0x3C) >> 5;
    if (mode == 0) {
        return;
    }
    g_GfxCurWorkBuffer = work + 8;
    work[3] = 1;
    *(u32*)(work + 4) = 0xE1000000u | (((mode - 1) & 3) << 5);
    ot = (u8*)g_GfxCurOT + (intptr_t)(s32)((u32)depth << 2);
    PcPort_AddPrimDomainAware(ot, work);
}

/* func_800B5FBC.s: hide the object (obj->0x34 = 1), install func_800B5DF4 as
 * the draw callback of its work entry (obj->0x6C task + 0x1C) and set
 * obj->0x2B = 0x40. */
void func_800B5FBC(u8* obj) {
    *(u16*)(obj + 0x34) = 1;
    WorkListSetTaskCallback(BG_PTR(BG_U32(obj + 0x6C) + 0x1C),
                            (void*)(uintptr_t)0x800B5DF4u);
    obj[0x2B] = 0x40;
}

/* func_800B6004.s: like func_800B5DF4, but always with D_8004FBB8 and with
 * the second vertex taken from the linked object at obj->0x70 (+2/+6/+A). */
void func_800B6004(u8* task) {
    u32 obj = BG_U32(task + 4);
    u8* work;
    u8* ot;
    u32 other;
    SVECTOR sv;
    s32 depth;
    u32 mode;

    if (BG_U16(obj + 0x34) != 0) {
        return;
    }
    work = (u8*)g_GfxCurWorkBuffer;
    other = BG_U32(obj + 0x70);
    if (!((uintptr_t)(work + 0x10) < (uintptr_t)g_GfxCurWorkBufferEnd)) {
        return;
    }
    sv.vx = BG_S16(obj + 0x2);
    sv.vy = BG_S16(obj + 0x6);
    sv.vz = BG_S16(obj + 0xA);
    sv.pad = 0;
    g_GfxCurWorkBuffer = work + 0x10;
    SetRotMatrix(&D_8004FBB8);
    SetTransMatrix(&D_8004FBB8);
    depth = bat87_rot_trans_pers(&sv, work + 0x8);
    depth = depth >> ((u32)D_80050100 & 31);
    BG_U16(obj + 0x2E) = (u16)depth;
    sv.vx = BG_S16(other + 0x2);
    sv.vy = BG_S16(other + 0x6);
    sv.vz = BG_S16(other + 0xA);
    bat87_rot_trans_pers(&sv, work + 0xC);
    work[3] = 3;
    *(u32*)(work + 4) = BG_U32(obj + 0x28);
    ot = (u8*)g_GfxCurOT + (intptr_t)(s32)((u32)depth << 2);
    PcPort_AddPrimDomainAware(ot, work);

    work = (u8*)g_GfxCurWorkBuffer;
    if (!((uintptr_t)(work + 8) < (uintptr_t)g_GfxCurWorkBufferEnd)) {
        return;
    }
    mode = BG_U8(obj + 0x3C) >> 5;
    if (mode == 0) {
        return;
    }
    g_GfxCurWorkBuffer = work + 8;
    work[3] = 1;
    *(u32*)(work + 4) = 0xE1000000u | (((mode - 1) & 3) << 5);
    ot = (u8*)g_GfxCurOT + (intptr_t)(s32)((u32)depth << 2);
    PcPort_AddPrimDomainAware(ot, work);
}

/* func_800B61B0.s: func_800B5FBC with func_800B6004 as the draw callback. */
void func_800B61B0(u8* obj) {
    *(u16*)(obj + 0x34) = 1;
    WorkListSetTaskCallback(BG_PTR(BG_U32(obj + 0x6C) + 0x1C),
                            (void*)(uintptr_t)0x800B6004u);
    obj[0x2B] = 0x40;
}

/* func_800B61F8.s: clear D_800D36BC and run func_800A96B4(p[0]).  Retail runs
 * the callee on a stack switched onto a HeapAlloc(0x4000, 1) block (saved
 * $sp at block+0x3E00); natively only the alloc/free pair is kept. */
void func_800B61F8(u32 unused, u8* p) {
    void* stack = HeapAlloc(0x4000, 1);

    BG_U16(0x800D36BC) = 0; /* D_800D36BC */
    func_800A96B4(p[0]);
    HeapFree(stack);
}

/* func_800B626C.s: func_800A9FF0(0xB) on a HeapAlloc(0x4000, 1) stack. */
void func_800B626C(void) {
    void* stack = HeapAlloc(0x4000, 1);

    func_800A9FF0(0xB);
    HeapFree(stack);
}

/* func_800B62C8.s: on a HeapAlloc(0x4000, 1) stack, call func_800BEE2C(0xB,
 * mask, p[0]).  Mode 2 first runs func_800A979C(0xB, 0x300, 0x100, 0, 0x1DB)
 * and takes the mask as one bit selected by the current record's
 * (D_800C3E1C) +0xAC low two bits and +0xA8 top two bits; otherwise the mask
 * is D_800D3634. */
void func_800B62C8(u32 unused, u8* p) {
    void* stack = HeapAlloc(0x4000, 1);
    u32 mask;

    if (p[0] == 2) {
        u32 rec;
        u32 bit;

        func_800A979C(0xB, 0x300, 0x100, 0, 0x1DB);
        rec = BG_U32(0x800C3E1C); /* D_800C3E1C */
        bit = ((BG_U32(rec + 0xAC) & 3) << 2) | (BG_U32(rec + 0xA8) >> 30);
        mask = 1u << (bit & 31);
    } else {
        mask = BG_U16(0x800D3634); /* D_800D3634 */
    }
    func_800BEE2C(0xB, mask, p[0]);
    HeapFree(stack);
}
#endif
