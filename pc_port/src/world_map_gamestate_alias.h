/*
 * Retail GameState channel-control aliases used by world callbacks.
 *
 * On PSX, GameState offset 0x1D34 is absolute address 0x8006F368, so the
 * three channel IDs and the callback-visible bytes are the same storage.
 * The PC port keeps authoritative GameState in a separate host allocation;
 * this bridge restores those three retail aliases before world package
 * creation and scheduler execution.
 *
 * Additionally, the gear/resource byte for each channel's primary character
 * (GameState offset 0x030C + character_id * 0xA4, guest address 0x8006D940)
 * must be synchronized so that retail C364's secondary-resource dispatch
 * reads the same authoritative value as the host package-selection path.
 */
#ifndef WORLD_MAP_GAMESTATE_ALIAS_H
#define WORLD_MAP_GAMESTATE_ALIAS_H

#include "common.h"

#define WM_GAMESTATE_CHANNEL_HOST_OFFSET  0x1D34u
#define WM_GAMESTATE_CHANNEL_GUEST_BASE   0x8006F368u
#define WM_GAMESTATE_CHANNEL_COUNT        3u

#define WM_GAMESTATE_GEAR_HOST_BASE       0x030Cu
#define WM_GAMESTATE_GEAR_GUEST_BASE      0x8006D940u
#define WM_GAMESTATE_CHARACTER_STRIDE     0xA4u

void wm_sync_gamestate_channel_aliases(const u8* host_gamestate);

#endif /* WORLD_MAP_GAMESTATE_ALIAS_H */
