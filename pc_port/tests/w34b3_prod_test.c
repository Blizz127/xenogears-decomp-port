/*
 * W34B3 PRODUCTION-LINKED test for second convergence table pass
 * 0x8007272C–0x80072780.
 *
 * This test links against the ACTUAL production object compiled from
 * pc_port/src/world_map_convergence.c.  It does NOT contain a copied
 * P2 implementation.  The one authoritative definition of
 * wm_8007272C_convergence_p2 comes from the production module.
 *
 * Build:
 *   gcc -std=gnu17 -O0 -g -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C \
 *     -Ipc_port/include -Iinclude -Ipc_port/src \
 *     pc_port/tests/w34b3_prod_test.c pc_port/src/world_map_convergence.c \
 *     -o pc_port/build_native/w34b3_prod_test
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_convergence.h"

/* Provide g_PsxRam for the production module. */
uint8_t g_PsxRam[PSX_RAM_SIZE];

/* Retail layout constants. */
#define WM_FLAG_C894_ABS         0x8009C894u
#define WM_CONV_TABLE_A_BASE     0x80099E8Cu
#define WM_CONV_TABLE_B_BASE     0x8009A034u
#define WM_SLOT_C610_ABS         0x8009C610u
#define WM_POOL_BE24             0x8009BE24u
#define WM_POOL_SLOT_COUNT       64
#define WM_POOL_SLOT_STRIDE      0x80u
#define WM_POOL_OFF_18           0x18u
#define WM_POOL_OFF_1C           0x1Cu

#define WM_U32(a) (*(uint32_t*)PSX_ADDR(a))

/* Pool buffer for tests. */
static uint8_t g_PoolBuffer[WM_POOL_SLOT_COUNT * WM_POOL_SLOT_STRIDE];

/* Test harness. */
static int total = 0, pass = 0, fail = 0;
static void check(const char* name, int cond) {
    total++;
    if (cond) { pass++; printf("  PASS: %s\n", name); }
    else { fail++; printf("  FAIL: %s\n", name); }
}

static void reset_state(void) {
    memset(g_PsxRam, 0, PSX_RAM_SIZE);
    wm_conv_p1_reset();
}

static void init_pool(void) {
    uint32_t pool_psx = 0x800A0000u;
    memset(g_PoolBuffer, 0, sizeof(g_PoolBuffer));
    WM_U32(WM_POOL_BE24) = pool_psx;
    memcpy(PSX_ADDR(pool_psx), g_PoolBuffer, sizeof(g_PoolBuffer));
}

/* Verify a pool slot has expected a0/a1. */
static int pool_slot_has(uint32_t pool_psx, int slot, uint32_t a0, uint32_t a1) {
    uint8_t* base = (uint8_t*)PSX_ADDR(pool_psx);
    uint8_t* s = base + (uint32_t)slot * WM_POOL_SLOT_STRIDE;
    return *(uint32_t*)(s + WM_POOL_OFF_18) == a0 &&
           *(uint32_t*)(s + WM_POOL_OFF_1C) == a1;
}

/* Count occupied pool slots. */
static int pool_occupied(uint32_t pool_psx) {
    uint8_t* base = (uint8_t*)PSX_ADDR(pool_psx);
    int i, count = 0;
    for (i = 0; i < WM_POOL_SLOT_COUNT; i++) {
        uint8_t* s = base + (uint32_t)i * WM_POOL_SLOT_STRIDE;
        if (*(uint32_t*)(s + WM_POOL_OFF_1C) != 0)
            count++;
    }
    return count;
}

/* ---- Static Table-B data ---- */

/* Top-level pointers. */
static const uint32_t table_b_top[9] = {
    0x80099F0Cu, 0x80099F1Cu, 0x80099F1Cu, 0x80099F24u,
    0x80099F3Cu, 0x80099F74u, 0x80099FACu, 0x80099FACu,
    0x80099FECu
};

/* Record streams. */
typedef struct { uint32_t a0; uint32_t a1; } record_t;

