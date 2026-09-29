/*
 * World-map terrain triangle normal helper 0x80093740.
 *
 * Retail boundary: [0x80093740, 0x80093978), 0x238 bytes / 142 instructions.
 * This is world-overlay code and is unrelated to the field-overlay symbol
 * func_80093740.
 */
#include "psx_memory.h"
#include "world_map_terrain_cell.h"
#include "world_map_terrain_normal.h"

#include <libgte.h>

#include <stdint.h>
#include <string.h>

#define WM_COEFF_0_ABS 0x8009B244u
#define WM_COEFF_1_ABS 0x8009B24Cu
#define WM_COEFF_2_ABS 0x8009B254u
#define WM_COEFF_3_ABS 0x8009B25Cu

/* Main-executable PsyQ/GTE dependencies, provided by psyq_compat.c. */
extern void OuterProduct0(VECTOR *v0, VECTOR *v1, VECTOR *out);
extern long VectorNormal(VECTOR *input, VECTOR *output);

static s32 wm_bits_to_s32(u32 bits)
{
    s32 value;
    memcpy(&value, &bits, sizeof(value));
    return value;
}

static u32 wm_load_u32(u32 addr)
{
    u32 value;
    memcpy(&value, PSX_ADDR(addr), sizeof(value));
    return value;
}

static int8_t wm_load_s8(u32 addr)
{
    int8_t value;
    memcpy(&value, PSX_ADDR(addr), sizeof(value));
    return value;
}

/* Signed MULT followed by MFLO, with the low-word boundary kept explicit. */
static u32 wm_mult_lo(u32 lhs_bits, u32 rhs_bits)
{
    int64_t product = (int64_t)wm_bits_to_s32(lhs_bits) *
                      (int64_t)wm_bits_to_s32(rhs_bits);
    return (u32)(uint64_t)product;
}

static void wm_scratch_store_s32(u32 offset, s32 value)
{
    memcpy(g_PsxScratchpad + offset, &value, sizeof(value));
}

static void wm_scratch_store_vector(u32 offset, const VECTOR *value)
{
    wm_scratch_store_s32(offset + 0u, (s32)value->vx);
    wm_scratch_store_s32(offset + 4u, (s32)value->vy);
    wm_scratch_store_s32(offset + 8u, (s32)value->vz);
}

static void *wm_guest_pointer(u32 addr)
{
    /* Retail callers use the physical scratchpad address 0x1F800030. */
    if ((addr & UINT32_C(0xFFFFFC00)) == UINT32_C(0x1F800000))
        return g_PsxScratchpad + (addr & UINT32_C(0x3FF));
    return PSX_ADDR(addr);
}

static void wm_guest_store_vector(u32 addr, const VECTOR *value)
{
    uint8_t *dst = (uint8_t *)wm_guest_pointer(addr);
    s32 component;

    component = (s32)value->vx;
    memcpy(dst + 0, &component, sizeof(component));
    component = (s32)value->vy;
    memcpy(dst + 4, &component, sizeof(component));
    component = (s32)value->vz;
    memcpy(dst + 8, &component, sizeof(component));
}

s32 wm_80093740(u32 out_normal_addr, s32 x, s32 z)
{
    u32 cell_addr = wm_80093660(x, z);
    u32 local_x = (u32)(x / 8) & UINT32_C(0xFFFF);
    u32 local_z = (u32)(z / 8) & UINT32_C(0xFFFF);
    u32 selection_bits;
    VECTOR edge0 = { 0, 0, 0, 0 };
    VECTOR edge1 = { 0, 0, 0, 0 };
    VECTOR cross = { 0, 0, 0, 0 };
    VECTOR normal = { 0, 0, 0, 0 };
    long normal_result;

    if ((*(u8 *)PSX_ADDR(cell_addr + 1u) & 0x80u) != 0u) {
        u32 p0 = wm_mult_lo(local_x, wm_load_u32(WM_COEFF_2_ABS));
        u32 p1 = wm_mult_lo(0u - local_z, wm_load_u32(WM_COEFF_3_ABS));
        selection_bits = p0 + p1; /* ADDU, modulo 2^32 */

        if (wm_bits_to_s32(selection_bits) < 0) {
            /* Triangle h00/h01/h11. */
            edge0.vx = 16;
            edge0.vy = (s32)wm_load_s8(cell_addr + 0x28u) -
                       (s32)wm_load_s8(cell_addr + 0x00u);
            edge0.vz = -16;
            edge1.vx = 0;
            edge1.vy = (s32)wm_load_s8(cell_addr + 0x24u) -
                       (s32)wm_load_s8(cell_addr + 0x00u);
            edge1.vz = -16;
        } else {
            /* Triangle h00/h11/h10. */
            edge0.vx = 16;
            edge0.vy = (s32)wm_load_s8(cell_addr + 0x04u) -
                       (s32)wm_load_s8(cell_addr + 0x00u);
            edge0.vz = 0;
            edge1.vx = 16;
            edge1.vy = (s32)wm_load_s8(cell_addr + 0x28u) -
                       (s32)wm_load_s8(cell_addr + 0x00u);
            edge1.vz = -16;
        }
    } else {
        u32 local_x_from_right = local_x + UINT32_C(0xFFFF0000);
        u32 p0 = wm_mult_lo(local_x_from_right,
                            wm_load_u32(WM_COEFF_0_ABS));
        u32 p1 = wm_mult_lo(0u - local_z,
                            wm_load_u32(WM_COEFF_1_ABS));
        selection_bits = p0 + p1; /* ADDU, modulo 2^32 */

        if (wm_bits_to_s32(selection_bits) < 0) {
            /* Triangle h10/h00/h01. */
            edge0.vx = -16;
            edge0.vy = (s32)wm_load_s8(cell_addr + 0x24u) -
                       (s32)wm_load_s8(cell_addr + 0x04u);
            edge0.vz = -16;
            edge1.vx = -16;
            edge1.vy = (s32)wm_load_s8(cell_addr + 0x00u) -
                       (s32)wm_load_s8(cell_addr + 0x04u);
            edge1.vz = 0;
        } else {
            /* Triangle h10/h11/h01. */
            edge0.vx = 0;
            edge0.vy = (s32)wm_load_s8(cell_addr + 0x28u) -
                       (s32)wm_load_s8(cell_addr + 0x04u);
            edge0.vz = -16;
            edge1.vx = -16;
            edge1.vy = (s32)wm_load_s8(cell_addr + 0x24u) -
                       (s32)wm_load_s8(cell_addr + 0x04u);
            edge1.vz = -16;
        }
    }

    /* Retail writes only vx/vy/vz; VECTOR pad words remain untouched. */
    wm_scratch_store_vector(0x00u, &edge0);
    wm_scratch_store_vector(0x10u, &edge1);

    /* Retail order and operand winding: edge1 x edge0. */
    OuterProduct0(&edge1, &edge0, &cross);
    wm_scratch_store_vector(0x20u, &cross);

    normal_result = VectorNormal(&cross, &normal);
    wm_guest_store_vector(out_normal_addr, &normal);
    return (s32)normal_result;
}
