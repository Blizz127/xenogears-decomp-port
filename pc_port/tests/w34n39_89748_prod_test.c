/* W34N39 — production-linked certificate for full retail wm_80089748. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "psyq/libgte.h"
#include "world_map_helper_89748.h"

uint8_t g_PsxRam[PSX_RAM_SIZE];

#define SOURCE_TABLE    0x800D0000u
#define PARTICLE_POOL   0x80120000u
#define BCC0            0x8009BCC0u
#define BDF4            0x8009BDF4u
#define SCRATCH         0x1F800000u
#define SOURCE_STRIDE   0x54u
#define PARTICLE_STRIDE 0x4Cu
#define SOURCE_COUNT    512u

static int s_failures;
static int s_rot_calls;
static int s_apply_calls;
static int s_normal_calls;
static int s_ratan_calls;
static int s_rand_calls;
static int s_tick_calls;
static u32 s_rot_input;
static u32 s_rot_output;
static u32 s_apply_input[4];
static u32 s_apply_output[4];
static VECTOR s_normal_input[4];
static int s_ratan_y;
static int s_ratan_x;

static void st8(u32 address, u8 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static void st16(u32 address, u16 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static void st32(u32 address, u32 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static u8 ld8(u32 address)
{
    u8 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static u16 ld16(u32 address)
{
    u16 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static u32 ld32(u32 address)
{
    u32 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static u32 guest_of(const void *pointer)
{
    uintptr_t offset = (uintptr_t)((const u8*)pointer - g_PsxRam);
    return 0x80000000u | ((u32)offset & 0x001FFFFFu);
}

static void check(const char *name, int condition)
{
    if (!condition) {
        fprintf(stderr, "ASSERTION %s\n", name);
        s_failures++;
    }
}

MATRIX *RotMatrixYXZ(SVECTOR *input, MATRIX *output)
{
    s_rot_calls++;
    s_rot_input = guest_of(input);
    s_rot_output = guest_of(output);
    memset(output, 0, sizeof(*output));
    return output;
}

VECTOR *ApplyMatrix(MATRIX *matrix, SVECTOR *input, VECTOR *output)
{
    int slot = s_apply_calls;
    (void)matrix;
    if (slot < 4) {
        s_apply_input[slot] = guest_of(input);
        s_apply_output[slot] = guest_of(output);
    }
    if (slot == 0) {
        output->vx = 10;
        output->vy = 20;
        output->vz = 30;
    } else {
        output->vx = 40;
        output->vy = 50;
        output->vz = 60;
    }
    output->pad = 0;
    s_apply_calls++;
    return output;
}

long VectorNormal(VECTOR *input, VECTOR *output)
{
    int slot = s_normal_calls;
    if (slot < 4)
        s_normal_input[slot] = *input;
    if (slot == 0) {
        output->vx = 100;
        output->vy = 200;
        output->vz = 300;
    } else {
        output->vx = 400;
        output->vy = 500;
        output->vz = 600;
    }
    output->pad = 0;
    s_normal_calls++;
    return 0;
}

int ratan2(int y, int x)
{
    s_ratan_calls++;
    s_ratan_y = y;
    s_ratan_x = x;
    return 0x321;
}

int rand(void)
{
    s_rand_calls++;
    return s_rand_calls;
}

void wm_80089580(void)
{
    s_tick_calls++;
}

static void reset_fixture(void)
{
    u32 source0 = SOURCE_TABLE;
    u32 source1 = SOURCE_TABLE + SOURCE_STRIDE;
    u32 source = SOURCE_TABLE + (SOURCE_COUNT - 1u) * SOURCE_STRIDE;
    u32 occupied = PARTICLE_POOL;

    memset(g_PsxRam, 0, sizeof(g_PsxRam));
    s_failures = 0;
    s_rot_calls = 0;
    s_apply_calls = 0;
    s_normal_calls = 0;
    s_ratan_calls = 0;
    s_rand_calls = 0;
    s_tick_calls = 0;
    memset(s_apply_input, 0, sizeof(s_apply_input));
    memset(s_apply_output, 0, sizeof(s_apply_output));
    memset(s_normal_input, 0, sizeof(s_normal_input));

    st32(BCC0, SOURCE_TABLE);
    st32(BDF4, PARTICLE_POOL);

    st8(source0 + 0x4Fu, 0x90u);
    st32(source0 + 4u, (5u << 16) | 2u);
    st8(source1 + 0x4Fu, 0x90u);
    st32(source1 + 4u, 0u);

    st8(source + 0x4Fu, 0xDFu);
    st32(source + 4u, 2u << 16);
    st16(source + 0x08u, 2u);
    st16(source + 0x0Au, 0u);
    st32(source + 0x0Cu, (7u << 16) | 3u);
    st16(source + 0x0Eu, 7u);
    st16(source + 0x10u, 9u);
    st16(source + 0x12u, 0u);
    st16(source + 0x14u, 1u);
    st16(source + 0x16u, 2u);
    st16(source + 0x18u, 3u);
    st32(source + 0x34u, 4096u);
    st16(source + 0x38u, 0xFFFFu);
    st16(source + 0x3Au, 2u);
    st16(source + 0x3Cu, 0xFFFDu);
    st16(source + 0x40u, 2u);
    st16(source + 0x42u, 3u);
    st32(source + 0x44u, 0x11112222u);
    st32(source + 0x48u, 0x33334444u);
    /* The flags byte at +0x4f is the high byte of this copied word. */
    st32(source + 0x4Cu, 0xDF667788u);
    st32(source + 0x50u, 0x99AABBCCu);

    /* Occupied high half, zero low half: correct free test skips record 0. */
    st32(occupied + 4u, 1u << 16);
}

