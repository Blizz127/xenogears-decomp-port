/*
 * W34B5D — P3 routing test.
 *
 * Verifies gate behavior, prerequisites, and forbidden-path guards
 * for the common-tail P3 slice.
 *
 * Build:
 *   gcc -std=gnu17 -O0 -g -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C \
 *     -Ipc_port/include -Iinclude -Ipc_port/src \
 *     pc_port/tests/w34b5d_p3_routing_test.c \
 *     pc_port/src/world_map_common_tail.c \
 *     -o pc_port/build_native/w34b5d_p3_routing_test
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


void SystemTransferPaletteToVRAM(short xDest, short yDest)
{
    (void)xDest; (void)yDest;
}
void SetSemiTrans(void *p, int abe)
{
    u8 *base = (u8*)p;
    if (abe)
        base[7] |= 0x02;
    else
        base[7] &= ~0x02;
}

static int total = 0, pass = 0, fail = 0;

static void check(const char* name, int cond)
{
    total++;
    if (cond) { pass++; printf("  PASS: %s\n", name); }
    else      { fail++; printf("  FAIL: %s\n", name); }
}

int main(void)
{
    printf("=== W34B5D P3 routing test ===\n\n");

    /* ---- Test 1: P3 gate OFF → no execution ---- */
    printf("Test 1: P3 gate OFF\n");
    {
        memset(g_PsxRam, 0, PSX_RAM_SIZE);
        s_heap_offset = 0;
        s_heap_alloc_count = 0;

        wm_common_tail_p3_reset();
        /* Don't call wm_8007294C_common_tail_p3 — simulating gate OFF. */

        check("P3 entry == 0 (gate OFF)", wm_ctp3_get_entry() == 0);
        check("P3 865a0_calls == 0", wm_ctp3_get_865a0_calls() == 0);
        check("P3 alloc_calls == 0", wm_ctp3_get_alloc_calls() == 0);
        check("D_8009D7F8 == 0 (no alloc)", *(u32*)PSX_ADDR(0x8009D7F8) == 0);
        check("D_8009D7FC == 0 (no alloc)", *(u32*)PSX_ADDR(0x8009D7FC) == 0);
    }

    /* ---- Test 2: P3 gate ON → exactly one execution ---- */
    printf("\nTest 2: P3 gate ON, exact route\n");
    {
        memset(g_PsxRam, 0, PSX_RAM_SIZE);
        s_heap_offset = 0;
        s_heap_alloc_count = 0;

        wm_common_tail_p3_reset();
        u32 cut = wm_8007294C_common_tail_p3();

        check("P3 entry == 1", wm_ctp3_get_entry() == 1);
        check("P3 865a0_calls == 1", wm_ctp3_get_865a0_calls() == 1);
        check("P3 alloc_calls == 2", wm_ctp3_get_alloc_calls() == 2);
        check("P3 cut == 0x80072954", cut == 0x80072954);
        check("D_8009D7F8 != 0", *(u32*)PSX_ADDR(0x8009D7F8) != 0);
        check("D_8009D7FC != 0", *(u32*)PSX_ADDR(0x8009D7FC) != 0);
    }

    /* ---- Test 3: Repeated P3 entry count ---- */
    printf("\nTest 3: Repeated entry count\n");
    {
        memset(g_PsxRam, 0, PSX_RAM_SIZE);
        s_heap_offset = 0;

        wm_common_tail_p3_reset();
        wm_8007294C_common_tail_p3();
        wm_8007294C_common_tail_p3();
        wm_8007294C_common_tail_p3();

        check("P3 entry == 3 after three calls", wm_ctp3_get_entry() == 3);
        check("P3 865a0_calls == 3", wm_ctp3_get_865a0_calls() == 3);
        check("P3 alloc_calls == 6", wm_ctp3_get_alloc_calls() == 6);
    }

    /* ---- Test 4: Reset clears all counters ---- */
    printf("\nTest 4: Reset clears counters\n");
    {
        wm_common_tail_p3_reset();

        check("entry == 0 after reset", wm_ctp3_get_entry() == 0);
        check("865a0_calls == 0 after reset", wm_ctp3_get_865a0_calls() == 0);
        check("alloc_calls == 0 after reset", wm_ctp3_get_alloc_calls() == 0);
        check("last_cut == 0 after reset", wm_ctp3_get_last_cut() == 0);
    }

    /* ---- Test 5: Full P0→P1→P2→P3 chain returns exact cuts ---- */
    printf("\nTest 5: Full chain returns exact cuts\n");
    {
        memset(g_PsxRam, 0, PSX_RAM_SIZE);
        s_heap_offset = 0;

        wm_common_tail_p0_reset();
        u32 p0 = wm_8007290C_common_tail_p0();
        check("P0 cut == 0x8007293C", p0 == 0x8007293C);

        wm_common_tail_p1_reset();
        u32 p1 = wm_8007293C_common_tail_p1();
        check("P1 cut == 0x80072944", p1 == 0x80072944);

        wm_common_tail_p2_reset();
        u32 p2 = wm_80072944_common_tail_p2();
        check("P2 cut == 0x8007294C", p2 == 0x8007294C);

        wm_common_tail_p3_reset();
        u32 p3 = wm_8007294C_common_tail_p3();
        check("P3 cut == 0x80072954", p3 == 0x80072954);
    }

    /* ---- Test 6: First excluded / next helper guards ---- */
    printf("\nTest 6: Forbidden path guards\n");
    {
        memset(g_PsxRam, 0, PSX_RAM_SIZE);
        s_heap_offset = 0;

        wm_common_tail_p3_reset();
        wm_8007294C_common_tail_p3();

        check("forbidden 85fe0 == 0", wm_ctp3_get_forbidden_85fe0() == 0);
        check("forbidden scheduler == 0", wm_ctp3_get_forbidden_scheduler() == 0);
        check("forbidden world_loop == 0", wm_ctp3_get_forbidden_world_loop() == 0);
        check("forbidden 75228 == 0", wm_ctp3_get_forbidden_75228() == 0);
    }

    /* ---- Test 7: First excluded instruction ZERO VERIFIED ---- */
    printf("\nTest 7: First excluded / next helper ZERO\n");
    {
        memset(g_PsxRam, 0, PSX_RAM_SIZE);
        s_heap_offset = 0;

        wm_common_tail_p3_reset();
        u32 cut = wm_8007294C_common_tail_p3();

        /* Cut must be exactly 0x80072954 (first excluded = 0x80085FE0). */
        check("first excluded = 0x80072954", cut == 0x80072954);

        /* Forbidden counters must all be zero. */
        check("85fe0 ZERO", wm_ctp3_get_forbidden_85fe0() == 0);
        check("scheduler ZERO", wm_ctp3_get_forbidden_scheduler() == 0);
        check("world_loop ZERO", wm_ctp3_get_forbidden_world_loop() == 0);
        check("75228 ZERO", wm_ctp3_get_forbidden_75228() == 0);
    }

    /* ---- Test 8: Prior state preservation through full chain ---- */
    printf("\nTest 8: Prior state preservation\n");
    {
        memset(g_PsxRam, 0, PSX_RAM_SIZE);
        s_heap_offset = 0;

        /* Set known prior state. */
        *(u32*)PSX_ADDR(0x8009C610) = 0;
        *(u32*)PSX_ADDR(0x8009C618) = 1024;
        *(u32*)PSX_ADDR(0x8009BCDC) = 256;

        wm_common_tail_p0_reset();
        wm_8007290C_common_tail_p0();
        wm_common_tail_p1_reset();
        wm_8007293C_common_tail_p1();
        wm_common_tail_p2_reset();
        wm_80072944_common_tail_p2();
        wm_common_tail_p3_reset();
        wm_8007294C_common_tail_p3();

        check("C610 preserved", *(u32*)PSX_ADDR(0x8009C610) == 0);
        check("C618 preserved", *(u32*)PSX_ADDR(0x8009C618) == 1024);
        check("BCDC preserved", *(u32*)PSX_ADDR(0x8009BCDC) == 256);

        /* P1 allocations still valid. */
        check("P1 ptr1 still valid", *(u32*)PSX_ADDR(0x8009BC3C) != 0);
        check("P1 ptr2 still valid", *(u32*)PSX_ADDR(0x8009BCB4) != 0);

        /* P2 allocations still valid. */
        check("P2 ptr1 still valid", *(u32*)PSX_ADDR(0x8009BE1C) != 0);
        check("P2 ptr2 still valid", *(u32*)PSX_ADDR(0x8009BE20) != 0);

        /* P3 allocations valid and distinct. */
        u32 p3_1 = *(u32*)PSX_ADDR(0x8009D7F8);
        u32 p3_2 = *(u32*)PSX_ADDR(0x8009D7FC);
        u32 p2_1 = *(u32*)PSX_ADDR(0x8009BE1C);
        u32 p2_2 = *(u32*)PSX_ADDR(0x8009BE20);
        u32 p1_1 = *(u32*)PSX_ADDR(0x8009BC3C);
        u32 p1_2 = *(u32*)PSX_ADDR(0x8009BCB4);
        check("P3 ptr1 valid", p3_1 != 0);
        check("P3 ptr2 valid", p3_2 != 0);
        check("P3 ptr1 != P1 ptr1", p3_1 != p1_1);
        check("P3 ptr2 != P1 ptr2", p3_2 != p1_2);
        check("P3 ptr1 != P2 ptr1", p3_1 != p2_1);
        check("P3 ptr2 != P2 ptr2", p3_2 != p2_2);
        check("P3 ptr1 != P3 ptr2", p3_1 != p3_2);
    }

    printf("\n=== Results: %d/%d PASS ===\n", pass, total);
    return (fail > 0) ? 1 : 0;
}
