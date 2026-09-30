#include "common.h"
#include "psyq/libgpu.h"
#include "system/memory.h"
#include "system/math.h"
#include "field/main.h"
#include "field/actor.h"
#include "field/effects.h"
#include "field/graphics.h"
#include "field/particles.h"
#ifdef XENO_PC_PORT
#include "../../../pc_port/src/psx_memory.h"
#include "../../../pc_port/src/krom_rom.h"
#endif


#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/field/nonmatchings/main/misc9", func_800AA9DC);
#else
void func_800AA9DC(void* pModelData) {
    u8* pModel = pModelData;
    u8* pBounds = (u8*)(uintptr_t)*(u32*)(pModel + 0x4);
    s32 minX = *(s16*)(pBounds + 0x20);
    s32 minY = *(s16*)(pBounds + 0x22);
    s32 minZ = *(s16*)(pBounds + 0x24);
    s32 dx = *(s16*)(pBounds + 0x28) - minX;
    s32 dy = *(s16*)(pBounds + 0x2A) - minY;
    s32 dz = *(s16*)(pBounds + 0x2C) - minZ;
    s32 max = dx;

    if (max < dy) {
        max = dy;
    }
    if (max < dz) {
        max = dz;
    }

    *(s16*)(pModel + 0x18) = minX + ((dx + ((u32)dx >> 31)) >> 1);
    *(s16*)(pModel + 0x1A) = minY + ((dy + ((u32)dy >> 31)) >> 1);
    *(s16*)(pModel + 0x1C) = minZ + ((dz + ((u32)dz >> 31)) >> 1);
    *(s16*)(pModel + 0x20) = (max << 1) + 1;
}
#endif /* XENO_PC_PORT */

extern MATRIX D_800B00E8;
/* On PSX, D_800B00FC/D_800B0100/D_800B0104 alias D_800B00E8+0x14/+0x18/+0x1C —
 * i.e. they ARE D_800B00E8.t[0..2]. The port's generated data symbols detach
 * them, so writes below go directly into the matrix translation instead. */
extern s32 D_800C3A5C;
extern s32 D_800C3A60;

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/field/nonmatchings/main/misc9", func_800AAA74);
#else
s32 func_800AAA74(void* pModelData) {
    u8* pModel = (u8*)pModelData;
    VECTOR transformed;
    long flag;
    SVECTOR corner;
    long screenXY;
    long p;
    s32 radius;
    s32 minX;
    s32 minY;
    s32 maxX;
    s32 maxY;

    RotTrans((SVECTOR*)(pModel + 0x18), &transformed, &flag);
    D_800B00E8.t[0] = transformed.vx;
    D_800B00E8.t[1] = transformed.vy;
    D_800B00E8.t[2] = transformed.vz;

    SetRotMatrix(&D_800B00E8);
    SetTransMatrix(&D_800B00E8);

    radius = *(s16*)(pModel + 0x20);
    corner.vx = -radius;
    corner.vy = -radius;
    corner.vz = 0;
    corner.pad = 0;
    RotTransPers(&corner, &screenXY, &p, &flag);
    minX = (s16)(screenXY >> 16);
    minY = (s16)screenXY;

    corner.vx = radius;
    corner.vy = radius;
    corner.vz = 0;
    corner.pad = 0;
    RotTransPers(&corner, &screenXY, &p, &flag);
    maxX = (s16)(screenXY >> 16);
    maxY = (s16)screenXY;

    if (minX >= D_800C3A60 + 0xE0) {
        return -1;
    }
    if (-D_800C3A60 >= maxX) {
        return -1;
    }
    if (minY >= D_800C3A5C + 0x140) {
        return -1;
    }
    if (-D_800C3A5C >= maxY) {
        return -1;
    }
    return 0;
}
#endif /* XENO_PC_PORT */

extern u32 D_800AFC68;
#define D_800AFC68_PTR ((SpriteList*)(uintptr_t)D_800AFC68)

void func_800AABD8(void) {
    HeapFree((void*)(uintptr_t)D_800AFC68);
    DrawSync(0);
}

void func_800AAC08(void) {
    RECT rect;
    SPRT* pSprite;
    SPRT* pSprite2;
    int i;

    D_800AFC68 = (u32)(uintptr_t)HeapAlloc(0x840, 0x0);
    
    rect.x = 0;
    rect.y = 0;
    rect.w = 0xFF;
    rect.h = 0xFF;
    
    for (i = 0; i < 0x21; i++) {
        SetDrawMode(&D_800AFC68_PTR->drModes[i][0], 0, 0, GetTPage(0, 0, 0x3C0, 0x100) & 0xFFFF, &rect);
        SetDrawMode(&D_800AFC68_PTR->drModes[i][1], 0, 0, GetTPage(0, 0, 0x3C0, 0x140) & 0xFFFF, &rect);
        pSprite = &D_800AFC68_PTR->sprites[i][0];
        pSprite2 = &D_800AFC68_PTR->sprites[i][1];
        
        SetSprt(pSprite);
        setRGB0(pSprite, 0x80, 0x80, 0x80);
        if (i == 0) {
            setUV0(pSprite, 0xE0, 0x70);
            setWH(pSprite, 0x10, 0x10);
        } else {
            setUV0(pSprite, 0xE0, 0x60);
            setWH(pSprite, 8, 8);
        }
        setXY0(pSprite, 0xA0, 0x70);
        pSprite->clut = GetClut(0x100, 0xF7);
        *pSprite2 = *pSprite;
    };
}