static void run_certificate(void)
{
    u32 source0 = SOURCE_TABLE;
    u32 source1 = SOURCE_TABLE + SOURCE_STRIDE;
    u32 source = SOURCE_TABLE + (SOURCE_COUNT - 1u) * SOURCE_STRIDE;
    u32 particle = PARTICLE_POOL + PARTICLE_STRIDE;

    reset_fixture();
    wm_80089748();

    check("source-timer-stages-and-deactivation",
          ld32(source0 + 4u) == ((5u << 16) | 1u) &&
          ld8(source1 + 0x4Fu) == 0x10u &&
          ld32(source1 + 4u) == 0u);
    check("walks-512-sources-at-54-byte-stride",
          ld16(source + 0x0Au) == 1u &&
          ld32(source + 4u) == (1u << 16) &&
          ld16(source + 0x12u) == 9u);
    check("free-test-uses-packed-high-halfword",
          ld32(PARTICLE_POOL + 4u) == (1u << 16) &&
          ld16(particle + 0u) == 511u);
    check("retail-matrix-and-vector-addresses",
          s_rot_calls == 1 && s_rot_input == source + 0x1Cu &&
          s_rot_output == guest_of(PSX_ADDR(SCRATCH + 0xF0u)) &&
          s_apply_calls == 2 &&
          s_apply_input[0] == source + 0x24u &&
          s_apply_input[1] == source + 0x2Cu &&
          s_apply_output[0] == guest_of(PSX_ADDR(SCRATCH + 0x20u)) &&
          s_apply_output[1] == guest_of(PSX_ADDR(SCRATCH + 0x20u)));
    check("retail-random-order-and-normal-inputs",
          s_rand_calls == 5 && s_normal_calls == 3 &&
          s_normal_input[0].vx == -2046 &&
          s_normal_input[0].vy == -2047 &&
          s_normal_input[0].vz == -2045 &&
          s_normal_input[1].vx == -2044 &&
          s_normal_input[1].vy == 0 &&
          s_normal_input[1].vz == -2043 &&
          s_normal_input[2].vx == 30 &&
          s_normal_input[2].vy == 30 &&
          s_normal_input[2].vz == 30);
    check("spawned-first-and-second-vectors",
          ld32(particle + 0x08u) == 45256u &&
          ld32(particle + 0x0Cu) == 90512u &&
          ld32(particle + 0x10u) == 135768u &&
          ld32(particle + 0x18u) == 400u &&
          ld32(particle + 0x1Cu) == 500u &&
          ld32(particle + 0x20u) == 600u);
    check("spawned-heading-and-signed-fields",
          s_ratan_calls == 1 && s_ratan_y == 500 && s_ratan_x == 400 &&
          ld16(particle + 0x02u) == 0x0321u &&
          ld32(particle + 0x28u) == 0xFFFFFFFFu &&
          ld32(particle + 0x2Cu) == 2u &&
          ld32(particle + 0x30u) == 0xFFFFFFFDu);
    check("spawned-payload-and-flags",
          ld32(particle + 0x04u) == ((7u << 16) | 3u) &&
          ld32(particle + 0x38u) == 0x11112222u &&
          ld32(particle + 0x3Cu) == 0x33334444u &&
          ld32(particle + 0x40u) == 0xDF667788u &&
          ld32(particle + 0x44u) == 0x99AABBCCu &&
          ld16(particle + 0x48u) == 0x00FDu);
    check("particle-tick-runs-once-after-source-walk", s_tick_calls == 1);
}

int main(void)
{
    run_certificate();
    if (s_failures != 0)
        return EXIT_FAILURE;
    puts("W34N39 0x80089748 full-body certificate PASS");
    return EXIT_SUCCESS;
}
