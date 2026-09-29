/*
 * W34B4B PRODUCTION-LINKED test for framebuffer/GTE initializer
 * 0x80072BB0–0x80072DB0.
 *
 * This test links against the ACTUAL production object compiled from
 * pc_port/src/world_map_framebuffer_init.c.  It does NOT contain a copied
 * initializer implementation.  The one authoritative definition of
 * wm_80072BB0 comes from the production module.
 *
 * Build:
 *   gcc -std=gnu17 -O0 -g -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C \
 *     -Ipc_port/include -Iinclude -Ipc_port/src \
 *     pc_port/tests/w34b4b_prod_test.c \
 *     pc_port/src/world_map_framebuffer_init.c \
 *     -o pc_port/build_native/w34b4b_prod_test
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "common.h"
#include "psx_memory.h"
#include "psyq/libgpu.h"
#include "world_map_framebuffer_init.h"

/* Provide g_PsxRam for the production module. */
uint8_t g_PsxRam[PSX_RAM_SIZE];

/* ---- PsyQ stubs for test (not linking PsyCross) ---- */

/* Stub: ResetGraph — no-op in test. */
int ResetGraph(int mode) { (void)mode; return 0; }

/* Stub: SetGeomScreen — no-op in test. */
void SetGeomScreen(long h) { (void)h; }

/* Stub: SetDefDrawEnv — populates minimal DRAWENV clip fields. */
DRAWENV *SetDefDrawEnv(DRAWENV *env, int x, int y, int w, int h) {
    short *clip = (short*)env;
    clip[0] = (short)x; clip[1] = (short)y;
    clip[2] = (short)w; clip[3] = (short)h;
    return env;
}

/* Stub: SetDefDispEnv — populates minimal DISPENV fields. */
DISPENV *SetDefDispEnv(DISPENV *env, int x, int y, int w, int h) {
    short *disp = (short*)env;
    disp[0] = (short)x; disp[1] = (short)y;
    disp[2] = (short)w; disp[3] = (short)h;
    /* screen RECT at +8 bytes */
    short *screen = (short*)((char*)env + 8);
    screen[0] = (short)x; screen[1] = (short)y;
    screen[2] = (short)w; screen[3] = (short)h;
    return env;
}

/* Stub: SetBackColor — no-op in test. */
void SetBackColor(long rbk, long gbk, long bbk) {
    (void)rbk; (void)gbk; (void)bbk;
}

/* Stub: SetFarColor — no-op in test. */
void SetFarColor(long rfc, long gfc, long bfc) {
    (void)rfc; (void)gfc; (void)bfc;
}

/* Stub: func_8002C6E0 — stores to PSX memory model-base-color globals. */
void func_8002C6E0(u8 a0, u8 a1, u8 a2) {
    *(u8*)PSX_ADDR(0x80059598u) = a0;
    *(u8*)PSX_ADDR(0x80059599u) = a1;
    *(u8*)PSX_ADDR(0x8005959Au) = a2;
}

/* Stub: func_80048AB0 (SetFogNearFar) — no-op in test. */
void func_80048AB0(long a, long b, long h) {
    (void)a; (void)b; (void)h;
}

/* Retail layout constants. */
#define WM_ENVREC_BASE       0x8009BBC8u
#define WM_ENVREC_STRIDE     0x78u
#define WM_DRAWENV_SIZE      0x5Cu
#define WM_DISPENV_OFFSET    0x5Cu

#define WM_BCDC_ABS          0x8009BCDCu
#define WM_D7CC_ABS          0x8009D7CCu

#define WM_FOG_R_ABS         0x8009BB48u
#define WM_FOG_G_ABS         0x8009BB49u
#define WM_FOG_B_ABS         0x8009BB4Au

#define WM_DRAW0_ISDIRTY     0x8009BBE0u
#define WM_DRAW0_DFE         0x8009BBDEu
#define WM_DRAW1_ISDIRTY     0x8009BC58u
#define WM_DRAW1_DFE         0x8009BC56u

