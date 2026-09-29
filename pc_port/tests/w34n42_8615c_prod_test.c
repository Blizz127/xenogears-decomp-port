/* W34N42 — production-linked certificate for retail wm_8008615C. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "psyq/libgte.h"
#include "world_map_helper_8615c.h"

#define SCRATCH  0x1F800000u
#define MODEL    0x8009A180u
#define CAMERA   0x8009C808u
#define TILE_REC 0x8009C7ECu
#define MAP_X    0x8009C838u
#define MAP_Z    0x8009C83Cu
#define CLUTS    0x8009D478u
#define TILE_MAP 0x8009D570u
#define VALID    0x8009D618u
#define ROOTS    0x8009D7E8u
#define BUFFER   0x8009D7F0u
#define HEADING  0x8009BD3Cu
#define OUTCOUNT 0x8009BE04u
#define DRAW_G   0x8009BE3Cu
#define TABLE    0x80100000u
#define DRAW     0x80110000u
#define OT       0x80120000u
#define PACK0    0x80130000u
#define PACK1    0x80140000u

uint8_t g_PsxRam[PSX_RAM_SIZE];
static int s_failures;
static int s_rot_calls;
static int s_rot_angle;
static u32 s_rot_output;
static u8 s_model_at_rot[sizeof(MATRIX)];
static int s_submit_calls;
static u32 s_entries[32];
static s32 s_counts[32];
static u32 s_ots[32];
static u32 s_packets[32];

static void st16(u32 a, u16 v) { memcpy(PSX_ADDR(a), &v, 2); }
static void st32(u32 a, u32 v) { memcpy(PSX_ADDR(a), &v, 4); }
static u16 ld16(u32 a) { u16 v; memcpy(&v, PSX_ADDR(a), 2); return v; }
static u32 guest_of(const void *p) { return PsxMemory_GuestAddr(p); }

static void check(const char *name, int ok)
{
    if (!ok) {
        fprintf(stderr, "ASSERTION %s\n", name);
        s_failures++;
    }
}

MATRIX *RotMatrixZ(int angle, MATRIX *output)
{
    s_rot_calls++;
    s_rot_angle = angle;
    s_rot_output = guest_of(output);
    memcpy(s_model_at_rot, output, sizeof(s_model_at_rot));
    return output;
}

void wm_80099BFC(u32 entries, s32 count, u32 ot, u32 packet)
{
    u16 current;
    if (s_submit_calls < 32) {
        s_entries[s_submit_calls] = entries;
        s_counts[s_submit_calls] = count;
        s_ots[s_submit_calls] = ot;
        s_packets[s_submit_calls] = packet;
    }
    current = ld16(OUTCOUNT);
    st16(OUTCOUNT, (u16)(current + 1u));
    s_submit_calls++;
}

static void reset_fixture(void)
{
    u32 i;
    memset(g_PsxRam, 0, sizeof(g_PsxRam));
    s_rot_calls = 0;
    s_submit_calls = 0;
    memset(s_entries, 0, sizeof(s_entries));
    for (i = 0; i < sizeof(MATRIX); i++) {
        ((u8*)PSX_ADDR(CAMERA))[i] = (u8)(0x40u + i);
        ((u8*)PSX_ADDR(MODEL))[i] = (u8)(0x80u + i);
    }
    st16(HEADING, 0x0123u);
    for (i = 0; i < 16u; i++)
        st16(CLUTS + i * 2u, (u16)(0x600u + i));
    for (i = 0; i < 25u; i++)
        st16(VALID + i * 2u, 0xFFFFu);
    st16(VALID + 0u * 2u, 0u);
    st16(VALID + 12u * 2u, 0u);
    st16(VALID + 24u * 2u, 0u);
    st16(MAP_X, 2u);
    st16(MAP_Z, 2u);
    for (i = 0; i < 81u; i++)
        st16(TILE_MAP + i * 2u, 6u);
    st16(TILE_MAP + 20u * 2u, 3u);
    st16(TILE_MAP + 40u * 2u, 4u);
    st16(TILE_MAP + 60u * 2u, 5u);
    st32(TILE_REC, TABLE);
    st32(TABLE + 3u * 8u, 0x80150000u);
    st32(TABLE + 3u * 8u + 4u, 2u);
    st32(TABLE + 4u * 8u, 0x80151000u);
    st32(TABLE + 4u * 8u + 4u, 0u);
    st32(TABLE + 5u * 8u, 0x80152000u);
    st32(TABLE + 5u * 8u + 4u, 1u);
    st32(TABLE + 6u * 8u, 0x80153000u);
    st32(TABLE + 6u * 8u + 4u, 1u);
    st32(ROOTS + 0u, PACK0);
    st32(ROOTS + 4u, PACK1);
    st32(BUFFER, 1u);
    st32(DRAW_G, DRAW);
    st32(DRAW + 0x70u, OT);
    st32(OUTCOUNT, 0xDEAD0007u);
}

int main(void)
{
    u32 i;
    u32 scratch_guest;
    u8 camera_expected[sizeof(MATRIX)];
    u8 model_expected[sizeof(MATRIX)];

    s_failures = 0;
    reset_fixture();
    scratch_guest = guest_of(PSX_ADDR(SCRATCH));
    memcpy(camera_expected, PSX_ADDR(CAMERA), sizeof(camera_expected));
    memcpy(model_expected, PSX_ADDR(MODEL), sizeof(model_expected));
    wm_8008615C();

    check("retail-four-shared-vertices",
          *(s16*)PSX_ADDR(SCRATCH + 0x00u) == -24 &&
          *(s16*)PSX_ADDR(SCRATCH + 0x02u) == -72 &&
          *(s16*)PSX_ADDR(SCRATCH + 0x08u) == 24 &&
          *(s16*)PSX_ADDR(SCRATCH + 0x0Au) == -72 &&
          *(s16*)PSX_ADDR(SCRATCH + 0x10u) == -24 &&
          *(s16*)PSX_ADDR(SCRATCH + 0x18u) == 24);
    check("matrix-copy-and-negative-heading",
          s_rot_calls == 1 && s_rot_angle == -0x123 &&
          s_rot_output == scratch_guest + 0x48u &&
          memcmp(PSX_ADDR(SCRATCH + 0x28u), camera_expected,
                 sizeof(camera_expected)) == 0 &&
          memcmp(s_model_at_rot, model_expected, sizeof(model_expected)) == 0);
    for (i = 0u; i < 16u; i++)
        check("retail-sixteen-clut-copy",
              ld16(SCRATCH + 0x68u + i * 2u) == (u16)(0x600u + i));
    check("retail-five-by-five-validity-and-map-index",
          s_submit_calls == 2 && s_entries[0] == 0x80150000u &&
          s_entries[1] == 0x80152000u &&
          s_counts[0] == 2 && s_counts[1] == 1);
    check("active-ot-and-interleaved-packet-root",
          s_ots[0] == OT && s_ots[1] == OT &&
          s_packets[0] == PACK1 && s_packets[1] == PACK1 + 0x28u);
    check("output-count-cleared-before-dispatch", ld16(OUTCOUNT) == 2u);

    if (s_failures == 0)
        puts("W34N42 0x8008615C full-body certificate PASS");
    return s_failures == 0 ? 0 : 1;
}
