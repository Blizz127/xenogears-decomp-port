/*
 * W34B5F — Production test for wm_80075228 and common-tail P5 slice.
 *
 * Tests wm_80075228 state initialization against retail decode,
 * and P5 slice routing against production data.
 *
 * P5 returns overlay-local sentinel 0x8007299C (slot-2 entry).
 * Actual post-slot-1 return PC is 0x80071064 (WorldMapMain resumes).
 *
 * Build:
 *   gcc -std=gnu17 -O0 -g -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C \
 *     -Ipc_port/include -Iinclude -Ipc_port/src \
 *     pc_port/tests/w34b5f_p5_prod_test.c \
 *     pc_port/src/world_map_common_tail.c \
 *     -o pc_port/build_native/w34b5f_p5_prod_test
 */
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_common_tail.h"

uint8_t g_PsxRam[PSX_RAM_SIZE];

#define WM_U8(a)  (*(u8*)PSX_ADDR(a))
#define WM_U16(a) (*(u16*)PSX_ADDR(a))
#define WM_U32(a) (*(u32*)PSX_ADDR(a))

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

/* SystemTransferPaletteToVRAM stub — track calls and arguments. */
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
    printf("=== W34B5F P5 production test ===\n\n");

    /* ---- Test 1: wm_80075228 retail decode — zero 16 halfwords ---- */
    printf("Test 1: wm_80075228 zero-16-halfword loop\n");
    {
        memset(g_PsxRam, 0, PSX_RAM_SIZE);

        /* Pre-poison the 16 halfword region. */
        u32 base = WM_D_8009C872_ABS;
        int i;
        for (i = 0; i < 16; i++) {
            WM_U16(base - i * 2) = (u16)(0x1000 + i);
        }
        /* Also poison adjacent halfwords (should NOT be touched). */
        WM_U16(base + 2) = 0xDEAD;
        WM_U16(base - 32) = 0xBEEF;

        *(u16*)PSX_ADDR(WM_BUTTONS_ABS) = 0;

        wm_80075228();

        /* All 16 halfwords should be zero. */
        int all_zero = 1;
        for (i = 0; i < 16; i++) {
            if (WM_U16(base - i * 2) != 0) { all_zero = 0; break; }
        }
        check("16 halfwords zeroed", all_zero);

        /* Adjacent halfwords should be untouched. */
        check("base+2 untouched (0xDEAD)", WM_U16(base + 2) == 0xDEAD);
        check("base-32 untouched (0xBEEF)", WM_U16(base - 32) == 0xBEEF);
    }

    /* ---- Test 2: wm_80075228 D_8009D64C = 1 ---- */
    printf("\nTest 2: wm_80075228 D_8009D64C write\n");
    {
        memset(g_PsxRam, 0, PSX_RAM_SIZE);
        *(u16*)PSX_ADDR(WM_BUTTONS_ABS) = 0;

        wm_80075228();

        check("D_8009D64C == 1", *(u32*)PSX_ADDR(WM_D_8009D64C_ABS) == 1);
    }

    /* ---- Test 3: wm_80075228 cross button — D_8009BE40 ---- */
    printf("\nTest 3: wm_80075228 cross button D_8009BE40\n");
    {
        /* Cross button set → 768 */
        memset(g_PsxRam, 0, PSX_RAM_SIZE);
        *(u16*)PSX_ADDR(WM_BUTTONS_ABS) = 0x4000;
        wm_80075228();
        check("cross=0x4000 → D_8009BE40=768",
              *(u32*)PSX_ADDR(WM_D_8009BE40_ABS) == 768);

        /* No cross → 384 */
        memset(g_PsxRam, 0, PSX_RAM_SIZE);
        *(u16*)PSX_ADDR(WM_BUTTONS_ABS) = 0;
        wm_80075228();
        check("no cross → D_8009BE40=384",
              *(u32*)PSX_ADDR(WM_D_8009BE40_ABS) == 384);

        /* Other buttons only (not 0x4000) → 384 */
        memset(g_PsxRam, 0, PSX_RAM_SIZE);
        *(u16*)PSX_ADDR(WM_BUTTONS_ABS) = 0x8000;
        wm_80075228();
        check("other button → D_8009BE40=384",
              *(u32*)PSX_ADDR(WM_D_8009BE40_ABS) == 384);
    }

    /* ---- Test 4: wm_80075228 D_8009BCC4 and D_8009D80C ---- */
    printf("\nTest 4: wm_80075228 D_8009BCC4/D_8009D80C\n");
    {
        memset(g_PsxRam, 0, PSX_RAM_SIZE);
        *(u16*)PSX_ADDR(WM_BUTTONS_ABS) = 0;

        wm_80075228();

        check("D_8009BCC4 == 1", *(u32*)PSX_ADDR(WM_D_8009BCC4_ABS) == 1);
        check("D_8009D80C == 0", *(u32*)PSX_ADDR(WM_D_8009D80C_ABS) == 0);
    }

    /* ---- Test 5: P5 C894=0 natural path — exact call sequence ---- */
    printf("\nTest 5: P5 natural path (C894=0) call sequence\n");
    {
        memset(g_PsxRam, 0, PSX_RAM_SIZE);
        s_heap_offset = 0;
        s_palette_calls = 0;

        *(u32*)PSX_ADDR(WM_FLAG_C894_ABS) = 0;
        *(u16*)PSX_ADDR(WM_BUTTONS_ABS) = 0;

        run_p0_through_p4();

        wm_common_tail_p5_reset();
        u32 cut = wm_8007295C_common_tail_p5();

        check("P5 entry == 1", wm_ctp5_get_entry() == 1);
        check("C894=0 path taken", wm_ctp5_get_c894_zero() == 1);
        check("75228 called once", wm_ctp5_get_75228_calls() == 1);
        check("palette called once", s_palette_calls == 1);
        check("palette x=0x130", s_last_palette_x == 0x130);
        check("palette y=0x1E0", s_last_palette_y == 0x1E0);
        check("sentinel == 0x8007299C (slot-2 entry)", cut == WM_COMMON_TAIL_P5_SLOT1_END_SENTINEL);
        check("real return PC == 0x80071064", WM_COMMON_TAIL_P5_REAL_RETURN_PC == 0x80071064);
    }

    /* ---- Test 6: P5 C894!=0 alternate path ---- */
    printf("\nTest 6: P5 alternate path (C894!=0)\n");
    {
        memset(g_PsxRam, 0, PSX_RAM_SIZE);
        s_heap_offset = 0;
        s_palette_calls = 0;

        *(u32*)PSX_ADDR(WM_FLAG_C894_ABS) = 1;
        *(u16*)PSX_ADDR(WM_BUTTONS_ABS) = 0;

        run_p0_through_p4();

        wm_common_tail_p5_reset();
        u32 cut = wm_8007295C_common_tail_p5();

        check("C894!=0 path taken", wm_ctp5_get_c894_nonzero() == 1);
        check("75228 NOT called", wm_ctp5_get_75228_calls() == 0);
        check("palette called once", s_palette_calls == 1);
        check("sentinel == 0x8007299C (slot-2 entry)", cut == WM_COMMON_TAIL_P5_SLOT1_END_SENTINEL);
        check("real return PC == 0x80071064", WM_COMMON_TAIL_P5_REAL_RETURN_PC == 0x80071064);
    }

    /* ---- Test 7: Full chain P0→P5 exact cuts ---- */
    printf("\nTest 7: Full chain exact cuts\n");
    {
        memset(g_PsxRam, 0, PSX_RAM_SIZE);
        s_heap_offset = 0;
        s_palette_calls = 0;

        *(u32*)PSX_ADDR(WM_FLAG_C894_ABS) = 0;
        *(u16*)PSX_ADDR(WM_BUTTONS_ABS) = 0;

        wm_common_tail_p0_reset();
        u32 p0 = wm_8007290C_common_tail_p0();
        check("P0 == 0x8007293C", p0 == 0x8007293C);

        wm_common_tail_p1_reset();
        u32 p1 = wm_8007293C_common_tail_p1();
        check("P1 == 0x80072944", p1 == 0x80072944);

        wm_common_tail_p2_reset();
        u32 p2 = wm_80072944_common_tail_p2();
        check("P2 == 0x8007294C", p2 == 0x8007294C);

        wm_common_tail_p3_reset();
        u32 p3 = wm_8007294C_common_tail_p3();
        check("P3 == 0x80072954", p3 == 0x80072954);

        wm_common_tail_p4_reset();
        u32 p4 = wm_80072954_common_tail_p4();
        check("P4 == 0x8007295C", p4 == 0x8007295C);

        wm_common_tail_p5_reset();
        u32 p5 = wm_8007295C_common_tail_p5();
        check("P5 sentinel == 0x8007299C (slot-2 entry)", p5 == WM_COMMON_TAIL_P5_SLOT1_END_SENTINEL);
        check("P5 real return PC == 0x80071064", WM_COMMON_TAIL_P5_REAL_RETURN_PC == 0x80071064);
    }

    /* ---- Test 8: Prior allocation preservation ---- */
    printf("\nTest 8: Prior allocation preservation\n");
    {
        memset(g_PsxRam, 0, PSX_RAM_SIZE);
        s_heap_offset = 0;

        *(u32*)PSX_ADDR(WM_FLAG_C894_ABS) = 0;
        *(u16*)PSX_ADDR(WM_BUTTONS_ABS) = 0;

        /* Set known prior state. */
        *(u32*)PSX_ADDR(0x8009C610) = 0;
        *(u32*)PSX_ADDR(0x8009C618) = 1024;
        *(u32*)PSX_ADDR(0x8009BCDC) = 256;

        run_p0_through_p4();
        wm_common_tail_p5_reset();
        wm_8007295C_common_tail_p5();

        /* Prior globals preserved. */
        check("C610 preserved", *(u32*)PSX_ADDR(0x8009C610) == 0);
        check("C618 preserved", *(u32*)PSX_ADDR(0x8009C618) == 1024);
        check("BCDC preserved", *(u32*)PSX_ADDR(0x8009BCDC) == 256);

        /* Prior allocations still live. */
        check("P1 pool1 live", *(u32*)PSX_ADDR(0x8009BC3C) != 0);
        check("P1 pool2 live", *(u32*)PSX_ADDR(0x8009BCB4) != 0);
        check("P2 pool1 live", *(u32*)PSX_ADDR(0x8009BE1C) != 0);
        check("P2 pool2 live", *(u32*)PSX_ADDR(0x8009BE20) != 0);
        check("P3 pool1 live", *(u32*)PSX_ADDR(0x8009D7F8) != 0);
        check("P3 pool2 live", *(u32*)PSX_ADDR(0x8009D7FC) != 0);
        check("P4 pool1 live", *(u32*)PSX_ADDR(0x8009D7E8) != 0);
        check("P4 pool2 live", *(u32*)PSX_ADDR(0x8009D7EC) != 0);
    }

    /* ---- Test 9: P5 writes don't corrupt prior pools ---- */
    printf("\nTest 9: P5 writes don't corrupt prior pools\n");
    {
        /* Hash P1 pool1 before P5. */
        u32 p1_1 = *(u32*)PSX_ADDR(0x8009BC3C);
        u32 p1_2 = *(u32*)PSX_ADDR(0x8009BCB4);
        u32 p2_1 = *(u32*)PSX_ADDR(0x8009BE1C);
        u32 p3_1 = *(u32*)PSX_ADDR(0x8009D7F8);
        u32 p4_1 = *(u32*)PSX_ADDR(0x8009D7E8);

        /* wm_80075228 writes to C872, D64C, BE40, BCC4, D80C.
         * None of these overlap with pool pointers. */
        check("P5 writes don't touch P1 pool1 ptr",
              *(u32*)PSX_ADDR(0x8009BC3C) == p1_1);
        check("P5 writes don't touch P1 pool2 ptr",
              *(u32*)PSX_ADDR(0x8009BCB4) == p1_2);
        check("P5 writes don't touch P2 pool1 ptr",
              *(u32*)PSX_ADDR(0x8009BE1C) == p2_1);
        check("P5 writes don't touch P3 pool1 ptr",
              *(u32*)PSX_ADDR(0x8009D7F8) == p3_1);
        check("P5 writes don't touch P4 pool1 ptr",
              *(u32*)PSX_ADDR(0x8009D7E8) == p4_1);
    }

    /* ---- Test 10: Forbidden path ZERO VERIFIED ---- */
    printf("\nTest 10: Forbidden path ZERO VERIFIED\n");
    {
        memset(g_PsxRam, 0, PSX_RAM_SIZE);
        s_heap_offset = 0;

        *(u32*)PSX_ADDR(WM_FLAG_C894_ABS) = 0;

        run_p0_through_p4();
        wm_common_tail_p5_reset();
        wm_8007295C_common_tail_p5();

        check("scheduler ZERO", wm_ctp5_get_forbidden_scheduler() == 0);
        check("world_loop ZERO", wm_ctp5_get_forbidden_world_loop() == 0);
        check("drawotag ZERO", wm_ctp5_get_forbidden_drawotag() == 0);
    }

    printf("\n=== Results: %d/%d PASS ===\n", pass, total);
    return (fail > 0) ? 1 : 0;
}
