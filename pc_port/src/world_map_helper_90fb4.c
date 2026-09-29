/*
 * World-map helper 0x80090FB4 (camera control handler).
 */
#include <stdint.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_helper_90fb4.h"

extern long rcos(long a);
extern long rsin(long a);

/* Local vector struct for VectorNormal (same layout as PsyQ VECTOR) */
typedef struct { long vx, vy, vz; } WmVec;
extern long VectorNormal(WmVec* v0, WmVec* v1);

#define DIR_WORD       0x8009CD4Cu
#define FLAGS          0x8009BD10u
#define AREA           0x8009BD24u
#define CE68           0x8009CE68u
#define D804           0x8009D804u
#define DIRECTION_AREA 0x8009BD3Cu  /* -0x42C4: s16 direction area */
#define HEADING_MIRROR 0x8009BD3Au  /* -0x42C6: u16 heading mirror */

static u16 fb4_lhu(u32 a) { u16 v; memcpy(&v, PSX_ADDR(a), 2); return v; }
static s16 fb4_lh(u32 a) { s16 v; memcpy(&v, PSX_ADDR(a), 2); return v; }
static s32 fb4_lw(u32 a) { s32 v; memcpy(&v, PSX_ADDR(a), 4); return v; }
static void fb4_sh(u32 a, u16 v) { memcpy(PSX_ADDR(a), &v, 2); }
static void fb4_sw(u32 a, s32 v) { memcpy(PSX_ADDR(a), &v, 4); }

/* Slot field offsets. */
#define F_HEADING    0x48  /* u16 */
#define F_SPEED      0x4A  /* s16 */
#define F_VEL_X      0x38  /* s32 */
#define F_VEL_Y      0x3C  /* s32 */
#define F_VEL_Z      0x40  /* s32 */
#define F_SPEED_ACC  0x60  /* s32 */
#define F_HEAD_ACC   0x58  /* s32 (accumulated heading, 12.12 fixed) */
#define F_HEAD_SPD   0x5C  /* s32 (heading acceleration) */
#define F_SUBSTATE   0x64  /* s32 */
#define F_ZOOM_MODE  0x6C  /* s32 */
#define F_ZOOM_VAL   0x70  /* s32 */

