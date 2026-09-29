/* Focused production certificate for retail 0x8007BA08/0x8007BA10. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_callback_7ba08.h"
#include "world_map_scheduler.h"

/* matched C (src/world_map/main.c), linked from the matched object */
s32 func_8007BA08(void);

uint8_t g_PsxRam[PSX_RAM_SIZE];
uint8_t g_PsxScratchpad[4096];

#define POOL        UINT32_C(0x800A0000)
#define POOL_PTR    UINT32_C(0x8009BE24)
#define SLOT_INDEX  4
#define SLOT        (POOL + (u32)SLOT_INDEX * UINT32_C(0x80))
#define RESET_POS   UINT32_C(0x8009C5AC)
#define SCRATCH_VEC UINT32_C(0x1F8000A0)
#define MARKER_DATA UINT32_C(0x8009A488)

static int failures;
static int presence_calls;
static u32 presence_id;
static u32 presence_vector;
static u32 presence_data;
static int clear_calls;
static u32 clear_id;

static void check(int condition, const char* name)
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

static s16 read_s16(u32 address)
{
    s16 value;
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
    memset(g_PsxScratchpad, 0xA5, sizeof(g_PsxScratchpad));
    write32(POOL_PTR, POOL);
    presence_calls = 0;
    presence_id = 0u;
    presence_vector = 0u;
    presence_data = 0u;
    clear_calls = 0;
    clear_id = 0u;
}

void wm_80089160(u32 a0, u32 a1, u32 a2)
{
    presence_calls++;
    presence_id = a0;
    presence_vector = a1;
    presence_data = a2;
}

void wm_800894C8(u32 record_index)
{
    clear_calls++;
    clear_id = record_index;
}

static void test_leaf(void)
{
    uint8_t before[0x80];
    seed();
    memset(PSX_ADDR(SLOT), 0x5A, sizeof(before));
    memcpy(before, PSX_ADDR(SLOT), sizeof(before));
    check(func_8007BA08() == 3, "leaf.return");
    check(memcmp(before, PSX_ADDR(SLOT), sizeof(before)) == 0,
          "leaf.read_only");
}

static void test_latch_and_active_motion(void)
{
    seed();
    write16(SLOT + 4u, 1u);
    write32(RESET_POS + 0u, UINT32_C(0x00100000));
    write32(RESET_POS + 4u, UINT32_C(0xFFF00000));
    write32(RESET_POS + 8u, UINT32_C(0x00200000));
    write32(RESET_POS + 0x0Cu, UINT32_C(0x12345678));
    write32(SLOT + 0x34u, UINT32_C(0xDEADBEEF));

    check(wm_8007BA10(SLOT_INDEX) == 1, "latch.return");
    check(read16(SLOT + 4u) == 0u && read16(SLOT + 0x22u) == 59u,
          "latch.timer_seed");
    check(read32(SLOT + 0x28u) == UINT32_C(0x000F6A30) &&
          read32(SLOT + 0x2Cu) == UINT32_C(0xFFF00000) &&
          read32(SLOT + 0x30u) == UINT32_C(0x0022F130),
          "latch.position_and_step");
    check(read32(SLOT + 0x34u) == UINT32_C(0xDEADBEEF),
          "latch.heading_untouched");
    check(read_s16(SCRATCH_VEC + 0u) == 246 &&
          read_s16(SCRATCH_VEC + 2u) == -256 &&
          read_s16(SCRATCH_VEC + 4u) == 559,
          "latch.scratch_vector");
    check(presence_calls == 1 && presence_id == 9u &&
          presence_vector == SCRATCH_VEC && presence_data == MARKER_DATA,
          "latch.marker_arguments");
}

static void test_active_motion(void)
{
    seed();
    write16(SLOT + 0x22u, 2u);
    write32(SLOT + 0x28u, UINT32_C(0x00020000));
    write32(SLOT + 0x2Cu, UINT32_C(0xFFFE0000));
    write32(SLOT + 0x30u, UINT32_C(0x00030000));

    check(wm_8007BA10(SLOT_INDEX) == 1, "active.return");
    check(read16(SLOT + 0x22u) == 1u &&
          read32(SLOT + 0x28u) == UINT32_C(0x00016A30) &&
          read32(SLOT + 0x30u) == UINT32_C(0x0005F130),
          "active.velocity");
    check(read_s16(SCRATCH_VEC + 0u) == 22 &&
          read_s16(SCRATCH_VEC + 2u) == -32 &&
          read_s16(SCRATCH_VEC + 4u) == 95,
          "active.scratch_vector");
    check(presence_calls == 1 && presence_id == 9u &&
          presence_vector == SCRATCH_VEC && presence_data == MARKER_DATA,
          "active.marker_arguments");
}

static void test_expiry_reset(void)
{
    seed();
    write16(SLOT + 0x22u, 1u);
    write32(RESET_POS + 0u, UINT32_C(0x11111111));
    write32(RESET_POS + 4u, UINT32_C(0x22222222));
    write32(RESET_POS + 8u, UINT32_C(0x33333333));
    write32(RESET_POS + 0x0Cu, UINT32_C(0x44444444));

    check(wm_8007BA10(SLOT_INDEX) == 3, "expiry.return");
    check(read16(SLOT + 0x22u) == 60u &&
          read32(SLOT + 0x28u) == UINT32_C(0x11111111) &&
          read32(SLOT + 0x2Cu) == UINT32_C(0x22222222) &&
          read32(SLOT + 0x30u) == UINT32_C(0x33333333) &&
          read32(SLOT + 0x34u) == UINT32_C(0x44444444),
          "expiry.reset_record");
    check(clear_calls == 1 && clear_id == 9u && presence_calls == 0,
          "expiry.clear_marker");
}

static void dispatch_one(u32 cb0, u32 cb1, s16 state)
{
    memset(PSX_ADDR(POOL), 0, 64u * 0x80u);
    write32(SLOT + 0x18u, cb0);
    write32(SLOT + 0x1Cu, cb1);
    write16(SLOT + 0u, (u16)state);
    wm_sched_reset();
    wm_sched_callback_registry_clear();
    wm_80097800();
    check(wm_sched_get_callbacks_executed() == 1 &&
          wm_sched_get_missing_hits() == 0 &&
          wm_sched_get_invalid_hits() == 0,
          "scheduler.resolution");
}

static void test_scheduler_resolution(void)
{
    seed();
    dispatch_one(UINT32_C(0x8007BA08), UINT32_C(0x8007BA10), 0);
    dispatch_one(UINT32_C(0x8007BA08), UINT32_C(0x8007BA10), 1);
}

int main(void)
{
    test_leaf();
    test_latch_and_active_motion();
    test_active_motion();
    test_expiry_reset();
    test_scheduler_resolution();
    if (failures != 0) {
        fprintf(stderr,
                "W34N70 MODE14 TIMED MARKER CERTIFICATE: %d failure(s)\n",
                failures);
        return 1;
    }
    puts("W34N70 MODE14 TIMED MARKER CERTIFICATE PASS");
    return 0;
}
