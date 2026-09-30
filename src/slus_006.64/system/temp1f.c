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

/* Retail TU 0x80025C04..0x80027D64 (GTE colour, atlas and line-scroll
 * helpers), one of the TUs that were merged into system/temp1.c. This is the
 * only copy of these bodies; the port builds it too. */

extern void* g_GfxCurWorkBuffer;
extern void* g_GfxCurWorkBufferEnd;
extern s32 g_GfxCurContext;
extern u_long* g_GfxCurOT;

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp1f", func_80025C04);
#endif

/* Handwritten GTE color-processing routine retained as assembly. */
#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp1f", func_80025D4C);
#endif

#ifdef XENO_PC_PORT
#include "psx_memory.h"
/* Retail 80025FA8..80026338: atlas entry to double-buffered FT4 packets.
 * The two fixed data addresses refer to the loaded retail EXE image, not
 * replacement artwork. Keep hardware calls and their ordering intact. */
s32 func_80025FA8(u8* table, s32 index, u8* packets, s32 buffer,
                  s32 screenX, s32 screenY, s32 scaleX, s32 scaleY, s32 angle)
{
    extern MATRIX* ScaleMatrixL(MATRIX*, VECTOR*);
    extern void ReadGeomOffset(long*, long*);
    extern s32 ReadGeomScreen(void);
    MATRIX matrix;
    VECTOR scale;
    SVECTOR* corners = PSX_ADDR(0x8004FDC0u);
    u8* entry;
    long oldX = 0, oldY = 0, screen;
    s32 i;

    _Static_assert(sizeof(MATRIX) == 32, "retail atlas matrix size");
    _Static_assert(sizeof(POLY_FT4) == 40, "retail atlas packet size");
    __builtin_memcpy(&matrix, PSX_ADDR(0x800188CCu), 32);
    scale.vx = (s16)scaleX; scale.vy = (s16)scaleY; scale.vz = 0x1000;
    PushMatrix();
    ScaleMatrixL(&matrix, &scale);
    RotMatrixZ((s16)angle, &matrix);
    ReadGeomOffset(&oldX, &oldY);
    screen = ReadGeomScreen();
    SetGeomOffset((s16)screenX, (s16)screenY);
    SetGeomScreen(0x1000);
    SetRotMatrix(&matrix);
    SetTransMatrix(&matrix);
    entry = table + *(u16*)(table + index * 2 + 4);
    for (i = 0; i != *(s16*)entry; i++) {
        u8* item = entry + 4 + i * 28;
        POLY_FT4* poly = (POLY_FT4*)(packets + i * 80 + buffer * 40);
        s32 u, v, width, height, x, y, rotation;
        long interpolation = 0, flag = 0;
        SetPolyFT4(poly);
        SetSemiTrans(poly, 0);
        SetShadeTex(poly, 1);
        poly->tpage = GetTPage(*(s16*)(item+16), 0, *(s16*)(item+22), *(s16*)(item+24));
        poly->clut = GetClut(*(s16*)(item+18), *(s16*)(item+20));
        width = *(u16*)(item+4); height = *(u16*)(item+6);
        x = *(u16*)(item+8); y = *(u16*)(item+10);
        corners[0].vx = corners[3].vx = item[26] ? x + width : x;
        corners[1].vx = corners[2].vx = item[26] ? x : x + width;
        corners[0].vy = corners[1].vy = item[27] ? y + height : y;
        corners[2].vy = corners[3].vy = item[27] ? y : y + height;
        /* Projection order is perimeter order; FT4 storage is grid order.
         * XY outputs are packed words; p/FLAG are actual host long objects. */
        RotTransPers4(corners, corners+1, corners+2, corners+3,
            (long*)&poly->x0, (long*)&poly->x1, (long*)&poly->x3, (long*)&poly->x2,
            &interpolation, &flag);
        u = *(u16*)item; v = *(u16*)(item+2);
        width = *(u16*)(item+4); height = *(u16*)(item+6);
        rotation = (u16)angle & 0xfff;
        if (rotation == 0xc00) u--;
        if (rotation == 0) {
            if (poly->x3 < poly->x0) {
                u--;
                if ((s16)u < 0) { u = 0; width--; }
            }
            if (poly->y3 < poly->y0) {
                v--;
                if ((s16)v < 0) { v = 0; height--; }
            }
        }
        poly->u0 = u; poly->v0 = v;
        poly->u1 = u + width; poly->v1 = v;
        poly->u2 = u; poly->v2 = v + height;
        poly->u3 = u + width; poly->v3 = v + height;
    }
    SetGeomOffset(oldX, oldY);
    SetGeomScreen(screen);
    PopMatrix();
    return *(s16*)entry;
}
#else
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp1f", func_80025FA8);
#endif

