#include "common.h"
#include "field/main.h"
#include "field/actor.h"
#include "system/memory.h"
#include "psyq/libgpu.h"
#include "psyq/libgte.h"
#include "field/effects.h"
#include "field/graphics.h"
#include "field/camera.h"
#include "system/archive.h"
#ifdef XENO_PC_PORT
#include <assert.h>
#include <stdio.h>
#include "../../../pc_port/src/psx_memory.h"
#else
/* <assert.h> is unavailable under the matching build's -nostdinc MIPS
 * preprocessor. The assert(0) below marks an unimplemented path in a function
 * not yet byte-matched, so a no-op assert compiles safely there. */
#define assert(x) ((void)0)
#endif

//
void func_800A55B8(void* arg0, s32 arg1, s32 arg2, s32 arg3) {
    *(s32*)((u8*)arg0 + 0x14) = arg1;
    *(s32*)((u8*)arg0 + 0x18) = arg2;
    *(s32*)((u8*)arg0 + 0x1C) = arg3;
}

extern void* D_800AFE80;
extern void* D_800B069C;

void func_800A55C8(void) {
    HeapFree(D_800AFE80);
    HeapFree(D_800B069C);
}

// Set PolyFT4 color for PolyFT4s of next swap chain
void func_800A5600(u_char color) {
    int i;
    POLY_FT4* pPoly;
    int nOffset;
    
    for (i = 0; i < 5; i++) {
        nOffset = i * 2;
        (g_FieldZoomFadeEffect.polygons + nOffset + ((g_FieldCurRenderContextIndex + 1) & 1))->r0 = color;
        (g_FieldZoomFadeEffect.polygons + nOffset + ((g_FieldCurRenderContextIndex + 1) & 1))->g0 = color;
        (g_FieldZoomFadeEffect.polygons + nOffset + ((g_FieldCurRenderContextIndex + 1) & 1))->b0 = color;
    }
}

// Field Fade Effect
// ---------------------------------------------------
void func_800A56A8(int duration) {
    g_FieldEffects.fades[0].b0 = 0xFF00;
    g_FieldEffects.fades[0].g0 = 0xFF00;
    g_FieldEffects.fades[0].r0 = 0xFF00;
    g_FieldEffects.fades[0].duration = duration + 1;
    g_FieldEffects.fades[0].isVisible = 1;
    g_FieldEffects.fades[0].semitransparency = 1;
    g_FieldEffects.fades[0].blueDelta = -0x10000 / duration;
    g_FieldEffects.fades[0].greenDelta = -0x10000 / duration;
    g_FieldEffects.fades[0].redDelta = -0x10000 / duration;
}

void func_800A5710(int duration) {
    g_FieldEffects.fades[0].b0 = 0x0;
    g_FieldEffects.fades[0].g0 = 0x0;
    g_FieldEffects.fades[0].r0 = 0x0;
    g_FieldEffects.fades[0].duration = duration + 1;
    g_FieldEffects.fades[0].isVisible = 1;
    g_FieldEffects.fades[0].semitransparency = 1;
    g_FieldEffects.fades[0].blueDelta = 0x10000 / duration;
    g_FieldEffects.fades[0].greenDelta = 0x10000 / duration;
    g_FieldEffects.fades[0].redDelta = 0x10000 / duration;
}
// ---------------------------------------------------

void func_800A5774(int x, int y, int h) {
    RECT rect;
    int nSize;
    int i;
    /* 15-bit VRAM words: u32 (not u_long) so each |= 0x80008000 covers exactly two
     * pixels and the stride stays 4 bytes on the 64-bit port. u32 == u_long on PSX,
     * and nSize keys off sizeof, so this is byte-identical for matching. */
    u32* pImageBuffer;
    u32* pWorkBuffer;

    rect.x = x;
    rect.y = y;
    rect.w = 0x40;
    rect.h = h;
    pImageBuffer = HeapAlloc(h * 0x80, 0x1);
    StoreImage( &rect, pImageBuffer);
    DrawSync(0);

    pWorkBuffer = pImageBuffer;
    nSize = h * (0x80 / sizeof(u32));
    for (i = 0; i < nSize; i += 8) {
        pWorkBuffer[0] |= 0x80008000;
        pWorkBuffer[1] |= 0x80008000;
        pWorkBuffer[2] |= 0x80008000;
        pWorkBuffer[3] |= 0x80008000;
        pWorkBuffer[4] |= 0x80008000;
        pWorkBuffer[5] |= 0x80008000;
        pWorkBuffer[6] |= 0x80008000;
        pWorkBuffer[7] |= 0x80008000; 
        pWorkBuffer += 8;
    }
    
    LoadImage(&rect, pImageBuffer);
    DrawSync(0);
    HeapFree(pImageBuffer);
}

// Zoom fade effect stuff
void func_800A5884(int semiTrans, int abr) {
    int i;
    int nCurX;

    FieldZoomFadeEffectInitialize(semiTrans, abr);
    for (i = 0; i < 2; i++) {
        FieldClearAndSwapOTag();
        FieldZoomFadeEffectUpdate();
        FieldDisplay();
    }

    nCurX = 0x2C0;
    for (i = 0; i < 5; i++) {
        func_800A5774(nCurX, 0x100, 0xE0);
        nCurX += 0x40;
    }

    for (i = 0; i < 2; i++) {
        FieldClearAndSwapOTag();
        FieldZoomFadeEffectUpdate();
        FieldDisplay();
    }
}

extern s32 D_800ADB38;
extern s32 D_800ADB3C;
extern s16 D_800ADC08;
extern s32 D_800C3A40;
extern void FontFree(void);
extern void func_80070C84(void);
extern void func_800A915C(void);
extern void func_800A4748(void);
extern void FieldClearAndSwapOTag(void);
extern void FieldRenderSync(void);
extern void func_800A6C40(void);
extern void func_800A6E70(void);
extern void func_80077DAC(void);
extern void func_8007554C(void);
extern void func_80078B5C(void);
extern void FieldFadeToBlack(s32 duration);
extern void FieldFadeToWhite(s32 duration);
extern void func_800A91F0(void);
extern void func_80077544(void);

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/field/nonmatchings/main/misc5", func_800A5924);
#else
void func_800A5924(void) {
    s32 i;
    s32 color;

    if (D_800ADB38 == 0) {
        return;
    }

    FontFree();
    func_80070C84();
    func_800A915C();

    if (D_800ADB38 == 1 || D_800ADB38 == 4) {
        func_800A4748();
        DrawSync(0);
        FieldClearAndSwapOTag();
        FieldRenderSync();
    }

    FieldRenderSync();

    for (;;) {
        if (D_800ADB38 == 3) {
            FieldZoomFadeEffectInitialize(1, 1);
            for (i = 0; i < 5; i++) {
                func_800A5774(0x2C0 + i * 0x40, 0x100, 0xE0);
            }
            FieldFadeToWhite(D_800ADB3C);
            func_800A5600(0);

            color = 0;
            for (i = 0; i < D_800ADB3C; i++) {
                func_80077DAC();
                FieldZoomFadeEffectUpdate();
                func_8007554C();
                func_80078B5C();
                func_800A5600(color >> 16);
                color += 0x800000 / D_800ADB3C;
            }

            while (D_800ADB38 == 3) {
                func_80077DAC();
                FieldZoomFadeEffectUpdate();
                func_8007554C();
                func_80078B5C();
            }
            continue;
        }

        if (D_800ADB38 == 4) {
            func_800A5884(1, 1);
            func_800A6E70();
            D_800ADC08 = 1;
            FieldFadeToBlack(D_800ADB3C);
            D_800C3A40 = 0;

            for (i = 0; i < D_800ADB3C; i++) {
                func_80077DAC();
                func_800A6C40();
                func_8007554C();
                func_80078B5C();
                D_800C3A40 += 6;
            }

            DrawSync(0);
            func_800A7064();
        } else if (D_800ADB38 > 0 && D_800ADB38 < 4) {
            func_800A5884(1, 1);
            D_800ADC08 = 1;
            FieldFadeToBlack(D_800ADB3C);

            color = 0x800000;
            for (i = 0; i < D_800ADB3C; i++) {
                func_80077DAC();
                FieldZoomFadeEffectUpdate();
                func_8007554C();
                func_80078B5C();
                func_800A5600(color >> 16);
                color -= 0x800000 / D_800ADB3C;
                if (color < 0) {
                    color = 0;
                }
            }
        }

        D_800ADB38 = 0;
        func_800A91F0();
        func_80077544();
        return;
    }
}
#endif /* XENO_PC_PORT */

extern s32 D_800B0048;
extern s32 D_800AFD14;
extern s32 D_800C2684;
extern s32 D_800ADB60;
extern void* D_800ADC14;
extern s32 D_800AFD04;
extern s32 D_8004F308;
extern s32 D_8004F324;
extern int D_8005A4C0;
extern void* D_8005A4E0;
extern void func_800864F0(void);
extern void func_800A915C(void);
extern void func_800A4748(void);
extern void func_800A90B4(s32);
extern void func_800A5710(int);
extern void func_800A56A8(int);
extern void func_80078C5C(void);
extern void FieldLoad(void);
extern void func_8007FFE8(void);
extern void FieldParticlesFreeAll(void);
extern int ArchiveDataSync(void);
extern void func_80070488(void);
extern void func_80070508(void);
extern void GamePartySyncSkinData(void);
extern void GamePartySyncStreamedData(void);
extern void FieldFadeUpdateAndDraw(void* ot, int renderContextIndex);

