#include "common.h"
#include "system/graphics.h"
#include "system/memory.h"
#include "psyq/libgpu.h"
#ifdef XENO_PC_PORT
#include "../../../pc_port/src/psx_memory.h"

_Static_assert(sizeof(LineScroll) == 0x18, "LineScroll must match retail 0x18");
_Static_assert(__builtin_offsetof(LineScroll, pData) == 0x10,
               "LineScroll pData must stay at retail +0x10");
_Static_assert(__builtin_offsetof(LineScroll, unk14) == 0x14,
               "LineScroll unk14 must stay at retail +0x14");

static void* LineScrollSlotPtr(u32 slot)
{
    if (slot == 0)
        return NULL;
    if ((slot & 0xffe00000u) == 0x80000000u ||
        (slot & 0xffe00000u) == 0xa0000000u)
        return PSX_ADDR(slot);
    return (void*)(uintptr_t)slot;
}
#endif

LineScroll* GfxLineScrollInitialize(LineScroll* pLineScroll, 
    u16 x, u16 y, u16 modulus, s16 width, s16 numLines, u16 destX, u16 destY, s8* pData
) {
    u_short* pWork;
    int i;

    HeapChangeCurrentUser(HEAP_USER_MASA, 0);
    pLineScroll->x = x;
    pLineScroll->y = y;
    pLineScroll->modulus = modulus;
    pLineScroll->width = width;
    pLineScroll->numLines = numLines;
    pLineScroll->destX = destX;
    pLineScroll->destY = destY;
#ifdef XENO_PC_PORT
    pLineScroll->pData = PsxMemory_GuestAddr(pData);
#else
    pLineScroll->pData = pData;
#endif
    pLineScroll->height = width / numLines;
    
    pWork = HeapAlloc(numLines * sizeof(u_short), 0);
#ifdef XENO_PC_PORT
    pLineScroll->unk14 = PsxMemory_GuestAddr(pWork);
#else
    pLineScroll->unk14 = pWork;
#endif
    if (pWork == 0) {
        pLineScroll = NULL;
    } else {
        for (i = 0; i < numLines; i++) {
#ifdef XENO_PC_PORT
            ((u_short*)LineScrollSlotPtr(pLineScroll->unk14))[i] = 0;
#else
            pLineScroll->unk14[i] = 0;
#endif
        }
    }
    return pLineScroll;
}

void GfxLineScrollUpdate(LineScroll* pLineScroll) {
    RECT srcRect;
    int i;
    u16 destY;
    u16 offset;
    u16 remainder;
#ifdef XENO_PC_PORT
    u_short* pOff;
    s8* pDat;

    pOff = (u_short*)LineScrollSlotPtr(pLineScroll->unk14);
    pDat = (s8*)LineScrollSlotPtr(pLineScroll->pData);
    if (!pOff) {
        return;
    }
#else
    if (!pLineScroll->unk14) {
        return;
    }
#endif

    srcRect.y = pLineScroll->y;
    srcRect.h = pLineScroll->height;
    destY = pLineScroll->destY;
    
    for (i = 0; i < pLineScroll->numLines; i++) {
#ifdef XENO_PC_PORT
        pOff[i] += pDat[i];
#else
        pLineScroll->unk14[i] += pLineScroll->pData[i];
#endif

        // Divide pLineScroll->unk14[i] by 0x10 and modulo it
#ifdef XENO_PC_PORT
        remainder = (u32) (((pOff[i] << 0x10) >> 0x14) & 0xFFFF) % pLineScroll->modulus;
#else
        remainder = (u32) (((pLineScroll->unk14[i] << 0x10) >> 0x14) & 0xFFFF) % pLineScroll->modulus;
#endif
        
        offset = pLineScroll->modulus - remainder;
        
        srcRect.x = pLineScroll->x;
        srcRect.w = remainder;
        MoveImage(&srcRect, pLineScroll->destX + offset, destY);
        
        srcRect.x = remainder + pLineScroll->x;
        srcRect.w = offset;
        MoveImage(&srcRect, pLineScroll->destX, destY);
        
        destY += pLineScroll->height;
        srcRect.y += pLineScroll->height;
    }
}

void GfxLineScrollFree(LineScroll* pLineScroll) {
#ifdef XENO_PC_PORT
    void* pWork = LineScrollSlotPtr(pLineScroll->unk14);
    if (pWork) {
        HeapFree(pWork);
        pLineScroll->unk14 = 0;
    }
#else
    if (pLineScroll->unk14) {
        HeapFree(pLineScroll->unk14);
        pLineScroll->unk14 = NULL;
    }
#endif
}


// NOTE: Probably part of libarchive, not line_scroll
/* Transcribed from asm/slus_006.64/nonmatchings/graphics/line_scroll/func_8002804C.s
 * (0x8002804C-0x80028224, 121 instructions). Disc-error retry screen: paints an
 * 8x0x1C0 marker at x = retryCount*10 + 10 (white once the count reaches 4),
 * ClearImage + DrawSync, and only from the fourth retry re-initialises the GPU
 * (ResetGraph/InitGeom, default draw/disp envs with dtd=0/dfE=1/isbg=0 stored by
 * offset, the font, a 0x280x0x30 clear) and then spins forever redrawing an
 * 8-entry OT with the current archive offset and file path. The retry loop never
 * returns, so callers only leave it via a reset/interrupt. */
extern char* ArchiveGetFilePath(s32);
extern u32 g_CurArchiveOffset;
extern void FontLoadFont(s32, s32, s32, s32, s32, s32, s32, s32, s32, s32, s32, s32);
extern void FontDrawLetters(void*);
extern char D_800188EC[];
extern char D_800188F0[];
extern u32 D_80059F0C;

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/slus_006.64/nonmatchings/graphics/line_scroll", func_8002804C);
#else
void func_8002804C(s32 retryCount, u8 r, u8 g, u8 b) {
    RECT rect;
    DRAWENV draw;
    DISPENV disp;
    u_long ot[8];
    s32 retry = retryCount + 1;

    rect.x = (s16)(retryCount * 10 + 10);
    rect.y = 0;
    rect.w = 8;
    rect.h = 0x1C0;
    if (retry >= 4) {
        r = 0xFF;
        g = 0xFF;
        b = 0xFF;
    }
    ClearImage(&rect, r, g, b);
    DrawSync(0);
    if (retry < 4) {
        return;
    }

    ResetGraph(0);
    InitGeom();
    SetDefDrawEnv(&draw, 0, 0, 0x140, 0x100);
    SetDefDispEnv(&disp, 0, 0, 0x140, 0xF0);
    ((u8*)&draw)[0x16] = 0;
    ((u8*)&draw)[0x17] = 1;
    ((u8*)&draw)[0x18] = 0;
    PutDrawEnv(&draw);
    ((u8*)&disp)[0x10] = 0;
    PutDispEnv(&disp);
    FontLoadFont(0x10, 0x10, 0x280, 0xF0, 0x400, 0x280, 0, 0x280, 0, 0x280, 0x100, 0);
    SetDispMask(1);

    rect.x = 0;
    rect.y = 0;
    rect.w = 0x280;
    rect.h = 0x30;
    ClearImage(&rect, 0, 0, 0);

    for (;;) {
        ClearOTagR(ot, 8);
        FontPrintf(D_800188EC, (s32)(g_CurArchiveOffset + D_80059F0C - 1));
        FontPrintf(D_800188F0, (s32)ArchiveGetFilePath(D_80059F0C));
        FontDrawLetters(ot);
        DrawOTag(&ot[7]);
        DrawSync(0);
    }
}
#endif
