#include "common.h"
#ifdef XENO_PC_PORT
#include <stdio.h>
#include "psx_memory.h"
#include "guest_prim_link.h"
#endif
#include "field/actor.h"
#include "psyq/libgpu.h"
#include "psyq/libgte.h"
#include "system/memory.h"

/* Retail TU 0x800250E0..0x80025C04 (sprite/prim work-buffer allocation and
 * render callbacks), one of the TUs that were merged into system/temp1.c (the
 * groups reach the shared Gfx globals differently: %gp_rel for their own
 * .sbss, absolute for other TUs'). This is the only copy of these bodies; the
 * port builds it too. Built with gcc-2.7.2-cdk + aspsx 2.67 (XenoGraphics).
 *
 * It is itself three retail TUs: temp1e proper (func_800250E0,
 * func_80025180), temp1e1 (func_800251C8 alone, which only gcc-2.6.3
 * reproduces) and temp1e2 (func_80025224..). temp1e1.c / temp1e2.c build
 * their part by including this file with TEMP1E_PART_1 / TEMP1E_PART_2; the
 * .sbss definitions below are commons, emitted by every part as retail's
 * %gp_rel accesses require. The port builds all three parts here. */
#ifdef TEMP1E_PART_2
#define TEMP1E_ASM "asm/slus_006.64/nonmatchings/system/temp1e2"
#else
#define TEMP1E_ASM "asm/slus_006.64/nonmatchings/system/temp1e"
#endif

extern void* g_GfxWorkBuffer2;
extern u32 g_GfxImageList[];
extern u_long* g_GfxCurOT;
extern MATRIX D_8004FBB8;

/* This TU's own .sbss: retail reaches it %gp_rel, which cc1 only emits for a
 * same-TU definition (g_GfxWorkBufferSize is shared with temp1b as a
 * common). The port keeps the storage elsewhere. */
#ifdef XENO_PC_PORT
extern s32 g_GfxCurContext;
extern s32 g_GfxWorkBufferSize;
extern uintptr_t D_80059524;
extern void* g_GfxCurWorkBufferEnd;
extern void* g_GfxCurWorkBuffer;
extern u32 D_80059300;
extern u32 D_80059304;
extern void* g_GfxWorkBuffers;
static u8* SpriteRenderAddress(uintptr_t address);
#else
s32 g_GfxCurContext;
s32 g_GfxWorkBufferSize;
s32 D_80059524;
void* g_GfxCurWorkBufferEnd;
void* g_GfxCurWorkBuffer;
extern u32 D_80059300[2];
extern void* g_GfxWorkBuffers[2];
#endif

#if defined(XENO_PC_PORT) || (!defined(TEMP1E_PART_1) && !defined(TEMP1E_PART_2))
/* Starts a frame on work buffer `context`: resets the bump pointer and frees
 * the heap blocks queued on that context's list (nodes: pData, pNext). The
 * port keeps the two contexts in separate variables and resolves the
 * serialized links through the host memory map. */
void func_800250E0(int context) {
#ifdef XENO_PC_PORT
    u8* pPrimBuffer = context ? (u8*)g_GfxWorkBuffer2 : (u8*)g_GfxWorkBuffers;
    u32* pListHead = context ? &D_80059304 : &D_80059300;
    u8* pCurEntry = (u8*)(uintptr_t)*pListHead;
    /* Battle effects serialize guest pointers into the deferred-free list.
     * Resolve each packed slot independently, including mixed native nodes. */
    pCurEntry = SpriteRenderAddress((uintptr_t)pCurEntry);
#else
    u8* pPrimBuffer = (u8*)g_GfxWorkBuffers[context];
    u8* pCurEntry = (u8*)(uintptr_t)D_80059300[context];
#endif

    g_GfxCurContext = context;
    g_GfxCurWorkBuffer = pPrimBuffer;
    D_80059524 = (uintptr_t)pPrimBuffer;
    g_GfxCurWorkBufferEnd = pPrimBuffer + g_GfxWorkBufferSize;

    while (pCurEntry != NULL) {
#ifdef XENO_PC_PORT
        HeapFree(SpriteRenderAddress(*(u32*)(pCurEntry + 0x0)));
        pCurEntry = SpriteRenderAddress(*(u32*)(pCurEntry + 0x4));
#else
        HeapFree((void*)(uintptr_t)*(u32*)(pCurEntry + 0x0));
        pCurEntry = (u8*)(uintptr_t)*(u32*)(pCurEntry + 0x4);
#endif
    }

#ifdef XENO_PC_PORT
    *pListHead = 0;
#else
    D_80059300[context] = 0;
#endif
}

