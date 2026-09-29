/*
 * World-map scheduler callback 0x8008E190.
 *
 * Register-faithful, finite-width transcription of retail world_map.bin
 * [0x8008E190, 0x8008E4F4). The function initializes a scheduler slot from
 * the global world vector, runs the mode-specific terrain/position arm,
 * publishes the result into the already-live C620 transform context, builds
 * two adjacent record matrices, and returns the state selected by EE68.
 */
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "psyq/libgte.h"
#include "world_map_callback_8e190.h"
#include "world_map_helper_8dff4.h"
#include "world_map_helper_8e034.h"
#include "world_map_terrain_sampler.h"

#define WM_E190_POOL_PTR          0x8009BE24u
#define WM_E190_MODE              0x8009BE10u
#define WM_E190_HEADING           0x8006EE66u
#define WM_E190_VARIANT           0x8006EE68u
#define WM_E190_SHARED_HEADING    0x8009C584u
#define WM_E190_SHARED_TAIL_MODE  0x8009C5A8u
#define WM_E190_SHARED_X          0x8009C5ACu
#define WM_E190_SHARED_Y          0x8009C5B0u
#define WM_E190_SHARED_Z          0x8009C5B4u
#define WM_E190_SHARED_AUX        0x8009C170u
#define WM_E190_CONTEXT_PTR       0x8009C620u
#define WM_E190_MATRIX_Z          0x8009BD3Cu
#define WM_E190_POSITION_VECTOR   0x8009D55Cu
#define WM_E190_PUBLISHED_HEADING 0x8009D52Cu

#define WM_E190_SLOT_CONTROL       0x20u
#define WM_E190_SLOT_FLAG          0x24u
#define WM_E190_SLOT_X             0x28u
#define WM_E190_SLOT_Y             0x2Cu
#define WM_E190_SLOT_Z             0x30u
#define WM_E190_SLOT_W             0x34u
#define WM_E190_SLOT_CLEAR38       0x38u
#define WM_E190_SLOT_CLEAR3C       0x3Cu
#define WM_E190_SLOT_CLEAR40       0x40u
#define WM_E190_SLOT_HEADING       0x48u
#define WM_E190_SLOT_VARIANT_VALUE 0x4Au
#define WM_E190_SLOT_CLEAR60       0x60u
#define WM_E190_SLOT_CLEAR64       0x64u
#define WM_E190_SLOT_CONST68       0x68u
#define WM_E190_SLOT_CLEAR6C       0x6Cu
#define WM_E190_SLOT_ROTATION_X    0x70u
#define WM_E190_SLOT_AUX           0x74u
#define WM_E190_SLOT_CLEAR7C       0x7Cu

#define WM_E190_CONTEXT_KIND       0x00u
#define WM_E190_CONTEXT_X          0x08u
#define WM_E190_CONTEXT_Y          0x0Cu
#define WM_E190_CONTEXT_Z          0x10u
#define WM_E190_CONTEXT_W          0x14u
#define WM_E190_CONTEXT_MATRIX0    0x20u
#define WM_E190_CONTEXT_RECORD1    0x54u
#define WM_E190_CONTEXT_MATRIX1    0x74u
#define WM_E190_CONTEXT_RECORD2    0xA8u
#define WM_E190_CONTEXT_RECORD3    0xFCu

#define WM_E190_SCRATCH_VECTOR_OFFSET 0xA0u

