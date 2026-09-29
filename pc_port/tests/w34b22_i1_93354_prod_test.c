/* Focused production-linked oracle for retail world helper 0x80093354. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_helper_93354.h"

#define WRAP_X     0x8009D160u
#define WRAP_Z     0x8009D2B4u
#define VEC_ADDR   0x800A2000u
#define VEC_X      (VEC_ADDR + 0u)
#define VEC_Y      (VEC_ADDR + 4u)
#define VEC_Z      (VEC_ADDR + 8u)

typedef struct AccessEvent {
    u32 address;
    u32 value;
} AccessEvent;

static AccessEvent s_loads[16];
static AccessEvent s_stores[16];
static u32 s_load_count;
static u32 s_store_count;
static int s_failures;

void wm_93354_test_load(u32 address, u32 value)
{
    if (s_load_count < 16u) {
        s_loads[s_load_count].address = address;
        s_loads[s_load_count].value = value;
    }
    s_load_count++;
}

void wm_93354_test_store(u32 address, u32 value)
{
    if (s_store_count < 16u) {
        s_stores[s_store_count].address = address;
        s_stores[s_store_count].value = value;
    }
    s_store_count++;
}

static void check_u32(const char *name, u32 got, u32 expected)
{
    if (got != expected) {
        fprintf(stderr, "ASSERTION %s: got=0x%08x expected=0x%08x\n",
                name, got, expected);
        s_failures++;
    }
}

static void poke_u32(u32 address, u32 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static u32 peek_u32(u32 address)
{
    u32 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static u32 bits_from_s32(s32 value)
{
    u32 bits;
    memcpy(&bits, &value, sizeof(bits));
    return bits;
}

static s32 s32_from_bits(u32 bits)
{
    s32 value;
    memcpy(&value, &bits, sizeof(value));
    return value;
}

static u32 retail_period(u32 extent)
{
    return extent << 23;
}

static u32 retail_wrap(u32 bits, u32 extent)
{
    u32 period = retail_period(extent);
    s32 coord = s32_from_bits(bits);
    s32 period_s = s32_from_bits(period);

    if (!(coord < period_s))
        bits = bits - period;
    coord = s32_from_bits(bits);
    if (coord < 0)
        bits = bits + period;
    return bits;
}

static void reset_fixture(u32 x, u32 y, u32 z, u32 wrap_x, u32 wrap_z)
{
    memset(g_PsxRam, 0xA5, PSX_RAM_SIZE);
    poke_u32(VEC_X, x);
    poke_u32(VEC_Y, y);
    poke_u32(VEC_Z, z);
    poke_u32(WRAP_X, wrap_x);
    poke_u32(WRAP_Z, wrap_z);
    poke_u32(VEC_ADDR - 4u, 0x11111111u);
    poke_u32(VEC_ADDR + 12u, 0x22222222u);
    poke_u32(WRAP_X - 4u, 0x33333333u);
    poke_u32(WRAP_X + 4u, 0x44444444u);
    poke_u32(WRAP_Z - 4u, 0x55555555u);
    poke_u32(WRAP_Z + 4u, 0x66666666u);
    memset(s_loads, 0, sizeof(s_loads));
    memset(s_stores, 0, sizeof(s_stores));
    s_load_count = 0u;
    s_store_count = 0u;
}

static void check_untouched_neighbors(void)
{
    check_u32("pre-vector canary", peek_u32(VEC_ADDR - 4u), 0x11111111u);
    check_u32("post-vector canary", peek_u32(VEC_ADDR + 12u), 0x22222222u);
    check_u32("wrap-x predecessor", peek_u32(WRAP_X - 4u), 0x33333333u);
    check_u32("wrap-x successor", peek_u32(WRAP_X + 4u), 0x44444444u);
    check_u32("wrap-z predecessor", peek_u32(WRAP_Z - 4u), 0x55555555u);
    check_u32("wrap-z successor", peek_u32(WRAP_Z + 4u), 0x66666666u);
}

static void check_globals_readonly(u32 wrap_x, u32 wrap_z)
{
    check_u32("wrap-x unread-write", peek_u32(WRAP_X), wrap_x);
    check_u32("wrap-z unread-write", peek_u32(WRAP_Z), wrap_z);
}

static void run_case(const char *name, u32 x_bits, u32 y_bits, u32 z_bits,
                     u32 wrap_x, u32 wrap_z, u32 expect_stores)
{
    u32 expect_x = retail_wrap(x_bits, wrap_x);
    u32 expect_z = retail_wrap(z_bits, wrap_z);
    char label[96];

    reset_fixture(x_bits, y_bits, z_bits, wrap_x, wrap_z);
    wm_80093354(VEC_ADDR);

    snprintf(label, sizeof(label), "%s X", name);
    check_u32(label, peek_u32(VEC_X), expect_x);
    snprintf(label, sizeof(label), "%s Y untouched", name);
    check_u32(label, peek_u32(VEC_Y), y_bits);
    snprintf(label, sizeof(label), "%s Z", name);
    check_u32(label, peek_u32(VEC_Z), expect_z);
    snprintf(label, sizeof(label), "%s store count", name);
    check_u32(label, s_store_count, expect_stores);
    check_globals_readonly(wrap_x, wrap_z);
    check_untouched_neighbors();
}

static void test_interior_and_inclusive_bounds(void)
{
    u32 period4 = retail_period(4u);

    /* Interior positives must not wrap; unsigned sltiu vs a negative
     * period is a different case below. */
    run_case("interior", 100u, 0x00C0FFEEu, 200u, 4u, 7u, 0u);
    check_u32("interior load count", s_load_count, 6u);
    check_u32("interior load0 wrap-x", s_loads[0].address, WRAP_X);
    check_u32("interior load1 X", s_loads[1].address, VEC_X);
    check_u32("interior load2 X reload", s_loads[2].address, VEC_X);
    check_u32("interior load3 wrap-z", s_loads[3].address, WRAP_Z);
    check_u32("interior load4 Z", s_loads[4].address, VEC_Z);
    check_u32("interior load5 Z reload", s_loads[5].address, VEC_Z);

    /* slt vs period is exclusive of the stored interior; 0 is not < 0. */
    run_case("x zero inclusive", 0u, 0x13579BDFu, 0u, 9u, 3u, 0u);
    run_case("x period-1 inclusive", period4 - 1u, 0x2468ACE0u,
             bits_from_s32(-100), 4u, 3u, 1u);
    run_case("z zero inclusive", 8u, 0x11111111u, 0u, 5u, 6u, 0u);
}

