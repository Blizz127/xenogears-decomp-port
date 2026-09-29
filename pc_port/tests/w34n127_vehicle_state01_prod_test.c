#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_callback_8e76c.h"
#include "world_map_state01_8eb30.h"

#define SLOT        UINT32_C(0x800A1000)
#define POOL        (SLOT - UINT32_C(7 * 0x80))
#define POOL_PTR    UINT32_C(0x8009BE24)
#define CONTEXT     UINT32_C(0x800A2000)
#define CONTEXT_PTR UINT32_C(0x8009C620)
#define NATIVE_HEAD UINT32_C(0x8006EE66)

uint8_t g_PsxRam[PSX_RAM_SIZE];
uint8_t g_PsxScratchpad[4096];

static int s_failures;
static s32 s_terrain;
static int s_terrain_calls;
static s32 s_terrain_x;
static s32 s_terrain_z;
static int s_ring_calls;
static int s_event;
static int s_terrain_event;
static int s_ring_event;
static u16 s_ring_tag;
static u32 s_ring_vector;
static u16 s_ring_half2;
static s16 s_ring_value;
static u32 s_ring_snapshot[4];
static int s_unload_calls;
static u32 s_unload_vector;
static int s_publish_calls;
static int s_state2_calls;

static void check(const char *name, int condition)
{
    if (!condition) {
        fprintf(stderr, "FAIL [state01]: %s\n", name);
        s_failures++;
    }
}

