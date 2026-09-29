/* Focused production certificate for retail 0x800816DC..0x800819C8. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_callback_817a0.h"
#include "world_map_scheduler.h"

uint8_t g_PsxRam[PSX_RAM_SIZE];
uint8_t g_PsxScratchpad[4096];

#define POOL        UINT32_C(0x800A0000)
#define POOL_PTR    UINT32_C(0x8009BE24)
#define SLOT_INDEX  3
#define SLOT        (POOL + (u32)SLOT_INDEX * UINT32_C(0x80))
#define CONTEXT     UINT32_C(0x800A1000)
#define CONTEXT_PTR UINT32_C(0x8009C620)
#define OWNER       (CONTEXT + UINT32_C(0xA8))
#define DESCRIPTOR  UINT32_C(0x800A2000)
#define STREAM_A    UINT32_C(0x800A3000)
#define STREAM_B    UINT32_C(0x800A3100)
#define RESET_X     UINT32_C(0x8009C5AC)
#define RESET_Y     UINT32_C(0x8009C5B0)
#define RESET_Z     UINT32_C(0x8009C5B4)
#define STREAM_SIDE UINT32_C(0x8009D7F0)

typedef struct TPageCall {
    int tp;
    int abr;
    int x;
    int y;
} TPageCall;

typedef struct ColorCall {
    u32 stream;
    s32 count;
    u8 red;
    u8 green;
    u8 blue;
} ColorCall;

static int failures;
static TPageCall tpage_calls[8];
static unsigned tpage_call_count;
static ColorCall color_calls[8];
static unsigned color_call_count;

static void check(int condition, const char *name)
{
    if (!condition) {
        fprintf(stderr, "ASSERTION %s FAILED\n", name);
        failures++;
    }
}

static void write8(u32 address, u8 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static void write16(u32 address, u16 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static void write32(u32 address, u32 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static u8 read8(u32 address)
{
    u8 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
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

static void reset_trace(void)
{
    memset(tpage_calls, 0, sizeof(tpage_calls));
    memset(color_calls, 0, sizeof(color_calls));
    tpage_call_count = 0u;
    color_call_count = 0u;
}

static void seed(void)
{
    u32 index;

    memset(g_PsxRam, 0, sizeof(g_PsxRam));
    memset(g_PsxScratchpad, 0, sizeof(g_PsxScratchpad));
    write32(POOL_PTR, POOL);
    write32(CONTEXT_PTR, CONTEXT);
    write32(RESET_X, UINT32_C(0x01234000));
    write32(RESET_Y, UINT32_C(0xFFE00000));
    write32(RESET_Z, UINT32_C(0x04567000));
    write32(OWNER + 0x40u, DESCRIPTOR);
    write32(OWNER + 0x48u, STREAM_A);
    write32(OWNER + 0x4Cu, STREAM_B);
    write16(DESCRIPTOR + 4u, 2u);
    for (index = 0u; index < 80u; index++) {
        write8(STREAM_A + index, (u8)(index + 1u));
        write8(STREAM_B + index, UINT8_C(0xCC));
    }
    write8(STREAM_A - 1u, UINT8_C(0xA5));
    write8(STREAM_A + 80u, UINT8_C(0x5A));
    write8(STREAM_B - 1u, UINT8_C(0xB6));
    write8(STREAM_B + 80u, UINT8_C(0x6B));
    reset_trace();
}

u16 GetTPage(int tp, int abr, int x, int y)
{
    unsigned call = tpage_call_count;

    check(call < 8u, "trace.tpage_capacity");
    if (call < 8u) {
        tpage_calls[call].tp = tp;
        tpage_calls[call].abr = abr;
        tpage_calls[call].x = x;
        tpage_calls[call].y = y;
    }
    tpage_call_count++;
    return (u16)(UINT16_C(0x1200) + (u16)call);
}

void wm_800809EC(u32 stream, s32 count, u8 red, u8 green, u8 blue)
{
    unsigned call = color_call_count;

    check(call < 8u, "trace.color_capacity");
    if (call < 8u) {
        color_calls[call].stream = stream;
        color_calls[call].count = count;
        color_calls[call].red = red;
        color_calls[call].green = green;
        color_calls[call].blue = blue;
    }
    color_call_count++;
}

static void test_stream_helper(void)
{
    u32 record;

    seed();
    wm_800816DC(OWNER, STREAM_A, 2u, 1);
    check(tpage_call_count == 2u &&
          tpage_calls[0].tp == 0 && tpage_calls[0].abr == 1 &&
          tpage_calls[0].x == 384 && tpage_calls[0].y == 0 &&
          tpage_calls[1].tp == 0 && tpage_calls[1].abr == 1 &&
          tpage_calls[1].x == 384 && tpage_calls[1].y == 0,
          "helper.tpage_args");
    for (record = 0u; record < 2u; record++) {
        u32 offset = record * 40u;
        check(read8(STREAM_A + offset + 3u) == 9u &&
              read8(STREAM_A + offset + 7u) == 46u &&
              read16(STREAM_A + offset + 22u) ==
                  (u16)(UINT16_C(0x1200) + (u16)record),
              "helper.packet_layout");
    }
    check(memcmp(PSX_ADDR(STREAM_A), PSX_ADDR(STREAM_B), 80u) == 0,
          "helper.mirror");
    check(read8(STREAM_B + 3u) == 9u &&
          read8(STREAM_B + 7u) == 46u &&
          read16(STREAM_B + 22u) == UINT16_C(0x1200) &&
          read8(STREAM_B + 43u) == 9u &&
          read8(STREAM_B + 47u) == 46u &&
          read16(STREAM_B + 62u) == UINT16_C(0x1201),
          "helper.copy_direction");
    check(read8(STREAM_A - 1u) == UINT8_C(0xA5) &&
          read8(STREAM_A + 80u) == UINT8_C(0x5A) &&
          read8(STREAM_B - 1u) == UINT8_C(0xB6) &&
          read8(STREAM_B + 80u) == UINT8_C(0x6B),
          "helper.write_bounds");
}

static void test_initializer(void)
{
    seed();
    check(wm_800817A0(SLOT_INDEX) == 1, "init.return");
    check(read16(SLOT + 0x20u) == 0u &&
          read32(SLOT + 0x28u) == UINT32_C(0x01234000) &&
          read32(SLOT + 0x2Cu) == UINT32_C(0xFFD00000) &&
          read32(SLOT + 0x30u) == UINT32_C(0x04567000),
          "init.position");
    check(read32(SLOT + 0x50u) == 0u &&
          read32(SLOT + 0x54u) == 0u &&
          read32(SLOT + 0x58u) == 0u,
          "init.fade");
    check(read32(CONTEXT + 0xB0u) == UINT32_C(0x00001234) &&
          read32(CONTEXT + 0xB4u) == UINT32_C(0xFFFFFD00) &&
          read32(CONTEXT + 0xB8u) == UINT32_C(0x00004567),
          "init.position_publish");
    check(tpage_call_count == 2u &&
          memcmp(PSX_ADDR(STREAM_A), PSX_ADDR(STREAM_B), 80u) == 0,
          "init.stream_setup");
}

static void test_latch_one(void)
{
    seed();
    write16(SLOT + 0x04u, 1u);
    write16(SLOT + 0x20u, 0u);
    write32(SLOT + 0x50u, 10u);
    write32(SLOT + 0x54u, 20u);
    write32(SLOT + 0x58u, 30u);
    write32(STREAM_SIDE, 1u);
    check(wm_80081868(SLOT_INDEX) == 1, "update.return");
    check(read16(SLOT + 0x04u) == 0u &&
          read16(SLOT + 0x20u) == 1u &&
          read32(SLOT + 0x50u) == 11u &&
          read32(SLOT + 0x54u) == 21u &&
          read32(SLOT + 0x58u) == 31u,
          "update.latch1");
    check(color_call_count == 1u && color_calls[0].stream == STREAM_B &&
          color_calls[0].count == 2 && color_calls[0].red == 11u &&
          color_calls[0].green == 21u && color_calls[0].blue == 31u,
          "update.color_call");
}

static void test_latch_two(void)
{
    u32 record;

    seed();
    write16(SLOT + 0x04u, 2u);
    write16(SLOT + 0x20u, 1u);
    write32(SLOT + 0x50u, 9u);
    write32(SLOT + 0x54u, 10u);
    write32(SLOT + 0x58u, 11u);
    write32(STREAM_SIDE, 1u);
    for (record = 0u; record < 2u; record++) {
        u32 primitive = STREAM_B + record * 40u;
        write8(primitive + 4u, 1u);
        write8(primitive + 5u, 2u);
        write8(primitive + 6u, 3u);
        write8(primitive + 7u, UINT8_C(0x2E));
    }
    check(wm_80081868(SLOT_INDEX) == 1 &&
          read16(SLOT + 0x04u) == 0u &&
          read16(SLOT + 0x20u) == 0u,
          "update.latch2_state");
    for (record = 0u; record < 2u; record++) {
        u32 primitive = STREAM_B + record * 40u;
        check(read8(primitive + 4u) == 128u &&
              read8(primitive + 5u) == 128u &&
              read8(primitive + 6u) == 128u &&
              read8(primitive + 7u) == UINT8_C(0x2C),
              "update.latch2_restore");
    }
    check(read32(SLOT + 0x50u) == 9u &&
          read32(SLOT + 0x54u) == 10u &&
          read32(SLOT + 0x58u) == 11u &&
          color_call_count == 1u && color_calls[0].red == 9u &&
          color_calls[0].green == 10u && color_calls[0].blue == 11u,
          "update.latch2_no_advance");
}

static void test_fade_saturation(void)
{
    seed();
    write16(SLOT + 0x20u, 1u);
    write32(SLOT + 0x50u, 254u);
    write32(SLOT + 0x54u, 12u);
    write32(SLOT + 0x58u, 13u);
    check(wm_80081868(SLOT_INDEX) == 1 &&
          read16(SLOT + 0x20u) == 0u &&
          read32(SLOT + 0x50u) == 255u &&
          read32(SLOT + 0x54u) == 255u &&
          read32(SLOT + 0x58u) == 255u &&
          color_call_count == 1u && color_calls[0].red == 255u &&
          color_calls[0].green == 255u && color_calls[0].blue == 255u,
          "update.fade_saturation");
}

static void test_scheduler_resolution(void)
{
    seed();
    write32(SLOT + 0x18u, UINT32_C(0x800817A0));
    write32(SLOT + 0x1Cu, UINT32_C(0x80081868));
    write16(SLOT + 0u, 0u);
    wm_sched_reset();
    wm_sched_callback_registry_clear();
    wm_80097800();
    check(wm_sched_get_callbacks_executed() == 1 &&
          wm_sched_get_missing_hits() == 0 &&
          wm_sched_get_invalid_hits() == 0 && read16(SLOT + 0u) == 1u,
          "scheduler.init_resolution");

    reset_trace();
    wm_sched_reset();
    wm_80097800();
    check(wm_sched_get_callbacks_executed() == 1 &&
          wm_sched_get_missing_hits() == 0 &&
          wm_sched_get_invalid_hits() == 0 && color_call_count == 1u,
          "scheduler.update_resolution");
}

int main(void)
{
    test_stream_helper();
    test_initializer();
    test_latch_one();
    test_latch_two();
    test_fade_saturation();
    test_scheduler_resolution();
    if (failures != 0) {
        fprintf(stderr,
                "W34N105 MODE16 OBJECT FADE CERTIFICATE: %d failure(s)\n",
                failures);
        return 1;
    }
    puts("W34N105 MODE16 OBJECT FADE CERTIFICATE PASS");
    return 0;
}
