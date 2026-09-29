/*
 * World-map scheduler callback 0x800914D0 (slot-9 cb1, on-foot camera).
 *
 * Register-faithful transcription of retail world_map.bin
 * [0x800914D0, 0x80091B54).  See world_map_callback_914d0.h.
 *
 * Pre-dispatch JT @0x80070BE4 (index = (s16)(lhu(slot+0x04) - 9),
 * sltiu 9):
 *   0 -> 0x80091528   1 -> 0x80091558   2..5 -> 0x800915A4 (skip)
 *   6 -> 0x80091570   7 -> 0x80091580   8 -> 0x80091590
 * Main dispatch JT @0x80070C0C (index = lh(slot+0x20), sltiu 17):
 *   0 -> 0x800915D0   1 -> 0x800916CC   2 -> 0x800916F4
 *   16 -> 0x800917BC  3..15 -> 0x80091870 (skip)
 * Post-dispatch (0x80091870): states 0/1/2/16 follow the pose block
 * with a 1/8 step (0x800918A4); state 3 snaps X/Z and eases Y by 1/16
 * (0x80091A2C).  Every path exits through the common tail 0x80091B04,
 * which wraps slot+0x28 via 0x80093354 and publishes slot+0x28..+0x34
 * to the world position block D_8009BE28..D_8009BE34.
 */
#include <stdint.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_callback_914d0.h"
#include "world_map_helper_93354.h"
#include "world_map_helper_93484.h"
#include "world_map_helper_97770.h"

#define POOL_PTR    0x8009BE24u
#define HEAD_MIRROR 0x8009BD3Au /* lh/sh -17094 */
#define D_8009D52C  0x8009D52Cu /* lh -10964 */
#define DIR_WORD    0x8009CD4Cu /* lhu -12980 */
#define D_8009D55C  0x8009D55Cu /* pose block X */
#define D_8009D560  0x8009D560u /* pose block Y */
#define D_8009D564  0x8009D564u /* pose block Z */
#define D_8009BBB4  0x8009BBB4u
#define D_8009BBBC  0x8009BBBCu
#define D_8009BE28  0x8009BE28u

/* Dead retail-stack region, following WM_95414_FRAME_ATTR.  Retail
 * builds the pose delta on its own stack (sp+0x10..0x18) and hands that
 * address to the guest-pointer wrap helper 0x80093484. */
#define WM_914D0_FRAME_DELTA 0x801FFDC0u

static s16 d0_lh(u32 a) { s16 v; memcpy(&v, PSX_ADDR(a), 2); return v; }
static u16 d0_lhu(u32 a) { u16 v; memcpy(&v, PSX_ADDR(a), 2); return v; }
static s32 d0_lw(u32 a) { s32 v; memcpy(&v, PSX_ADDR(a), 4); return v; }
static void d0_sh(u32 a, u16 v) { memcpy(PSX_ADDR(a), &v, 2); }
static void d0_sw(u32 a, s32 v) { memcpy(PSX_ADDR(a), &v, 4); }

/* 0x8009164C..0x800916C8: shared heading accumulator step.  The
 * accumulator slot+0x58 is masked to 24 bits before the >>12 publish. */
static void d0_heading_step(u32 slot)
{
    s32 current = (s32)d0_lh(HEAD_MIRROR);
    s32 target = d0_lw(slot + 0x50u);
    s32 diff;
    s32 magnitude;
    u32 accumulator;

    if (current == target) {
        d0_sw(slot + 0x58u, (s32)((u32)current << 12));
        return;
    }

    diff = (s32)((u32)target - (u32)current);
    magnitude = diff < 0 ? -diff : diff;
    if (!(magnitude < 3073)) {
        if (diff >= 0)
            diff -= 4096;
        else
            diff += 4096;
    }

    accumulator = (u32)d0_lw(slot + 0x58u) +
                  (u32)(((s32)((u32)diff << 12)) >> 3);
    accumulator &= 0x00FFFFFFu;
    d0_sw(slot + 0x58u, (s32)accumulator);
    d0_sh(HEAD_MIRROR, (u16)(accumulator >> 12));
}

/* 0x800916F4 / 0x800917BC: approach the heading target with a clamped
 * step (±0x180 per frame, divided by 8 for state 2 and by 32 for state
 * 16).  Returns the newly published heading. */