// FieldMain's seamless map-transition orchestrator (called once from
// main.c's after_ctx block once func_800932D0/CHANGE_FIELD has cleared
// D_800ADBEC and the stream loader is idle). Tears down the outgoing field,
// preserves the one persistent 0x8005A4C0-sized heap block across that
// teardown, then dispatches on D_800B0048 (the transition-effect selector,
// jtbl_8006FDAC) to one of 7 fade/zoom variants around the actual
// FieldLoad() call, and finally normalizes state for the next transition
// (D_800B0048=2, D_800AFD14=0x20, D_800AFD04=0).
#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/field/nonmatchings/main/misc5", func_800A5C40);
#else
void func_800A5C40(void) {
    void* pSaved;
    s32 savedMapMode;
    s32 savedFadeDur;
    s32 transitionMode;
    s32 runSharedFadeToBlack;
    s32 color;
    s32 i;
    RECT rect;

    FontFree();
    FieldParticlesFreeAll();
    func_800864F0();
    func_8007FFE8();

    if (D_800B0048 != 6) {
        func_800A915C();
        if (D_800B0048 != 4) {
            func_800A4748();
        }
    }

    DrawSync(0);
    FieldClearAndSwapOTag();
    FieldRenderSync();
    FieldFree();

    pSaved = HeapAlloc(D_8005A4C0, 0);
    memcpy(pSaved, D_8005A4E0, D_8005A4C0);
    HeapUnpinBlock(D_8005A4E0);
    HeapFree(D_8005A4E0);

    if (D_800B0048 != 6) {
        func_800A90B4(1);
    }

    D_8005A4E0 = HeapAlloc(D_8005A4C0, 1);
    memcpy(D_8005A4E0, pSaved, D_8005A4C0);
    HeapPinBlock(D_8005A4E0);
    HeapFree(pSaved);

    transitionMode = D_800B0048;
    runSharedFadeToBlack = 0;

    if ((u32)transitionMode < 7) {
        switch (transitionMode) {
        case 6:
            FieldFadeToWhite(D_800AFD14);
            for (i = 0; i < D_800AFD14; i++) {
                FieldClearAndSwapOTag();
                FieldFadeUpdateAndDraw((u8*)g_FieldCurRenderContext + 0x80D4, g_FieldCurRenderContextIndex);
                FieldDisplay();
            }
            runSharedFadeToBlack = 1;
            break;

        case 0:
            FieldZoomFadeEffectInitialize(0, 0);
            FieldFadeToWhite(D_800AFD14);
            for (i = 0; i < D_800AFD14; i++) {
                FieldClearAndSwapOTag();
                FieldFadeUpdateAndDraw((u8*)g_FieldCurRenderContext + 0x80D4, g_FieldCurRenderContextIndex);
                FieldZoomFadeEffectUpdate();
                FieldDisplay();
            }
            runSharedFadeToBlack = 1;
            break;

        case 1:
            FieldZoomFadeEffectInitialize(0, 0);
            func_800A5710(D_800AFD14);
            for (i = 0; i < D_800AFD14; i++) {
                FieldClearAndSwapOTag();
                FieldFadeUpdateAndDraw((u8*)g_FieldCurRenderContext + 0x80D4, g_FieldCurRenderContextIndex);
                FieldZoomFadeEffectUpdate();
                FieldDisplay();
            }
            FieldRenderSync();
            GamePartySyncSkinData();
            GamePartySyncStreamedData();
            savedMapMode = D_800B0048;
            savedFadeDur = D_800AFD14;
            FieldLoad();
            func_80070488();
            func_80070508();
            D_800B0048 = savedMapMode;
            D_800AFD14 = savedFadeDur;
            if (D_8004F308 == -1) {
                func_80085B20(D_8004F324, 0);
            }
            func_800A56A8(D_800AFD14);
            break;

        case 2:
        case 4:
            func_800A5884(1, 1);
            GamePartySyncSkinData();
            GamePartySyncStreamedData();
            savedMapMode = D_800B0048;
            savedFadeDur = D_800AFD14;
            FieldLoad();
            func_80070488();
            if (D_800ADB60 == 1) {
                while (ArchiveDataSync() != 0) {
                    FieldClearAndSwapOTag();
                    FieldZoomFadeEffectUpdate();
                    FieldDisplay();
                    if (D_800C2684 < 0x22C0) {
                        D_800C2684 += 0x20;
                    }
                }
                HeapFree(D_800ADC14);
                D_800ADB60 = 0;
                func_80078C5C();
            }
            D_800AFD04 = 1;
            D_800B0048 = savedMapMode;
            D_800AFD14 = savedFadeDur;
            if (D_8004F308 == -1) {
                func_80085B20(D_8004F324, 0);
            }
            FieldFadeToBlack(D_800AFD14);
            if (D_800AFD14 > 0) {
                color = 0x800000;
                for (i = 0; i < D_800AFD14; i++) {
                    func_80077DAC();
                    FieldZoomFadeEffectUpdate();
                    func_8007554C();
                    func_80078B5C();
                    func_800A5600(color >> 16);
                    color -= 0x800000 / D_800AFD14;
                    if (color < 0) {
                        color = 0;
                    }
                    if (D_800C2684 < 0x22C0) {
                        D_800C2684 += 0x20;
                    }
                }
            }
            break;

        case 3:
            FieldZoomFadeEffectInitialize(0, 0);
            func_80070488();
            FieldClearAndSwapOTag();
            FieldZoomFadeEffectUpdate();
            FieldDisplay();
            GamePartySyncSkinData();
            GamePartySyncStreamedData();
            savedMapMode = D_800B0048;
            savedFadeDur = D_800AFD14;
            D_800AFD04 = 1;
            FieldLoad();
            func_80070508();
            D_800B0048 = savedMapMode;
            D_800AFD14 = savedFadeDur;
            if (D_8004F308 == -1) {
                func_80085B20(D_8004F324, 0);
            }
            for (i = 0; i < 4; i++) {
                func_80077DAC();
                FieldZoomFadeEffectUpdate();
                func_8007554C();
                func_80078B5C();
            }
            break;

        case 5:
            FieldZoomFadeEffectInitialize(0, 0);
            func_80070488();
            FieldClearAndSwapOTag();
            FieldZoomFadeEffectUpdate();
            FieldDisplay();
            GamePartySyncSkinData();
            GamePartySyncStreamedData();
            savedMapMode = D_800B0048;
            savedFadeDur = D_800AFD14;
            D_800AFD04 = 1;
            FieldLoad();
            func_80070508();
            rect.x = 0x2C0;
            rect.y = 0x100;
            rect.w = 0x140;
            rect.h = 0xFF;
            D_800B0048 = savedMapMode;
            D_800AFD14 = savedFadeDur;
            MoveImage(&rect, 0x140, 0xFF);
            if (D_8004F308 == -1) {
                func_80085B20(D_8004F324, 0);
            }
            for (i = 0; i < 4; i++) {
                func_80077DAC();
                FieldZoomFadeEffectUpdate();
                func_8007554C();
                func_80078B5C();
            }
            break;
        }

        if (runSharedFadeToBlack) {
            FieldClearAndSwapOTag();
            FieldDisplay();
            GamePartySyncSkinData();
            GamePartySyncStreamedData();
            savedMapMode = D_800B0048;
            savedFadeDur = D_800AFD14;
            FieldLoad();
            func_80070488();
            func_80070508();
            D_800B0048 = savedMapMode;
            D_800AFD14 = savedFadeDur;
            if (D_8004F308 == -1) {
                func_80085B20(D_8004F324, 0);
            }
            FieldFadeToBlack(D_800AFD14);
        }
    }

    if (D_800B0048 != 6) {
        func_800A91F0();
    }
    D_800B0048 = 2;
    D_800AFD14 = 0x20;
    D_800AFD04 = 0;
    func_80077544();
    HeapConsolidate();
}
#endif /* XENO_PC_PORT */

extern s32 D_800C2684;
extern SVECTOR D_800B00B8;

