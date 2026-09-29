/* W34N37 — production-linked certificate for the full retail wm_80089C78.
 *
 * The retail loop reads 256 x 0x4c records through the pointer at 0x8009BDF4,
 * skips zero state halfwords at record+6, builds a camera-relative VECTOR in
 * scratchpad at 0x1f800088, and passes that scratch address to wm_80093534.
 * The fixture makes the obsolete fixed 0x8009B040 interpretation live at the
 * exact record whose +0x30 aliases allocator global 0x8009D7EC. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "psyq/libgte.h"
#include <psx/gtereg.h>
#include "world_map_helper_89c78.h"

#define SCRATCH       0x1F800000u
#define CAMERA_MATRIX 0x8009C808u
#define MODEL_MATRIX  0x8009A180u
#define FIXED_TABLE   0x8009B040u
#define POOL_GLOBAL   0x8009BDF4u
#define CAMERA_X      0x8009BE28u
#define CAMERA_Z      0x8009BE30u
#define WRAP_X        0x8009D160u
#define WRAP_Z        0x8009D2B4u
#define ALLOC_A       0x8009D7E8u
#define ALLOC_B       0x8009D7ECu
#define PACKET_ROOT   0x8009BE1Cu
#define DRAW_RECORD_G 0x8009BE3Cu
#define BUFFER_INDEX  0x8009D7F0u
#define UV_TABLE      0x8009AFF0u
#define DRAW_RECORD   0x800E0000u
#define OT_BASE       0x800E1000u
#define PACKETS       0x80130000u
#define POOL           0x80110000u
#define COUNT          256u
#define STRIDE         0x4Cu

GTERegisters gteRegs;

/* Satisfies world_map_helper_89c78.c's TEMP-DIAG per-object trace hook. The
 * trace only fires when XENO_WM_OBJ_DIAG is set, which no test does. */
int PcPort_WorldCaptureCurFrame(void) { return 0; }
static int s_pass;
static int s_total;
static int s_rot_calls;
static int s_scale_calls;
static int s_load_count;
static int s_store_count;
static int s_asserted;
static int s_rot_angle;
static int s_set_rot_calls;
static int s_set_trans_calls;
static int s_apply_calls;
static int s_projection_calls;
static int s_link_calls;
static int s_tail_scenario;
static u32 s_load_addr[32];
static u32 s_store_addr[16];
static u32 s_store_value[16];
static VECTOR s_scales[4];
static u32 s_scale_address[16];
static u32 s_set_rot_address[32];
static u32 s_set_trans_address[16];
static s32 s_set_trans_t[16][3];
static SVECTOR s_apply_input[16];
static u32 s_projection_vertex_word[16];
static u32 s_link_ot[16];
static u32 s_link_packet[16];
static u8 s_matrix_at_rot[32];
static u8 s_matrix_at_scale[4][32];

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

static void check(const char *name, int ok)
{
    s_total++;
    if (ok) {
        s_pass++;
        printf("PASS %s\n", name);
    } else {
        fprintf(stderr, "ASSERTION %s\n", name);
        s_asserted = 1;
    }
}

MATRIX *RotMatrixZ(int angle, MATRIX *matrix)
{
    s_rot_calls++;
    s_rot_angle = angle;
    memcpy(s_matrix_at_rot, matrix, sizeof(s_matrix_at_rot));
    return matrix;
}

MATRIX *ScaleMatrix(MATRIX *matrix, VECTOR *scale)
{
    int slot = s_scale_calls;
    if (slot < 16)
        s_scale_address[slot] = guest_of(scale);
    if (slot < 4) {
        s_scales[slot] = *scale;
        memcpy(s_matrix_at_scale[slot], matrix,
               sizeof(s_matrix_at_scale[slot]));
    }
    s_scale_calls++;
    return matrix;
}

void SetRotMatrix(MATRIX *matrix)
{
    if (s_set_rot_calls < 32)
        s_set_rot_address[s_set_rot_calls] = guest_of(matrix);
    s_set_rot_calls++;
}

void SetTransMatrix(MATRIX *matrix)
{
    if (s_set_trans_calls < 16) {
        s_set_trans_address[s_set_trans_calls] = guest_of(matrix);
        s_set_trans_t[s_set_trans_calls][0] = matrix->t[0];
        s_set_trans_t[s_set_trans_calls][1] = matrix->t[1];
        s_set_trans_t[s_set_trans_calls][2] = matrix->t[2];
    }
    s_set_trans_calls++;
}