static s32 d0_heading_approach(u32 slot, u32 shift)
{
    s32 target = d0_lw(slot + 0x50u);
    s32 diff = (s32)((u32)target - (u32)(s32)d0_lh(HEAD_MIRROR));
    s32 magnitude = diff < 0 ? -diff : diff;
    u32 accumulator;

    if (!(magnitude < 2049)) {
        if (diff >= 0)
            diff -= 4096;
        else
            diff += 4096;
    }

    magnitude = diff < 0 ? -diff : diff;
    if (!(magnitude < 385))
        diff = diff >= 0 ? 384 : -384;

    accumulator = (u32)d0_lw(slot + 0x58u) +
                  (u32)(((s32)((u32)diff << 12)) >> shift);
    accumulator &= 0x00FFFFFFu;
    d0_sw(slot + 0x58u, (s32)accumulator);
    d0_sh(HEAD_MIRROR, (u16)(accumulator >> 12));
    return (s32)(accumulator >> 12);
}

s32 wm_800914D0(s32 slot_idx)
{
    u32 pool_ptr = (u32)d0_lw(POOL_PTR);
    u32 slot = pool_ptr + (u32)(slot_idx << 7);
    s16 state;

    /* Pre-dispatch on (s16)(slot[+0x04] - 9), sltiu 9. */
    {
        s32 idx = (s32)(s16)(d0_lhu(slot + 0x04u) - 9u);

        if ((u32)idx < 9u) {
            switch (idx) {
            case 0: /* 0x80091528 */
                d0_sh(slot + 0x04u, 0);
                d0_sh(slot + 0x20u, 2);
                d0_sw(slot + 0x50u, (s32)d0_lh(D_8009D52C));
                d0_sw(slot + 0x58u,
                      (s32)((u32)(s32)d0_lh(HEAD_MIRROR) << 12));
                break;
            case 1: /* 0x80091558 */
                d0_sh(slot + 0x04u, 0);
                d0_sh(slot + 0x20u, 0);
                d0_sw(slot + 0x50u, (s32)d0_lh(HEAD_MIRROR));
                break;
            case 6: /* 0x80091570 */
                d0_sh(slot + 0x20u, 16);
                d0_sh(slot + 0x04u, 0);
                d0_sw(slot + 0x50u, 0x800);
                break;
            case 7: /* 0x80091580 */
                d0_sh(slot + 0x20u, 16);
                d0_sh(slot + 0x04u, 0);
                d0_sw(slot + 0x50u, 0xA00);
                break;
            case 8: /* 0x80091590 */
                d0_sh(slot + 0x20u, 16);
                d0_sh(slot + 0x04u, 0);
                d0_sw(slot + 0x50u, 0xC00);
                break;
            default: /* 2..5 -> 0x800915A4 */
                break;
            }
        }
    }

    /* Main dispatch on lh(slot[+0x20]), sltiu 17. */
    state = d0_lh(slot + 0x20u);
    if ((u32)(s32)state < 17u) {
        switch (state) {
        case 0: { /* 0x800915D0: d-pad turn request, then heading step */
            u32 dir_bits = ((u32)d0_lhu(DIR_WORD) >> 2) & 3u;

            if (dir_bits == 2u) {
                d0_sh(slot + 0x20u, 1);
                d0_sw(slot + 0x54u, 64);
                d0_sw(slot + 0x5Cu,
                      (s32)(((u32)d0_lw(slot + 0x50u) + 512u) & 0xFFFu));
            } else if (dir_bits == 1u || dir_bits == 3u) {
                d0_sh(slot + 0x20u, 1);
                d0_sw(slot + 0x54u, -64);
                d0_sw(slot + 0x5Cu,
                      (s32)(((u32)d0_lw(slot + 0x50u) - 512u) & 0xFFFu));
            }
            d0_heading_step(slot);
            break;
        }
        case 1: { /* 0x800916CC: sweep target by +0x54 until +0x5C */
            s32 next = (s32)(((u32)d0_lw(slot + 0x50u) +
                              (u32)d0_lw(slot + 0x54u)) & 0xFFFu);

            d0_sw(slot + 0x50u, next);
            if (next == d0_lw(slot + 0x5Cu))
                d0_sh(slot + 0x20u, 0);
            d0_heading_step(slot);
            break;
        }
        case 2: { /* 0x800916F4 */
            s32 heading = d0_heading_approach(slot, 3u);

            if (heading == d0_lw(slot + 0x50u) && d0_lw(slot + 0x60u) == 0) {
                d0_sh(slot + 0x20u, 3);
                wm_80097770(7, 0x0B);
            }
            break;
        }
        case 16: { /* 0x800917BC */
            s32 heading = d0_heading_approach(slot, 5u);

            if (heading == d0_lw(slot + 0x50u) && d0_lw(slot + 0x60u) == 0)
                d0_sh(slot + 0x20u, 3);
            break;
        }
        default: /* 3..15 -> 0x80091870 */
            break;
        }
    }

    /* Post-dispatch (0x80091870): follow the pose block D_8009D55C. */
    state = d0_lh(slot + 0x20u);
    if (state == 3) {
        /* 0x80091A2C: snap X/Z, ease Y by 1/16. */
        s32 px = d0_lw(slot + 0x28u);
        s32 py = d0_lw(slot + 0x2Cu);
        s32 pz = d0_lw(slot + 0x30u);
        s32 bx = d0_lw(D_8009D55C);
        s32 by = d0_lw(D_8009D560);
        s32 bz = d0_lw(D_8009D564);

        if (px != bx || py != by || pz != bz) {
            u32 delta[3];

            delta[0] = (u32)bx - (u32)px;
            delta[1] = (u32)by - (u32)py;
            delta[2] = (u32)bz - (u32)pz;
            memcpy(PSX_ADDR(WM_914D0_FRAME_DELTA), delta, sizeof(delta));
            wm_80093484(WM_914D0_FRAME_DELTA);
            memcpy(delta, PSX_ADDR(WM_914D0_FRAME_DELTA), sizeof(delta));

            d0_sw(slot + 0x2Cu,
                  (s32)((u32)d0_lw(slot + 0x2Cu) +
                        (u32)((s32)delta[1] >> 4)));
            d0_sw(D_8009BBB4, (s32)((u32)d0_lw(D_8009BBB4) + delta[0]));
            d0_sw(D_8009BBBC, (s32)((u32)d0_lw(D_8009BBBC) + delta[2]));
            d0_sw(slot + 0x28u, d0_lw(D_8009D55C));
            d0_sw(slot + 0x30u, d0_lw(D_8009D564));
        }
    } else if (state == 0 || state == 1 || state == 2 || state == 16) {
        /* 0x800918A4: step 1/8 of the delta; snap when within 64 units. */
        s32 px = d0_lw(slot + 0x28u);
        s32 py = d0_lw(slot + 0x2Cu);
        s32 pz = d0_lw(slot + 0x30u);
        s32 bx = d0_lw(D_8009D55C);
        s32 by = d0_lw(D_8009D560);
        s32 bz = d0_lw(D_8009D564);

        if (px != bx || py != by || pz != bz) {
            u32 delta[3];
            s32 step_x;
            s32 step_y;
            s32 step_z;
            s32 abs_x;
            s32 abs_z;

            delta[0] = (u32)bx - (u32)px;
            delta[1] = (u32)by - (u32)py;
            delta[2] = (u32)bz - (u32)pz;
            memcpy(PSX_ADDR(WM_914D0_FRAME_DELTA), delta, sizeof(delta));
            wm_80093484(WM_914D0_FRAME_DELTA);
            memcpy(delta, PSX_ADDR(WM_914D0_FRAME_DELTA), sizeof(delta));

            step_x = (s32)delta[0] >> 3;
            step_y = (s32)delta[1] >> 3;
            step_z = (s32)delta[2] >> 3;
            abs_x = step_x < 0 ? -step_x : step_x;
            abs_z = step_z < 0 ? -step_z : step_z;

            if (abs_x < 64 && abs_z < 64) {
                d0_sw(slot + 0x60u, 0);
                d0_sw(D_8009BBB4,
                      (s32)((u32)d0_lw(D_8009BBB4) + delta[0]));
                d0_sw(D_8009BBBC,
                      (s32)((u32)d0_lw(D_8009BBBC) + delta[2]));
                d0_sw(slot + 0x28u, d0_lw(D_8009D55C));
                d0_sw(slot + 0x30u, d0_lw(D_8009D564));
            } else {
                d0_sw(slot + 0x60u, 1);
                d0_sw(slot + 0x28u,
                      (s32)((u32)d0_lw(slot + 0x28u) + (u32)step_x));
                d0_sw(slot + 0x30u,
                      (s32)((u32)d0_lw(slot + 0x30u) + (u32)step_z));
                d0_sw(D_8009BBB4,
                      (s32)((u32)d0_lw(D_8009BBB4) + (u32)step_x));
                d0_sw(D_8009BBBC,
                      (s32)((u32)d0_lw(D_8009BBBC) + (u32)step_z));
            }
            d0_sw(slot + 0x2Cu,
                  (s32)((u32)d0_lw(slot + 0x2Cu) + (u32)step_y));
        }
    }

    /* Common tail 0x80091B04: wrap the camera anchor and publish it as
     * the world position block consumed by the object/sprite passes and
     * the view-matrix builder. */
    wm_80093354(slot + 0x28u);
    d0_sw(D_8009BE28 + 0x0u, d0_lw(slot + 0x28u));
    d0_sw(D_8009BE28 + 0x4u, d0_lw(slot + 0x2Cu));
    d0_sw(D_8009BE28 + 0x8u, d0_lw(slot + 0x30u));
    d0_sw(D_8009BE28 + 0xCu, d0_lw(slot + 0x34u));

    return 1;
}
