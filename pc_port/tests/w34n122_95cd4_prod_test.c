#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_helper_95cd4.h"

#define POS          UINT32_C(0x800A1000)
#define VEL          UINT32_C(0x800A1010)
#define OUT          UINT32_C(0x800A1020)
#define SCRATCH_VEC  UINT32_C(0x1F800060)
#define FRAME_ATTR   UINT32_C(0x801FFDE0)
#define CANDIDATES   UINT32_C(0x8009D718)

uint8_t g_PsxRam[PSX_RAM_SIZE];
uint8_t g_PsxScratchpad[4096];

static int s_passes;
static int s_total;
static int s_wrap_calls;
static s32 s_terrain;
static s32 s_candidate_count;
static u16 s_attribute;
static s32 s_filter_results[8];
static int s_filter_calls;
static u32 s_filter_attributes[8];
static u32 s_filter_nodes[8];
static int s_resolver_calls;
static u32 s_resolver_pos;
static u32 s_resolver_vel;
static u32 s_resolver_out;
static s32 s_resolver_scale;
static s32 s_resolver_mode;
static s32 s_resolver_result;

static void check(const char* name, int condition)
{
    s_total++;
    if (condition) {
        s_passes++;
        printf("PASS [95cd4]: %s\n", name);
    } else {
        printf("FAIL [95cd4]: %s\n", name);
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

static void reset_case(void)
{
    memset(g_PsxRam, 0, sizeof(g_PsxRam));
    memset(g_PsxScratchpad, 0, sizeof(g_PsxScratchpad));
    s_wrap_calls = 0;
    s_terrain = INT32_C(0x20000);
    s_candidate_count = 0;
    s_attribute = UINT16_C(0x1357);
    memset(s_filter_results, 0, sizeof(s_filter_results));
    s_filter_calls = 0;
    memset(s_filter_attributes, 0, sizeof(s_filter_attributes));
    memset(s_filter_nodes, 0, sizeof(s_filter_nodes));
    s_resolver_calls = 0;
    s_resolver_pos = 0;
    s_resolver_vel = 0;
    s_resolver_out = 0;
    s_resolver_scale = 0;
    s_resolver_mode = 0;
    s_resolver_result = 37;
}

void wm_80093354(u32 vec_addr)
{
    check("wrap helper receives one of the two retail vectors",
          vec_addr == SCRATCH_VEC || vec_addr == OUT);
    s_wrap_calls++;
}

s32 wm_80093978(s32 x, s32 z)
{
    (void)x;
    (void)z;
    return s_terrain;
}

s32 wm_80084D00(u32 pos_vec, u32 out_attr)
{
    check("region scanner receives guest scratch target",
          pos_vec == SCRATCH_VEC);
    check("region scanner receives reserved retail frame attribute",
          out_attr == FRAME_ATTR);
    if (s_candidate_count != 0)
        store_u16(out_attr, s_attribute);
    return s_candidate_count;
}

s32 wm_80085418(u32 pos_vec, s32 y_offset, u32 attr, u32 node_id)
{
    int index = s_filter_calls;
    check("candidate filter receives retail vector and Y offset",
          pos_vec == SCRATCH_VEC && y_offset == 0x70);
    if (index < 8) {
        s_filter_attributes[index] = attr;
        s_filter_nodes[index] = node_id;
        s_filter_calls++;
        return s_filter_results[index];
    }
    return 1;
}

s32 wm_800951A8(u32 pos, u32 vel, u32 out, s32 scale, s32 mode)
{
    s_resolver_calls++;
    s_resolver_pos = pos;
    s_resolver_vel = vel;
    s_resolver_out = out;
    s_resolver_scale = scale;
    s_resolver_mode = mode;
    return s_resolver_result;
}

static void set_vector(u32 address, u32 x, u32 y, u32 z)
{
    store_u32(address + 0u, x);
    store_u32(address + 4u, y);
    store_u32(address + 8u, z);
}

static void test_empty_candidate_fallback(void)
{
    u8 pos_before[12];
    u8 vel_before[12];
    s32 result;

    reset_case();
    set_vector(POS, UINT32_C(0x10000), UINT32_C(0x30000),
               UINT32_C(0x50000));
    set_vector(VEL, UINT32_C(0x1000), UINT32_C(0x2000),
               UINT32_C(0x3000));
    memcpy(pos_before, PSX_ADDR(POS), sizeof(pos_before));
    memcpy(vel_before, PSX_ADDR(VEL), sizeof(vel_before));
    s_terrain = INT32_C(0x18000);

    result = wm_80095CD4(POS, VEL, OUT, INT32_C(0x1000), -1234);
    check("empty candidate path returns resolver result", result == 37);
    check("exact caller mode forwarded",
          s_resolver_calls == 1 && s_resolver_pos == POS &&
          s_resolver_vel == VEL && s_resolver_out == OUT &&
          s_resolver_scale == INT32_C(0x1000) &&
          s_resolver_mode == -1234);
    check("exact 12-bit scale",
          load_u32(SCRATCH_VEC + 0u) == UINT32_C(0x11000) &&
          load_u32(SCRATCH_VEC + 8u) == UINT32_C(0x53000));
    check("terrain offset applied",
          load_u32(SCRATCH_VEC + 4u) == UINT32_C(0xFFFF8000) &&
          load_u32(OUT + 4u) == UINT32_C(0xFFFF8000));
    check("both vectors pass through retail wrap helper", s_wrap_calls == 2);
    check("position and velocity inputs are read-only",
          memcmp(pos_before, PSX_ADDR(POS), sizeof(pos_before)) == 0 &&
          memcmp(vel_before, PSX_ADDR(VEL), sizeof(vel_before)) == 0);
}

static void test_all_rejected_delegates(void)
{
    s32 result;

    reset_case();
    set_vector(POS, 0u, 0u, 0u);
    set_vector(VEL, UINT32_C(0x1000), 0u, UINT32_C(0x1000));
    s_terrain = INT32_C(0x20000);
    s_candidate_count = 4;
    s_filter_results[0] = 0;
    s_filter_results[1] = 0;
    store_u16(CANDIDATES + 0u, UINT16_C(0x1111));
    store_u16(CANDIDATES + 4u, UINT16_C(0x2222));

    result = wm_80095CD4(POS, VEL, OUT, INT32_C(0x1000), 9);
    check("all rejected delegates to resolver",
          result == 37 && s_resolver_calls == 1);
    check("fixed attribute reused",
          s_filter_calls == 2 &&
          s_filter_attributes[0] == UINT32_C(0x1357) &&
          s_filter_attributes[1] == UINT32_C(0x1357));
    check("candidate records invalidated",
          load_u16(CANDIDATES + 0u) == UINT16_C(0xFFFF) &&
          load_u16(CANDIDATES + 4u) == UINT16_C(0xFFFF));
    check("attribute frame remains scanner output",
          load_u16(FRAME_ATTR) == UINT16_C(0x1357));
    check("candidate walk advances by four bytes",
          s_filter_nodes[0] == UINT32_C(0x1111) &&
          s_filter_nodes[1] == UINT32_C(0x2222));
}

static void test_surviving_candidate_reflects(void)
{
    s32 result;

    reset_case();
    set_vector(POS, 0u, 0u, 0u);
    set_vector(VEL, UINT32_C(0x1000), UINT32_C(0xFFFFE000),
               UINT32_C(0x80000000));
    s_candidate_count = 4;
    s_filter_results[0] = 0;
    s_filter_results[1] = 1;
    store_u16(CANDIDATES + 0u, UINT16_C(0x0101));
    store_u16(CANDIDATES + 4u, UINT16_C(0x0202));

    result = wm_80095CD4(POS, VEL, OUT, INT32_C(0x1000), 7);
    check("surviving candidate bypasses resolver",
          result == 0 && s_resolver_calls == 0);
    check("half reflected velocity",
          load_u32(OUT + 0u) == UINT32_C(0xFFFFF800) &&
          load_u32(OUT + 4u) == UINT32_C(0x00001000) &&
          load_u32(OUT + 8u) == UINT32_C(0xC0000000));
    check("only rejected candidate record invalidated",
          load_u16(CANDIDATES + 0u) == UINT16_C(0xFFFF) &&
          load_u16(CANDIDATES + 4u) == UINT16_C(0x0202));
}

static void test_lower_crossing_clamp(void)
{
    reset_case();
    set_vector(POS, 0u, UINT32_C(0xFFD00000), 0u);
    set_vector(VEL, 0u, 0u, 0u);
    s_terrain = 0;

    (void)wm_80095CD4(POS, VEL, OUT, 0, 0);
    check("lower crossing clamp",
          load_u32(SCRATCH_VEC + 4u) == UINT32_C(0xFFD80000) &&
          load_u32(OUT + 4u) == UINT32_C(0xFFD80000));
}

int main(void)
{
    test_empty_candidate_fallback();
    test_all_rejected_delegates();
    test_surviving_candidate_reflects();
    test_lower_crossing_clamp();
    printf("W34N122 95CD4 CERTIFICATE %s (%d/%d)\n",
           s_passes == s_total ? "PASS" : "FAIL", s_passes, s_total);
    return s_passes == s_total ? 0 : 1;
}