static void store_u16(u32 address, u16 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static u16 load_u16(u32 address)
{
    u16 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static void store_u32(u32 address, u32 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static u32 load_u32(u32 address)
{
    u32 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

s32 wm_80093978(s32 x, s32 z)
{
    s_terrain_calls++;
    s_terrain_event = ++s_event;
    s_terrain_x = x;
    s_terrain_z = z;
    return s_terrain;
}

void wm_8008BFD4(u16 tag, u32 vector, u16 half2, s16 value)
{
    s_ring_calls++;
    s_ring_event = ++s_event;
    s_ring_tag = tag;
    s_ring_vector = vector;
    s_ring_half2 = half2;
    s_ring_value = value;
    s_ring_snapshot[0] = load_u32(vector + 0u);
    s_ring_snapshot[1] = load_u32(vector + 4u);
    s_ring_snapshot[2] = load_u32(vector + 8u);
    s_ring_snapshot[3] = load_u32(vector + 12u);
}

void wm_8008E034(u32 vector)
{
    s_unload_calls++;
    s_unload_vector = vector;
}

void wm_80074794(s32 tag, u32 vector)
{
    (void)tag;
    (void)vector;
    s_publish_calls++;
}

s32 wm_8008E76C_state2(u32 slot)
{
    check("state2 receives parent slot", slot == SLOT);
    s_state2_calls++;
    return 1;
}

s32 wm_80097770(u32 slot_index, s32 value)
{
    (void)slot_index;
    (void)value;
    return 0;
}
void wm_8008E078(void) {}
void wm_80089160(u32 a0, u32 a1, u32 a2)
{
    (void)a0;
    (void)a1;
    (void)a2;
}
void wm_8008C040(u32 vector, s32 a1, s32 a2, u32 out1, u32 out2)
{
    (void)vector;
    (void)a1;
    (void)a2;
    (void)out1;
    (void)out2;
}
void wm_8007528C(void) {}
s32 wm_80094238(u32 position, u32 list_index)
{
    (void)position;
    (void)list_index;
    return 0;
}
s32 wm_80090FB4(u32 slot)
{
    (void)slot;
    return 0;
}
s32 wm_80093F18(u32 vector)
{
    (void)vector;
    return 0;
}
s32 wm_80094060(s32 a0, s32 a1)
{
    (void)a0;
    (void)a1;
    return 0;
}
s32 wm_80090E14(u32 slot)
{
    (void)slot;
    return 0;
}
s32 wm_8008E0F0(u32 position, u32 unused, u32 out)
{
    (void)position;
    (void)unused;
    (void)out;
    return -1;
}
s32 wm_80095CD4(u32 position, u32 velocity, u32 out, s32 scale, s32 mode)
{
    (void)position;
    (void)velocity;
    (void)out;
    (void)scale;
    (void)mode;
    return 0;
}

static void reset_case(u16 state, s16 predispatch)
{
    memset(g_PsxRam, 0, sizeof(g_PsxRam));
    memset(g_PsxScratchpad, 0, sizeof(g_PsxScratchpad));
    s_terrain = (s32)UINT32_C(0x00056000);
    s_terrain_calls = 0;
    s_terrain_x = 0;
    s_terrain_z = 0;
    s_ring_calls = 0;
    s_event = 0;
    s_terrain_event = 0;
    s_ring_event = 0;
    s_ring_tag = 0;
    s_ring_vector = 0;
    s_ring_half2 = 0;
    s_ring_value = 0;
    memset(s_ring_snapshot, 0, sizeof(s_ring_snapshot));
    s_unload_calls = 0;
    s_unload_vector = 0;
    s_publish_calls = 0;
    s_state2_calls = 0;

    store_u32(CONTEXT_PTR, CONTEXT);
    store_u32(POOL_PTR, POOL);
    store_u16(SLOT + 0x04u, (u16)predispatch);
    store_u16(SLOT + 0x20u, state);
    store_u32(SLOT + 0x28u, UINT32_C(0x81234000));
    store_u32(SLOT + 0x2Cu, UINT32_C(0xDEADBEEF));
    store_u32(SLOT + 0x30u, UINT32_C(0xFFF67000));
    store_u16(SLOT + 0x48u, UINT16_C(0x0456));
}

static void run_state(u16 state, s16 predispatch)
{
    s32 result;

    reset_case(state, predispatch);
    result = wm_8008E76C(7);

    check("returns retail constant one", result == 1);
    check("terrain sample uses signed X and Z",
          s_terrain_calls == 1 &&
          (u32)s_terrain_x == UINT32_C(0x81234000) &&
          (u32)s_terrain_z == UINT32_C(0xFFF67000));
    check("Y is terrain height minus 0x1000",
          load_u32(SLOT + 0x2Cu) == UINT32_C(0x00055000));
    check("ring helper receives retail arguments",
          s_ring_calls == 1 && s_ring_tag == 7u &&
          s_ring_vector == SLOT + 0x28u && s_ring_half2 == 64u &&
          s_ring_value == 1024);
    check("terrain Y store precedes ring helper",
          s_terrain_event == 1 && s_ring_event == 2 &&
          s_ring_snapshot[0] == UINT32_C(0x81234000) &&
          s_ring_snapshot[1] == UINT32_C(0x00055000) &&
          s_ring_snapshot[2] == UINT32_C(0xFFF67000) &&
          s_ring_snapshot[3] == 0u);
    check("shared tail publishes signed fixed-point coordinates",
          load_u32(CONTEXT + 0x5Cu) == UINT32_C(0xFFF81234) &&
          load_u32(CONTEXT + 0x08u) == UINT32_C(0xFFF81234) &&
          load_u32(CONTEXT + 0x60u) == UINT32_C(0x00000055) &&
          load_u32(CONTEXT + 0x0Cu) == UINT32_C(0x00000055) &&
          load_u32(CONTEXT + 0x64u) == UINT32_C(0xFFFFFF67) &&
          load_u32(CONTEXT + 0x10u) == UINT32_C(0xFFFFFF67));
    check("shared tail calls position helper and mirrors heading",
          s_unload_calls == 1 && s_unload_vector == SLOT + 0x28u &&
          load_u16(NATIVE_HEAD) == UINT16_C(0x0456));
    check("states zero and one do not publish vehicle pose ring",
          s_publish_calls == 0);
}

static void run_state2_route(s16 predispatch)
{
    reset_case(2u, predispatch);
    check("parent dispatches signed state two",
          wm_8008E76C(7) == 1 && s_state2_calls == 1);
    check("state two bypasses state zero/one body",
          s_terrain_calls == 0 && s_ring_calls == 0);
}

int main(void)
{
    static const s16 predispatch_values[] = {1, 5, 6, 7, 8, -1};
    size_t i;

    for (i = 0; i < sizeof(predispatch_values) / sizeof(predispatch_values[0]);
         i++) {
        run_state(0u, predispatch_values[i]);
        run_state(1u, predispatch_values[i]);
        run_state2_route(predispatch_values[i]);
    }

    if (s_failures != 0)
        return 1;
    puts("W34N127 VEHICLE STATE01 CERTIFICATE PASS");
    return 0;
}
