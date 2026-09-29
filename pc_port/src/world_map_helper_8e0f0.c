/*
 * World-map helper 0x8008E0F0 (angle search via wm_80095414).
 */
#include <string.h>
#include "common.h"
#include "psx_memory.h"
#include "world_map_helper_8e0f0.h"
#include "world_map_func_95414.h"

extern long rcos(long a);
extern long rsin(long a);

/* Dead retail-stack region, following WM_95414_FRAME_ATTR. */
#define WM_8E0F0_FRAME_DIR 0x801FFDD0u

s32 wm_8008E0F0(u32 pos, u32 unused, u32 out)
{
    s32 angle;
    for (angle = 0; angle < 0x1000; angle += 0x100) {
        /* Build direction vector from angle */
        long cos_val = rcos((long)angle);
        long sin_val = rsin((long)angle);

        /* Direction: (rcos, 0, -rsin) */
        u32 dir[3];
        dir[0] = (u32)cos_val;
        dir[1] = 0;
        dir[2] = (u32)(-(s32)sin_val);

        /* F2 repair - W34C17R/W34C18.  wm_80095414 only reads dir. */
        s32 result;
        memcpy(PSX_ADDR(WM_8E0F0_FRAME_DIR), dir, sizeof(dir));
        result = wm_80095414(pos, WM_8E0F0_FRAME_DIR, out, 0, 2);
        if (result == 1) {
            return angle;
        }
    }
    return -1;
}