// Or Draw?
#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/field/nonmatchings/main/misc5", FieldZoomFadeEffectUpdate);
#else
void FieldZoomFadeEffectUpdate(void) {
    MATRIX matrix;
    VECTOR scale;
    /* PsyQ RotAverage4(long *p, long *flag): retail asm stores FLAG with a 32-bit
     * `sw` (RotAverage4.s 8004A824) into adjacent sp slots (FieldZoomFadeEffectUpdate.s
     * passes sp+0x58 / sp+0x5C). On MIPSel int==long so `int flag` was fine. On the
     * LP64 port, psyq_compat RotAverage4 sign-extends with a full `long` store through
     * long* — `(long*)&int flag` overwrites the next stack word (loop `i`) and the
     * for-i never terminates (New Game freezes on the last menu frame). Match the
     * API / FieldRenderQuad: real long locals. */
    long interpolation;
    long flag;
    int i;

    RotMatrix(&D_800B00B8, &matrix);
    scale.vx = D_800C2684;
    scale.vy = D_800C2684;
    scale.vz = D_800C2684;
    ScaleMatrix(&matrix, &scale);
    SetRotMatrix(&matrix);
    SetTransMatrix(&matrix);

    for (i = 0; i < 5; i++) {
        POLY_FT4* poly = &g_FieldZoomFadeEffect.polygons[(i * 2) + g_FieldCurRenderContextIndex];
        DR_MODE* drMode = &g_FieldZoomFadeEffect.drawModes[(i * 2) + g_FieldCurRenderContextIndex];

        if (D_800C2684 != 0x1000) {
            SVECTOR* vertices = &g_FieldZoomFadeEffect.vectors[i * 4];
            RotAverage4(
                &vertices[0], &vertices[1], &vertices[2], &vertices[3],
                (long*)&poly->x0, (long*)&poly->x1, (long*)&poly->x2, (long*)&poly->x3,
                &interpolation, &flag
            );
        }

        addPrim(g_FieldCurRenderContext->ot3, poly);
        addPrim(g_FieldCurRenderContext->ot3, drMode);
    }
}
#endif /* XENO_PC_PORT */

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/field/nonmatchings/main/misc5", FieldZoomFadeEffectInitialize);
#else
void FieldZoomFadeEffectInitialize(int semiTrans, int abr) {
    int i;
    int tpageX;
    int x0;
    int x1;
    int vx0;
    int vx1;

    D_800C2684 = 0x1000;
    D_800B00B8.vx = 0;
    D_800B00B8.vy = 0;
    D_800B00B8.vz = 0;

    tpageX = 0x2C0;
    x0 = 0;
    x1 = 0x40;
    vx0 = -0x50;
    vx1 = -0x30;

    for (i = 0; i < 5; i++) {
        POLY_FT4* poly0 = &g_FieldZoomFadeEffect.polygons[i * 2];
        POLY_FT4* poly1 = &g_FieldZoomFadeEffect.polygons[(i * 2) + 1];
        RECT* rect0 = &g_FieldZoomFadeEffect.rects[i * 2];
        RECT* rect1 = &g_FieldZoomFadeEffect.rects[(i * 2) + 1];
        u_short tpage = GetTPage(2, abr, tpageX, 0x100);
        SVECTOR* vertices = &g_FieldZoomFadeEffect.vectors[i * 4];

        SetPolyFT4(poly0);
        setRGB0(poly0, 0x80, 0x80, 0x80);
        setXY4(poly0, x0, 0, x1, 0, x0, 0xDF, x1, 0xDF);
        setUV4(poly0, 0, 0, 0x40, 0, 0, 0xDF, 0x40, 0xDF);
        poly0->tpage = tpage;
        SetSemiTrans(poly0, semiTrans);

        vertices[0].vx = vx0;
        vertices[0].vy = -0x38;
        vertices[0].vz = 0;
        vertices[1].vx = vx1;
        vertices[1].vy = -0x38;
        vertices[1].vz = 0;
        vertices[2].vx = vx0;
        vertices[2].vy = 0x38;
        vertices[2].vz = 0;
        vertices[3].vx = vx1;
        vertices[3].vy = 0x38;
        vertices[3].vz = 0;

        rect0->x = 0;
        rect0->y = 0;
        rect0->w = 0xFF;
        rect0->h = 0xFF;
        rect1->x = 0;
        rect1->y = 0;
        rect1->w = 0xFF;
        rect1->h = 0xFF;

        SetDrawMode(&g_FieldZoomFadeEffect.drawModes[i * 2], 0, 0, tpage, rect0);
        SetDrawMode(&g_FieldZoomFadeEffect.drawModes[(i * 2) + 1], 0, 0, tpage, rect1);

        *poly1 = *poly0;

        tpageX += 0x40;
        x0 += 0x40;
        x1 += 0x40;
        vx0 += 0x20;
        vx1 += 0x20;
    }
}
#endif /* XENO_PC_PORT */

void FieldDisplay(void) {
    DrawSync(0);
    Vsync(2);
    ClearImage(&g_FieldCurRenderContext->drawEnvs[0].clip, 0x0, 0x0, 0x0);
    PutDrawEnv(&g_FieldCurRenderContext->drawEnvs[0]);
    PutDispEnv(&g_FieldCurRenderContext->dispEnv);
    DrawOTag(g_FieldCurRenderContext->ot3 + 7);
}

/* Fade one corner of a starfield quad by its distance from screen centre.
 * `corner` selects vertex 0..3 of the POLY_GT4; `threshold` is D_800C3A40 and
 * `pFade` points at that corner's entry in the matching fade table. When the
 * corner is within `threshold` of (160, 112) its fade value steps down by 6
 * (clamped at 0) and is written to all three colour channels of that vertex.
 * Callers: func_800A6C40, once per corner per cell.
 *
 * cx/cy are locals rather than literals on purpose: retail materialises 0xA0
 * and 0x70 once ahead of the switch and shares them across all four cases.
 * With bare literals GCC re-materialises them inside each case, costing four
 * instructions against retail.
 * Retail: asm/field/main/misc5.s 800A6998-800A6C3C. */
void func_800A6998(POLY_GT4* poly, s32 corner, s32 threshold, s16* pFade) {
    VECTOR d;
    VECTOR sq;
    s32 cx = 0xA0;
    s32 cy = 0x70;

    d.vz = 0;

    switch (corner) {
    case 0:
        d.vx = cx - poly->x0;
        d.vy = cy - poly->y0;
        Square0(&d, &sq);
        if ((SquareRoot0(sq.vx + sq.vy) >> 1) < threshold) {
            *pFade -= 6;
            if (*pFade < 0) {
                *pFade = 0;
            }
            poly->r0 = *pFade;
            poly->g0 = *pFade;
            poly->b0 = *pFade;
        }
        break;
    case 1:
        d.vx = cx - poly->x1;
        d.vy = cy - poly->y1;
        Square0(&d, &sq);
        if ((SquareRoot0(sq.vx + sq.vy) >> 1) < threshold) {
            *pFade -= 6;
            if (*pFade < 0) {
                *pFade = 0;
            }
            poly->r1 = *pFade;
            poly->g1 = *pFade;
            poly->b1 = *pFade;
        }
        break;
    case 2:
        d.vx = cx - poly->x2;
        d.vy = cy - poly->y2;
        Square0(&d, &sq);
        if ((SquareRoot0(sq.vx + sq.vy) >> 1) < threshold) {
            *pFade -= 6;
            if (*pFade < 0) {
                *pFade = 0;
            }
            poly->r2 = *pFade;
            poly->g2 = *pFade;
            poly->b2 = *pFade;
        }
        break;
    case 3:
        d.vx = cx - poly->x3;
        d.vy = cy - poly->y3;
        Square0(&d, &sq);
        if ((SquareRoot0(sq.vx + sq.vy) >> 1) < threshold) {
            *pFade -= 6;
            if (*pFade < 0) {
                *pFade = 0;
            }
            poly->r3 = *pFade;
            poly->g3 = *pFade;
            poly->b3 = *pFade;
        }
        break;
    }
}

extern void* D_800B00C4;
extern u8 D_800B1E24[];
extern void func_800A6998(POLY_GT4* poly, s32 corner, s32 threshold, s16* pFade);

/* Per-frame update of the 14x20 starfield grid allocated by func_800A6E70.
 * D_800B00C4 holds one 0x7A80 block: two 280-entry POLY_GT4 pages (one per
 * render context, 0x38E0 each) followed by four 280-entry s16 fade tables at
 * 0x71C0/0x73F0/0x7620/0x7850 -- one per quad corner. Each cell hands its quad
 * and the matching corner's fade cell to func_800A6998, which dims that corner
 * as it approaches screen centre, then links the quad into the OT. The tail
 * links the shared 0xC0-byte D_800B1E24 primitive for this context.
 *
 * The address expression is deliberately repeated at the addPrim rather than
 * reusing `poly`: retail recomputes it there (the calls invalidate GCC's CSE of
 * the two globals), and folding it back into the cached pointer costs 11
 * instructions against retail.
 * Retail: asm/field/main/misc5.s 800A6C40-800A6E6C. */
#define STAR_K(row, col) (((row) * 0x14) + (col))
#define STAR_POLY(row, col) \
    ((POLY_GT4*)((u8*)D_800B00C4 + (g_FieldCurRenderContextIndex * 0x38E0) + \
                 (STAR_K(row, col) * 0x34)))
#define STAR_FADE(ofs, row, col) \
    ((s16*)((u8*)D_800B00C4 + (ofs) + (STAR_K(row, col) * 2)))

void func_800A6C40(void) {
    s32 row;
    s32 col;
    POLY_GT4* poly;

    for (row = 0; row < 0xE; row++) {
        for (col = 0; col < 0x14; col++) {
            poly = STAR_POLY(row, col);
            func_800A6998(poly, 0, D_800C3A40, STAR_FADE(0x71C0, row, col));
            func_800A6998(poly, 1, D_800C3A40, STAR_FADE(0x73F0, row, col));
            func_800A6998(poly, 2, D_800C3A40, STAR_FADE(0x7620, row, col));
            func_800A6998(poly, 3, D_800C3A40, STAR_FADE(0x7850, row, col));
            addPrim(g_FieldCurRenderContext->ot3, STAR_POLY(row, col));
        }
    }

    addPrim(g_FieldCurRenderContext->ot3, &D_800B1E24[g_FieldCurRenderContextIndex * 0xC0]);
}

