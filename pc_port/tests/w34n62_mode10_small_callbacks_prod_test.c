/* Focused production certificate for three retail mode-10 callback pairs. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "psyq/libgte.h"
#include "world_map_callback_794d8.h"
#include "world_map_scheduler.h"

/* matched C (src/world_map/main.c), linked from the matched object */
s32 func_8007A410(s32 arg0);
s32 func_8007A568(void);

/* matched C (src/world_map/main.c), linked from the matched object */
s32 func_800794D8(s32 arg0);
s32 func_80079538(s32 arg0);
s32 func_8007A570(s32 arg0);

uint8_t g_PsxRam[PSX_RAM_SIZE];
uint8_t g_PsxScratchpad[4096];

#define POOL       0x800A0000u
#define POOL_PTR   0x8009BE24u
#define SLOT_INDEX 2
#define SLOT       (POOL + (u32)SLOT_INDEX * 0x80u)
#define CONTEXT_PTR 0x8009C620u
#define CONTEXT     0x800A8000u
#define RESET_POS   0x8009C5ACu
#define SCRATCH_VEC 0x1F8000A0u

static int failures;
static int rot_calls;
static s16 rot_angle[3];
static u32 rot_matrix_address;
static int wrap_calls;
static u32 wrap_address;
static int height_calls;
static u32 height_x;
static u32 height_z;
static s32 height_result;
static int presence_calls;
static u32 presence_id;
static u32 presence_vector;
static u32 presence_third;
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
    write32(CONTEXT_PTR, CONTEXT);
    rot_calls = 0;
    memset(rot_angle, 0, sizeof(rot_angle));
    rot_matrix_address = 0u;
    wrap_calls = 0;
    wrap_address = 0u;
    height_calls = 0;
    height_x = 0u;
    height_z = 0u;
    height_result = 0x00034000;
    presence_calls = 0;
    presence_id = 0u;
    presence_vector = 0u;
    presence_third = 0u;
    clear_calls = 0;
    clear_id = 0u;
}

MATRIX* RotMatrixYXZ(SVECTOR* rotation, MATRIX* matrix)
{
    u32 index;
    memcpy(rot_angle, rotation, sizeof(rot_angle));
    rot_matrix_address = 0x80000000u |
        (u32)((const uint8_t*)matrix - g_PsxRam);
    for (index = 0u; index < 8u; index++) {
        u32 word = 0xA5000000u + index;
        memcpy((uint8_t*)matrix + index * 4u, &word, sizeof(word));
    }
    rot_calls++;
    return matrix;
}

void wm_80093354(u32 address)
{
    wrap_calls++;
    wrap_address = address;
}

s32 wm_80093A5C(u32 packed_xz, u32 data_ptr)
{
    height_calls++;
    height_x = packed_xz;
    height_z = data_ptr;
    return height_result;
}

void wm_80089160(u32 a0, u32 a1, u32 a2)
{
    presence_calls++;
    presence_id = a0;
    presence_vector = a1;
    presence_third = a2;
}

s32 func_80093A5C(s32 x, s32 z)
{
    return wm_80093A5C((u32)x, (u32)z);
}

void func_80089160(s32 id, void *vector, void *flags)
{
    wm_80089160((u32)id, PsxMemory_GuestAddr(vector),
                flags != NULL ? PsxMemory_GuestAddr(flags) : 0u);
}

void wm_800894C8(u32 record_index)
{
    clear_calls++;
    clear_id = record_index;
}

static void test_orbit_init(void)
{
    u32 index;
    seed();
    check(func_800794D8(SLOT_INDEX) == 1, "orbit_init.return");
    check(read32(SLOT + 0x28u) == 0x02000000u &&
          read32(SLOT + 0x30u) == 0x01500000u &&
          read16(SLOT + 0x20u) == 0u,
          "orbit_init.slot");
    check(rot_calls == 1 && rot_angle[0] == 0 &&
          rot_angle[1] == 0x0780 && rot_angle[2] == 0 &&
          rot_matrix_address == CONTEXT + 0x560u,
          "orbit_init.rotation");
    for (index = 0u; index < 8u; index++)
        check(read32(CONTEXT + 0x560u + index * 4u) ==
              0xA5000000u + index, "orbit_init.matrix_destination");
}

static void test_orbit_follow(void)
{
    seed();
    write16(SLOT + 4u, 7u);
    write32(SLOT + 0x28u, 0xFFF23000u);
    write32(SLOT + 0x30u, 0x00145000u);
    check(func_80079538(SLOT_INDEX) == 1, "orbit_follow.return");
    check(read16(SLOT + 4u) == 0u && read16(CONTEXT + 0x540u) == 1u,
          "orbit_follow.latch");
    /* The wrap is the matched func_80093354 (linked from the matched object),
     * so it runs for real instead of reaching the wm_80093354 mock. */
    check(height_calls == 1 && height_x == 0xFFF23000u &&
          height_z == 0x00145000u,
          "orbit_follow.sources");
    check(read32(SLOT + 0x2Cu) == 0x0004C000u &&
          read32(CONTEXT + 0x548u) == 0xFFFFFF23u &&
          read32(CONTEXT + 0x54Cu) == 0x0000004Cu &&
          read32(CONTEXT + 0x550u) == 0x00000145u,
          "orbit_follow.height");
}

