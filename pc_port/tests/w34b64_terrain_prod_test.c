/* Focused production-linked certificate for the retail terrain chain. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"

static int failures;

static void check(int condition, const char *name)
{
    if (!condition) {
        fprintf(stderr, "ASSERTION %s\n", name);
        failures++;
    }
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

static void write16(u32 address, u16 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static void write32(u32 address, u32 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

#if defined(W34B64_SUBMIT_TEST) || defined(W34B64_DISPATCH_TEST)
static u32 guest_of(const void *pointer)
{
    uintptr_t base = (uintptr_t)g_PsxRam;
    uintptr_t value = (uintptr_t)pointer;
    uintptr_t offset = value - base;

    if (value < base || offset >= PSX_RAM_SIZE)
        return UINT32_C(0xFFFFFFFF);
    if (offset < 0x400u)
        return UINT32_C(0x1F800000) + (u32)offset;
    return UINT32_C(0x80000000) | (u32)offset;
}
#endif

#if defined(W34B64_GRID_TEST)
#include "world_map_helper_99708.h"

static int submit_calls;
static u32 submit_args[3];

void wm_8009980C(u32 tile_data, u32 ot_base, u32 packet_base)
{
    submit_calls++;
    submit_args[0] = tile_data;
    submit_args[1] = ot_base;
    submit_args[2] = packet_base;
}

static s16 expected_y(u32 packed, s16 sine_x, s16 sine_z)
{
    /* Retail 0x80099790-94 / 0x800997AC-B0: low byte, sign-extended, x8. */
    s32 signed_height = (s32)(s8)(packed & 0xFFu) * 8;

    if ((packed & 0x1000u) != 0u) {
        int64_t product = (int64_t)sine_z * (int64_t)((s32)sine_x * 2);
        signed_height += (s32)(product >> 20);
    }
    return (s16)signed_height;
}

static void run_grid_test(void)
{
    const u32 tile = 0x80010000u;
    const u32 origin = 0x80018000u;
    const u32 ot = 0x80020000u;
    const u32 packets = 0x80024000u;
    unsigned char source_before[81u * 4u];
    u32 row;

    PsxMemory_Init();
    write16(origin, (u16)(s16)-321);
    write16(origin + 4u, (u16)(s16)777);
    write32(0x8009C618u, 0x137u);
    write32(0x8009C5BCu, 0x2A5u);
    for (row = 0u; row < 0x1000u; row++)
        write16(0x800523F0u + row * 4u, (u16)(s16)((s32)row - 1700));
    for (row = 0u; row < 81u; row++) {
        u32 packed = ((u32)(u8)(row * 37u + 0x83u) << 24) |
                     ((row & 1u) != 0u ? 0x1000u : 0u) |
                     (row * 3u + 1u);
        write32(tile + row * 4u, packed);
    }
    memcpy(source_before, PSX_ADDR(tile), sizeof(source_before));

    wm_80099708(tile, ot, packets, origin);

    check(submit_calls == 1, "grid calls submitter once");
    check(submit_args[0] == tile && submit_args[1] == ot &&
              submit_args[2] == packets,
          "grid preserves submit arguments");
    check(memcmp(source_before, PSX_ADDR(tile), sizeof(source_before)) == 0,
          "grid source is read-only");
    for (row = 0u; row < 9u; row++) {
        u32 column;
        for (column = 0u; column < 9u; column++) {
            u32 index = row * 9u + column;
            u32 packed = read32(tile + index * 4u);
            u32 vertex = 0x1F800000u + index * 8u;
            s16 sine_x = (s16)read16(
                0x800523F0u + ((0x137u + row * 0x200u) & 0xFFFu) * 4u);
            s16 sine_z = (s16)read16(
                0x800523F0u + ((0x2A5u + column * 0x200u) & 0xFFFu) * 4u);

            check((s16)read16(vertex) == (s16)(-321 + (s32)column * 0x80),
                  "grid X resets and advances by column");
            check((s16)read16(vertex + 2u) == expected_y(packed, sine_x, sine_z),
                  "grid Y follows packed height contract");
            check((s16)read16(vertex + 4u) == (s16)(777 - (s32)row * 0x80),
                  "grid Z advances by row");
            check(read16(vertex + 6u) == 0u, "grid vertex pad remains untouched");
        }
    }
}

