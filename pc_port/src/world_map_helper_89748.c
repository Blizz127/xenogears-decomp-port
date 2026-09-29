/*
 * World-map helper 0x80089748 (particle effect spawner/updater).
 *
 * Retail function boundary: [0x80089748, 0x80089C78).  The function walks
 * 512 effect-source records at 0x54-byte stride, advances their spawn timers,
 * materializes eligible records into the dynamic 256 x 0x4c particle pool,
 * then calls the retail particle tick.
 */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "psyq/libgte.h"
#include "world_map_helper_89580.h"
#include "world_map_helper_89748.h"

extern long VectorNormal(VECTOR *input, VECTOR *output);

#define D_8009BCC0       0x8009BCC0u
#define D_8009BDF4       0x8009BDF4u
#define SCRATCH          0x1F800000u
#define SOURCE_COUNT     512u
#define SOURCE_STRIDE    0x54u
#define PARTICLE_COUNT   256u
#define PARTICLE_STRIDE  0x4Cu

static u8 p_lbu(u32 a) { return *(u8*)PSX_ADDR(a); }
static s16 p_lh(u32 a) { s16 v; memcpy(&v, PSX_ADDR(a), 2); return v; }
static u16 p_lhu(u32 a) { u16 v; memcpy(&v, PSX_ADDR(a), 2); return v; }
static s32 p_lw(u32 a) { s32 v; memcpy(&v, PSX_ADDR(a), 4); return v; }
static u32 p_lwu(u32 a) { u32 v; memcpy(&v, PSX_ADDR(a), 4); return v; }
static void p_sb(u32 a, u8 v) { memcpy(PSX_ADDR(a), &v, 1); }
static void p_sh(u32 a, u16 v) { memcpy(PSX_ADDR(a), &v, 2); }
static void p_sw(u32 a, u32 v) { memcpy(PSX_ADDR(a), &v, 4); }

static u32 p_add3(u32 a, u32 b, u32 c)
{
    return a + b + c;
}

static u32 p_mul_lo(s32 a, s32 b)
{
    return (u32)a * (u32)b;
}

static s32 p_sra12(u32 bits)
{
    s32 value;
    memcpy(&value, &bits, sizeof(value));
    return value >> 12;
}

static s32 p_random_angle(void)
{
    return (s32)((u32)rand() & 0x0FFFu) - 0x800;
}

static s32 p_random_amplitude(u16 ceiling)
{
    return (s32)((u32)rand() % (u32)ceiling);
}

static u32 p_source_stride(void)
{
#if defined(WM_89748_MUTANT_SOURCE_STRIDE_4C)
    return 0x4Cu;
#else
    return SOURCE_STRIDE;
#endif
}

static u32 p_source_count(void)
{
#if defined(WM_89748_MUTANT_SOURCE_COUNT_256)
    return 256u;
#else
    return SOURCE_COUNT;
#endif
}

static u32 p_find_free_particle(u32 pool)
{
    u32 i;
    for (i = 0u; i < PARTICLE_COUNT; i++) {
        u32 record = pool + i * PARTICLE_STRIDE;
#if defined(WM_89748_MUTANT_FREE_TEST_LOW_HALF)
        if (p_lh(record + 4u) == 0)
#else
        /* Retail 0x80089830-0x80089840 checks packed owner at record+6. */
        if (p_lh(record + 6u) == 0)
#endif
            return record;
    }
    return 0u;
}

static void p_seed_random_unit(VECTOR *unit, u8 flags, u8 zero_y_flag)
{
    VECTOR *angles = (VECTOR*)PSX_ADDR(SCRATCH + 0x10u);

    /* Retail draws Y first when it is random, then X, then Z. */
#if defined(WM_89748_MUTANT_RANDOM_X_FIRST)
    angles->vx = p_random_angle();
    angles->vy = (flags & zero_y_flag) != 0u ? 0 : p_random_angle();
#else
    angles->vy = (flags & zero_y_flag) != 0u ? 0 : p_random_angle();
    angles->vx = p_random_angle();
#endif
    angles->vz = p_random_angle();
    angles->pad = 0;
    (void)VectorNormal(angles, unit);
}