/* Queues pData to be freed when this context's buffer is next reused: bumps
 * an 8-byte node off the work buffer (retail advances even when it is NULL).
 * The port owns this symbol as a host-safe override in
 * pc_port/src/game_overrides.c, so the body is retail-only. */
#ifndef XENO_PC_PORT
typedef struct GfxFreeNode {
    void* pData;
    struct GfxFreeNode* pNext;
} GfxFreeNode;

void func_80025180(void* pData) {
    GfxFreeNode* pEntry = (GfxFreeNode*)g_GfxCurWorkBuffer;

    g_GfxCurWorkBuffer = pEntry + 1;
    if (pEntry != NULL) {
        pEntry->pData = pData;
        pEntry->pNext = (GfxFreeNode*)D_80059300[g_GfxCurContext];
        D_80059300[g_GfxCurContext] = (u32)pEntry;
    }
}
#endif
#endif /* temp1e proper */

#if defined(XENO_PC_PORT) || defined(TEMP1E_PART_1)
/* GfxQueueShapeTransfer: queues a LoadImage of addr into (x, y, width,
 * height) on this frame's g_GfxImageList[g_GfxCurContext]. The entry's links
 * are 32-bit fields in both builds. */
typedef struct GfxQueuedImage {
    RECT rect;
    u32 addr;
    u32 pNext;
} GfxQueuedImage;

void func_800251C8(u_long* addr, int x, int y, int width, int height) {
    GfxQueuedImage* pCurrent = (GfxQueuedImage*)g_GfxCurWorkBuffer;
    GfxQueuedImage* pNext = pCurrent + 1;

    if ((void*)pNext < g_GfxCurWorkBufferEnd) {
        pCurrent->rect.h = height;
        pCurrent->rect.x = x;
        pCurrent->rect.y = y;
        pCurrent->rect.w = width;
        pCurrent->addr = (u32)(uintptr_t)addr;
        g_GfxCurWorkBuffer = (void*)pNext;
        pCurrent->pNext = g_GfxImageList[g_GfxCurContext];
        g_GfxImageList[g_GfxCurContext] = (u32)(uintptr_t)pCurrent;
    }
}
#endif /* temp1e1 */

#if defined(XENO_PC_PORT) || defined(TEMP1E_PART_2)
/* Sets a sprite task's trigger callback from the D_8004FD40 handler table
 * (16 entries: 80025258, 80025710, 80025718, 0, 0, 80025258, 80025258,
 * 80025718, 8002541C, 80025544, 0, 0, 0, 0, 80025258, 800257F0). */
extern void (*D_8004FD40[])(void*);
extern void WorkListSetTaskCallback(void* pTask, void (*pCallback)(void*));

void func_80025224(void* pTask, int handlerIndex) {
#ifdef TEMP1_25224_MUTANT_INDEX_PLUS1
    WorkListSetTaskCallback(pTask, D_8004FD40[handlerIndex + 1]);
#else
    WorkListSetTaskCallback(pTask, D_8004FD40[handlerIndex]);
#endif
}


extern s32 D_80050100;
extern u8 D_800C3664;
extern void func_8001E3D8(void* pSpriteData, void* ot);
extern void func_8001E298(void* pSpriteData, void* ot);

#ifdef XENO_PC_PORT
static u8* SpriteRenderAddress(uintptr_t address) {
#ifdef XENO_PC_PORT
    if ((address & ~(uintptr_t)0x1FFFFFu) == 0x80000000u ||
        (address & ~(uintptr_t)0x1FFFFFu) == 0xA0000000u) {
        return PSX_ADDR(address);
    }
#endif
    return (u8*)address;
}
#endif

#ifndef XENO_PC_PORT
INCLUDE_ASM(TEMP1E_ASM, func_80025258);
#else
/* Retail 80025258..80025418: shared render callback for types 0/5/6/14.
 * Task fields and OT entries remain four bytes on the native host. */