s32 wm_80090FB4(u32 slot_record)
{
    u16 dir_word;
    s32 target_head_delta, head_step_limit;
    s32 target_speed_delta, speed_step_limit;
    s32 v1, a3;

    /* Sub-state dispatch */
    s32 substate = fb4_lw(slot_record + F_SUBSTATE);

    if (substate == 0) {
        /* Init: set state=1, clear acceleration, set heading = current << 12 */
        fb4_sw(slot_record + F_SUBSTATE, 1);
        fb4_sw(slot_record + F_HEAD_ACC, 0);
        fb4_sw(slot_record + F_HEAD_SPD, 0);
        fb4_sw(slot_record + F_HEAD_ACC,
               (s32)(s16)fb4_lhu(slot_record + F_HEADING) << 12);
        goto compute_velocity;
    }

    if (substate != 1) {
        /* States 2/3: zoom modes — skip heading control */
        goto zoom_dispatch;
    }

    /* === State 1: Main heading + speed control === */
    dir_word = fb4_lhu(DIR_WORD);
    target_head_delta = 0;
    head_step_limit = 8;
    a3 = 0; /* target speed delta */

    /* D-pad heading: 0x8000 = left (-0x60), 0x2000 = right (+0x60) */
    if (dir_word & 0x8000) {
        target_head_delta = -0x60;
        a3 = 0xFFFF0000; /* speed target = -0x10000 */
    }
    if (dir_word & 0x2000) {
        target_head_delta += 0x60;
        a3 += 0x10000;
    }

    /* Forward/backward: 0x04 = back (-0x60), 0x08 = forward (+0x60) */
    if (dir_word & 0x04) {
        target_head_delta -= 0x60;
        a3 -= 0x10000;
    }
    if (dir_word & 0x08) {
        target_head_delta += 0x60;
        a3 += 0x10000;
    }

    /* If any directional input, compute step limit from current area */
    if (dir_word & 0xA00C) {
        s32 area_val = (s32)fb4_lh(DIRECTION_AREA); /* 0x8009BD3C */
        head_step_limit = 0x1000;
        v1 = area_val - target_head_delta;
        if (v1 < 0) v1 = -v1;
        head_step_limit = v1 >> 4;
    }

    /* Interpolate heading toward target */
    {
        s16 *area_ptr = (s16*)PSX_ADDR(DIRECTION_AREA);
        s32 current = (s32)*area_ptr;
        s32 diff, abs_diff;

        if (current != target_head_delta) {
            diff = current - target_head_delta;
            abs_diff = diff < 0 ? -diff : diff;
            if (abs_diff < head_step_limit) {
                head_step_limit = abs_diff;
            }
            if (current < target_head_delta) {
                *area_ptr = (s16)(current + head_step_limit);
            } else {
                *area_ptr = (s16)(current - head_step_limit);
            }
        }
    }

    /* Interpolate speed toward target */
    {
        s32 current_speed = fb4_lw(slot_record + F_HEAD_SPD);
        if (current_speed != a3) {
            s32 diff = current_speed - a3;
            s32 abs_diff = diff < 0 ? -diff : diff;
            speed_step_limit = 0x1200;
            if (abs_diff < speed_step_limit) {
                speed_step_limit = abs_diff;
            }
            if (current_speed < a3) {
                fb4_sw(slot_record + F_HEAD_SPD, current_speed + speed_step_limit);
            } else {
                fb4_sw(slot_record + F_HEAD_SPD, current_speed - speed_step_limit);
            }
        }
    }

compute_velocity:
    /* Accumulate heading: head_acc += head_spd */
    {
        s32 head_acc = fb4_lw(slot_record + F_HEAD_ACC);
        s32 head_spd = fb4_lw(slot_record + F_HEAD_SPD);
        head_acc += head_spd;
        fb4_sw(slot_record + F_HEAD_ACC, head_acc);

        /* Update heading (12-bit) */
        u16 heading = (u16)((head_acc >> 12) & 0xFFF);
        fb4_sh(slot_record + F_HEADING, heading);
        fb4_sh(HEADING_MIRROR, heading); /* 0x8009BD3A */
    }

zoom_dispatch:
    /* Zoom mode dispatch (slot[+0x6C]) */
    {
        s32 zoom_mode = fb4_lw(slot_record + F_ZOOM_MODE);

        if (zoom_mode == 0) {
            fb4_sw(slot_record + F_ZOOM_MODE, 1);
            fb4_sw(slot_record + F_ZOOM_VAL, 0);
        } else if (zoom_mode == 1) {
            /* Decelerate zoom toward 0 */
            s32 zoom = fb4_lw(slot_record + F_ZOOM_VAL);
            if (zoom != 0) {
                if (zoom < 0) {
                    zoom += 0x2000;
                } else {
                    zoom -= 0x2000;
                }
                fb4_sw(slot_record + F_ZOOM_VAL, zoom);
            }
            /* Check zoom-in button (0x4000) */
            if (fb4_lhu(DIR_WORD) & 0x4000) {
                fb4_sw(slot_record + F_ZOOM_MODE, 2);
            }
            /* Check zoom-out button (0x1000) */
            if (fb4_lhu(DIR_WORD) & 0x1000) {
                fb4_sw(slot_record + F_ZOOM_MODE, 3);
            }
        } else if (zoom_mode == 2) {
            /* Zoom-in: decrease zoom_val by 0x4000 per frame */
            if (fb4_lhu(DIR_WORD) & 0x4000) {
                s32 zoom = fb4_lw(slot_record + F_ZOOM_VAL) - 0x4000;
                fb4_sw(slot_record + F_ZOOM_VAL, zoom);
                if (zoom < (s32)0xFFF80000) { /* -0x80000 */
                    fb4_sw(slot_record + F_ZOOM_VAL, 0xFFF80000);
                }
            } else {
                fb4_sw(slot_record + F_ZOOM_MODE, 1);
            }
        } else if (zoom_mode == 3) {
            /* Zoom-out: increase zoom_val by 0x4000 per frame */
            if (fb4_lhu(DIR_WORD) & 0x1000) {
                s32 zoom = fb4_lw(slot_record + F_ZOOM_VAL) + 0x4000;
                fb4_sw(slot_record + F_ZOOM_VAL, zoom);
                if (zoom > 0x80000) {
                    fb4_sw(slot_record + F_ZOOM_VAL, 0x80000);
                }
            } else {
                fb4_sw(slot_record + F_ZOOM_MODE, 1);
            }
        }
    }

    /* Speed control: button 0x80 accelerates */
    if (fb4_lhu(DIR_WORD) & 0x80) {
        s32 speed = fb4_lw(slot_record + F_SPEED_ACC);
        if (fb4_lhu(DIR_WORD) & 0x02) {
            speed -= 0x1000;
        } else {
            speed += 0x1000;
        }
        fb4_sw(slot_record + F_SPEED_ACC, speed);

        /* Clamp speed to [-max_speed, +max_speed] */
        if (fb4_lhu(DIR_WORD) & 0x80) {
            s32 max_speed = (s32)(s16)fb4_lh(slot_record + F_SPEED) << 12;
            if (speed < -max_speed) {
                fb4_sw(slot_record + F_SPEED_ACC, -max_speed);
            }
            if (speed > max_speed) {
                fb4_sw(slot_record + F_SPEED_ACC, max_speed);
            }
        }
    } else {
        /* No speed input: decelerate toward 0 */
        s32 speed = fb4_lw(slot_record + F_SPEED_ACC);
        if (speed != 0) {
            if (speed < 0) {
                speed += 0x1000;
            } else {
                speed -= 0x1000;
            }
            fb4_sw(slot_record + F_SPEED_ACC, speed);
        }
    }

    /* Compute velocity from heading + zoom */
    {
        s16 heading = fb4_lh(HEADING_MIRROR);
        s32 zoom = fb4_lw(slot_record + F_ZOOM_VAL) >> 12;

        fb4_sw(slot_record + F_VEL_X, (s32)rcos((long)heading));
        fb4_sw(slot_record + F_VEL_Y, (s32)rcos((long)zoom));
        fb4_sw(slot_record + F_VEL_Z, -(s32)rsin((long)heading));

        /* Normalize velocity vector */
        {
            WmVec in, out;
            in.vx = fb4_lw(slot_record + F_VEL_X);
            in.vy = fb4_lw(slot_record + F_VEL_Y);
            in.vz = fb4_lw(slot_record + F_VEL_Z);
            VectorNormal(&in, &out);
            fb4_sw(slot_record + F_VEL_X, out.vx);
            fb4_sw(slot_record + F_VEL_Y, out.vy);
            fb4_sw(slot_record + F_VEL_Z, out.vz);
        }
    }

    /* Boundary/flag checks (same pattern as 90C68/90E14) */
    if (fb4_lhu(FLAGS) & 0x20) {
        s16 area = fb4_lh(AREA);
        if (area != -1) return 1;
    }
    if (fb4_lhu(FLAGS) & 0x40) {
        s32 speed_acc = fb4_lw(slot_record + F_SPEED_ACC);
        s32 area_val = (s32)fb4_lh(0x8009BD3Cu);
        if (speed_acc == 0 && area_val == 0) return 4;
    }
    if (fb4_lhu(FLAGS) & 0x10) {
        s32 ce68 = (s32)fb4_lh(CE68);
        if (ce68 == -1) {
            s32 area2 = (s32)fb4_lh(AREA);
            if (area2 == ce68) {
                fb4_sw(D804, 1);
            }
        }
    }

    return 0;
}
