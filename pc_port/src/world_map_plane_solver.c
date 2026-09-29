/*
 * W34B5-N: World-map terrain plane-Y solver 0x800935DC.
 *
 * Exact transcription of retail 0x800935DC–0x8009365C.
 * Leaf function: no calls, no external dependencies.
 *
 * Solves the terrain plane equation for the Y (height) coordinate
 * at a given X/Z point, given plane coefficients, base point, and
 * surface normal.
 *
 * Inputs (as guest-memory 12-byte s32 records):
 *   a0 = coefficients: {A[0], result_slot, A[2]}
 *   a1 = base point:    {B[0], B[1], B[2]}
 *   a2 = surface normal: {N[0], N[1], N[2]}
 *
 * Operation (finite-width R3000A semantics):
 *   dx_bits  = A[0] - B[0]          (SUBU, modulo 2^32)
 *   p0_bits  = LOW32(N[0] * dx)     (MULT → MFLO)
 *   dz_bits  = A[2] - B[2]          (SUBU, modulo 2^32)
 *   p1_bits  = LOW32(N[2] * dz)     (MULT → MFLO)
 *   num_bits = 0 - p0 - p1          (SUBU, modulo 2^32)
 *   quotient = num / N[1]           (signed DIV, truncation toward zero)
 *   a0[1]    = quotient             (first store)
 *   result   = quotient + B[1]      (ADDU, modulo 2^32)
 *   a0[1]    = result               (second store, jr delay slot)
 *   return   = result
 *
 * Retail BREAK traps (halt CPU, no stores, no normal return):
 *   break 0x1C00: N[1] == 0
 *   break 0x1800: N[1] == -1 && numerator == INT32_MIN
 *
 * Alias rule: if a0 == a1, the load of B[1] after store #1 observes
 * the quotient just written.  Two stores are NOT collapsed.
 */
#include "psx_memory.h"
#include "world_map_plane_solver.h"

#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <limits.h>

/*
 * Reinterpret 32-bit unsigned pattern as signed 32-bit integer.
 * Avoids implementation-defined u32→s32 conversion for values > INT32_MAX.
 */
static s32 bits_to_s32(u32 bits)
{
    s32 value;
    memcpy(&value, &bits, sizeof(value));
    return value;
}

u32 wm_800935DC(u32 a0, u32 a1, u32 a2)
{
    /* ---- Load operands from guest memory ---- */

    u32 a0_0 = *(u32 *)PSX_ADDR(a0);       /* A[0] */
    u32 a1_0 = *(u32 *)PSX_ADDR(a1);       /* B[0] */
    u32 a2_0 = *(u32 *)PSX_ADDR(a2);       /* N[0] */

    /* SUBU: dx_bits = A[0] - B[0] (modulo 2^32) */
    u32 dx_bits = a0_0 - a1_0;
    s32 dx = bits_to_s32(dx_bits);
    s32 nx = bits_to_s32(a2_0);

    /* MULT → MFLO: p0 = LOW32(N[0] * dx) */
    s64 product_x = (s64)nx * (s64)dx;
    u32 p0_bits = (u32)product_x;

    u32 a0_2 = *(u32 *)PSX_ADDR(a0 + 8);   /* A[2] */
    u32 a1_2 = *(u32 *)PSX_ADDR(a1 + 8);   /* B[2] */
    u32 a2_2 = *(u32 *)PSX_ADDR(a2 + 8);   /* N[2] */

    /* SUBU: dz_bits = A[2] - B[2] (modulo 2^32) */
    u32 dz_bits = a0_2 - a1_2;
    s32 dz = bits_to_s32(dz_bits);
    s32 nz = bits_to_s32(a2_2);

    /* MULT → MFLO: p1 = LOW32(N[2] * dz) */
    s64 product_z = (s64)nz * (s64)dz;
    u32 p1_bits = (u32)product_z;

    /* SUBU: numerator_bits = 0 - p0_bits - p1_bits (modulo 2^32) */
    u32 numerator_bits = 0u - p0_bits - p1_bits;
    s32 numerator = bits_to_s32(numerator_bits);

    /* ---- Load N[1] ---- */
    s32 ny = bits_to_s32(*(u32 *)PSX_ADDR(a2 + 4));

    /* ---- Retail DIV BREAK guards ---- */

    if (ny == 0) {
        /* Retail BREAK 0x1C00: division by zero.
         * Halts CPU.  No quotient, no stores, no normal return. */
        abort();
    }

    if (ny == -1 && numerator == INT32_MIN) {
        /* Retail BREAK 0x1800: INT32_MIN / -1 signed overflow.
         * Halts CPU.  No quotient, no stores, no normal return. */
        abort();
    }

    /* ---- Valid division ---- */

    /* DIV → MFLO: quotient = numerator / ny (truncation toward zero) */
    s32 quotient = numerator / ny;
    u32 quotient_bits = (u32)quotient;

    /* ---- First store ---- */

    /* sw v0,4(a0) — must occur before loading B[1] */
    *(u32 *)PSX_ADDR(a0 + 4) = quotient_bits;

    /* ---- Alias-sensitive load ---- */

    /* lw v1,4(a1) — if a0 == a1, reads quotient just written */
    u32 base_bits = *(u32 *)PSX_ADDR(a1 + 4);

    /* ---- Final ADDU + second store ---- */

    /* addu v0,v0,v1 — modulo 2^32 */
    u32 result_bits = quotient_bits + base_bits;

    /* sw v0,4(a0) — jr delay slot */
    *(u32 *)PSX_ADDR(a0 + 4) = result_bits;

    /* jr ra */
    return result_bits;
}