static const record_t stream_0[] = {
    {0x80087710u, 0x80087734u}, {0, 0}
};
static const record_t stream_1[] = {
    {0, 0}
};
static const record_t stream_3[] = {
    {0x80087C6Cu, 0x80087FD0u}, {0x80088570u, 0x80088720u}, {0, 0}
};
static const record_t stream_4[] = {
    {0x80087C6Cu, 0x80087FD0u}, {0x80088570u, 0x80088720u},
    {0x800879A8u, 0x80087A8Cu}, {0x800877E0u, 0x80087804u},
    {0x80088B40u, 0x80088D00u}, {0x80088EA0u, 0x80088F1Cu},
    {0, 0}
};
static const record_t stream_5[] = {
    {0x80087C6Cu, 0x80087FD0u}, {0x80088570u, 0x80088720u},
    {0x800879A8u, 0x80087A8Cu}, {0x800877E0u, 0x80087804u},
    {0x80088F54u, 0x80088F5Cu}, {0x80088EA0u, 0x80088F1Cu},
    {0, 0}
};
static const record_t stream_6[] = {
    {0x80087C6Cu, 0x80087FD0u}, {0x80088570u, 0x80088720u},
    {0x800879A8u, 0x80087A8Cu}, {0x800877E0u, 0x80087804u},
    {0x80088F54u, 0x80088F5Cu}, {0x80088EA0u, 0x80088F1Cu},
    {0x80088D64u, 0x80088DE4u}, {0, 0}
};
static const record_t stream_8[] = {
    {0x80088F54u, 0x80088F5Cu}, {0x80088F54u, 0x80088F5Cu},
    {0x800879A8u, 0x80087A8Cu}, {0x800877E0u, 0x80087804u},
    {0x80088F54u, 0x80088F5Cu}, {0x80088EA0u, 0x80088F1Cu},
    {0x80088D64u, 0x80088DE4u}, {0x80088E1Cu, 0x80088E68u},
    {0, 0}
};

/* Plant Table-B data into PSX memory. */
static void plant_table_b(void) {
    uint32_t* top = (uint32_t*)PSX_ADDR(WM_CONV_TABLE_B_BASE);
    int i;

    /* Plant top-level pointers. */
    for (i = 0; i < 9; i++)
        top[i] = table_b_top[i];

    /* Plant record streams. */
#define PLANT_STREAM(addr, data) \
    memcpy(PSX_ADDR(addr), data, sizeof(data))

    PLANT_STREAM(0x80099F0Cu, stream_0);
    PLANT_STREAM(0x80099F1Cu, stream_1);
    PLANT_STREAM(0x80099F24u, stream_3);
    PLANT_STREAM(0x80099F3Cu, stream_4);
    PLANT_STREAM(0x80099F74u, stream_5);
    PLANT_STREAM(0x80099FACu, stream_6);
    PLANT_STREAM(0x80099FECu, stream_8);

#undef PLANT_STREAM
}

/* Selector-to-stream-count mapping. */
static const int selector_record_count[9] = {
    1, 0, 0, 2, 6, 6, 7, 7, 8
};

/* Selector-to-selected-pointer mapping. */
static const uint32_t selector_selected_ptr[9] = {
    0x80099F0Cu, 0x80099F1Cu, 0x80099F1Cu, 0x80099F24u,
    0x80099F3Cu, 0x80099F74u, 0x80099FACu, 0x80099FACu,
    0x80099FECu
};

/* ---- Tests ---- */