void func_80025258(u8* pEntry) {
    u8* pSprite = SpriteRenderAddress(*(u32*)(pEntry + 0x04));
    u32 flagsB0 = *(u32*)(pSprite + 0xB0);
    s32 depth;
    u32 flags3C;
    SVECTOR pos;
    long pxy = 0;
    long flg = 0;

    if ((flagsB0 >> 8) & 1) {
#ifdef XENO_PC_PORT
        if (*(u8*)PSX_ADDR(0x800C3664u) != 0) return;
#else
        if (D_800C3664 != 0) return;
#endif
    }

    pos.vx = *(s16*)(pSprite + 0x02);
    pos.vy = *(s16*)(pSprite + 0x06);
    pos.vz = *(s16*)(pSprite + 0x0A);
    SetRotMatrix(&D_8004FBB8);
    SetTransMatrix(&D_8004FBB8);
    depth = (s32)RotTransPers(&pos, &pxy, &pxy, &flg) >> (D_80050100 & 31);
    depth = (s32)((u32)depth + (u32)(s32)*(s16*)(pSprite + 0x30));
    if (flg & 0x8000) {
        depth = 0;
    }

    flags3C = *(u32*)(pSprite + 0x3C);
    *(u16*)(pSprite + 0x2E) = (u16)depth;
    if ((flags3C >> 24) & 1) {
        VECTOR trans;
        func_80022038(pSprite);
        trans.vx = *(s16*)(pSprite + 0x02);
        trans.vy = *(s16*)(pSprite + 0x06);
        trans.vz = *(s16*)(pSprite + 0x0A);
        TransMatrix((MATRIX*)(SpriteRenderAddress(*(u32*)(pSprite + 0x20)) + 0xC), &trans);
        SetRotMatrix((MATRIX*)(SpriteRenderAddress(*(u32*)(pSprite + 0x20)) + 0xC));
        SetTransMatrix((MATRIX*)(SpriteRenderAddress(*(u32*)(pSprite + 0x20)) + 0xC));
        /* The transform helper can change flags: retail reloads them here. */
        depth = (*(u32*)(pSprite + 0x3C) & 0x02000000u)
                    ? 0xFFF : *(s16*)(pSprite + 0x30);
        if ((u32)depth - 1u < 0xFFFu) {
            func_8001E3D8(pSprite,
                SpriteRenderAddress((uintptr_t)g_GfxCurOT) + ((u32)depth << 2));
        }
    } else {
        if ((flags3C >> 29) & 1) {
            u8* pSub = SpriteRenderAddress(*(u32*)(pSprite + 0x70));
            depth = *(s16*)(pSub + 0x2E);
        }
        if ((u32)depth - 1u < 0xFFFu) {
            func_8001E298(pSprite,
                SpriteRenderAddress((uintptr_t)g_GfxCurOT) + ((u32)depth << 2));
        }
    }
}
#endif

extern s32 D_80050100;

#ifndef XENO_PC_PORT
INCLUDE_ASM(TEMP1E_ASM, func_8002541C);
#else
/* Retail 8002541C..80025540: projected TILE_1 (12-byte packet, tag len 2)
 * followed by an 8-byte draw-mode prim. Sibling of func_80025544. */
void func_8002541C(u8* pEntry) {
    u8* pSprite = SpriteRenderAddress(*(u32*)(pEntry + 0x04));
    u32 cursor, next, otOffset, otAddress, color;
    u8* pTile;
    u8* pMode;
    SVECTOR v0;
    long flag = 0;
    s32 depth;

    if (*(u16*)(pSprite + 0x34) != 0) {
        return;
    }
    cursor = (u32)(uintptr_t)g_GfxCurWorkBuffer;
    next = cursor + 0xCu;
    if (next >= (u32)(uintptr_t)g_GfxCurWorkBufferEnd) {
        return;
    }
    pTile = SpriteRenderAddress(cursor);
    v0.vx = *(s16*)(pSprite + 0x02);
    v0.vy = *(s16*)(pSprite + 0x06);
    v0.vz = *(s16*)(pSprite + 0x0A);
    v0.pad = 0;
    g_GfxCurWorkBuffer = (void*)(uintptr_t)next;
    SetRotMatrix(&D_8004FBB8);
    SetTransMatrix(&D_8004FBB8);
    depth = (s32)RotTransPers(&v0, (long*)(pTile + 8), &flag, &flag);
    depth >>= (D_80050100 & 31);
    *(u16*)(pSprite + 0x2E) = (u16)depth;
    pTile[3] = 2;
    color = *(u32*)(pSprite + 0x28);
    *(u32*)(pTile + 4) = color;
    otAddress = (u32)(uintptr_t)g_GfxCurOT;
    otOffset = (u32)depth << 2;
    PcPort_AddPrimDomainAware(SpriteRenderAddress(otAddress + otOffset), pTile);

    cursor = (u32)(uintptr_t)g_GfxCurWorkBuffer;
    next = cursor + 8u;
    if (next >= (u32)(uintptr_t)g_GfxCurWorkBufferEnd) {
        return;
    }
    g_GfxCurWorkBuffer = (void*)(uintptr_t)next;
    pMode = SpriteRenderAddress(cursor);
    pMode[3] = 1;
    *(u32*)(pMode + 4) = 0xE1000000u | (*(u32*)(pSprite + 0x3C) & 0x60u);
    PcPort_AddPrimDomainAware(SpriteRenderAddress(otAddress + otOffset), pMode);
}
#endif

