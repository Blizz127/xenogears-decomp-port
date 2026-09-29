/*
 * W34-R4WORLD-B — production-linked memory-state certificate for 0x80073B04.
 *
 * The oracle is separate from the production TU: it independently performs
 * the accepted GTE sequence into local expected XY values, then derives the
 * complete packet/tag/OT post-image from those values.  It compares every
 * byte of the 160-byte packet array, both DR_TWIN carriers, and the touched
 * OT word for asymmetric heading/camera/depth fixtures.
 */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_helper_73b04.h"
#include <libgte.h>

s32 D_80050100 = 2;

#if defined(WM_73B04_TRACE)
static u32 trace_events[32];
static u32 trace_count;
void wm_73b04_test_trace(u32 event, u32 address)
{
    if (trace_count < 32u)
        trace_events[trace_count++] = (event << 28) | (address & 0x0FFFFFFFu);
}
#endif

#define ANGLE_ADDR  0x8009BD3Au
#define INDEX_ADDR  0x8009D7F0u
#define DB_PTR_ADDR 0x8009BE3Cu
#define DB_ADDR     0x8009BBC8u
#define OT_BASE     0x800F0000u
#define CAMERA_ADDR 0x8009C808u
#define PACKET_BASE 0x8009C744u
#define TWIN_A      0x8009D3D8u
#define TWIN_B      0x8009D3E4u
#define VERTEX_BASE 0x8009A300u
#define SHIFT_ADDR  0x80050100u