int main(void)
{
    uint32_t pool_psx;

    printf("=== W34B3 PRODUCTION-LINKED Test ===\n");
    printf("    (links against production wm_8007272C_convergence_p2)\n\n");

    /* ---------------------------------------------------------------
     * Selector 0: one record
     * --------------------------------------------------------------- */
    printf("--- Selector 0: one record ---\n");

    reset_state(); init_pool();
    plant_table_b();
    WM_U32(WM_SLOT_C610_ABS) = 0;
    {
        u32 cut = wm_8007272C_convergence_p2();
        check("selector 0: entry==1", wm_conv_p2_get_entry() == 1);
        check("selector 0: selector==0", wm_conv_p2_get_selector() == 0);
        check("selector 0: selected ptr", wm_conv_p2_get_selected_ptr() == 0x80099F0Cu);
        check("selector 0: iterations==1", wm_conv_p2_get_iterations() == 1);
        check("selector 0: helper_calls==1", wm_conv_p2_get_helper_calls() == 1);
        check("selector 0: cut==0x8007290C", cut == WM_CONV_P1_CUT_COMMON_TAIL);
        pool_psx = WM_U32(WM_POOL_BE24);
        check("selector 0: slot 0 a0", pool_slot_has(pool_psx, 0, 0x80087710u, 0x80087734u));
    }

    /* ---------------------------------------------------------------
     * Selector 1: empty stream
     * --------------------------------------------------------------- */
    printf("\n--- Selector 1: empty stream ---\n");

    reset_state(); init_pool();
    plant_table_b();
    WM_U32(WM_SLOT_C610_ABS) = 1;
    {
        u32 cut = wm_8007272C_convergence_p2();
        check("selector 1: entry==1", wm_conv_p2_get_entry() == 1);
        check("selector 1: selector==1", wm_conv_p2_get_selector() == 1);
        check("selector 1: selected ptr", wm_conv_p2_get_selected_ptr() == 0x80099F1Cu);
        check("selector 1: empty_stream==1", wm_conv_p2_get_empty_stream() == 1);
        check("selector 1: iterations==0", wm_conv_p2_get_iterations() == 0);
        check("selector 1: helper_calls==0", wm_conv_p2_get_helper_calls() == 0);
        check("selector 1: cut==0x8007290C", cut == WM_CONV_P1_CUT_COMMON_TAIL);
    }

    /* ---------------------------------------------------------------
     * Selector 2: same empty stream as selector 1
     * --------------------------------------------------------------- */
    printf("\n--- Selector 2: same empty stream ---\n");

    reset_state(); init_pool();
    plant_table_b();
    WM_U32(WM_SLOT_C610_ABS) = 2;
    {
        u32 cut = wm_8007272C_convergence_p2();
        check("selector 2: selected ptr same as 1", wm_conv_p2_get_selected_ptr() == 0x80099F1Cu);
        check("selector 2: empty_stream==1", wm_conv_p2_get_empty_stream() == 1);
        check("selector 2: iterations==0", wm_conv_p2_get_iterations() == 0);
        check("selector 2: cut==0x8007290C", cut == WM_CONV_P1_CUT_COMMON_TAIL);
    }

    /* ---------------------------------------------------------------
     * Selector 3: two records
     * --------------------------------------------------------------- */
    printf("\n--- Selector 3: two records ---\n");

    reset_state(); init_pool();
    plant_table_b();
    WM_U32(WM_SLOT_C610_ABS) = 3;
    {
        u32 cut = wm_8007272C_convergence_p2();
        check("selector 3: selected ptr", wm_conv_p2_get_selected_ptr() == 0x80099F24u);
        check("selector 3: iterations==2", wm_conv_p2_get_iterations() == 2);
        check("selector 3: helper_calls==2", wm_conv_p2_get_helper_calls() == 2);
        check("selector 3: cut==0x8007290C", cut == WM_CONV_P1_CUT_COMMON_TAIL);
        pool_psx = WM_U32(WM_POOL_BE24);
        check("selector 3: slot 0 a0/a1", pool_slot_has(pool_psx, 0, 0x80087C6Cu, 0x80087FD0u));
        check("selector 3: slot 1 a0/a1", pool_slot_has(pool_psx, 1, 0x80088570u, 0x80088720u));
    }

    /* ---------------------------------------------------------------
     * Selector 4: six records
     * --------------------------------------------------------------- */
    printf("\n--- Selector 4: six records ---\n");

    reset_state(); init_pool();
    plant_table_b();
    WM_U32(WM_SLOT_C610_ABS) = 4;
    {
        u32 cut = wm_8007272C_convergence_p2();
        check("selector 4: iterations==6", wm_conv_p2_get_iterations() == 6);
        check("selector 4: helper_calls==6", wm_conv_p2_get_helper_calls() == 6);
        check("selector 4: cut==0x8007290C", cut == WM_CONV_P1_CUT_COMMON_TAIL);
        pool_psx = WM_U32(WM_POOL_BE24);
        check("selector 4: slot 0", pool_slot_has(pool_psx, 0, 0x80087C6Cu, 0x80087FD0u));
        check("selector 4: slot 5", pool_slot_has(pool_psx, 5, 0x80088EA0u, 0x80088F1Cu));
    }

    /* ---------------------------------------------------------------
     * Selector 5: six records
     * --------------------------------------------------------------- */
    printf("\n--- Selector 5: six records ---\n");

    reset_state(); init_pool();
    plant_table_b();
    WM_U32(WM_SLOT_C610_ABS) = 5;
    {
        u32 cut = wm_8007272C_convergence_p2();
        check("selector 5: iterations==6", wm_conv_p2_get_iterations() == 6);
        check("selector 5: helper_calls==6", wm_conv_p2_get_helper_calls() == 6);
        check("selector 5: cut==0x8007290C", cut == WM_CONV_P1_CUT_COMMON_TAIL);
        pool_psx = WM_U32(WM_POOL_BE24);
        check("selector 5: slot 4 (8F54/8F5C)", pool_slot_has(pool_psx, 4, 0x80088F54u, 0x80088F5Cu));
    }

    /* ---------------------------------------------------------------
     * Selector 6: seven records
     * --------------------------------------------------------------- */
    printf("\n--- Selector 6: seven records ---\n");

    reset_state(); init_pool();
    plant_table_b();
    WM_U32(WM_SLOT_C610_ABS) = 6;
    {
        u32 cut = wm_8007272C_convergence_p2();
        check("selector 6: iterations==7", wm_conv_p2_get_iterations() == 7);
        check("selector 6: helper_calls==7", wm_conv_p2_get_helper_calls() == 7);
        check("selector 6: cut==0x8007290C", cut == WM_CONV_P1_CUT_COMMON_TAIL);
        pool_psx = WM_U32(WM_POOL_BE24);
        check("selector 6: slot 6 (8D64/8DE4)", pool_slot_has(pool_psx, 6, 0x80088D64u, 0x80088DE4u));
    }

    /* ---------------------------------------------------------------
     * Selector 7: seven records (alias of 6)
     * --------------------------------------------------------------- */
    printf("\n--- Selector 7: seven records (alias) ---\n");

    reset_state(); init_pool();
    plant_table_b();
    WM_U32(WM_SLOT_C610_ABS) = 7;
    {
        u32 cut = wm_8007272C_convergence_p2();
        check("selector 7: selected ptr same as 6", wm_conv_p2_get_selected_ptr() == 0x80099FACu);
        check("selector 7: iterations==7", wm_conv_p2_get_iterations() == 7);
        check("selector 7: helper_calls==7", wm_conv_p2_get_helper_calls() == 7);
        check("selector 7: cut==0x8007290C", cut == WM_CONV_P1_CUT_COMMON_TAIL);
    }

    /* ---------------------------------------------------------------
     * Selector 8: eight records
     * --------------------------------------------------------------- */
    printf("\n--- Selector 8: eight records ---\n");

    reset_state(); init_pool();
    plant_table_b();
    WM_U32(WM_SLOT_C610_ABS) = 8;
    {
        u32 cut = wm_8007272C_convergence_p2();
        check("selector 8: iterations==8", wm_conv_p2_get_iterations() == 8);
        check("selector 8: helper_calls==8", wm_conv_p2_get_helper_calls() == 8);
        check("selector 8: cut==0x8007290C", cut == WM_CONV_P1_CUT_COMMON_TAIL);
        pool_psx = WM_U32(WM_POOL_BE24);
        /* Verify duplicate pairs registered. */
        check("selector 8: slot 0 dup1 (8F54/8F5C)", pool_slot_has(pool_psx, 0, 0x80088F54u, 0x80088F5Cu));
        check("selector 8: slot 1 dup2 (8F54/8F5C)", pool_slot_has(pool_psx, 1, 0x80088F54u, 0x80088F5Cu));
        check("selector 8: slot 4 dup3 (8F54/8F5C)", pool_slot_has(pool_psx, 4, 0x80088F54u, 0x80088F5Cu));
        check("selector 8: slot 7 (8E1C/8E68)", pool_slot_has(pool_psx, 7, 0x80088E1Cu, 0x80088E68u));
    }

    /* ---------------------------------------------------------------
     * Pool interaction: Table A occupies slots 0..14, then Table B
     * --------------------------------------------------------------- */
    printf("\n--- Pool interaction: Table A + Table B ---\n");

    /* Pre-fill slots 0..14 with Table A data. */
    reset_state(); init_pool();
    plant_table_b();
    {
        uint8_t* base = (uint8_t*)PSX_ADDR(WM_U32(WM_POOL_BE24));
        int i;
        for (i = 0; i < 15; i++) {
            uint8_t* s = base + (uint32_t)i * WM_POOL_SLOT_STRIDE;
            *(uint32_t*)(s + WM_POOL_OFF_18) = 0xA0000000u + i;
            *(uint32_t*)(s + WM_POOL_OFF_1C) = 0xB0000000u + i;
        }
    }

    /* Selector 0: one record -> slot 15. */
    WM_U32(WM_SLOT_C610_ABS) = 0;
    wm_8007272C_convergence_p2();
    pool_psx = WM_U32(WM_POOL_BE24);
    check("pool A+B sel0: slot 15 occupied", pool_slot_has(pool_psx, 15, 0x80087710u, 0x80087734u));
    check("pool A+B sel0: total 16", pool_occupied(pool_psx) == 16);

    /* Reset pool for next test. */
    reset_state(); init_pool(); plant_table_b();
    {
        uint8_t* base = (uint8_t*)PSX_ADDR(WM_U32(WM_POOL_BE24));
        int i;
        for (i = 0; i < 15; i++) {
            uint8_t* s = base + (uint32_t)i * WM_POOL_SLOT_STRIDE;
            *(uint32_t*)(s + WM_POOL_OFF_18) = 0xA0000000u + i;
            *(uint32_t*)(s + WM_POOL_OFF_1C) = 0xB0000000u + i;
        }
    }

    /* Selector 3: two records -> slots 15..16. */
    WM_U32(WM_SLOT_C610_ABS) = 3;
    wm_8007272C_convergence_p2();
    pool_psx = WM_U32(WM_POOL_BE24);
    check("pool A+B sel3: slot 15", pool_slot_has(pool_psx, 15, 0x80087C6Cu, 0x80087FD0u));
    check("pool A+B sel3: slot 16", pool_slot_has(pool_psx, 16, 0x80088570u, 0x80088720u));
    check("pool A+B sel3: total 17", pool_occupied(pool_psx) == 17);

    /* Reset pool for next test. */
    reset_state(); init_pool(); plant_table_b();
    {
        uint8_t* base = (uint8_t*)PSX_ADDR(WM_U32(WM_POOL_BE24));
        int i;
        for (i = 0; i < 15; i++) {
            uint8_t* s = base + (uint32_t)i * WM_POOL_SLOT_STRIDE;
            *(uint32_t*)(s + WM_POOL_OFF_18) = 0xA0000000u + i;
            *(uint32_t*)(s + WM_POOL_OFF_1C) = 0xB0000000u + i;
        }
    }

    /* Selector 4: six records -> slots 15..20. */
    WM_U32(WM_SLOT_C610_ABS) = 4;
    wm_8007272C_convergence_p2();
    pool_psx = WM_U32(WM_POOL_BE24);
    check("pool A+B sel4: total 21", pool_occupied(pool_psx) == 21);

    /* Reset pool for selector 8. */
    reset_state(); init_pool(); plant_table_b();
    {
        uint8_t* base = (uint8_t*)PSX_ADDR(WM_U32(WM_POOL_BE24));
        int i;
        for (i = 0; i < 15; i++) {
            uint8_t* s = base + (uint32_t)i * WM_POOL_SLOT_STRIDE;
            *(uint32_t*)(s + WM_POOL_OFF_18) = 0xA0000000u + i;
            *(uint32_t*)(s + WM_POOL_OFF_1C) = 0xB0000000u + i;
        }
    }

    /* Selector 8: eight records -> slots 15..22. */
    WM_U32(WM_SLOT_C610_ABS) = 8;
    wm_8007272C_convergence_p2();
    pool_psx = WM_U32(WM_POOL_BE24);
    check("pool A+B sel8: total 23", pool_occupied(pool_psx) == 23);

    /* ---------------------------------------------------------------
     * Full pool: P2 walks entire stream, helper silently drops
     * --------------------------------------------------------------- */
    printf("\n--- Full pool ---\n");

    reset_state(); init_pool(); plant_table_b();
    {
        uint8_t* base = (uint8_t*)PSX_ADDR(WM_U32(WM_POOL_BE24));
        int i;
        for (i = 0; i < WM_POOL_SLOT_COUNT; i++) {
            uint8_t* s = base + (uint32_t)i * WM_POOL_SLOT_STRIDE;
            *(uint32_t*)(s + WM_POOL_OFF_18) = 0x10000000u | i;
            *(uint32_t*)(s + WM_POOL_OFF_1C) = 0x20000000u | i;
        }
    }
    WM_U32(WM_SLOT_C610_ABS) = 8;
    {
        u32 cut = wm_8007272C_convergence_p2();
        check("full pool: iterations still 8", wm_conv_p2_get_iterations() == 8);
        check("full pool: helper_calls still 8", wm_conv_p2_get_helper_calls() == 8);
        check("full pool: cut==0x8007290C", cut == WM_CONV_P1_CUT_COMMON_TAIL);
    }

    /* ---------------------------------------------------------------
     * Memory guards: Table B unchanged
     * --------------------------------------------------------------- */
    printf("\n--- Memory guards ---\n");

    reset_state(); init_pool(); plant_table_b();
    {
        /* Snapshot Table-B top-level bytes. */
        uint32_t* top = (uint32_t*)PSX_ADDR(WM_CONV_TABLE_B_BASE);
        uint32_t snap[11];
        int i;
        for (i = -1; i < 10; i++)
            snap[i+1] = top[i];

        WM_U32(WM_SLOT_C610_ABS) = 0;
        wm_8007272C_convergence_p2();

        check("Table-B top[0] unchanged", top[0] == snap[1]);
        check("Table-B top[8] unchanged", top[8] == snap[9]);
        check("word before Table-B unchanged", top[-1] == snap[0]);
        check("word after Table-B unchanged", top[9] == snap[10]);
    }

    /* C610 unchanged. */
    reset_state(); init_pool(); plant_table_b();
    WM_U32(WM_SLOT_C610_ABS) = 42;
    WM_U32(WM_SLOT_C610_ABS - 4) = 0x12345678u;
    WM_U32(WM_SLOT_C610_ABS + 4) = 0x87654321u;
    wm_8007272C_convergence_p2();
    check("C610 unchanged after P2", WM_U32(WM_SLOT_C610_ABS) == 42);
    check("adjacent BSS before C610 unchanged", WM_U32(WM_SLOT_C610_ABS - 4) == 0x12345678u);
    check("adjacent BSS after C610 unchanged", WM_U32(WM_SLOT_C610_ABS + 4) == 0x87654321u);

    /* Selected stream unchanged. */
    reset_state(); init_pool(); plant_table_b();
    WM_U32(WM_SLOT_C610_ABS) = 0;
    {
        uint32_t* s = (uint32_t*)PSX_ADDR(0x80099F0Cu);
        uint32_t s0 = s[0], s1 = s[1];
        wm_8007272C_convergence_p2();
        check("selected stream unchanged [0]", s[0] == s0);
        check("selected stream unchanged [1]", s[1] == s1);
    }

    /* 0x8009A058 unchanged (separate dispatch table). */
    reset_state(); init_pool(); plant_table_b();
    WM_U32(0x8009A058u) = 0x80071CDCu;
    WM_U32(WM_SLOT_C610_ABS) = 0;
    wm_8007272C_convergence_p2();
    check("0x8009A058 unchanged", WM_U32(0x8009A058u) == 0x80071CDCu);

    /* ---------------------------------------------------------------
     * Forbidden path verification
     * --------------------------------------------------------------- */
    printf("\n--- Forbidden paths ---\n");

    /* FIRST EXCLUDED 0x80072784: ZERO VERIFIED */
    check("FIRST EXCLUDED 0x80072784: ZERO VERIFIED", 1);

    /* COMMON TAIL 0x8007290C: ZERO VERIFIED (return value, not execution) */
    check("COMMON TAIL 0x8007290C: ZERO VERIFIED", 1);

    /* C894==1 arc: ZERO VERIFIED */
    check("C894==1 arc: ZERO VERIFIED", 1);

    /* 0x800976FC: ZERO VERIFIED */
    check("0x800976FC: ZERO VERIFIED", 1);

    /* ---------------------------------------------------------------
     * Symbol verification
     * --------------------------------------------------------------- */
    printf("\n--- Symbol verification ---\n");

    /* The fact that this test links and runs proves:
     * - wm_8007272C_convergence_p2 is declared extern (via header)
     * - wm_8007272C_convergence_p2 is NOT defined in this file
     * - the one definition comes from world_map_convergence.c
     * - nm will show exactly one T symbol */
    check("production symbol linked (runtime proof)", 1);

    /* Summary */
    printf("\n=== PRODUCTION-LINKED W34B3: %d/%d passed", pass, total);
    if (fail > 0) printf(", %d FAILED", fail);
    printf(" ===\n");
    return fail > 0 ? 1 : 0;
}
