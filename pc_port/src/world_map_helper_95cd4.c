/*
 * World-map helper 0x80095CD4 (collision-aware movement probe).
 */
#include <string.h>
#include "common.h"
#include "psx_memory.h"
#include "world_map_helper_95cd4.h"
#include "world_map_helper_84d00.h"
#include "world_map_helper_85418.h"
#include "world_map_helper_93354.h"
#include "world_map_helper_951a8.h"
#include "world_map_terrain_sampler.h"

#define SCRATCH 0x1F800000u
#define Y_MIN   UINT32_C(0xFFD80000) /* -0x280000 */
#define CANDIDATES UINT32_C(0x8009D718)

/* Dead retail-stack region, following WM_95414_FRAME_ATTR. */
#define WM_95CD4_FRAME_ATTR 0x801FFDE0u

static u32 cd_lw(u32 a) { u32 v; memcpy(&v, PSX_ADDR(a), 4); return v; }
static void cd_sw(u32 a, u32 v) { memcpy(PSX_ADDR(a), &v, 4); }
static u16 cd_lhu(u32 a) { u16 v; memcpy(&v, PSX_ADDR(a), 2); return v; }
static void cd_sh(u32 a, u16 v) { memcpy(PSX_ADDR(a), &v, 2); }

static s32 cd_bits_to_s32(u32 bits)
{
    s32 value;
    memcpy(&value, &bits, sizeof(value));
    return value;
}

static u32 cd_sra(u32 bits, u32 shift)
{
    u32 result = bits >> shift;
    if ((bits & UINT32_C(0x80000000)) != 0u)
        result |= UINT32_MAX << (32u - shift);
    return result;
}

static u32 cd_scaled_sum(u32 position, u32 velocity, s32 scale)
{
    int64_t product = (int64_t)cd_bits_to_s32(velocity) * (int64_t)scale;
    u32 low = (u32)(uint64_t)product;
#if defined(W34N122_MUTANT_M7_WRONG_SCALE_SHIFT)
    return position + cd_sra(low, 11u);
#else
    return position + cd_sra(low, 12u);
#endif
}

static int cd_slt(u32 lhs, u32 rhs)
{
    return cd_bits_to_s32(lhs) < cd_bits_to_s32(rhs);
}

static u32 cd_reflect_half(u32 velocity)
{
    u32 negated = 0u - velocity;
#if defined(W34N122_MUTANT_M8_FULL_REFLECTION)
    return negated;
#else
    return cd_sra(negated, 1u);
#endif
}

s32 wm_80095CD4(u32 pos, u32 vel, u32 out, s32 scale, s32 mode)
{
    s32 i;
    s32 count;
    s32 examined;
    s32 rejected;
    u32 terrain;
    u32 candidate;
    u32 scratch_y;
    const u32 scratch_target = SCRATCH + 0x60u;

    /* Compute target = pos + vel * scale >> 12 */
    for (i = 0; i < 3; i++) {
        u32 offset = (u32)i * 4u;
        cd_sw(scratch_target + offset,
              cd_scaled_sum(cd_lw(pos + offset), cd_lw(vel + offset),
                            scale));
    }

    /* Clamp target Y via wm_80093354 */
    wm_80093354(scratch_target);

    /* Compute out = pos + vel * scale >> 12 (same again) */
    for (i = 0; i < 3; i++) {
        u32 offset = (u32)i * 4u;
        cd_sw(out + offset,
              cd_scaled_sum(cd_lw(pos + offset), cd_lw(vel + offset),
                            scale));
    }

    /* Clamp out Y via wm_80093354 */
    wm_80093354(out);

    /* Sample terrain height at target X/Z */
    terrain = (u32)wm_80093978(cd_bits_to_s32(cd_lw(scratch_target)),
                               cd_bits_to_s32(cd_lw(scratch_target + 8u)));
#if defined(W34N122_MUTANT_M5_MISSING_TERRAIN_OFFSET)
    candidate = terrain;
#else
    candidate = terrain + UINT32_C(0xFFFE0000); /* terrain - 0x20000 */
#endif
    if (cd_slt(UINT32_C(0x00020000), candidate))
        candidate = UINT32_C(0x00020000);

    scratch_y = cd_lw(scratch_target + 4u);
    if (cd_slt(candidate, Y_MIN) || cd_slt(candidate, scratch_y)) {
        cd_sw(scratch_target + 4u, candidate);
        cd_sw(out + 4u, candidate);
    }

#if !defined(W34N122_MUTANT_M6_SKIP_LOWER_CROSSING_CLAMP)
    scratch_y = cd_lw(scratch_target + 4u);
    if (cd_slt(Y_MIN, candidate) && cd_slt(scratch_y, Y_MIN)) {
        cd_sw(scratch_target + 4u, Y_MIN);
        cd_sw(out + 4u, Y_MIN);
    }
#endif

    /* Nav-mesh probe */
    count = wm_80084D00(scratch_target, WM_95CD4_FRAME_ATTR);
    if (count == 0)
        goto resolve;

    examined = 0;
    rejected = 0;
    if (count > 0) {
#if defined(W34N122_MUTANT_M9_WRONG_CANDIDATE_BASE)
        u32 record = SCRATCH + 0xD718u;
#else
        u32 record = CANDIDATES;
#endif
        do {
            u32 attribute_address = WM_95CD4_FRAME_ATTR;
#if defined(W34N122_MUTANT_M3_ADVANCE_ATTRIBUTE)
            attribute_address += (u32)examined;
#endif
            if (wm_80085418(scratch_target, 0x70,
                            (u32)cd_lhu(attribute_address),
                            (u32)cd_lhu(record)) == 0) {
#if defined(W34N122_MUTANT_M4_WRITE_ATTRIBUTE_FRAME)
                cd_sh(attribute_address, UINT16_C(0xFFFF));
#else
                cd_sh(record, UINT16_C(0xFFFF));
#endif
                rejected += 2;
            }
            examined += 2;
            record += 4u;
        } while (examined < count);
    }

#if defined(W34N122_MUTANT_M2_REVERSED_RESOLUTION)
    if (rejected != count)
        goto resolve;
#else
    if (rejected == count)
        goto resolve;
#endif

    cd_sw(out + 0u, cd_reflect_half(cd_lw(vel + 0u)));
    cd_sw(out + 4u, cd_reflect_half(cd_lw(vel + 4u)));
    cd_sw(out + 8u, cd_reflect_half(cd_lw(vel + 8u)));
    return 0;

resolve:
#if defined(W34N122_MUTANT_M1_WRONG_FORWARD_MODE)
    return wm_800951A8(pos, vel, out, scale,
                       cd_bits_to_s32((u32)mode ^ UINT32_C(1)));
#else
    return wm_800951A8(pos, vel, out, scale, mode);
#endif
}