#ifdef XENO_PC_PORT
/* Unpack a texture-atlas entry (indexed by `index` into `table`) into the
 * caller's tpage / CLUT / texcoord output fields. `table[index*2+4]` is the
 * byte offset of the entry within `table`; entry[0]=item count, entry+4 is the
 * first item's descriptor. Stubbed, the window-border sprites get zeroed
 * tpage/clut. All outputs are s32/u32 fields, matching the asm sw width. */
void func_80026338(u8* table, s32 index, u32* pOut0, s32* pTPage, s32* pClutX,
                   s32* pClutY, s32* pTexX, s32* pTexY) {
    u8* pEntry = table + *(u16*)(table + index * 2 + 4);
    u8* item = pEntry + 4;
    u16 packed = *(u16*)(pEntry + 4);
    s16 tpage = *(s16*)(item + 0x10);
    s32 shift;

    *pOut0 = (u32)(s32)*(s16*)(pEntry);
    if (tpage == 0) {
        shift = ((s32)((u32)packed << 16)) >> 20;
    } else {
        shift = ((s32)((u32)packed << 16)) >> 18;
    }
#ifdef TEMP1_26338_MUTANT_SHIFT3
    shift++;
#endif
    *pTPage = *(s16*)(item + 0x10);
    *pClutX = *(s16*)(item + 0x12);
    *pClutY = *(s16*)(item + 0x14);
    *pTexX = (s32)(s16)(*(u16*)(item + 0x16) & 0xFFC0) + shift;
    *pTexY = (s32)(s16)(*(u16*)(item + 0x18) & 0xFF00) + *(s16*)(item + 2);
}
#else
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp1f", func_80026338);
#endif

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp1f", func_800263E4);
#else
/* Retail 800263E4..8002675C. The last two arguments are independent
 * byte-sized axis flips; packet coordinates narrow before the UV test. */
s32 func_800263E4(u8* table, s32 index, void* packets, s32 buffer,
                  s32 x, s32 y, s32 scale, s32 flipX, s32 flipY) {
    u8* entry = table + *(u16*)(table + index * 2 + 4);
    u8* output = packets;
    s32 i = 0;
    s32 itemOffset = 4;
    s32 factor = (u16)scale;
    if (*(s16*)entry != 0) {
        do {
            u8* item = entry + itemOffset;
            POLY_FT4* poly = (POLY_FT4*)(output + buffer * 40);
            s32 dx, dy, width, height, product;
            s32 u, v, uw, vh;
            s32 left, right, top, bottom;
            product = *(s16*)(item + 8) * factor;
            if (product < 0) product += 0xFFF;
            dx = product >> 12;
            product = *(s16*)(item + 10) * factor;
            if (product < 0) product += 0xFFF;
            dy = product >> 12;
            product = *(s16*)(item + 4) * factor;
            if (product < 0) product += 0xFFF;
            width = product >> 12;
            product = *(s16*)(item + 6) * factor;
            if (product < 0) product += 0xFFF;
            height = product >> 12;
            SetPolyFT4(poly);
            SetSemiTrans(poly, 0);
            SetShadeTex(poly, 1);
            poly->tpage = GetTPage(*(s16*)(item + 16), 0,
                                   *(s16*)(item + 22), *(s16*)(item + 24));
            poly->clut = GetClut(*(s16*)(item + 18), *(s16*)(item + 20));
            if ((u8)flipX) { dx = -dx; width = -width; }
            if ((u8)flipY) { dy = -dy; height = -height; }
            u = *(u16*)item; v = *(u16*)(item + 2);
            uw = *(u16*)(item + 4); vh = *(u16*)(item + 6);
            left = (u16)x + dx; right = left + width;
            if (item[26]) {
                poly->x0 = poly->x2 = right;
                poly->x1 = poly->x3 = left;
            } else {
                poly->x0 = poly->x2 = left;
                poly->x1 = poly->x3 = right;
            }
            top = (u16)y + dy; bottom = top + height;
            if (item[27]) {
                poly->y0 = poly->y1 = bottom;
                poly->y2 = poly->y3 = top;
            } else {
                poly->y0 = poly->y1 = top;
                poly->y2 = poly->y3 = bottom;
            }
            if (poly->x3 < poly->x0) {
                --u;
                if ((s16)u < 0) { u = 0; --uw; }
            }
            if (poly->y3 < poly->y0) {
                --v;
                if ((s16)v < 0) { v = 0; --vh; }
            }
            poly->u0 = poly->u2 = u; poly->u1 = poly->u3 = u + uw;
            poly->v0 = poly->v1 = v; poly->v2 = poly->v3 = v + vh;
            itemOffset += 28;
            output += 80;
            ++i;
        } while (i != *(s16*)entry);
    }
    return *(s16*)entry;
}
#endif

