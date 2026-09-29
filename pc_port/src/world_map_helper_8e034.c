/*
 * World-map fixed-point vector unloader 0x8008E034.
 *
 * Register-faithful, finite-width transcription of retail world_map.bin
 * [0x8008E034, 0x8008E078).  Loads three 32-bit fixed-point words from the
 * caller-supplied input buffer, performs arithmetic right shift by 12 on
 * each, and stores the low 16 bits of each result as signed halfwords to
 * the global world-vector destinations at 0x8006EE60/62/64.
 */
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_gamestate.h"
#include "world_map_helper_8e034.h"

#define WM_E034_DST_X  0x8006EE60u
#define WM_E034_DST_Y  0x8006EE62u
#define WM_E034_DST_Z  0x8006EE64u

/* Arithmetic right shift by 12 with fully defined 32-bit behavior.
 * MIPS SRA: if bit31==0, upper 12 bits become 0; if bit31==1, they become 1. */
static u32 sra_12(u32 value)
{
    u32 shifted = value >> 12;
    if (value & 0x80000000u) {
        shifted |= 0xFFF00000u;
    }
    return shifted;
}

void wm_8008E034(u32 in_addr)
{
    u32 word;
    u32 shifted;
    u16 result;

    /* X component */
    memcpy(&word, PSX_ADDR(in_addr + 0u), sizeof(word));
    shifted = sra_12(word);
    result = (u16)shifted;
    wm_gs_sh(WM_E034_DST_X, result); /* guest + host g_GameState */

    /* Y component */
    memcpy(&word, PSX_ADDR(in_addr + 4u), sizeof(word));
    shifted = sra_12(word);
    result = (u16)shifted;
    wm_gs_sh(WM_E034_DST_Y, result); /* guest + host g_GameState */

    /* Z component */
    memcpy(&word, PSX_ADDR(in_addr + 8u), sizeof(word));
    shifted = sra_12(word);
    result = (u16)shifted;
    wm_gs_sh(WM_E034_DST_Z, result); /* guest + host g_GameState */
}