#ifndef XENO_PC_PORT
INCLUDE_ASM(TEMP1E_ASM, func_80025544);
#else
/* Retail 80025544..80025710: projected square TILE followed by draw mode. */
void func_80025544(u8* pEntry) {
    u8* pSprite = SpriteRenderAddress(*(u32*)(pEntry + 0x04));
    u16 size;
    u32 cursor, next, otOffset, otAddress, color;
    u32 originX, originY;
    s32 depth, dimension, halfSize;
    u8* pTile;
    u8* pMode;
    SVECTOR v0, v1;
    long xy0 = 0, xy1 = 0, xy2 = 0, sharedFlag = 0;

    if (*(u16*)(pSprite + 0x34) != 0) return;
    size = *(u16*)(pSprite + 0x36);
    cursor = (u32)(uintptr_t)g_GfxCurWorkBuffer;
    next = cursor + 0x10u;
    if (next >= (u32)(uintptr_t)g_GfxCurWorkBufferEnd) return;
    pTile = SpriteRenderAddress(cursor);

    v0.vx = *(s16*)(pSprite + 0x02);
    v0.vy = *(s16*)(pSprite + 0x06);
    v0.vz = *(s16*)(pSprite + 0x0A);
    v0.pad = 0;
    g_GfxCurWorkBuffer = (void*)(uintptr_t)next;
    SetRotMatrix(&D_8004FBB8);
    SetTransMatrix(&D_8004FBB8);
    v1 = v0;
    v1.vx = (s16)((u16)v0.vx + size);
    depth = (s32)RotTransPers3(&v0, &v1, &v0, &xy0, &xy1, &xy2,
                              &sharedFlag, &sharedFlag);
    *(u32*)(pTile + 0x08) = (u32)xy0;
    depth >>= (D_80050100 & 31);
    *(u16*)(pSprite + 0x2E) = (u16)depth;

    dimension = (s32)(s16)(u32)xy1 - (s32)*(s16*)(pTile + 0x08);
    originX = *(u16*)(pTile + 0x08);
    if (dimension == 0) dimension = 1;
    if (dimension < 0) dimension = -dimension;
    halfSize = dimension >> 1;
    originY = *(u16*)(pTile + 0x0A);
    *(u16*)(pTile + 0x08) = (u16)(originX - (u32)halfSize);
    *(u16*)(pTile + 0x0A) = (u16)(originY - (u32)halfSize);
    color = *(u32*)(pSprite + 0x28);
    pTile[3] = 3;
    *(u32*)(pTile + 4) = color;
    otAddress = (u32)(uintptr_t)g_GfxCurOT;
    otOffset = (u32)depth << 2;
    *(u16*)(pTile + 0x0E) = (u16)dimension;
    *(u16*)(pTile + 0x0C) = (u16)dimension;
#ifdef XENO_PC_PORT
    PcPort_AddPrimDomainAware(SpriteRenderAddress(otAddress + otOffset), pTile);
#else
    AddPrim(SpriteRenderAddress(otAddress + otOffset), pTile);
#endif

    cursor = (u32)(uintptr_t)g_GfxCurWorkBuffer;
    next = cursor + 8u;
    if (next >= (u32)(uintptr_t)g_GfxCurWorkBufferEnd) return;
    g_GfxCurWorkBuffer = (void*)(uintptr_t)next;
    pMode = SpriteRenderAddress(cursor);
    pMode[3] = 1;
    otAddress = (u32)(uintptr_t)g_GfxCurOT;
    *(u32*)(pMode + 4) = 0xE1000000u | (*(u32*)(pSprite + 0x3C) & 0x60u);
#ifdef XENO_PC_PORT
    PcPort_AddPrimDomainAware(SpriteRenderAddress(otAddress + otOffset), pMode);
#else
    AddPrim(SpriteRenderAddress(otAddress + otOffset), pMode);
#endif
}
#endif

