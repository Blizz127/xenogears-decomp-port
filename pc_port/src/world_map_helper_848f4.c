/*
 * World-map region-model renderer 0x800848F4.
 *
 * Register-faithful structural transcription of retail world_map.bin
 * [0x800848F4, 0x80084D00), 0x40C bytes / 259 instructions.  The live C620
 * table contains 0x54-byte region records.  Active records are transformed
 * through an optional parent chain, wrapped relative to the camera, composed
 * with the camera matrix, projected, depth-gated, and dispatched through the
 * main model renderer.
 */
#include <stdint.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "psyq/libgte.h"
#include <psx/gtereg.h>
#include "world_map_helper_848f4.h"
#include "world_map_helper_93534.h"

#define WM_848F4_SCRATCH       0x1F800000u
#define WM_848F4_CAMERA        0x8009C808u
#define WM_848F4_CAMERA_X      0x8009BE28u
#define WM_848F4_CAMERA_Z      0x8009BE30u
#define WM_848F4_DRAW_RECORD   0x8009BE3Cu
#define WM_848F4_RECORD_ROOT   0x8009C620u
#define WM_848F4_RECORD_COUNT  0x8009D7E0u
#define WM_848F4_BUFFER_INDEX  0x8009D7F0u
#define WM_848F4_VARIANTS      0x8009AD2Cu

#define WM_848F4_SC_WRAP       (WM_848F4_SCRATCH + 0x000u)
#define WM_848F4_SC_SCALE      (WM_848F4_SCRATCH + 0x010u)
#define WM_848F4_SC_FLAG       (WM_848F4_SCRATCH + 0x020u)
#define WM_848F4_SC_DEPTH      (WM_848F4_SCRATCH + 0x028u)
#define WM_848F4_SC_ZERO       (WM_848F4_SCRATCH + 0x0A0u)
#define WM_848F4_SC_MATRIX     (WM_848F4_SCRATCH + 0x0F0u)
#define WM_848F4_SC_COMPOSED   (WM_848F4_SCRATCH + 0x110u)

#define WM_848F4_RECORD_STRIDE 0x54u

/* These executable-resident renderer globals are native authorities in the
 * PC link.  Retail addresses 0x800595C0/0x80059578 use a sign-extended
 * 0x95C0/0x9578 immediate after LUI 0x8006; they are not 0x800695xx guest
 * addresses. */
extern s32 D_80050104;
extern u32 D_800595C0;
extern s32 D_80059578;
extern s32 func_8002C700(u8 *model, u8 *buffer, u32 *ot, s32 variant);

