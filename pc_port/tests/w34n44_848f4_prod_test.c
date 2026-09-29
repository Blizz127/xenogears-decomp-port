/* W34N44 — production-linked certificate for retail wm_800848F4. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "psyq/libgte.h"
#include <psx/gtereg.h>
#include "world_map_helper_848f4.h"

#define SCRATCH       0x1F800000u
#define CAMERA        0x8009C808u
#define CAMERA_X      0x8009BE28u
#define CAMERA_Z      0x8009BE30u
#define DRAW_GLOBAL   0x8009BE3Cu
#define RECORD_ROOT   0x8009C620u
#define RECORD_COUNT  0x8009D7E0u
#define BUFFER_INDEX  0x8009D7F0u
#define VARIANTS      0x8009AD2Cu
#define RECORDS       0x800D0000u
#define DRAW_RECORD   0x800E0000u
#define OT_BASE       0x800E1000u
#define BUFFER0       0x800E2000u
#define BUFFER1       0x800E2100u
#define MODEL_BASE    0x800B0000u

uint8_t g_PsxRam[PSX_RAM_SIZE];
GTERegisters gteRegs;
s32 D_80050104;
u32 D_800595C0;
s32 D_80059578;

static int s_failures;
static int s_wrap_calls;
static int s_scale_calls;
static int s_parent_mul_calls;
static int s_set_rot_calls;
static int s_set_trans_calls;
static int s_rot_trans_calls;
static int s_comp_calls;
static int s_project_calls;
static int s_dispatch_calls;
static s32 s_wrap_in[8][2];
static s32 s_scale_value[8][3];
static s32 s_scale_translation[8][3];
static s16 s_scale_m00[8];
static u32 s_parent_lhs[8];
static u32 s_parent_rhs[8];
static SVECTOR s_parent_input[8];
static u32 s_comp_lhs[8];
static u32 s_comp_rhs[8];
static s32 s_comp_rhs_t[8][3];
static u32 s_set_rot_arg[8];
static u32 s_set_trans_arg[16];
static u32 s_project_vertex[8];
static u32 s_dispatch_model[8];
static uintptr_t s_dispatch_buffer[8];
static u32 s_dispatch_ot[8];
static s32 s_dispatch_variant[8];

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

static s32 lds32(u32 address)
{
    s32 value;

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

static void reset_trace(void)
{
    s_wrap_calls = 0;
    s_scale_calls = 0;
    s_parent_mul_calls = 0;
    s_set_rot_calls = 0;
    s_set_trans_calls = 0;
    s_rot_trans_calls = 0;
    s_comp_calls = 0;
    s_project_calls = 0;
    s_dispatch_calls = 0;
    memset(s_wrap_in, 0, sizeof(s_wrap_in));
    memset(s_scale_value, 0, sizeof(s_scale_value));
    memset(s_scale_translation, 0, sizeof(s_scale_translation));
    memset(s_scale_m00, 0, sizeof(s_scale_m00));
    memset(s_parent_lhs, 0, sizeof(s_parent_lhs));
    memset(s_parent_rhs, 0, sizeof(s_parent_rhs));
    memset(s_parent_input, 0, sizeof(s_parent_input));
    memset(s_comp_lhs, 0, sizeof(s_comp_lhs));
    memset(s_comp_rhs, 0, sizeof(s_comp_rhs));
    memset(s_comp_rhs_t, 0, sizeof(s_comp_rhs_t));
    memset(s_set_rot_arg, 0, sizeof(s_set_rot_arg));
    memset(s_set_trans_arg, 0, sizeof(s_set_trans_arg));
    memset(s_project_vertex, 0, sizeof(s_project_vertex));
    memset(s_dispatch_model, 0, sizeof(s_dispatch_model));
    memset(s_dispatch_buffer, 0, sizeof(s_dispatch_buffer));
    memset(s_dispatch_ot, 0, sizeof(s_dispatch_ot));
    memset(s_dispatch_variant, 0, sizeof(s_dispatch_variant));
}

void wm_80093534(u32 vector)
{
    int call = s_wrap_calls;
    s32 x = lds32(vector + 0u);
    s32 z = lds32(vector + 8u);

    if (call < 8) {
        s_wrap_in[call][0] = x;
        s_wrap_in[call][1] = z;
    }
    st32(vector + 0u, (u32)(x + 10));
    st32(vector + 8u, (u32)(z + 20));
    s_wrap_calls++;
}

MATRIX *ScaleMatrix(MATRIX *matrix, VECTOR *scale)
{
    int call = s_scale_calls;

    if (call < 8) {
        s_scale_value[call][0] = scale->vx;
        s_scale_value[call][1] = scale->vy;
        s_scale_value[call][2] = scale->vz;
        s_scale_translation[call][0] = matrix->t[0];
        s_scale_translation[call][1] = matrix->t[1];
        s_scale_translation[call][2] = matrix->t[2];
        s_scale_m00[call] = matrix->m[0][0];
    }
    s_scale_calls++;
    return matrix;
}

MATRIX *MulMatrix0(MATRIX *left, MATRIX *right, MATRIX *output)
{
    int call = s_parent_mul_calls;

    if (call < 8) {
        s_parent_lhs[call] = guest_of(left);
        s_parent_rhs[call] = guest_of(right);
    }
    memset(output, 0, sizeof(*output));
    output->m[0][0] = (s16)0x0710;
    output->m[1][1] = (s16)0x0720;
    output->m[2][2] = (s16)0x0730;
    s_parent_mul_calls++;
    return output;
}

void SetRotMatrix(MATRIX *matrix)
{
    if (s_set_rot_calls < 8)
        s_set_rot_arg[s_set_rot_calls] = guest_of(matrix);
    s_set_rot_calls++;
}

void SetTransMatrix(MATRIX *matrix)
{
    if (s_set_trans_calls < 16)
        s_set_trans_arg[s_set_trans_calls] = guest_of(matrix);
    s_set_trans_calls++;
}

void RotTrans(SVECTOR *input, VECTOR *output, long *flag)
{
    int call = s_rot_trans_calls;

    if (call < 8)
        s_parent_input[call] = *input;
    output->vx = 5000 + call;
    output->vy = 6000 + call;
    output->vz = -7000 - call;
    output->pad = 0;
    *flag = 0;
    s_rot_trans_calls++;
}

MATRIX *CompMatrix(MATRIX *left, MATRIX *right, MATRIX *output)
{
    int call = s_comp_calls;

    if (call < 8) {
        s_comp_lhs[call] = guest_of(left);
        s_comp_rhs[call] = guest_of(right);
        s_comp_rhs_t[call][0] = right->t[0];
        s_comp_rhs_t[call][1] = right->t[1];
        s_comp_rhs_t[call][2] = right->t[2];
    }
    memset(output, 0, sizeof(*output));
    output->m[0][0] = (s16)(0x0400 + call);
    output->t[0] = 100 + call;
    output->t[1] = 200 + call;
    output->t[2] = 300 + call;
    s_comp_calls++;
    return output;
}

int RotTransPers(SVECTOR *vertex, int *xy, long *p, long *flag)
{
    int call = s_project_calls;

    if (call < 8)
        s_project_vertex[call] = guest_of(vertex);
    *xy = 0x00100010 + call;
    *p = 0;
    if (call == 1) {
        *flag = (long)UINT32_C(0x80000000);
        C2_SZ3 = 0x0500;
    } else if (call == 2) {
        *flag = 0;
        C2_SZ3 = 0x0D80;
    } else {
        *flag = 0;
        C2_SZ3 = 0x0500;
    }
    s_project_calls++;
    return (int)C2_SZ3;
}

s32 func_8002C700(u8 *model, u8 *buffer, u32 *ot, s32 variant)
{
    int call = s_dispatch_calls;

    if (call < 8) {
        s_dispatch_model[call] = guest_of(model);
        s_dispatch_buffer[call] = (uintptr_t)buffer;
        s_dispatch_ot[call] = guest_of(ot);
        s_dispatch_variant[call] = variant;
    }
    s_dispatch_calls++;
    return 1;
}

static void put_matrix(u32 address, s16 seed)
{
    MATRIX matrix;
    int row;
    int column;

    memset(&matrix, 0, sizeof(matrix));
    for (row = 0; row < 3; row++) {
        for (column = 0; column < 3; column++)
            matrix.m[row][column] =
                (s16)(seed + (s16)(row * 3 + column));
    }
    memcpy(PSX_ADDR(address), &matrix, sizeof(matrix));
}

static void put_record(int index, s16 state, s16 kind, s32 x, s32 y,
                       s32 z, u32 model, u32 link)
{
    u32 record = RECORDS + (u32)index * 0x54u;

    memset(PSX_ADDR(record), 0xA5, 0x54u);
    st16(record + 0u, (u16)state);
    st16(record + 4u, (u16)kind);
    st32(record + 8u, (u32)x);
    st32(record + 0x0Cu, (u32)y);
    st32(record + 0x10u, (u32)z);
    put_matrix(record + 0x20u, (s16)(0x0100 + index * 0x20));
    st32(record + 0x40u, model);
    st32(record + 0x48u, BUFFER0);
    st32(record + 0x4Cu, BUFFER1);
    st32(record + 0x50u, link);
}

static void initialize_fixture(void)
{
    memset(g_PsxRam, 0, sizeof(g_PsxRam));
    memset(&gteRegs, 0, sizeof(gteRegs));
    memset(PSX_ADDR(BUFFER0), 0x10, 64u);
    memset(PSX_ADDR(BUFFER1), 0x20, 64u);
    reset_trace();

    D_80050104 = 99;
    D_800595C0 = 1234u;
    D_80059578 = 5678;
    st32(RECORD_ROOT, RECORDS);
    st16(RECORD_COUNT, 4u);
    st32(BUFFER_INDEX, 1u);
    st32(CAMERA_X, 100u << 12u);
    st32(CAMERA_Z, 200u << 12u);
    st32(DRAW_GLOBAL, DRAW_RECORD);
    st32(DRAW_RECORD + 0x70u, OT_BASE);
    put_matrix(CAMERA, (s16)0x0300);

    put_record(0, 0, -1, 1000, 20, 2000, MODEL_BASE + 0x000u, 0u);
    put_record(1, 1, 2, 2000, 30, 3000, MODEL_BASE + 0x100u, 0u);
    put_record(2, 0, 3, 3000, 40, 4000, MODEL_BASE + 0x200u, RECORDS);
    put_record(3, 0, 4, 4000, 50, 5000, MODEL_BASE + 0x300u, 0u);
    st16(VARIANTS - 2u, (u16)(s16)-3);
    st16(VARIANTS + 6u, 5u);
    st16(VARIANTS + 8u, 6u);
}

static void check_prefix_and_gate(void)
{
    memset(g_PsxRam, 0x5A, sizeof(g_PsxRam));
    memset(&gteRegs, 0, sizeof(gteRegs));
    reset_trace();
    D_80050104 = -1;
    D_800595C0 = 123u;
    D_80059578 = 456;
    st16(RECORD_COUNT, 0u);
    wm_800848F4();

    check("zero-count-prefix",
          D_80050104 == 3 && D_800595C0 == 0u && D_80059578 == 0 &&
          ld32(SCRATCH + 0x10u) == 0x800u &&
          ld32(SCRATCH + 0x14u) == 0x800u &&
          ld32(SCRATCH + 0x18u) == 0x800u &&
          ld16(SCRATCH + 0xA0u) == 0u &&
          ld16(SCRATCH + 0xA2u) == 0u &&
          ld16(SCRATCH + 0xA4u) == 0u);
    check("zero-count-no-body",
          s_wrap_calls == 0 && s_scale_calls == 0 &&
          s_comp_calls == 0 && s_project_calls == 0 &&
          s_dispatch_calls == 0);
}

static void check_active_path(void)
{
    u8 skipped_before[0x54];
    u8 rec2_before[0x54];
    u8 rec3_before[0x54];

    initialize_fixture();
    memcpy(skipped_before, PSX_ADDR(RECORDS + 0x54u), sizeof(skipped_before));
    memcpy(rec2_before, PSX_ADDR(RECORDS + 2u * 0x54u), sizeof(rec2_before));
    memcpy(rec3_before, PSX_ADDR(RECORDS + 3u * 0x54u), sizeof(rec3_before));
    wm_800848F4();

    check("native-renderer-authorities",
          D_80050104 == 3 && D_800595C0 == 0u && D_80059578 == 0);
    check("record-stride-and-state-gate",
          s_scale_calls == 3 && s_comp_calls == 3 &&
          s_project_calls == 3 &&
          memcmp(skipped_before, PSX_ADDR(RECORDS + 0x54u),
                 sizeof(skipped_before)) == 0);
    check("camera-relative-wrap-input",
          s_wrap_calls == 3 &&
          s_wrap_in[0][0] == 900 && s_wrap_in[0][1] == 1800 &&
          s_wrap_in[1][0] == 4900 && s_wrap_in[1][1] == 6800 &&
          s_wrap_in[2][0] == 3900 && s_wrap_in[2][1] == 4800);
    check("parent-chain-order",
          s_parent_mul_calls == 1 && s_rot_trans_calls == 1 &&
          s_parent_lhs[0] == RECORDS + 0x20u &&
          s_parent_rhs[0] == 0x800000F0u &&
          s_parent_input[0].vx == 3000 &&
          s_parent_input[0].vy == 40 &&
          s_parent_input[0].vz == -4000);
    check("parent-translation-publication",
          lds32(RECORDS + 0x34u) == 1000 &&
          lds32(RECORDS + 0x38u) == 20 &&
          lds32(RECORDS + 0x3Cu) == -2000);
    check("retail-scale",
          s_scale_value[0][0] == 0x800 &&
          s_scale_value[0][1] == 0x800 &&
          s_scale_value[0][2] == 0x800 &&
          s_scale_value[1][0] == 0x800 &&
          s_scale_value[2][0] == 0x800);
    check("matrix-copy-source",
          s_scale_m00[0] == (s16)0x0100 &&
          s_scale_m00[1] == (s16)0x0710 &&
          s_scale_m00[2] == (s16)0x0160);
    check("wrapped-translation",
          s_scale_translation[0][0] == 910 &&
          s_scale_translation[0][1] == 20 &&
          s_scale_translation[0][2] == -1820 &&
          s_scale_translation[1][0] == 4910 &&
          s_scale_translation[1][1] == 6000 &&
          s_scale_translation[1][2] == -6820);
    check("camera-compmatrix-order",
          s_comp_lhs[0] == CAMERA && s_comp_lhs[1] == CAMERA &&
          s_comp_lhs[2] == CAMERA &&
          s_comp_rhs[0] == 0x800000F0u &&
          s_comp_rhs[1] == 0x800000F0u &&
          s_comp_rhs[2] == 0x800000F0u);
    check("composed-gte-publication",
          s_set_rot_calls == 3 &&
          s_set_rot_arg[0] == 0x80000110u &&
          s_set_rot_arg[1] == 0x80000110u &&
          s_set_rot_arg[2] == 0x80000110u &&
          s_set_trans_calls == 4 &&
          s_set_trans_arg[0] == 0x80000110u &&
          s_set_trans_arg[1] == RECORDS + 0x20u &&
          s_set_trans_arg[2] == 0x80000110u &&
          s_set_trans_arg[3] == 0x80000110u);
    check("origin-projection",
          s_project_vertex[0] == 0x800000A0u &&
          s_project_vertex[1] == 0x800000A0u &&
          s_project_vertex[2] == 0x800000A0u);
    check("flag-and-depth-gates", s_dispatch_calls == 1);
    check("renderer-domain-and-buffer",
          s_dispatch_model[0] == MODEL_BASE &&
          s_dispatch_buffer[0] == (uintptr_t)PSX_ADDR(BUFFER1) &&
          s_dispatch_ot[0] == OT_BASE);
    check("signed-variant-table", s_dispatch_variant[0] == -3);
    check("nonparent-records-read-only",
          memcmp(rec2_before, PSX_ADDR(RECORDS + 2u * 0x54u),
                 sizeof(rec2_before)) == 0 &&
          memcmp(rec3_before, PSX_ADDR(RECORDS + 3u * 0x54u),
                 sizeof(rec3_before)) == 0);
}

int main(void)
{
    check_prefix_and_gate();
    check_active_path();
    if (s_failures != 0)
        return 1;
    puts("W34N44 0x800848F4 full-body certificate PASS");
    return 0;
}