void func_80025710(void) {}

extern MATRIX D_8004FBB8;
extern void func_8002C700(void* a, void* b, u_long* ot, s32 flags);

#ifndef XENO_PC_PORT
INCLUDE_ASM(TEMP1E_ASM, func_80025718);
#else
void func_80025718(u8* pEntry) {
    u8* pSprite = *(u8**)(pEntry + 0x04);
    u8* pSub;
    u8* pMatrix;
    SVECTOR trans;

    func_80022038(pSprite);
    pSub = *(u8**)(pSprite + 0x20);
    if (*(u32*)(pSub + 0x34) == 0) return;

    pMatrix = pSub + 0x0C;
    trans.vx = *(s16*)(pSprite + 0x02);
    trans.vy = *(s16*)(pSprite + 0x06);
    trans.vz = *(s16*)(pSprite + 0x0A);
    TransMatrix((MATRIX*)pMatrix, &trans);

    if (!(*(u8*)(pSprite + 0x3F) & 1)) {
        MATRIX result;
        CompMatrix(&D_8004FBB8, (MATRIX*)(pSub + 0x0C), &result);
        SetRotMatrix(&result);
        SetTransMatrix(&result);
    } else {
        SetRotMatrix((MATRIX*)(pSub + 0x0C));
        SetTransMatrix((MATRIX*)(pSub + 0x0C));
    }

    {
        s32 ctxIdx = g_GfxCurContext;
        u32 flags = *(u16*)(pSprite + 0x42) & 4;
        func_8002C700(
            *(void**)(pSub + 0x34),
            *(void**)(pSub + 0x2C + ctxIdx * 4),
            g_GfxCurOT,
            flags
        );
    }
}
#endif

/* Transcribed from asm/slus_006.64/nonmatchings/system/temp1/func_800257F0.s
 * (0x800257F0-0x800258A0 ... 0x80025A6C). Sprite transform/render setup:
 * func_80022038 on the sprite, an optional colour/light matrix block built from
 * the +0x20 data (RotMatrix + two MulMatrix0 with D_8004FDA0, SetBackColor
 * 0x202020, SetColorMatrix/SetLightMatrix then PopMatrix), the +2/+6/+0xA
 * translation via TransMatrix, either CompMatrix(D_8004FBB8) or the raw matrix
 * for SetRotMatrix, SetTransMatrix, an optional geom-offset override
 * (ReadGeomOffset/SetGeomOffset 0xA0,0x70) and finally func_800B1F6C with either
 * the +0x30 half-word and D_80050100 unchanged, or D_80050100 = 0x10 with 0xFEC
 * (restored afterwards); the geom offset is restored when the flag word was
 * negative. */
extern void func_80022038(void* p);
extern u16 D_8004FD80[];
extern void func_800B1F6C();
extern MATRIX D_8004FDA0;

