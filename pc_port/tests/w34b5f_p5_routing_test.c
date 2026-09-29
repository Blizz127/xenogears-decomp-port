/*
 * W34B5F — P5 routing test.
 *
 * Verifies gate behavior, C894 routing, wm_80075228 execution,
 * SystemTransferPaletteToVRAM call, forbidden-path guards,
 * and return-handoff semantics for the common-tail P5 slice.
 *
 * P5 returns overlay-local sentinel 0x8007299C (slot-2 entry).
 * Actual post-slot-1 return PC is 0x80071064 (WorldMapMain resumes).
 *
 * Build:
 *   gcc -std=gnu17 -O0 -g -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C \
 *     -Ipc_port/include -Iinclude -Ipc_port/src \
 *     pc_port/tests/w34b5f_p5_routing_test.c \
 *     pc_port/src/world_map_common_tail.c \
 *     -o pc_port/build_native/w34b5f_p5_routing_test
 */
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_common_tail.h"

uint8_t g_PsxRam[PSX_RAM_SIZE];

static int s_heap_offset = 0;
static int s_heap_alloc_count = 0;

void* HeapAlloc(u_int allocSize, u_int allocFlags)
{
    (void)allocFlags;
    s_heap_offset += allocSize;
    s_heap_offset = (s_heap_offset + 15) & ~15;
    void *p = &g_PsxRam[PSX_RAM_SIZE - s_heap_offset];
    memset(p, 0, allocSize);
    s_heap_alloc_count++;
    return p;
}

u_short GetTPage(int tp, int abr, int x, int y)
{
    return (u_short)(((tp & 3) << 7) | ((abr & 3) << 5) |
                     ((y & 0x100) >> 4) | ((x & 0x3FF) >> 6));
}

u_short GetClut(int x, int y)
{
    return (u_short)((y << 6) | ((x >> 4) & 0x3F));
}

/* Stub for SystemTransferPaletteToVRAM — track calls. */
static int s_palette_calls = 0;
static short s_last_palette_x = -1;
static short s_last_palette_y = -1;

void SystemTransferPaletteToVRAM(short xDest, short yDest)
{
    s_palette_calls++;
    s_last_palette_x = xDest;
    s_last_palette_y = yDest;
}

static int total = 0, pass = 0, fail = 0;

static void check(const char* name, int cond)
{
    total++;
    if (cond) { pass++; printf("  PASS: %s\n", name); }
    else      { fail++; printf("  FAIL: %s\n", name); }
}

/* Run full chain P0→P4 to set up prior state. */
static void run_p0_through_p4(void)
{
    wm_common_tail_p0_reset();
    wm_8007290C_common_tail_p0();
    wm_common_tail_p1_reset();
    wm_8007293C_common_tail_p1();
    wm_common_tail_p2_reset();
    wm_80072944_common_tail_p2();
    wm_common_tail_p3_reset();
    wm_8007294C_common_tail_p3();
    wm_common_tail_p4_reset();
    wm_80072954_common_tail_p4();
}

