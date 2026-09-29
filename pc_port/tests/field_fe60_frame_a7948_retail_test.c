/*
 * Retail certificate for func_800A7948 (FE60 wide-frame pump).
 *
 * Retail (func_800A7948.s 0x800A7948-0x800A7C54): D_8004F300 and D_800B06A0
 * in [0x687,0x18E2). SetDefDrawEnv/DispEnv 0x280 on contexts 0/1, loop while
 * D_800B06A0 < 0x18E2 (check first), restore 0x140 and isrgb24=1.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "psyq/libgpu.h"
#include "field/main.h"

extern void func_800A7948(void);

s32 D_8004F300;
s32 D_800AFE74;
s32 D_800B06A0;
s32 D_801E89E0;
s32 D_800B00E4;
RenderContext g_FieldRenderContexts[2];
RenderContext* g_FieldCurRenderContext;

static unsigned s_checks;
static int s_draw_count;
static int s_draw_w[8];
static DRAWENV* s_draw_env[8];
static int s_disp_count;
static int s_disp_w[8];
static int s_clear_count;
static RECT s_clear_rect[4];
static int s_acb90;
static int s_ac99c;
static int s_accf4;
static int s_accb0;
static int s_otag;
static int s_732c;
static int s_732c_arg;
static int s_soft;
static int s_swap;

DRAWENV* SetDefDrawEnv(DRAWENV* env, int x, int y, int w, int h)
{
    (void)x;
    (void)y;
    (void)h;
    if (s_draw_count < 8) {
        s_draw_env[s_draw_count] = env;
        s_draw_w[s_draw_count] = w;
    }
    s_draw_count++;
    env->clip.w = (short)w;
    env->clip.h = (short)h;
    env->clip.x = (short)x;
    env->clip.y = (short)y;
    return env;
}

DISPENV* SetDefDispEnv(DISPENV* env, int x, int y, int w, int h)
{
    (void)x;
    (void)y;
    (void)h;
    if (s_disp_count < 8) {
        s_disp_w[s_disp_count] = w;
    }
    s_disp_count++;
    env->disp.w = (short)w;
    return env;
}

int ClearImage(RECT* rect, u_char r, u_char g, u_char b)
{
    (void)r;
    (void)g;
    (void)b;
    if (s_clear_count < 4) {
        s_clear_rect[s_clear_count] = *rect;
    }
    s_clear_count++;
    return 0;
}

int DrawSync(int mode)
{
    (void)mode;
    return 0;
}

int Vsync(int mode)
{
    (void)mode;
    return 0;
}

DISPENV* PutDispEnv(DISPENV* env)
{
    return env;
}

DRAWENV* PutDrawEnv(DRAWENV* env)
{
    return env;
}

void DrawOTag(u_long* p)
{
    (void)p;
    s_otag++;
}

void GameCheckAndHandleSoftReset(void)
{
    s_soft++;
}

void FieldClearAndSwapOTagInternal(void)
{
    s_swap++;
}

void func_800ACB90(void)
{
    s_acb90++;
}

void func_800AC99C(void)
{
    s_ac99c++;
}

void func_800ACCF4(void)
{
    s_accf4++;
}

void func_800ACCB0(void)
{
    s_accb0++;
}

void func_800A732C(s32 count)
{
    s_732c++;
    s_732c_arg = count;
    D_800B06A0 = 0x18E2;
}

static void fail(const char* name, const char* detail)
{
    fprintf(stderr, "ASSERTION %s %s\n", name, detail);
    exit(1);
}

static void expect_eq_s32(const char* field, s32 actual, s32 expected)
{
    char detail[160];

    s_checks++;
    if (actual == expected) {
        return;
    }
    snprintf(detail, sizeof(detail), "field=%s actual=%d expected=%d",
             field, (int)actual, (int)expected);
    fail("fe60.frame", detail);
}

static void reset_state(void)
{
    memset(g_FieldRenderContexts, 0, sizeof(g_FieldRenderContexts));
    g_FieldCurRenderContext = &g_FieldRenderContexts[0];
    D_800AFE74 = 0;
    D_801E89E0 = 0xFF;
    s_draw_count = 0;
    s_disp_count = 0;
    s_clear_count = 0;
    s_acb90 = 0;
    s_ac99c = 0;
    s_accf4 = 0;
    s_accb0 = 0;
    s_otag = 0;
    s_732c = 0;
    s_soft = 0;
    s_swap = 0;
}

int main(void)
{
    /* Early-outs. */
    reset_state();
    D_8004F300 = 0;
    D_800B06A0 = 0x700;
    func_800A7948();
    expect_eq_s32("early.f300.draw", s_draw_count, 0);

    reset_state();
    D_8004F300 = 1;
    D_800B06A0 = 0x686;
    func_800A7948();
    expect_eq_s32("early.low.draw", s_draw_count, 0);

    reset_state();
    D_8004F300 = 1;
    D_800B06A0 = 0x18E2;
    func_800A7948();
    expect_eq_s32("early.high.draw", s_draw_count, 0);

    /* In-range: wide 0x280, one loop (732C forces 0x18E2), restore 0x140. */
    reset_state();
    D_8004F300 = 1;
    D_800B06A0 = 0x700;
    func_800A7948();
    expect_eq_s32("run.draw.count", s_draw_count, 4);
    expect_eq_s32("run.draw0.w", s_draw_w[0], 0x280);
    expect_eq_s32("run.draw1.w", s_draw_w[1], 0x280);
    expect_eq_s32("run.draw2.w", s_draw_w[2], 0x140);
    expect_eq_s32("run.draw3.w", s_draw_w[3], 0x140);
    expect_eq_s32("run.disp.count", s_disp_count, 4);
    expect_eq_s32("run.disp0.w", s_disp_w[0], 0x280);
    expect_eq_s32("run.disp2.w", s_disp_w[2], 0x140);
    expect_eq_s32("run.env0", (s32)(s_draw_env[0] == &g_FieldRenderContexts[0].drawEnvs[0]), 1);
    expect_eq_s32("run.env1", (s32)(s_draw_env[1] == &g_FieldRenderContexts[1].drawEnvs[0]), 1);
    expect_eq_s32("run.clear0.w", s_clear_rect[0].w, 0x500);
    expect_eq_s32("run.clear0.h", s_clear_rect[0].h, 0x200);
    expect_eq_s32("run.clear1.x", s_clear_rect[1].x, 0x300);
    expect_eq_s32("run.acb90", s_acb90, 1);
    expect_eq_s32("run.ac99c", s_ac99c, 1);
    expect_eq_s32("run.732c", s_732c, 1);
    expect_eq_s32("run.732c.arg", s_732c_arg, 5);
    expect_eq_s32("run.otag", s_otag, 1);
    expect_eq_s32("run.accb0", s_accb0, 1);
    expect_eq_s32("run.flag.busy", D_800AFE74, 0);
    expect_eq_s32("run.flag.end", D_801E89E0, 1);
    expect_eq_s32("run.isrgb24.0", g_FieldRenderContexts[0].dispEnv.isrgb24, 1);
    expect_eq_s32("run.isrgb24.1", g_FieldRenderContexts[1].dispEnv.isrgb24, 1);
    expect_eq_s32("run.isinter.0", g_FieldRenderContexts[0].dispEnv.isinter, 0);

    printf("FIELD FE60 FRAME A7948 certificate PASS checks=%u\n", s_checks);
    return 0;
}