static s16 wm_848f4_lh(u32 address)
{
    s16 value;

    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static u32 wm_848f4_lwu(u32 address)
{
    u32 value;

    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static s32 wm_848f4_lw(u32 address)
{
    s32 value;

    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static void wm_848f4_sh(u32 address, u16 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static void wm_848f4_sw(u32 address, u32 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static s32 wm_848f4_as_s32(u32 bits)
{
    s32 value;

    memcpy(&value, &bits, sizeof(value));
    return value;
}

static u32 wm_848f4_as_u32(s32 value)
{
    u32 bits;

    memcpy(&bits, &value, sizeof(bits));
    return bits;
}

static s32 wm_848f4_sra12(s32 value)
{
    u32 bits = wm_848f4_as_u32(value);

    bits >>= 12u;
    if (value < 0)
        bits |= UINT32_C(0xFFF00000);
    return wm_848f4_as_s32(bits);
}

static s32 wm_848f4_sub(s32 left, s32 right) __attribute__((unused));
static s32 wm_848f4_sub(s32 left, s32 right)
{
    return wm_848f4_as_s32(wm_848f4_as_u32(left) -
                           wm_848f4_as_u32(right));
}

static s32 wm_848f4_neg(s32 value)
{
    return wm_848f4_as_s32(0u - wm_848f4_as_u32(value));
}

static void wm_848f4_copy_record_matrix(u32 record)
{
    u32 word;
    u32 offset;

#if defined(WM_848F4_MUTANT_MATRIX_SOURCE_1C)
    record -= 4u;
#endif
    for (offset = 0u; offset < 0x20u; offset += 4u) {
        word = wm_848f4_lwu(record + 0x20u + offset);
        wm_848f4_sw(WM_848F4_SC_MATRIX + offset, word);
    }
}

static void wm_848f4_apply_parent_chain(u32 first_parent)
    __attribute__((unused));
static void wm_848f4_apply_parent_chain(u32 first_parent)
{
    MATRIX *current = (MATRIX *)PSX_ADDR(WM_848F4_SC_MATRIX);
    u32 parent_address = first_parent;

    while (parent_address != 0u) {
        MATRIX *parent = (MATRIX *)PSX_ADDR(parent_address + 0x20u);
        SVECTOR input;
        VECTOR output;
        long dead_flag = 0;

        /* Retail publishes each parent's live position into its own MATRIX
         * translation before using that transform. */
        parent->t[0] = wm_848f4_lw(parent_address + 0x08u);
        parent->t[1] = wm_848f4_lw(parent_address + 0x0Cu);
        parent->t[2] = wm_848f4_neg(wm_848f4_lw(parent_address + 0x10u));

#if !defined(WM_848F4_MUTANT_SKIP_PARENT_ROTATION)
        MATRIX product;

        (void)MulMatrix0(parent, current, &product);
        memcpy(current->m, product.m, sizeof(current->m));
#endif

        /* The retail MVMVA loads the low halfword of each current 32-bit
         * translation component as its SVECTOR input.  MulMatrix0 leaves the
         * parent rotation live in the GTE; only T needs publishing here. */
        input.vx = (s16)current->t[0];
        input.vy = (s16)current->t[1];
        input.vz = (s16)current->t[2];
        input.pad = 0;
        SetTransMatrix(parent);
        RotTrans(&input, &output, &dead_flag);
        current->t[0] = output.vx;
        current->t[1] = output.vy;
        current->t[2] = output.vz;

        parent_address = wm_848f4_lwu(parent_address + 0x50u);
    }
}

static void wm_848f4_prepare_transform(u32 record, s32 camera_x,
                                        s32 camera_z)
{
    MATRIX *current = (MATRIX *)PSX_ADDR(WM_848F4_SC_MATRIX);
    MATRIX *camera = (MATRIX *)PSX_ADDR(WM_848F4_CAMERA);
    MATRIX *composed = (MATRIX *)PSX_ADDR(WM_848F4_SC_COMPOSED);
    VECTOR *scale = (VECTOR *)PSX_ADDR(WM_848F4_SC_SCALE);
    s32 wrapped_x;
    s32 wrapped_z;

    wm_848f4_copy_record_matrix(record);
    current->t[0] = wm_848f4_lw(record + 0x08u);
    current->t[1] = wm_848f4_lw(record + 0x0Cu);
    current->t[2] = wm_848f4_neg(wm_848f4_lw(record + 0x10u));

#if !defined(WM_848F4_MUTANT_SKIP_PARENT_CHAIN)
    wm_848f4_apply_parent_chain(wm_848f4_lwu(record + 0x50u));
#endif

#if defined(WM_848F4_MUTANT_NO_CAMERA_SUBTRACT)
    (void)camera_x;
    (void)camera_z;
    wrapped_x = current->t[0];
    wrapped_z = wm_848f4_neg(current->t[2]);
#else
    wrapped_x = wm_848f4_sub(current->t[0], camera_x);
    wrapped_z = wm_848f4_sub(wm_848f4_neg(current->t[2]), camera_z);
#endif
    wm_848f4_sw(WM_848F4_SC_WRAP + 0u, wm_848f4_as_u32(wrapped_x));
    wm_848f4_sw(WM_848F4_SC_WRAP + 8u, wm_848f4_as_u32(wrapped_z));
#if !defined(WM_848F4_MUTANT_SKIP_WRAP)
    wm_80093534(WM_848F4_SC_WRAP);
#endif
    current->t[0] = wm_848f4_lw(WM_848F4_SC_WRAP + 0u);
    current->t[2] = wm_848f4_neg(wm_848f4_lw(WM_848F4_SC_WRAP + 8u));

#if defined(WM_848F4_MUTANT_SCALE_1000)
    scale->vx = 0x1000;
    scale->vy = 0x1000;
    scale->vz = 0x1000;
#else
    scale->vx = 0x0800;
    scale->vy = 0x0800;
    scale->vz = 0x0800;
#endif
    (void)ScaleMatrix(current, scale);

#if defined(WM_848F4_MUTANT_REVERSE_COMPOSE)
    (void)CompMatrix(current, camera, composed);
#else
    (void)CompMatrix(camera, current, composed);
#endif
    SetRotMatrix(composed);
    SetTransMatrix(composed);
}

static int wm_848f4_project_origin(void)
{
    int xy = 0;
    long p = 0;
    long flag = 0;
    u32 flag_bits;
    u32 depth;

    (void)RotTransPers((SVECTOR *)PSX_ADDR(WM_848F4_SC_ZERO),
                       &xy, &p, &flag);
    flag_bits = (u32)(unsigned long)flag;
    wm_848f4_sw(WM_848F4_SC_FLAG, flag_bits);
#if !defined(WM_848F4_MUTANT_SKIP_FLAG_GATE)
    if ((flag_bits & UINT32_C(0x80000000)) != 0u)
        return 0;
#endif

    depth = (u32)(u16)C2_SZ3;
    wm_848f4_sw(WM_848F4_SC_DEPTH, depth);
#if defined(WM_848F4_MUTANT_DEPTH_LIMIT_1000)
    return depth < 0x1000u;
#else
    return depth < 0x0D80u;
#endif
}

static void wm_848f4_dispatch(u32 record_offset)
{
    u32 record = wm_848f4_lwu(WM_848F4_RECORD_ROOT) + record_offset;
    u32 buffer_index = wm_848f4_lwu(WM_848F4_BUFFER_INDEX);
    u32 model = wm_848f4_lwu(record + 0x40u);
    u32 buffer;
    u32 draw_record = wm_848f4_lwu(WM_848F4_DRAW_RECORD);
    u32 ot = wm_848f4_lwu(draw_record + 0x70u);
    s32 kind = (s32)wm_848f4_lh(record + 0x04u);
    s32 variant;

#if defined(WM_848F4_MUTANT_FIXED_BUFFER_ZERO)
    (void)buffer_index;
    buffer = wm_848f4_lwu(record + 0x48u);
#else
    buffer = wm_848f4_lwu(record + 0x48u + buffer_index * 4u);
#endif
#if defined(WM_848F4_MUTANT_VARIANT_UNSIGNED)
    variant = (s32)wm_848f4_lh(WM_848F4_VARIANTS +
                               (u32)(u16)kind * 2u);
#else
    variant = (s32)wm_848f4_lh(WM_848F4_VARIANTS +
                               (u32)kind * 2u);
#endif

#if !defined(WM_848F4_MUTANT_NO_MODEL_DISPATCH)
    (void)func_8002C700((u8 *)PSX_ADDR(model),
#if defined(WM_848F4_MUTANT_RAW_GUEST_BUFFER)
                        (u8 *)(uintptr_t)buffer,
#else
                        (u8 *)PSX_ADDR(buffer),
#endif
                        (u32 *)PSX_ADDR(ot), variant);
#else
    (void)model;
    (void)buffer;
    (void)ot;
    (void)variant;
#endif
}

void wm_800848F4(void)
{
    s32 camera_x;
    s32 camera_z;
    s32 index;
    s32 count;
    u32 record_offset;

    wm_848f4_sw(WM_848F4_SC_SCALE + 8u, 0x800u);
    wm_848f4_sw(WM_848F4_SC_SCALE + 4u, 0x800u);
    wm_848f4_sw(WM_848F4_SC_SCALE + 0u, 0x800u);
    D_80050104 = 3;
    camera_x = wm_848f4_sra12(wm_848f4_lw(WM_848F4_CAMERA_X));
    camera_z = wm_848f4_sra12(wm_848f4_lw(WM_848F4_CAMERA_Z));
    D_800595C0 = 0u;
    D_80059578 = 0;
    wm_848f4_sh(WM_848F4_SC_ZERO + 4u, 0u);
    wm_848f4_sh(WM_848F4_SC_ZERO + 2u, 0u);
    wm_848f4_sh(WM_848F4_SC_ZERO + 0u, 0u);

    count = (s32)wm_848f4_lh(WM_848F4_RECORD_COUNT);
    if (count <= 0)
        return;

    index = 0;
    record_offset = 0u;
    for (;;) {
        u32 record = wm_848f4_lwu(WM_848F4_RECORD_ROOT) + record_offset;

#if defined(WM_848F4_MUTANT_SKIP_STATE_GATE)
        if (1) {
#else
        if (wm_848f4_lh(record + 0u) == 0) {
#endif
            /* Retail reloads C620 before the matrix copy. */
            record = wm_848f4_lwu(WM_848F4_RECORD_ROOT) + record_offset;
            wm_848f4_prepare_transform(record, camera_x, camera_z);
            if (wm_848f4_project_origin())
                wm_848f4_dispatch(record_offset);
        }

        index++;
#if defined(WM_848F4_MUTANT_RECORD_STRIDE_04)
        record_offset += 4u;
#else
        record_offset += WM_848F4_RECORD_STRIDE;
#endif
        /* Retail reloads the signed count at the loop tail. */
        if (!(index < (s32)wm_848f4_lh(WM_848F4_RECORD_COUNT)))
            break;
    }
}