static void test_timed_marker(void)
{
    seed();
    check(func_8007A410(SLOT_INDEX) == 3 &&
          read16(SLOT + 0x22u) == 96u,
          "beacon_init.timer");

    seed();
    write16(SLOT + 4u, 1u);
    write16(SLOT + 0x22u, 2u);
    write32(CONTEXT + 8u, 0xFFFFFFFEu);
    write32(CONTEXT + 0x0Cu, 3u);
    write32(CONTEXT + 0x10u, 4u);
    check(wm_8007A430(SLOT_INDEX) == 1, "beacon_active.return");
    check(read16(SLOT + 4u) == 0u && read16(SLOT + 0x22u) == 1u &&
          read32(SLOT + 0x28u) == 0xFFFFE000u &&
          read32(SLOT + 0x2Cu) == 0x00003000u &&
          read32(SLOT + 0x30u) == 0x00024000u,
          "beacon_active.motion");
    check(read_s16(SCRATCH_VEC + 0u) == -2 &&
          read_s16(SCRATCH_VEC + 2u) == 3 &&
          read_s16(SCRATCH_VEC + 4u) == 36 &&
          presence_calls == 1 && presence_id == 9u &&
          presence_vector == SCRATCH_VEC && presence_third == 0u,
          "beacon_active.marker");

    seed();
    write16(SLOT + 0x22u, 1u);
    write32(RESET_POS + 0u, 0x11111111u);
    write32(RESET_POS + 4u, 0x22222222u);
    write32(RESET_POS + 8u, 0x33333333u);
    write32(RESET_POS + 0x0Cu, 0x44444444u);
    check(wm_8007A430(SLOT_INDEX) == 3, "beacon_reset.return");
    check(read16(SLOT + 0x22u) == 96u &&
          read32(SLOT + 0x28u) == 0x11111111u &&
          read32(SLOT + 0x2Cu) == 0x22222222u &&
          read32(SLOT + 0x30u) == 0x33333333u &&
          read32(SLOT + 0x34u) == 0x44444444u,
          "beacon_reset.heading");
    check(clear_calls == 1 && clear_id == 9u, "beacon_reset.clear");
}

static void test_marker_initializer(void)
{
    uint8_t ram_before[32];
    seed();
    memcpy(ram_before, PSX_ADDR(SLOT + 0x28u), sizeof(ram_before));
    check(func_8007A568() == 3, "marker_leaf.return");
    check(memcmp(ram_before, PSX_ADDR(SLOT + 0x28u),
                 sizeof(ram_before)) == 0 && presence_calls == 0,
          "marker_leaf.read_only");

    seed();
    write16(SLOT + 4u, 9u);
    write32(CONTEXT + 8u, 0x12345678u);
    write32(CONTEXT + 0x0Cu, 0xABCDEF01u);
    write32(CONTEXT + 0x10u, 0x76543210u);
    check(func_8007A570(SLOT_INDEX) == 3, "marker_init.return");
    check(read16(SLOT + 4u) == 0u &&
          read16(SCRATCH_VEC + 0u) == 0x5678u &&
          read16(SCRATCH_VEC + 2u) == 0xEF01u &&
          read16(SCRATCH_VEC + 4u) == 0x3210u,
          "marker_init.vector");
    /* The port maps the scratchpad through PSX_ADDR, so the matched body's
     * WM_SPAD(0x1F8000A0) comes back as the RAM alias of the same bytes. */
    check(presence_calls == 1 && presence_id == 10u &&
          PSX_ADDR(presence_vector) == PSX_ADDR(SCRATCH_VEC) && presence_third == 0u,
          "marker_init.id");
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
    dispatch_one(0x800794D8u, 0x80079538u, 0);
    dispatch_one(0x800794D8u, 0x80079538u, 1);
    dispatch_one(0x8007A410u, 0x8007A430u, 0);
    write16(SLOT + 0x22u, 1u);
    dispatch_one(0x8007A410u, 0x8007A430u, 1);
    dispatch_one(0x8007A568u, 0x8007A570u, 0);
    dispatch_one(0x8007A568u, 0x8007A570u, 1);
}

int main(void)
{
    test_orbit_init();
    test_orbit_follow();
    test_timed_marker();
    test_marker_initializer();
    test_scheduler_resolution();
    if (failures != 0) {
        fprintf(stderr, "W34N62 MODE10 SMALL CALLBACK CERTIFICATE: %d failure(s)\n",
                failures);
        return 1;
    }
    puts("W34N62 MODE10 SMALL CALLBACK CERTIFICATE PASS");
    return 0;
}