#ifdef XENO_PC_PORT
/* Build a run of POLY_FT4 sprites for atlas entry `index` (window borders,
 * scroll-bar ornaments, etc.). pEntry[0] = item count; each 0x1C-byte item
 * holds signed position/size (scaled by `scale`>>12) + raw UV/wh + per-axis
 * flip flags at +0x1A/+0x1B. Writes one 0x28-byte POLY_FT4 per item into
 * `polys` at the current renderCtx slot (+0x50 stride, two contexts). Returns
 * the item count. Stubbed, the border sprites never build (empty frame). */
s32 func_8002675C(u8* table, s32 index, void* polys, s32 renderCtx, s32 x, s32 y,
                  s32 scale) {
    u8* pEntry = table + *(u16*)(table + index * 2 + 4);
    u32 s4 = (u32)scale & 0xFFFF;
    s32 fp = 4;
    u8* poly = (u8*)polys;
    s32 slot;

    if (*(s16*)(pEntry) == 0) {
        return 0;
    }
    slot = 0;
    do {
        u8* item = pEntry + fp;
        u8* s0 = poly + renderCtx * 0x28;
        s32 t, s3, s6, s2, s5;
        s32 u0, v0r, uw, vh, u1, v1;
        s32 px0, px1, py0, py1;

        t = *(s16*)(item + 0x8) * (s32)s4; if (t < 0) t += 0xFFF; s3 = t >> 12;
        t = *(s16*)(item + 0xA) * (s32)s4; if (t < 0) t += 0xFFF; s6 = (s32)((u32)t >> 12);
        t = *(s16*)(item + 0x4) * (s32)s4; if (t < 0) t += 0xFFF; s2 = t >> 12;
        t = *(s16*)(item + 0x6) * (s32)s4; if (t < 0) t += 0xFFF; s5 = (s32)((u32)t >> 12);

        SetPolyFT4((POLY_FT4*)s0);
        SetSemiTrans((POLY_FT4*)s0, 0);
        SetShadeTex((POLY_FT4*)s0, 1);
        *(s16*)(s0 + 0x16) = GetTPage(*(s16*)(item + 0x10), 0,
                                      *(s16*)(item + 0x16), *(s16*)(item + 0x18));
        *(s16*)(s0 + 0xE) = GetClut(*(s16*)(item + 0x12), *(s16*)(item + 0x14));

        u0 = *(u16*)(item + 0x0);
        v0r = *(u16*)(item + 0x2);
        uw = *(u16*)(item + 0x4);
        vh = *(u16*)(item + 0x6);

        if (*(u8*)(item + 0x1A) == 0) {          /* no horizontal flip */
            px0 = x + s3;
            px1 = s2 + px0;
        } else {                                  /* horizontal flip: swap L/R x */
            u0 -= 1;
            px1 = x + s3;
            px0 = s2 + px1;
            if ((s16)u0 < 0) { u0 = 0; uw -= 1; }
        }
        *(s16*)(s0 + 0x8) = px0; *(s16*)(s0 + 0x10) = px1;
        *(s16*)(s0 + 0x18) = px0; *(s16*)(s0 + 0x20) = px1;

        if (*(u8*)(item + 0x1B) == 0) {          /* no vertical flip */
            py0 = y + s6;
            py1 = s5 + py0;
            *(s16*)(s0 + 0xA) = py0; *(s16*)(s0 + 0x12) = py0;
            *(s16*)(s0 + 0x1A) = py1; *(s16*)(s0 + 0x22) = py1;
        } else {                                  /* vertical flip: swap T/B y */
            v0r -= 1;
            py0 = y + s6;
            py1 = s5 + py0;
            *(s16*)(s0 + 0xA) = py1; *(s16*)(s0 + 0x12) = py1;
            *(s16*)(s0 + 0x1A) = py0; *(s16*)(s0 + 0x22) = py0;
            if ((s16)v0r < 0) { v0r = 0; vh -= 1; }
        }

        u1 = u0 + uw;
        v1 = v0r + vh;
        *(u8*)(s0 + 0xC) = u0;  *(u8*)(s0 + 0xD) = v0r;
        *(u8*)(s0 + 0x14) = u1; *(u8*)(s0 + 0x15) = v0r;
        *(u8*)(s0 + 0x1C) = u0; *(u8*)(s0 + 0x1D) = v1;
        *(u8*)(s0 + 0x24) = u1; *(u8*)(s0 + 0x25) = v1;

        fp += 0x1C;
        poly += 0x50;
        slot++;
    } while (slot != *(s16*)(pEntry));
    return *(s16*)(pEntry);
}
#else
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp1f", func_8002675C);
#endif

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp1f", func_80026A0C);
#else
s32 func_80026A0C(u8* pTable, s32 index, u8* pPrimBuffer, s32 primStride, s16 ofsX, s16 ofsY) {
    u8* pDesc = pTable + *(u16*)(pTable + index * 2 + 4);
    s32 count = *(s16*)(pDesc);
    s32 i;
    u8* pEntry = pDesc + 4;
    s32 stride = primStride * 20; /* primStride * 0x28 per SPRT */
    u8* pPrim = pPrimBuffer + stride;

    for (i = 0; i < count; i++) {
        s32 clut = GetClut(*(s16*)(pEntry + 0x12), *(s16*)(pEntry + 0x14));
        SetSprt((void*)pPrim);
        SetSemiTrans((void*)pPrim, 0);
        SetShadeTex((void*)pPrim, 1);
        *(u16*)(pPrim + 0x0E) = (u16)clut;
        *(u16*)(pPrim + 0x08) = *(u16*)(pEntry + 0x08) + ofsX;
        *(u16*)(pPrim + 0x0A) = *(u16*)(pEntry + 0x0A) + ofsY;
        *(u8*)(pPrim + 0x0C) = *(u8*)(pEntry + 0x00);
        *(u8*)(pPrim + 0x0D) = *(u8*)(pEntry + 0x02);
        *(u16*)(pPrim + 0x10) = *(u16*)(pEntry + 0x04);
        *(u16*)(pPrim + 0x12) = *(u16*)(pEntry + 0x06);
        pEntry += 0x1C;
        pPrim += 0x28;
    }

    /* Add DR_MODE */
    {
        s32 tpage = GetTPage(0, 0, *(s16*)(pDesc + 4 + 0x10), *(s16*)(pDesc + 4 + 0x16));
        u8* pMode = pPrimBuffer + count * 20 + stride;
        SetDrawMode((void*)pMode, 0, 0, tpage & 0xFFFF, NULL);
    }
    return count + 1;
}
#endif

