/* SPDX-License-Identifier: MIT */
#include "field/main.h"
#include "main/movie_player.h"
#include "psyq/libetc.h"
#include "psyq/libgpu.h"

extern s32 D_8004F300, D_800AFE74, D_800B06A0, D_801E89E0;
extern void GameCheckAndHandleSoftReset(void);
extern void FieldClearAndSwapOTagInternal(void);
extern void func_800ACB90(void);
extern void func_800AC99C(void);
extern void func_800ACCF4(void);
extern void func_800ACCB0(void);
extern void func_800A732C(s32);

static void PcPort_SetFieldMovieWidth(int width, int restore)
{
    for (int i = 0; i < 2; ++i) {
        RenderContext *ctx = &g_FieldRenderContexts[i];
        DRAWENV *draw = &ctx->drawEnvs[0];
        DISPENV *display = &ctx->dispEnv;
        int draw_height = draw->clip.h > 0 ? draw->clip.h : 240;
        int display_height = display->disp.h > 0 ? display->disp.h : draw_height;
        SetDefDrawEnv(draw, draw->clip.x, draw->clip.y, width, draw_height);
        SetDefDispEnv(display, display->disp.x, display->disp.y, width, display_height);
        display->isrgb24 = restore;
        display->isinter = 0;
    }
}

void func_800A7948(void)
{
    enum { first_sector = 0x687, end_sector = 0x18e2, sectors_per_tick = 5 };
    if (!D_8004F300 || D_800B06A0 < first_sector || D_800B06A0 >= end_sector ||
        MoviePlayerStartFailed())
        return;

    RECT full_frame = {0, 0, 1280, 512};
    RECT side_region = {768, 0, 256, 512};
    D_800AFE74 = 1;
    DrawSync(0);
    ClearImage(&full_frame, 0, 0, 0);
    PcPort_SetFieldMovieWidth(640, 0);
    func_800ACB90();
    func_800AC99C();
    while (D_800B06A0 < end_sector && !MoviePlayerStartFailed()) {
        GameCheckAndHandleSoftReset();
        FieldClearAndSwapOTagInternal();
        func_800ACCF4();
        DrawSync(0);
        Vsync(0);
        RenderContext *current = g_FieldCurRenderContext;
        if (current) {
            PutDispEnv(&current->dispEnv);
            PutDrawEnv(&current->drawEnvs[0]);
            DrawOTag(FIELD_CTX_OT3_HEAD(current));
        }
        func_800A732C(sectors_per_tick);
    }
    func_800ACCB0();
    DrawSync(0);
    ClearImage(&side_region, 0, 0, 0);
    PcPort_SetFieldMovieWidth(320, 1);
    if (g_FieldCurRenderContext) {
        PutDispEnv(&g_FieldCurRenderContext->dispEnv);
        PutDrawEnv(&g_FieldCurRenderContext->drawEnvs[0]);
    }
    D_800AFE74 = 0;
    D_801E89E0 = 1;
}