// Set RGB of sprites
void func_800AADC8(int index, int red, int green, int blue) {
    setRGB0(&D_800AFC68_PTR->sprites[index][0], red, green, blue);
    setRGB0(&D_800AFC68_PTR->sprites[index][1], red, green, blue);
}

void func_800AAE4C(int index, int x, int y, int type) {
    switch (type) {
        case 0:
            y -= 0xC;
            x -= 4;
            break;
        case 1:
            y -= 4;
            x -= 4;
            break;
    }

    D_800AFC68_PTR->sprites[index][g_FieldCurRenderContextIndex].x0 = x;
    D_800AFC68_PTR->sprites[index][g_FieldCurRenderContextIndex].y0 = y;
    addPrim(g_FieldCurRenderContext->ot3, &D_800AFC68_PTR->sprites[index][g_FieldCurRenderContextIndex]);
    addPrim(g_FieldCurRenderContext->ot3, &D_800AFC68_PTR->drModes[index][g_FieldCurRenderContextIndex]);
}

extern u32 D_800B1DF0;
extern u32 D_800C3A3C;
#define D_800B1DF0_PTR ((SpriteList2*)(uintptr_t)D_800B1DF0)
#define D_800C3A3C_PTR ((PolyList2*)(uintptr_t)D_800C3A3C)

void func_800AAF80(void) {
    RECT rect;
    POLY_FT4* pPoly;
    POLY_FT4* pPoly2;
    SPRT* pSprite;
    SPRT* pSprite2;
    int i;

    D_800C3A3C = (u32)(uintptr_t)HeapAlloc(0x2F8, 0x0);
    D_800B1DF0 = (u32)(uintptr_t)HeapAlloc(0x400, 0x0);
    
    rect.x = 0;
    rect.y = 0;
    rect.w = 0xFF;
    rect.h = 0xFF;
    
    for (i = 0; i < 4; i++) {
        SetDrawMode(&D_800B1DF0_PTR->drModes[i][0], 0, 0, GetTPage(0, 0, 0x3C0, 0x140), &rect);
        SetDrawMode(&D_800B1DF0_PTR->drModes[i][1], 0, 0, GetTPage(0, 0, 0x3C0, 0x140), &rect);
        pSprite = &D_800B1DF0_PTR->sprites[i][0];
        pSprite2 = pSprite + 1;
        SetSprt(pSprite);
        setRGB0(pSprite, 0x80, 0x80, 0x80);
        setXY0(pSprite, 0xA0, 0x70);
        if (i == 0) {
            setUV0(pSprite, 0xE0, 0x70);
            setWH(pSprite, 0x10, 0x10);
        } else {
            setUV0(pSprite, 0xE0, 0x60);
            setWH(pSprite, 0x8, 0x8);
        }
        pSprite->clut = GetClut(0x100, 0xF7);
        *pSprite2 = *pSprite;
    }

    for (i = 0; i < 3; i++) {
        pPoly = &D_800C3A3C_PTR->polys[i][0];
        pPoly2 = pPoly + 1;
        SetPolyFT4(pPoly);
        setXY4(pPoly, 
           i * 0x80, 0x0, 
           i * 0x80 + 0x80, 0x0, 
           i * 0x80, 0xDF, 
           i * 0x80 + 0x80, 0xDF
        );
        setRECT(&D_800C3A3C_PTR->rects[i][0], 0x0, 0x0, 0xFF, 0xFF);
        setRECT(&D_800C3A3C_PTR->rects[i][1], 0x0, 0x0, 0xFF, 0xFF);
        SetDrawMode(&D_800C3A3C_PTR->drModes[i][0], 0, 0, GetTPage(1, 0, 0x300 + (i * 0x40), 0x100), &D_800C3A3C_PTR->rects[i][0]);
        SetDrawMode(&D_800C3A3C_PTR->drModes[i][1], 0, 0, GetTPage(1, 0, 0x300 + (i * 0x40), 0x100), &D_800C3A3C_PTR->rects[i][1]);
        setRGB0(pPoly, 0x80, 0x80, 0x80);
        SetSemiTrans(pPoly, 0x1);
        setUV4(pPoly, 0x0, 0x0, 0x80, 0x0, 0x0, 0xDF, 0x80, 0xDF);
        pPoly->tpage = GetTPage(1, 0, 0x300 + (i * 0x40), 0x100);
        pPoly->clut = GetClut(0, 0xF6);
        *pPoly2 = *pPoly;
    }
}