#ifndef XENO_PC_PORT
INCLUDE_ASM(TEMP1E_ASM, func_800257F0);
#else
void func_800257F0(u8* pArg) {
    u8* pSprite = *(u8**)(pArg + 4);
    u8* pData;
    MATRIX matA;
    MATRIX matB;
    VECTOR trans;
    MATRIX comp;
    s32 saved;
    s32 geomX;
    s32 geomY;
    u8 flag;
    s32 halved;

    func_80022038(pSprite);
    pData = *(u8**)(pSprite + 0x20);
    if (*(u32*)(pData + 0x34) == 0) {
        return;
    }

    if (((*(u32*)(pSprite + 0x40) >> 1) & 1) != 0) {
        PushMatrix();
        D_8004FD80[0] = *(u16*)(pData + 0x4C);
        D_8004FD80[3] = *(u16*)(pData + 0x4E);
        D_8004FD80[6] = *(u16*)(pData + 0x50);
        RotMatrix(pData + 0x44, &matA);
        MulMatrix0(&matA, pData + 0x0C, &matA);
        MulMatrix0(&D_8004FDA0, &matA, &matB);
        SetBackColor(0x20, 0x20, 0x20);
        SetColorMatrix((u32*)D_8004FD80);
        SetLightMatrix(&matB);
        PopMatrix();
    }

    trans.vx = *(s16*)(pSprite + 0x02);
    trans.vy = *(s16*)(pSprite + 0x06);
    trans.vz = *(s16*)(pSprite + 0x0A);
    TransMatrix(pData + 0x0C, &trans);

    if ((*(u8*)(pSprite + 0x3F) & 1) == 0) {
        CompMatrix(&D_8004FBB8, pData + 0x0C, &comp);
        SetRotMatrix(&comp);
        SetTransMatrix(&comp);
    } else {
        SetRotMatrix(pData + 0x0C);
        SetTransMatrix(pData + 0x0C);
    }

    flag = *(u8*)(pSprite + 0x3C);
    if (*(s32*)(pSprite + 0x3C) < 0) {
        ReadGeomOffset(&geomX, &geomY);
        SetGeomOffset(0xA0, 0x70);
    }
    halved = (s32)((*(u32*)(pSprite + 0x3C)) >> 25) & 1;

    if (halved == 0) {
        ((void (*)(s32, void*, void*, s32, s32, s32))func_800B1F6C)(*(s32*)(pData + 0x34),
                      *(void**)((u8*)pData + (g_GfxCurContext << 2) + 0x2C),
                      g_GfxCurOT, 0,
                      (s32)*(s16*)(pSprite + 0x30), (s32)(flag >> 5));
    } else {
        saved = D_80050100;
        D_80050100 = 0x10;
        ((void (*)(s32, void*, void*, s32, s32, s32))func_800B1F6C)(*(s32*)(pData + 0x34),
                      *(void**)((u8*)pData + (g_GfxCurContext << 2) + 0x2C),
                      g_GfxCurOT, 0, 0xFEC, (s32)(flag >> 5));
        D_80050100 = saved;
    }

    if (*(s32*)(pSprite + 0x3C) < 0) {
        SetGeomOffset(geomX, geomY);
    }
}
#endif

extern s32 D_80050100;
extern void func_800B1F6C(void* a, void* b, u_long* ot, s32 c, s32 d, s32 e, s32 f);

#ifndef XENO_PC_PORT
INCLUDE_ASM(TEMP1E_ASM, func_80025A88);
#else
void func_80025A88(u8* pEntry) {
    u8* pSprite = *(u8**)(pEntry + 0x04);
    u8* pSub;
    SVECTOR pos;
    VECTOR result;

    func_80022038(pSprite);
    pSub = *(u8**)(pSprite + 0x20);
    if (*(u32*)(pSub + 0x34) == 0) return;

    pos.vx = *(s16*)(pSprite + 0x02);
    pos.vy = *(s16*)(pSprite + 0x06);
    pos.vz = *(s16*)(pSprite + 0x0A);
    ApplyMatrix(&D_8004FBB8, &pos, &result);

    pSub = *(u8**)(pSprite + 0x20);
    *(s32*)(pSub + 0x20) = D_8004FBB8.t[0] + result.vx;
    *(s32*)(pSub + 0x24) = D_8004FBB8.t[1] + result.vy;
    *(s32*)(pSub + 0x28) = D_8004FBB8.t[2] + result.vz;

    SetRotMatrix((MATRIX*)(pSub + 0x0C));
    SetTransMatrix((MATRIX*)(pSub + 0x0C));

    {
        s32 ctxIdx = g_GfxCurContext;
        u32 flags3C = *(u32*)(pSprite + 0x3C);
        s32 arg7;
        s32 oldShift;
        if ((flags3C >> 25) & 1) {
            oldShift = D_80050100;
            D_80050100 = 0x10;
            arg7 = 0xFEC;
        } else {
            arg7 = *(s16*)(pSprite + 0x30);
        }
        func_800B1F6C(
            *(void**)(pSub + 0x34),
            *(void**)(pSub + 0x2C + ctxIdx * 4),
            g_GfxCurOT, 0, arg7, (flags3C >> 5) & 1, 0
        );
        if ((flags3C >> 25) & 1) {
            D_80050100 = oldShift;
        }
    }
}
#endif
#endif /* temp1e2 */