void func_80026B9C(void) {
}

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp1f", func_80026BA4);
#else
/* Retail 80026BA4..80026DCC takes five arguments. Packets come from the
 * current graphics work buffer; the fifth argument is the ordering-table
 * entry itself. A sixth stack word belongs to the caller, not this API. */
void func_80026BA4(u8* pTable, s32 index, s32 ofsX, s32 ofsY, void* ot) {
    u8* pDesc = pTable + *(u16*)(pTable + index * 2 + 4);
    s32 count = *(s16*)pDesc;
    s32 i;

    /* Retail uses an unsigned strict comparison, rejecting an exact fit. */
    if ((u32)((uintptr_t)g_GfxCurWorkBuffer + (u32)count * 40u) >=
        (u32)(uintptr_t)g_GfxCurWorkBufferEnd || count == 0) {
        return;
    }
    for (i = 0; i != count; i++) {
        u8* item = pDesc + 4 + i * 28;
        u8* poly = g_GfxCurWorkBuffer;
        s32 u = *(s16*)item;
        s32 v = *(s16*)(item + 2);
        s32 width = *(s16*)(item + 4);
        s32 height = *(s16*)(item + 6);
        u32 x = (u32)*(s16*)(item + 8) + (u32)ofsX;
        u32 y = (u32)*(s16*)(item + 10) + (u32)ofsY;
        s32 mode = *(s16*)(item + 16);
        s32 shift = mode == 0 ? u >> 4 : u >> 2;
        s32 texX = (s16)(*(u16*)(item + 22) & 0xFFC0) + shift;
        s32 texY = (s16)(*(u16*)(item + 24) & 0xFF00) + v;

        g_GfxCurWorkBuffer = poly + 40;
        poly[3] = 9;
        poly[7] = 0x2D;
        *(u16*)(poly + 14) = GetClut(*(s16*)(item + 18), *(s16*)(item + 20));
        *(u16*)(poly + 22) = GetTPage(mode, 0, texX, texY);
        *(u16*)(poly + 8) = *(u16*)(poly + 24) = x;
        *(u16*)(poly + 16) = *(u16*)(poly + 32) = x + (u32)width;
        *(u16*)(poly + 10) = *(u16*)(poly + 18) = y;
        *(u16*)(poly + 26) = *(u16*)(poly + 34) = y + (u32)height;
        poly[12] = poly[28] = u;
        poly[20] = poly[36] = u + width;
        poly[13] = poly[21] = v;
        poly[29] = poly[37] = v + height;
        AddPrim(ot, poly);
    }
}
#endif

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp1f", func_80026DCC);
#else
s32 func_80026DCC(u8* pTable, s32 index, u8* pPrimBuffer, s16 ofsX, s16 ofsY) {
    u8* pDesc = pTable + *(u16*)(pTable + index * 2 + 4);
    s32 count = *(s16*)(pDesc);
    s32 i;
    u8* pEntry = pDesc + 4;
    u8* pPrim = pPrimBuffer;

    if (count == 0) return 0;

    for (i = 0; i < count; i++) {
        u16 packed = *(u16*)(pEntry);
        s16 tpageFlag = *(s16*)(pEntry + 0x6);
        s32 shift;
        s16 texX, texY;
        s32 clut, tpage;

        if (tpageFlag == 0) {
            shift = ((s32)((u32)packed << 16)) >> 20;
        } else {
            shift = ((s32)((u32)packed << 16)) >> 18;
        }

        texX = (s16)((s32)((u16)*(u16*)(pEntry + 0xC) & 0xFFC0) << 16 >> 16) + shift;
        texY = (s16)((s32)((u16)*(u16*)(pEntry + 0xE) & 0xFF00) << 16 >> 16) + *(s16*)(pEntry - 0x8);

        clut = GetClut(*(s16*)(pEntry + 0x8), *(s16*)(pEntry + 0xA));
        tpage = GetTPage(tpageFlag, 1, texX, texY);

        *(u16*)(pPrim + 0x08) = (u16)tpage;
        *(u16*)(pPrim + 0x0A) = (u16)tpage;
        *(u8*)(pPrim + 0x02) = *(u8*)(pEntry);
        *(u8*)(pPrim + 0x03) = *(u8*)(pEntry - 0x8);
        *(u8*)(pPrim + 0x04) = *(u8*)(pEntry - 0x6);
        *(u8*)(pPrim + 0x05) = *(u8*)(pEntry - 0x4);
        *(u16*)(pPrim + 0x00) = *(u16*)(pEntry - 0x2) + ofsX;
        *(u16*)(pPrim + 0x00) |= (*(u16*)(pEntry) + ofsY) << 16;

        pEntry += 0x1C;
        pPrim += 0x18;
    }
    return count;
}
#endif

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp1f", func_80026F44);
#endif

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp1f", func_80026FE8);
#endif

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp1f", func_8002709C);
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp1f", func_800273C4);
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp1f", func_800278F8);
#else
/* Horizon / line-scroll backdrop (title map 490 path). Layout of the 0x34C
 * heap block matches func_8002709C.s / func_800273C4.s / func_800278F8.s. */

