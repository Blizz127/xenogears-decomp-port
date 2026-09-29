/* W34N40 — production-linked certificate for retail wm_800737EC. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "guest_prim_link.h"
#include "psx_memory.h"
#include "psyq/libgte.h"
#include "world_map_helper_737ec.h"

#define SCRATCH      0x1F800000u
#define VERTICES     0x8009A280u
#define HEADING      0x8009BD3Au
#define CAMERA       0x8009C808u
#define PACKETS      0x8009D194u
#define DRAW_GLOBAL  0x8009BE3Cu
#define BUFFER_INDEX 0x8009D7F0u
#define DRAW_RECORD  0x800E0000u
#define OT_BASE      0x800E1000u

uint8_t g_PsxRam[PSX_RAM_SIZE];

static int s_failures;
static int s_rot_calls;
static int s_comp_calls;
static int s_set_rot_calls;
static int s_set_trans_calls;
static int s_projection_calls;
static int s_shift_calls;
static int s_link_calls;
static u32 s_rot_input;
static u32 s_rot_output;
static u32 s_comp_input0;
static u32 s_comp_input1;
static u32 s_comp_output;
static s32 s_rotation_t[3];
static u32 s_set_rot_address;
static u32 s_set_trans_address;
static u32 s_vertex_address[4][4];
static u32 s_link_ot[4];
static u32 s_link_packet[4];

static void st16(u32 address, u16 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static void st32(u32 address, u32 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static u32 ld32(u32 address)
{
    u32 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static u32 guest_of(const void *pointer)
{
    return PsxMemory_GuestAddr(pointer);
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
    memset(output, 0x11, sizeof(*output));
    output->t[0] = 111;
    output->t[1] = 222;
    output->t[2] = 333;
    return output;
}

MATRIX *CompMatrix(MATRIX *input0, MATRIX *input1, MATRIX *output)
{
    s_comp_calls++;
    s_comp_input0 = guest_of(input0);
    s_comp_input1 = guest_of(input1);
    s_comp_output = guest_of(output);
    s_rotation_t[0] = input1->t[0];
    s_rotation_t[1] = input1->t[1];
    s_rotation_t[2] = input1->t[2];
    memset(output, 0x22, sizeof(*output));
    output->t[0] = 100;
    output->t[1] = 200;
    output->t[2] = 300;
    return output;
}

void SetRotMatrix(MATRIX *matrix)
{
    s_set_rot_calls++;
    s_set_rot_address = guest_of(matrix);
}

void SetTransMatrix(MATRIX *matrix)
{
    s_set_trans_calls++;
    s_set_trans_address = guest_of(matrix);
}

int RotTransPers4(SVECTOR *v0, SVECTOR *v1, SVECTOR *v2, SVECTOR *v3,
                  long *xy0, long *xy1, long *xy2, long *xy3,
                  long *p, long *flag)
{
    int call = s_projection_calls;
    SVECTOR *vertices[4] = { v0, v1, v2, v3 };
    int vertex;

    if (call < 4) {
        for (vertex = 0; vertex < 4; vertex++)
            s_vertex_address[call][vertex] = guest_of(vertices[vertex]);
    }
    *xy0 = (long)(UINT32_C(0x00100010) + (u32)call);
    *xy1 = (long)(UINT32_C(0x00200020) + (u32)call);
    *xy2 = (long)(UINT32_C(0x00300030) + (u32)call);
    *xy3 = (long)(UINT32_C(0x00400040) + (u32)call);
    *p = (long)(UINT32_C(0x700) + (u32)call);
    if (call == 2)
        *flag = (long)UINT32_C(0x80000000);
    else if (call == 1)
        *flag = (long)UINT32_C(0x00001000);
    else
        *flag = 0;
    s_projection_calls++;
    return 64 + call * 4;
}

s32 wm_73b04_ot_shift(void)
{
    s_shift_calls++;
    return 2;
}

void PcPort_AddPrimDomainAware(void *ot_pointer, void *prim_pointer)
{
    u32 ot = guest_of(ot_pointer);
    u32 prim = guest_of(prim_pointer);
    u32 old_ot = ld32(ot);
    u32 old_prim = ld32(prim);

    if (s_link_calls < 4) {
        s_link_ot[s_link_calls] = ot;
        s_link_packet[s_link_calls] = prim;
    }
    st32(prim, (old_prim & 0xFF000000u) | (old_ot & 0x00FFFFFFu));
    st32(ot, (old_ot & 0xFF000000u) | (prim & 0x00FFFFFFu));
    s_link_calls++;
}

void PcPort_PrimLinkReset(void) { }
int PcPort_PrimLinkGuestCount(void) { return s_link_calls; }
int PcPort_PrimLinkNativeCount(void) { return 0; }
int PcPort_PrimLinkRejectCount(void) { return 0; }

static void reset_fixture(void)
{
    u32 quad_index;

    memset(g_PsxRam, 0, sizeof(g_PsxRam));
    s_failures = 0;
    s_rot_calls = 0;
    s_comp_calls = 0;
    s_set_rot_calls = 0;
    s_set_trans_calls = 0;
    s_projection_calls = 0;
    s_shift_calls = 0;
    s_link_calls = 0;
    memset(s_vertex_address, 0, sizeof(s_vertex_address));
    memset(s_link_ot, 0, sizeof(s_link_ot));
    memset(s_link_packet, 0, sizeof(s_link_packet));

    st16(HEADING, UINT16_C(0x345));
    memset(PSX_ADDR(CAMERA), 0x5A, sizeof(MATRIX));
    st32(DRAW_GLOBAL, DRAW_RECORD);
    st32(DRAW_RECORD + 0x70u, OT_BASE);
    st32(BUFFER_INDEX, 1u);

    for (quad_index = 0u; quad_index < 4u; quad_index++) {
        u32 packet = PACKETS + 0x24u + quad_index * 0x48u;
        u32 bucket = 16u + quad_index;
        st32(packet, UINT32_C(0x09000000) | (0x100u + quad_index));
        st32(OT_BASE + bucket * 4u,
             UINT32_C(0xAA000000) | (0x200u + quad_index));
    }
}

static void run_certificate(void)
{
    u32 quad_index;
    u32 scratch_guest = guest_of(PSX_ADDR(SCRATCH));
    u8 vertices_before[0x80];
    u8 camera_before[sizeof(MATRIX)];

    reset_fixture();
    memcpy(vertices_before, PSX_ADDR(VERTICES), sizeof(vertices_before));
    memcpy(camera_before, PSX_ADDR(CAMERA), sizeof(camera_before));
    wm_800737EC();

    check("heading-y-axis",
          s_rot_calls == 1 && s_rot_input == scratch_guest &&
          *(u16*)PSX_ADDR(SCRATCH + 0u) == 0u &&
          *(u16*)PSX_ADDR(SCRATCH + 2u) == UINT16_C(0x345) &&
          *(u16*)PSX_ADDR(SCRATCH + 4u) == 0u);
    check("matrix-scratch-addresses",
          s_rot_output == scratch_guest + 0x28u &&
          s_comp_output == scratch_guest + 0x08u);
    check("rotation-translation-cleared",
          s_rotation_t[0] == 0 && s_rotation_t[1] == 0 &&
          s_rotation_t[2] == 0);
    check("camera-left-composition",
          s_comp_calls == 1 && s_comp_input0 == CAMERA &&
          s_comp_input1 == scratch_guest + 0x28u);
    check("composed-matrix-installed",
          s_set_rot_calls == 1 && s_set_trans_calls == 1 &&
          s_set_rot_address == scratch_guest + 0x08u &&
          s_set_trans_address == scratch_guest + 0x08u);
    check("fixed-four-quads", s_projection_calls == 4);

    for (quad_index = 0u; quad_index < 4u; quad_index++) {
        u32 vertex = VERTICES + quad_index * 0x20u;
        u32 packet = PACKETS + 0x24u + quad_index * 0x48u;
        int addresses_ok =
            s_vertex_address[quad_index][0] == vertex + 0x00u &&
            s_vertex_address[quad_index][1] == vertex + 0x08u &&
            s_vertex_address[quad_index][2] == vertex + 0x10u &&
            s_vertex_address[quad_index][3] == vertex + 0x18u;
        int xy_ok =
            ld32(packet + 0x08u) == UINT32_C(0x00100010) + quad_index &&
            ld32(packet + 0x10u) == UINT32_C(0x00200020) + quad_index &&
            ld32(packet + 0x18u) == UINT32_C(0x00300030) + quad_index &&
            ld32(packet + 0x20u) == UINT32_C(0x00400040) + quad_index;
        check("retail-vertex-addresses", addresses_ok);
        check("retail-interleaved-packet-addresses", xy_ok);
    }

    check("negative-flag-rejects-publication",
          s_link_calls == 3 &&
          s_link_packet[0] == PACKETS + 0x24u &&
          s_link_packet[1] == PACKETS + 0x24u + 0x48u &&
          s_link_packet[2] == PACKETS + 0x24u + 3u * 0x48u);
    check("retail-depth-shift-and-ot-buckets",
          s_shift_calls == 3 &&
          s_link_ot[0] == OT_BASE + 16u * 4u &&
          s_link_ot[1] == OT_BASE + 17u * 4u &&
          s_link_ot[2] == OT_BASE + 19u * 4u);
    check("domain-aware-tag-publication",
          (ld32(PACKETS + 0x24u) & 0xFF000000u) == 0x09000000u &&
          (ld32(PACKETS + 0x24u) & 0x00FFFFFFu) == 0x200u &&
          (ld32(OT_BASE + 16u * 4u) & 0xFF000000u) == 0xAA000000u &&
          (ld32(OT_BASE + 16u * 4u) & 0x00FFFFFFu) ==
              ((PACKETS + 0x24u) & 0x00FFFFFFu));
    check("positive-flag-is-not-rejected",
          s_link_packet[1] == PACKETS + 0x24u + 0x48u);
    check("inputs-read-only",
          memcmp(vertices_before, PSX_ADDR(VERTICES),
                 sizeof(vertices_before)) == 0 &&
          memcmp(camera_before, PSX_ADDR(CAMERA),
                 sizeof(camera_before)) == 0);

    if (s_failures == 0)
        puts("W34N40 0x800737EC full-body certificate PASS");
}

int main(void)
{
    run_certificate();
    return s_failures == 0 ? 0 : 1;
}
