#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_mode_selector_73300.h"

uint8_t g_PsxRam[PSX_RAM_SIZE];
uint8_t g_PsxScratchpad[4096];

#define CONTROL UINT32_C(0x8006EE68)
#define FLAGS   UINT32_C(0x8006F8E5)
#define MODE    UINT32_C(0x8009BE10)

static int s_passes;
static int s_total;

static void store_u16(u32 address, u16 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
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

static void check(const char* name, int condition)
{
    s_total++;
    if (condition) {
        s_passes++;
        printf("PASS [selector]: %s\n", name);
    } else {
        printf("FAIL [selector]: %s\n", name);
    }
}

static void reset(u16 control, u8 flag0, u8 flag1, u8 flag2, u32 sentinel)
{
    memset(g_PsxRam, 0xA5, sizeof(g_PsxRam));
    store_u16(CONTROL, control);
    *(u8*)PSX_ADDR(FLAGS) = flag0;
    *(u8*)PSX_ADDR(FLAGS + 1u) = flag1;
    *(u8*)PSX_ADDR(FLAGS + 2u) = flag2;
    store_u32(MODE, sentinel);
}

static void run_case(const char* name, u16 control,
                     u8 flag0, u8 flag1, u8 flag2,
                     u32 before, u32 expected)
{
    u8 snapshot[32];
    u8* region = (u8*)PSX_ADDR(UINT32_C(0x8006EE60));
    char assertion[96];

    reset(control, flag0, flag1, flag2, before);
    memcpy(snapshot, region, sizeof(snapshot));
    wm_80073300();

    snprintf(assertion, sizeof(assertion), "%s mode result", name);
    check(assertion, load_u32(MODE) == expected);
    snprintf(assertion, sizeof(assertion), "%s inputs read-only", name);
    check(assertion, memcmp(snapshot, region, sizeof(snapshot)) == 0);
}

int main(void)
{
    const u32 sentinel = UINT32_C(0xC0DEC0DE);

    run_case("flags clear", UINT16_C(0), 0, 0, 0,
             sentinel, UINT32_C(1));
    run_case("flag set", UINT16_C(0), 0, 0x40, 0,
             sentinel, UINT32_C(2));
    run_case("index zero no-write", UINT16_C(0x4000), 1, 1, 1,
             sentinel, sentinel);
    run_case("index one", UINT16_C(0x4001), 0, 0, 0,
             sentinel, UINT32_C(4));
    run_case("index two", UINT16_C(0x4002), 0, 0, 0,
             sentinel, UINT32_C(5));
    run_case("cold index three", UINT16_C(0x4003), 0, 0, 0,
             sentinel, UINT32_C(7));
    run_case("index four", UINT16_C(0x4004), 0, 0, 0,
             sentinel, UINT32_C(7));
    run_case("masked cold index three", UINT16_C(0xE003), 0, 0, 0,
             sentinel, UINT32_C(7));
    run_case("index five no-write", UINT16_C(0x4005), 0, 0, 0,
             sentinel, sentinel);
    run_case("index max no-write", UINT16_C(0x5FFF), 0, 0, 0,
             sentinel, sentinel);

    printf("W34N119 MODE SELECTOR CERTIFICATE %s (%d/%d)\n",
           s_passes == s_total ? "PASS" : "FAIL", s_passes, s_total);
    return s_passes == s_total ? 0 : 1;
}
