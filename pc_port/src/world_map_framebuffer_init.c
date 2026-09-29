/*
 * W34B4B: World-map framebuffer/GTE initializer 0x80072BB0.
 *
 * Exact transcription of retail 0x80072BB0–0x80072DB0.
 * Initializes two complementary 320×216 draw/display environments for
 * PSX double-buffering, GTE screen distance, back/far color, and fog.
 *
 * Called from mode_update_init at retail 0x80072244.
 *
 * Both the production game build and the production-linked test link
 * this same object.  Do NOT duplicate this function elsewhere.
 */
#include <stdio.h>

#include "common.h"
#include "psx_memory.h"
#include "psyq/libgpu.h"
#include "psyq/libgte.h"
#include "world_map_framebuffer_init.h"

/* ---- Retail absolute addresses (signed-immediate derivation) ---- */

/* Environment record base: lui 0x800A + addiu -17464 = 0x8009BBC8.
 * Record 0 draw env at +0x00, display env at +0x5C.
 * Record 1 draw env at +0x78, display env at +0xD4. */
#define WM_ENVREC_BASE       0x8009BBC8u
#define WM_ENVREC_STRIDE     0x78u
#define WM_DRAWENV_SIZE      0x5Cu  /* sizeof(DRAWENV) */
#define WM_DISPENV_OFFSET    0x5Cu  /* display env within record */

/* Draw env record 0: 0x8009BBC8
 * Draw env record 1: 0x8009BC40 (BBC8 + 78)
 * Display env 0:     0x8009BC24 (BBC8 + 5C)
 * Display env 1:     0x8009BC9C (BC40 + 5C) */

/* GTE screen distance (word). */
#define WM_BCDC_ABS          0x8009BCDCu

/* Phase/entrance mode (word). Compared against 2 for background color. */
#define WM_D7CC_ABS          0x8009D7CCu

/* Fog color bytes (3 consecutive bytes). */
#define WM_FOG_R_ABS         0x8009BB48u
#define WM_FOG_G_ABS         0x8009BB49u
#define WM_FOG_B_ABS         0x8009BB4Au

/* Draw-env dirty/isbg flags (bytes within DRAWENV). */
#define WM_DRAW0_ISDIRTY     0x8009BBE0u  /* record0 + 0x18: isbg */
#define WM_DRAW0_DFE         0x8009BBDEu  /* record0 + 0x16: dfe */
#define WM_DRAW1_ISDIRTY     0x8009BC58u  /* record1 + 0x18: isbg */
#define WM_DRAW1_DFE         0x8009BC56u  /* record1 + 0x16: dfe */

/* Draw-env background RGB (bytes within DRAWENV).
 * record0: r0=+0x19, g0=+0x1A, b0=+0x1B
 * record1: r0=+0x91, g0=+0x92, b0=+0x93 */
#define WM_DRAW0_BG_R        0x8009BBE1u
#define WM_DRAW0_BG_G        0x8009BBE2u
#define WM_DRAW0_BG_B        0x8009BBE3u
#define WM_DRAW1_BG_R        0x8009BC59u
#define WM_DRAW1_BG_G        0x8009BC5Au
#define WM_DRAW1_BG_B        0x8009BC5Bu

/* Display-env screen RECT fields (halfwords within DISPENV).
 * record0: x=+0x64, y=+0x66, w=+0x68, h=+0x6A
 * record1: x=+0xDC, y=+0xDE, w=+0xE0, h=+0xE2
 * But offsets relative to record base:
 *   disp0.x = BBC8 + 5C + 08 = BC2C
 *   disp0.y = BBC8 + 5C + 0A = BC2E
 *   disp0.w = BBC8 + 5C + 0C = BC30
 *   disp0.h = BBC8 + 5C + 0E = BC32
 *   disp1.x = BC40 + 5C + 08 = BCA4
 *   disp1.y = BC40 + 5C + 0A = BCA6
 *   disp1.w = BC40 + 5C + 0C = BCA8
 *   disp1.h = BC40 + 5C + 0E = BCAA */
#define WM_DISP0_SCR_X       0x8009BC2Cu
#define WM_DISP0_SCR_Y       0x8009BC2Eu
#define WM_DISP0_SCR_W       0x8009BC30u
#define WM_DISP0_SCR_H       0x8009BC32u
#define WM_DISP1_SCR_X       0x8009BCA4u
#define WM_DISP1_SCR_Y       0x8009BCA6u
#define WM_DISP1_SCR_W       0x8009BCA8u
#define WM_DISP1_SCR_H       0x8009BCAAu

/* Helper to set display-env screen RECT halfwords via PSX memory.
 * Retail stores at disp-env-relative offsets +8/+A/+C/+E. */
#define WM_H16(a) (*(u16*)PSX_ADDR(a))
#define WM_U32(a) (*(u32*)PSX_ADDR(a))
#define WM_U8(a)  (*(u8*)PSX_ADDR(a))

/* func_8002C6E0: stores 3 model-base-color bytes to globals.
 * Retail at 0x8002C6E0; defined in temp2.c. */
extern void func_8002C6E0(u8 a0, u8 a1, u8 a2);

/* func_80048AB0: PsyQ SetFogNearFar wrapper.
 * Retail at 0x80048AB0; defined in psyq_compat.c. */
extern void func_80048AB0(long a, long b, long h);

/* Instrumentation. */
static int s_wm_fbi_calls;

int wm_fbi_get_calls(void) { return s_wm_fbi_calls; }

void wm_fbi_reset(void) { s_wm_fbi_calls = 0; }

/* ---- Production function ---- */