int main(void)
{
    run_grid_test();
    if (failures != 0)
        return EXIT_FAILURE;
    puts("W34B64 terrain grid certificate PASS");
    return EXIT_SUCCESS;
}

#elif defined(W34B64_DISPATCH_TEST)
#include "psyq/libgte.h"
#include "world_map_helper_9932c.h"

static int dispatch_calls;
static u32 dispatch_args[4][4];
static s16 dispatch_origins[4][2];
static u32 comp_args[3];
static u32 rot_arg;
static u32 trans_arg;

MATRIX *CompMatrix(MATRIX *left, MATRIX *right, MATRIX *output)
{
    comp_args[0] = guest_of(left);
    comp_args[1] = guest_of(right);
    comp_args[2] = guest_of(output);
    memcpy(output, right, sizeof(*output));
    return output;
}

void SetRotMatrix(MATRIX *matrix)
{
    rot_arg = guest_of(matrix);
}

void SetTransMatrix(MATRIX *matrix)
{
    trans_arg = guest_of(matrix);
}

void wm_80099708(u32 tile_data, u32 ot_base, u32 packet_base, u32 origin)
{
    u32 count;

    if (dispatch_calls < 4) {
        dispatch_args[dispatch_calls][0] = tile_data;
        dispatch_args[dispatch_calls][1] = ot_base;
        dispatch_args[dispatch_calls][2] = packet_base;
        dispatch_args[dispatch_calls][3] = origin;
        dispatch_origins[dispatch_calls][0] = (s16)read16(origin);
        dispatch_origins[dispatch_calls][1] = (s16)read16(origin + 4u);
    }
    count = read32(0x8009D7DCu);
    write32(0x8009D7DCu, count + 3u);
    dispatch_calls++;
}

static void run_dispatch_test(void)
{
    const u32 ot = 0x80020000u;
    const u32 packets = 0x80030000u;
    const u32 position = 0x80040000u;
    const u32 tile_base = 0x80050000u;
    unsigned char matrix_before[32];
    u32 i;

    PsxMemory_Init();
    for (i = 0u; i < 64u; i++)
        write16(0x8009CCB4u + i * 2u, (u16)(0x6100u + i * 3u));
    for (i = 0u; i < 7u; i++)
        write16(0x8009CD54u + i * 2u, (u16)(0x7200u + i * 5u));
    for (i = 0u; i < 32u; i++)
        ((u8 *)PSX_ADDR(0x8009D534u))[i] = (u8)(0x80u + i);
    memcpy(matrix_before, PSX_ADDR(0x8009D534u), sizeof(matrix_before));
    write32(position, 0x12345000u);
    write32(position + 8u, 0x0FEDC000u);
    write16(0x8009C838u, 0u);
    write16(0x8009C83Cu, 0u);
    for (i = 0u; i < 25u; i++)
        write16(0x8009D618u + i * 2u, UINT16_C(0xFFFF));
    write16(0x8009D618u, 0u);
    write16(0x8009D570u, 2u);
    write32(0x8009C184u + 8u, tile_base);

    wm_8009932C(ot, packets, position);

    check(dispatch_calls == 4, "dispatcher submits four enabled quadrants");
    for (i = 0u; i < 4u; i++) {
        check(dispatch_args[i][0] == tile_base + i * 0x144u,
              "dispatcher terrain quadrant offset");
        check(dispatch_args[i][1] == ot, "dispatcher preserves OT base");
        check(dispatch_args[i][2] == packets + i * 3u * 0x20u,
              "dispatcher uses live compacted packet count");
        check(dispatch_args[i][3] == 0x1F800328u + i * 8u,
              "dispatcher quadrant origin layout");
    }
    check(dispatch_origins[0][0] == (s16)-0x1345 &&
              dispatch_origins[0][1] == (s16)0x16DC &&
              dispatch_origins[1][0] == (s16)-0x0F45 &&
              dispatch_origins[1][1] == (s16)0x16DC &&
              dispatch_origins[2][0] == (s16)-0x1345 &&
              dispatch_origins[2][1] == (s16)0x12DC &&
              dispatch_origins[3][0] == (s16)-0x0F45 &&
              dispatch_origins[3][1] == (s16)0x12DC,
          "dispatcher passes exact quadrant coordinates");
    check(comp_args[0] == 0x8009C808u && comp_args[1] == 0x1F800350u &&
              comp_args[2] == 0x1F800370u,
          "dispatcher composes retail camera and terrain matrix");
    check(rot_arg == 0x1F800370u && trans_arg == 0x1F800370u,
          "dispatcher publishes completed GTE matrix");
    check(memcmp(matrix_before, PSX_ADDR(0x1F800350u), 32u) == 0,
          "dispatcher copies terrain matrix into scratch");
    check(read16(0x1F800288u) == 0x6100u &&
              read16(0x1F800306u) == (u16)(0x6100u + 63u * 3u),
          "dispatcher copies 64-entry shade table");
    check(read16(0x1F800308u) == 0x7200u &&
              read16(0x1F800314u) == (u16)(0x7200u + 6u * 5u),
          "dispatcher copies seven-entry color table");
    check(read32(0x8009D7DCu) == 12u,
          "dispatcher initializes and preserves live packet count");
}

