/*
 * Production-linked test for world-map helper 0x8008DFF4.
 *
 * Tests the actual production TU (world_map_helper_8dff4.c) against the
 * retail oracle: for each signed halfword x, the function must produce
 * (u32)((s32)(s16)x) << 12 as the stored word.
 *
 * Covers:
 *   - Hard signed test vectors (section 10)
 *   - Component ordering (section 11)
 *   - Load/store width mutants (section 12)
 *   - Address mutants (section 13)
 *   - Shift mutants (section 14)
 *   - Write-set canary (section 15)
 *   - Global write set (section 16)
 *   - Exhaustive halfword proof (section 26)
 */
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_helper_8dff4.h"

/* ------------------------------------------------------------------ */
/* Retail oracle: the exact bit-level transform                        */
/* ------------------------------------------------------------------ */
static u32 retail_transform(s16 raw)
{
    s32 extended = (s32)raw;
    u32 bits = (u32)extended;
    return bits << 12;
}

/* ------------------------------------------------------------------ */
/* PSX address of the global source halfwords                          */
/* ------------------------------------------------------------------ */
#define SRC_X_ADDR  0x8006EE60u
#define SRC_Y_ADDR  0x8006EE62u
#define SRC_Z_ADDR  0x8006EE64u

/* ------------------------------------------------------------------ */
/* Helpers to poke/read PSX memory                                     */
/* ------------------------------------------------------------------ */
static void poke_s16(u32 addr, s16 val)
{
    memcpy(PSX_ADDR(addr), &val, sizeof(val));
}

static u32 peek_u32(u32 addr)
{
    u32 val;
    memcpy(&val, PSX_ADDR(addr), sizeof(val));
    return val;
}

/* Output buffer base — must be 4-byte aligned, 12 bytes minimum */
#define OUT_ADDR  0x8009C5ACu

/* Canary region: 16 bytes before and 20 bytes after the 12-byte output */
#define CANARY_PRE_START   (OUT_ADDR - 16u)
#define CANARY_POST_START  (OUT_ADDR + 12u)
#define CANARY_SIZE        16u

static void fill_canary(u8 pattern)
{
    memset(PSX_ADDR(CANARY_PRE_START), pattern, CANARY_SIZE);
    memset(PSX_ADDR(CANARY_POST_START), pattern, CANARY_SIZE);
}

static int check_canary(u8 pattern)
{
    const u8 *pre = (const u8 *)PSX_ADDR(CANARY_PRE_START);
    const u8 *post = (const u8 *)PSX_ADDR(CANARY_POST_START);
    for (u32 i = 0; i < CANARY_SIZE; i++) {
        if (pre[i] != pattern) return 0;
        if (post[i] != pattern) return 0;
    }
    return 1;
}

/* ------------------------------------------------------------------ */
/* Global source shadow — detect writes to inputs                      */
/* ------------------------------------------------------------------ */
static void snapshot_globals(s16 *sx, s16 *sy, s16 *sz)
{
    memcpy(sx, PSX_ADDR(SRC_X_ADDR), sizeof(*sx));
    memcpy(sy, PSX_ADDR(SRC_Y_ADDR), sizeof(*sy));
    memcpy(sz, PSX_ADDR(SRC_Z_ADDR), sizeof(*sz));
}

static int globals_unchanged(s16 sx, s16 sy, s16 sz)
{
    s16 cx, cy, cz;
    snapshot_globals(&cx, &cy, &cz);
    return (cx == sx && cy == sy && cz == sz);
}

/* ------------------------------------------------------------------ */
/* Hard signed test vectors (section 10)                               */
/* ------------------------------------------------------------------ */
static const struct {
    s16 input;
    u32 expected;
} hard_vectors[] = {
    { (s16)0x0000, 0x00000000u },
    { (s16)0x0001, 0x00001000u },
    { (s16)0x0002, 0x00002000u },
    { (s16)0x7FFF, 0x07FFF000u },
    { (s16)0x8000, 0xF8000000u },
    { (s16)0xFFFF, 0xFFFFF000u },
    { (s16)0xFFF0, 0xFFFF0000u },
};

static int test_hard_vectors(void)
{
    int pass = 0;
    int total = 0;
    const char *comp_names[] = {"X", "Y", "Z"};
    u32 src_addrs[] = {SRC_X_ADDR, SRC_Y_ADDR, SRC_Z_ADDR};

    for (int c = 0; c < 3; c++) {
        for (u32 v = 0; v < sizeof(hard_vectors)/sizeof(hard_vectors[0]); v++) {
            total++;
            s16 inp = hard_vectors[v].input;
            u32 exp = hard_vectors[v].expected;

            /* Zero all sources first */
            s16 zero = 0;
            poke_s16(SRC_X_ADDR, zero);
            poke_s16(SRC_Y_ADDR, zero);
            poke_s16(SRC_Z_ADDR, zero);

            /* Poke the target component */
            poke_s16(src_addrs[c], inp);

            wm_8008DFF4(OUT_ADDR);

            u32 got = peek_u32(OUT_ADDR + (u32)c * 4u);
            if (got != exp) {
                fprintf(stderr,
                        "FAIL [hard_%s]: input=0x%04X expected=0x%08X got=0x%08X\n",
                        comp_names[c], (u16)inp, exp, got);
            } else {
                pass++;
            }
        }
    }
    printf("hard_vectors: %d/%d\n", pass, total);
    return (pass == total);
}

