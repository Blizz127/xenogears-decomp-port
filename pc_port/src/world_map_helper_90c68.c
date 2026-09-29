/*
 * World-map helper 0x80090C68 (slot-4 heading input handler).
 *
 * Register-faithful transcription of retail world_map.bin
 * [0x80090C68, 0x80090E14).  See world_map_helper_90c68.h.
 */
#include <stdint.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_helper_90c68.h"
#include "world_map_common_tail.h"

extern long rcos(long a);
extern long rsin(long a);
extern void wm_80090A18(void);

/* Guest addresses. */
#define G90C_DIR_WORD   0x8009CD4Cu  /* lhu -0x32B4: direction input */
#define G90C_HEAD_BASE  0x8009BD3Au  /* lhu -0x42C6: base heading */
#define G90C_FLAGS      0x8009BD10u  /* lhu -0x42F0: button/state flags */
#define G90C_BOUNDARY   0x8009D738u  /* lbu -0x28C8: boundary byte */
#define G90C_AREA       0x8009BD24u  /* lh -0x42DC: area halfword */
#define G90C_D804       0x8009D804u  /* sw -0x27FC: write-on-boundary */
#define G90C_CE68       0x8009CE68u  /* lh -0x3198: check halfword */

static u16 g90c_lhu(u32 a)
{
    u16 v;
    memcpy(&v, PSX_ADDR(a), 2);
    return v;
}

static s16 g90c_lh(u32 a)
{
    s16 v;
    memcpy(&v, PSX_ADDR(a), 2);
    return v;
}

static u8 g90c_lbu(u32 a)
{
    return *(u8*)PSX_ADDR(a);
}

static void g90c_sh(u32 a, u16 v)
{
    memcpy(PSX_ADDR(a), &v, 2);
}

static void g90c_sw(u32 a, u32 v)
{
    memcpy(PSX_ADDR(a), &v, 4);
}

s32 wm_80090C68(u32 slot_record)
{
    u32 dir_word;
    u32 dir_idx;
    u16 base_heading;
    u16 new_heading;
    u32 flags;

    /* Read direction input: u16 at 0x8009CD4C >> 12 */
    dir_word = g90c_lhu(G90C_DIR_WORD);
    dir_idx = (dir_word >> 12) - 1;

    /* Direction dispatch (12 entries) */
    base_heading = g90c_lhu(G90C_HEAD_BASE);

    if (dir_idx < 12) {
        /* JT @0x80070B84: each entry loads base_heading + offset, masks 0xFFF */
        static const s16 heading_offsets[12] = {
            0x000, 0x400, 0x200, 0x800, 0x600, -0x400,
            -0x200, 0x000, -0x600, 0x000, 0x000, 0x000
        };
        /* Note: entries 7,9,10,11 in the retail asm just store base_heading
         * (offset 0). Entry 8 adds -0x600 and masks. Entries 0-6 add their
         * offset and mask. */
        new_heading = (u16)((base_heading + heading_offsets[dir_idx]) & 0xFFF);
        g90c_sh(slot_record + 0x48, new_heading);
    }

    /* Button velocity check: flags & 0xF000 */
    flags = g90c_lhu(G90C_FLAGS);
    if (flags & 0xF000) {
        /* Compute velocity from heading */
        s16 heading = g90c_lh(slot_record + 0x48);
        long cos_val = rcos((long)heading);
        g90c_sw(slot_record + 0x38, (u32)cos_val);
        long sin_val = rsin((long)heading);
        g90c_sw(slot_record + 0x40, (u32)(-(s32)sin_val));
    }

    /* Boundary check: flags & 0x20 */
    if (g90c_lhu(G90C_FLAGS) & 0x20) {
        u8 boundary = g90c_lbu(G90C_BOUNDARY);
        if (boundary != 0) {
            return 3;
        }
        s16 area = g90c_lh(G90C_AREA);
        if (area != -1) {
            return 1;
        }
    }

    /* Secondary check: flags & 0x10 */
    if (g90c_lhu(G90C_FLAGS) & 0x10) {
        s16 ce68 = g90c_lh(G90C_CE68);
        if (ce68 == -1) {
            s16 area2 = g90c_lh(G90C_AREA);
            if (area2 == -1) {
                g90c_sw(G90C_D804, 1);
            }
        }
    }

    /* Call wm_80090A18 */
    wm_80090A18();

    return 0;
}