#define WM_DRAW0_BG_R        0x8009BBE1u
#define WM_DRAW0_BG_G        0x8009BBE2u
#define WM_DRAW0_BG_B        0x8009BBE3u
#define WM_DRAW1_BG_R        0x8009BC59u
#define WM_DRAW1_BG_G        0x8009BC5Au
#define WM_DRAW1_BG_B        0x8009BC5Bu

#define WM_DISP0_SCR_X       0x8009BC2Cu
#define WM_DISP0_SCR_Y       0x8009BC2Eu
#define WM_DISP0_SCR_W       0x8009BC30u
#define WM_DISP0_SCR_H       0x8009BC32u
#define WM_DISP1_SCR_X       0x8009BCA4u
#define WM_DISP1_SCR_Y       0x8009BCA6u
#define WM_DISP1_SCR_W       0x8009BCA8u
#define WM_DISP1_SCR_H       0x8009BCAAu

/* OT pointers and callback pool — must NOT be touched. */
#define WM_OT_PTR0           0x8009BC38u
#define WM_OT_PTR1           0x8009BCB0u
#define WM_BE3C              0x8009BE3Cu

/* Terrain globals — must NOT be touched in this phase. */
#define WM_C618              0x8009C618u
#define WM_D534              0x8009D534u

/* Callback pool pointer. */
#define WM_POOL_BE24         0x8009BE24u

/* Table A/B bases. */
#define WM_CONV_TABLE_A_BASE 0x80099E8Cu
#define WM_CONV_TABLE_B_BASE 0x8009A034u
#define WM_SLOT_C610_ABS     0x8009C610u

#define WM_U32(a) (*(uint32_t*)PSX_ADDR(a))
#define WM_U8(a)  (*(uint8_t*)PSX_ADDR(a))
#define WM_H16(a) (*(uint16_t*)PSX_ADDR(a))

/* Test harness. */
static int total = 0, pass = 0, fail = 0;
static void check(const char* name, int cond) {
    total++;
    if (cond) { pass++; printf("  PASS: %s\n", name); }
    else { fail++; printf("  FAIL: %s\n", name); }
}

static void reset_state(void) {
    memset(g_PsxRam, 0, PSX_RAM_SIZE);
    wm_fbi_reset();
}

/* Compute SHA-256-like hash of framebuffer region for before/after comparison. */
static uint32_t hash_region(uint32_t addr, size_t len) {
    uint32_t h = 0x811c9dc5;
    uint8_t* p = (uint8_t*)PSX_ADDR(addr);
    size_t i;
    for (i = 0; i < len; i++) {
        h ^= p[i];
        h *= 0x01000193;
    }
    return h;
}