static void test_single_axis_wraps(void)
{
    u32 period4 = retail_period(4u);
    u32 period9 = retail_period(9u);

    run_case("x low wrap", bits_from_s32(-1), 0x33333333u, 12u, 4u, 9u, 1u);
    check_u32("x low store addr", s_stores[0].address, VEC_X);
    check_u32("x low store value", s_stores[0].value,
              bits_from_s32(-1) + period4);
    check_u32("x low loads wrap-x first", s_loads[0].address, WRAP_X);
    check_u32("x low loads X", s_loads[1].address, VEC_X);
    check_u32("x low reloads X", s_loads[2].address, VEC_X);
    check_u32("x low reloads wrap-x", s_loads[3].address, WRAP_X);

    run_case("x high wrap", period4, 0x44444444u, bits_from_s32(-12),
             4u, 9u, 2u);
    check_u32("x high store addr", s_stores[0].address, VEC_X);
    check_u32("x high store value", s_stores[0].value, 0u);

    run_case("z low wrap", 8u, 0x55555555u, bits_from_s32(-1), 4u, 9u, 1u);
    check_u32("z low store addr", s_stores[0].address, VEC_Z);
    check_u32("z low store value", s_stores[0].value,
              bits_from_s32(-1) + period9);
    check_u32("z low loads wrap-x first", s_loads[0].address, WRAP_X);
    check_u32("z low loads X", s_loads[1].address, VEC_X);

    run_case("z high wrap", bits_from_s32(-8), 0x66666666u, period9,
             4u, 9u, 2u);
    check_u32("z high last store addr", s_stores[1].address, VEC_Z);
    check_u32("z high last store value", s_stores[1].value, 0u);
}