static u32 load_u32(u32 address)
{
    u32 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static void store_u16(u32 address, u16 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static void store_u32(u32 address, u32 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static s32 bits_as_s32(u32 value)
{
    s32 result;
    memcpy(&result, &value, sizeof(result));
    return result;
}

static s32 oracle_srav(s32 value, s32 shift)
{
    u32 amount = (u32)shift & 31u;
    u32 bits;
    u32 result;

    memcpy(&bits, &value, sizeof(bits));
    if (amount == 0u)
        return value;
    result = bits >> amount;
    if (value < 0)
        result |= UINT32_MAX << (32u - amount);
    return bits_as_s32(result);
}

static void seed_camera(const u16 heading, s32 depth)
{
    MATRIX camera;
    SVECTOR camera_angles;

    memset(&camera, 0, sizeof(camera));
    camera_angles.vx = (s16)0x0180;
    camera_angles.vy = (s16)0x02C0;
    camera_angles.vz = (s16)0x00A0;
    RotMatrixYXZ(&camera_angles, &camera);
    camera.t[0] = 192;
    camera.t[1] = -128;
    camera.t[2] = depth;
    (void)heading;
    memcpy(PSX_ADDR(CAMERA_ADDR), &camera, sizeof(camera));
}

static void seed_templates(void)
{
    u8* packets = (u8*)PSX_ADDR(PACKET_BASE);
    u8* twin_a = (u8*)PSX_ADDR(TWIN_A);
    u8* twin_b = (u8*)PSX_ADDR(TWIN_B);
    u32 i;

    memset(packets, 0xA7, 160u);
    for (i = 0u; i < 4u; i++) {
        u8* p = packets + i * 40u;
        store_u32(PACKET_BASE + i * 40u, 0x09000000u);
        p[4] = 0x30;
        p[5] = 0x30;
        p[6] = 0x30;
        p[7] = 0x2E;
        store_u16(PACKET_BASE + i * 40u + 0x0Eu, 0x7F91u);
        store_u16(PACKET_BASE + i * 40u + 0x16u, 0x003Eu);
    }
    memset(twin_a, 0xC1, 12u);
    memset(twin_b, 0xD2, 12u);
    store_u32(TWIN_A, 0x020000AAu);
    store_u32(TWIN_B, 0x020000BBu);
    store_u32(TWIN_A + 4u, 0xE2000010u);
    store_u32(TWIN_B + 4u, 0xE2000000u);
    store_u32(TWIN_A + 8u, 0u);
    store_u32(TWIN_B + 8u, 0u);
}

static void seed_vertices(void)
{
    static const s16 values[8][4] = {
        { -4096, -896, 4032, 0 }, { 0, -896, 4032, 0 },
        { -4096, -640, 4032, 0 }, { 0, -640, 4032, 0 },
        { 0, -896, 4032, 0 }, { 4096, -896, 4032, 0 },
        { 0, -640, 4032, 0 }, { 4096, -640, 4032, 0 },
    };
    memcpy(PSX_ADDR(VERTEX_BASE), values, sizeof(values));
}

static void seed_case(u16 theta, u32 index, s32 shift, s32 depth)
{
    u32 i;

    PsxMemory_Init();
    InitGeom();
    SetGeomScreen(320);
    SetGeomOffset(160, 120);
    seed_templates();
    seed_vertices();
    seed_camera(theta, depth);
    store_u16(ANGLE_ADDR, theta);
    store_u32(INDEX_ADDR, index);
    store_u32(DB_PTR_ADDR, DB_ADDR);
    store_u32(DB_ADDR + 0x70u, OT_BASE);
    store_u32(SHIFT_ADDR, (u32)shift);
    D_80050100 = shift; /* W34C2: host authority for the OT shift */
    for (i = 0u; i < 0x400u; i++)
        store_u32(OT_BASE + i * 4u, 0xA5000000u | (0x1000u + i));

    /* M3 must expose the missing R.t zeroing rather than inherit zero BSS. */
    store_u32(0x1F80004Cu, 0x11111111u);
    store_u32(0x1F800050u, 0x22222222u);
    store_u32(0x1F800054u, 0x33333333u);
}

static void expected_uv(u8* packet, u16 theta)
{
    u16 value = (u16)((theta >> 2) & 0x7Fu);
    u16 uv[4] = { value, (u16)(value | 0x80u),
                  (u16)(value | 0x3F00u),
                  (u16)(value | 0x3F80u) };
    u32 offsets[4] = { 0x0Cu, 0x14u, 0x1Cu, 0x24u };
    u32 i;

    for (i = 0u; i < 4u; i++)
        memcpy(packet + offsets[i], &uv[i], sizeof(uv[i]));
}

static void expected_push(u8* packets, u8* twins, u32 ot_address,
                          u32 packet_address, u32 packet_offset,
                          u32* old_ot)
{
    u32 tag;
    u32 new_tag;
    u32 new_ot;
    u32 twin_offset = packet_address == TWIN_A ? 0u : 12u;

    if (packet_address == TWIN_A || packet_address == TWIN_B) {
        memcpy(&tag, twins + twin_offset, sizeof(tag));
        new_tag = (tag & 0xFF000000u) | (*old_ot & 0x00FFFFFFu);
        memcpy(twins + twin_offset, &new_tag, sizeof(new_tag));
    } else {
        memcpy(&tag, packets + packet_offset, sizeof(tag));
        new_tag = (tag & 0xFF000000u) | (*old_ot & 0x00FFFFFFu);
        memcpy(packets + packet_offset, &new_tag, sizeof(new_tag));
    }
    new_ot = (*old_ot & 0xFF000000u) |
             (packet_address & 0x00FFFFFFu);
    *old_ot = new_ot;
    (void)ot_address;
}

static int oracle_case(u16 theta, u32 index, s32 shift, s32 depth,
                       u8* expected_packets, u8* expected_twins,
                       u32* expected_ot_address, u32* expected_ot_word)
{
    MATRIX camera;
    union {
        u8 bytes[0x80];
        u32 align;
    } scratch;
    MATRIX* rotation = (MATRIX*)(scratch.bytes + 0x38u);
    MATRIX* composite = (MATRIX*)(scratch.bytes + 0x18u);
    SVECTOR* angles = (SVECTOR*)scratch.bytes;
    s32 otz = 0;
    long p = 0;
    long flag = 0;
    u32 packet_addresses[2] = {
        PACKET_BASE + index * 0x28u,
        PACKET_BASE + 0x50u + index * 0x28u
    };
    u32 packet_offsets[2] = { index * 0x28u, 0x50u + index * 0x28u };
    u32 q;

    memcpy(expected_packets, PSX_ADDR(PACKET_BASE), 160u);
    memcpy(expected_twins, PSX_ADDR(TWIN_A), 12u);
    memcpy(expected_twins + 12u, PSX_ADDR(TWIN_B), 12u);
    expected_uv(expected_packets + packet_offsets[0], theta);
    expected_uv(expected_packets + packet_offsets[1], theta);

    memcpy(&camera, PSX_ADDR(CAMERA_ADDR), sizeof(camera));
    memset(&scratch, 0, sizeof(scratch));
    angles->vx = 0;
    angles->vy = (s16)theta;
    angles->vz = 0;
    angles->pad = 0;
    (void)RotMatrixYXZ(angles, rotation);
    rotation->t[0] = 0;
    rotation->t[1] = 0;
    rotation->t[2] = 0;
    (void)CompMatrix(&camera, rotation, composite);
    SetRotMatrix(composite);
    SetTransMatrix(composite);

    for (q = 0u; q < 2u; q++) {
        u32 vertex = VERTEX_BASE + q * 0x20u;
        u32 xy[4];
        long local_xy[4] = { 0, 0, 0, 0 };
        SVECTOR* v0 = (SVECTOR*)PSX_ADDR(vertex);
        SVECTOR* v1 = (SVECTOR*)PSX_ADDR(vertex + 8u);
        SVECTOR* v2 = (SVECTOR*)PSX_ADDR(vertex + 16u);
        SVECTOR* v3 = (SVECTOR*)PSX_ADDR(vertex + 24u);

        otz = (s32)RotTransPers4(v0, v1, v2, v3,
                                 &local_xy[0], &local_xy[1],
                                 &local_xy[2], &local_xy[3],
                                 &p, &flag);
        for (u32 i = 0u; i < 4u; i++)
            memcpy(&xy[i], (u8*)local_xy + i * sizeof(long), sizeof(xy[i]));
        memcpy(expected_packets + packet_offsets[q] + 8u, &xy[0], 4u);
        memcpy(expected_packets + packet_offsets[q] + 16u, &xy[1], 4u);
        memcpy(expected_packets + packet_offsets[q] + 24u, &xy[2], 4u);
        memcpy(expected_packets + packet_offsets[q] + 32u, &xy[3], 4u);
    }

    if ((u32)flag & UINT32_C(0x80000000))
        return 0;

    {
        s32 bucket = oracle_srav(otz, shift);
        u32 ot_address = OT_BASE + ((u32)bucket << 2);
        u32 old_ot = load_u32(ot_address);
        *expected_ot_address = ot_address;
        expected_push(expected_packets, expected_twins, ot_address,
                      TWIN_B, 0u, &old_ot);
        expected_push(expected_packets, expected_twins, ot_address,
                      packet_addresses[0], packet_offsets[0], &old_ot);
        expected_push(expected_packets, expected_twins, ot_address,
                      packet_addresses[1], packet_offsets[1], &old_ot);
        expected_push(expected_packets, expected_twins, ot_address,
                      TWIN_A, 0u, &old_ot);
        *expected_ot_word = old_ot;
    }
    (void)depth;
    return 1;
}

static int compare_case(const char* name, u8* expected_packets,
                        u8* expected_twins, u32 ot_address,
                        u32 expected_ot)
{
    u8* actual_packets = (u8*)PSX_ADDR(PACKET_BASE);
    u8 actual_twins[24];
    u32 actual_ot = load_u32(ot_address);
    int ok = 1;

    memcpy(actual_twins, PSX_ADDR(TWIN_A), 12u);
    memcpy(actual_twins + 12u, PSX_ADDR(TWIN_B), 12u);
    if (memcmp(actual_packets, expected_packets, 160u) != 0) {
        fprintf(stderr, "ASSERTION packet_bytes[%s]\n", name);
        ok = 0;
    }
    if (memcmp(actual_twins, expected_twins, 24u) != 0) {
        fprintf(stderr, "ASSERTION dr_twin_bytes[%s]\n", name);
        ok = 0;
    }
    if (actual_ot != expected_ot) {
        fprintf(stderr, "ASSERTION ot_head[%s] got=0x%08X expected=0x%08X\n",
                name, actual_ot, expected_ot);
        ok = 0;
    }
    if (!ok) {
        u32 i;
        for (i = 0u; i < 0x400u; i++) {
            u32 value = load_u32(OT_BASE + i * 4u);
            if (value != (0xA5000000u | (0x1000u + i))) {
                fprintf(stderr, "DIAG %s expected_ot_addr=0x%08X changed_ot=0x%08X value=0x%08X\n",
                        name, ot_address, OT_BASE + i * 4u, value);
                break;
            }
        }
        fprintf(stderr,
                "DIAG %s flag=0x%08X packet0tag=0x%08X twinA=0x%08X twinB=0x%08X\n",
                name, load_u32(0x1F80005Cu), load_u32(PACKET_BASE),
                load_u32(TWIN_A), load_u32(TWIN_B));
        fprintf(stderr,
                "DIAG %s xy_actual=%08X/%08X/%08X/%08X xy_expect=%02X%02X%02X%02X/%02X%02X%02X%02X\n",
                name, load_u32(PACKET_BASE + 8u), load_u32(PACKET_BASE + 16u),
                load_u32(PACKET_BASE + 24u), load_u32(PACKET_BASE + 32u),
                expected_packets[8], expected_packets[9], expected_packets[10], expected_packets[11],
                expected_packets[16], expected_packets[17], expected_packets[18], expected_packets[19]);
    }
    return ok;
}

int main(void)
{
    static const struct {
        const char* name;
        u16 theta;
        u32 index;
        s32 shift;
        s32 depth;
    } cases[] = {
        { "zero_depth_a", 0x0000u, 0u, 2, 8192 },
        { "quarter_depth_a", 0x0400u, 1u, 2, 8192 },
        { "negative_asym_depth_a", 0xF800u, 0u, 3, 8192 },
        { "near_wrap_depth_a", 0xFF00u, 1u, 2, 8192 },
        { "quarter_depth_b", 0x0400u, 0u, 2, 12288 },
        { "negative_asym_depth_b", 0xF800u, 1u, 3, 12288 },
    };
    u32 i;
    int failures = 0;

    for (i = 0u; i < sizeof(cases) / sizeof(cases[0]); i++) {
        u8 expected_packets[160];
        u8 expected_twins[24];
        u32 ot_address = 0u;
        u32 expected_ot = 0u;
        int expected_submits;

        seed_case(cases[i].theta, cases[i].index, cases[i].shift,
                  cases[i].depth);
#if defined(WM_73B04_TRACE)
        trace_count = 0u;
#endif
        expected_submits = oracle_case(cases[i].theta, cases[i].index,
                                       cases[i].shift, cases[i].depth,
                                       expected_packets, expected_twins,
                                       &ot_address, &expected_ot);
        wm_80073B04();
#if defined(WM_73B04_TRACE)
        if (trace_count < 2u || (trace_events[0] >> 28) != 1u ||
            (trace_events[1] >> 28) != 2u) {
            fprintf(stderr, "ASSERTION ot_link_publication_order[%s]\n",
                    cases[i].name);
            failures++;
            continue;
        }
#endif
        if (expected_submits != 1) {
            fprintf(stderr, "ASSERTION fixture_projection[%s]\n", cases[i].name);
            failures++;
            continue;
        }
        if (!compare_case(cases[i].name, expected_packets, expected_twins,
                          ot_address, expected_ot))
            failures++;
        else
            printf("PASS %s ot=0x%08X head=0x%08X\n", cases[i].name,
                   ot_address, load_u32(ot_address));
    }

    if (failures != 0) {
        fprintf(stderr, "73B04 oracle failures=%d\n", failures);
        return 1;
    }
    printf("PASS 73B04 memory-state oracle cases=%u; packet_bytes=160; twins=24; ot_word=1\n",
           (unsigned)(sizeof(cases) / sizeof(cases[0])));
    return 0;
}