static s32 HorizonMulShift12(s32 a, s32 b) {
    s32 t = a * b;
    if (t < 0) {
        t += 0xFFF;
    }
    return t >> 12;
}

static s32 HorizonMulShift8(s32 a, s32 b) {
    s32 t = a * b;
    if (t < 0) {
        t += 0xFF;
    }
    return t >> 8;
}

void func_800278F8(u8* ctx, s32 scroll, s32 screenY, s32 fade, void* ot,
                   s32 renderCtx);

void* func_8002709C(s32 a0, s32 a1, s32 a2, s32 a3, s32 clutX, s32 clutY,
                    s32 abr, s32 scrollSignArg, s16* pCoords, u8* pColors,
                    s32 skyScale, s32 fadeDiv, s32 fadeSub) {
    DRAWENV drawEnv;
    u8* ctx;
    s32 i;
    s32 t;
    u16 clut;

    HeapChangeCurrentUser(HEAP_USER_MASA, NULL);
    ctx = (u8*)HeapAlloc(0x34C, 0);
    if (ctx == NULL) {
        return NULL;
    }

#ifdef XENO_PC_PORT
    /* DIAGNOSTIC: one-shot horizon init probe. Remove once title backdrop
     * dest_nz/src704_nz are non-zero. */
    {
        static int s_hzInitDiag;
        if (!s_hzInitDiag) {
            s_hzInitDiag = 1;
            printf("[xeno-port][horizon] DIAG 2709C a0=%d a1=%d w=%d h=%d "
                   "clut=(%d,%d) mode=%d sign=%d coords=(%d,%d,%d) "
                   "rgb0=%02x%02x%02x sky=%d fade=%d/%d ctx=%p\n",
                   a0, a1, a2, a3, clutX, clutY, abr, scrollSignArg,
                   pCoords ? (int)pCoords[0] : -1,
                   pCoords ? (int)pCoords[2] : -1,
                   pCoords ? (int)pCoords[4] : -1,
                   pColors ? pColors[0] : 0, pColors ? pColors[1] : 0,
                   pColors ? pColors[2] : 0, skyScale, fadeDiv, fadeSub,
                   (void*)ctx);
            fflush(stdout);
        }
    }
#endif

    GetDrawEnv(&drawEnv); /* asm calls it; result unused */

    *(s32*)(ctx + 0x328) = a2;
    *(s32*)(ctx + 0x32C) = a3;
    *(s16*)(ctx + 0x33C) = (s16)pCoords[0];
    *(s16*)(ctx + 0x33E) = (s16)pCoords[2]; /* +0x4 as halfwords */
    *(s16*)(ctx + 0x346) = (s16)skyScale;
    *(s16*)(ctx + 0x348) = (s16)fadeDiv;
    *(s16*)(ctx + 0x34A) = (s16)fadeSub;
    *(s16*)(ctx + 0x340) = (s16)pCoords[4]; /* +0x8 */

    if (*(s32*)((u8*)pCoords + 8) < 0) {
        *(s32*)(ctx + 0x330) = -scrollSignArg;
    } else {
        *(s32*)(ctx + 0x330) = scrollSignArg;
    }

    *(s16*)(ctx + 0x336) = (s16)a1;
    *(s16*)(ctx + 0x334) = (s16)a0;
    *(s16*)(ctx + 0x338) = (s16)abr;

    t = a1;
    if (a1 < 0) {
        t = a1 + 0xFF;
    }
    *(s16*)(ctx + 0x33A) = (s16)(a1 - ((t >> 8) << 8));

    clut = GetClut(clutX, clutY);
    {
        u8* p = ctx;
        for (i = 0; i < 0x10; i++) {
            SetPolyFT4((POLY_FT4*)p);
            SetShadeTex((POLY_FT4*)p, 1);
            ((POLY_FT4*)p)->clut = clut;
            p += 0x28;
        }
    }

    if (pColors == NULL) {
        *(s16*)(ctx + 0x344) = 0;
        return ctx;
    }

    *(s16*)(ctx + 0x344) = 1;

    /* Two top POLY_F4 sky fills at 0x280 (shared RGB from pColors[0..2]). */
    {
        u8* p = ctx;
        s32 off = 0x280;
        for (i = 0; i < 2; i++) {
            POLY_F4* poly = (POLY_F4*)(ctx + off);
            SetPolyF4(poly);
            poly->r0 = pColors[0];
            poly->g0 = pColors[1];
            poly->b0 = pColors[2];
            poly->x0 = 0;
            poly->y0 = 0;
            poly->x1 = 0x140;
            poly->y1 = 0;
            poly->x2 = 0;
            poly->x3 = 0x140;
            off += 0x18;
            p += 0x18;
            (void)p;
        }
    }

    pColors += 4;

    /* Two POLY_G4 gradient bands at 0x2E0. */
    {
        s32 off = 0x2E0;
        for (i = 0; i < 2; i++) {
            POLY_G4* poly = (POLY_G4*)(ctx + off);
            u8* c0 = pColors;
            u8* c1 = pColors + 4;
            SetPolyG4(poly);
            poly->r0 = c0[0];
            poly->g0 = c0[1];
            poly->b0 = c0[2];
            poly->r1 = c0[0];
            poly->g1 = c0[1];
            poly->b1 = c0[2];
            poly->r2 = c1[0];
            poly->g2 = c1[1];
            poly->b2 = c1[2];
            poly->r3 = c1[0];
            poly->g3 = c1[1];
            poly->b3 = c1[2];
            poly->x0 = 0;
            poly->x1 = 0x140;
            poly->x2 = 0;
            poly->x3 = 0x140;
            off += 0x24;
        }
    }

    pColors += 4;

    /* Two bottom POLY_F4 fills at 0x2B0 (RGB from pColors[0..2]). */
    {
        s32 off = 0x2B0;
        for (i = 0; i < 2; i++) {
            POLY_F4* poly = (POLY_F4*)(ctx + off);
            SetPolyF4(poly);
            poly->r0 = pColors[0];
            poly->g0 = pColors[1];
            poly->b0 = pColors[2];
            poly->x0 = 0;
            poly->x1 = 0x140;
            poly->x2 = 0;
            poly->y2 = 0xF0;
            poly->x3 = 0x140;
            poly->y3 = 0xF0;
            off += 0x18;
        }
    }

    return ctx;
}

