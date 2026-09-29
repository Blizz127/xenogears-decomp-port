/*
 * W34B5A — Production-linked test for wm_8007290C_common_tail_p0.
 *
 * Links the real production implementation from world_map_common_tail.c.
 * Tests every selector path represented in the bounded prefix.
 *
 * Build:
 *   gcc -std=gnu17 -O0 -g -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C \
 *     -Ipc_port/include -Iinclude -Ipc_port/src \
 *     pc_port/tests/w34b5a_common_tail_prod_test.c \
 *     pc_port/src/world_map_common_tail.c \
 *     -o pc_port/build_native/w34b5a_common_tail_prod_test
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_common_tail.h"

/* Provide g_PsxRam for the production module. */
uint8_t g_PsxRam[PSX_RAM_SIZE];

/* Stubs for helpers used by wm_800865A0 (not exercised by P0 tests). */
void* HeapAlloc(u_int allocSize, u_int allocFlags)
{
    (void)allocSize; (void)allocFlags;
    return NULL;
}
u_short GetTPage(int tp, int abr, int x, int y)
{
    (void)tp; (void)abr; (void)x; (void)y;
    return 0;
}
u_short GetClut(int x, int y)
{
    (void)x; (void)y;
    return 0;
}


void SystemTransferPaletteToVRAM(short xDest, short yDest)
{
    (void)xDest; (void)yDest;
}
#define WM_U8(a)  (*(u8*)PSX_ADDR(a))
#define WM_U16(a) (*(u16*)PSX_ADDR(a))
#define WM_U32(a) (*(u32*)PSX_ADDR(a))

/* Test harness. */
static int total = 0, pass = 0, fail = 0;

static void check(const char* name, int cond)
{
    total++;
    if (cond) {
        pass++;
        printf("  PASS: %s\n", name);
    } else {
        fail++;
        printf("  FAIL: %s\n", name);
    }
}

/* C610 selector address. */
#define C610_ADDR        0x8009C610u
#define D_80059179_ADDR  0x80059179u

/* Record table setup. */
#define TABLE_BASE_ADDR  0x8009BCC0u
#define TABLE_PSX        0x800A0000u
#define TABLE_RECORDS    16
#define RECORD_STRIDE    672u

