#include "psx_memory.h"
#include "world_map_gamestate_alias.h"

void wm_sync_gamestate_channel_aliases(const u8* host_gamestate)
{
    u8* guest_channels =
        (u8*)PSX_ADDR(WM_GAMESTATE_CHANNEL_GUEST_BASE);
    u32 channel;

    for (channel = 0; channel < WM_GAMESTATE_CHANNEL_COUNT; channel++) {
        u8 primary_id;

        guest_channels[channel] =
            host_gamestate[WM_GAMESTATE_CHANNEL_HOST_OFFSET + channel];

        primary_id = guest_channels[channel];
        if (primary_id != 0xFFu) {
            u32 host_offset = WM_GAMESTATE_GEAR_HOST_BASE +
                              (u32)primary_id * WM_GAMESTATE_CHARACTER_STRIDE;
            u32 guest_addr = WM_GAMESTATE_GEAR_GUEST_BASE +
                             (u32)primary_id * WM_GAMESTATE_CHARACTER_STRIDE;
            *(u8*)PSX_ADDR(guest_addr) = host_gamestate[host_offset];
        }
    }
}