/* W34B4B: framebuffer/GTE initializer.
 * Exact transcription of retail 0x80072BB0–0x80072DB0. */
void wm_80072BB0(void)
{
    u32 mode;

    s_wm_fbi_calls++;
    fprintf(stderr, "[worldmap-fbi] entry (call %d)\n", s_wm_fbi_calls);

    /* 0x80072BC0: ResetGraph(1) */
    ResetGraph(1);

    /* 0x80072BCC: store 256 at BCDC (GTE screen distance). */
    WM_U32(WM_BCDC_ABS) = 256;

    /* 0x80072BD4: SetGeomScreen(256) */
    SetGeomScreen(256);

    /* 0x80072BF8: SetDefDrawEnv(record0, 0, 0, 320, 216)
     * Draw env 0: renders to top half of VRAM (y=0). */
    SetDefDrawEnv((DRAWENV*)PSX_ADDR(WM_ENVREC_BASE), 0, 0, 320, 216);

    /* 0x80072C10: SetDefDrawEnv(record1, 0, 216, 320, 216)
     * Draw env 1: renders to bottom half of VRAM (y=216). */
    SetDefDrawEnv((DRAWENV*)PSX_ADDR(WM_ENVREC_BASE + WM_ENVREC_STRIDE),
                  0, 216, 320, 216);

    /* 0x80072C28: SetDefDispEnv(record0_disp, 0, 216, 320, 216)
     * Display env 0: displays bottom half (complementary to draw 0). */
    SetDefDispEnv((DISPENV*)PSX_ADDR(WM_ENVREC_BASE + WM_DISPENV_OFFSET),
                  0, 216, 320, 216);

    /* 0x80072C40: SetDefDispEnv(record1_disp, 0, 0, 320, 216)
     * Display env 1: displays top half (complementary to draw 1). */
    SetDefDispEnv((DISPENV*)PSX_ADDR(WM_ENVREC_BASE + WM_ENVREC_STRIDE +
                                     WM_DISPENV_OFFSET),
                  0, 0, 320, 216);

    /* 0x80072C48: load mode from D7CC. */
    mode = WM_U32(WM_D7CC_ABS);

    /* 0x80072C50–0x80072C70: set dirty/flags.
     * Draw env 0: isbg=1 (0xBBE0), dfe=1 (0xBBDE).
     * Draw env 1: isbg=1 (0xBC58), dfe=1 (0xBC56). */
    WM_U8(WM_DRAW1_ISDIRTY) = 1;
    WM_U8(WM_DRAW0_ISDIRTY) = 1;
    WM_U8(WM_DRAW1_DFE) = 1;
    WM_U8(WM_DRAW0_DFE) = 1;

    /* 0x80072C78: bne v1(D7CC), v0(2), 0x80072CB8
     * Mode-dependent background color.
     * mode == 2: background = (0, 0, 0) — black.
     * mode != 2: background = (0, 0, 0x70) — dark blue. */
    if (mode == 2) {
        /* 0x80072C80–0x80072CAC: zero background RGB for both draw envs. */
        WM_U8(WM_DRAW0_BG_R) = 0;
        WM_U8(WM_DRAW0_BG_G) = 0;
        WM_U8(WM_DRAW0_BG_B) = 0;
        WM_U8(WM_DRAW1_BG_R) = 0;
        WM_U8(WM_DRAW1_BG_G) = 0;
        WM_U8(WM_DRAW1_BG_B) = 0;
    } else {
        /* 0x80072CB8–0x80072CE8: non-mode-2 background. */
        WM_U8(WM_DRAW0_BG_R) = 0;
        WM_U8(WM_DRAW0_BG_G) = 0;
        WM_U8(WM_DRAW0_BG_B) = 112; /* 0x70 */
        WM_U8(WM_DRAW1_BG_R) = 0;
        WM_U8(WM_DRAW1_BG_G) = 0;
        WM_U8(WM_DRAW1_BG_B) = 112;
    }

    /* 0x80072CF0–0x80072D38: display-env screen RECT fields.
     * Both display envs: x=0, y=10, w=256, h=216. */
    WM_H16(WM_DISP1_SCR_Y) = 10;
    WM_H16(WM_DISP0_SCR_Y) = 10;
    WM_H16(WM_DISP1_SCR_W) = 256;
    WM_H16(WM_DISP0_SCR_W) = 256;
    WM_H16(WM_DISP1_SCR_X) = 0;
    WM_H16(WM_DISP0_SCR_X) = 0;
    WM_H16(WM_DISP1_SCR_H) = 216;
    WM_H16(WM_DISP0_SCR_H) = 216;

    /* 0x80072D3C: func_8002C6E0(128, 128, 128)
     * Model base color bytes (D_80059598/99/9A). */
    func_8002C6E0(128, 128, 128);

    /* 0x80072D4C: SetBackColor(128, 128, 128) */
    SetBackColor(128, 128, 128);

    /* 0x80072D6C: SetFarColor(fog_r, fog_g, fog_b)
     * Retail loads from 0x8009BB48–BB4A (fog color bytes). */
    SetFarColor(WM_U8(WM_FOG_R_ABS), WM_U8(WM_FOG_G_ABS),
                WM_U8(WM_FOG_B_ABS));

    /* 0x80072D80–0x80072D94: fog near/far.
     * mode == 2: near = 0x800 (2048); else near = 0xB00 (2816).
     * far = 0xE80 (3712), H = *(BCDC) = 256. */
    if (mode == 2) {
        func_80048AB0(2048, 3712, 256);
    } else {
        func_80048AB0(2816, 3712, 256);
    }

    fprintf(stderr, "[worldmap-fbi] exit mode=%u\n", mode);
}