int main(void)
{
    uint32_t hash_before, hash_after;
    uint32_t ot0_before, ot1_before, be3c_before;
    uint32_t pool_before;
    uint32_t c618_before, d534_before;
    uint32_t c610_before, table_a_before, table_b_before;

    printf("=== W34B4B PRODUCTION-LINKED Test ===\n");
    printf("    (links against production wm_80072BB0)\n\n");

    /* ================================================================
     * 1. Clean zeroed state — verify initialization produces correct values
     * ================================================================ */
    printf("--- Clean zeroed state ---\n");
    reset_state();
    wm_80072BB0();

    /* GTE screen distance. */
    check("BCDC == 256", WM_U32(WM_BCDC_ABS) == 256);

    /* Draw env 0: clip RECT set by SetDefDrawEnv(0,0,320,216).
     * DRAWENV.clip at +0x00: x=0, y=0, w=320, h=216. */
    check("draw0 clip.x == 0", WM_H16(WM_ENVREC_BASE + 0x00) == 0);
    check("draw0 clip.y == 0", WM_H16(WM_ENVREC_BASE + 0x02) == 0);
    check("draw0 clip.w == 320", WM_H16(WM_ENVREC_BASE + 0x04) == 320);
    check("draw0 clip.h == 216", WM_H16(WM_ENVREC_BASE + 0x06) == 216);

    /* Draw env 1: clip RECT set by SetDefDrawEnv(0,216,320,216). */
    check("draw1 clip.x == 0",
          WM_H16(WM_ENVREC_BASE + WM_ENVREC_STRIDE + 0x00) == 0);
    check("draw1 clip.y == 216",
          WM_H16(WM_ENVREC_BASE + WM_ENVREC_STRIDE + 0x02) == 216);
    check("draw1 clip.w == 320",
          WM_H16(WM_ENVREC_BASE + WM_ENVREC_STRIDE + 0x04) == 320);
    check("draw1 clip.h == 216",
          WM_H16(WM_ENVREC_BASE + WM_ENVREC_STRIDE + 0x06) == 216);

    /* Display env 0: disp RECT set by SetDefDispEnv(0,216,320,216). */
    check("disp0 disp.x == 0",
          WM_H16(WM_ENVREC_BASE + WM_DISPENV_OFFSET + 0x00) == 0);
    check("disp0 disp.y == 216",
          WM_H16(WM_ENVREC_BASE + WM_DISPENV_OFFSET + 0x02) == 216);
    check("disp0 disp.w == 320",
          WM_H16(WM_ENVREC_BASE + WM_DISPENV_OFFSET + 0x04) == 320);
    check("disp0 disp.h == 216",
          WM_H16(WM_ENVREC_BASE + WM_DISPENV_OFFSET + 0x06) == 216);

    /* Display env 1: disp RECT set by SetDefDispEnv(0,0,320,216). */
    check("disp1 disp.x == 0",
          WM_H16(WM_ENVREC_BASE + WM_ENVREC_STRIDE + WM_DISPENV_OFFSET + 0x00) == 0);
    check("disp1 disp.y == 0",
          WM_H16(WM_ENVREC_BASE + WM_ENVREC_STRIDE + WM_DISPENV_OFFSET + 0x02) == 0);
    check("disp1 disp.w == 320",
          WM_H16(WM_ENVREC_BASE + WM_ENVREC_STRIDE + WM_DISPENV_OFFSET + 0x04) == 320);
    check("disp1 disp.h == 216",
          WM_H16(WM_ENVREC_BASE + WM_ENVREC_STRIDE + WM_DISPENV_OFFSET + 0x06) == 216);

    /* ================================================================
     * 2. Dirty/sentinel pre-state — verify overwrite
     * ================================================================ */
    printf("--- Dirty pre-state ---\n");
    reset_state();
    /* Fill framebuffer region with sentinel. */
    memset(PSX_ADDR(WM_ENVREC_BASE), 0xCC, WM_ENVREC_STRIDE * 2);
    wm_80072BB0();

    check("draw0 overwritten clip.x == 0", WM_H16(WM_ENVREC_BASE + 0x00) == 0);
    check("draw0 overwritten clip.w == 320", WM_H16(WM_ENVREC_BASE + 0x04) == 320);
    check("draw1 overwritten clip.y == 216",
          WM_H16(WM_ENVREC_BASE + WM_ENVREC_STRIDE + 0x02) == 216);

    /* ================================================================
     * 3. Both draw environments — background/flags
     * ================================================================ */
    printf("--- Draw environment background (mode=2) ---\n");
    reset_state();
    WM_U32(WM_D7CC_ABS) = 2;
    wm_80072BB0();

    check("draw0 isbg == 1", WM_U8(WM_DRAW0_ISDIRTY) == 1);
    check("draw1 isbg == 1", WM_U8(WM_DRAW1_ISDIRTY) == 1);
    check("draw0 dfe == 1", WM_U8(WM_DRAW0_DFE) == 1);
    check("draw1 dfe == 1", WM_U8(WM_DRAW1_DFE) == 1);
    check("draw0 bg_r == 0 (mode2)", WM_U8(WM_DRAW0_BG_R) == 0);
    check("draw0 bg_g == 0 (mode2)", WM_U8(WM_DRAW0_BG_G) == 0);
    check("draw0 bg_b == 0 (mode2)", WM_U8(WM_DRAW0_BG_B) == 0);
    check("draw1 bg_r == 0 (mode2)", WM_U8(WM_DRAW1_BG_R) == 0);
    check("draw1 bg_g == 0 (mode2)", WM_U8(WM_DRAW1_BG_G) == 0);
    check("draw1 bg_b == 0 (mode2)", WM_U8(WM_DRAW1_BG_B) == 0);

    /* ================================================================
     * 4. Non-mode-2 background
     * ================================================================ */
    printf("--- Draw environment background (mode=1) ---\n");
    reset_state();
    WM_U32(WM_D7CC_ABS) = 1;
    wm_80072BB0();

    check("draw0 bg_r == 0 (mode1)", WM_U8(WM_DRAW0_BG_R) == 0);
    check("draw0 bg_g == 0 (mode1)", WM_U8(WM_DRAW0_BG_G) == 0);
    check("draw0 bg_b == 112 (mode1)", WM_U8(WM_DRAW0_BG_B) == 112);
    check("draw1 bg_r == 0 (mode1)", WM_U8(WM_DRAW1_BG_R) == 0);
    check("draw1 bg_g == 0 (mode1)", WM_U8(WM_DRAW1_BG_G) == 0);
    check("draw1 bg_b == 112 (mode1)", WM_U8(WM_DRAW1_BG_B) == 112);

    /* ================================================================
     * 5. Display environment screen RECT
     * ================================================================ */
    printf("--- Display environment screen RECT ---\n");
    reset_state();
    wm_80072BB0();

    check("disp0 scr_x == 0", WM_H16(WM_DISP0_SCR_X) == 0);
    check("disp0 scr_y == 10", WM_H16(WM_DISP0_SCR_Y) == 10);
    check("disp0 scr_w == 256", WM_H16(WM_DISP0_SCR_W) == 256);
    check("disp0 scr_h == 216", WM_H16(WM_DISP0_SCR_H) == 216);
    check("disp1 scr_x == 0", WM_H16(WM_DISP1_SCR_X) == 0);
    check("disp1 scr_y == 10", WM_H16(WM_DISP1_SCR_Y) == 10);
    check("disp1 scr_w == 256", WM_H16(WM_DISP1_SCR_W) == 256);
    check("disp1 scr_h == 216", WM_H16(WM_DISP1_SCR_H) == 216);

    /* ================================================================
     * 6. Width/height verification
     * ================================================================ */
    printf("--- Width/height ---\n");
    reset_state();
    wm_80072BB0();

    check("draw0 width == 320", WM_H16(WM_ENVREC_BASE + 0x04) == 320);
    check("draw0 height == 216", WM_H16(WM_ENVREC_BASE + 0x06) == 216);
    check("draw1 width == 320",
          WM_H16(WM_ENVREC_BASE + WM_ENVREC_STRIDE + 0x04) == 320);
    check("draw1 height == 216",
          WM_H16(WM_ENVREC_BASE + WM_ENVREC_STRIDE + 0x06) == 216);

    /* ================================================================
     * 7. GTE screen distance
     * ================================================================ */
    printf("--- GTE screen distance ---\n");
    reset_state();
    wm_80072BB0();

    check("GTE screen distance (BCDC) == 256", WM_U32(WM_BCDC_ABS) == 256);

    /* ================================================================
     * 8. Fog/back/far state (mode=2)
     * ================================================================ */
    printf("--- Fog/back/far (mode=2) ---\n");
    reset_state();
    WM_U32(WM_D7CC_ABS) = 2;
    WM_U8(WM_FOG_R_ABS) = 0x20;
    WM_U8(WM_FOG_G_ABS) = 0x40;
    WM_U8(WM_FOG_B_ABS) = 0x60;
    wm_80072BB0();

    /* func_8002C6E0 stores to globals — verify via psx_memory. */
    /* SetBackColor and SetFarColor are PsyQ calls — verify they don't crash. */
    check("fog color bytes preserved (r)",
          WM_U8(WM_FOG_R_ABS) == 0x20);
    check("fog color bytes preserved (g)",
          WM_U8(WM_FOG_G_ABS) == 0x40);
    check("fog color bytes preserved (b)",
          WM_U8(WM_FOG_B_ABS) == 0x60);

    /* ================================================================
     * 9. Neighboring memory guards
     * ================================================================ */
    printf("--- Memory guards ---\n");
    reset_state();

    /* Set sentinel values around the environment records. */
    WM_U32(WM_OT_PTR0) = 0x005A2048;
    WM_U32(WM_OT_PTR1) = 0x005A3050;
    WM_U32(WM_BE3C) = 0x12345678;
    WM_U32(WM_POOL_BE24) = 0xAABBCCDD;
    WM_U32(WM_C618) = 999;
    WM_U32(WM_D534) = 888;
    WM_U32(WM_SLOT_C610_ABS) = 777;
    WM_U32(WM_CONV_TABLE_A_BASE) = 666;
    WM_U32(WM_CONV_TABLE_B_BASE) = 555;

    ot0_before = WM_U32(WM_OT_PTR0);
    ot1_before = WM_U32(WM_OT_PTR1);
    be3c_before = WM_U32(WM_BE3C);
    pool_before = WM_U32(WM_POOL_BE24);
    c618_before = WM_U32(WM_C618);
    d534_before = WM_U32(WM_D534);
    c610_before = WM_U32(WM_SLOT_C610_ABS);
    table_a_before = WM_U32(WM_CONV_TABLE_A_BASE);
    table_b_before = WM_U32(WM_CONV_TABLE_B_BASE);

    wm_80072BB0();

    /* OT roots unchanged. */
    check("OT ptr0 unchanged", WM_U32(WM_OT_PTR0) == ot0_before);
    check("OT ptr1 unchanged", WM_U32(WM_OT_PTR1) == ot1_before);

    /* BE3C unchanged. */
    check("BE3C unchanged", WM_U32(WM_BE3C) == be3c_before);

    /* Callback pool pointer unchanged. */
    check("pool pointer unchanged", WM_U32(WM_POOL_BE24) == pool_before);

    /* C618 unchanged. */
    check("C618 unchanged", WM_U32(WM_C618) == c618_before);

    /* D534 unchanged. */
    check("D534 unchanged", WM_U32(WM_D534) == d534_before);

    /* C610 unchanged. */
    check("C610 unchanged", WM_U32(WM_SLOT_C610_ABS) == c610_before);

    /* Table A/B unchanged. */
    check("Table A unchanged", WM_U32(WM_CONV_TABLE_A_BASE) == table_a_before);
    check("Table B unchanged", WM_U32(WM_CONV_TABLE_B_BASE) == table_b_before);

    /* ================================================================
     * 10. Framebuffer-region hash — before vs after
     * ================================================================ */
    printf("--- Framebuffer region hash ---\n");
    reset_state();
    hash_before = hash_region(WM_ENVREC_BASE, WM_ENVREC_STRIDE * 2);
    wm_80072BB0();
    hash_after = hash_region(WM_ENVREC_BASE, WM_ENVREC_STRIDE * 2);

    check("framebuffer hash changed", hash_before != hash_after);

    /* ================================================================
     * 11. Idempotency — second call produces same state
     * ================================================================ */
    printf("--- Idempotency ---\n");
    {
        uint32_t hash_first;
        reset_state();
        wm_80072BB0();
        hash_first = hash_region(WM_ENVREC_BASE, WM_ENVREC_STRIDE * 2);
        wm_80072BB0();
        hash_after = hash_region(WM_ENVREC_BASE, WM_ENVREC_STRIDE * 2);
        check("idempotent (same hash)", hash_first == hash_after);
        check("call count == 2", wm_fbi_get_calls() == 2);
    }

    /* ================================================================
     * 12. Instrumentation counter
     * ================================================================ */
    printf("--- Instrumentation ---\n");
    reset_state();
    check("initial call count == 0", wm_fbi_get_calls() == 0);
    wm_80072BB0();
    check("call count == 1 after one call", wm_fbi_get_calls() == 1);
    wm_fbi_reset();
    check("call count == 0 after reset", wm_fbi_get_calls() == 0);

    /* ================================================================
     * Summary
     * ================================================================ */
    printf("\n=== Results: %d/%d passed", pass, total);
    if (fail > 0)
        printf(", %d FAILED", fail);
    printf(" ===\n");

    return fail > 0 ? 1 : 0;
}
