/* Focused production-linked certificate for retail helper 0x80073398. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_helper_73398.h"

uint8_t g_PsxRam[PSX_RAM_SIZE];
uint8_t g_PsxScratchpad[4096];

#define MODE_ADDR      0x8009BE10u
#define FLAG_ADDR      0x8006EE6Au
#define DIRECT_X_ADDR  0x8006EE54u
#define DIRECT_Z_ADDR  0x8006EE56u
#define DIRECT_ID_ADDR 0x8006EE58u
#define DFF_ID_ADDR    0x8006EE66u
#define STATE_ID_ADDR  0x8009C584u
#define POS_X_ADDR     0x8009C5ACu
#define POS_Y_ADDR     0x8009C5B0u
#define POS_Z_ADDR     0x8009C5B4u

typedef struct Event {
    enum Wm73398TestEvent kind;
    u32 address;
    u32 value;
} Event;

static Event events[16];
static int event_count;
static int failures;
static int dff4_calls;
static u32 dff4_arg;
static uint8_t before_ram[PSX_RAM_SIZE];

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

static u32 read32(u32 address)
{
    u32 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

void wm_73398_test_event(enum Wm73398TestEvent kind,
                         u32 address,
                         u32 value)
{
    if (event_count < (int)(sizeof(events) / sizeof(events[0]))) {
        events[event_count].kind = kind;
        events[event_count].address = address;
        events[event_count].value = value;
        event_count++;
    }
}

void wm_8008DFF4(u32 out_addr)
{
    dff4_calls++;
    dff4_arg = out_addr;
    write32(out_addr + 0u, 0x11111111u);
    write32(out_addr + 4u, 0x22222222u);
    write32(out_addr + 8u, 0x33333333u);
}

static int byte_in_range(size_t offset, u32 address, size_t size)
{
    size_t start = (size_t)(address & 0x1FFFFFu);
    return offset >= start && offset < start + size;
}

static int allowed_write(size_t offset, int group)
{
    if (byte_in_range(offset, FLAG_ADDR, 2u) ||
        byte_in_range(offset, STATE_ID_ADDR, 4u))
        return 1;
    if (group == 1) {
        return byte_in_range(offset, POS_X_ADDR, 4u) ||
               byte_in_range(offset, POS_Z_ADDR, 4u);
    }
    if (group == 2)
        return byte_in_range(offset, POS_X_ADDR, 12u);
    return 0;
}

static void check_write_set(int group, const char* name)
{
    size_t i;
    for (i = 0u; i < sizeof(g_PsxRam); i++) {
        if (before_ram[i] != g_PsxRam[i] && !allowed_write(i, group)) {
            fprintf(stderr,
                    "ASSERTION %s FAILED offset=0x%zx before=%02x after=%02x\n",
                    name, i, before_ram[i], g_PsxRam[i]);
            failures++;
            return;
        }
    }
}

static void seed(u32 mode)
{
    memset(g_PsxRam, 0xA5, sizeof(g_PsxRam));
    memset(g_PsxScratchpad, 0x5A, sizeof(g_PsxScratchpad));
    write32(MODE_ADDR, mode);
    write16(FLAG_ADDR, 0x6B6Bu);
    write16(DIRECT_X_ADDR, 0xF234u);
    write16(DIRECT_Z_ADDR, 0x8123u);
    write16(DIRECT_ID_ADDR, 0xCAFEu);
    write16(DFF_ID_ADDR, 0xBEEFu);
    write32(STATE_ID_ADDR, 0x44556677u);
    write32(POS_X_ADDR, 0x89ABCDEFu);
    write32(POS_Y_ADDR, 0x13579BDFu);
    write32(POS_Z_ADDR, 0x2468ACE0u);
    event_count = 0;
    dff4_calls = 0;
    dff4_arg = 0u;
    memcpy(before_ram, g_PsxRam, sizeof(before_ram));
}

static void check_event(int index,
                        enum Wm73398TestEvent kind,
                        u32 address,
                        u32 value,
                        const char* name)
{
    check(index < event_count, name);
    if (index >= event_count)
        return;
    check(events[index].kind == kind && events[index].address == address &&
              events[index].value == value,
          name);
}

static void test_direct_mode(u32 mode)
{
    seed(mode);
    wm_80073398();

    check(read16(FLAG_ADDR) == 0u, "flag.cleared");
    check(read32(POS_X_ADDR) == 0x0F234000u,
          "direct.values.unsigned");
    if (mode == 2u)
        check(read32(POS_X_ADDR) == 0x0F234000u,
              "mode2.group.direct");
    check(read32(POS_Z_ADDR) == 0x08123000u,
          "direct.values.mapping");
    check(read32(POS_Y_ADDR) == 0x13579BDFu,
          "direct.c5b0.preserved");
    check(read32(STATE_ID_ADDR) == 0x0000CAFEu,
          "direct.state_id");
    check(dff4_calls == 0, "direct.no_dff4");
    check(event_count == 8, "direct.event_count");
    check_event(0, WM_73398_TEST_READ, MODE_ADDR, mode,
                "order.mode_before_clear");
    check_event(1, WM_73398_TEST_WRITE, FLAG_ADDR, 0u,
                "order.mode_before_clear");
    check_event(2, WM_73398_TEST_READ, DIRECT_X_ADDR, 0xF234u,
                "direct.retail_order");
    check_event(3, WM_73398_TEST_READ, DIRECT_ID_ADDR, 0xCAFEu,
                "direct.retail_order");
    check_event(4, WM_73398_TEST_WRITE, POS_X_ADDR, 0x0F234000u,
                "direct.retail_order");
    check_event(5, WM_73398_TEST_READ, DIRECT_Z_ADDR, 0x8123u,
                "direct.retail_order");
    check_event(6, WM_73398_TEST_WRITE, STATE_ID_ADDR, 0xCAFEu,
                "direct.retail_order");
    check_event(7, WM_73398_TEST_WRITE, POS_Z_ADDR, 0x08123000u,
                "direct.retail_order");
    check_write_set(1, "direct.write_set");
}

static void test_dff_mode(u32 mode)
{
    seed(mode);
    wm_80073398();

    check(read16(FLAG_ADDR) == 0u, "flag.cleared");
    check(dff4_calls == 1 && dff4_arg == POS_X_ADDR,
          "dff4.called");
    check(read32(POS_X_ADDR) == 0x11111111u &&
              read32(POS_Y_ADDR) == 0x22222222u &&
              read32(POS_Z_ADDR) == 0x33333333u,
          "dff4.destination");
    check(read32(STATE_ID_ADDR) == 0x0000BEEFu,
          "dff4.state_id");
    check(event_count == 5, "dff4.event_count");
    check_event(0, WM_73398_TEST_READ, MODE_ADDR, mode,
                "order.mode_before_clear");
    check_event(1, WM_73398_TEST_WRITE, FLAG_ADDR, 0u,
                "order.mode_before_clear");
    check_event(2, WM_73398_TEST_DFF4, POS_X_ADDR, 0u,
                "dff4.retail_order");
    check_event(3, WM_73398_TEST_READ, DFF_ID_ADDR, 0xBEEFu,
                "dff4.retail_order");
    check_event(4, WM_73398_TEST_WRITE, STATE_ID_ADDR, 0xBEEFu,
                "dff4.retail_order");
    check_write_set(2, "dff4.write_set");
}

static void test_noop_mode(u32 mode)
{
    seed(mode);
    wm_80073398();

    check(read16(FLAG_ADDR) == 0u, "flag.cleared");
    check(read32(POS_X_ADDR) == 0x89ABCDEFu &&
              read32(POS_Y_ADDR) == 0x13579BDFu &&
              read32(POS_Z_ADDR) == 0x2468ACE0u &&
              read32(STATE_ID_ADDR) == 0x44556677u,
          "noop.outputs.preserved");
    check(dff4_calls == 0, "noop.no_dff4");
    check(event_count == 2, "noop.event_count");
    check_event(0, WM_73398_TEST_READ, MODE_ADDR, mode,
                "order.mode_before_clear");
    check_event(1, WM_73398_TEST_WRITE, FLAG_ADDR, 0u,
                "order.mode_before_clear");
    check_write_set(0, "noop.write_set");
}

int main(void)
{
    static const u32 noop_modes[] = {
        0u, 3u, 6u, 8u, 0xFFFFFFFFu
    };
    size_t i;

    test_direct_mode(1u);
    test_direct_mode(2u);
    test_dff_mode(4u);
    test_dff_mode(5u);
    test_dff_mode(7u);
    for (i = 0u; i < sizeof(noop_modes) / sizeof(noop_modes[0]); i++)
        test_noop_mode(noop_modes[i]);

    if (failures != 0) {
        fprintf(stderr, "W34N25 73398 CERTIFICATE: %d failure(s)\n",
                failures);
        return 1;
    }
    puts("W34N25 73398 CERTIFICATE PASS");
    return 0;
}