extern void* D_800B00C4;

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/field/nonmatchings/main/misc5", func_800A6E70);
#else
void func_800A6E70(void) {
    u8* pBuf;
    s32 row, col;
    D_800B00C4 = HeapAlloc(0x7A80, 1);
    pBuf = (u8*)D_800B00C4;
    for (row = 0; row < 0xE; row++) {
        for (col = 0; col < 0x14; col++) {
            u8* pPrim = pBuf + (row * 0x14 + col) * 0x34;
            u8* pDest = pBuf + (row * 0x14 + col) * 0x34 + 0x38E0;
            s16 uOfs = col * 0x10;
            s16 vOfs = row * 0x10;
            s32 tpageX;
            s32 i;

            /* Set vertex positions */
            *(s16*)(pPrim + 0x08) = uOfs;
            *(s16*)(pPrim + 0x0A) = vOfs;
            *(s16*)(pPrim + 0x20) = uOfs;
            *(s16*)(pPrim + 0x22) = vOfs + 0x10;
            *(s16*)(pPrim + 0x14) = uOfs + 0x10;
            *(s16*)(pPrim + 0x16) = vOfs;
            *(s16*)(pPrim + 0x2C) = uOfs + 0x10;
            *(s16*)(pPrim + 0x2E) = vOfs + 0x10;

            SetPolyGT4(pPrim);

            /* Set vertex colors (0x80 per component) */
            pPrim[0x04] = pPrim[0x05] = pPrim[0x06] = 0x80;
            pPrim[0x10] = pPrim[0x11] = pPrim[0x12] = 0x80;
            pPrim[0x1C] = pPrim[0x1D] = pPrim[0x1E] = 0x80;
            pPrim[0x28] = pPrim[0x29] = pPrim[0x2A] = 0x80;

            /* Set UVs */
            {
                s32 uBase = uOfs & 0x3F;
                pPrim[0x0C] = (u8)uBase;
                pPrim[0x0D] = (u8)vOfs;
                pPrim[0x18] = (u8)(uBase + 0x10);
                pPrim[0x19] = (u8)vOfs;
                pPrim[0x24] = (u8)uBase;
                pPrim[0x25] = (u8)(vOfs + 0x10);
                pPrim[0x30] = (u8)(uBase + 0x10);
                pPrim[0x31] = (u8)(vOfs + 0x10);
            }

            /* Tpage */
            tpageX = (col < 0 ? col + 3 : col) >> 2;
            *(u16*)(pPrim + 0x1A) = GetTPage(2, 1, tpageX * 0x40 + 0x2C0, 0x100);
            SetSemiTrans(pPrim, 1);

            /* Copy to destination */
            for (i = 0; i < 48; i += 16) {
                *(s32*)(pDest + i) = *(s32*)(pPrim + i);
                *(s32*)(pDest + i + 4) = *(s32*)(pPrim + i + 4);
                *(s32*)(pDest + i + 8) = *(s32*)(pPrim + i + 8);
                *(s32*)(pDest + i + 12) = *(s32*)(pPrim + i + 12);
            }
            *(s32*)(pDest + 48) = *(s32*)(pPrim + 48);
        }
    }
}
#endif /* XENO_PC_PORT */

extern void* D_800B00C4;

void func_800A7064(void) {
    HeapFree(D_800B00C4);
}

extern s32 D_800ADB6C;
extern s32 D_800ADB74;
extern s32 D_800ADB78;
extern s32 D_800ADB80;
extern s32 D_800AFE74;
extern s32 D_800B00E4;
extern s32 D_800B06A0;
extern s32 D_801E89E0;
extern s32 D_801D68B4;
extern s16 D_800C3A20, D_800C3A22, D_800C3A24, D_800C3A26, D_800C3A28, D_800C3A2A, D_800C3A2C, D_800C3A2E;
extern s16 D_800C3A30, D_800C3A32, D_800C3A34, D_800C3A36, D_800C3A38, D_800C3A3A;
extern void func_801D3538(s32 a, s32 b, s32 c, s32 d, s32 e, s32 f, s32 g);

void func_800A708C(void) {
    HeapChangeCurrentUser(4, NULL);
    if (D_800ADB74 == 2) {
        D_801D68B4 = 1;
    } else {
        D_801D68B4 = 0;
    }
    func_801D3538(0x140, 0xE0, 0x80, 0x10, 0x20, 0x800, (u16)D_800C3A36);
    D_800ADB6C = 0;
    HeapChangeCurrentUser(8, NULL);
}

extern s32 D_800B06A0;
extern s32 D_800B00E4;
extern s32 D_800ADB78;
extern s32 D_800ADB74;
extern s32 D_800AFE74;
extern s16 D_800C3A36;

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/field/nonmatchings/main/misc5", func_800A7120);
#else
void func_800A7120(u16 arg0, s32 arg1, u16 arg2) {
    D_800B06A0 = arg0;
    D_800B00E4 = 0;
    if (arg2 == 0) {
        D_800ADB78 = 1;
    } else {
        D_800ADB78 = 0;
    }
    if (D_800ADB74 == 0 && D_800AFE74 == 0) {
        DrawSync(0);
        g_FieldCurRenderContext = &g_FieldRenderContexts[D_800ADB78];
        if (D_800C3A36 == 1) {
            g_FieldCurRenderContext->dispEnv.isrgb24 = 1;
        }
    }
}
#endif /* XENO_PC_PORT */

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/field/nonmatchings/main/misc5", func_800A7218);
#else
void func_800A7218(void) {
    D_800B06A0 = 0;
    HeapChangeCurrentUser(4, NULL);
    if (D_800ADB6C == 0) {
        s32 fadeType;
        ArchiveSetIndex(0x18, 1);
        if (D_800C3A38 == 0xFF) {
            fadeType = 1;
            if (D_800ADB80 & 0x40) {
                fadeType = 3;
            }
        } else {
            fadeType = 3;
        }
        func_801D37CC(
            D_800C3A20 + 2, D_800C3A2A, D_800C3A2C,
            D_800C3A2E, 1, fadeType,
            D_800C3A3A, D_800C3A22, D_800C3A24,
            D_800C3A26, D_800C3A28, 0xE0, (void*)func_800A7120
        );
        ArchiveSetIndex(4, 0);
    }
    HeapChangeCurrentUser(8, NULL);
}
#endif /* XENO_PC_PORT */

extern s32 D_800ADB6C;

void func_800A732C(s32 arg0) {
    s32 i;
    u8 unused[1]; /* retail frame has one stack slot no code touches */

    GameCheckAndHandleSoftReset();
    if (D_800ADB6C == 0) {
        for (i = 0; i < arg0; i++) {
            func_801D3F7C();
            func_80085678();
        }
    }
}

void func_800A7394(void) {
    do {
        func_80077DAC();
        func_8007554C();
    } while (ArchiveDataSync() != 0 || g_FieldCurRenderContextIndex != 0);
    CdDataSync(0);
}

extern void* D_8005A418;
extern void* D_8005A41C;

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/field/nonmatchings/main/misc5", func_800A73E8);
#else
void func_800A73E8(void) {
    if (D_800ADB74 == 2) {
        HeapUnpinBlock(D_8005A418);
        HeapUnpinBlock(D_8005A41C);
    } else {
        RECT rect;
        rect.x = 0x200; rect.y = 0;
        rect.w = 0x140; rect.h = 0x80;
        LoadImage(&rect, D_8005A418);
        DrawSync(0);
        rect.y = 0x80;
        LoadImage(&rect, D_8005A41C);
        DrawSync(0);
        HeapUnpinBlock(D_8005A418);
        HeapUnpinBlock(D_8005A41C);
    }
    HeapFree(D_8005A418);
    HeapFree(D_8005A41C);
}
#endif /* XENO_PC_PORT */

/* Transcribed from asm/field/nonmatchings/main/misc5/func_800A74F8.s
 * (0x800A74F8-0x800A7740). Captures party slots 1 and 2 into D_8005A418 /
 * D_8005A41C (not g_PartyDataBuffers). Mode 2 streams skins[1..2]+5 and
 * LZSSDecompress into those two blocks; otherwise StoreImage 0x200,0 /
 * 0x200,0x80. */