VECTOR *ApplyRotMatrix(SVECTOR *input, VECTOR *output)
{
    int slot = s_apply_calls;
    if (slot < 16)
        s_apply_input[slot] = *input;
    output->vx = 100 + slot;
    output->vy = 200 + slot;
    output->vz = 300 + slot;
    output->pad = 0;
    s_apply_calls++;
    return output;
}

static u32 pack_xy(s16 x, s16 y)
{
    return (u32)(u16)x | ((u32)(u16)y << 16);
}

int RotTransPers4(SVECTOR *v0, SVECTOR *v1, SVECTOR *v2, SVECTOR *v3,
                  long *xy0, long *xy1, long *xy2, long *xy3,
                  long *p, long *flag)
{
    int slot = s_projection_calls;
    (void)v1;
    (void)v2;
    (void)v3;
    if (slot < 16)
        memcpy(&s_projection_vertex_word[slot], v0,
               sizeof(s_projection_vertex_word[slot]));
    *p = 0;
    *flag = 0;
    if (s_tail_scenario == 0) {
        *xy0 = (long)pack_xy(10, 10);
        *xy1 = (long)pack_xy(20, 10);
        *xy2 = (long)pack_xy(20, 20);
        *xy3 = (long)pack_xy(10, 20);
        *flag = (long)(s32)UINT32_C(0x80000000);
        C2_SZ3 = 0x0100u;
    } else if (slot == 0) {
        *xy0 = (long)pack_xy(-10, 300);
        *xy1 = (long)pack_xy(400, 100);
        *xy2 = (long)pack_xy(410, 110);
        *xy3 = (long)pack_xy(420, 120);
        C2_SZ3 = 0x0120u;
    } else if (slot == 1) {
        *xy0 = (long)pack_xy(20, 20);
        *xy1 = (long)pack_xy(30, 20);
        *xy2 = (long)pack_xy(30, 30);
        *xy3 = (long)pack_xy(20, 30);
        *flag = (long)(s32)UINT32_C(0x80000000);
        C2_SZ3 = 0x0130u;
    } else if (slot == 2) {
        *xy0 = (long)pack_xy(400, 20);
        *xy1 = (long)pack_xy(410, 20);
        *xy2 = (long)pack_xy(410, 30);
        *xy3 = (long)pack_xy(400, 30);
        C2_SZ3 = 0x0140u;
    } else if (slot == 3) {
        *xy0 = (long)pack_xy(20, 20);
        *xy1 = (long)pack_xy(30, 20);
        *xy2 = (long)pack_xy(30, 30);
        *xy3 = (long)pack_xy(20, 30);
        C2_SZ3 = 0x0C00u;
    } else {
        *xy0 = (long)pack_xy(100, 80);
        *xy1 = (long)pack_xy(120, 80);
        *xy2 = (long)pack_xy(120, 100);
        *xy3 = (long)pack_xy(100, 100);
        C2_SZ3 = 0x0230u;
    }
    s_projection_calls++;
    return 0;
}

void PcPort_AddPrimDomainAware(void *ot, void *prim)
{
    u32 old_tag = *(u32*)ot;
    u32 packet_guest = guest_of(prim);
    if (s_link_calls < 16) {
        s_link_ot[s_link_calls] = guest_of(ot);
        s_link_packet[s_link_calls] = packet_guest;
    }
    *(u32*)prim = (*(u32*)prim & 0xFF000000u) |
                  (old_tag & 0x00FFFFFFu);
    *(u32*)ot = (old_tag & 0xFF000000u) |
                (packet_guest & 0x00FFFFFFu);
    s_link_calls++;
}

void wm_93534_test_load(u32 address, u32 value)
{
    (void)value;
    if (s_load_count < 32)
        s_load_addr[s_load_count] = address;
    s_load_count++;
}

void wm_93534_test_store(u32 address, u32 value)
{
    if (s_store_count < 16) {
        s_store_addr[s_store_count] = address;
        s_store_value[s_store_count] = value;
    }
    s_store_count++;
}

static void init_record(u32 index, u16 state, s16 heading, u8 flags,
                        u16 scale_x, u16 scale_y,
                        s32 x, s32 y, s32 z)
{
    u32 record = POOL + index * STRIDE;
    st16(record + 2u, (u16)heading);
    st16(record + 6u, state);
    st32(record + 8u, (u32)x);
    st32(record + 0x0Cu, (u32)y);
    st32(record + 0x10u, (u32)z);
    st16(record + 0x38u, scale_x);
    st16(record + 0x3Au, scale_y);
    st8(record + 0x47u, flags);
}