int main(void)
{
    run_dispatch_test();
    if (failures != 0)
        return EXIT_FAILURE;
    puts("W34B64 terrain dispatcher certificate PASS");
    return EXIT_SUCCESS;
}

#elif defined(W34B64_SUBMIT_TEST)
#include <psx/gtereg.h>
#include "psyq/libgte.h"
#include "world_map_helper_9980c.h"

GTERegisters gteRegs;
static int projection_calls;
static int nclip_calls;
static int projection_mode;
static u32 projected_vertices[4][3];

int RotTransPers3(SVECTOR *v0, SVECTOR *v1, SVECTOR *v2,
                  long *xy0, long *xy1, long *xy2, long *p, long *flag)
{
    if (projection_calls < 4) {
        projected_vertices[projection_calls][0] = guest_of(v0);
        projected_vertices[projection_calls][1] = guest_of(v1);
        projected_vertices[projection_calls][2] = guest_of(v2);
    }
    *xy0 = (long)(0x00320028u + (u32)projection_calls);
    *xy1 = (long)(0x00460050u + (u32)projection_calls);
    *xy2 = (long)(0x005A0078u + (u32)projection_calls);
    *p = 0;
    *flag = projection_mode == 1 ? (long)(s32)0x80000000u : 0;
    C2_SZ1 = 0x0120u;
    C2_SZ2 = 0x0130u;
    C2_SZ3 = projection_mode == 3 ? 0x0F00u : 0x0140u;
    C2_IR0 = (s16)(projection_mode == 4 ? -7 : 0x0345);
    projection_calls++;
    return 0x0140;
}

int NormalClip(int xy0, int xy1, int xy2)
{
    (void)xy0;
    (void)xy1;
    (void)xy2;
    nclip_calls++;
    return projection_mode == 2 ? 0 : 17;
}

static void initialize_submit_data(u32 tile, u32 ot, u32 packets)
{
    u32 i;

    memset(g_PsxRam, 0, PSX_RAM_SIZE);
    memset(&gteRegs, 0, sizeof(gteRegs));
    projection_calls = 0;
    nclip_calls = 0;
    for (i = 0u; i < 72u; i++)
        write32(tile + i * 4u, 0x12005601u + i);
    write32(tile, 0x12A65601u);
    write32(tile + 4u, 0x34B7D203u);
    for (i = 0u; i < 64u; i++)
        write16(0x1F800288u + i * 2u, (u16)(0x4100u + i));
    for (i = 0u; i < 7u; i++)
        write16(0x1F800308u + i * 2u, (u16)(0x5200u + i));
    for (i = 0u; i < 0xF0u; i++)
        write32(ot + i * 4u, 0x000ABCDEu + i);
    memset(PSX_ADDR(packets), 0xA5, 128u * 0x20u + 64u);
    write32(0x8009D7DCu, 0u);
}

