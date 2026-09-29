/* Production-linked certificate for retail wm_80094028 and wm_80075E7C. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_helper_75e7c.h"

#define VEC              0x8009D55Cu
#define CELL             0x800A8000u
#define AREA_REMAP       0x8009A3A0u
#define THRESHOLDS       0x8009B57Au
#define RECORD_BASES     0x8009D73Cu
#define BASE_TWO         0x800B0000u
#define BASE_FIVE        0x800B1000u
#define BASE_SIX         0x800B2000u

u8 D_800658DC[0x210];
u8 D_80059508;

static s32 s_cell_x;
static s32 s_cell_z;
static u32 s_cell_result;
static int s_cell_calls;
static s32 s_area_result;
static u32 s_area_arg;
static int s_area_calls;
static int s_rand_value;
static int s_rand_calls;
static u8 *s_guest_before;

static void wr16(u32 address, u16 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static void wr32(u32 address, u32 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static int expect_true(const char *assertion, int condition)
{
    if (condition == 0) {
        fprintf(stderr, "ASSERTION %s\n", assertion);
        return 0;
    }
    return 1;
}

static int expect_int(const char *assertion, s32 expected, s32 actual)
{
    if (expected != actual) {
        fprintf(stderr, "ASSERTION %s expected=%d actual=%d\n",
                assertion, (int)expected, (int)actual);
        return 0;
    }
    return 1;
}

u32 wm_80093660(s32 x, s32 z)
{
    s_cell_x = x;
    s_cell_z = z;
    s_cell_calls++;
    return s_cell_result;
}

s32 wm_80093F18(u32 vec_addr)
{
    s_area_arg = vec_addr;
    s_area_calls++;
    return s_area_result;
}

int rand(void)
{
    s_rand_calls++;
    return s_rand_value;
}

static void reset_fixture(void)
{
    u32 i;

    memset(g_PsxRam, 0, PSX_RAM_SIZE);
    memset(g_PsxScratchpad, 0, 4096u);
    memset(D_800658DC, 0xCC, sizeof(D_800658DC));
    D_80059508 = 0xA5u;
    s_cell_x = 0;
    s_cell_z = 0;
    s_cell_result = CELL;
    s_cell_calls = 0;
    s_area_result = 3;
    s_area_arg = 0u;
    s_area_calls = 0;
    s_rand_value = 0;
    s_rand_calls = 0;
    wr32(VEC, 0xFFF00000u);
    wr32(VEC + 8u, 0x12345678u);
    *(u8 *)PSX_ADDR(CELL + 2u) = 0x10u;
    *(u8 *)PSX_ADDR(CELL + 3u) = 0x08u; /* selector 2 */
    wr16(THRESHOLDS + 0u, 100u);
    wr16(THRESHOLDS + 2u, 200u);
    wr16(THRESHOLDS + 4u, 300u);
    wr16(THRESHOLDS + 6u, 0xFFFFu);
    wr32(RECORD_BASES + 2u * 4u, BASE_TWO);
    wr32(RECORD_BASES + 5u * 4u, BASE_FIVE);
    wr32(RECORD_BASES + 6u * 4u, BASE_SIX);
    wr16(AREA_REMAP + 2u * 2u, 5u);
    for (i = 0u; i < 0x200u; i++) {
        *(u8 *)PSX_ADDR(BASE_TWO + i) = (u8)(i ^ 0x5Au);
        *(u8 *)PSX_ADDR(BASE_FIVE + i) = (u8)(i ^ 0xA5u);
        *(u8 *)PSX_ADDR(BASE_SIX + i) = (u8)(i ^ 0x3Cu);
    }
    /* Bucket 0 distinguishes the threshold and remap paths. */
    *(u8 *)PSX_ADDR(BASE_TWO + 0x200u + 1u) = 1u;
    *(u8 *)PSX_ADDR(BASE_FIVE + 0x200u + 7u) = 1u;
    /* Bucket 1 weights: [0,2,0,3,1]. */
    *(u8 *)PSX_ADDR(BASE_TWO + 0x210u + 1u) = 2u;
    *(u8 *)PSX_ADDR(BASE_TWO + 0x210u + 3u) = 3u;
    *(u8 *)PSX_ADDR(BASE_TWO + 0x210u + 4u) = 1u;
}

static int guest_is_unchanged(const char *assertion)
{
    return expect_true(assertion,
                       memcmp(s_guest_before, g_PsxRam, PSX_RAM_SIZE) == 0);
}

