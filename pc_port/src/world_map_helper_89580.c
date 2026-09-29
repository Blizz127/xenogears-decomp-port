/*
 * World-map helper 0x80089580 (particle system tick).
 *
 * Retail function boundary: [0x80089580, 0x80089748).  This leaf walks the
 * 256 records in the dynamic 0x4c-byte particle pool.  A nonzero high
 * halfword marks an owned record; a positive low halfword advances its
 * vectors, UVs, and color, while an expired low halfword releases the
 * owning 0x54-byte world-object slot.
 */
#include <stdint.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_helper_89580.h"

#define D_8009BCC0       0x8009BCC0u
#define D_8009BDF4       0x8009BDF4u
#define PARTICLE_COUNT   256u
#define PARTICLE_STRIDE  0x4Cu
#define OWNER_STRIDE     0x54u

static u32 pt_lw(u32 a) { u32 v; memcpy(&v, PSX_ADDR(a), 4); return v; }
static void pt_sw(u32 a, u32 v) { memcpy(PSX_ADDR(a), &v, 4); }
static u16 pt_lhu(u32 a) { u16 v; memcpy(&v, PSX_ADDR(a), 2); return v; }
static void pt_sh(u32 a, u16 v) { memcpy(PSX_ADDR(a), &v, 2); }
static s16 pt_lh(u32 a) { s16 v; memcpy(&v, PSX_ADDR(a), 2); return v; }

static u32 pt_slot_table_base(void)
{
#if defined(WM_89580_MUTANT_WRONG_SLOT_GLOBAL)
    return pt_lw(0x8009BDE0u);
#elif defined(WM_89580_MUTANT_PARTICLE_GLOBAL)
    return pt_lw(D_8009BDF4);
#elif defined(WM_89580_MUTANT_SLOT_OFFSET)
    return pt_lw(D_8009BCC0 + 4u);
#else
    /* Retail 0x800896FC-0x80089708. */
    return pt_lw(D_8009BCC0);
#endif
}

static u8 pt_clamp_color(s32 value)
{
#if defined(WM_89580_MUTANT_NO_COLOR_CLAMP)
    return (u8)value;
#else
    if (value < 0)
        return 0u;
    if (value >= 0x100)
        return 0xFFu;
    return (u8)value;
#endif
}

void wm_80089580(void)
{
    u32 record_base = pt_lw(D_8009BDF4);
    u32 entry = record_base + 4u;
    u32 i;

#if defined(WM_89580_MUTANT_WRONG_COUNT)
    const u32 count = PARTICLE_COUNT - 1u;
#else
    const u32 count = PARTICLE_COUNT;
#endif
#if defined(WM_89580_MUTANT_WRONG_PARTICLE_STRIDE)
    const u32 record_stride = 0x54u;
#else
    const u32 record_stride = PARTICLE_STRIDE;
#endif

    for (i = 0u; i < count; i++) {
        u32 packed = pt_lw(entry);
        s16 owner = (s16)(u16)(packed >> 16);
        s16 remaining = (s16)(u16)packed;

#if defined(WM_89580_MUTANT_SKIP_OWNER_GATE)
        (void)owner;
#else
        /* Retail 0x800895A0-0x800895AC: an unowned record is untouched. */
        if (owner == 0) {
            entry += record_stride;
            record_base += record_stride;
            continue;
        }
#endif

#if defined(WM_89580_MUTANT_REMAINING_FROM_HIGH)
        remaining = owner;
#endif

        if (remaining > 0) {
            u32 color = pt_lw(entry + 0x3Cu);
            u32 deltas = pt_lw(entry + 0x40u);
            s32 red_delta;
            s32 green_delta;
            s32 blue_delta;
            u8 red;
            u8 green;
            u8 blue;

            /* Retail uses wrapping addu for both integration stages. */
            pt_sw(entry + 0x04u,
                  pt_lw(entry + 0x04u) + pt_lw(entry + 0x14u));
            pt_sw(entry + 0x08u,
                  pt_lw(entry + 0x08u) + pt_lw(entry + 0x18u));
            pt_sw(entry + 0x0Cu,
                  pt_lw(entry + 0x0Cu) + pt_lw(entry + 0x1Cu));
#if !defined(WM_89580_MUTANT_SKIP_SECOND_INTEGRATION)
            pt_sw(entry + 0x14u,
                  pt_lw(entry + 0x14u) + pt_lw(entry + 0x24u));
            pt_sw(entry + 0x18u,
                  pt_lw(entry + 0x18u) + pt_lw(entry + 0x28u));
            pt_sw(entry + 0x1Cu,
                  pt_lw(entry + 0x1Cu) + pt_lw(entry + 0x2Cu));
#endif

            pt_sh(entry + 0x34u,
                  (u16)(pt_lhu(entry + 0x34u) +
#if defined(WM_89580_MUTANT_WRONG_UV_DELTA)
                        pt_lhu(entry + 0x3Au)));
#else
                        pt_lhu(entry + 0x38u)));
#endif
            pt_sh(entry + 0x36u,
                  (u16)(pt_lhu(entry + 0x36u) + pt_lhu(entry + 0x3Au)));

#if defined(WM_89580_MUTANT_COLOR_FROM_PACKED)
            (void)deltas;
            red_delta = (s8)(u8)(packed >> 0);
            green_delta = (s8)(u8)(packed >> 8);
            blue_delta = (s8)(u8)(packed >> 16);
#else
            red_delta = (s8)(u8)(deltas >> 0);
            green_delta = (s8)(u8)(deltas >> 8);
            blue_delta = (s8)(u8)(deltas >> 16);
#endif
            red = pt_clamp_color((s32)(u8)color + red_delta);
            green = pt_clamp_color((s32)(u8)(color >> 8) + green_delta);
            blue = pt_clamp_color((s32)(u8)(color >> 16) + blue_delta);
            pt_sw(entry + 0x3Cu,
                  (color & 0xFF000000u) | ((u32)blue << 16) |
                  ((u32)green << 8) | (u32)red);

#if defined(WM_89580_MUTANT_WRONG_COUNTER_HALF)
            pt_sh(entry, (u16)(owner - 1));
#else
            pt_sh(entry, (u16)(remaining - 1));
#endif
        } else {
            s16 slot_index = pt_lh(record_base);
            u32 slot_stride;
            u32 slot_record;

#if defined(WM_89580_MUTANT_WRONG_SLOT_STRIDE)
            slot_stride = 0x2A0u;
#else
            slot_stride = OWNER_STRIDE;
#endif
            slot_record = pt_slot_table_base() +
                          (u32)((s32)slot_index * (s32)slot_stride);
            pt_sh(slot_record + 0x0Au,
                  (u16)(pt_lhu(slot_record + 0x0Au) - 1u));
            pt_sh(record_base, 0u);
            pt_sw(entry, 0u);
        }

        entry += record_stride;
        record_base += record_stride;
    }
}
