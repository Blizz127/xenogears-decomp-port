/*
 * Retail-private collision probes used only by world function 0x80094A5C.
 *
 *   0x8009443C  X positive  [0x8009443C,0x800945C8)
 *   0x800945C8  X negative  [0x800945C8,0x80094750)
 *   0x80094750  Z positive  [0x80094750,0x800948D8)
 *   0x800948D8  Z negative  [0x800948D8,0x80094A5C)
 *
 * All arithmetic below is expressed in unsigned register bits where retail
 * wraps.  Conversions to signed values use memcpy, avoiding implementation-
 * defined casts and native signed overflow.
 */
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_helper_93354.h"
#include "world_map_helper_93f18.h"
#include "world_map_helper_94060.h"
#include "world_map_private_collision.h"

typedef enum WmProbeAxis { WM_PROBE_X, WM_PROBE_Z } WmProbeAxis;
typedef enum WmProbeDirection { WM_PROBE_POS, WM_PROBE_NEG } WmProbeDirection;

#define WM_PROBE_GRID_MASK UINT32_C(0xFFF80000)

#if defined(WM_PRIVATE_PROBE_TEST_TRACE)
extern void wm_private_test_93354(u32 addr);
extern s32 wm_private_test_93f18(u32 addr);
extern s32 wm_private_test_94060(s32 mode, s32 code);
#define WM_CALL_93354(a) wm_private_test_93354(a)
#define WM_CALL_93F18(a) wm_private_test_93f18(a)
#define WM_CALL_94060(m, c) wm_private_test_94060((m), (c))
#else
#define WM_CALL_93354(a) wm_80093354(a)
#define WM_CALL_93F18(a) wm_80093F18(a)
#define WM_CALL_94060(m, c) wm_80094060((m), (c))
#endif

static u32 wm_probe_load_u32(u32 addr)
{
    u32 value;
    memcpy(&value, PSX_ADDR(addr), sizeof(value));
    return value;
}

static void wm_probe_store_u32(u32 addr, u32 value)
{
    memcpy(PSX_ADDR(addr), &value, sizeof(value));
}

static s32 wm_probe_as_s32(u32 bits)
{
    s32 value;
    memcpy(&value, &bits, sizeof(value));
    return value;
}

static u32 wm_probe_as_u32(s32 value)
{
    u32 bits;
    memcpy(&bits, &value, sizeof(bits));
    return bits;
}

static u32 wm_probe_sra(u32 bits, u32 amount)
{
    u32 value = bits >> amount;
    if ((bits & UINT32_C(0x80000000)) != 0u)
        value |= UINT32_MAX << (32u - amount);
    return value;
}

static s32 wm_probe_sign16(u32 bits)
{
    u32 extended = bits & UINT32_C(0xFFFF);
    if ((extended & UINT32_C(0x8000)) != 0u)
        extended |= UINT32_C(0xFFFF0000);
    return wm_probe_as_s32(extended);
}

static void wm_probe_div_trap(void) __attribute__((noreturn, unused));
static void wm_probe_div_trap(void)
{
    __builtin_trap();
}

static u32 wm_probe_ratio(u32 numerator_bits, u32 denominator_bits)
{
    u32 shifted = numerator_bits << 12;
    s32 numerator = wm_probe_as_s32(shifted);
    s32 denominator = wm_probe_as_s32(denominator_bits);

#if defined(WM_PRIVATE_MUTANT_SWAP_RATIO)
    {
        s32 temp = numerator;
        numerator = denominator;
        denominator = temp;
    }
#endif

#if defined(WM_PRIVATE_MUTANT_MISSING_TRAP)
    if (denominator == 0 ||
        (numerator == INT32_MIN && denominator == -1))
        return 0u;
#else
    if (denominator == 0 ||
        (numerator == INT32_MIN && denominator == -1))
        wm_probe_div_trap();
#endif

#if defined(WM_PRIVATE_MUTANT_UNSIGNED_ARITH)
    return (u32)numerator / (u32)denominator;
#else
    return wm_probe_as_u32((s32)((s64)numerator / (s64)denominator));
#endif
}

