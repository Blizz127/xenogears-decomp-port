/*
 * World-map helper 0x80090E14 (heading input handler variant).
 * Same structure as wm_80090C68 but different boundary logic.
 */
#include <stdint.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_helper_90e14.h"

extern long rcos(long a);
extern long rsin(long a);

#define G90E_DIR_WORD   0x8009CD4Cu
#define G90E_HEAD_BASE  0x8009BD3Au
#define G90E_FLAGS      0x8009BD10u
#define G90E_AREA       0x8009BD24u
#define G90E_CE68       0x8009CE68u
#define G90E_D804       0x8009D804u

static u16 g90e_lhu(u32 a) { u16 v; memcpy(&v, PSX_ADDR(a), 2); return v; }
static s16 g90e_lh(u32 a) { s16 v; memcpy(&v, PSX_ADDR(a), 2); return v; }
static void g90e_sh(u32 a, u16 v) { memcpy(PSX_ADDR(a), &v, 2); }
static void g90e_sw(u32 a, u32 v) { memcpy(PSX_ADDR(a), &v, 4); }

s32 wm_80090E14(u32 slot_record)
{
    u16 dir_word = g90e_lhu(G90E_DIR_WORD);
    u32 dir_idx = (dir_word >> 12) - 1;
    u16 base_heading = g90e_lhu(G90E_HEAD_BASE);

    /* Direction dispatch (12 entries, JT @0x80070BB4) */
    if (dir_idx < 12) {
        static const s16 offsets[12] = {
            0x000, 0x400, 0x200, 0x800, 0x600, -0x400,
            -0x200, 0x000, -0x600, 0x000, 0x000, 0x000
        };
        u16 new_h = (u16)((base_heading + offsets[dir_idx]) & 0xFFF);
        g90e_sh(slot_record + 0x48, new_h);
    }

    /* Button velocity: flags & 0xF000 */
    if (g90e_lhu(G90E_FLAGS) & 0xF000) {
        s16 heading = g90e_lh(slot_record + 0x48);
        g90e_sw(slot_record + 0x38, (u32)rcos((long)heading));
        g90e_sw(slot_record + 0x40, (u32)(-(s32)rsin((long)heading)));
    }

    /* Boundary check: flags & 0x20 */
    if (g90e_lhu(G90E_FLAGS) & 0x20) {
        s16 area = g90e_lh(G90E_AREA);
        if (area != -1) {
            return 1;
        }
    }

    /* Flag 0x40 check */
    if (g90e_lhu(G90E_FLAGS) & 0x40) {
        return 4;
    }

    /* Flag 0x10 check */
    if (g90e_lhu(G90E_FLAGS) & 0x10) {
        s16 ce68 = g90e_lh(G90E_CE68);
        if (ce68 == -1) {
            s16 area2 = g90e_lh(G90E_AREA);
            if (area2 == ce68) {
                g90e_sw(G90E_D804, 1);
            }
        }
    }

    return 0;
}