static void reset_fixture(void)
{
    u8 camera[32];
    u8 model[32];
    u32 wrong_record = FIXED_TABLE + 133u * STRIDE;
    u32 i;

    memset(g_PsxRam, 0, PSX_RAM_SIZE);
    memset(g_PsxScratchpad, 0, 4096u);
    memset(s_load_addr, 0, sizeof(s_load_addr));
    memset(s_store_addr, 0, sizeof(s_store_addr));
    memset(s_store_value, 0, sizeof(s_store_value));
    memset(s_scales, 0, sizeof(s_scales));
    memset(s_scale_address, 0, sizeof(s_scale_address));
    memset(s_set_rot_address, 0, sizeof(s_set_rot_address));
    memset(s_set_trans_address, 0, sizeof(s_set_trans_address));
    memset(s_set_trans_t, 0, sizeof(s_set_trans_t));
    memset(s_apply_input, 0, sizeof(s_apply_input));
    memset(s_projection_vertex_word, 0, sizeof(s_projection_vertex_word));
    memset(s_link_ot, 0, sizeof(s_link_ot));
    memset(s_link_packet, 0, sizeof(s_link_packet));
    memset(&gteRegs, 0, sizeof(gteRegs));
    memset(s_matrix_at_rot, 0, sizeof(s_matrix_at_rot));
    memset(s_matrix_at_scale, 0, sizeof(s_matrix_at_scale));
    s_rot_calls = 0;
    s_scale_calls = 0;
    s_load_count = 0;
    s_store_count = 0;
    s_asserted = 0;
    s_rot_angle = 0;
    s_set_rot_calls = 0;
    s_set_trans_calls = 0;
    s_apply_calls = 0;
    s_projection_calls = 0;
    s_link_calls = 0;
    s_tail_scenario = 0;

    for (i = 0u; i < 32u; i++) {
        camera[i] = (u8)(0x40u + i);
        model[i] = (u8)(0x80u + i);
    }
    memcpy(PSX_ADDR(CAMERA_MATRIX), camera, sizeof(camera));
    memcpy(PSX_ADDR(MODEL_MATRIX), model, sizeof(model));
    st32(POOL_GLOBAL, POOL);
    st32(CAMERA_X, (u32)(1000 * 4096));
    st32(CAMERA_Z, (u32)(-500 * 4096));
    st32(WRAP_X, 10u); /* period 20480 */
    st32(WRAP_Z, 12u); /* period 24576 */
    st32(ALLOC_A, 0x801C3A7Cu);
    st32(ALLOC_B, 0x801BEA74u);
    st32(PACKET_ROOT, PACKETS);
    st32(BUFFER_INDEX, 0u);
    st32(DRAW_RECORD_G, DRAW_RECORD);
    st32(DRAW_RECORD + 0x70u, OT_BASE);

    /* Two live records and one deliberately tempting inactive record. */
    init_record(7u, 3u, (s16)0x0123, 1u, 0x1111u, 0x2222u,
                (s32)(20000 * 4096), (s32)(7 * 4096),
                (s32)(-20000 * 4096));
    init_record(8u, 0u, (s16)0x0555, 1u, 0x3333u, 0x4444u,
                (s32)(25000 * 4096), (s32)(99 * 4096),
                (s32)(25000 * 4096));
    init_record(9u, 5u, (s16)0x0666, 0u, 0x5555u, 0x6666u,
                (s32)(-20000 * 4096), (s32)(-9 * 4096),
                (s32)(20000 * 4096));

    /* Old fixed-base record 133 is the exact alias witness:
     * wrong_record+0x30 == 0x8009D7EC. */
    st16(wrong_record + 6u, 1u);
}

static int address_sequence_is_scratch(void)
{
    int i;
    if (s_load_count != 8)
        return 0;
    for (i = 0; i < 8; i += 4) {
        if (s_load_addr[i] != SCRATCH + 0x88u ||
                s_load_addr[i + 1] != WRAP_X ||
                s_load_addr[i + 2] != SCRATCH + 0x90u ||
                s_load_addr[i + 3] != WRAP_Z)
            return 0;
    }
    return 1;
}

