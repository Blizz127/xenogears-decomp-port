/* world_map_gamestate.h -- the world map's g_GameState fields, one copy.
 *
 * Retail keeps g_GameState at 0x8006D634 and the world map reads and writes
 * its fields there (the field transition tuple 0x8006F94E.., the saved world
 * position 0x8006EE60..).  The port's world-map code addresses them as guest
 * addresses in emulated PSX RAM, but everything else in the port (FieldMain,
 * save/load, checkpoints) uses the host g_GameState blob (data_game_state.c).
 * Storing through wm_gs_sh keeps both copies equal, and wm_gs_lhu reads the
 * host copy, which is the authoritative one.  Outside the g_GameState range
 * both fall back to plain guest RAM.
 *
 * g_GameState is a weak reference so standalone tests that link a world-map
 * TU without the port's data TUs still link (guest RAM only then). */
#ifndef WORLD_MAP_GAMESTATE_H
#define WORLD_MAP_GAMESTATE_H

#include <string.h>
#include "psx_memory.h"

#define WM_GS_BASE UINT32_C(0x8006D634)
#define WM_GS_SIZE UINT32_C(0x2358) /* retail g_GameState span (see data_game_state.c) */

extern unsigned char g_GameState[];
#pragma weak g_GameState

static inline unsigned char* wm_gs_host(uint32_t guest, uint32_t width)
{
    uint32_t off = (guest & 0x1FFFFFu) - (WM_GS_BASE & 0x1FFFFFu);
    if (g_GameState == NULL || (guest & 0x1FFFFFu) < (WM_GS_BASE & 0x1FFFFFu) ||
        off + width > WM_GS_SIZE)
        return NULL;
    return g_GameState + off;
}

static inline void wm_gs_sh(uint32_t guest, uint16_t value)
{
    unsigned char* host = wm_gs_host(guest, 2u);
    memcpy(PSX_ADDR(guest), &value, sizeof(value));
    if (host != NULL)
        memcpy(host, &value, sizeof(value));
}

static inline uint16_t wm_gs_lhu(uint32_t guest)
{
    unsigned char* host = wm_gs_host(guest, 2u);
    uint16_t value;
    memcpy(&value, host != NULL ? (const void*)host : PSX_ADDR(guest), sizeof(value));
    return value;
}

static inline void wm_gs_sb(uint32_t guest, uint8_t value)
{
    unsigned char* host = wm_gs_host(guest, 1u);
    *(uint8_t*)PSX_ADDR(guest) = value;
    if (host != NULL)
        *host = value;
}

static inline uint8_t wm_gs_lbu(uint32_t guest)
{
    unsigned char* host = wm_gs_host(guest, 1u);
    return host != NULL ? *host : *(const uint8_t*)PSX_ADDR(guest);
}

#endif /* WORLD_MAP_GAMESTATE_H */