s32 func_800273C4(void* ctxPtr, SVECTOR* eye, SVECTOR* at, MATRIX* mtx,
                  void* ot, s32 renderCtx) {
    u8* ctx = (u8*)ctxPtr;
    VECTOR dir;
    SVECTOR dirN;
    SVECTOR pt;
    long sxy;
    long p, flag;
    s32 fade;
    s32 scroll;
    s32 screenY;
    s32 screenY2;
    s32 yTop;
    s32 t;
    s32 dx, dy, dz;

    if (ctx == NULL) {
        return 0;
    }

    dir.vx = at->vx - eye->vx;
    dir.vy = 0;
    dir.vz = at->vz - eye->vz;
    VectorNormalS(&dir, &dirN);

    t = HorizonMulShift12(dirN.vx, *(s16*)(ctx + 0x340));
    pt.vx = (s16)(at->vx + t);
    pt.vy = *(s16*)(ctx + 0x33E);
    t = HorizonMulShift12(dirN.vz, *(s16*)(ctx + 0x340));
    pt.vz = (s16)(at->vz + t);

    SetRotMatrix(mtx);
    SetTransMatrix(mtx);
    RotTransPers(&pt, &sxy, &p, &flag);
    screenY = (s16)(sxy >> 16);

    dx = dir.vx;
    dz = dir.vz;
    if (*(s16*)(ctx + 0x348) != 0) {
        dx = at->vx - eye->vx;
        dy = at->vy - eye->vy;
        dz = at->vz - eye->vz;
        {
            s32 dist = SquareRoot0(dx * dx + dy * dy + dz * dz);
            s32 denom = *(s16*)(ctx + 0x348);
            fade = (dist - *(s16*)(ctx + 0x34A)) / denom;
            if (fade < 0) {
                fade = 0;
            } else if (fade > 0x100) {
                fade = 0x100;
            }
        }
    } else {
        fade = 0;
    }

    {
        s32 ang = ratan2(dx, dz) & 0xFFF;
        s32 prod = *(s32*)(ctx + 0x328) * *(s32*)(ctx + 0x330);
        scroll = HorizonMulShift12(prod, ang);
    }

    func_800278F8(ctx, scroll, screenY, fade, ot, renderCtx);

#ifdef XENO_PC_PORT
    /* DIAGNOSTIC: one-shot horizon draw probe. Remove with 2709C DIAG. */
    {
        static int s_hzDrawDiag;
        if (!s_hzDrawDiag) {
            s_hzDrawDiag = 1;
            printf("[xeno-port][horizon] DIAG 273C4 screenY=%d scroll=%d fade=%d "
                   "hasColors=%d h=%d stripScale=%d ot=%p rcx=%d "
                   "ft4[0]=(%d,%d)-(%d,%d) tpage=0x%x\n",
                   screenY, scroll, fade, (int)*(s16*)(ctx + 0x344),
                   *(s32*)(ctx + 0x32C), (int)*(s16*)(ctx + 0x346), ot,
                   renderCtx,
                   (int)((POLY_FT4*)ctx)->x0, (int)((POLY_FT4*)ctx)->y0,
                   (int)((POLY_FT4*)ctx)->x3, (int)((POLY_FT4*)ctx)->y3,
                   (unsigned)((POLY_FT4*)ctx)->tpage);
            fflush(stdout);
        }
    }
#endif

    if (*(s16*)(ctx + 0x344) <= 0) {
        return screenY;
    }

    yTop = screenY - *(s32*)(ctx + 0x32C);
    if (yTop > 0xF0) {
        yTop = 0xF0;
    }
    if (yTop > 0) {
        POLY_F4* poly = (POLY_F4*)(ctx + 0x280 + renderCtx * 0x18);
        poly->y2 = (s16)yTop;
        poly->y3 = (s16)yTop;
        AddPrim(ot, poly);
    }

    t = HorizonMulShift12(dirN.vx, *(s16*)(ctx + 0x340));
    t = HorizonMulShift8(t, *(s16*)(ctx + 0x346));
    pt.vx = (s16)(at->vx + t);
    t = HorizonMulShift8(*(s16*)(ctx + 0x33E), *(s16*)(ctx + 0x346));
    pt.vy = (s16)t;
    t = HorizonMulShift12(dirN.vz, *(s16*)(ctx + 0x340));
    t = HorizonMulShift8(t, *(s16*)(ctx + 0x346));
    pt.vz = (s16)(at->vz + t);

    RotTransPers(&pt, &sxy, &p, &flag);
    screenY2 = (s16)(sxy >> 16);

    if ((screenY2 - screenY) >= 0xF1) {
        screenY2 = screenY + 0xF0;
    }

    if (screenY2 >= 0 && screenY < 0xF0) {
        POLY_G4* poly = (POLY_G4*)(ctx + 0x2E0 + renderCtx * 0x24);
        poly->y0 = (s16)screenY;
        poly->y1 = (s16)screenY;
        poly->y2 = (s16)screenY2;
        poly->y3 = (s16)screenY2;
        AddPrim(ot, poly);
    }

    {
        s32 yBot = (screenY2 < 0) ? 0 : screenY2;
        if (yBot < 0xF0) {
            POLY_F4* poly = (POLY_F4*)(ctx + 0x2B0 + renderCtx * 0x18);
            poly->y0 = (s16)yBot;
            poly->y1 = (s16)yBot;
            AddPrim(ot, poly);
        }
    }

    return screenY;
}