static void run_submit_test(void)
{
    const u32 tile = 0x80010000u;
    const u32 ot = 0x80020000u;
    const u32 packets = 0x80030000u;
    u32 bucket = ot + 0x14u * 4u;

    PsxMemory_Init();
    projection_mode = 0;
    initialize_submit_data(tile, ot, packets);
    wm_8009980C(tile, ot, packets);

    check(read32(0x8009D7DCu) == 128u, "submit emits two triangles per cell");
    check(projection_calls == 128 && nclip_calls == 128,
          "submit projects and clips every triangle");
    /* Retail 0x800998D8-0x8009990C: V0 = vertex, V1 = vertex+8 (bit 15 clear),
     * V2 = vertex+0x48 (lwc2 $4/$5). */
    check(projected_vertices[0][0] == 0x1F800000u &&
              projected_vertices[0][1] == 0x1F800008u &&
              projected_vertices[0][2] == 0x1F800048u,
          "triangle A vertex order (bit 15 clear)");
    check(projected_vertices[1][0] == 0x1F800008u &&
              projected_vertices[1][1] == 0x1F800050u &&
              projected_vertices[1][2] == 0x1F800048u,
          "clear orientation triangle two order");
    /* Retail 0x800998F0: bit 15 set -> V1 = vertex+0x50, V2 = vertex+0x48. */
    check(projected_vertices[2][0] == 0x1F800008u &&
              projected_vertices[2][1] == 0x1F800058u &&
              projected_vertices[2][2] == 0x1F800050u,
          "triangle A vertex order (bit 15 set)");
    check(projected_vertices[3][0] == 0x1F800010u &&
              projected_vertices[3][1] == 0x1F800058u &&
              projected_vertices[3][2] == 0x1F800008u,
          "set orientation triangle two order");
    check((read32(packets) >> 24) == 7u, "packet tag length is seven");
    check((read32(packets) & 0x00FFFFFFu) ==
              ((0x000ABCDEu + 0x14u) & 0x00FFFFFFu),
          "packet retains previous OT link");
    check(read32(bucket) == ((packets + 127u * 0x20u) & 0x00FFFFFFu),
          "OT points at final compacted packet");
    check(read32(packets + 8u) == 0x00320028u &&
              read32(packets + 0x10u) == 0x00460050u &&
              read32(packets + 0x18u) == 0x005A0078u,
          "packet receives projected XY words");
    check(read16(packets + 0x1Cu) == 0xA060u,
          "packet receives retail third color");

    projection_mode = 0;
    initialize_submit_data(tile, ot, packets);
    write32(0x8009D7DCu, 0x7FDu);
    wm_8009980C(tile, ot, packets);
    check(read32(0x8009D7DCu) == 0x7FFu && projection_calls == 2,
          "packet ceiling is checked at each cell");

    projection_mode = 1;
    initialize_submit_data(tile, ot, packets);
    wm_8009980C(tile, ot, packets);
    check(read32(0x8009D7DCu) == 0u && nclip_calls == 0,
          "negative GTE FLAG rejects before NCLIP");

    projection_mode = 2;
    initialize_submit_data(tile, ot, packets);
    wm_8009980C(tile, ot, packets);
    check(read32(0x8009D7DCu) == 0u && nclip_calls == 128,
          "nonpositive NCLIP rejects triangles");

    projection_mode = 3;
    initialize_submit_data(tile, ot, packets);
    wm_8009980C(tile, ot, packets);
    check(read32(0x8009D7DCu) == 0u && nclip_calls == 0,
          "depth ceiling rejects before NCLIP");

    projection_mode = 4;
    initialize_submit_data(tile, ot, packets);
    wm_8009980C(tile, ot, packets);
    check((read32(packets + 0x0Cu) >> 16) == read16(0x1F8002C6u),
          "negative IR0 clamps to 0x0FFF for shade lookup");
}

int main(void)
{
    run_submit_test();
    if (failures != 0)
        return EXIT_FAILURE;
    puts("W34B64 terrain submitter certificate PASS");
    return EXIT_SUCCESS;
}

#else
#error "select a W34B64 terrain certificate component"
#endif
