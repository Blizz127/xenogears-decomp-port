/* Focused production certificate for retail 0x80081FB4..0x80082324. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_callback_81fb4.h"
#include "world_map_scheduler.h"

/* matched C (src/world_map/main.c), linked from the matched object */
s32 func_80081FB4(s32 arg0);

uint8_t g_PsxRam[PSX_RAM_SIZE];
uint8_t g_PsxScratchpad[4096];

#define POOL        UINT32_C(0x800B0000)
#define POOL_PTR    UINT32_C(0x8009BE24)
#define SLOT_INDEX  6
#define SLOT        (POOL + (u32)SLOT_INDEX * UINT32_C(0x80))
#define TABLE       UINT32_C(0x800A8000)
#define TABLE_PTR   UINT32_C(0x8009D148)
#define COUNT       UINT32_C(192)

static int failures;
static int rand_values[512];
static u32 rand_value_count;
static u32 rand_call_count;

static void check(int condition, const char *name)
{
    if (!condition) {
        fprintf(stderr, "ASSERTION %s FAILED\n", name);
        failures++;
    }
}

static void write16(u32 address, u16 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static void write32(u32 address, u32 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static u16 read16(u32 address)
{
    u16 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static u32 read32(u32 address)
{
    u32 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static void reset_random(void)
{
    memset(rand_values, 0, sizeof(rand_values));
    rand_value_count = 0u;
    rand_call_count = 0u;
}

static void push_random(int value)
{
    check(rand_value_count < 512u, "trace.random_capacity");
    if (rand_value_count < 512u)
        rand_values[rand_value_count] = value;
    rand_value_count++;
}

int rand(void)
{
    u32 call = rand_call_count;

    rand_call_count++;
    if (call < rand_value_count && call < 512u)
        return rand_values[call];
    return 1;
}

static void seed(u16 table_value)
{
    u32 index;

    memset(g_PsxRam, 0, sizeof(g_PsxRam));
    memset(g_PsxScratchpad, 0, sizeof(g_PsxScratchpad));
    write32(POOL_PTR, POOL);
    write32(TABLE_PTR, TABLE);
    for (index = 0u; index < COUNT + 64u; index++)
        write16(TABLE + index * 2u, table_value);
    reset_random();
}

static int table_is(u16 value)
{
    u32 index;

    for (index = 0u; index < COUNT; index++) {
        if (read16(TABLE + index * 2u) != value)
            return 0;
    }
    return 1;
}

static void test_initializer(void)
{
    seed(UINT16_C(0x7777));
    write16(SLOT + 0x20u, UINT16_C(0x1234));
    write32(SLOT + 0x50u, UINT32_C(0x55667788));
    check(func_80081FB4(SLOT_INDEX) == 1, "init.return");
    check(read16(SLOT + 0x20u) == 0u, "init.state");
    check(read32(SLOT + 0x50u) == 1u, "init.width");
}

static void seed_state0_random(void)
{
    static const int values[] = {
        2, 1, 0, 4, 1,
        3, 0, 0, 9,
        4, 0, 0, 19
    };
    u32 index;

    for (index = 0u; index < (u32)(sizeof(values) / sizeof(values[0]));
         index++)
        push_random(values[index]);
}

static void test_state0_bands(void)
{
    u32 index;
    int layout_ok = 1;

    seed(UINT16_C(0x7777));
    write16(SLOT + 0x20u, 0u);
    write32(SLOT + 0x50u, 7u);
    seed_state0_random();
    check(wm_80081FD8(SLOT_INDEX) == 1, "state0.return");
    for (index = 0u; index < COUNT; index++) {
        u16 expected = 7u;

        if (index == 2u)
            expected = 5u;
        else if (index == 67u)
            expected = 10u;
        else if (index == 132u)
            expected = 20u;
        if (read16(TABLE + index * 2u) != expected)
            layout_ok = 0;
    }
    check(rand_call_count == 13u && layout_ok != 0,
          "state0.fill_and_bands");
}

static void test_command_mapping(void)
{
    seed(UINT16_C(0x3456));
    write16(SLOT + 0x04u, 5u);
    write16(SLOT + 0x20u, 0u);
    write32(SLOT + 0x50u, 9u);
    check(wm_80081FD8(SLOT_INDEX) == 1, "command.return");
    check(read16(SLOT + 0x04u) == 0u &&
          read16(SLOT + 0x20u) == 5u &&
          table_is(UINT16_C(0x3456)),
          "command.consume_and_map");
}

static void test_grow(void)
{
    seed(UINT16_C(0x7777));
    write16(SLOT + 0x20u, 1u);
    write32(SLOT + 0x50u, 64u);
    check(wm_80081FD8(SLOT_INDEX) == 1 &&
          read32(SLOT + 0x50u) == 64u && table_is(64u),
          "state1.grow_clamp_fill");
}

static void test_shrink(void)
{
    seed(UINT16_C(0x7777));
    write16(SLOT + 0x20u, 2u);
    write32(SLOT + 0x50u, 2u);
    check(wm_80081FD8(SLOT_INDEX) == 1 &&
          read32(SLOT + 0x50u) == 2u && table_is(2u),
          "state2.shrink_clamp_fill");
}

static void test_sparse_randomization(void)
{
    u32 index;
    int layout_ok = 1;

    seed(9u);
    write16(SLOT + 0x20u, 3u);
    for (index = 0u; index < COUNT; index++) {
        if (index == 17u) {
            push_random(0);
            push_random(22);
        } else {
            push_random(1);
        }
    }
    check(wm_80081FD8(SLOT_INDEX) == 1, "state3.return");
    for (index = 0u; index < COUNT; index++) {
        u16 expected = index == 17u ? 23u : 9u;

        if (read16(TABLE + index * 2u) != expected)
            layout_ok = 0;
    }
    check(rand_call_count == COUNT + 1u && layout_ok != 0,
          "state3.sparse_randomization");
}

static void test_constant_and_unknown_states(void)
{
    seed(9u);
    write16(SLOT + 0x20u, 4u);
    check(wm_80081FD8(SLOT_INDEX) == 1 && table_is(2u),
          "state4.constant_fill");

    seed(UINT16_C(0x2468));
    write16(SLOT + 0x20u, 5u);
    check(wm_80081FD8(SLOT_INDEX) == 1 && table_is(UINT16_C(0x2468)) &&
          rand_call_count == 0u,
          "state5.noop");
}

static void test_scheduler_resolution(void)
{
    seed(7u);
    write32(SLOT + 0x18u, UINT32_C(0x80081FB4));
    write32(SLOT + 0x1Cu, UINT32_C(0x80081FD8));
    write16(SLOT + 0x00u, 0u);
    wm_sched_reset();
    wm_sched_callback_registry_clear();
    wm_80097800();
    check(wm_sched_get_callbacks_executed() == 1 &&
          wm_sched_get_missing_hits() == 0 &&
          wm_sched_get_invalid_hits() == 0 &&
          read16(SLOT + 0x00u) == 1u &&
          read16(SLOT + 0x20u) == 0u && read32(SLOT + 0x50u) == 1u,
          "scheduler.init_resolution");

    seed_state0_random();
    wm_sched_reset();
    wm_80097800();
    check(wm_sched_get_callbacks_executed() == 1 &&
          wm_sched_get_missing_hits() == 0 &&
          wm_sched_get_invalid_hits() == 0 && rand_call_count == 13u,
          "scheduler.update_resolution");
}

int main(void)
{
    test_initializer();
    test_state0_bands();
    test_command_mapping();
    test_grow();
    test_shrink();
    test_sparse_randomization();
    test_constant_and_unknown_states();
    test_scheduler_resolution();
    if (failures != 0) {
        fprintf(stderr,
                "W34N108 MODE16 WIDTH CONTROL CERTIFICATE: %d failure(s)\n",
                failures);
        return 1;
    }
    puts("W34N108 MODE16 WIDTH CONTROL CERTIFICATE PASS");
    return 0;
}