void func_800278F8(u8* ctx, s32 scroll, s32 screenY, s32 fade, void* ot,
                   s32 renderCtx) {
    s32 width = *(s32*)(ctx + 0x328);
    s32 height = *(s32*)(ctx + 0x32C);
    s32 fadePlus = fade + 0x100;
    s32 stripH;
    s32 xCursor;
    s32 uPos;
    s32 i;
    s32 half;
    s32 t;
    s32 vFixed;
    POLY_FT4* poly;

    t = (width << 8) / fadePlus;
    t = 0x140 - t;
    half = (t + (t >> 31)) >> 1;
    t = half + HorizonMulShift8(half, fade);
    t = (s16)(scroll - t);

    if (width != 0) {
        uPos = t % width;
        if ((uPos << 16) < 0) {
            uPos += (u16)width;
        }
    } else {
        uPos = 0;
    }

    if (screenY < 0 || screenY > height + 0xF0) {
        stripH = 0;
    } else {
        stripH = (height << 8) / fadePlus;
    }
    xCursor = 0;

    poly = (POLY_FT4*)(ctx + ((renderCtx & 1) * 0x140));

    if ((s16)stripH <= 0) {
        return;
    }

    {
        s32 tpX = *(s16*)(ctx + 0x334);
        s32 abr = *(s16*)(ctx + 0x338);
        if (tpX < 0) {
            tpX += 0x3F;
        }
        vFixed = tpX - ((tpX >> 6) << 6);
        vFixed <<= (2 - abr);
    }

    for (i = 0; i < 8; i++) {
        s32 abr = *(s16*)(ctx + 0x338);
        s32 tpXbase = *(u16*)(ctx + 0x334);
        s32 shift = 2 - abr;
        s32 tpageX = tpXbase + ((s16)uPos >> shift);
        s32 mask = (0x100 >> abr) - 1;
        s32 u0 = (uPos + vFixed) & mask;
        s32 uSpan = 0x100 - u0;
        s32 spanPx;
        s32 x1;
        s32 tpY = *(s16*)(ctx + 0x336);
        s32 gx;

        if ((s16)(uPos + (s16)uSpan) > width) {
            uSpan = (s16)(width - uPos);
        }

        /* asm: sll uSpan,16; sra 8 → (s16)uSpan << 8, then / fadePlus */
        spanPx = (((s32)(s16)uSpan << 8) / fadePlus);

        if ((s16)(xCursor + spanPx) > 0x140) {
            spanPx = 0x140 - xCursor;
            {
                s32 tmp = (s16)spanPx * fadePlus;
                if (tmp < 0) {
                    tmp += 0xFF;
                }
                /* asm: srl (logical) after signed round — positive spans only */
                uSpan = (u32)tmp >> 8;
            }
        }

        x1 = xCursor + spanPx;

        {
            s32 next = uPos + (s16)uSpan;
            if (width != 0) {
                uPos = next % width;
                /* asm uses mfhi of div next/width, then if negative add width —
                 * already handled for C % with positive width after adjust: */
                if (uPos < 0) {
                    uPos += width;
                }
            }
        }

        poly->x0 = (s16)xCursor;
        poly->y0 = (s16)(screenY - stripH);
        poly->x1 = (s16)x1;
        poly->y1 = (s16)(screenY - stripH);
        poly->x2 = (s16)xCursor;
        poly->y2 = (s16)screenY;
        poly->x3 = (s16)x1;
        poly->y3 = (s16)screenY;

        poly->u0 = (u8)u0;
        poly->v0 = *(u8*)(ctx + 0x33A);
        poly->u1 = (u8)(u0 + (s16)uSpan - 1);
        poly->v1 = *(u8*)(ctx + 0x33A);
        poly->u2 = (u8)u0;
        poly->v2 = (u8)(*(u8*)(ctx + 0x33A) + *(u8*)(ctx + 0x32C));
        poly->u3 = (u8)(u0 + (s16)uSpan - 1);
        poly->v3 = (u8)(*(u8*)(ctx + 0x33A) + *(u8*)(ctx + 0x32C));

        gx = tpageX;
        if (gx < 0) {
            gx += 0x3F;
        }
        gx = (gx >> 6) << 6;
        if (tpY < 0) {
            tpY += 0xFF;
        }
        poly->tpage = GetTPage(abr, 0, gx, (tpY >> 8) << 8);

        AddPrim(ot, poly);

        xCursor = x1;
        if ((s16)xCursor >= 0x140) {
            break;
        }
        poly = (POLY_FT4*)((u8*)poly + 0x28);
    }
}
#endif

void func_80027D40(void* ptr) {
    if (ptr != NULL) {
        HeapFree(ptr);
    }
}