static int test_cell_selector(void)
{
    int ok = 1;
    s32 got;

    reset_fixture();
    *(u8 *)PSX_ADDR(CELL + 3u) = 0xACu;
    got = wm_80094028(VEC);
    ok &= expect_int("selector.cell.byte3.bitfield", 11, got);
    ok &= expect_int("selector.cell.lookup.once", 1, s_cell_calls);
    ok &= expect_true("selector.vector.loads.signed.xz",
                      (u32)s_cell_x == 0xFFF00000u &&
                      (u32)s_cell_z == 0x12345678u);
    return ok;
}

static int run_weighted_case(int random_value, u8 expected)
{
    int ok = 1;
    s32 rc;

    reset_fixture();
    s_rand_value = random_value;
    memcpy(s_guest_before, g_PsxRam, PSX_RAM_SIZE);
    rc = wm_80075E7C(VEC, 100);
    ok &= expect_int("success.return.one", 1, rc);
    ok &= expect_int("threshold.retail.bucket", (s32)expected,
                     (s32)D_80059508);
    ok &= expect_int("weighted.rand.boundaries", (s32)expected,
                     (s32)D_80059508);
    ok &= expect_int("success.rand.once", 1, s_rand_calls);
    ok &= expect_true("success.copies.base.0x200",
                      memcmp(D_800658DC, PSX_ADDR(BASE_TWO), 0x200u) == 0);
    ok &= expect_true("success.copy.exact.0x200",
                      D_800658DC[0x200] == 0xCCu &&
                      D_800658DC[0x20F] == 0xCCu);
    ok &= expect_true("success.selector.inputs",
                      s_cell_calls == 1 && s_area_calls == 1 &&
                      s_area_arg == VEC);
    ok &= guest_is_unchanged("success.guest.input.read.only");
    return ok;
}

static int test_weighted_selection(void)
{
    int ok = 1;
    ok &= run_weighted_case(0, 1u);
    ok &= run_weighted_case(1, 1u);
    ok &= run_weighted_case(2, 3u);
    ok &= run_weighted_case(4, 3u);
    ok &= run_weighted_case(5, 4u);
    return ok;
}

static int test_area_remap(void)
{
    int ok = 1;
    s32 rc;

    reset_fixture();
    s_area_result = 4;
    memcpy(s_guest_before, g_PsxRam, PSX_RAM_SIZE);
    rc = wm_80075E7C(VEC, -1);
    ok &= expect_int("transition.area4.remaps.table", 1, rc);
    ok &= expect_int("transition.area4.selected.remapped.record", 7,
                     (s32)D_80059508);
    ok &= expect_true("transition.area4.copies.remapped.base",
                      memcmp(D_800658DC, PSX_ADDR(BASE_FIVE), 0x200u) == 0);
    ok &= guest_is_unchanged("transition.area4.guest.read.only");
    return ok;
}

static int test_zero_sum(void)
{
    int ok = 1;
    s32 rc;
    u8 before[sizeof(D_800658DC)];

    reset_fixture();
    *(u8 *)PSX_ADDR(CELL + 3u) = 0x18u; /* selector 6 */
    memcpy(before, D_800658DC, sizeof(before));
    memcpy(s_guest_before, g_PsxRam, PSX_RAM_SIZE);
    rc = wm_80075E7C(VEC, -1);
    ok &= expect_int("zero.sum.no.publication", 0, rc);
    ok &= expect_int("zero.sum.no.rand", 0, s_rand_calls);
    ok &= expect_int("zero.sum.selected.unchanged", 0xA5,
                     (s32)D_80059508);
    ok &= expect_true("zero.sum.destination.unchanged",
                      memcmp(before, D_800658DC, sizeof(before)) == 0);
    ok &= guest_is_unchanged("zero.sum.guest.read.only");
    return ok;
}

int main(void)
{
    int ok = 1;

    s_guest_before = malloc(PSX_RAM_SIZE);
    if (s_guest_before == NULL)
        return EXIT_FAILURE;
    ok &= test_cell_selector();
    ok &= test_weighted_selection();
    ok &= test_area_remap();
    ok &= test_zero_sum();
    free(s_guest_before);
    if (ok == 0)
        return EXIT_FAILURE;
    puts("W34N18 TRANSITION SELECTOR CERTIFICATE PASS");
    return EXIT_SUCCESS;
}
