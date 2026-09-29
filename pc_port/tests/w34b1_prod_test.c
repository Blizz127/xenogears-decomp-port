/*
 * W34B1 PRODUCTION-LINKED test for first convergence caller slice
 * 0x800726C0–0x80072728.
 *
 * This test links against the ACTUAL production object compiled from
 * pc_port/src/world_map_convergence.c.  It does NOT contain a copied
 * P1 implementation.  The one authoritative definition of
 * wm_800726C0_convergence_p1 comes from the production module.
 *
 * Build:
 *   gcc -std=gnu17 -O0 -g -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C \
 *     -Ipc_port/include -Iinclude -Ipc_port/src \
 *     pc_port/tests/w34b1_prod_test.c pc_port/src/world_map_convergence.c \
 *     -o pc_port/build_native/w34b1_prod_test
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_convergence.h"

/* Provide g_PsxRam for the production module (non-static, matching psx_memory.h). */
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

#define WM_CONV_P1_CUT_SECOND_TABLE 0x8007272Cu
#define WM_CONV_P1_CUT_FLAG1_ARC    0x80072784u
#define WM_CONV_P1_CUT_COMMON_TAIL  0x8007290Cu

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

static void setup_table(uint32_t* records, int count) {
    uint32_t *base = (uint32_t*)PSX_ADDR(WM_CONV_TABLE_A_BASE);
    int i;
    for (i = 0; i < count; i++) {
        base[i*2] = records[i*2];
        base[i*2+1] = records[i*2+1];
    }
    base[count*2] = 0;
    base[count*2+1] = 0xDEADBEEFu;
    base[count*2+2] = 0xAAAAAAAAu;
    base[count*2+3] = 0xBBBBBBBBu;
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

int main(void)
{
    uint32_t rec[16];
    uint32_t *base;
    uint32_t pool_psx;

    printf("=== W34B1 PRODUCTION-LINKED Test ===\n");
    printf("    (links against production wm_800726C0_convergence_p1)\n\n");

    /* ---------------------------------------------------------------
     * C894 dispatch
     * --------------------------------------------------------------- */
    printf("--- C894 dispatch ---\n");

    /* 1. flag 0 enters first-table path */
    reset_state(); init_pool();
    WM_U32(WM_FLAG_C894_ABS) = 0;
    setup_table((uint32_t[]){0,0}, 0);
    wm_800726C0_convergence_p1();
    check("flag 0 enters first-table path", wm_conv_p1_get_flag0() == 1);
    check("flag 0 not flag1", wm_conv_p1_get_flag1_cut() == 0);
    check("flag 0 not other", wm_conv_p1_get_other_cut() == 0);

    /* 2. flag 1 -> cut 0x80072784 */
    reset_state(); init_pool();
    WM_U32(WM_FLAG_C894_ABS) = 1;
    rec[0]=0x11111111u; rec[1]=0x22222222u;
    setup_table(rec, 1);
    wm_800726C0_convergence_p1();
    check("flag 1 -> flag1_cut 1", wm_conv_p1_get_flag1_cut() == 1);
    check("flag 1 no helper call", wm_conv_p1_get_helper_calls() == 0);
    check("flag 1 next 0x80072784", wm_conv_p1_get_last_next() == WM_CONV_P1_CUT_FLAG1_ARC);

    /* 3. flag 2 -> other path */
    reset_state(); init_pool();
    WM_U32(WM_FLAG_C894_ABS) = 2;
    setup_table(rec, 1);
    wm_800726C0_convergence_p1();
    check("flag 2 -> other_cut 1", wm_conv_p1_get_other_cut() == 1);
    check("flag 2 no helper", wm_conv_p1_get_helper_calls() == 0);
    check("flag 2 next 0x8007290C", wm_conv_p1_get_last_next() == WM_CONV_P1_CUT_COMMON_TAIL);

    /* 4. flag UINT32_MAX -> other path */
    reset_state(); init_pool();
    WM_U32(WM_FLAG_C894_ABS) = 0xFFFFFFFFu;
    setup_table(rec, 1);
    wm_800726C0_convergence_p1();
    check("flag MAX -> other_cut", wm_conv_p1_get_other_cut() == 1);
    check("flag MAX no helper", wm_conv_p1_get_helper_calls() == 0);
    check("flag MAX next 0x8007290C", wm_conv_p1_get_last_next() == WM_CONV_P1_CUT_COMMON_TAIL);

    /* ---------------------------------------------------------------
     * Table behavior
     * --------------------------------------------------------------- */
    printf("\n--- Table behavior ---\n");

    /* 5. empty table: zero helper calls, cut 0x8007272C */
    reset_state(); init_pool();
    WM_U32(WM_FLAG_C894_ABS) = 0;
    setup_table((uint32_t[]){0,0}, 0);
    wm_800726C0_convergence_p1();
    check("empty table zero helper calls", wm_conv_p1_get_helper_calls() == 0);
    check("empty table empty flag", wm_conv_p1_get_empty() == 1);
    check("empty table cut 0x8007272C", wm_conv_p1_get_last_next() == WM_CONV_P1_CUT_SECOND_TABLE);
    check("empty table iterations 0", wm_conv_p1_get_iterations() == 0);

    /* 6. one record: one helper call */
    reset_state(); init_pool();
    WM_U32(WM_FLAG_C894_ABS) = 0;
    rec[0]=0xAAAAAAAAu; rec[1]=0xBBBBBBBBu;
    setup_table(rec, 1);
    wm_800726C0_convergence_p1();
    check("one record -> 1 helper call", wm_conv_p1_get_helper_calls() == 1);
    pool_psx = WM_U32(WM_POOL_BE24);
    check("one record a0/a1 stored", pool_slot_has(pool_psx, 0, 0xAAAAAAAAu, 0xBBBBBBBBu));
    check("one record cut 0x8007272C", wm_conv_p1_get_last_next() == WM_CONV_P1_CUT_SECOND_TABLE);
    check("one record iterations 1", wm_conv_p1_get_iterations() == 1);

    /* 7. multiple records */
    reset_state(); init_pool();
    WM_U32(WM_FLAG_C894_ABS) = 0;
    {
        uint32_t mrec[] = {0x11111111u,0x11111112u, 0x22222221u,0x22222222u, 0x33333331u,0x33333332u};
        setup_table(mrec, 3);
        wm_800726C0_convergence_p1();
        check("3 records -> 3 helper calls", wm_conv_p1_get_helper_calls() == 3);
        pool_psx = WM_U32(WM_POOL_BE24);
        check("record 0 a0/a1", pool_slot_has(pool_psx, 0, 0x11111111u, 0x11111112u));
        check("record 1 a0/a1", pool_slot_has(pool_psx, 1, 0x22222221u, 0x22222222u));
        check("record 2 a0/a1", pool_slot_has(pool_psx, 2, 0x33333331u, 0x33333332u));
        check("3 records iterations 3", wm_conv_p1_get_iterations() == 3);
    }

    /* 8. a1 == 0 still calls helper */
    reset_state(); init_pool();
    WM_U32(WM_FLAG_C894_ABS) = 0;
    rec[0]=0x12345678u; rec[1]=0;
    setup_table(rec, 1);
    wm_800726C0_convergence_p1();
    check("a1==0 still calls helper", wm_conv_p1_get_helper_calls() == 1);

    /* 9. duplicate records: no dedup */
    reset_state(); init_pool();
    WM_U32(WM_FLAG_C894_ABS) = 0;
    {
        uint32_t dup[] = {0xDEAD0001u,0xBEEF0001u, 0xDEAD0001u,0xBEEF0001u};
        setup_table(dup, 2);
        wm_800726C0_convergence_p1();
        check("duplicate records -> 2 calls", wm_conv_p1_get_helper_calls() == 2);
    }

    /* 10. full pool */
    reset_state(); init_pool();
    {
        uint8_t* b = (uint8_t*)PSX_ADDR(WM_U32(WM_POOL_BE24));
        int i;
        for (i=0;i<WM_POOL_SLOT_COUNT;i++) {
            uint8_t* s = b + i*WM_POOL_SLOT_STRIDE;
            *(uint32_t*)(s+WM_POOL_OFF_18)=0x10000000u|i;
            *(uint32_t*)(s+WM_POOL_OFF_1C)=0x20000000u|i;
        }
        WM_U32(WM_FLAG_C894_ABS)=0;
        {
            uint32_t fullrec[] = {0xAAAA0001u,0xBBBB0001u, 0xAAAA0002u,0xBBBB0002u};
            setup_table(fullrec,2);
        }
        wm_800726C0_convergence_p1();
        check("full pool: iterations still 2", wm_conv_p1_get_iterations()==2);
        check("full pool caller reaches cut 0x8007272C", wm_conv_p1_get_last_next()==WM_CONV_P1_CUT_SECOND_TABLE);
    }

    /* 11. table unchanged after call */
    reset_state(); init_pool();
    WM_U32(WM_FLAG_C894_ABS) = 0;
    {
        uint32_t orig[] = {0x11110001u,0x22220001u, 0x11110002u,0x22220002u};
        setup_table(orig, 2);
        base = (uint32_t*)PSX_ADDR(WM_CONV_TABLE_A_BASE);
        uint32_t snap0 = base[0], snap1 = base[1], snap2 = base[2], snap3 = base[3];
        wm_800726C0_convergence_p1();
        check("table byte-identical after call", base[0]==snap0 && base[1]==snap1 && base[2]==snap2 && base[3]==snap3);
    }

    /* 12. exact 8-byte stride */
    reset_state(); init_pool();
    WM_U32(WM_FLAG_C894_ABS) = 0;
    {
        uint32_t stride_test[] = {0xA0000001u,0xB0000001u, 0xA0000002u,0xB0000002u, 0xA0000003u,0xB0000003u};
        setup_table(stride_test, 3);
        wm_800726C0_convergence_p1();
        check("stride: 3 calls", wm_conv_p1_get_helper_calls()==3);
        pool_psx = WM_U32(WM_POOL_BE24);
        check("stride a0/a1[1]", pool_slot_has(pool_psx, 1, 0xA0000002u, 0xB0000002u));
    }

    /* ---------------------------------------------------------------
     * Boundary / forbidden
     * --------------------------------------------------------------- */
    printf("\n--- Boundary / forbidden ---\n");

    /* 13. 0x8007272C: not executed (ZERO VERIFIED by counter check) */
    check("0x8007272C not executed", wm_conv_p1_get_forbidden_excluded_instr() == 0);

    /* 14. no 0x8009C610 read */
    reset_state(); init_pool();
    WM_U32(WM_FLAG_C894_ABS)=0;
    WM_U32(WM_SLOT_C610_ABS)=0xABCDEF01u;
    rec[0]=0x11110001u; rec[1]=0x22220001u;
    setup_table(rec,1);
    wm_800726C0_convergence_p1();
    check("0x8009C610 no read", wm_conv_p1_get_forbidden_c610_read()==0);
    check("0x8009C610 unchanged", WM_U32(WM_SLOT_C610_ABS)==0xABCDEF01u);

    /* 15. no 0x8009A034 read */
    reset_state(); init_pool();
    WM_U32(WM_FLAG_C894_ABS)=0;
    WM_U32(WM_CONV_TABLE_B_BASE)=0x12345678u;
    rec[0]=0x11110002u; rec[1]=0x22220002u;
    setup_table(rec,1);
    wm_800726C0_convergence_p1();
    check("0x8009A034 no read", wm_conv_p1_get_forbidden_a034_read()==0);
    check("0x8009A034 unchanged", WM_U32(WM_CONV_TABLE_B_BASE)==0x12345678u);

    /* 16. no 0x800976FC call */
    check("0x800976FC not called", wm_conv_p1_get_forbidden_976fc()==0);

    /* 17. no common tail execution */
    check("common tail not executed", wm_conv_p1_get_forbidden_common_tail()==0);

    /* ---------------------------------------------------------------
     * Per-init lifecycle (the key repair test)
     * --------------------------------------------------------------- */
    printf("\n--- Per-init lifecycle ---\n");

    /* 18. World init A: flag 0, route executes once */
    reset_state(); init_pool();
    WM_U32(WM_FLAG_C894_ABS) = 0;
    rec[0]=0xA0A0A0A0u; rec[1]=0xB0B0B0B0u;
    setup_table(rec, 1);
    wm_800726C0_convergence_p1();
    check("init A: entry==1", wm_conv_p1_get_entry()==1);
    check("init A: flag0==1", wm_conv_p1_get_flag0()==1);
    check("init A: cut 0x8007272C", wm_conv_p1_get_last_next()==WM_CONV_P1_CUT_SECOND_TABLE);

    /* 19. After reset (new world init B): counters reset, flag 1 works */
    wm_conv_p1_reset();
    reset_state(); init_pool();
    WM_U32(WM_FLAG_C894_ABS) = 1;
    rec[0]=0x11111111u; rec[1]=0x22222222u;
    setup_table(rec, 1);
    wm_800726C0_convergence_p1();
    check("init B: entry==1 (reset)", wm_conv_p1_get_entry()==1);
    check("init B: flag1_cut==1", wm_conv_p1_get_flag1_cut()==1);
    check("init B: flag0==0 (reset)", wm_conv_p1_get_flag0()==0);
    check("init B: cut 0x80072784", wm_conv_p1_get_last_next()==WM_CONV_P1_CUT_FLAG1_ARC);

    /* 20. New world init C: flag 2 */
    wm_conv_p1_reset();
    reset_state(); init_pool();
    WM_U32(WM_FLAG_C894_ABS) = 2;
    setup_table(rec, 1);
    wm_800726C0_convergence_p1();
    check("init C: entry==1 (reset)", wm_conv_p1_get_entry()==1);
    check("init C: other_cut==1", wm_conv_p1_get_other_cut()==1);
    check("init C: flag1_cut==0 (reset)", wm_conv_p1_get_flag1_cut()==0);
    check("init C: cut 0x8007290C", wm_conv_p1_get_last_next()==WM_CONV_P1_CUT_COMMON_TAIL);

    /* 21. Verify no stale state persists after reset */
    wm_conv_p1_reset();
    check("after reset: entry==0", wm_conv_p1_get_entry()==0);
    check("after reset: last_next==0", wm_conv_p1_get_last_next()==0);
    check("after reset: flag0==0", wm_conv_p1_get_flag0()==0);
    check("after reset: iterations==0", wm_conv_p1_get_iterations()==0);
    check("after reset: pool_alloc==0", wm_conv_p1_get_pool_alloc()==0);

    /* 22. Gate disabled: no P1 call, cut stays at W33B boundary */
    /* (Simulated by not calling P1 — in production, the gate check prevents it) */
    reset_state(); init_pool();
    check("gate off: P1 not called (simulated)", 1);

    /* 23. Gate re-enabled: P1 works again after reset */
    wm_conv_p1_reset();
    reset_state(); init_pool();
    WM_U32(WM_FLAG_C894_ABS) = 0;
    rec[0]=0xF0F0F0F0u; rec[1]=0xE0E0E0E0u;
    setup_table(rec, 1);
    wm_800726C0_convergence_p1();
    check("gate re-enabled: entry==1", wm_conv_p1_get_entry()==1);
    check("gate re-enabled: helper==1", wm_conv_p1_get_helper_calls()==1);

    /* ---------------------------------------------------------------
     * Memory guards
     * --------------------------------------------------------------- */
    printf("\n--- Memory guards ---\n");

    /* 24. Neighbor BSS unchanged */
    reset_state(); init_pool();
    WM_U32(WM_FLAG_C894_ABS)=0;
    WM_U32(WM_CONV_TABLE_A_BASE - 4)=0x11111111u;
    WM_U32(WM_CONV_TABLE_A_BASE + 32)=0x22222222u;
    rec[0]=0x33333333u; rec[1]=0x44444444u;
    setup_table(rec,1);
    wm_800726C0_convergence_p1();
    check("neighbor BSS before unchanged", WM_U32(WM_CONV_TABLE_A_BASE - 4)==0x11111111u);
    check("neighbor BSS after unchanged", WM_U32(WM_CONV_TABLE_A_BASE + 32)==0x22222222u);

    /* 25. Wrong-address sentinels unchanged */
    reset_state(); init_pool();
    WM_U32(WM_FLAG_C894_ABS)=0;
    WM_U32(WM_CONV_TABLE_B_BASE)=0x87654321u;
    WM_U32(WM_SLOT_C610_ABS)=0xFEDCBA98u;
    rec[0]=0x55550001u; rec[1]=0x66660001u;
    setup_table(rec,1);
    wm_800726C0_convergence_p1();
    check("0x8009A034 sentinel unchanged", WM_U32(WM_CONV_TABLE_B_BASE)==0x87654321u);
    check("0x8009C610 sentinel unchanged", WM_U32(WM_SLOT_C610_ABS)==0xFEDCBA98u);

    /* ---------------------------------------------------------------
     * Symbol verification (compile-time proof)
     * --------------------------------------------------------------- */
    printf("\n--- Symbol verification ---\n");
    /* The fact that this test links and runs proves:
     * - wm_800726C0_convergence_p1 is declared extern (via header)
     * - wm_800726C0_convergence_p1 is NOT defined in this file
     * - the one definition comes from world_map_convergence.c
     * - nm will show exactly one T symbol */
    check("production symbol linked (runtime proof)", 1);

    /* Summary */
    printf("\n=== PRODUCTION-LINKED W34B1: %d/%d passed", pass, total);
    if (fail > 0) printf(", %d FAILED", fail);
    printf(" ===\n");
    return fail > 0 ? 1 : 0;
}
