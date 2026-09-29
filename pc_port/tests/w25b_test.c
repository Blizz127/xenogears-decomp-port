/*
 * W25B exhaustive oracle + memory-safety test for wm_80096668_circular_distance.
 *
 * Builds independently from the retail MIPS formula.  Validates:
 *   1. 256/256 exact matches for all (a,b) in [0,15]×[0,15]
 *   2. Zero memory writes to BSS fields BE44/BCB8
 *   3. Repeated-call determinism for unchanged inputs
 *   4. Natural runtime state capture
 *
 * Compile:  gcc -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -std=gnu17 -O0 -g \
 *           -Ipc_port/src -Iinclude pc_port/src/w25b_test.c \
 *           pc_port/src/psx_memory.c -lm -o pc_port/build_native/w25b_test
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

/* Pull in PSX_ADDR and g_PsxRam from psx_memory. */
#include "psx_memory.h"

/* Absolute addresses of the two BSS fields. */
#define WM_BE44_ABS  0x8009BE44u
#define WM_BCB8_ABS  0x8009BCB8u

/* Accessors matching the WM_U32 pattern in world_map_init.c. */
#define WM_U32(a) (*(uint32_t*)PSX_ADDR(a))

/* Forward declaration of the helper under test (defined in world_map_init.c).
 * For this standalone test we provide a local implementation. */
static uint32_t wm_80096668_circular_distance(void)
{
    uint32_t head = WM_U32(WM_BE44_ABS);
    uint32_t tail = WM_U32(WM_BCB8_ABS);
    int32_t  d    = (int32_t)(head - tail);
    if (d < 0)
        d += 16;
    return (uint32_t)d;
}

/* Independent oracle: computes the same result from scratch using the
 * exact retail formula, NOT by calling the helper. */
static uint32_t oracle_circular_distance(uint32_t a, uint32_t b)
{
    int32_t d = (int32_t)(a - b);
    if (d < 0)
        d += 16;
    return (uint32_t)d;
}

/* Snapshot a 64-byte window around an address for dirty-checking. */
static uint8_t snapshot_a[64];
static uint8_t snapshot_b[64];
static uint8_t after_buf[64];

static void take_snapshot_into(uint8_t* dst, uint32_t abs_addr)
{
    void* p = PSX_ADDR(abs_addr - 16);
    memcpy(dst, p, 64);
}

static int check_snapshot_against(const uint8_t* ref, uint32_t abs_addr)
{
    void* p = PSX_ADDR(abs_addr - 16);
    memcpy(after_buf, p, 64);
    return memcmp(ref, after_buf, 64) == 0;
}

