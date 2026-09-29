/*
 * World-map helper 0x8008E078 (area-dependent table selector).
 * Leaf function, no external calls.
 */
#include <string.h>
#include "common.h"
#include "psx_memory.h"
#include "world_map_helper_8e078.h"

#define E078_BOUNDARY  0x8009D738u
#define E078_AREA_BYTE 0x8009BD60u
#define E078_TBL_PTR   0x8009D7D8u
#define E078_AREA2     0x8009BD24u

static u8 e078_lbu(u32 a) { return *(u8*)PSX_ADDR(a); }
static u16 e078_lhu(u32 a) { u16 v; memcpy(&v, PSX_ADDR(a), 2); return v; }
static void e078_sw(u32 a, u32 v) { memcpy(PSX_ADDR(a), &v, 4); }
static void e078_sh(u32 a, u16 v) { memcpy(PSX_ADDR(a), &v, 2); }

void wm_8008E078(void)
{
    u8 boundary = e078_lbu(E078_BOUNDARY);
    if (boundary == 0) return;

    {
        u8 area = e078_lbu(E078_AREA_BYTE);
        u32 tbl_ptr;
        u16 size;

        if (area == 0x0F) {
            size = e078_lhu(0x8009B6D0u);
            tbl_ptr = 0x8009B6C4u;
        } else if (area == 0x10) {
            size = e078_lhu(0x8009B6E0u);
            tbl_ptr = 0x8009B6D4u;
        } else {
            return;
        }

        e078_sw(E078_TBL_PTR, tbl_ptr);
        e078_sh(E078_AREA2, size);
    }
}