static void run_certificate(void)
{
    u8 pool_before[COUNT * STRIDE];
    u8 camera[32];
    u8 model[32];
    u8 scratch_camera[32];
    u8 scratch_model[32];

    reset_fixture();
    memcpy(pool_before, PSX_ADDR(POOL), sizeof(pool_before));
    memcpy(camera, PSX_ADDR(CAMERA_MATRIX), sizeof(camera));
    memcpy(model, PSX_ADDR(MODEL_MATRIX), sizeof(model));

    wm_80089C78(0xDEADBEEFu);

    memcpy(scratch_camera, PSX_ADDR(SCRATCH + 0x28u),
           sizeof(scratch_camera));
    memcpy(scratch_model, PSX_ADDR(SCRATCH + 0x68u),
           sizeof(scratch_model));
    check("camera-matrix-copied-to-retail-scratch",
          memcmp(camera, scratch_camera, sizeof(camera)) == 0);
    check("model-matrix-copied-to-retail-scratch",
          memcmp(model, scratch_model, sizeof(model)) == 0);
    check("dynamic-BDF4-pool-and-nonzero-state-guard",
          s_scale_calls == 2 && s_rot_calls == 1);
    check("rotation-angle-and-order",
          s_rot_angle == 0x0123 &&
          memcmp(s_matrix_at_rot, model, sizeof(model)) == 0 &&
          memcmp(s_matrix_at_scale[0], model, sizeof(model)) == 0 &&
          memcmp(s_matrix_at_scale[1], model, sizeof(model)) == 0);
    check("scale-vector-is-retail-32-bit-layout",
          s_scales[0].vx == 0x1111 && s_scales[0].vy == 0x2222 &&
          s_scales[0].vz == 0x1000 && s_scales[0].pad == 0 &&
          s_scales[1].vx == 0x5555 && s_scales[1].vy == 0x6666 &&
          s_scales[1].vz == 0x1000 && s_scales[1].pad == 0);
    check("scale-vector-resides-in-retail-guest-scratch",
          s_scale_address[0] == guest_of(PSX_ADDR(SCRATCH + 0x98u)) &&
          s_scale_address[1] == guest_of(PSX_ADDR(SCRATCH + 0x98u)));
    check("wrap-loads-only-retail-scratch-vector",
          address_sequence_is_scratch());
    check("camera-relative-fields-and-wrap-results",
          s_store_count == 4 &&
          s_store_addr[0] == SCRATCH + 0x88u &&
          s_store_value[0] == (u32)-1480 &&
          s_store_addr[1] == SCRATCH + 0x90u &&
          s_store_value[1] == (u32)5076 &&
          s_store_addr[2] == SCRATCH + 0x88u &&
          s_store_value[2] == (u32)-520 &&
          s_store_addr[3] == SCRATCH + 0x90u &&
          s_store_value[3] == (u32)-4076);
    check("wrapped-position-packed-for-camera-rotation",
          s_apply_calls == 2 &&
          s_apply_input[0].vx == -1480 && s_apply_input[0].vy == 7 &&
          s_apply_input[0].vz == -5076 &&
          s_apply_input[1].vx == -520 && s_apply_input[1].vy == -9 &&
          s_apply_input[1].vz == 4076);
    check("object-pool-is-read-only",
          memcmp(pool_before, PSX_ADDR(POOL), sizeof(pool_before)) == 0);
    check("allocator-globals-not-aliased",
          ld32(ALLOC_A) == 0x801C3A7Cu &&
          ld32(ALLOC_B) == 0x801BEA74u);
}

static void reset_tail_fixture(void)
{
    static const s16 model_indices[5] = {-1, 2, 3, 4, 5};
    u32 record;
    s16 model;
    u32 model_offset;
    u32 i;

    reset_fixture();
    memset(PSX_ADDR(POOL), 0, COUNT * STRIDE);
    memset(PSX_ADDR(PACKETS), 0, 8u * 0x28u);
    memset(PSX_ADDR(OT_BASE), 0, 0x400u);
    st32(CAMERA_X, (u32)(10 * 4096));
    st32(CAMERA_Z, (u32)(-5 * 4096));
    st32(CAMERA_MATRIX + 0x14u, 1000u);
    st32(CAMERA_MATRIX + 0x18u, 2000u);
    st32(CAMERA_MATRIX + 0x1Cu, 3000u);
    st32(PACKETS + 0u, 0x09000011u);
    st32(PACKETS + 0x28u, 0x09000022u);
    st32(OT_BASE + (0x0120u >> 4) * 4u, 0xAA00ABCDu);
    st32(OT_BASE + (0x0230u >> 4) * 4u, 0xBB001234u);

    for (i = 0u; i < 5u; i++) {
        model = model_indices[i];
        init_record(20u + i, (u16)model, 0, 0u,
                    (u16)(0x1000u + i), (u16)(0x1100u + i),
                    (s32)((100 + (s32)i) * 4096),
                    (s32)((20 + (s32)i) * 4096),
                    (s32)((-30 + (s32)i) * 4096));
        record = POOL + (20u + i) * STRIDE;
        st8(record + 0x40u, (u8)(0x20u + i));
        st8(record + 0x41u, (u8)(0x30u + i));
        st8(record + 0x42u, (u8)(0x40u + i));
        st16(record + 0x48u, (u16)(0x5000u + i));
        model_offset = (u32)((s32)model * 0x20);
        st32(FIXED_TABLE + model_offset,
             UINT32_C(0xA0000000) | (u32)(u16)model);
        model_offset = (u32)((s32)model * 8);
        st16(UV_TABLE + model_offset + 0u, (u16)(0x1000 + model));
        st16(UV_TABLE + model_offset + 2u, (u16)(0x2000 + model));
        st16(UV_TABLE + model_offset + 4u, (u16)(0x3000 + model));
        st16(UV_TABLE + model_offset + 6u, (u16)(0x4000 + model));
    }
    s_tail_scenario = 1;
}