extern void* g_pGameState;

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/field/nonmatchings/main/misc9", func_800AB328);
#else
s32 func_800AB328(u8 characterId) {
    u8* pGS = (u8*)g_pGameState;
    s32 i;
    for (i = 0; i < 0x96; i++) {
        if (pGS[0x2026 + i] == characterId && pGS[0x1F90 + i] != 0) {
            return i;
        }
    }
    return -1;
}
#endif /* XENO_PC_PORT */

extern s32 D_800AFE78;
extern s32 D_800AFE7C;
extern s32 g_PlayerActorIndex;
extern s32 D_800C3914;
extern s32 D_800C3A18;

void func_800AB378(s8 color) {
    ActorData* pActor;
    int i;
    int y;
    int x;
    
    pActor = (ActorData*)(uintptr_t)g_FieldActors[g_PlayerActorIndex].pActorData;
    x = (CONV_TO_GTE(pActor->position.vx) * D_800C3914) >> 0x10;
    y = -(CONV_TO_GTE(pActor->position.vz) * D_800C3A18) >> 0x10;
    
    for (i = 0; i < 1; i++) {
        if (i == 0) {
            y -= 0xC;
            x -= 4;
        }
        
        D_800B1DF0_PTR->sprites[i][g_FieldCurRenderContextIndex].x0 = x + D_800AFE78;
        D_800B1DF0_PTR->sprites[i][g_FieldCurRenderContextIndex].y0 = y + D_800AFE7C;

        setRGB0(&D_800B1DF0_PTR->sprites[i][g_FieldCurRenderContextIndex & 1], color, color, color);
        addPrim(g_FieldCurRenderContext->ot3, &D_800B1DF0_PTR->sprites[i][g_FieldCurRenderContextIndex]);
        addPrim(g_FieldCurRenderContext->ot3, &D_800B1DF0_PTR->drModes[i][g_FieldCurRenderContextIndex]);
    }

    for (i = 0; i < 3; i++) {
        setRGB0(&D_800C3A3C_PTR->polys[i][g_FieldCurRenderContextIndex & 1], color, color, color);
        addPrim(g_FieldCurRenderContext->ot3, &D_800C3A3C_PTR->polys[i][g_FieldCurRenderContextIndex]);
        addPrim(g_FieldCurRenderContext->ot3, &D_800C3A3C_PTR->drModes[i][g_FieldCurRenderContextIndex]);
    }
}

/* Transcribed from asm/field/nonmatchings/main/misc9/func_800AB748.s
 * (0x800AB748-0x800AB804). Retail: sltiu $a0, 5 then jr jtbl_8006FDD8
 * (five entries in asm/field/data/2D8.rodata.s). Cases 0-3 andi 0x8/0x10/
 * 0x20/0x40 and share .L800AB7CC (bit ? 0 : -1). Case 4 andi 0x80 inverts
 * (bit ? -1 : 0). Slot >= 5 returns -1. */
#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/field/nonmatchings/main/misc9", func_800AB748);
#else
s32 func_800AB748(s32 slot) {
    s32 ret;

    switch (slot) {
    case 0:
        ret = (*(u16*)((u8*)g_pGameState + 0x1A16) & 0x8) ? 0 : -1;
        break;
    case 1:
        ret = (*(u16*)((u8*)g_pGameState + 0x1A16) & 0x10) ? 0 : -1;
        break;
    case 2:
        ret = (*(u16*)((u8*)g_pGameState + 0x1A16) & 0x20) ? 0 : -1;
        break;
    case 3:
        ret = (*(u16*)((u8*)g_pGameState + 0x1A16) & 0x40) ? 0 : -1;
        break;
    case 4:
#ifdef FIELD_AB748_MUTANT_CASE4_SENSE
        ret = (*(u16*)((u8*)g_pGameState + 0x1A16) & 0x80) ? 0 : -1;
#else
        ret = (*(u16*)((u8*)g_pGameState + 0x1A16) & 0x80) ? -1 : 0;
#endif
        break;
    default:
        ret = -1;
        break;
    }

    return ret;
}
#endif /* XENO_PC_PORT */

/* Transcribed from asm/field/nonmatchings/main/misc9/func_800AB808.s
 * (0x800AB808-0x800ABA94). Loads archive 0x802, OpenTIM/ReadTIM, then for
 * slots 0..4: if func_800AB748(i)==-1 and TIM.paddr!=0, blit D_800AF5C6[i]
 * rows from paddr+(y+j)*320+x into a 0xF20 buffer and LoadImage to
 * (x/2+0x300, y+0x100, w/2, h). Always HeapFree both buffers. */