extern s32 D_800ADB74;
extern s32 g_GamePartyMemberSkins[3];
extern s32 g_GamePartyMembers[3];
extern void* g_PartyStreamDataPointers[3];
extern void* g_PartyDataBuffers[3];
extern s32 func_80029AFC(StreamDataQueueEntry* entries, s32 arg1, s32 arg2);
extern u32 LZSSDecompress(void* source, void* destination);

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/field/nonmatchings/main/misc5", func_800A74F8);
#else
void func_800A74F8(void) {
    s32 slot;

    if (D_800ADB74 == 2) {
        StreamDataQueueEntry requests[3];
        s32 count;
        void* dests[2];

        D_8005A418 = HeapAlloc(0x14000, 0);
        D_8005A41C = HeapAlloc(0x14000, 0);
        HeapPinBlock(D_8005A418);
        HeapPinBlock(D_8005A41C);

        count = 0;
        ArchiveSetIndex(4, 0);
        for (slot = 1; slot < 3; slot++) {
            if (g_GamePartyMemberSkins[slot] != 0xFF) {
                requests[count].archiveIndex =
                    (s16)(g_GamePartyMemberSkins[slot] + 5);
                g_PartyStreamDataPointers[slot] = HeapAlloc(
                    ArchiveDecodeAlignedSize(g_GamePartyMemberSkins[slot] + 5),
                    1);
                requests[count].pData = g_PartyStreamDataPointers[slot];
                count++;
            }
        }
        requests[count].archiveIndex = 0;
        requests[count].pData = NULL;
        func_80029AFC(requests, 0, 0);
        ArchiveCdDataSync(0);

        dests[0] = D_8005A418;
        dests[1] = D_8005A41C;
        for (slot = 1; slot < 3; slot++) {
            if (g_GamePartyMembers[slot] != 0xFF) {
                LZSSDecompress(g_PartyStreamDataPointers[slot], dests[slot - 1]);
                HeapFree(g_PartyStreamDataPointers[slot]);
            }
        }
    } else {
        RECT rect;

        D_8005A418 = HeapAlloc(0x14000, 0);
        D_8005A41C = HeapAlloc(0x14000, 0);
        HeapPinBlock(D_8005A418);
        HeapPinBlock(D_8005A41C);

        rect.x = 0x200;
        rect.y = 0;
        rect.w = 0x140;
        rect.h = 0x80;
#ifdef FIELD_A74F8_MUTANT_PARTY_BUFFERS
        StoreImage(&rect, g_PartyDataBuffers[1]);
#else
        StoreImage(&rect, D_8005A418);
#endif
        DrawSync(0);
        rect.y = 0x80;
#ifdef FIELD_A74F8_MUTANT_PARTY_BUFFERS
        StoreImage(&rect, g_PartyDataBuffers[2]);
#else
        StoreImage(&rect, D_8005A41C);
#endif
        DrawSync(0);
    }
}
#endif /* XENO_PC_PORT */

extern int g_FieldPixelIndex;
/* These image-convert buffers are 32-bit words on PSX (u_long == u32 == 4 bytes
 * there). The 64-bit port widens u_long to 8 bytes, which would double the deref
 * width and pointer stride below -- overflowing pImage15Bit (0x7000 bytes, written
 * 0x1C00 times) into adjacent heap blocks. Use u32 so the stride stays 4 bytes;
 * byte-identical to u_long on the MIPS target, so matching is preserved. */
extern u32 g_FieldCurPixel;
extern u32 g_Field24BitImageData;
extern u32 g_Field15BitImageData;

u_int FieldImageConvert24BitColorTo15Bit(void) {
    u32 nPixel;
    u_int nLSB;

    // Are we done reading RGB channels?
    if (!(g_FieldPixelIndex & 3)) {
        nPixel = *(u32*)(uintptr_t)g_Field24BitImageData;
        g_Field24BitImageData += sizeof(u32);
        g_FieldCurPixel = nPixel;
    }
    
    g_FieldPixelIndex++;
    
    nLSB = g_FieldCurPixel & 0xFF;
    g_FieldCurPixel >>= 8;
    
    if (nLSB) {
        nLSB = nLSB >> 3;
        if (nLSB == 0)
            nLSB = 1;
    }
    
    return nLSB;
}

void FieldImageConvert24BitTo15Bit(void) {
    RECT rect;
    int i;
    int j;
    u32* pImage24Bit;
    u32* pImage15Bit;
    u32 n15BitPixels;

    pImage24Bit = HeapAlloc(0xA800, 0x0);
    pImage15Bit = HeapAlloc(0x7000, 0x0);
    
    for (i = 0; i < 5; i++) {
        rect.x = i * 0x60;
        rect.y = 0;
        rect.w = 0x60;
        rect.h = 0xE0;
        StoreImage(&rect, pImage24Bit);
        DrawSync(0);
        g_Field24BitImageData = (u32)(uintptr_t)pImage24Bit;
        g_Field15BitImageData = (u32)(uintptr_t)pImage15Bit;
        g_FieldPixelIndex = 0;
        
        for (j = 0; j < 0x1C00; j++) {
            // 15 Bit: RGBRGB
            n15BitPixels = FieldImageConvert24BitColorTo15Bit();
            n15BitPixels |= FieldImageConvert24BitColorTo15Bit() << 0x5;
            n15BitPixels |= FieldImageConvert24BitColorTo15Bit() << 0xA;
            n15BitPixels |= FieldImageConvert24BitColorTo15Bit() << 0x10;
            n15BitPixels |= FieldImageConvert24BitColorTo15Bit() << 0x15;
            n15BitPixels |= FieldImageConvert24BitColorTo15Bit() << 0x1A;
            *(u32*)(uintptr_t)g_Field15BitImageData = n15BitPixels;
            g_Field15BitImageData += sizeof(u32);
        }
        
        rect.x = i << 6;
        rect.y = 0x100;
        rect.w = 0x40;
        rect.h = 0xE0;
        LoadImage(&rect, pImage15Bit);
        DrawSync(0);
    }
    HeapFree(pImage24Bit);
    HeapFree(pImage15Bit);
}

/* Transcribed from asm/field/nonmatchings/main/misc5/func_800A7948.s
 * (0x800A7948-0x800A7C54). Gate D_8004F300 and D_800B06A0 in [0x687,0x18E2).
 * Wide 0x280 draw/disp on g_FieldRenderContexts[0]/[1] (retail +0 / +0x80F4),
 * loop while D_800B06A0 < 0x18E2 (check first), then restore 0x140 and
 * isrgb24=1 / isinter=0. */
extern s32 D_8004F300;
extern void GameCheckAndHandleSoftReset(void);
extern void FieldClearAndSwapOTagInternal(void);
extern void func_800ACB90(void);
extern void func_800AC99C(void);
extern void func_800ACCF4(void);
extern void func_800ACCB0(void);

void func_800A7948(void) {
    RECT rect;
    s32 frame;

    if (D_8004F300 == 0) {
        return;
    }
    frame = D_800B06A0;
    if (frame < 0x687) {
        return;
    }
    __asm__ volatile("" ::: "memory");
    if (frame >= 0x18E2) {
        return;
    }

    D_800AFE74 = 1;
    D_801E89E0 = 0;

    rect.x = 0;
    rect.y = 0;
    rect.w = 0x500;
    rect.h = 0x200;
    ClearImage(&rect, 0, 0, 0);
    DrawSync(0);
    Vsync(0);

    SetDefDrawEnv(&g_FieldRenderContexts[0].drawEnvs[0], 0, 0, 0x280, 0xE0);
    SetDefDrawEnv(&g_FieldRenderContexts[1].drawEnvs[0], 0, 0x100, 0x280, 0xE0);
    SetDefDispEnv(&g_FieldRenderContexts[0].dispEnv, 0, 0x100, 0x280, 0xE0);
    SetDefDispEnv(&g_FieldRenderContexts[1].dispEnv, 0, 0, 0x280, 0xE0);
    g_FieldRenderContexts[1].dispEnv.isrgb24 = 0;
    g_FieldRenderContexts[0].dispEnv.isrgb24 = 0;
    PutDispEnv(&g_FieldCurRenderContext->dispEnv);
    PutDrawEnv(&g_FieldCurRenderContext->drawEnvs[0]);

    rect.x = 0x300;
    rect.y = 0;
    rect.w = 0x200;
    rect.h = 0x100;
    ClearImage(&rect, 0, 0, 0);
    func_800ACB90();
    Vsync(0);
    DrawSync(0);

    while (D_800B06A0 < 0x18E2) {
        GameCheckAndHandleSoftReset();
        FieldClearAndSwapOTagInternal();
        if (D_800B06A0 < 0x18DE) {
            func_800AC99C();
        }
        DrawSync(0);
        Vsync(2);
        ClearImage((RECT*)g_FieldCurRenderContext, 0, 0, 0);
        PutDispEnv(&g_FieldCurRenderContext->dispEnv);
        PutDrawEnv(&g_FieldCurRenderContext->drawEnvs[0]);
        DrawOTag(&g_FieldCurRenderContext->ot3[7]);
        func_800ACCF4();
        func_800A732C(5);
    }

    D_800AFE74 = 0;
    D_801E89E0 = 1;
    DrawSync(0);
    Vsync(0);
#ifdef FIELD_A7948_MUTANT_KEEP_WIDE
    SetDefDrawEnv(&g_FieldRenderContexts[0].drawEnvs[0], 0, 0, 0x280, 0xE0);
    SetDefDrawEnv(&g_FieldRenderContexts[1].drawEnvs[0], 0, 0x100, 0x280, 0xE0);
    SetDefDispEnv(&g_FieldRenderContexts[0].dispEnv, 0, 0x100, 0x280, 0xE0);
    SetDefDispEnv(&g_FieldRenderContexts[1].dispEnv, 0, 0, 0x280, 0xE0);
#else
    SetDefDrawEnv(&g_FieldRenderContexts[0].drawEnvs[0], 0, 0, 0x140, 0xE0);
    SetDefDrawEnv(&g_FieldRenderContexts[1].drawEnvs[0], 0, 0x100, 0x140, 0xE0);
    SetDefDispEnv(&g_FieldRenderContexts[0].dispEnv, 0, 0x100, 0x140, 0xE0);
    SetDefDispEnv(&g_FieldRenderContexts[1].dispEnv, 0, 0, 0x140, 0xE0);
#endif
    g_FieldRenderContexts[1].dispEnv.isrgb24 = 1;
    g_FieldRenderContexts[0].dispEnv.isrgb24 = 1;
    g_FieldRenderContexts[1].dispEnv.isinter = 0;
    g_FieldRenderContexts[0].dispEnv.isinter = 0;
    func_800ACCB0();
}

