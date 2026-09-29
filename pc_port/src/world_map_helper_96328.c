/*
 * World-map helper 0x80096328 (queue record allocator variant A).
 */
#include <stdint.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_helper_96328.h"
#include "world_map_helper_9623c.h"

#define D_8009BE44  0x8009BE44u
#define D_8009BE08  0x8009BE08u
#define D_8009D808  0x8009D808u
#define D_8009D788  0x8009D788u
#define RECORD_STRIDE 1056u  /* 33 * 32 */

static u32 a328_lw(u32 a) { u32 v; memcpy(&v, PSX_ADDR(a), 4); return v; }
static void a328_sw(u32 a, u32 v) { memcpy(PSX_ADDR(a), &v, 4); }

/* Retail [0x80096328, 0x800963E4): publish record block BE44 to D788 only when
 * its first record is non-empty (0x80096358-60) and D788[idx] is free
 * (0x80096370-7C); then sort it (0x80096384), clear the queue counter,
 * store the block, advance BE44 mod 16 (0x8009638C-B4). Otherwise clear the
 * counter and return -1 (0x800963C0-C8). */
s32 wm_80096328(void)
{
    u32 idx = a328_lw(D_8009BE44);
    u32 base = a328_lw(D_8009BE08);
    u32 record = base + idx * RECORD_STRIDE;

#if defined(WM_96328_MUTANT_M9)            /* publishes empty blocks */
    if (a328_lw(D_8009D788 + idx * 4) != 0) {
#else
    if (a328_lw(record) == 0 || a328_lw(D_8009D788 + idx * 4) != 0) {
#endif
        a328_sw(D_8009D808, 0);
        return -1;
    }
    wm_800963E4(record);
    a328_sw(D_8009D808, 0);
    a328_sw(D_8009D788 + idx * 4, record);
    a328_sw(D_8009BE44, (idx + 1) & 0xF);
    return 0;
}