static void run_tail_certificate(void)
{
    u8 pool_before[COUNT * STRIDE];
    u32 packet0 = PACKETS;
    u32 packet1 = PACKETS + 0x28u;

    reset_tail_fixture();
    memcpy(pool_before, PSX_ADDR(POOL), sizeof(pool_before));
    wm_80089C78(0u);

    check("tail-projects-every-live-record",
          s_projection_calls == 5 && s_apply_calls == 5 &&
          s_set_rot_calls == 10 && s_set_trans_calls == 5);
    check("tail-loads-signed-model-vertex-records",
          s_projection_vertex_word[0] == UINT32_C(0xA000FFFF) &&
          s_projection_vertex_word[4] == UINT32_C(0xA0000005));
    check("tail-camera-then-model-matrix-order",
          s_set_rot_address[0] == guest_of(PSX_ADDR(SCRATCH + 0x28u)) &&
          s_set_rot_address[1] == guest_of(PSX_ADDR(SCRATCH + 0x48u)) &&
          s_set_trans_address[0] == guest_of(PSX_ADDR(SCRATCH + 0x48u)));
    check("tail-camera-relative-translation-installed",
          s_apply_input[0].vx == 90 && s_apply_input[0].vy == 20 &&
          s_apply_input[0].vz == 25 &&
          s_set_trans_t[0][0] == 1100 &&
          s_set_trans_t[0][1] == 2200 &&
          s_set_trans_t[0][2] == 3300);
    check("tail-gates-flag-screen-depth-and-compacts",
          s_link_calls == 2 && s_link_packet[0] == packet0 &&
          s_link_packet[1] == packet1 &&
          s_link_ot[0] == OT_BASE + (0x0120u >> 4) * 4u &&
          s_link_ot[1] == OT_BASE + (0x0230u >> 4) * 4u);
    check("tail-first-packet-xy-and-color",
          ld32(packet0 + 0x08u) == pack_xy(-10, 300) &&
          ld32(packet0 + 0x10u) == pack_xy(400, 100) &&
          ld32(packet0 + 0x18u) == pack_xy(410, 110) &&
          ld32(packet0 + 0x20u) == pack_xy(420, 120) &&
          *(u8*)PSX_ADDR(packet0 + 4u) == 0x20u &&
          *(u8*)PSX_ADDR(packet0 + 5u) == 0x30u &&
          *(u8*)PSX_ADDR(packet0 + 6u) == 0x40u);
    check("tail-first-packet-uv-and-tpage",
          *(u16*)PSX_ADDR(packet0 + 0x0Cu) == 0x0FFFu &&
          *(u16*)PSX_ADDR(packet0 + 0x14u) == 0x1FFFu &&
          *(u16*)PSX_ADDR(packet0 + 0x1Cu) == 0x2FFFu &&
          *(u16*)PSX_ADDR(packet0 + 0x24u) == 0x3FFFu &&
          *(u16*)PSX_ADDR(packet0 + 0x16u) == 0x5000u);
    check("tail-guest-ot-links-preserve-high-bytes",
          ld32(packet0) == 0x0900ABCDu &&
          ld32(OT_BASE + (0x0120u >> 4) * 4u) ==
              (0xAA000000u | (packet0 & 0x00FFFFFFu)) &&
          ld32(packet1) == 0x09001234u &&
          ld32(OT_BASE + (0x0230u >> 4) * 4u) ==
              (0xBB000000u | (packet1 & 0x00FFFFFFu)));
    check("tail-object-pool-remains-read-only",
          memcmp(pool_before, PSX_ADDR(POOL), sizeof(pool_before)) == 0);
}

int main(void)
{
    run_certificate();
    run_tail_certificate();
    if (s_asserted != 0 || s_pass != s_total)
        return EXIT_FAILURE;
    printf("W34N37 0x80089C78 full-body certificate PASS\n");
    return EXIT_SUCCESS;
}