/* Transcribed from asm/field/nonmatchings/main/misc5/func_800A7C58.s
 * (0x800A7C58-0x800A8310). FE60 title/field 2D-background transition:
 * archive 0xA9 upload, overlay HeapAlloc, attract pump, input gates,
 * teardown MoveImage. LoadImage RECT is 0x140,0 x 0xC0,0x100. First
 * MoveImage dest y is D_800ADB78<<8 (a2 leftover), not 0. Both
 * context isrgb24 bytes ( +0xC9 / +0x81BD ) are cleared. */
extern s32 D_800ADB7C;
extern s32 D_800ADB84;
extern s32 D_800ADB50;
extern s32 D_800ADB60;
extern s32 D_800ADB3C;
extern s32 D_800ADB38;
extern s32 D_800B2264;
extern s32 D_800AFE84;
extern void* D_800ADB20;
extern void* D_800ADB30;
extern u16 D_800C3900;
extern s32 D_8004F370;
extern void func_8002A2D0(s32 entryIndex);
extern void ArchiveCdSeekOrPause(s32 entryIndex);
extern void func_801E7FD4(void);
extern void func_800ACC58(void);
extern void func_80085788(void);
extern void func_80085738(void);
extern void func_80077AB4(void);
extern void func_80077884(void);
extern void func_80077DAC(void);
extern void func_8007554C(void);
extern void func_80070488(void);
extern void func_80070508(void);
extern void SoundSetCdVolumeWithFade(s32 targetVolume, s32 fadeFrames);
extern void FieldPollControllers(void);
extern void func_80075910(void);
extern void func_801D43B0(void);
extern void FieldRenderSyncAndFlush(void);
extern void FieldRenderSync(void);

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/field/nonmatchings/main/misc5", func_800A7C58);
#else
void func_800A7C58(void) {
    RECT rect;
    void* pArchiveA9;
    void* pTransition;
    s32 i;
    s32 moveY;
#ifdef XENO_PC_PORT
    s32 transitionSize;
#endif

#ifdef XENO_PC_PORT
    printf("[xeno-port][fe60] enter str=%d start=%d end=%d mode=%d "
           "flags=0x%02x loop=%d rgb24=%d sysmode=%d\n",
           (int)D_800C3A20, (int)D_800C3A2A, (int)D_800C3A2E,
           (int)D_800ADB74, (unsigned)D_800ADB80, (int)D_800C3A3A,
           (int)D_800C3A36, (int)g_FieldSystemMode);
    fflush(stdout);
#endif

    D_800ADB84 = 0;
    D_800B00E4 = 0;
    D_800ADB78 = 0;

    pArchiveA9 = HeapAlloc(ArchiveDecodeAlignedSize(0xA9), 0);
    ArchiveReadFileToBuffer(0xA9, pArchiveA9, 0, 0x80);
    D_800B06A0 = 0;
    D_800AFE74 = 0;
    func_800A7394();

    ArchiveSetIndex(0x18, 0);
#ifdef XENO_PC_PORT
    ArchiveCdSeekOrPause(D_800C3A20);
#else
    func_8002A2D0(D_800C3A20);
#endif
    ArchiveSetIndex(4, 0);
    func_800A7394();

    if (D_800B2264 != 0) {
        func_801E7FD4();
        FieldRenderSyncAndFlush();
        FieldRenderSync();
        HeapFree(D_800ADB20);
    }
    FieldParticlesFreeAll();

    if (D_800ADB74 != 2) {
        rect.x = 0x140;
        rect.y = 0;
#ifdef FIELD_A7C58_MUTANT_SWAP_RECT
        rect.w = 0x100;
        rect.h = 0xC0;
#else
        rect.w = 0xC0;
        rect.h = 0x100;
#endif
        LoadImage(&rect, pArchiveA9);
        DrawSync(0);
        HeapFree(pArchiveA9);

        pArchiveA9 = HeapAlloc(0x18000, 0);
        StoreImage(&rect, pArchiveA9);
    }

    if (D_8004F370 == 0) {
#ifdef XENO_PC_PORT
        /* Retail reserves [0x801D3008,D_800ADB30).  Heap pointers live in
         * g_PsxRam on the native port, so translate the host pointer back to
         * its guest address before applying the original fixed-gap formula. */
        transitionSize = (s32)((PsxMemory_GuestAddr(D_800ADB30) & 0xFFFFFFu) +
                               0xFFE2CFF8u);
        pTransition = HeapAlloc(transitionSize, 1);
#else
        pTransition = HeapAlloc(
            ((u32)D_800ADB30 & 0xFFFFFF) + 0xFFE2CFF8, 1);
#endif
        memcpy(pTransition, pArchiveA9, ArchiveDecodeAlignedSize(0xA9));
    } else {
        /* Delay-slot ori a0,8 on the D_8004F370!=0 branch. */
        pTransition = HeapAlloc(8, 1);
    }
    HeapFree(pArchiveA9);

    func_80085788();
    func_800ACC58();
    D_800B00E4 = 1;
    func_800A73E8();
    FieldRenderSyncAndFlush();
    HeapConsolidate();
    func_800A708C();
    func_800A7218();
    D_800ADB7C = 1;
    do {
        Vsync(0);
        func_800A732C(3);
    } while (D_800B00E4 != 0);
#ifdef XENO_PC_PORT
    printf("[xeno-port][fe60] first STR frame delivered (D_800ADB7C=1)\n");
    fflush(stdout);
    /* Menu confirm (Circle) can still sit in the host pad queue as
     * PressedOnce. Retail's edge is consumed by the menu reader; without
     * a drain the first FieldPollControllers here skips the STR after a
     * handful of frames (observed exit frame=10/11 after New Game). */
    {
        extern int ControllerPopState(void);
        extern void ControllerResetState(void);
        while (ControllerPopState() != 0) {}
        ControllerResetState();
    }
#endif

    for (;;) {
        if (D_800ADB74 == 1) {
            FieldClearAndSwapOTagInternal();
            func_80075910();
            func_800A732C(6);
        } else if (D_800ADB74 < 2) {
            if (D_800ADB74 == 0) {
                DrawSync(0);
                Vsync(0);
                PutDispEnv(&g_FieldCurRenderContext->dispEnv);
                PutDrawEnv(&g_FieldCurRenderContext->drawEnvs[0]);
                func_800A732C(3);
                DrawSync(0);
                Vsync(0);
                PutDispEnv(&g_FieldCurRenderContext->dispEnv);
                PutDrawEnv(&g_FieldCurRenderContext->drawEnvs[0]);
                func_800A732C(3);
                func_800A7948();
            }
        } else if (D_800ADB74 == 2) {
            func_80077DAC();
            func_8007554C();
            func_800A732C(9);
        }

        if (g_FieldSystemMode != 0) {
            if (D_800ADB80 & 0x80) {
                FieldPollControllers();
                if (D_800C3900 & 0x20) {
#ifdef XENO_PC_PORT
                    printf("[xeno-port][fe60] skip Circle frame=%d end=%d\n",
                           (int)D_800B06A0, (int)D_800C3A2E);
                    fflush(stdout);
#endif
                    SoundSetCdVolumeWithFade(0, 10);
                    for (i = 0; i < 5; i++) {
                        Vsync(0);
                    }
                    break;
                }
            }
        } else if (D_800ADB74 == 2) {
            if ((D_800C3900 & 0x80) != 0 || D_800ADB84 != 0) {
                break;
            }
        } else {
            FieldPollControllers();
            if (D_800C3900 & 0x20) {
#ifdef XENO_PC_PORT
                printf("[xeno-port][fe60] skip Circle frame=%d end=%d\n",
                       (int)D_800B06A0, (int)D_800C3A2E);
                fflush(stdout);
#endif
                break;
            }
        }

        if (D_800ADB74 == 2 && D_800ADB84 != 0) {
            break;
        }
        if (D_800C3A3A != 0) {
            continue;
        }
        if (D_800B06A0 < (s32)(u16)D_800C3A2E) {
            continue;
        }
        break;
    }
#ifdef XENO_PC_PORT
    printf("[xeno-port][fe60] exit frame=%d end=%d\n",
           (int)D_800B06A0, (int)D_800C3A2E);
    fflush(stdout);
#endif

    Vsync(0);
    DrawSync(0);
    func_801D43B0();
    FieldRenderSyncAndFlush();
    PutDispEnv(&g_FieldCurRenderContext->dispEnv);
    PutDrawEnv(&g_FieldCurRenderContext->drawEnvs[0]);
    Vsync(0);
    DrawSync(0);
    HeapFree(pTransition);
    HeapConsolidate();
    func_800A74F8();
    func_80077884();

    moveY = D_800ADB78 << 8;
    rect.x = 0;
    rect.y = (s16)moveY;
    rect.w = 0x1E0;
    rect.h = 0xE0;
#ifdef FIELD_A7C58_MUTANT_MOVE_DST0
    MoveImage(&rect, 0, 0);
#else
    MoveImage(&rect, 0, moveY);
#endif
    DrawSync(0);
    Vsync(0);

    g_FieldCurRenderContext = &g_FieldRenderContexts[1];
    PutDispEnv(&g_FieldCurRenderContext->dispEnv);
    PutDrawEnv(&g_FieldCurRenderContext->drawEnvs[0]);
    if (D_800ADB74 != 2) {
        FieldImageConvert24BitTo15Bit();
    }
    Vsync(0);

    g_FieldRenderContexts[0].dispEnv.isrgb24 = 0;
    g_FieldCurRenderContext = &g_FieldRenderContexts[0];
    PutDispEnv(&g_FieldCurRenderContext->dispEnv);
    PutDrawEnv(&g_FieldCurRenderContext->drawEnvs[0]);
    rect.x = 0;
    rect.y = 0x100;
    rect.w = 0x140;
    rect.h = 0xE0;
    MoveImage(&rect, 0, 0);
    DrawSync(0);
    Vsync(0);

    g_FieldRenderContexts[1].dispEnv.isrgb24 = 0;
    g_FieldCurRenderContext = &g_FieldRenderContexts[1];
    PutDispEnv(&g_FieldCurRenderContext->dispEnv);
    PutDrawEnv(&g_FieldCurRenderContext->drawEnvs[0]);
    if (D_800AFE84 != 0) {
        D_800ADB50 = 1;
    } else {
        D_800ADB50 = 0;
    }
    func_80077AB4();

    if (D_800ADB74 != 2) {
        ArchiveSetIndex(4, 0);
        HeapChangeCurrentUser(8, NULL);
        D_800ADB60 = 0;
        func_80070488();
        func_80070508();
        D_800ADB3C = 0x20;
        D_800ADB38 = 1;
    }
    func_80085738();
    D_800C3A38 = 0xFF;
    D_800ADB74 = 0;
    D_800ADB6C = -1;
}
#endif /* XENO_PC_PORT */