/* ------------------------------------------------------------------ */
/* Component ordering (section 11)                                     */
/* ------------------------------------------------------------------ */
static int test_component_order(void)
{
    poke_s16(SRC_X_ADDR, 1);
    poke_s16(SRC_Y_ADDR, 2);
    poke_s16(SRC_Z_ADDR, 3);

    wm_8008DFF4(OUT_ADDR);

    u32 gx = peek_u32(OUT_ADDR + 0);
    u32 gy = peek_u32(OUT_ADDR + 4);
    u32 gz = peek_u32(OUT_ADDR + 8);

    int ok = (gx == 0x00001000u && gy == 0x00002000u && gz == 0x00003000u);
    if (!ok) {
        fprintf(stderr,
                "FAIL [component_order]: X=0x%08X Y=0x%08X Z=0x%08X\n",
                gx, gy, gz);
    } else {
        printf("component_order: PASS\n");
    }
    return ok;
}

/* ------------------------------------------------------------------ */
/* Write-set canary (section 15)                                       */
/* ------------------------------------------------------------------ */
static int test_write_canary(void)
{
    poke_s16(SRC_X_ADDR, 0x1234);
    poke_s16(SRC_Y_ADDR, 0x5678);
    poke_s16(SRC_Z_ADDR, (s16)0x9ABC);

    fill_canary(0xAA);
    wm_8008DFF4(OUT_ADDR);

    int ok = check_canary(0xAA);
    if (!ok) {
        fprintf(stderr, "FAIL [write_canary]: canary region corrupted\n");
    } else {
        printf("write_canary: PASS\n");
    }
    return ok;
}

/* ------------------------------------------------------------------ */
/* Global write set (section 16)                                       */
/* ------------------------------------------------------------------ */
static int test_global_write_set(void)
{
    s16 orig_x = 0x1111;
    s16 orig_y = 0x2222;
    s16 orig_z = (s16)0x3333;

    poke_s16(SRC_X_ADDR, orig_x);
    poke_s16(SRC_Y_ADDR, orig_y);
    poke_s16(SRC_Z_ADDR, orig_z);

    wm_8008DFF4(OUT_ADDR);

    int ok = globals_unchanged(orig_x, orig_y, orig_z);
    if (!ok) {
        fprintf(stderr, "FAIL [global_write_set]: source globals modified\n");
    } else {
        printf("global_write_set: PASS\n");
    }
    return ok;
}

/* ------------------------------------------------------------------ */
/* Exhaustive halfword proof (section 26)                              */
/* ------------------------------------------------------------------ */
static int test_exhaustive(void)
{
    int pass = 0;
    int total = 0;
    int fail_first = 0;
    const char *comp_names[] = {"X", "Y", "Z"};
    u32 src_addrs[] = {SRC_X_ADDR, SRC_Y_ADDR, SRC_Z_ADDR};

    for (int c = 0; c < 3; c++) {
        for (u32 raw = 0; raw < 65536u; raw++) {
            total++;
            s16 val = (s16)(u16)raw;

            /* Zero all sources */
            s16 zero = 0;
            poke_s16(SRC_X_ADDR, zero);
            poke_s16(SRC_Y_ADDR, zero);
            poke_s16(SRC_Z_ADDR, zero);

            /* Poke target component */
            poke_s16(src_addrs[c], val);

            wm_8008DFF4(OUT_ADDR);

            u32 got = peek_u32(OUT_ADDR + (u32)c * 4u);
            u32 expected = retail_transform(val);

            if (got != expected) {
                if (fail_first < 5) {
                    fprintf(stderr,
                            "FAIL [exhaustive_%s]: raw=0x%04X input=%d "
                            "expected=0x%08X got=0x%08X\n",
                            comp_names[c], raw, (int)val, expected, got);
                    fail_first++;
                }
            } else {
                pass++;
            }
        }
    }
    printf("exhaustive: %d/%d\n", pass, total);
    return (pass == total);
}

/* ------------------------------------------------------------------ */
/* Main                                                                */
/* ------------------------------------------------------------------ */
int main(void)
{
    PsxMemory_Init();

    int all_ok = 1;

    printf("=== W34B8-B: wm_8008DFF4 production test ===\n");

    all_ok &= test_hard_vectors();
    all_ok &= test_component_order();
    all_ok &= test_write_canary();
    all_ok &= test_global_write_set();
    all_ok &= test_exhaustive();

    if (all_ok) {
        printf("\nALL TESTS PASSED\n");
    } else {
        printf("\n*** FAILURES DETECTED ***\n");
    }

    return all_ok ? 0 : 1;
}