extern int ArchiveDecodeAlignedSize(int entryIndex);
extern s32 ArchiveReadFileToBuffer(s32 index, void* pBuffer, u32 arg2, u32 flags);
extern void ArchiveCdDataSync(int mode);
extern void* memcpy(void* dest, const void* src, int n);
extern s16 D_800AF5C0[];

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/field/nonmatchings/main/misc9", func_800AB808);
#else
void func_800AB808(void) {
    TIM_IMAGE tim;
    void* pBuf;
    u8* pImage;
    s32 i;
    s16* pX;
    s16* pY;
    s16* pW;
    s16* pH;

    pBuf = HeapAlloc(ArchiveDecodeAlignedSize(0x802), 0);
    ArchiveReadFileToBuffer(0x802, pBuf, 0, 0x80);
    ArchiveCdDataSync(0);
    pImage = (u8*)HeapAlloc(0xF20, 0);
    OpenTIM((u_long*)pBuf);
    if (ReadTIM(&tim) != NULL) {
        pX = D_800AF5C0;
        pY = D_800AF5C0 + 1;
        pW = D_800AF5C0 + 2;
        pH = D_800AF5C0 + 3;
        for (i = 0; i < 5; i++) {
#ifdef FIELD_AB808_MUTANT_SKIP_GATE
            if (tim.paddr != NULL)
#else
            if (func_800AB748(i) == -1 && tim.paddr != NULL)
#endif
            {
                s32 rows = *pH;
                if (rows > 0) {
                    s32 j;
                    u8* dst = pImage;
                    for (j = 0; j < rows; j++) {
                        s32 x = D_800AF5C0[i * 4];
                        s32 w;
                        if (x < 0) {
                            x += 3;
                        }
                        memcpy(dst,
                               (u8*)tim.paddr
                                   + (((*pY + j) * 0x50 + (x >> 2)) << 2),
                               *pW);
                        w = *pW;
                        if (w < 0) {
                            w += 3;
                        }
                        dst += (w >> 2) << 2;
                    }
                }
                tim.prect->x = *pX / 2 + 0x300;
                tim.prect->y = (s16)((u16)*pY + 0x100);
                tim.prect->w = *pW / 2;
                tim.prect->h = *pH;
                LoadImage(tim.prect, (u_long*)pImage);
                DrawSync(0);
            }
            pX += 4;
            pY += 4;
            pW += 4;
            pH += 4;
        }
    }
    HeapFree(pBuf);
    HeapFree(pImage);
}
#endif /* XENO_PC_PORT */

/* Transcribed from asm/field/nonmatchings/main/misc9/func_800ABA98.s
 * (0x800ABA98-0x800ABD14). Walk D_800AF47C (stride 0x20, terminator 0xFFFF)
 * for g_GameSceneMapNum&0x3FFF; func_800AB328(entry+0x10)==-1 returns.
 * Stores +4/+8/+14/+18 into D_800C3914/D_800C3A18/D_800AFE78/D_800AFE7C,
 * StoreImage 0x300,0x100,0xA0,0x100, loads archive (entry+0xC)+0x7FB via
 * FieldLoadTIMWithClut(..., 0x300, 0x100, 0, 0xF6, 0, 0). entry+0x1C==1
 * calls func_800AB808. Fade 0..0xF then wait D_800C3900&0x100 then fade
 * 0x10..1; restore the screenshot. */
extern s32 g_GameSceneMapNum;
extern s32 D_800AF47C[];
extern u16 D_800C3900;
extern void FieldLoadTIMWithClut(u_long* pTimData, short x, short y, short clutX,
                                 short clutY, short clutWidth, short clutHeight);
extern void FieldClearAndSwapOTag(void);
extern void FieldDisplay(void);
extern void FieldPollControllers(void);
extern int ArchiveSetIndex(int directoryIndex, int entryIndex);

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/field/nonmatchings/main/misc9", func_800ABA98);
#else
void func_800ABA98(void) {
    RECT rect;
    s32 i;
    s32 mapNum;
    s32* p;
    void* pShot;
    void* pTim;
    s32 archiveIndex;
    s32* entry;

    i = 0;
    p = D_800AF47C;
    mapNum = g_GameSceneMapNum & 0x3FFF;
    for (;;) {
        if (p[0] == 0xFFFF) {
            return;
        }
        if (p[0] == mapNum) {
            break;
        }
        p += 8;
        i += 1;
    }

    entry = (s32*)((u8*)D_800AF47C + (i << 5));
    if (func_800AB328(entry[4]) == -1) {
        return;
    }

    D_800C3914 = entry[1];
    D_800C3A18 = entry[2];
    archiveIndex = entry[3];
    D_800AFE78 = entry[5];
    D_800AFE7C = entry[6];

    rect.x = 0x300;
    rect.y = 0x100;
    rect.w = 0xA0;
    rect.h = 0x100;
    pShot = HeapAlloc(0x14000, 0);
    StoreImage(&rect, (u_long*)pShot);
    DrawSync(0);
    ArchiveCdDataSync(0);
    ArchiveSetIndex(4, 0);
    archiveIndex += 0x7FB;
    pTim = HeapAlloc(ArchiveDecodeAlignedSize(archiveIndex), 0);
    ArchiveReadFileToBuffer(archiveIndex, pTim, 0, 0x80);
    ArchiveCdDataSync(0);
    FieldLoadTIMWithClut((u_long*)pTim, 0x300, 0x100, 0, 0xF6, 0, 0);
    HeapFree(pTim);
#ifdef FIELD_ABA98_MUTANT_SKIP_AB808
    (void)entry[7];
#else
    if (entry[7] == 1) {
        func_800AB808();
    }
#endif
    func_800AAF80();
    for (i = 0; i < 0x10; i++) {
        FieldClearAndSwapOTag();
        func_800AB378(i << 3);
        FieldDisplay();
    }
    do {
        FieldClearAndSwapOTag();
        func_800AB378(0x80);
        FieldDisplay();
        FieldPollControllers();
    } while ((D_800C3900 & 0x100) == 0);
    for (i = 0x10; i > 0; i--) {
        FieldClearAndSwapOTag();
        func_800AB378(i << 3);
        FieldDisplay();
    }
    DrawSync(0);
    HeapFree((void*)(uintptr_t)D_800C3A3C);
    HeapFree((void*)(uintptr_t)D_800B1DF0);
    LoadImage(&rect, (u_long*)pShot);
    DrawSync(0);
    HeapFree(pShot);
}
#endif /* XENO_PC_PORT */