static u32 wm_probe_mul_sra12(u32 left_bits, u32 right_bits)
{
#if defined(WM_PRIVATE_MUTANT_UNSIGNED_ARITH)
    u64 product = (u64)left_bits * (u64)right_bits;
#else
    s64 product = (s64)wm_probe_as_s32(left_bits) *
                  (s64)wm_probe_as_s32(right_bits);
#endif
    u32 low = (u32)((u64)product & UINT64_C(0xFFFFFFFF));
#if defined(WM_PRIVATE_MUTANT_WRONG_SHIFT_TRUNC)
    return wm_probe_sra(low, 11u);
#else
    return wm_probe_sra(low, 12u);
#endif
}

static int wm_probe_blocked(s32 value)
{
    return (wm_probe_as_u32(value) & UINT32_C(0xFFFF)) != 0u;
}

static s32 wm_private_probe_core(u32 base_vec, u32 direction_vec,
                                 u32 workspace, s32 mode,
                                 WmProbeAxis axis,
                                 WmProbeDirection direction)
{
    u32 axis_off = axis == WM_PROBE_X ? 0u : 8u;
    u32 ortho_off = axis == WM_PROBE_X ? 8u : 0u;
    u32 seed_off = axis == WM_PROBE_X ? 0x10u : 0x18u;
    u32 c0_axis_off = axis == WM_PROBE_X ? 0x20u : 0x28u;
    u32 c0_ortho_off = axis == WM_PROBE_X ? 0x28u : 0x20u;
    u32 c1_axis_off = axis == WM_PROBE_X ? 0x30u : 0x38u;
    u32 c1_ortho_off = axis == WM_PROBE_X ? 0x38u : 0x30u;
    u32 numerator_off = axis == WM_PROBE_X ? 8u : 0u;
    u32 denominator_off = axis == WM_PROBE_X ? 0u : 8u;
    u32 base_axis;
    u32 base_ortho;
    u32 ratio;
    u32 edge_source;
    u32 edge;
    u32 delta0;
    u32 delta1;
    u32 c0_addr = workspace + 0x20u;
    u32 c1_addr = workspace + 0x30u;
    s32 signed_mode;
    s32 code;
    s32 blocked;

#if defined(WM_PRIVATE_MUTANT_WRONG_SEED)
    seed_off = axis == WM_PROBE_X ? 0x18u : 0x10u;
#endif
#if defined(WM_PRIVATE_MUTANT_WRONG_FIELD)
    c0_axis_off = c0_ortho_off;
#endif
#if defined(WM_PRIVATE_MUTANT_SWAP_AXIS_ORTHO)
    {
        u32 temp = axis_off;
        axis_off = ortho_off;
        ortho_off = temp;
    }
#endif

    ratio = wm_probe_ratio(wm_probe_load_u32(direction_vec + numerator_off),
                           wm_probe_load_u32(direction_vec + denominator_off));
    base_axis = wm_probe_load_u32(base_vec + axis_off);
    base_ortho = wm_probe_load_u32(base_vec + ortho_off);

#if defined(WM_PRIVATE_MUTANT_WRONG_MASK)
    edge_source = (direction == WM_PROBE_POS)
        ? wm_probe_load_u32(workspace + seed_off) : base_axis;
    edge = (edge_source & UINT32_C(0xFFFF0000)) - base_axis;
#elif defined(WM_PRIVATE_MUTANT_WRONG_EDGE_SOURCE)
    edge_source = (direction == WM_PROBE_POS)
        ? base_axis : wm_probe_load_u32(workspace + seed_off);
    edge = (edge_source & WM_PROBE_GRID_MASK) - base_axis;
#else
    edge_source = (direction == WM_PROBE_POS)
        ? wm_probe_load_u32(workspace + seed_off) : base_axis;
    edge = (edge_source & WM_PROBE_GRID_MASK) - base_axis;
#endif

#if defined(WM_PRIVATE_MUTANT_WRONG_MINUS_ONE)
    delta0 = edge;
    delta1 = edge;
#else
    delta0 = direction == WM_PROBE_POS ? edge - 1u : edge;
    delta1 = direction == WM_PROBE_POS ? edge : edge - 1u;
#endif

    /* Preserve the retail store sequence and its failure-path workspace
     * residue: raw candidate deltas first, then orthogonal projections, then
     * base-axis addition.  Candidate-0 success later overwrites +0..+0xC. */
    wm_probe_store_u32(workspace + c0_axis_off, delta0);
    wm_probe_store_u32(workspace + axis_off, edge);
    wm_probe_store_u32(workspace + c1_axis_off, delta1);
    wm_probe_store_u32(workspace + c0_ortho_off,
                       base_ortho + wm_probe_mul_sra12(ratio, delta0));
    wm_probe_store_u32(workspace + c1_ortho_off,
                       base_ortho + wm_probe_mul_sra12(ratio, delta1));
    wm_probe_store_u32(workspace + c0_axis_off,
                       wm_probe_load_u32(workspace + c0_axis_off) + base_axis);
    wm_probe_store_u32(workspace + c1_axis_off,
                       wm_probe_load_u32(workspace + c1_axis_off) + base_axis);

#if defined(WM_PRIVATE_MUTANT_NO_MODE_SIGNEXT)
    signed_mode = mode;
#else
    signed_mode = wm_probe_sign16(wm_probe_as_u32(mode));
#endif

#if defined(WM_PRIVATE_MUTANT_WRONG_CANDIDATE_PTR)
    c0_addr = c1_addr;
#endif

#if defined(WM_PRIVATE_MUTANT_WRONG_CALL_ORDER)
    code = WM_CALL_93F18(c0_addr);
    WM_CALL_93354(c0_addr);
#else
    WM_CALL_93354(c0_addr);
    code = WM_CALL_93F18(c0_addr);
#endif
#if defined(WM_PRIVATE_MUTANT_NO_CODE_SIGNEXT)
#else
    code = wm_probe_sign16(wm_probe_as_u32(code));
#endif
#if defined(WM_PRIVATE_MUTANT_SWAP_94060_ARGS)
    blocked = WM_CALL_94060(code, signed_mode);
#else
    blocked = WM_CALL_94060(signed_mode, code);
#endif

#if defined(WM_PRIVATE_MUTANT_WRONG_FIRST_BRANCH)
    if (wm_probe_blocked(blocked)) {
#else
    if (!wm_probe_blocked(blocked)) {
#endif
        u32 i;
#if defined(WM_PRIVATE_MUTANT_NO_SHORT_CIRCUIT)
        WM_CALL_93354(c1_addr);
        (void)WM_CALL_93F18(c1_addr);
        (void)WM_CALL_94060(signed_mode, code);
#endif
        for (i = 0u; i < 4u; i++) {
#if defined(WM_PRIVATE_MUTANT_WRONG_COPY)
            wm_probe_store_u32(workspace + i * 4u,
                               wm_probe_load_u32(c0_addr + ((i + 1u) & 3u) * 4u));
#else
            wm_probe_store_u32(workspace + i * 4u,
                               wm_probe_load_u32(c0_addr + i * 4u));
#endif
        }
        return 1;
    }

    WM_CALL_93354(c1_addr);
    code = WM_CALL_93F18(c1_addr);
#if defined(WM_PRIVATE_MUTANT_NO_CODE_SIGNEXT)
#else
    code = wm_probe_sign16(wm_probe_as_u32(code));
#endif
#if defined(WM_PRIVATE_MUTANT_SWAP_94060_ARGS)
    blocked = WM_CALL_94060(code, signed_mode);
#else
    blocked = WM_CALL_94060(signed_mode, code);
#endif
    if (!wm_probe_blocked(blocked)) {
#if defined(WM_PRIVATE_MUTANT_BOOLEAN_RETURN)
        return 1;
#else
        if (axis == WM_PROBE_X) {
#if defined(WM_PRIVATE_MUTANT_WRONG_X_CODE)
            return 2;
#else
            return 3;
#endif
        }
#if defined(WM_PRIVATE_MUTANT_WRONG_Z_CODE)
        return 3;
#else
        return 2;
#endif
#endif
    }
    return 0;
}

s32 wm_8009443C(u32 a0, u32 a1, u32 a2, s32 a3)
{
    return wm_private_probe_core(a0, a1, a2, a3, WM_PROBE_X, WM_PROBE_POS);
}

s32 wm_800945C8(u32 a0, u32 a1, u32 a2, s32 a3)
{
    return wm_private_probe_core(a0, a1, a2, a3, WM_PROBE_X, WM_PROBE_NEG);
}

s32 wm_80094750(u32 a0, u32 a1, u32 a2, s32 a3)
{
    return wm_private_probe_core(a0, a1, a2, a3, WM_PROBE_Z, WM_PROBE_POS);
}

s32 wm_800948D8(u32 a0, u32 a1, u32 a2, s32 a3)
{
    return wm_private_probe_core(a0, a1, a2, a3, WM_PROBE_Z, WM_PROBE_NEG);
}