static u32 p_scaled_component(s32 unit, s32 amplitude, s32 transformed,
                              s16 source_offset)
{
    return p_add3(p_mul_lo(unit, amplitude),
                  (u32)transformed * 0x1000u,
                  (u32)(s32)source_offset * 0x1000u);
}

static void p_spawn_particle(u32 source, u32 source_index, u8 flags,
                             u32 particle)
{
    MATRIX *matrix = (MATRIX*)PSX_ADDR(SCRATCH + 0xF0u);
    VECTOR *unit = (VECTOR*)PSX_ADDR(SCRATCH + 0x00u);
    VECTOR *angles = (VECTOR*)PSX_ADDR(SCRATCH + 0x10u);
    VECTOR *transformed = (VECTOR*)PSX_ADDR(SCRATCH + 0x20u);
    s32 amplitude;
    s32 scale;
    s32 dx;
    s32 dy;
    s32 dz;

    p_sh(particle + 0u, (u16)source_index);
    p_sw(particle + 4u, p_lwu(source + 0x0Cu));

    (void)RotMatrixYXZ((SVECTOR*)PSX_ADDR(source + 0x1Cu), matrix);
#if defined(WM_89748_MUTANT_WRONG_FIRST_VECTOR)
    (void)ApplyMatrix(matrix, (SVECTOR*)PSX_ADDR(source + 0x28u),
#else
    (void)ApplyMatrix(matrix, (SVECTOR*)PSX_ADDR(source + 0x24u),
#endif
                      transformed);

    p_seed_random_unit(unit, flags, 0x20u);
    amplitude = (flags & 0x04u) != 0u
#if defined(WM_89748_MUTANT_WRONG_FIRST_AMPLITUDE)
                    ? (s32)p_lhu(source + 0x42u)
#else
                    ? (s32)p_lhu(source + 0x40u)
#endif
                    : p_random_amplitude(p_lhu(source + 0x40u));
    p_sw(particle + 0x08u,
         p_scaled_component(unit->vx, amplitude, transformed->vx,
                            p_lh(source + 0x14u)));
    p_sw(particle + 0x0Cu,
         p_scaled_component(unit->vy, amplitude, transformed->vy,
                            p_lh(source + 0x16u)));
    p_sw(particle + 0x10u,
         p_scaled_component(unit->vz, amplitude, transformed->vz,
                            p_lh(source + 0x18u)));

#if defined(WM_89748_MUTANT_SKIP_SECOND_APPLY)
    (void)source;
#else
    (void)ApplyMatrix(matrix, (SVECTOR*)PSX_ADDR(source + 0x2Cu),
                      transformed);
#endif
    p_seed_random_unit(unit, flags, 0x40u);
    amplitude = (flags & 0x08u) != 0u
                    ? (s32)p_lhu(source + 0x42u)
                    : p_random_amplitude(p_lhu(source + 0x42u));

    dx = p_sra12(p_scaled_component(unit->vx, amplitude, transformed->vx,
                                    p_lh(source + 0x14u)) -
                 p_lwu(particle + 0x08u));
    dy = p_sra12(p_scaled_component(unit->vy, amplitude, transformed->vy,
                                    p_lh(source + 0x16u)) -
                 p_lwu(particle + 0x0Cu));
    dz = p_sra12(p_scaled_component(unit->vz, amplitude, transformed->vz,
                                    p_lh(source + 0x18u)) -
                 p_lwu(particle + 0x10u));
    p_sw(SCRATCH + 0x00u, (u32)dx);
    p_sw(SCRATCH + 0x04u, (u32)dy);
    p_sw(SCRATCH + 0x08u, (u32)dz);
    (void)VectorNormal((VECTOR*)PSX_ADDR(SCRATCH + 0x00u), angles);

    scale = p_lw(source + 0x34u);
    p_sw(particle + 0x18u,
         (u32)p_sra12(p_mul_lo(angles->vx, scale)));
    p_sw(particle + 0x1Cu,
         (u32)p_sra12(p_mul_lo(angles->vy, scale)));
    p_sw(particle + 0x20u,
         (u32)p_sra12(p_mul_lo(angles->vz, scale)));
    p_sh(particle + 0x02u, (u16)ratan2(angles->vy, angles->vx));

#if defined(WM_89748_MUTANT_UNSIGNED_COPIES)
    p_sw(particle + 0x28u, (u32)p_lhu(source + 0x38u));
#else
    p_sw(particle + 0x28u, (u32)(s32)p_lh(source + 0x38u));
#endif
    p_sw(particle + 0x2Cu, (u32)(s32)p_lh(source + 0x3Au));
    p_sw(particle + 0x30u, (u32)(s32)p_lh(source + 0x3Cu));
    p_sw(particle + 0x40u, p_lwu(source + 0x4Cu));
    p_sw(particle + 0x44u, p_lwu(source + 0x50u));
    p_sw(particle + 0x38u, p_lwu(source + 0x44u));
    p_sw(particle + 0x3Cu, p_lwu(source + 0x48u));
    p_sh(particle + 0x48u,
         (u16)((((u16)flags & 3u) << 5) | 0x009Du));

#if !defined(WM_89748_MUTANT_NO_SOURCE_COUNT_INCREMENT)
    p_sh(source + 0x0Au, (u16)(p_lhu(source + 0x0Au) + 1u));
#endif
}

void wm_80089748(void)
{
    u32 source_table = p_lwu(D_8009BCC0);
    u32 stride = p_source_stride();
    u32 count = p_source_count();
    u32 source_index;

    for (source_index = 0u; source_index < count; source_index++) {
        u32 source = source_table + source_index * stride;
#if defined(WM_89748_MUTANT_FLAGS_AT_4B)
        u32 flags_address = source + 0x4Bu;
#else
        u32 flags_address = source + 0x4Fu;
#endif
        u8 flags = p_lbu(flags_address);
        u32 packed;
        s16 high_timer;
        s16 low_timer;
        s16 spawn_timer;

        if ((flags & 0x80u) == 0u)
            continue;

        packed = p_lwu(source + 4u);
        high_timer = (s16)(u16)(packed >> 16);
        low_timer = (s16)(u16)packed;
#if defined(WM_89748_MUTANT_HIGH_TIMER_ZERO)
        high_timer = 0;
#endif

        if ((flags & 0x10u) != 0u) {
            if (low_timer != 0) {
                low_timer = (s16)(low_timer - 1);
                goto store_timers;
            }
            if (high_timer == 0) {
                p_sb(flags_address, (u8)(flags ^ 0x80u));
                goto store_timers;
            }
            high_timer = (s16)(high_timer - 1);
        }

        spawn_timer = p_lh(source + 0x12u);
        if (spawn_timer != 0) {
            p_sh(source + 0x12u, (u16)(spawn_timer - 1));
            goto store_timers;
        }
        p_sh(source + 0x12u, p_lhu(source + 0x10u));

        if (p_lh(source + 0x08u) > 0 &&
            p_lh(source + 0x0Au) < p_lh(source + 0x08u) &&
            p_lh(source + 0x0Eu) != 0) {
            u32 particle = p_find_free_particle(p_lwu(D_8009BDF4));
            if (particle != 0u)
                p_spawn_particle(source, source_index, flags, particle);
        }

store_timers:
        p_sw(source + 4u,
             ((u32)(u16)high_timer << 16) | (u32)(u16)low_timer);
    }

#if !defined(WM_89748_MUTANT_NO_TICK)
    wm_80089580();
#endif
}