static u16 wm_e190_load_u16(u32 address)
{
    u16 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static u32 wm_e190_load_u32(u32 address)
{
    u32 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static void wm_e190_store_u16(u32 address, u16 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static void wm_e190_store_u32(u32 address, u32 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static void wm_e190_store_scratch_u16(u32 offset, u16 value)
{
    memcpy(g_PsxScratchpad + offset, &value, sizeof(value));
}

static s32 wm_e190_u32_as_s32(u32 value)
{
    s32 result;
    memcpy(&result, &value, sizeof(result));
    return result;
}

static u32 wm_e190_s32_as_u32(s32 value)
{
    u32 result;
    memcpy(&result, &value, sizeof(result));
    return result;
}

/* MIPS SRA by 12, expressed with fully defined unsigned operations. */
static u32 wm_e190_sra12(u32 value)
{
    u32 result = value >> 12;
    if ((value & 0x80000000u) != 0u)
        result |= 0xFFF00000u;
    return result;
}

static void wm_e190_publish_position(u32 slot_addr)
{
    u32 x_bits = wm_e190_load_u32(slot_addr + WM_E190_SLOT_X);
    u32 y_bits = wm_e190_load_u32(slot_addr + WM_E190_SLOT_Y);
    u32 z_bits = wm_e190_load_u32(slot_addr + WM_E190_SLOT_Z);
    u32 w_bits;

    wm_e190_store_u32(WM_E190_POSITION_VECTOR + 0u, x_bits);
    wm_e190_store_u32(WM_E190_POSITION_VECTOR + 4u, y_bits);
    wm_e190_store_u32(WM_E190_POSITION_VECTOR + 8u, z_bits);
    w_bits = wm_e190_load_u32(slot_addr + WM_E190_SLOT_W);
    wm_e190_store_u32(WM_E190_POSITION_VECTOR + 12u, w_bits);
}

static void wm_e190_mode_4_or_5(u32 slot_addr)
{
    u32 x_bits;
    u32 z_bits;
    u32 terrain_bits;
    u32 heading_bits;
    u32 aux_bits;
    u32 context_addr;

    wm_e190_store_u16(slot_addr + WM_E190_SLOT_CONTROL, 3u);
    x_bits = wm_e190_load_u32(WM_E190_SHARED_X);
    wm_e190_store_u32(slot_addr + WM_E190_SLOT_X, x_bits);
    z_bits = wm_e190_load_u32(WM_E190_SHARED_Z);
    x_bits = wm_e190_load_u32(slot_addr + WM_E190_SLOT_X);

    /* Retail publishes Z in the terrain JAL delay slot. */
    wm_e190_store_u32(slot_addr + WM_E190_SLOT_Z, z_bits);
    terrain_bits = wm_e190_s32_as_u32(
        wm_80093978(wm_e190_u32_as_s32(x_bits),
                    wm_e190_u32_as_s32(z_bits)));

    heading_bits = wm_e190_load_u32(WM_E190_SHARED_HEADING);
    aux_bits = wm_e190_load_u32(WM_E190_SHARED_AUX);
    terrain_bits += 0xFFFC0000u;
    wm_e190_store_u32(slot_addr + WM_E190_SLOT_Y, terrain_bits);
    wm_e190_store_u16(slot_addr + WM_E190_SLOT_HEADING,
                      (u16)heading_bits);
    wm_e190_store_u32(slot_addr + WM_E190_SLOT_AUX, aux_bits);

    wm_e190_publish_position(slot_addr);

    heading_bits = wm_e190_load_u16(slot_addr + WM_E190_SLOT_HEADING);
    context_addr = wm_e190_load_u32(WM_E190_CONTEXT_PTR);
    wm_e190_store_u16(context_addr + WM_E190_CONTEXT_RECORD3, 1u);
    wm_e190_store_u16(context_addr + WM_E190_CONTEXT_RECORD2, 1u);
    wm_e190_store_u16(context_addr + WM_E190_CONTEXT_RECORD1, 1u);
    wm_e190_store_u16(WM_E190_PUBLISHED_HEADING, (u16)heading_bits);
}

static void wm_e190_mode_7(u32 slot_addr)
{
    u32 x_bits;
    u32 heading_bits;
    u32 z_bits;
    u32 y_bits;
    u32 aux_bits;

    wm_e190_store_u16(slot_addr + WM_E190_SLOT_CONTROL, 2u);
    x_bits = wm_e190_load_u32(WM_E190_SHARED_X);
    heading_bits = wm_e190_load_u32(WM_E190_SHARED_HEADING);
    wm_e190_store_u32(slot_addr + WM_E190_SLOT_X, x_bits);
    z_bits = wm_e190_load_u32(WM_E190_SHARED_Z);
    wm_e190_store_u32(slot_addr + WM_E190_SLOT_Z, z_bits);
    y_bits = wm_e190_load_u32(WM_E190_SHARED_Y);
    aux_bits = wm_e190_load_u32(WM_E190_SHARED_AUX);
    wm_e190_store_u16(slot_addr + WM_E190_SLOT_HEADING,
                      (u16)heading_bits);
    wm_e190_store_u32(slot_addr + WM_E190_SLOT_AUX, aux_bits);
    wm_e190_store_u32(slot_addr + WM_E190_SLOT_Y, y_bits);

    wm_e190_publish_position(slot_addr);
    heading_bits = wm_e190_load_u16(slot_addr + WM_E190_SLOT_HEADING);
    wm_e190_store_u16(WM_E190_PUBLISHED_HEADING, (u16)heading_bits);
}

static void wm_e190_common_context(u32 slot_addr)
{
    u32 context_addr;
    u32 x_bits;
    u32 y_bits;
    u32 z_bits;
    u32 w_bits;
    u32 rotation_bits;
    u16 heading;
    u16 matrix_z;
    SVECTOR* rotation;

    context_addr = wm_e190_load_u32(WM_E190_CONTEXT_PTR);
    x_bits = wm_e190_load_u32(slot_addr + WM_E190_SLOT_X);
    y_bits = wm_e190_load_u32(slot_addr + WM_E190_SLOT_Y);
    z_bits = wm_e190_load_u32(slot_addr + WM_E190_SLOT_Z);
    w_bits = wm_e190_load_u32(slot_addr + WM_E190_SLOT_W);
    wm_e190_store_u32(context_addr + WM_E190_CONTEXT_X, x_bits);
    wm_e190_store_u32(context_addr + WM_E190_CONTEXT_Y, y_bits);
    wm_e190_store_u32(context_addr + WM_E190_CONTEXT_Z, z_bits);
    wm_e190_store_u32(context_addr + WM_E190_CONTEXT_W, w_bits);

    /* Retail reloads C620 before publishing the record kind and retains this
     * particular root for the first matrix destination. */
    context_addr = wm_e190_load_u32(WM_E190_CONTEXT_PTR);
    wm_e190_store_u16(context_addr + WM_E190_CONTEXT_KIND,
                      wm_e190_load_u16(slot_addr + WM_E190_SLOT_FLAG));

    rotation_bits = wm_e190_load_u32(slot_addr + WM_E190_SLOT_ROTATION_X);
    rotation_bits = 0u - wm_e190_sra12(rotation_bits);
    wm_e190_store_scratch_u16(WM_E190_SCRATCH_VECTOR_OFFSET + 0u,
                              (u16)rotation_bits);
    heading = wm_e190_load_u16(slot_addr + WM_E190_SLOT_HEADING);
    matrix_z = wm_e190_load_u16(WM_E190_MATRIX_Z);

    wm_e190_store_scratch_u16(WM_E190_SCRATCH_VECTOR_OFFSET + 4u,
                              matrix_z);
    wm_e190_store_scratch_u16(WM_E190_SCRATCH_VECTOR_OFFSET + 2u,
                              heading);

    rotation = (SVECTOR*)(void*)(g_PsxScratchpad +
                                 WM_E190_SCRATCH_VECTOR_OFFSET);
    (void)RotMatrixYXZ(rotation,
                       (MATRIX*)PSX_ADDR(context_addr +
                                         WM_E190_CONTEXT_MATRIX0));

    /* The second retail call freshly reloads the root in case the first
     * matrix helper changed global state. */
    context_addr = wm_e190_load_u32(WM_E190_CONTEXT_PTR);
    (void)RotMatrixYXZ(rotation,
                       (MATRIX*)PSX_ADDR(context_addr +
                                         WM_E190_CONTEXT_MATRIX1));

    wm_8008E034(slot_addr + WM_E190_SLOT_X);
}

s32 wm_8008E190(s32 slot_index)
{
    u32 slot_offset = (u32)slot_index << 7;
    u32 slot_addr = wm_e190_load_u32(WM_E190_POOL_PTR) + slot_offset;
    u32 variant;
    s32 mode;
    s32 return_state;
    s32 tail_mode;
    u32 x_bits;
    u32 z_bits;
    u32 terrain_bits;
    u16 heading;

    /* Retail JAL delay slot clears +0x24 before DFF4 starts. */
    wm_e190_store_u16(slot_addr + WM_E190_SLOT_FLAG, 0u);
    wm_8008DFF4(slot_addr + WM_E190_SLOT_X);

    return_state = 1;
    heading = wm_e190_load_u16(WM_E190_HEADING);
    wm_e190_store_u32(slot_addr + WM_E190_SLOT_CLEAR40, 0u);
    wm_e190_store_u32(slot_addr + WM_E190_SLOT_CLEAR3C, 0u);
    wm_e190_store_u32(slot_addr + WM_E190_SLOT_CLEAR38, 0u);
    wm_e190_store_u32(slot_addr + WM_E190_SLOT_CLEAR64, 0u);
    wm_e190_store_u32(slot_addr + WM_E190_SLOT_CLEAR60, 0u);
    wm_e190_store_u32(slot_addr + WM_E190_SLOT_AUX, 0u);
    wm_e190_store_u32(slot_addr + WM_E190_SLOT_CONST68, 0xFFD80000u);
    wm_e190_store_u32(slot_addr + WM_E190_SLOT_ROTATION_X, 0u);
    wm_e190_store_u32(slot_addr + WM_E190_SLOT_CLEAR6C, 0u);
    wm_e190_store_u16(slot_addr + WM_E190_SLOT_HEADING, heading);

    variant = (u32)wm_e190_load_u16(WM_E190_VARIANT) & 0x1FFFu;
    switch (variant) {
    case 0u:
        return_state = 3;
        break;
    case 1u:
    case 2u:
        wm_e190_store_u16(slot_addr + WM_E190_SLOT_VARIANT_VALUE, 12u);
        break;
    case 3u:
    case 4u:
        wm_e190_store_u16(slot_addr + WM_E190_SLOT_VARIANT_VALUE, 32u);
        break;
    default:
        break;
    }

    mode = wm_e190_u32_as_s32(wm_e190_load_u32(WM_E190_MODE));
    switch (mode) {
    case 1:
        x_bits = wm_e190_load_u32(slot_addr + WM_E190_SLOT_X);
        z_bits = wm_e190_load_u32(slot_addr + WM_E190_SLOT_Z);
        terrain_bits = wm_e190_s32_as_u32(
            wm_80093978(wm_e190_u32_as_s32(x_bits),
                        wm_e190_u32_as_s32(z_bits)));
        wm_e190_store_u32(slot_addr + WM_E190_SLOT_Y, terrain_bits);
        break;
    case 2:
    case 3:
        x_bits = wm_e190_load_u32(slot_addr + WM_E190_SLOT_X);
        z_bits = wm_e190_load_u32(slot_addr + WM_E190_SLOT_Z);
        wm_e190_store_u16(slot_addr + WM_E190_SLOT_CONTROL, 1u);
        terrain_bits = wm_e190_s32_as_u32(
            wm_80093978(wm_e190_u32_as_s32(x_bits),
                        wm_e190_u32_as_s32(z_bits)));
        wm_e190_store_u32(slot_addr + WM_E190_SLOT_Y, terrain_bits);
        break;
    case 4:
    case 5:
        wm_e190_mode_4_or_5(slot_addr);
        break;
    case 7:
        wm_e190_mode_7(slot_addr);
        break;
    default:
        break;
    }

    wm_e190_common_context(slot_addr);

    heading = wm_e190_load_u16(slot_addr + WM_E190_SLOT_HEADING);
    tail_mode = wm_e190_u32_as_s32(
        wm_e190_load_u32(WM_E190_SHARED_TAIL_MODE));
    wm_e190_store_u16(WM_E190_HEADING, heading);

    if (tail_mode == 3) {
        wm_e190_store_u16(slot_addr + WM_E190_SLOT_CONTROL, 40u);
        wm_e190_store_u32(slot_addr + WM_E190_SLOT_CLEAR7C, 0u);
    } else if (tail_mode < 4) {
        if (tail_mode == 2)
            wm_e190_store_u16(slot_addr + WM_E190_SLOT_CONTROL, 36u);
    } else if (tail_mode == 4) {
        wm_e190_store_u16(slot_addr + WM_E190_SLOT_CONTROL, 48u);
        wm_e190_store_u32(slot_addr + WM_E190_SLOT_CLEAR7C, 0u);
    } else if (tail_mode == 5) {
        wm_e190_store_u16(slot_addr + WM_E190_SLOT_CONTROL, 52u);
        wm_e190_store_u32(slot_addr + WM_E190_SLOT_CLEAR7C, 0u);
    }

    return return_state;
}