extern SpriteList3 D_800B0188;

void func_800ABD18(void) {
    int i;
    RECT rect;
    
    rect.x = 0;
    rect.y = 0;
    rect.w = 0xFF;
    rect.h = 0xFF;
    
    for (i = 0; i < 5; i++) {
        SetDrawMode(&D_800B0188.drModes[i][0], 0, 0, GetTPage(1, 0, 0x280 + (i * 0x40), 0), &rect);
        SetDrawMode(&D_800B0188.drModes[i][1], 0, 0, GetTPage(1, 0, 0x280 + (i * 0x40), 0), &rect);
        SetSprt(&D_800B0188.sprites[i][0]);
        setRGB0(&D_800B0188.sprites[i][0], 0x80, 0x80, 0x80);
        setXY0(&D_800B0188.sprites[i][0], i * 0x80, 0x0);
        setUV0(&D_800B0188.sprites[i][0], 0x0, 0x0);
        setWH(&D_800B0188.sprites[i][0], 0x80, 0xE0);
        SetSemiTrans(&D_800B0188.sprites[i][0], 0);
        (&D_800B0188.sprites[i][0])->clut = GetClut(0, 0xE8);
        D_800B0188.sprites[i][1] = D_800B0188.sprites[i][0];
    }
}

extern s16 D_800ADB54;

void func_800ABEC8(void) {
    int i;

    if (D_800ADB54) {
        for (i = 0; i < 5; i++) {
            addPrim(g_FieldCurRenderContext->ot3, &D_800B0188.sprites[i][g_FieldCurRenderContextIndex]);
            addPrim(g_FieldCurRenderContext->ot3, &D_800B0188.drModes[i][g_FieldCurRenderContextIndex]);
        }
    }
}

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/field/nonmatchings/main/misc9", func_800ABFDC);
#else
intptr_t func_800ABFDC(u8* pChar, s32* pOutIndex) {
    u16 code = (u16)((u16)pChar[0] << 8) | pChar[1];
    u16 range = (code + 0x7AC0) & 0xFFFF;
    if (range >= 0x340) {
        *pOutIndex = 0;
#ifdef XENO_PC_PORT
        return (intptr_t)PcPortKromFont(code, 30);
#else
        return Krom2RawAdd(code);
#endif
    }
    *pOutIndex = 1;
    return (s32)(u16)(code - 0x8540);
}
#endif /* XENO_PC_PORT */

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/field/nonmatchings/main/misc9", func_800AC03C);
#else
void func_800AC03C(u8* pOut, u16* pData, s32 count) {
    s32 i;
    if (count == -1) {
        /* Clear all pixels */
        for (i = 0x11F; i >= 0; i--) {
            *pOut++ = 0xFF;
        }
        return;
    }
    for (i = 0; i < 0xF; i++) {
        u16 val = *pData;
        s32 bit;
        u8* pCur = pOut;
        /* High byte bits 7..0 */
        for (bit = 7; bit >= 0; bit--) {
            *pCur++ = (u8)(0 - ((val >> bit) & 1));
        }
        /* Low byte bits 15..8 */
        for (bit = 15; bit >= 8; bit--) {
            *pCur++ = (u8)(0 - ((val >> bit) & 1));
        }
        *pCur++ = 0;
        *pCur++ = 0;
        pOut = pCur;
        pData++;
    }
}
#endif /* XENO_PC_PORT */

/* Transcribed from asm/field/nonmatchings/main/misc9/func_800AC0F0.s
 * (0x800AC0F0-0x800AC304). Copy 0x40 stream bytes, clear 0x120 image.
 * Remaining count is D_800AF780 (not D_800B06A0). 0x0D consumes 1 byte and
 * stops. sourceIndex==1: signed div-by-7 (mult 0x92492493) MoveImage from
 * ((raw%7)*9+0x380, (raw/7)*0x10+0x100). Else func_800AC03C+LoadImage.
 * Fill remaining of 0x1C slots with the cleared image. */