void func_800A8314(void) {
    u8* pBuf;
    HeapChangeCurrentUser(8, NULL);
    ArchiveSetIndex(4, 0);
    pBuf = HeapAlloc(ArchiveDecodeAlignedSize(0xAA), 1);
    ArchiveReadFileToBuffer(0xAA, pBuf, 0, 0x80);
    ArchiveCdDataSync(0);
    FieldLoadTIMWithClut(pBuf, 0x380, 0, 0, 0xE8, 0, 0);
    DrawSync(0);
    HeapFree(pBuf);
}

extern s32 D_800AF278;
/* These remain packed four-byte pointer slots, adjacent to D_800AFC68.
 * A host-width load merges two unrelated retail words on LP64. */
extern u32 D_800AFC60;
extern u32 D_800AFC64;

void func_800A83B4(void) {
    if (D_800AF278 != 0) {
        D_800AF278 = 0;
        DrawSync(0);
        HeapFree((void*)(uintptr_t)D_800AFC60);
        HeapFree((void*)(uintptr_t)D_800AFC64);
    }
}

extern u16 D_800AEF14[];
extern u16 D_800AEB68[];
extern u16 D_800AEB6A[];
extern u16 D_800AEB6C[];
extern u16 D_800AEB6E[];

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/field/nonmatchings/main/misc5", func_800A8408);
#else
void func_800A8408(s32 index, s32 xOfs, s32 yOfs) {
    s32 texIdx = D_800AEF14[index * 4];
    s32 x = D_800AEB68[texIdx * 4] + xOfs;
    s32 y = D_800AEB6A[texIdx * 4] + yOfs;
    s32 w = D_800AEB6C[texIdx * 4];
    s32 h = D_800AEB6E[texIdx * 4];
    /* Retail selects the current packed buffer slot and uses vertically
     * reversed endpoints; it does not shrink both axes by one texel. */
#ifdef XENO_PC_PORT
    extern u8 g_FieldBss_800AFC60[];
    u32 buffer = *(u32*)(g_FieldBss_800AFC60 + g_FieldCurRenderContextIndex * 4);
#else
    u32 buffer = *(u32*)((u8*)&D_800AFC60 + g_FieldCurRenderContextIndex * 4);
#endif
    u8* pPoly = (u8*)(uintptr_t)buffer + index * 40;
    s32 x2 = x + w;
    s32 y2 = y + h - 1;
    y -= 1;
    FieldClampPolyFT4UVs(pPoly, x, y2, x2, y2, x, y, x2, y);
}
#endif /* XENO_PC_PORT */

/* Transcribed from asm/field/nonmatchings/main/misc5/func_800A84C0.s
 * (0x800A84C0-0x800A8BA0). Early-out if D_800AF278==0. Pitch is
 * ratan2(FieldGetVec2Magnitude(at.x-eye.x, at.z-eye.z), at.y-eye.y)&0xFFF;
 * yaw is g_CamInterpolation.curAngleY&0xFFF. Range loops 0..15 / 16..28 /
 * 29..33 / 34..38 / 39 / 40..41 / 42 (skip OT if AEB60&0x10) / 43..108
 * all insert into context+0x80E4 (ot3[4]). Frame 0x38. */
extern VECTOR g_CameraEye;
extern VECTOR g_CameraAt;
extern long FieldGetVec2Magnitude(long x, long y);
extern s32 D_800AEB60;
extern s32 D_800AEB64;

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/field/nonmatchings/main/misc5", func_800A84C0);
#else
void func_800A84C0(void) {
    s32 yaw;
    s32 pitch;
    s32 i;
    u32 buffer;
    u32* packet;
    u32* ot;

    if (D_800AF278 == 0) {
        return;
    }

    /* Retail subu+sra: wrap the 32-bit difference, then arithmetic >> 16. */
    pitch = ratan2(
                FieldGetVec2Magnitude(
                    (s32)((u32)g_CameraAt.vx - (u32)g_CameraEye.vx) >> 16,
                    (s32)((u32)g_CameraAt.vz - (u32)g_CameraEye.vz) >> 16),
                (s32)((u32)g_CameraAt.vy - (u32)g_CameraEye.vy) >> 16)
            & 0xFFF;
    yaw = (u16)g_CamInterpolation.curAngleY & 0xFFF;

    for (i = 0; i < 109; ++i) {
        if (i < 16) {
            func_800A8408(i, (yaw >> 4) & 15, 0);
        } else if (i < 29) {
            func_800A8408(i, 0, (pitch >> 4) & 15);
        } else if (i < 34) {
            func_800A8408(i, (yaw & 15) << 3, 0);
            yaw >>= 2;
        } else if (i < 39) {
            func_800A8408(i, (pitch & 15) << 3, 0);
            pitch >>= 2;
        } else if (i == 39) {
            if (((u32)D_800AEB60 & 15) == 0) {
                D_800AEB64 = (s32)((u32)D_800AEB64 + 1u);
            }
            if (D_800AEB64 >= 3) {
                D_800AEB64 = 0;
            }
            func_800A8408(i, 0, (s32)((u32)D_800AEB64 << 3));
        } else if (i < 42) {
            func_800A8408(i, (D_800AEB60 >> 2) & 15, 0);
        } else if (i == 42 && ((u32)D_800AEB60 & 0x10)) {
            continue;
        }
#ifdef XENO_PC_PORT
        {
            extern u8 g_FieldBss_800AFC60[];
            buffer = *(u32*)(g_FieldBss_800AFC60 + g_FieldCurRenderContextIndex * 4);
        }
#else
        buffer = *(u32*)((u8*)&D_800AFC60 + g_FieldCurRenderContextIndex * 4);
#endif
        packet = (u32*)((u8*)(uintptr_t)buffer + i * 40);
        ot = (u32*)((u8*)g_FieldCurRenderContext + 0x80E4);
        *packet = (*packet & 0xff000000u) | (*ot & 0xffffffu);
        *ot = (*ot & 0xff000000u) | ((u32)(uintptr_t)packet & 0xffffffu);
    }
    D_800AEB60 = (s32)((u32)D_800AEB60 + 1u);
}
#endif /* XENO_PC_PORT */

/* Transcribed from asm/field/nonmatchings/main/misc5/func_800A8BA4.s
 * (0x800A8BA4-0x800A8EA8). HeapAlloc two 0x1108 buffers into D_800AFC60/64,
 * 109 POLY_FT4s from D_800AEF10/14/16 and D_800AEB68..6E. High-nibble 0/1
 * set blend 1/2; other nibbles keep the previous selector. UV clamp only
 * when (flags&15)<=3. 10-word copy into the second buffer. Frame 0x50. */