int main(void)
{
    printf("=== W34B5F P5 routing test ===\n\n");

    /* ---- Test 1: P5 gate OFF → no execution ---- */
    printf("Test 1: P5 gate OFF\n");
    {
        memset(g_PsxRam, 0, PSX_RAM_SIZE);
        s_heap_offset = 0;
        s_heap_alloc_count = 0;
        s_palette_calls = 0;

        wm_common_tail_p5_reset();
        /* Don't call wm_8007295C_common_tail_p5 — simulating gate OFF. */

        check("P5 entry == 0 (gate OFF)", wm_ctp5_get_entry() == 0);
        check("P5 75228_calls == 0", wm_ctp5_get_75228_calls() == 0);
        check("P5 palette_calls == 0", wm_ctp5_get_palette_calls() == 0);
        check("palette stub calls == 0", s_palette_calls == 0);
    }

    /* ---- Test 2: P5 gate ON, C894=0 (natural path) ---- */
    printf("\nTest 2: P5 gate ON, C894=0 (natural path)\n");
    {
        memset(g_PsxRam, 0, PSX_RAM_SIZE);
        s_heap_offset = 0;
        s_palette_calls = 0;

        /* Set C894 = 0 (natural Lahan value). */
        *(u32*)PSX_ADDR(WM_FLAG_C894_ABS) = 0;
        /* Set button state (no cross button). */
        *(u16*)PSX_ADDR(WM_BUTTONS_ABS) = 0;

        wm_common_tail_p5_reset();
        u32 cut = wm_8007295C_common_tail_p5();

        check("P5 entry == 1", wm_ctp5_get_entry() == 1);
        check("P5 c894_zero == 1", wm_ctp5_get_c894_zero() == 1);
        check("P5 c894_nonzero == 0", wm_ctp5_get_c894_nonzero() == 0);
        check("P5 75228_calls == 1", wm_ctp5_get_75228_calls() == 1);
        check("P5 palette_calls == 1", wm_ctp5_get_palette_calls() == 1);
        check("palette stub calls == 1", s_palette_calls == 1);
        check("palette x == 0x130", s_last_palette_x == 0x130);
        check("palette y == 0x1E0", s_last_palette_y == 0x1E0);
        check("P5 sentinel == 0x8007299C (slot-2 entry)", cut == WM_COMMON_TAIL_P5_SLOT1_END_SENTINEL);
        check("P5 real return PC == 0x80071064", WM_COMMON_TAIL_P5_REAL_RETURN_PC == 0x80071064);
    }

    /* ---- Test 3: P5 gate ON, C894=0 with cross button ---- */
    printf("\nTest 3: P5 C894=0, cross button set\n");
    {
        memset(g_PsxRam, 0, PSX_RAM_SIZE);
        s_heap_offset = 0;
        s_palette_calls = 0;

        *(u32*)PSX_ADDR(WM_FLAG_C894_ABS) = 0;
        *(u16*)PSX_ADDR(WM_BUTTONS_ABS) = 0x4000; /* cross button */

        wm_common_tail_p5_reset();
        wm_8007295C_common_tail_p5();

        check("75228_calls == 1", wm_ctp5_get_75228_calls() == 1);
        check("D_8009BE40 == 768 (cross)",
              *(u32*)PSX_ADDR(WM_D_8009BE40_ABS) == 0x0300);
    }

    /* ---- Test 4: P5 gate ON, C894=0 without cross button ---- */
    printf("\nTest 4: P5 C894=0, no cross button\n");
    {
        memset(g_PsxRam, 0, PSX_RAM_SIZE);
        s_heap_offset = 0;
        s_palette_calls = 0;

        *(u32*)PSX_ADDR(WM_FLAG_C894_ABS) = 0;
        *(u16*)PSX_ADDR(WM_BUTTONS_ABS) = 0; /* no buttons */

        wm_common_tail_p5_reset();
        wm_8007295C_common_tail_p5();

        check("D_8009BE40 == 384 (no cross)",
              *(u32*)PSX_ADDR(WM_D_8009BE40_ABS) == 0x0180);
    }

    /* ---- Test 5: P5 gate ON, C894!=0 (alternate path) ---- */
    printf("\nTest 5: P5 C894!=0 (alternate path)\n");
    {
        memset(g_PsxRam, 0, PSX_RAM_SIZE);
        s_heap_offset = 0;
        s_palette_calls = 0;

        *(u32*)PSX_ADDR(WM_FLAG_C894_ABS) = 1; /* C894 = 1 */

        wm_common_tail_p5_reset();
        u32 cut = wm_8007295C_common_tail_p5();

        check("P5 entry == 1", wm_ctp5_get_entry() == 1);
        check("P5 c894_zero == 0", wm_ctp5_get_c894_zero() == 0);
        check("P5 c894_nonzero == 1", wm_ctp5_get_c894_nonzero() == 1);
        check("P5 75228_calls == 0 (skipped)", wm_ctp5_get_75228_calls() == 0);
        check("P5 palette_calls == 1", wm_ctp5_get_palette_calls() == 1);
        check("palette stub calls == 1", s_palette_calls == 1);
        check("P5 sentinel == 0x8007299C (slot-2 entry)", cut == WM_COMMON_TAIL_P5_SLOT1_END_SENTINEL);
        check("P5 real return PC == 0x80071064", WM_COMMON_TAIL_P5_REAL_RETURN_PC == 0x80071064);
    }

    /* ---- Test 6: wm_80075228 global writes ---- */
    printf("\nTest 6: wm_80075228 global writes\n");
    {
        memset(g_PsxRam, 0, PSX_RAM_SIZE);
        s_heap_offset = 0;

        /* Pre-set some halfwords to verify they get zeroed. */
        *(u16*)PSX_ADDR(0x8009C872) = 0x1234;
        *(u16*)PSX_ADDR(0x8009C870) = 0x5678;
        *(u16*)PSX_ADDR(0x8009C860) = 0xABCD;
        *(u16*)PSX_ADDR(WM_BUTTONS_ABS) = 0; /* no cross */

        wm_80075228();

        /* 16 halfwords from D_8009C872 downward should be zero. */
        check("D_8009C872 == 0 after zero",
              *(u16*)PSX_ADDR(0x8009C872) == 0);
        check("D_8009C870 == 0 after zero",
              *(u16*)PSX_ADDR(0x8009C870) == 0);
        check("D_8009C860 == 0 after zero",
              *(u16*)PSX_ADDR(0x8009C860) == 0);

        check("D_8009D64C == 1",
              *(u32*)PSX_ADDR(WM_D_8009D64C_ABS) == 1);
        check("D_8009BE40 == 384 (no cross)",
              *(u32*)PSX_ADDR(WM_D_8009BE40_ABS) == 0x0180);
        check("D_8009BCC4 == 1",
              *(u32*)PSX_ADDR(WM_D_8009BCC4_ABS) == 1);
        check("D_8009D80C == 0",
              *(u32*)PSX_ADDR(WM_D_8009D80C_ABS) == 0);
    }

    /* ---- Test 7: Repeated P5 entry count ---- */
    printf("\nTest 7: Repeated entry count\n");
    {
        memset(g_PsxRam, 0, PSX_RAM_SIZE);
        s_heap_offset = 0;
        s_palette_calls = 0;

        *(u32*)PSX_ADDR(WM_FLAG_C894_ABS) = 0;

        wm_common_tail_p5_reset();
        wm_8007295C_common_tail_p5();
        wm_8007295C_common_tail_p5();
        wm_8007295C_common_tail_p5();

        check("P5 entry == 3", wm_ctp5_get_entry() == 3);
        check("P5 75228_calls == 3", wm_ctp5_get_75228_calls() == 3);
        check("P5 palette_calls == 3", wm_ctp5_get_palette_calls() == 3);
    }

    /* ---- Test 8: Reset clears all counters ---- */
    printf("\nTest 8: Reset clears counters\n");
    {
        wm_common_tail_p5_reset();

        check("entry == 0 after reset", wm_ctp5_get_entry() == 0);
        check("c894_zero == 0 after reset", wm_ctp5_get_c894_zero() == 0);
        check("c894_nonzero == 0 after reset", wm_ctp5_get_c894_nonzero() == 0);
        check("75228_calls == 0 after reset", wm_ctp5_get_75228_calls() == 0);
        check("palette_calls == 0 after reset", wm_ctp5_get_palette_calls() == 0);
        check("last_cut == 0 after reset", wm_ctp5_get_last_cut() == 0);
    }

    /* ---- Test 9: Full P0→P1→P2→P3→P4→P5 chain ---- */
    printf("\nTest 9: Full chain returns exact cuts\n");
    {
        memset(g_PsxRam, 0, PSX_RAM_SIZE);
        s_heap_offset = 0;
        s_palette_calls = 0;

        *(u32*)PSX_ADDR(WM_FLAG_C894_ABS) = 0;
        *(u16*)PSX_ADDR(WM_BUTTONS_ABS) = 0;

        run_p0_through_p4();

        wm_common_tail_p5_reset();
        u32 p5 = wm_8007295C_common_tail_p5();
        check("P5 sentinel == 0x8007299C (slot-2 entry)", p5 == WM_COMMON_TAIL_P5_SLOT1_END_SENTINEL);
        check("P5 real return PC == 0x80071064", WM_COMMON_TAIL_P5_REAL_RETURN_PC == 0x80071064);
        check("75228 called once", wm_ctp5_get_75228_calls() == 1);
        check("palette called once", wm_ctp5_get_palette_calls() == 1);
    }

    /* ---- Test 10: Forbidden path guards ---- */
    printf("\nTest 10: Forbidden path guards\n");
    {
        memset(g_PsxRam, 0, PSX_RAM_SIZE);
        s_heap_offset = 0;

        *(u32*)PSX_ADDR(WM_FLAG_C894_ABS) = 0;

        wm_common_tail_p5_reset();
        wm_8007295C_common_tail_p5();

        check("forbidden scheduler == 0", wm_ctp5_get_forbidden_scheduler() == 0);
        check("forbidden world_loop == 0", wm_ctp5_get_forbidden_world_loop() == 0);
        check("forbidden drawotag == 0", wm_ctp5_get_forbidden_drawotag() == 0);
    }

    /* ---- Test 11: First excluded ZERO VERIFIED ---- */
    printf("\nTest 11: First excluded / next helper ZERO\n");
    {
        memset(g_PsxRam, 0, PSX_RAM_SIZE);
        s_heap_offset = 0;

        *(u32*)PSX_ADDR(WM_FLAG_C894_ABS) = 0;

        wm_common_tail_p5_reset();
        u32 cut = wm_8007295C_common_tail_p5();

        check("slot-2 sentinel = 0x8007299C", cut == WM_COMMON_TAIL_P5_SLOT1_END_SENTINEL);
        check("real return PC = 0x80071064", WM_COMMON_TAIL_P5_REAL_RETURN_PC == 0x80071064);
        check("scheduler ZERO", wm_ctp5_get_forbidden_scheduler() == 0);
        check("world_loop ZERO", wm_ctp5_get_forbidden_world_loop() == 0);
        check("drawotag ZERO", wm_ctp5_get_forbidden_drawotag() == 0);
    }

    /* ---- Test 12: Prior state preservation ---- */
    printf("\nTest 12: Prior state preservation\n");
    {
        memset(g_PsxRam, 0, PSX_RAM_SIZE);
        s_heap_offset = 0;

        /* Set known prior state. */
        *(u32*)PSX_ADDR(0x8009C610) = 0;
        *(u32*)PSX_ADDR(0x8009C618) = 1024;
        *(u32*)PSX_ADDR(0x8009BCDC) = 256;
        *(u32*)PSX_ADDR(WM_FLAG_C894_ABS) = 0;

        run_p0_through_p4();
        wm_common_tail_p5_reset();
        wm_8007295C_common_tail_p5();

        check("C610 preserved", *(u32*)PSX_ADDR(0x8009C610) == 0);
        check("C618 preserved", *(u32*)PSX_ADDR(0x8009C618) == 1024);
        check("BCDC preserved", *(u32*)PSX_ADDR(0x8009BCDC) == 256);

        /* Prior allocations still valid. */
        check("P1 ptr1 still valid", *(u32*)PSX_ADDR(0x8009BC3C) != 0);
        check("P1 ptr2 still valid", *(u32*)PSX_ADDR(0x8009BCB4) != 0);
        check("P2 ptr1 still valid", *(u32*)PSX_ADDR(0x8009BE1C) != 0);
        check("P2 ptr2 still valid", *(u32*)PSX_ADDR(0x8009BE20) != 0);
        check("P3 ptr1 valid", *(u32*)PSX_ADDR(0x8009D7F8) != 0);
        check("P3 ptr2 valid", *(u32*)PSX_ADDR(0x8009D7FC) != 0);
        check("P4 ptr1 valid", *(u32*)PSX_ADDR(0x8009D7E8) != 0);
        check("P4 ptr2 valid", *(u32*)PSX_ADDR(0x8009D7EC) != 0);
    }

    /* ---- Test 13: C894=1 edge value ---- */
    printf("\nTest 13: C894 edge value (1)\n");
    {
        memset(g_PsxRam, 0, PSX_RAM_SIZE);
        s_heap_offset = 0;
        s_palette_calls = 0;

        *(u32*)PSX_ADDR(WM_FLAG_C894_ABS) = 1;

        wm_common_tail_p5_reset();
        wm_8007295C_common_tail_p5();

        check("c894_nonzero == 1", wm_ctp5_get_c894_nonzero() == 1);
        check("c894_zero == 0", wm_ctp5_get_c894_zero() == 0);
        check("75228_calls == 0", wm_ctp5_get_75228_calls() == 0);
        check("palette_calls == 1", wm_ctp5_get_palette_calls() == 1);
    }

    /* ---- Test 14: C894=0xFF edge value ---- */
    printf("\nTest 14: C894 edge value (0xFF)\n");
    {
        memset(g_PsxRam, 0, PSX_RAM_SIZE);
        s_heap_offset = 0;
        s_palette_calls = 0;

        *(u32*)PSX_ADDR(WM_FLAG_C894_ABS) = 0xFF;

        wm_common_tail_p5_reset();
        wm_8007295C_common_tail_p5();

        check("c894_nonzero == 1", wm_ctp5_get_c894_nonzero() == 1);
        check("75228_calls == 0", wm_ctp5_get_75228_calls() == 0);
        check("palette_calls == 1", wm_ctp5_get_palette_calls() == 1);
    }

    printf("\n=== Results: %d/%d PASS ===\n", pass, total);
    return (fail > 0) ? 1 : 0;
}