extern s32 D_800AF780;

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/field/nonmatchings/main/misc9", func_800AC0F0);
#else
void* func_800AC0F0(void* pStream, s32 destinationX, s32 renderContext) {
    u8 image[0x120];
    u8 encoded[0x40];
    RECT rect;
    RECT move;
    s32 sourceIndex;
    s32 streamOffset;
    s32 strip;
    s32 destX;
    s32 i;

    for (i = 0; i < 0x40; i++) {
        encoded[i] = ((u8*)pStream)[i];
    }
    for (i = 0x11F; i >= 0; i--) {
        image[i] = 0;
    }
    rect.w = 9;
    rect.h = 0x10;
    rect.y = (s16)(renderContext << 4);

    streamOffset = 0;
    strip = 0;
    if (D_800AF780 > 0) {
        destX = destinationX;
        while (strip < 0x1C) {
            if (encoded[streamOffset] == 0x0D) {
                streamOffset += 1;
                break;
            }
            {
#ifdef XENO_PC_PORT
                intptr_t raw = func_800ABFDC(encoded + streamOffset, &sourceIndex);
#else
                s32 raw = func_800ABFDC(encoded + streamOffset, &sourceIndex);
#endif
                streamOffset += 2;
                if (sourceIndex == 1) {
                    s32 q;
                    s32 r;
#ifdef FIELD_AC0F0_MUTANT_DIV9
                    q = raw / 9;
                    r = raw - q * 9;
#else
                    q = raw / 7;
                    r = raw - q * 7;
#endif
                    move.w = 9;
                    move.h = 0x10;
                    move.y = (s16)(q * 0x10 + 0x100);
                    move.x = (s16)(r * 9 + 0x380);
                    MoveImage(&move, destX, renderContext << 4);
                } else {
#ifdef XENO_PC_PORT
                    /* Provider already returned a host ROM span, not guest RAM. */
                    u16* glyph = (u16*)raw;
#else
                    u16* glyph = (u16*)(uintptr_t)(u32)raw;
#endif
                    func_800AC03C(image, glyph, sourceIndex);
                    rect.x = (s16)destX;
                    LoadImage(&rect, image);
                }
            }
            DrawSync(0);
            strip += 1;
            destX += 9;
        }
    }

    for (i = 0x11F; i >= 0; i--) {
        image[i] = 0;
    }
    if (strip < 0x1C) {
        destX = destinationX + strip * 9;
        while (strip < 0x1C) {
            rect.x = (s16)destX;
            LoadImage(&rect, image);
            DrawSync(0);
            strip += 1;
            destX += 9;
        }
    }

    D_800AF780 -= streamOffset;
    return (u8*)pStream + streamOffset;
}
#endif /* XENO_PC_PORT */

extern s32 D_800AF780;
extern void* D_800AF76C;
extern void* D_800AF784;

void func_800AC308(void) {
    ArchiveSetIndex(4, 0);
    D_800AF780 = ArchiveDecodeSize(0xAB);
    D_800AF76C = (void*)HeapAlloc(ArchiveDecodeAlignedSize(0xAB), 1);
    ArchiveReadFileToBuffer(0xAB, D_800AF76C, 0, 0x80);
    ArchiveCdDataSync(0);
    D_800AF784 = (void*)HeapAlloc(ArchiveDecodeAlignedSize(0xAC), 1);
    ArchiveReadFileToBuffer(0xAC, D_800AF784, 0, 0x80);
    ArchiveCdDataSync(0);
}

/* Transcribed from asm/field/nonmatchings/main/misc9/func_800AC3AC.s
 * (0x800AC3AC-0x800AC998). POLY_GT4 at D_800AF788 and D_800AF7F0: RGB0/1 of
 * first and RGB2/3 of second are 0xFF; the other corners 0. Copy each 0x34
 * bytes to +0x34 (D_800AF824 for the second). HeapAlloc(0x1000,1) with no
 * NULL check. 16 groups of 8 SPRT at +0x60, then per-strip x0 and
 * SetDrawMode/GetTPage(1,0,0x300+strip*0x40,0). */
