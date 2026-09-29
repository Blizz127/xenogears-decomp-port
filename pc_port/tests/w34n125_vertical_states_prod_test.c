/* Focused production certificate for the retail vertical-transition states
 * 8/12/16/20 of 0x8008E76C ([0x8008F690, 0x8008F9D8) and the copy arm
 * [0x800905E8, 0x80090620)) and the sound-bank switch 0x800767D4. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_state_vertical_8f690.h"

uint8_t g_PsxRam[PSX_RAM_SIZE];
uint8_t g_PsxScratchpad[4096];
unsigned char g_GameState[0x2358];

#define POOL        UINT32_C(0x800A0000)
#define POOL_PTR    UINT32_C(0x8009BE24)
#define SLOT        (POOL + UINT32_C(7) * UINT32_C(0x80))
#define CONTEXT     UINT32_C(0x800A8000)
#define CONTEXT_PTR UINT32_C(0x8009C620)
#define GS          UINT32_C(0x8006D634)
#define SONG_A      UINT32_C(0x800B0000)
#define SONG_B      UINT32_C(0x800B4000)

static int failures;
static unsigned tail_count;
static unsigned claim_count;
static u32 claims[16][2];
static unsigned marker_count;
static u32 marker_id, marker_vector, marker_flags;
static unsigned destroy_count;
static u32 destroys[4];
static unsigned palette_count;
static s32 region_result;
static unsigned stop_count, release_count, start_count;
static void *released_manager;
static u32 decoded_entry;
static void *started_manager;
static int start_level, start_steps;

unsigned char D_80062648[0x100];
void *D_80062528;
static unsigned char manager_object[4];

static void check(int condition, const char *name)
{
    if (!condition) {
        fprintf(stderr, "ASSERTION %s FAILED\n", name);
        failures++;
    }
}

static void write16(u32 address, u16 value) { memcpy(PSX_ADDR(address), &value, sizeof(value)); }
static void write32(u32 address, u32 value) { memcpy(PSX_ADDR(address), &value, sizeof(value)); }
static u16 read16(u32 address) { u16 v; memcpy(&v, PSX_ADDR(address), sizeof(v)); return v; }
static u32 read32(u32 address) { u32 v; memcpy(&v, PSX_ADDR(address), sizeof(v)); return v; }

s32 wm_8008E76C_shared_tail(u32 slot) { (void)slot; tail_count++; return 1; }
s32 wm_80097770(u32 slot_idx, s32 value)
{
    if (claim_count < 16u) {
        claims[claim_count][0] = slot_idx;
        claims[claim_count][1] = (u32)value;
    }
    claim_count++;
    return 1;
}
s32 wm_80093F18(u32 vec_addr) { (void)vec_addr; return region_result; }
void wm_80089160(u32 a0, u32 a1, u32 a2) { marker_count++; marker_id = a0; marker_vector = a1; marker_flags = a2; }
void wm_800894C8(u32 record_index) { if (destroy_count < 4u) destroys[destroy_count] = record_index; destroy_count++; }
void wm_80075228(void) { palette_count++; }

void func_80039CC4(void) { stop_count++; }
void func_800399D4(void *manager) { release_count++; released_manager = manager; }
int ArchiveDecodeAlignedSize(unsigned int entry_index) { decoded_entry = entry_index; return 16; }
void *func_80039850(void *song_file) { check(song_file == D_80062648, "bank.start_file"); return manager_object; }
void func_80039A80(void *manager, int level, int steps) { start_count++; started_manager = manager; start_level = level; start_steps = steps; }

static void seed(u16 state, u32 y, u32 ground)
{
    u32 i;

    memset(g_PsxRam, 0, sizeof(g_PsxRam));
    memset(g_PsxScratchpad, 0, sizeof(g_PsxScratchpad));
    memset(g_GameState, 0, sizeof(g_GameState));
    memset(D_80062648, 0, sizeof(D_80062648));
    D_80062528 = (void *)&failures;
    write32(POOL_PTR, POOL);
    write32(CONTEXT_PTR, CONTEXT);
    write16(SLOT + 0x20u, state);
    write32(SLOT + 0x28u, UINT32_C(0x00123000));
    write32(SLOT + 0x2Cu, y);
    write32(SLOT + 0x30u, UINT32_C(0x00456000));
    write32(SLOT + 0x34u, UINT32_C(0x11223344));
    write16(SLOT + 0x48u, UINT16_C(0x0ABC));
    write32(SLOT + 0x68u, ground);
    write32(SLOT + 0x38u, 7u);
    write32(SLOT + 0x3Cu, 7u);
    write32(SLOT + 0x40u, 7u);
    write32(SLOT + 0x74u, 7u);
    write32(UINT32_C(0x8009C888), SONG_A);
    write32(UINT32_C(0x8009D800), 0x1111u);
    write32(UINT32_C(0x8009C884), SONG_B);
    write32(UINT32_C(0x8009D3D0), 0x2222u);
    for (i = 0u; i < 16u; i++) {
        *(u8 *)PSX_ADDR(SONG_A + i) = (u8)(0xA0u + i);
        *(u8 *)PSX_ADDR(SONG_B + i) = (u8)(0xB0u + i);
    }
    g_GameState[0x1D35] = 0x02u;
    g_GameState[0x1D36] = 0x03u;
    g_GameState[0x1834] = 0xFFu;
    g_GameState[0x1835] = 0xFFu;
    tail_count = claim_count = marker_count = destroy_count = palette_count = 0u;
    stop_count = release_count = start_count = 0u;
    memset(claims, 0, sizeof(claims));
    region_result = 0;
}

static int claims_are(const u32 (*want)[2], unsigned n)
{
    unsigned i;

    if (claim_count != n)
        return 0;
    for (i = 0u; i < n; i++)
        if (claims[i][0] != want[i][0] || claims[i][1] != want[i][1])
            return 0;
    return 1;
}

static void test_state8_climbing(void)
{
    seed(8u, 0u, 0u);
    check(wm_8008E76C_state8(SLOT) == 1, "s8.return");
    check(read32(SLOT + 0x2Cu) == UINT32_C(0xFFFF8000) && read16(SLOT + 0x20u) == 8u,
          "s8.climb");
    check(read32(UINT32_C(0x8009D55C)) == UINT32_C(0x00123000) &&
          read32(UINT32_C(0x8009D560)) == UINT32_C(0xFFFF8000) &&
          read32(UINT32_C(0x8009D564)) == UINT32_C(0x00456000) &&
          read32(UINT32_C(0x8009D568)) == UINT32_C(0x11223344) &&
          read16(UINT32_C(0x8009D52C)) == UINT16_C(0x0ABC),
          "s8.copy_arm");
    check(tail_count == 1u && start_count == 0u, "s8.tail");
}

static void test_state8_top(void)
{
    seed(8u, UINT32_C(0xFFE24000), 0u);
    write16(SLOT + 4u, 0xBu);
    write32(SLOT + 0x64u, 9u);
    check(wm_8008E76C_state8(SLOT) == 1, "s8.top_return");
    check(read32(SLOT + 0x2Cu) == UINT32_C(0xFFE20000) && read16(SLOT + 0x20u) == 2u &&
          read16(SLOT + 4u) == 0u && read32(SLOT + 0x64u) == 0u,
          "s8.top_state");
    check(stop_count == 1u && release_count == 1u && released_manager == (void *)&failures &&
          decoded_entry == 0x1111u && D_80062648[0] == 0xA0u && D_80062648[15] == 0xAFu &&
          D_80062528 == manager_object && start_count == 1u &&
          started_manager == manager_object && start_level == 0x7F && start_steps == 0,
          "s8.bank");
    check(read32(UINT32_C(0x8009D560)) == UINT32_C(0xFFE20000) && tail_count == 1u,
          "s8.top_copy");

    seed(8u, UINT32_C(0xFFE24000), 0u);
    write16(SLOT + 4u, 0xAu);
    (void)wm_8008E76C_state8(SLOT);
    check(read32(SLOT + 0x2Cu) == UINT32_C(0xFFE20000) && read16(SLOT + 0x20u) == 8u &&
          start_count == 0u,
          "s8.top_without_trigger");
}

static void test_state12(void)
{
    seed(12u, UINT32_C(0x00001000), UINT32_C(0x00001400));
    (void)wm_8008E76C_state12(SLOT);
    check(read32(SLOT + 0x2Cu) == UINT32_C(0x00001400) && read16(SLOT + 0x20u) == 3u &&
          read16(CONTEXT + 0xFCu) == 1u && read16(CONTEXT + 0xA8u) == 1u &&
          read16(CONTEXT + 0x54u) == 1u && tail_count == 1u,
          "s12.settle");
}

static void test_state16_landing(void)
{
    static const u32 want[][2] = {{8, 10}, {9, 10}, {10, 10}, {4, 3}, {1, 3},
                                  {5, 3}, {2, 3}, {6, 3}, {3, 3}};

    seed(16u, UINT32_C(0x00008000), UINT32_C(0x0000C000));
    (void)wm_8008E76C_state16(SLOT);
    check(read32(SLOT + 0x2Cu) == UINT32_C(0x0000C000) && read16(SLOT + 0x20u) == 1u,
          "s16.clamp");
    check(decoded_entry == 0x2222u && D_80062648[0] == 0xB0u && start_count == 1u, "s16.bank");
    check(claims_are(want, 9u), "s16.party_order");
    check(g_GameState[0x22B1] == 1u && g_GameState[0x22B2] == 1u && g_GameState[0x22B3] == 1u &&
          *(u8 *)PSX_ADDR(GS + 0x22B3u) == 1u,
          "s16.party_flags");
    check(read32(SLOT + 0x38u) == 0u && read32(SLOT + 0x3Cu) == 0u &&
          read32(SLOT + 0x40u) == 0u && read32(SLOT + 0x74u) == 0u &&
          g_GameState[0x1835] == 0x3Fu,
          "s16.stop");
    check(destroy_count == 2u && destroys[0] == 0u && destroys[1] == 1u &&
          palette_count == 1u, "s16.cleanup");
    check(read32(UINT32_C(0x8009D560)) == UINT32_C(0x0000C000) && tail_count == 1u,
          "s16.height_publish");
}

static void test_state16_falling(void)
{
    seed(16u, UINT32_C(0xFFE00000), UINT32_C(0x00100000));
    write32(SLOT + 0x7Cu, 1u);
    region_result = 3;
    (void)wm_8008E76C_state16(SLOT);
    check(read32(SLOT + 0x2Cu) == UINT32_C(0xFFE08000) && read16(SLOT + 0x20u) == 16u &&
          read32(SLOT + 0x7Cu) == 0u,
          "s16.fall");
    check(marker_count == 1u && marker_id == 0x3Eu && marker_vector == UINT32_C(0x1F8000A0) &&
          marker_flags == 0u && read16(UINT32_C(0x1F8000A0)) == 0x123u &&
          read16(UINT32_C(0x1F8000A2)) == 0x100u && read16(UINT32_C(0x1F8000A4)) == 0x456u,
          "s16.marker");
    check(read32(UINT32_C(0x8009D560)) == UINT32_C(0xFFE08000) && start_count == 0u,
          "s16.fall_publish");
}

static void test_state20(void)
{
    static const u32 want[][2] = {{8, 10}, {1, 3}, {4, 3}, {2, 3}, {5, 3}, {3, 3}, {6, 3}};

    seed(20u, UINT32_C(0x00001400), UINT32_C(0x00001000));
    write16(CONTEXT + 0xFCu, 1u);
    (void)wm_8008E76C_state20(SLOT);
    check(read32(SLOT + 0x2Cu) == UINT32_C(0x00001000) && read16(SLOT + 0x20u) == 1u &&
          claims_are(want, 7u) && read16(CONTEXT + 0xFCu) == 0u && palette_count == 1u &&
          start_count == 0u && read32(UINT32_C(0x8009D560)) == UINT32_C(0x00001000),
          "s20.land");
}

int main(void)
{
    test_state8_climbing();
    test_state8_top();
    test_state12();
    test_state16_landing();
    test_state16_falling();
    test_state20();
    if (failures != 0) {
        fprintf(stderr, "W34N125 VERTICAL STATES CERTIFICATE: %d failure(s)\n", failures);
        return 1;
    }
    puts("W34N125 VERTICAL STATES CERTIFICATE PASS");
    return 0;
}