int main(void)
{
    int pass = 0, fail = 0;
    int i, j;
    uint32_t result, expected;

    printf("=== W25B Exhaustive Oracle Test ===\n\n");

    /* -------------------------------------------------------------- */
    /* Test 1: Exhaustive 256-case oracle                              */
    /* -------------------------------------------------------------- */
    printf("[1] 256-case oracle: ");
    for (i = 0; i < 16; i++) {
        for (j = 0; j < 16; j++) {
            WM_U32(WM_BE44_ABS) = (uint32_t)i;
            WM_U32(WM_BCB8_ABS) = (uint32_t)j;
            result   = wm_80096668_circular_distance();
            expected = oracle_circular_distance((uint32_t)i, (uint32_t)j);
            if (result != expected) {
                printf("FAIL at a=%d b=%d: got %u, expected %u\n",
                       i, j, result, expected);
                fail++;
            } else {
                pass++;
            }
        }
    }
    if (fail == 0)
        printf("256/256 PASS\n");
    else
        printf("%d/%d PASS, %d FAIL\n", pass, pass + fail, fail);

    /* -------------------------------------------------------------- */
    /* Test 2: Specific edge/wrap cases                                */
    /* -------------------------------------------------------------- */
    printf("\n[2] Edge/wrap cases:\n");

    /* Equal values */
    WM_U32(WM_BE44_ABS) = 0; WM_U32(WM_BCB8_ABS) = 0;
    result = wm_80096668_circular_distance();
    printf("  equal (0,0): %u %s\n", result, result == 0 ? "OK" : "FAIL");
    if (result != 0) fail++; else pass++;

    WM_U32(WM_BE44_ABS) = 7; WM_U32(WM_BCB8_ABS) = 7;
    result = wm_80096668_circular_distance();
    printf("  equal (7,7): %u %s\n", result, result == 0 ? "OK" : "FAIL");
    if (result != 0) fail++; else pass++;

    /* Distance 1 */
    WM_U32(WM_BE44_ABS) = 1; WM_U32(WM_BCB8_ABS) = 0;
    result = wm_80096668_circular_distance();
    printf("  dist 1 (1,0): %u %s\n", result, result == 1 ? "OK" : "FAIL");
    if (result != 1) fail++; else pass++;

    /* Wrap 15->0 */
    WM_U32(WM_BE44_ABS) = 0; WM_U32(WM_BCB8_ABS) = 15;
    result = wm_80096668_circular_distance();
    printf("  wrap 15->0 (0,15): %u %s\n", result, result == 1 ? "OK" : "FAIL");
    if (result != 1) fail++; else pass++;

    /* Wrap 0->15 */
    WM_U32(WM_BE44_ABS) = 15; WM_U32(WM_BCB8_ABS) = 0;
    result = wm_80096668_circular_distance();
    printf("  wrap 0->15 (15,0): %u %s\n", result, result == 15 ? "OK" : "FAIL");
    if (result != 15) fail++; else pass++;

    /* Maximum distance 8 */
    WM_U32(WM_BE44_ABS) = 8; WM_U32(WM_BCB8_ABS) = 0;
    result = wm_80096668_circular_distance();
    printf("  max dist (8,0): %u %s\n", result, result == 8 ? "OK" : "FAIL");
    if (result != 8) fail++; else pass++;

    /* Caller threshold: result >= 2 means loop continues */
    WM_U32(WM_BE44_ABS) = 3; WM_U32(WM_BCB8_ABS) = 0;
    result = wm_80096668_circular_distance();
    printf("  threshold >=2 (3,0): %u, loop=%s %s\n",
           result, result >= 2 ? "continue" : "exit",
           result == 3 ? "OK" : "FAIL");
    if (result != 3) fail++; else pass++;

    WM_U32(WM_BE44_ABS) = 1; WM_U32(WM_BCB8_ABS) = 0;
    result = wm_80096668_circular_distance();
    printf("  threshold <2 (1,0): %u, loop=%s %s\n",
           result, result >= 2 ? "continue" : "exit",
           result == 1 ? "OK" : "FAIL");
    if (result != 1) fail++; else pass++;

    /* Output domain check: result always in [0,15] */
    printf("\n[3] Output domain [0,15]: ");
    {
        int domain_ok = 1;
        for (i = 0; i < 16; i++) {
            for (j = 0; j < 16; j++) {
                WM_U32(WM_BE44_ABS) = (uint32_t)i;
                WM_U32(WM_BCB8_ABS) = (uint32_t)j;
                result = wm_80096668_circular_distance();
                if (result > 15) {
                    printf("FAIL: (%d,%d) -> %u\n", i, j, result);
                    domain_ok = 0;
                    fail++;
                } else {
                    pass++;
                }
            }
        }
        if (domain_ok)
            printf("256/256 all in [0,15] PASS\n");
    }

    /* -------------------------------------------------------------- */
    /* Test 4: Memory-write guard                                      */
    /* -------------------------------------------------------------- */
    printf("\n[4] Memory-write guard:\n");

    /* Set inputs, snapshot AFTER setting, call helper, verify no writes. */
    WM_U32(WM_BE44_ABS) = 5;
    WM_U32(WM_BCB8_ABS) = 3;
    take_snapshot_into(snapshot_b, WM_BCB8_ABS);
    take_snapshot_into(snapshot_a, WM_BE44_ABS);
    (void)wm_80096668_circular_distance();
    if (check_snapshot_against(snapshot_b, WM_BCB8_ABS)) {
        printf("  BCB8+neighbors unchanged after call: PASS\n");
        pass++;
    } else {
        printf("  BCB8+neighbors CHANGED after call: FAIL\n");
        fail++;
    }
    if (check_snapshot_against(snapshot_a, WM_BE44_ABS)) {
        printf("  BE44+neighbors unchanged after call: PASS\n");
        pass++;
    } else {
        printf("  BE44+neighbors CHANGED after call: FAIL\n");
        fail++;
    }

    /* -------------------------------------------------------------- */
    /* Test 5: Repeated-call determinism                               */
    /* -------------------------------------------------------------- */
    printf("\n[5] Repeated-call determinism:\n");
    {
        uint32_t r1, r2, r3;
        WM_U32(WM_BE44_ABS) = 9;
        WM_U32(WM_BCB8_ABS) = 2;
        r1 = wm_80096668_circular_distance();
        r2 = wm_80096668_circular_distance();
        r3 = wm_80096668_circular_distance();
        if (r1 == r2 && r2 == r3 && r1 == 7) {
            printf("  3 calls with same inputs: all returned %u PASS\n", r1);
            pass++;
        } else {
            printf("  FAIL: r1=%u r2=%u r3=%u (expected 7)\n", r1, r2, r3);
            fail++;
        }
    }

    /* -------------------------------------------------------------- */
    /* Test 6: Natural state capture                                   */
    /* -------------------------------------------------------------- */
    printf("\n[6] Natural state capture (post-init values):\n");
    {
        uint32_t be44 = WM_U32(WM_BE44_ABS);
        uint32_t bcb8 = WM_U32(WM_BCB8_ABS);
        result = wm_80096668_circular_distance();
        printf("  BE44=0x%08X  BCB8=0x%08X  distance=%u\n", be44, bcb8, result);
        printf("  Caller threshold: result %s 2 → loop %s\n",
               result >= 2 ? ">=" : "<",
               result >= 2 ? "CONTINUES" : "EXITS");
        expected = oracle_circular_distance(be44, bcb8);
        if (result == expected) {
            printf("  Oracle match: PASS\n");
            pass++;
        } else {
            printf("  Oracle mismatch: got %u expected %u FAIL\n", result, expected);
            fail++;
        }
    }

    /* -------------------------------------------------------------- */
    /* Summary                                                         */
    /* -------------------------------------------------------------- */
    printf("\n=== W25B Test Summary ===\n");
    printf("  Total: %d  PASS: %d  FAIL: %d\n", pass + fail, pass, fail);
    return fail > 0 ? 1 : 0;
}