extern u8 D_800AF788[];
extern u8 D_800AF7F0[];
extern u8 D_800AF824[];
extern void* D_800AF770;

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/field/nonmatchings/main/misc9", func_800AC3AC);
#else
void func_800AC3AC(void) {
    POLY_GT4* first = (POLY_GT4*)D_800AF788;
    POLY_GT4* second = (POLY_GT4*)D_800AF7F0;
    s32 group;

    SetPolyGT4(first);
    SetPolyGT4(second);
    setRGB0(first, 0xFF, 0xFF, 0xFF);
    setRGB1(first, 0xFF, 0xFF, 0xFF);
    setRGB2(second, 0xFF, 0xFF, 0xFF);
    setRGB3(second, 0xFF, 0xFF, 0xFF);
    first->y2 = 0x18;
    first->y3 = 0x18;
    setRGB2(first, 0, 0, 0);
    setRGB3(first, 0, 0, 0);
    setRGB0(second, 0, 0, 0);
    setRGB1(second, 0, 0, 0);
    first->x0 = 0;
    first->y0 = 0;
    first->x1 = 0x280;
    first->y1 = 0;
    first->x2 = 0;
    first->x3 = 0x280;
    second->x0 = 0;
    second->y0 = 0xC8;
    second->y1 = 0xC8;
    second->y2 = 0xE0;
    second->y3 = 0xE0;
    second->x1 = 0x280;
    second->x2 = 0;
    second->x3 = 0x280;
    first->u0 = 0;
    first->v0 = 0;
    first->u1 = 2;
    first->v1 = 0;
    first->u2 = 0;
    first->v2 = 2;
    first->u3 = 2;
    first->v3 = 2;
    second->u0 = 0;
    second->v0 = 0;
    second->u1 = 2;
    second->v1 = 0;
    second->u2 = 0;
    second->v2 = 2;
    second->u3 = 2;
    second->v3 = 2;
    first->tpage = GetTPage(1, 2, 0x3C0, 0x100);
    second->tpage = GetTPage(1, 2, 0x3C0, 0x100);
    first->clut = GetClut(0, 0x1FF);
    second->clut = GetClut(0, 0x1FF);
    SetSemiTrans(first, 1);
    SetSemiTrans(second, 1);
    *(POLY_GT4*)(D_800AF788 + 0x34) = *first;
    *(POLY_GT4*)D_800AF824 = *second;

    D_800AF770 = HeapAlloc(0x1000, 1);

    for (group = 0; group < 0x10; group++) {
        u8* packet = (u8*)D_800AF770 + group * 0x100;
        SPRT* sprite = (SPRT*)(packet + 0x60);
        s32 strip;
        s16 x;
        s32 tpageX;

        SetSprt(sprite);
        setRGB0(sprite, 0x80, 0x80, 0x80);
        SetSemiTrans(sprite, 0);
        sprite->clut = GetClut(0, 0x1FF);
        sprite->h = 0x10;
        sprite->u0 = 0;
        sprite->v0 = (u8)(group * 0x10);
        sprite->w = 0x80;
        sprite->x0 = 0x40;
        sprite->y0 = (s16)(group * 0x10);

        /* Retail unrolls 0x14-byte copies to +0xB0, +0x74, +0xC4, +0x88,
         * +0xD8, +0x9C, +0xEC (slots 4,1,5,2,6,3,7). */
        *(SPRT*)(packet + 0xB0) = *sprite;
        *(SPRT*)(packet + 0x74) = *sprite;
        *(SPRT*)(packet + 0xC4) = *sprite;
        *(SPRT*)(packet + 0x88) = *sprite;
        *(SPRT*)(packet + 0xD8) = *sprite;
        *(SPRT*)(packet + 0x9C) = *sprite;
        *(SPRT*)(packet + 0xEC) = *sprite;

        x = 0x40;
        tpageX = 0x300;
        for (strip = 0; strip < 4; strip++) {
            SPRT* left = (SPRT*)(packet + 0x60 + strip * 0x14);
            SPRT* right = (SPRT*)(packet + 0xB0 + strip * 0x14);
            u16 page;

            left->x0 = x;
#ifndef FIELD_AC3AC_MUTANT_SKIP_RIGHT_X0
            right->x0 = x;
#endif
            page = GetTPage(1, 0, tpageX, 0);
            SetDrawMode((DR_MODE*)(packet + strip * 0x0C), 0, 0, page, NULL);
            page = GetTPage(1, 0, tpageX, 0);
            SetDrawMode((DR_MODE*)(packet + 0x30 + strip * 0x0C), 0, 0,
                        page, NULL);
            x += 0x80;
            tpageX += 0x40;
        }
    }
}
#endif /* XENO_PC_PORT */

/* Transcribed from asm/field/nonmatchings/main/misc9/func_800AC99C.s
 * (0x800AC99C-0x800ACB8C). OT-insert D_800AF7F0+idx*0x34 then
 * D_800AF788+idx*0x34 (pSecond.tag sees the post-insert OT). Per 0x100
 * group: decrement y0 from the other context's sprite at +0x6A, write the
 * four y0s at group+idx*0x50+{0x6A,0x7E,0x92,0xA6}, then chain SPRT then
 * DR_MODE. */
extern void* D_800AF770;
extern u8 D_800AF7F0[];

