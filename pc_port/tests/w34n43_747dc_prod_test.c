/* W34N43 — production-linked certificate for retail wm_800747DC. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "guest_prim_link.h"
#include "psx_memory.h"
#include "psyq/libgte.h"
#include <psx/gtereg.h>
#include "world_map_helper_747dc.h"

#define SCRATCH       0x1F800000u
#define CAMERA        0x8009C808u
#define BASE_MATRIX   0x8009A180u
#define QUEUE_COUNT   0x8009BE38u
#define QUEUE_ROOT    0x8009D30Cu
#define PACKET_ROOTS  0x8009BE14u
#define BUFFER_INDEX  0x8009D7F0u
#define CAMERA_X      0x8009BE28u
#define CAMERA_Z      0x8009BE30u
#define DRAW_GLOBAL   0x8009BE3Cu
#define MODE2_ANGLE   0x8006EE66u
#define RECORDS       0x800F1000u
#define PACKETS       0x800F2000u
#define DRAW_RECORD   0x800E0000u
#define OT_BASE       0x800E1000u

uint8_t g_PsxRam[PSX_RAM_SIZE];
GTERegisters gteRegs;

static int s_failures;
static int s_height_calls;
static int s_normal_calls;
static int s_outer_calls;
static int s_vector_normal_calls;
static int s_scale_calls;
static int s_rot_y_calls;
static int s_mode_mul_calls;
static int s_compose_calls;
static int s_set_rot_calls;
static int s_set_trans_calls;
static int s_rot_trans_calls;
static int s_rtpt_calls;
static int s_rtps_calls;
static int s_link_calls;
static s32 s_height_x[8];
static s32 s_height_z[8];
static s32 s_normal_x[8];
static s32 s_normal_z[8];
static u32 s_normal_out[8];
static u32 s_outer_in0[16];
static u32 s_outer_in1[16];
static u32 s_outer_out[16];
static VECTOR s_scale_values[8];
static u32 s_scale_matrix[8];
static u32 s_rot_y_matrix;
static int s_rot_y_angle;
static u32 s_mode_mul_lhs;
static u32 s_mode_mul_rhs;
static u32 s_compose_lhs[8];
static u32 s_compose_rhs[8];
static u32 s_compose_out[8];
static u32 s_set_rot_address[16];
static u32 s_set_trans_address[16];
static s32 s_set_trans_t[16][3];
static SVECTOR s_rot_trans_input[8];
static u32 s_rtpt_vertex[8][3];
static u32 s_rtps_vertex[8];
static u32 s_link_ot[8];
static u32 s_link_packet[8];
static int s_last_rtpt;

static void st16(u32 address, u16 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static void st32(u32 address, u32 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
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
    return PsxMemory_GuestAddr(pointer);
}

static void check(const char *name, int condition)
{
    if (!condition) {
        fprintf(stderr, "ASSERTION %s\n", name);
        s_failures++;
    }
}

static void reset_counters(void)
{
    s_height_calls = 0;
    s_normal_calls = 0;
    s_outer_calls = 0;
    s_vector_normal_calls = 0;
    s_scale_calls = 0;
    s_rot_y_calls = 0;
    s_mode_mul_calls = 0;
    s_compose_calls = 0;
    s_set_rot_calls = 0;
    s_set_trans_calls = 0;
    s_rot_trans_calls = 0;
    s_rtpt_calls = 0;
    s_rtps_calls = 0;
    s_link_calls = 0;
    s_last_rtpt = -1;
    memset(s_height_x, 0, sizeof(s_height_x));
    memset(s_height_z, 0, sizeof(s_height_z));
    memset(s_normal_x, 0, sizeof(s_normal_x));
    memset(s_normal_z, 0, sizeof(s_normal_z));
    memset(s_normal_out, 0, sizeof(s_normal_out));
    memset(s_outer_in0, 0, sizeof(s_outer_in0));
    memset(s_outer_in1, 0, sizeof(s_outer_in1));
    memset(s_outer_out, 0, sizeof(s_outer_out));
    memset(s_scale_values, 0, sizeof(s_scale_values));
    memset(s_scale_matrix, 0, sizeof(s_scale_matrix));
    memset(s_compose_lhs, 0, sizeof(s_compose_lhs));
    memset(s_compose_rhs, 0, sizeof(s_compose_rhs));
    memset(s_compose_out, 0, sizeof(s_compose_out));
    memset(s_set_rot_address, 0, sizeof(s_set_rot_address));
    memset(s_set_trans_address, 0, sizeof(s_set_trans_address));
    memset(s_set_trans_t, 0, sizeof(s_set_trans_t));
    memset(s_rot_trans_input, 0, sizeof(s_rot_trans_input));
    memset(s_rtpt_vertex, 0, sizeof(s_rtpt_vertex));
    memset(s_rtps_vertex, 0, sizeof(s_rtps_vertex));
    memset(s_link_ot, 0, sizeof(s_link_ot));
    memset(s_link_packet, 0, sizeof(s_link_packet));
}

s32 wm_80093978(s32 x, s32 z)
{
    int call = s_height_calls;
    u32 value = (u32)(500 + call) << 12u;
    s32 result;

    if (call < 8) {
        s_height_x[call] = x;
        s_height_z[call] = z;
    }
    s_height_calls++;
    memcpy(&result, &value, sizeof(result));
    return result;
}

s32 wm_80093740(u32 out_normal_addr, s32 x, s32 z)
{
    int call = s_normal_calls;
    s32 values[3] = { 10 + call, 20 + call, 30 + call };

    if (call < 8) {
        s_normal_out[call] = out_normal_addr;
        s_normal_x[call] = x;
        s_normal_z[call] = z;
    }
    memcpy(PSX_ADDR(out_normal_addr), values, sizeof(values));
    s_normal_calls++;
    return 0;
}

void OuterProduct12(VECTOR *left, VECTOR *right, VECTOR *output)
{
    int call = s_outer_calls;

    if (call < 16) {
        s_outer_in0[call] = guest_of(left);
        s_outer_in1[call] = guest_of(right);
        s_outer_out[call] = guest_of(output);
    }
    output->vx = 1000 + call;
    output->vy = 1100 + call;
    output->vz = 1200 + call;
    output->pad = 0;
    s_outer_calls++;
}

long VectorNormal(VECTOR *input, VECTOR *output)
{
    int call = s_vector_normal_calls;
    int record = call / 2;

    (void)input;
    if ((call & 1) == 0) {
        output->vx = 100 + record;
        output->vy = 200 + record;
        output->vz = 300 + record;
    } else {
        output->vx = 400 + record;
        output->vy = 500 + record;
        output->vz = 600 + record;
    }
    output->pad = 0;
    s_vector_normal_calls++;
    return 0;
}

MATRIX *ScaleMatrix(MATRIX *matrix, VECTOR *scale)
{
    int call = s_scale_calls;

    if (call < 8) {
        s_scale_matrix[call] = guest_of(matrix);
        s_scale_values[call] = *scale;
    }
    s_scale_calls++;
    return matrix;
}

MATRIX *RotMatrixY(int angle, MATRIX *matrix)
{
    s_rot_y_calls++;
    s_rot_y_angle = angle;
    s_rot_y_matrix = guest_of(matrix);
    matrix->m[0][0] = 0x123;
    return matrix;
}

MATRIX *MulMatrix(MATRIX *left, MATRIX *right)
{
    s_mode_mul_calls++;
    s_mode_mul_lhs = guest_of(left);
    s_mode_mul_rhs = guest_of(right);
    left->m[0][1] = 0x234;
    return left;
}

MATRIX *MulMatrix0(MATRIX *left, MATRIX *right, MATRIX *output)
{
    int call = s_compose_calls;

    if (call < 8) {
        s_compose_lhs[call] = guest_of(left);
        s_compose_rhs[call] = guest_of(right);
        s_compose_out[call] = guest_of(output);
    }
    memset(output, 0, sizeof(*output));
    output->m[0][0] = (s16)(0x400 + call);
    s_compose_calls++;
    return output;
}

void SetRotMatrix(MATRIX *matrix)
{
    if (s_set_rot_calls < 16)
        s_set_rot_address[s_set_rot_calls] = guest_of(matrix);
    s_set_rot_calls++;
}

void SetTransMatrix(MATRIX *matrix)
{
    if (s_set_trans_calls < 16) {
        int call = s_set_trans_calls;
        s_set_trans_address[call] = guest_of(matrix);
        s_set_trans_t[call][0] = matrix->t[0];
        s_set_trans_t[call][1] = matrix->t[1];
        s_set_trans_t[call][2] = matrix->t[2];
    }
    s_set_trans_calls++;
}

void RotTrans(SVECTOR *input, VECTOR *output, long *flag)
{
    int call = s_rot_trans_calls;

    if (call < 8)
        s_rot_trans_input[call] = *input;
    output->vx = 1000 + call;
    output->vy = 2000 + call;
    output->vz = 3000 + call;
    output->pad = 0;
    *flag = 0;
    s_rot_trans_calls++;
}

int RotTransPers3(SVECTOR *v0, SVECTOR *v1, SVECTOR *v2,
                  long *xy0, long *xy1, long *xy2,
                  long *p, long *flag)
{
    int call = s_rtpt_calls;

    if (call < 8) {
        s_rtpt_vertex[call][0] = guest_of(v0);
        s_rtpt_vertex[call][1] = guest_of(v1);
        s_rtpt_vertex[call][2] = guest_of(v2);
    }
    *xy0 = (long)(UINT32_C(0x00100010) + (u32)call);
    *xy1 = (long)(UINT32_C(0x00200020) + (u32)call);
    *xy2 = (long)(UINT32_C(0x00300030) + (u32)call);
    *p = 0;
    *flag = call == 1 ? (long)UINT32_C(0x80000000) : 0;
    if (call == 0) {
        C2_SZ1 = 0x0300u;
        C2_SZ2 = 0x0200u;
        C2_SZ3 = 0x0280u;
    } else if (call == 2) {
        C2_SZ1 = 0x1000u;
        C2_SZ2 = 0x1100u;
        C2_SZ3 = 0x1200u;
    } else {
        C2_SZ1 = 0x0D50u;
        C2_SZ2 = 0x0D60u;
        C2_SZ3 = 0x0D70u;
    }
    s_last_rtpt = call;
    s_rtpt_calls++;
    return 0;
}

int RotTransPers(SVECTOR *vertex, int *xy, long *p, long *flag)
{
    int call = s_rtps_calls;

    if (call < 8)
        s_rtps_vertex[call] = guest_of(vertex);
    *xy = (int)(UINT32_C(0x00400040) + (u32)s_last_rtpt);
    *p = 0;
    *flag = 0;
    if (s_last_rtpt == 0)
        C2_SZ3 = 0x0250u;
    else if (s_last_rtpt == 2)
        C2_SZ3 = 0x1300u;
    else
        C2_SZ3 = 0x0D40u;
    s_rtps_calls++;
    return 0;
}

void PcPort_AddPrimDomainAware(void *ot_pointer, void *prim_pointer)
{
    u32 ot = guest_of(ot_pointer);
    u32 prim = guest_of(prim_pointer);
    u32 old_ot = ld32(ot);
    u32 old_prim = ld32(prim);

    if (s_link_calls < 8) {
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

static void initialize_active_fixture(void)
{
    u32 index;

    memset(g_PsxRam, 0, sizeof(g_PsxRam));
    memset(&gteRegs, 0, sizeof(gteRegs));
    reset_counters();

    st32(QUEUE_COUNT, 4u);
    st32(QUEUE_ROOT, RECORDS);
    st32(BUFFER_INDEX, 1u);
    st32(PACKET_ROOTS + 0u, PACKETS + 0x400u);
    st32(PACKET_ROOTS + 4u, PACKETS);
    st32(CAMERA_X, 100u << 12u);
    st32(CAMERA_Z, 200u << 12u);
    st32(DRAW_GLOBAL, DRAW_RECORD);
    st32(DRAW_RECORD + 0x70u, OT_BASE);
    st16(MODE2_ANGLE, 0x345u);

    for (index = 0u; index < 32u; index++) {
        *(u8*)PSX_ADDR(CAMERA + index) = (u8)(0x40u + index);
        *(u8*)PSX_ADDR(BASE_MATRIX + index) = (u8)(0x80u + index);
    }
    for (index = 0u; index < 4u; index++) {
        u32 record = RECORDS + index * 8u;
        st16(record + 0u, (u16)(110u + index * 10u));
        st16(record + 2u, (u16)index);
        st16(record + 4u, (u16)(220u + index * 10u));
        st16(record + 6u, (u16)(0xA000u + index));
    }
    for (index = 0u; index < 4u; index++)
        st32(PACKETS + index * 0x28u,
             UINT32_C(0x09000000) | (0x100u + index));
    st32(OT_BASE + 0x20u * 4u, UINT32_C(0xAA000220));
    st32(OT_BASE + 0xD4u * 4u, UINT32_C(0xAA0002D4));
}

static void check_zero_gate(void)
{
    u8 scratch_before[0x180];

    memset(g_PsxRam, 0x5A, sizeof(g_PsxRam));
    memset(&gteRegs, 0, sizeof(gteRegs));
    reset_counters();
    st32(QUEUE_COUNT, 0u);
    memcpy(scratch_before, PSX_ADDR(SCRATCH), sizeof(scratch_before));
    wm_800747DC();
    check("zero-count-is-noop",
          s_height_calls == 0 && s_normal_calls == 0 &&
          s_rtpt_calls == 0 && s_link_calls == 0 &&
          memcmp(scratch_before, PSX_ADDR(SCRATCH),
                 sizeof(scratch_before)) == 0);
}

static void check_active_path(void)
{
    u8 records_before[32];
    u8 camera_before[32];
    u8 base_before[32];
    u32 scratch_guest = guest_of(PSX_ADDR(SCRATCH));
    int index;

    initialize_active_fixture();
    memcpy(records_before, PSX_ADDR(RECORDS), sizeof(records_before));
    memcpy(camera_before, PSX_ADDR(CAMERA), sizeof(camera_before));
    memcpy(base_before, PSX_ADDR(BASE_MATRIX), sizeof(base_before));
    wm_800747DC();

    check("exact-record-count",
          s_height_calls == 4 && s_normal_calls == 4 &&
          s_outer_calls == 8 && s_vector_normal_calls == 8 &&
          s_compose_calls == 4 && s_rot_trans_calls == 4 &&
          s_rtpt_calls == 4);
    for (index = 0; index < 4; index++) {
        s32 expected_x = (110 + index * 10) * 4096;
        s32 expected_z = (220 + index * 10) * 4096;
        check("queue-record-source-and-stride",
              s_height_x[index] == expected_x &&
              s_height_z[index] == expected_z &&
              s_normal_x[index] == expected_x &&
              s_normal_z[index] == expected_z &&
              s_normal_out[index] == SCRATCH + 0x30u);
    }
    check("retail-fixed-quad",
          ld16(SCRATCH + 0xA0u) == (u16)(s16)-16 &&
          ld16(SCRATCH + 0xA2u) == 0u &&
          ld16(SCRATCH + 0xA4u) == 16u &&
          ld16(SCRATCH + 0xA8u) == 16u &&
          ld16(SCRATCH + 0xAAu) == 0u &&
          ld16(SCRATCH + 0xACu) == 16u &&
          ld16(SCRATCH + 0xB0u) == (u16)(s16)-16 &&
          ld16(SCRATCH + 0xB4u) == (u16)(s16)-16 &&
          ld16(SCRATCH + 0xD8u) == 16u &&
          ld16(SCRATCH + 0xDCu) == (u16)(s16)-16);
    check("basis-cross-order",
          s_outer_in0[0] == scratch_guest + 0x40u &&
          s_outer_in1[0] == scratch_guest + 0x30u &&
          s_outer_out[0] == scratch_guest + 0x50u &&
          s_outer_in0[1] == scratch_guest + 0x30u &&
          s_outer_in1[1] == scratch_guest + 0x60u &&
          s_outer_out[1] == scratch_guest + 0x80u);
    check("mode-scale-values",
          s_scale_calls == 2 &&
          s_scale_matrix[0] == scratch_guest + 0xF0u &&
          s_scale_values[0].vx == 0x1800 &&
          s_scale_values[0].vy == 0x1800 &&
          s_scale_values[0].vz == 0x1800 &&
          s_scale_matrix[1] == scratch_guest + 0xF0u &&
          s_scale_values[1].vx == 0x1800 &&
          s_scale_values[1].vy == 0x1000 &&
          s_scale_values[1].vz == 0x4800);
    check("mode2-rotation-chain",
          s_rot_y_calls == 1 && s_rot_y_angle == 0x345 &&
          s_rot_y_matrix == scratch_guest + 0x150u &&
          s_mode_mul_calls == 1 &&
          s_mode_mul_lhs == scratch_guest + 0xF0u &&
          s_mode_mul_rhs == scratch_guest + 0x150u);
    for (index = 0; index < 4; index++) {
        check("camera-left-basis-composition",
              s_compose_lhs[index] == scratch_guest + 0x130u &&
              s_compose_rhs[index] == scratch_guest + 0xF0u &&
              s_compose_out[index] == scratch_guest + 0x110u);
        check("camera-relative-position",
              s_rot_trans_input[index].vx == 10 + index * 10 &&
              s_rot_trans_input[index].vy == 500 + index &&
              s_rot_trans_input[index].vz == -20 - index * 10);
        check("camera-and-composed-install-order",
              s_set_rot_address[index * 2] == scratch_guest + 0x130u &&
              s_set_rot_address[index * 2 + 1] == scratch_guest + 0x110u &&
              s_set_trans_address[index * 2] == scratch_guest + 0x130u &&
              s_set_trans_address[index * 2 + 1] == scratch_guest + 0x110u &&
              s_set_trans_t[index * 2 + 1][0] == 1000 + index &&
              s_set_trans_t[index * 2 + 1][1] == 2000 + index &&
              s_set_trans_t[index * 2 + 1][2] == 3000 + index);
        check("projection-vertex-order",
              s_rtpt_vertex[index][0] == scratch_guest + 0xA0u &&
              s_rtpt_vertex[index][1] == scratch_guest + 0xA8u &&
              s_rtpt_vertex[index][2] == scratch_guest + 0xB0u);
    }
    check("negative-flag-stops-fourth-vertex",
          s_rtps_calls == 3 &&
          s_rtps_vertex[0] == scratch_guest + 0xD8u &&
          s_rtps_vertex[1] == scratch_guest + 0xD8u &&
          s_rtps_vertex[2] == scratch_guest + 0xD8u);
    check("minimum-depth-and-retail-limit",
          s_link_calls == 2 &&
          s_link_ot[0] == OT_BASE + 0x20u * 4u &&
          s_link_ot[1] == OT_BASE + 0xD4u * 4u);
    check("compact-packet-cursor",
          s_link_packet[0] == PACKETS &&
          s_link_packet[1] == PACKETS + 0x28u);
    check("accepted-xy-publication",
          ld32(PACKETS + 0x08u) == UINT32_C(0x00100010) &&
          ld32(PACKETS + 0x10u) == UINT32_C(0x00200020) &&
          ld32(PACKETS + 0x18u) == UINT32_C(0x00300030) &&
          ld32(PACKETS + 0x20u) == UINT32_C(0x00400040) &&
          ld32(PACKETS + 0x28u + 0x08u) == UINT32_C(0x00100013) &&
          ld32(PACKETS + 0x28u + 0x20u) == UINT32_C(0x00400043));
    check("domain-aware-publication",
          (ld32(PACKETS) & 0xFF000000u) == UINT32_C(0x09000000) &&
          (ld32(PACKETS + 0x28u) & 0xFF000000u) ==
              UINT32_C(0x09000000) &&
          (ld32(OT_BASE + 0x20u * 4u) & 0x00FFFFFFu) ==
              (PACKETS & 0x00FFFFFFu) &&
          (ld32(OT_BASE + 0xD4u * 4u) & 0x00FFFFFFu) ==
              ((PACKETS + 0x28u) & 0x00FFFFFFu));
    check("queue-drained", ld32(QUEUE_COUNT) == 0u);
    check("retail-inputs-read-only",
          memcmp(records_before, PSX_ADDR(RECORDS), sizeof(records_before)) == 0 &&
          memcmp(camera_before, PSX_ADDR(CAMERA), sizeof(camera_before)) == 0 &&
          memcmp(base_before, PSX_ADDR(BASE_MATRIX), sizeof(base_before)) == 0);
}

int main(void)
{
    check_zero_gate();
    check_active_path();
    if (s_failures != 0)
        return EXIT_FAILURE;
    puts("W34N43 0x800747DC full-body certificate PASS");
    return EXIT_SUCCESS;
}
