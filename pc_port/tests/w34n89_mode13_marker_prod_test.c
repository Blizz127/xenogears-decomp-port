/* Focused production certificate for retail 0x80080900/0x80080944. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_scheduler.h"

/* matched C (src/world_map/main.c), linked from the matched object */
s32 func_80080900(s32 arg0);
s32 func_80080944(s32 arg0);

uint8_t g_PsxRam[PSX_RAM_SIZE];
uint8_t g_PsxScratchpad[4096];

#define POOL       UINT32_C(0x800A0000)
#define POOL_PTR   UINT32_C(0x8009BE24)
#define SLOT_INDEX 3
#define SLOT       (POOL + (u32)SLOT_INDEX * UINT32_C(0x80))
#define RESET_X    UINT32_C(0x8009C5AC)
#define RESET_Y    UINT32_C(0x8009C5B0)
#define RESET_Z    UINT32_C(0x8009C5B4)
#define VECTOR     UINT32_C(0x1F8000A0)

static int failures;
static unsigned marker_count;
static u32 marker_ids[3];
static u32 marker_vectors[3];
static u32 marker_flags[3];

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
    memset(g_PsxScratchpad, 0x5A, sizeof(g_PsxScratchpad));
    write32(POOL_PTR, POOL);
    write32(RESET_X, UINT32_C(0x00123000));
    write32(RESET_Y, UINT32_C(0xFFFED000));
    write32(RESET_Z, UINT32_C(0x00456000));
    marker_count = 0u;
    memset(marker_ids, 0, sizeof(marker_ids));
    memset(marker_vectors, 0, sizeof(marker_vectors));
    memset(marker_flags, 0, sizeof(marker_flags));
}

void wm_80089160(u32 id, u32 vector, u32 flags)
{
    check(marker_count < 3u, "trace.marker_capacity");
    if (marker_count < 3u) {
        marker_ids[marker_count] = id;
        marker_vectors[marker_count] = vector;
        marker_flags[marker_count] = flags;
    }
    marker_count++;
}

void func_80089160(s32 id, void *vector, void *flags)
{
    wm_80089160((u32)id, PsxMemory_GuestAddr(vector),
                flags != NULL ? PsxMemory_GuestAddr(flags) : 0u);
}

static void test_initializer(void)
{
    seed();
    write32(SLOT + 0x24u, UINT32_C(0xA5A5A5A5));
    write32(SLOT + 0x34u, UINT32_C(0x5A5A5A5A));

    check(func_80080900(SLOT_INDEX) == 1, "init.return");
    check(read32(SLOT + 0x28u) == UINT32_C(0x00123000) &&
          read32(SLOT + 0x2Cu) == UINT32_C(0xFFFED000) &&
          read32(SLOT + 0x30u) == UINT32_C(0x00456000),
          "init.position_copy");
    check(read32(SLOT + 0x24u) == UINT32_C(0xA5A5A5A5) &&
          read32(SLOT + 0x34u) == UINT32_C(0x5A5A5A5A),
          "init.write_bounds");
}

static void test_latched_markers(void)
{
    seed();
    write16(SLOT + 0x04u, 1u);
    write32(SLOT + 0x28u, UINT32_C(0x00123000));
    write32(SLOT + 0x2Cu, UINT32_C(0xFFFED000));
    write32(SLOT + 0x30u, UINT32_C(0x00456000));

    check(func_80080944(SLOT_INDEX) == 1, "update.return");
    check(read16(SLOT + 0x04u) == 0u, "update.latch_clear");
    check(read16(VECTOR + 0u) == UINT16_C(0x0123) &&
          read16(VECTOR + 2u) == UINT16_C(0xFFED) &&
          read16(VECTOR + 4u) == UINT16_C(0x0456),
          "update.vector_shift");
    check(marker_count == 3u && marker_ids[0] == 40u &&
          marker_ids[1] == 41u && marker_ids[2] == 42u,
          "update.marker_ids");
    /* WM_SPAD(0x1F8000A0) reaches the mock as the RAM alias of the same bytes */
    check(PSX_ADDR(marker_vectors[0]) == PSX_ADDR(VECTOR) &&
          PSX_ADDR(marker_vectors[1]) == PSX_ADDR(VECTOR) &&
          PSX_ADDR(marker_vectors[2]) == PSX_ADDR(VECTOR) && marker_flags[0] == 0u &&
          marker_flags[1] == 0u && marker_flags[2] == 0u,
          "update.marker_args");
}

static void test_nonmatching_latch(void)
{
    seed();
    write16(SLOT + 0x04u, 2u);
    write16(VECTOR + 0u, UINT16_C(0x1111));
    check(func_80080944(SLOT_INDEX) == 1, "noop.return");
    check(read16(SLOT + 0x04u) == 2u && marker_count == 0u &&
          read16(VECTOR + 0u) == UINT16_C(0x1111),
          "noop.guard");
}

static void test_scheduler_resolution(void)
{
    seed();
    write32(SLOT + 0x18u, UINT32_C(0x80080900));
    write32(SLOT + 0x1Cu, UINT32_C(0x80080944));
    write16(SLOT + 0u, 0u);
    wm_sched_reset();
    wm_sched_callback_registry_clear();
    wm_80097800();
    check(wm_sched_get_callbacks_executed() == 1 &&
          wm_sched_get_missing_hits() == 0 &&
          wm_sched_get_invalid_hits() == 0 &&
          read16(SLOT + 0u) == 1u,
          "scheduler.init_resolution");

    write16(SLOT + 0x04u, 1u);
    marker_count = 0u;
    wm_sched_reset();
    wm_80097800();
    check(wm_sched_get_callbacks_executed() == 1 &&
          wm_sched_get_missing_hits() == 0 &&
          wm_sched_get_invalid_hits() == 0 && marker_count == 3u,
          "scheduler.update_resolution");
}

int main(void)
{
    test_initializer();
    test_latched_markers();
    test_nonmatching_latch();
    test_scheduler_resolution();
    if (failures != 0) {
        fprintf(stderr,
                "W34N89 MODE13 MARKER CERTIFICATE: %d failure(s)\n",
                failures);
        return 1;
    }
    puts("W34N89 MODE13 MARKER CERTIFICATE PASS");
    return 0;
}
