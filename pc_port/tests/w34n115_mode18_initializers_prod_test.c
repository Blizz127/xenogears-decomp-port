/* Focused production-linked certificate for retail mode-18 initializers. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_callback_838e8.h"
#include "world_map_scheduler.h"

uint8_t g_PsxRam[PSX_RAM_SIZE];
uint8_t g_PsxScratchpad[4096];

#define POOL       UINT32_C(0x800A0000)
#define POOL_PTR   UINT32_C(0x8009BE24)
#define SLOT_INDEX 2
#define SLOT       (POOL + (u32)SLOT_INDEX * UINT32_C(0x80))
#define RESET      UINT32_C(0x8009C5AC)
#define POSITION   UINT32_C(0x8009BE28)
#define TARGET     UINT32_C(0x8009D55C)
#define ANGLES     UINT32_C(0x8009BD38)
#define GATE       UINT32_C(0x8009D144)
#define HEIGHT     UINT32_C(0x8009D3F0)
#define VIEW       UINT32_C(0x8009BE0C)
#define OBJECT_PTR UINT32_C(0x8009C620)
#define OBJECT     UINT32_C(0x800B0000)

static int failures;

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
    write32(RESET + 0u, UINT32_C(0x01100000));
    write32(RESET + 4u, UINT32_C(0xFFE00000));
    write32(RESET + 8u, UINT32_C(0x03300000));
}

static void test_static_record_initializer(void)
{
    seed();
    write32(SLOT + 0x4Cu, UINT32_C(0x13579BDF));
    write32(SLOT + 0x54u, UINT32_C(0x2468ACE0));
    check(wm_800838E8(SLOT_INDEX) == 1, "init_static.return");
    check(read32(SLOT + 0x50u) == UINT32_C(0x8009AC60),
          "init_static.record_pointer");
    check(read32(SLOT + 0x4Cu) == UINT32_C(0x13579BDF) &&
          read32(SLOT + 0x54u) == UINT32_C(0x2468ACE0),
          "init_static.write_bounds");
}

static void test_camera_initializer(void)
{
    seed();
    write32(SLOT - 4u, UINT32_C(0x13579BDF));
    write32(SLOT + 0x80u, UINT32_C(0x2468ACE0));
    check(wm_8008390C(SLOT_INDEX) == 1, "camera_init.return");
    check(read32(SLOT + 0x7Cu) == UINT32_C(0x1000) &&
          read16(SLOT + 0x20u) == 0u && read16(SLOT + 0x04u) == 0u &&
          read32(SLOT + 0x5Cu) == UINT32_C(0x00960000) &&
          read32(VIEW) == 120u,
          "camera_init.constants");
    check(read32(POSITION + 0u) == UINT32_C(0x01100000) &&
          read32(POSITION + 4u) == UINT32_C(0xFFE00000) &&
          read32(POSITION + 8u) == UINT32_C(0x03300000) &&
          read32(TARGET + 0u) == UINT32_C(0x01100000) &&
          read32(TARGET + 4u) == UINT32_C(0xFFE00000) &&
          read32(TARGET + 8u) == UINT32_C(0x03300000) &&
          read32(SLOT + 0x28u) == UINT32_C(0x01100000) &&
          read32(SLOT + 0x2Cu) == UINT32_C(0xFFE00000) &&
          read32(SLOT + 0x30u) == UINT32_C(0x03300000),
          "camera_init.position_fanout");
    check(read16(ANGLES + 0u) == UINT16_C(0xFFE0) &&
          read16(ANGLES + 2u) == UINT16_C(0x0400) &&
          read16(ANGLES + 4u) == 0u,
          "camera_init.angles");
    check(read32(SLOT + 0x50u) == UINT32_C(0xFFFE0000) &&
          read32(SLOT + 0x38u) == UINT32_C(0xFFFE0000) &&
          read32(SLOT + 0x54u) == UINT32_C(0x00400000) &&
          read32(SLOT + 0x3Cu) == UINT32_C(0x00400000) &&
          read32(SLOT + 0x58u) == 0u && read32(SLOT + 0x40u) == 0u &&
          read32(GATE) == 0u,
          "camera_init.rotation_state");
    check(read32(HEIGHT) == UINT32_C(0x00960000),
          "camera_init.height");
    check(read32(SLOT - 4u) == UINT32_C(0x13579BDF) &&
          read32(SLOT + 0x80u) == UINT32_C(0x2468ACE0),
          "camera_init.write_bounds");
}

static void test_object_initializer(void)
{
    seed();
    write32(OBJECT + 0x248u, UINT32_C(0x13579BDF));
    write32(OBJECT + 0x2B4u, UINT32_C(0x2468ACE0));
    check(wm_80083FE4(SLOT_INDEX) == 1, "object_init.return");
    check(read32(SLOT + 0x28u) == UINT32_C(0x01800000) &&
          read32(SLOT + 0x2Cu) == UINT32_C(0x00080000) &&
          read32(SLOT + 0x30u) == UINT32_C(0x01A00000) &&
          read16(SLOT + 0x20u) == 0u,
          "object_init.slot_position");
    check(read16(OBJECT + 0x2A0u) == 1u &&
          read16(OBJECT + 0x24Cu) == 1u,
          "object_init.enable");
    check(read32(OBJECT + 0x2A8u) == UINT32_C(0x1800) &&
          read32(OBJECT + 0x254u) == UINT32_C(0x1800) &&
          read32(OBJECT + 0x2ACu) == UINT32_C(0x080) &&
          read32(OBJECT + 0x258u) == UINT32_C(0x080) &&
          read32(OBJECT + 0x2B0u) == UINT32_C(0x1A00) &&
          read32(OBJECT + 0x25Cu) == UINT32_C(0x1A00),
          "object_init.translation");
    check(read32(OBJECT + 0x248u) == UINT32_C(0x13579BDF) &&
          read32(OBJECT + 0x2B4u) == UINT32_C(0x2468ACE0),
          "object_init.write_bounds");
}

static void test_scheduler_resolution(void)
{
    static const u32 init_callbacks[] = {
        UINT32_C(0x800838E8), UINT32_C(0x8008390C),
        UINT32_C(0x80083FE4)
    };
    static const u32 update_callbacks[] = {
        UINT32_C(0x80076B34), UINT32_C(0x80083A00),
        UINT32_C(0x80084068)
    };
    unsigned index;

    for (index = 0u; index < 3u; index++) {
        seed();
        write32(SLOT + 0x18u, init_callbacks[index]);
        write32(SLOT + 0x1Cu, update_callbacks[index]);
        write16(SLOT + 0x00u, 0u);
        wm_sched_reset();
        wm_sched_callback_registry_clear();
        wm_80097800();
        check(wm_sched_get_callbacks_executed() == 1 &&
              wm_sched_get_missing_hits() == 0 &&
              wm_sched_get_invalid_hits() == 0 &&
              read16(SLOT + 0x00u) == 1u,
              "scheduler.initializer_resolution");
    }
}

int main(void)
{
    test_static_record_initializer();
    test_camera_initializer();
    test_object_initializer();
    test_scheduler_resolution();
    if (failures != 0) {
        fprintf(stderr, "W34N115 MODE18 INITIALIZERS: %d failure(s)\n",
                failures);
        return 1;
    }
    puts("W34N115 MODE18 INITIALIZERS CERTIFICATE PASS");
    return 0;
}
