/* Focused production-linked certificate for retail mode-18 object control. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_callback_84068.h"
#include "world_map_scheduler.h"

uint8_t g_PsxRam[PSX_RAM_SIZE];
uint8_t g_PsxScratchpad[4096];

#define POOL       UINT32_C(0x800A0000)
#define POOL_PTR   UINT32_C(0x8009BE24)
#define SLOT_INDEX 3
#define SLOT       (POOL + (u32)SLOT_INDEX * UINT32_C(0x80))
#define OBJECT_PTR UINT32_C(0x8009C620)
#define OBJECT     UINT32_C(0x800B0000)
#define SCRATCH    UINT32_C(0x1F800000)

static int failures;
static unsigned clear_count;
static u32 clear_ids[8];
static unsigned presence_count;
static u32 presence_ids[8];
static u16 presence_x[8];
static u16 presence_y[8];
static u16 presence_z[8];
static unsigned approach_count;
static s32 approach_target;
static s32 approach_step;

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

static void seed(void)
{
    memset(g_PsxRam, 0, sizeof(g_PsxRam));
    memset(g_PsxScratchpad, 0, sizeof(g_PsxScratchpad));
    write32(POOL_PTR, POOL);
    write32(OBJECT_PTR, OBJECT);
    write32(SLOT + 0x28u, UINT32_C(0x01234000));
    write32(SLOT + 0x2Cu, 0u);
    write32(SLOT + 0x30u, UINT32_C(0x00067000));
    write16(OBJECT + 0x2A0u, 1u);
    write16(OBJECT + 0x24Cu, 1u);
    clear_count = 0u;
    memset(clear_ids, 0, sizeof(clear_ids));
    presence_count = 0u;
    memset(presence_ids, 0, sizeof(presence_ids));
    memset(presence_x, 0, sizeof(presence_x));
    memset(presence_y, 0, sizeof(presence_y));
    memset(presence_z, 0, sizeof(presence_z));
    approach_count = 0u;
    approach_target = 0;
    approach_step = 0;
}

s32 wm_800771D8(s32 current, s32 target, s32 step)
{
    approach_count++;
    approach_target = target;
    approach_step = step;
    if (current == target)
        return current;
    return current + step;
}

void wm_800894C8(u32 record_index)
{
    if (clear_count < 8u)
        clear_ids[clear_count] = record_index;
    clear_count++;
}

void wm_80089160(u32 record_index, u32 vector, u32 flags)
{
    check(vector == SCRATCH + 0xA0u && flags == 0u,
          "presence.arguments");
    if (presence_count < 8u) {
        presence_ids[presence_count] = record_index;
        presence_x[presence_count] = read16(vector + 0u);
        presence_y[presence_count] = read16(vector + 2u);
        presence_z[presence_count] = read16(vector + 4u);
    }
    presence_count++;
}

static int ids_equal(const u32 *actual, const u32 *expected, unsigned count)
{
    unsigned index;

    for (index = 0u; index < count; index++)
        if (actual[index] != expected[index])
            return 0;
    return 1;
}

static void test_latches(void)
{
    static const u32 latch2[] = {3u, 5u, 6u, 7u, 10u};
    static const u32 latch3[] = {7u, 8u, 9u};
    static const u32 latch4[] = {5u, 6u, 7u, 10u, 11u, 12u};

    seed();
    write16(SLOT + 0x04u, 1u);
    check(wm_80084068(SLOT_INDEX) == 1, "latch.return");
    check(read16(SLOT + 0x04u) == 0u && read16(SLOT + 0x20u) == 1u,
          "latch1.state");
    check(read16(OBJECT + 0x2A0u) == 0u &&
          read16(OBJECT + 0x24Cu) == 0u,
          "latch1.object_disable");

    seed();
    write16(SLOT + 0x04u, 2u);
    (void)wm_80084068(SLOT_INDEX);
    check(clear_count == 5u && ids_equal(clear_ids, latch2, 5u),
          "latch2.clear_list");

    seed();
    write16(SLOT + 0x04u, 3u);
    (void)wm_80084068(SLOT_INDEX);
    check(clear_count == 3u && ids_equal(clear_ids, latch3, 3u),
          "latch3.clear_list");

    seed();
    write16(SLOT + 0x04u, 4u);
    (void)wm_80084068(SLOT_INDEX);
    check(clear_count == 6u && ids_equal(clear_ids, latch4, 6u),
          "latch4.clear_list");
}

static void test_state_lists(void)
{
    static const u32 state1[] = {3u, 5u, 6u, 7u, 10u};
    static const u32 state2[] = {7u, 8u, 9u};
    static const u32 state3[] = {5u, 6u, 7u, 10u, 11u, 12u};

    seed();
    write16(SLOT + 0x20u, 1u);
    (void)wm_80084068(SLOT_INDEX);
    check(approach_count == 1u &&
          approach_target == INT32_C(-0x00180000) &&
          approach_step == INT32_C(-0x00000400) &&
          read32(SLOT + 0x2Cu) == UINT32_C(0xFFFFFC00),
          "state1.interpolation");
    check(presence_count == 5u && ids_equal(presence_ids, state1, 5u),
          "state1.presence_list");
    check(presence_x[0] == UINT16_C(0x1234) &&
          presence_y[0] == UINT16_C(0xFFFF) &&
          presence_z[0] == UINT16_C(0x0067),
          "state1.vector");

    seed();
    write16(SLOT + 0x20u, 2u);
    (void)wm_80084068(SLOT_INDEX);
    check(presence_count == 3u && ids_equal(presence_ids, state2, 3u),
          "state2.presence_list");

    seed();
    write16(SLOT + 0x20u, 3u);
    (void)wm_80084068(SLOT_INDEX);
    check(presence_count == 6u && ids_equal(presence_ids, state3, 6u),
          "state3.presence_list");

    seed();
    write16(SLOT + 0x20u, 4u);
    (void)wm_80084068(SLOT_INDEX);
    check(presence_count == 2u && presence_ids[0] == 13u &&
          presence_ids[1] == 14u &&
          presence_y[0] == UINT16_C(0xFFFF) && presence_y[1] == 0u,
          "state4.second_y_zero");
}

static void test_object_publication(void)
{
    seed();
    write32(SLOT + 0x28u, UINT32_C(0x12345000));
    write32(SLOT + 0x2Cu, UINT32_C(0xFFFED000));
    write32(SLOT + 0x30u, UINT32_C(0x001AB000));
    (void)wm_80084068(SLOT_INDEX);
    check(read32(OBJECT + 0x2A8u) == UINT32_C(0x00012345) &&
          read32(OBJECT + 0x254u) == UINT32_C(0x00012345) &&
          read32(OBJECT + 0x2ACu) == UINT32_C(0xFFFFFFED) &&
          read32(OBJECT + 0x258u) == UINT32_C(0xFFFFFFED) &&
          read32(OBJECT + 0x2B0u) == UINT32_C(0x000001AB) &&
          read32(OBJECT + 0x25Cu) == UINT32_C(0x000001AB),
          "object.position_publish");
}

static void test_scheduler_resolution(void)
{
    seed();
    write32(SLOT + 0x1Cu, UINT32_C(0x80084068));
    write16(SLOT + 0x00u, 1u);
    wm_sched_reset();
    wm_sched_callback_registry_clear();
    wm_80097800();
    check(wm_sched_get_callbacks_executed() == 1 &&
          wm_sched_get_missing_hits() == 0 &&
          wm_sched_get_invalid_hits() == 0,
          "scheduler.update_resolution");
}

int main(void)
{
    test_latches();
    test_state_lists();
    test_object_publication();
    test_scheduler_resolution();
    if (failures != 0) {
        fprintf(stderr,
                "W34N117 MODE18 OBJECT CERTIFICATE: %d failure(s)\n",
                failures);
        return 1;
    }
    puts("W34N117 MODE18 OBJECT CERTIFICATE PASS");
    return 0;
}