#ifdef XENO_PC_PORT
#define AC99C_PACK_ADDR(p) (PsxMemory_GuestAddr(p) & 0x00FFFFFF)
#else
#define AC99C_PACK_ADDR(p) ((u32)(p) & 0x00FFFFFF)
#endif

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/field/nonmatchings/main/misc9", func_800AC99C);
#else
void func_800AC99C(void) {
    s32 idx = g_FieldCurRenderContextIndex;
    u32* otHead = (u32*)((u8*)g_FieldCurRenderContext + 0x80D4);
    u8* packet = (u8*)D_800AF770;
    u8* setup = D_800AF7F0;
    s32 row;

    {
        u8* pFirst = setup + idx * 0x34;
        u8* pSecond = setup - 0x68 + idx * 0x34;
        u32 ot = *otHead;

        *(u32*)pFirst = (*(u32*)pFirst & 0xFF000000) | (ot & 0x00FFFFFF);
        *otHead = (*otHead & 0xFF000000) | AC99C_PACK_ADDR(pFirst);
        ot = *otHead;
#ifdef FIELD_AC99C_MUTANT_STALE_SETUP_OT
        ot = *(u32*)pFirst;
#endif
        *(u32*)pSecond = (*(u32*)pSecond & 0xFF000000) | (ot & 0x00FFFFFF);
        *otHead = (*otHead & 0xFF000000) | AC99C_PACK_ADDR(pSecond);
    }

    for (row = 0; row < 0x1000; row += 0x100) {
        u8* group = packet + row;
        u8* prim = group + idx * 0x30;
        u8* strip = group + idx * 0x50 + 0x60;
        u8* phaseBase = group + idx * 0x50;
        u8* phaseSource = group + (((idx + 1) & 1) * 0x50);
        u16 phase = (u16)((*(s16*)(phaseSource + 0x6A) - 1) & 0xFF);
        s32 column;

        *(u16*)(phaseBase + 0x6A) = phase;
        *(u16*)(phaseBase + 0x7E) = phase;
        *(u16*)(phaseBase + 0x92) = phase;
        *(u16*)(phaseBase + 0xA6) = phase;

        for (column = 0; column < 4; column++) {
            u32 ot = *otHead;

            *(u32*)strip = (*(u32*)strip & 0xFF000000) | (ot & 0x00FFFFFF);
            *otHead = (*otHead & 0xFF000000) | AC99C_PACK_ADDR(strip);
            ot = *otHead;
            *(u32*)prim = (*(u32*)prim & 0xFF000000) | (ot & 0x00FFFFFF);
            *otHead = (*otHead & 0xFF000000) | AC99C_PACK_ADDR(prim);
            strip += 0x14;
            prim += 0x0C;
        }
    }
}
#endif /* XENO_PC_PORT */

#undef AC99C_PACK_ADDR

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/field/nonmatchings/main/misc9", func_800ACB90);
#else
void func_800ACB90(void) {
    RECT rect;
    s32 i;
    u8* pBuf;

#ifdef FIELD_ACB90_MUTANT_OLD_Y
    FieldLoadTIMWithClut(D_800AF784, 0x380, 0, 0, 0x1FF, 0, 0);
#else
    FieldLoadTIMWithClut(D_800AF784, 0x380, 0x100, 0, 0x1FF, 0, 0);
#endif
    DrawSync(0);
    HeapFree(D_800AF784);

    pBuf = HeapAlloc(0x200, 1);
    for (i = 0x7F; i >= 0; i--) {
#ifdef FIELD_ACB90_MUTANT_BAD_CLEAR_RANGE
        *(s32*)(pBuf + i * 4 + 0x1FC) = -1;
#else
        *(s32*)(pBuf + i * 4) = -1;
#endif
    }

    rect.x = 0x3C0;
    rect.y = 0x100;
    rect.w = 0x40;
    rect.h = 0x4;
    LoadImage(&rect, pBuf);
    DrawSync(0);
    HeapFree(pBuf);
}
#endif /* XENO_PC_PORT */

extern s32 D_8004F300;
extern void* D_800AF76C;
extern void* D_800AF774;
extern s32 D_800AF778;
extern s32 D_800AF77C;

void func_800ACC58(void) {
    u32 stackSlot;
    if (D_8004F300 != 0) {
        /* Retail reserves one extra word in this caller frame. Preserve that
         * otherwise-unused slot without emitting instructions. */
        __asm__ volatile("" : "=m"(stackSlot));
        func_800AC308();
        func_800AC3AC();
        D_800AF77C = 0;
        D_800AF778 = 0xF;
        D_800AF774 = D_800AF76C;
    }
}

extern s32 D_8004F300;
extern void* D_800AF76C;
extern void* D_800AF770;

void func_800ACCB0(void) {
    if (D_8004F300 != 0) {
        HeapFree(D_800AF76C);
        HeapFree(D_800AF770);
    }
}

void func_800ACCF4(void) {
    if (D_8004F300 == 0) return;
    if ((D_800AF77C & 0xF) == 0) {
        D_800AF774 = (void*)func_800AC0F0(D_800AF774, 0x300, D_800AF778 & 0xF);
        D_800AF778++;
    }
    D_800AF77C++;
}

short FieldScriptVMGetInstructionArgumentS16(int offset) {
    u_char* pData;

    pData = g_FieldScriptVMCurScriptData + (g_FieldScriptVMCurActor->scriptInstructionPointer + offset);
    return (pData[0] + (pData[1] << 8));
}

int FieldScriptVMGetInstructionArgument(int argumentIndex) {
    u_char* pData;

    pData = g_FieldScriptVMCurScriptData + (g_FieldScriptVMCurActor->scriptInstructionPointer + argumentIndex);
    return *pData | *(pData + 1) <<  8;
}

int FieldScriptVMGetArgument(int index) {
    int nArgument;

    nArgument = FieldScriptVMGetInstructionArgument(index);
    if (!(nArgument & 0x8000)) 
        return FieldScriptVMGetVariableValue(nArgument & 0xFFFF);
    return nArgument & 0x7FFF;
}