int main(void)
{
    u32 cut;
    u8* table_host;
    int i;

    printf("=== W34B5A common-tail P0 production test ===\n\n");

    /* Allocate table in PSX RAM. */
    WM_U32(TABLE_BASE_ADDR) = TABLE_PSX;
    table_host = (u8*)PSX_ADDR(TABLE_PSX);
    memset(table_host, 0, TABLE_RECORDS * RECORD_STRIDE);

    /* Reset per-init counters. */
    wm_common_tail_p0_reset();

    /* ---- Test 1: NATURAL path (C610=0) ---- */
    printf("Test 1: NATURAL path (C610=0)\n");
    {
        /* Set C610 = 0. */
        WM_U32(C610_ADDR) = 0;
        /* Clear D_80059179. */
        WM_U8(D_80059179_ADDR) = 0;

        cut = wm_8007290C_common_tail_p0();

        check("cut == 0x8007293C", cut == 0x8007293Cu);
        check("D_80059179 == 1 after C610=0",
              WM_U8(D_80059179_ADDR) == 1);
        check("entry counter == 1", wm_ctp0_get_entry() == 1);
        check("c610_zero counter == 1", wm_ctp0_get_c610_zero() == 1);
        check("c610_nonzero counter == 0", wm_ctp0_get_c610_nonzero() == 0);
        check("89160_calls counter == 1", wm_ctp0_get_89160_calls() == 1);
    }

    /* ---- Test 2: ALTERNATE path (C610=1) ---- */
    printf("\nTest 2: ALTERNATE path (C610=1)\n");
    {
        /* Reset counters. */
        wm_common_tail_p0_reset();

        /* Set C610 = 1. */
        WM_U32(C610_ADDR) = 1;
        /* Clear D_80059179. */
        WM_U8(D_80059179_ADDR) = 0;

        /* Clear record 14 flag to detect if wm_80089160 was called. */
        {
            u8* rec = (u8*)PSX_ADDR(TABLE_PSX + 14 * RECORD_STRIDE);
            rec[0x4F] = 0;
        }

        cut = wm_8007290C_common_tail_p0();

        check("cut == 0x8007293C", cut == 0x8007293Cu);
        check("D_80059179 == 0 after C610=1",
              WM_U8(D_80059179_ADDR) == 0);
        check("89160_calls counter == 0 (skipped)",
              wm_ctp0_get_89160_calls() == 0);
        check("c610_nonzero counter == 1",
              wm_ctp0_get_c610_nonzero() == 1);
        /* Verify wm_80089160 was NOT called (flag not set). */
        {
            u8* rec = (u8*)PSX_ADDR(TABLE_PSX + 14 * RECORD_STRIDE);
            check("record 14 flag NOT set (wm_80089160 not called)",
                  (rec[0x4F] & 0x80) == 0);
        }
    }

    /* ---- Test 3: ALTERNATE path (C610=2) ---- */
    printf("\nTest 3: ALTERNATE path (C610=2)\n");
    {
        wm_common_tail_p0_reset();
        WM_U32(C610_ADDR) = 2;
        WM_U8(D_80059179_ADDR) = 0;

        cut = wm_8007290C_common_tail_p0();

        check("cut == 0x8007293C", cut == 0x8007293Cu);
        check("D_80059179 == 0 after C610=2",
              WM_U8(D_80059179_ADDR) == 0);
        check("89160_calls == 0", wm_ctp0_get_89160_calls() == 0);
    }

    /* ---- Test 4: Gate disabled (counters should not be called) ---- */
    printf("\nTest 4: Repeated world initialization/reset\n");
    {
        /* Simulate two world init cycles. */
        wm_common_tail_p0_reset();

        /* First init: C610=0. */
        WM_U32(C610_ADDR) = 0;
        WM_U8(D_80059179_ADDR) = 0;
        cut = wm_8007290C_common_tail_p0();
        check("first init cut == 0x8007293C", cut == 0x8007293Cu);

        /* Reset (simulates new world init). */
        wm_common_tail_p0_reset();

        /* Second init: C610=0. */
        WM_U32(C610_ADDR) = 0;
        WM_U8(D_80059179_ADDR) = 0;
        cut = wm_8007290C_common_tail_p0();
        check("second init cut == 0x8007293C", cut == 0x8007293Cu);
        check("entry counter == 1 (after reset)",
              wm_ctp0_get_entry() == 1);
    }

    /* ---- Test 5: Forbidden path counters ---- */
    printf("\nTest 5: Forbidden path counters (should be 0)\n");
    {
        wm_common_tail_p0_reset();

        check("forbidden_978fc == 0",
              wm_ctp0_get_forbidden_978fc() == 0);
        check("forbidden_scheduler == 0",
              wm_ctp0_get_forbidden_scheduler() == 0);
        check("forbidden_world_loop == 0",
              wm_ctp0_get_forbidden_world_loop() == 0);
    }

    /* ---- Test 6: C610 unchanged after call ---- */
    printf("\nTest 6: C610 unchanged after call\n");
    {
        wm_common_tail_p0_reset();
        WM_U32(C610_ADDR) = 0;
        wm_8007290C_common_tail_p0();
        check("C610 still 0 after call", WM_U32(C610_ADDR) == 0);

        WM_U32(C610_ADDR) = 5;
        wm_8007290C_common_tail_p0();
        check("C610 still 5 after call", WM_U32(C610_ADDR) == 5);
    }

    /* ---- Summary ---- */
    printf("\n=== Results: %d/%d PASS ===\n", pass, total);
    return (fail > 0) ? 1 : 0;
}