static void test_both_axes_store_order(void)
{
    u32 wrap_x = 5u;
    u32 wrap_z = 6u;
    u32 expect_x = bits_from_s32(-20000) + retail_period(wrap_x);
    u32 expect_z = 0u;

    run_case("both wrap", bits_from_s32(-20000), 0x77777777u,
             retail_period(wrap_z), wrap_x, wrap_z, 2u);
    check_u32("both store0 X first", s_stores[0].address, VEC_X);
    check_u32("both store0 X value", s_stores[0].value, expect_x);
    check_u32("both store1 Z second", s_stores[1].address, VEC_Z);
    check_u32("both store1 Z value", s_stores[1].value, expect_z);
}

static void test_signed_compare_and_wrap_order(void)
{
    /* Signed slt vs a negative period high-wraps a small positive;
     * unsigned compare would skip and leave the value unchanged. */
    run_case("signed period X", 100u, 0x12121212u, 0u, 0x101u, 2u, 1u);
    check_u32("signed period used sub", peek_u32(VEC_X),
              100u - retail_period(0x101u));

    /* INT32_MIN vs period INT32_MIN: high wrap first yields 0, then
     * bgez skips.  Low-then-high would restore INT32_MIN. */
    run_case("int-min both-wrap-order X", 0x80000000u, 0x12121212u, 0u,
             0x100u, 2u, 1u);
    check_u32("int-min high-then-low is zero", peek_u32(VEC_X), 0u);

    run_case("int-max Z", 0u, 0x34343434u, 0x7FFFFFFFu, 3u, 1u, 1u);
    check_u32("int-max used sub", peek_u32(VEC_Z),
              0x7FFFFFFFu - retail_period(1u));
}

static void test_finite_width_shift_and_add(void)
{
    /* Bits 9+ of the extent drop out of a 32-bit SLL 23. */
    run_case("shift discard X", retail_period(1u), 0xABABABABu, 0u,
             0x00000201u, 1u, 1u);
    check_u32("shift discard period", peek_u32(VEC_X),
              retail_period(1u) - 0x00800000u);

    /* period = 0x80000000: SUBU/ADDU wrap in 32 bits. */
    run_case("add wrap X", 0x80000000u, 0xCDCDCDCDu, 1u, 0x00000100u, 2u,
             1u);
    check_u32("add wrap period", peek_u32(VEC_X), 0x00000000u);

    run_case("sub wrap Z", 2u, 0xEFEFEFEFu, 0x7FFFFFFFu, 3u, 4u, 1u);
    check_u32("sub wrap period", peek_u32(VEC_Z),
              0x7FFFFFFFu - retail_period(4u));
}

static void test_component_offset_discrimination(void)
{
    run_case("y bait", bits_from_s32(-1), bits_from_s32(-20000),
             retail_period(9u), 4u, 9u, 2u);
    check_u32("y bait still bait", peek_u32(VEC_Y), bits_from_s32(-20000));
}

static void test_abi_and_effective_return(void)
{
    u32 before;

    reset_fixture(0u, 0xCAFEBABEu, 0u, 4u, 7u);
    before = wm_80093354_get_exec_count();
    wm_80093354(VEC_ADDR);
    check_u32("exec increments", wm_80093354_get_exec_count(), before + 1u);
    check_u32("void ABI leaves vector interior", peek_u32(VEC_X), 0u);
    check_u32("void ABI leaves Y", peek_u32(VEC_Y), 0xCAFEBABEu);
}

int main(void)
{
    wm_80093354_reset_exec_count();
    test_interior_and_inclusive_bounds();
    test_single_axis_wraps();
    test_both_axes_store_order();
    test_signed_compare_and_wrap_order();
    test_finite_width_shift_and_add();
    test_component_offset_discrimination();
    test_abi_and_effective_return();
    if (s_failures != 0) {
        fprintf(stderr, "W34B22-I1 FAIL assertions=%d\n", s_failures);
        return 1;
    }
    printf("W34B22-I1 0x80093354 focused oracle PASS\n");
    return 0;
}