extern u16 D_800AEF10[];
extern u16 D_800AEF16[];

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/field/nonmatchings/main/misc5", func_800A8BA4);
#else
void func_800A8BA4(void) {
    s32 i;
    s32 blend = 0;
    func_800A8314();
    D_800AEB64 = 0;
    HeapChangeCurrentUser(8, NULL);
    D_800AFC60 = (u32)(uintptr_t)HeapAlloc(0x1108, 0);
    D_800AFC64 = (u32)(uintptr_t)HeapAlloc(0x1108, 0);
    for (i = 0; i < 109; ++i) {
        POLY_FT4* poly = (POLY_FT4*)((u8*)(uintptr_t)D_800AFC60 + i * 40);
        u8* copy = (u8*)(uintptr_t)D_800AFC64 + i * 40;
        s32 flags, texture, x, y, w, h, u, v, u0, v0, u1, v1;
        s32 j;
        SetPolyFT4(poly);
        poly->r0 = poly->g0 = poly->b0 = 0x80;
        poly->clut = GetClut(0, 0xe8);
        flags = D_800AEF16[i * 4];
        /* Other high-nibble values preserve the previous blend selector. */
        if (((flags >> 4) & 15) == 0) blend = 1;
        else if (((flags >> 4) & 15) == 1) blend = 2;
        poly->tpage = GetTPage(0, blend, 0x380, 0);
        SetSemiTrans(poly, 1);
        texture = D_800AEF14[i * 4];
        x = D_800AEF10[i * 4];
        y = D_800AEF10[i * 4 + 1];
        u = D_800AEB68[texture * 4];
        v = D_800AEB6A[texture * 4];
        w = D_800AEB6C[texture * 4];
        h = D_800AEB6E[texture * 4];
        poly->x0 = poly->x2 = x;
        poly->y0 = poly->y1 = y;
        poly->x1 = poly->x3 = x + w;
        poly->y2 = poly->y3 = y + h;
        if ((flags & 15) <= 3) {
            u0 = (flags & 1) ? u + w - 1 : u;
            u1 = (flags & 1) ? u - 1 : u + w;
            v0 = (flags & 2) ? v + h - 1 : v;
            v1 = (flags & 2) ? v - 1 : v + h;
            FieldClampPolyFT4UVs(poly, u0, v0, u1, v0, u0, v1, u1, v1);
        }
        for (j = 0; j < 10; ++j) ((u32*)copy)[j] = ((u32*)poly)[j];
    }
    D_800AF278 = 1;
}
#endif /* XENO_PC_PORT */

extern u16 D_800AF27C[];
extern u16 D_800AF27E[];
extern u16 D_800AF280[];
extern u16 D_800AF282[];
extern u16 D_800AF284[];
extern u16 D_800AF286[];
extern u16 D_800AF288[];
extern u16 D_800AF28A[];
extern u16 D_800AF28C[];
extern u16 D_800AF28E[];
extern u16 D_800AF290[];
extern u16 D_800AF292[];

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/field/nonmatchings/main/misc5", FieldInitializeParticlePrimitive);
#else
void FieldInitializeParticlePrimitive(void* pParticle, s32 index, s32 abr) {
    POLY_FT4* poly = (POLY_FT4*)((u8*)pParticle + 0x50);
    s32 stride = index * 24;
    s32 u0, v0, u1, v1;
    s32 tu0, tv0, tu1, tv1;
    s32 a, b, c, d;
    void* dst;

    SetPolyFT4(poly);

    u0 = *(u16*)((u8*)D_800AF27C + stride);
    v0 = *(u16*)((u8*)D_800AF27E + stride);
    u1 = *(u16*)((u8*)D_800AF280 + stride);
    v1 = *(u16*)((u8*)D_800AF282 + stride);

    *(s16*)((u8*)pParticle + 0xA4) = 0;
    *(s16*)((u8*)pParticle + 0xAC) = 0;
    *(s16*)((u8*)pParticle + 0xB4) = 0;
    *(s16*)((u8*)pParticle + 0xBC) = 0;
    ((u8*)pParticle)[0x54] = 0x80;
    ((u8*)pParticle)[0x55] = 0x80;
    ((u8*)pParticle)[0x56] = 0x80;

    /* The retail SLL/SUBU result may be negative. Multiplication preserves
     * the value without left-shifting a negative signed C operand. */
    a = (u1 - u0) * 16;
    b = (v1 - v0) * 16;
    c = (u0 + u1) << 4;
    d = (v0 + v1) << 4;

    *(s16*)((u8*)pParticle + 0xA0) = a;
    *(s16*)((u8*)pParticle + 0xA2) = b;
    *(s16*)((u8*)pParticle + 0xA8) = c;
    *(s16*)((u8*)pParticle + 0xAA) = b;
    *(s16*)((u8*)pParticle + 0xB0) = a;
    *(s16*)((u8*)pParticle + 0xB2) = d;
    *(s16*)((u8*)pParticle + 0xB8) = c;
    *(s16*)((u8*)pParticle + 0xBA) = d;

    tu0 = *(u16*)((u8*)D_800AF284 + stride);
    tv0 = *(u16*)((u8*)D_800AF286 + stride);
    tu1 = *(u16*)((u8*)D_800AF288 + stride);
    tv1 = *(u16*)((u8*)D_800AF28A + stride);

    {
        s32 s0 = *(u16*)((u8*)D_800AF28C + stride);
        s32 s1 = *(u16*)((u8*)D_800AF28E + stride) + 0x3F;
        s32 s2 = *(u16*)((u8*)D_800AF290 + stride) - 1;
        s32 s3 = *(u16*)((u8*)D_800AF292 + stride) + 0x3F;

        FieldClampPolyFT4UVs(poly, tu0, tv0 + 0x40, tu1 - 1, tv1 + 0x40,
                            s0, s1, s2, s3);
    }

    SetSemiTrans(poly, 1);
    *(u16*)((u8*)pParticle + 0x66) = GetTPage(0, abr, 0x3C0, 0x140);
    *(u16*)((u8*)pParticle + 0x5E) = GetClut(0x100, 0xF7);

    /* Retail 800A9058..800A9090 copies exactly 0x28 bytes: two 16-byte
     * iterations and two final words. The vertices begin at +0xA0. */
    dst = (u8*)pParticle + 0x78;
    {
        u32* src = (u32*)poly;
        u32* d = (u32*)dst;
        s32 i;
        for (i = 0; i < 2; i++) {
            d[i * 4 + 0] = src[i * 4 + 0];
            d[i * 4 + 1] = src[i * 4 + 1];
            d[i * 4 + 2] = src[i * 4 + 2];
            d[i * 4 + 3] = src[i * 4 + 3];
        }
        d[8] = src[8];
        d[9] = src[9];
    }
}
#endif /* XENO_PC_PORT */

extern s32 D_800ADB34;
extern u32 D_800AFC70;

// Re-pin the 0x8000-byte persistent buffer under a new heap user (only while
// D_800ADB34 marks it active, e.g. a pause-menu background snapshot): alloc
// a fresh block under HEAP_USER_YOSI, copy the old contents over, free the old.
#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/field/nonmatchings/main/misc5", func_800A90B4);
#else
void func_800A90B4(s32 pinFlag) {
    void* pNew;

    if (D_800ADB34 == 1) {
        /* Retail copies the 0x8000-byte block 16 bytes at a time - four loads
         * and four stores per iteration, walking to src+0x8000 - so the source
         * copied 4-word blocks rather than calling memcpy or copying single
         * words (a word loop stays un-unrolled and a memcpy is a call). */
        typedef struct { u32 w[4]; } Block16;
        Block16* pSrc;
        Block16* pDst;
        Block16* pEnd;

        HeapChangeCurrentUser(HEAP_USER_YOSI, NULL);
        pNew = HeapAlloc(0x8000, pinFlag);
        pDst = (Block16*)pNew;
        pSrc = (Block16*)(uintptr_t)D_800AFC70;
        pEnd = (Block16*)((u8*)pSrc + 0x8000);
        do {
            *pDst++ = *pSrc++;
        } while (pSrc != pEnd);
        HeapFree((void*)(uintptr_t)D_800AFC70);
        D_800AFC70 = (u32)(uintptr_t)pNew;
    }
}
#endif /* XENO_PC_PORT */

/* Matches, but g_FieldStoredImageDest seems to be part of some struct which
 * needs recovery first. */

extern int D_800ADB34;
extern RECT g_FieldStoredImageDest[];
extern u32 D_800AFC70;

void func_800A915C(void) {
    if (D_800ADB34 != 1) {
        D_800ADB34 = 1;
        HeapChangeCurrentUser(HEAP_USER_YOSI, NULL);
        D_800AFC70 = (u32)(uintptr_t)HeapAlloc(0x8000, 0x1);
        setRECT(&g_FieldStoredImageDest[0], 0x3C0, 0x100, 0x40, 0x100);
        StoreImage(&g_FieldStoredImageDest[0], (u_long*)(uintptr_t)D_800AFC70);
        DrawSync(0);
    }
}


void func_800A91F0(void) {
    if (D_800ADB34) {
        D_800ADB34 = 0;
        setRECT(&g_FieldStoredImageDest[0], 0x3C0, 0x100, 0x40, 0x100);
        LoadImage(&g_FieldStoredImageDest[0], (u_long*)(uintptr_t)D_800AFC70);
        DrawSync(0);
        HeapFree((void*)(uintptr_t)D_800AFC70);
    }
}
