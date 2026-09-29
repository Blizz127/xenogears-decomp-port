/*
 * World-map helper 0x800965A4 (queue record allocator variant B).
 */
#include <stdint.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_helper_965a4.h"
#include "world_map_helper_964b0.h"

#define D_8009BE44  0x8009BE44u
#define D_8009D3C0  0x8009D3C0u
#define D_8009D808  0x8009D808u
#define D_8009C624  0x8009C624u

static u32 a5a4_lw(u32 a) { u32 v; memcpy(&v, PSX_ADDR(a), 4); return v; }
static void a5a4_sw(u32 a, u32 v) { memcpy(PSX_ADDR(a), &v, 4); }

s32 wm_800965A4(void)
{
    u32 idx = a5a4_lw(D_8009BE44);
    u32 base = a5a4_lw(D_8009D3C0);

    /* Retail bank stride is 0x580 bytes (16-byte records). */
    u32 stride = idx * 0x580u;
    u32 record = base + stride;

    if (a5a4_lw(record) == 0u ||
        a5a4_lw(D_8009C624 + idx * 4u) != 0u) {
        a5a4_sw(D_8009D808, 0u);
        return -1;
    }
    wm_800964B0(record);

    /* Reset queue counter, store pointer, advance index (mod 16) */
    a5a4_sw(D_8009D808, 0);
    a5a4_sw(D_8009C624 + idx * 4u, record);
    a5a4_sw(D_8009BE44, (idx + 1) & 0xF);

    return 0;
}
