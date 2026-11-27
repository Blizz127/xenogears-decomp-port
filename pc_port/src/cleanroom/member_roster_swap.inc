#include <stdint.h>

u_short MemberChangeMenuIsCharacterFlagSet(u_short, u_char);

/* SPDX-License-Identifier: MIT */
static int MemberRosterAllowsExchange(u8 character_id)
{
    if (character_id == UINT8_MAX)
        return 0;

    /* The host allocation extends beyond the public GameState prefix.
     * Read the two little-endian save bytes without an alignment assumption. */
    const uintptr_t state_base = (uintptr_t)(const void *)&g_GameState;
    const unsigned char *flags = (const unsigned char *)(state_base + 0x2318);
    u16 mask = (u16)(flags[0] | ((u16)flags[1] << 8));
    return MemberChangeMenuIsCharacterFlagSet(mask, character_id) == 0;
}

u_char MemberChangeMenuSwapCharacters(u8 direction, s32 current, s32 offset,
                                      u8 selectedDirection, s32 selected, s32 selectedOffset) {
    (void)selectedDirection;
    u8 party_slot = direction ? (u8)selected : (u8)current;
    u8 reserve_slot = direction ? (u8)((u32)current + (u32)offset)
                                : (u8)((u32)selected + (u32)selectedOffset);
    int party_ok = MemberRosterAllowsExchange(g_Menu->pManager->currentCharacterIDs[party_slot]);
    int reserve_ok = MemberRosterAllowsExchange(g_Menu->unk1E14[reserve_slot]);
    if (!party_ok || !reserve_ok)
        return 0;

    u8 *party = g_Menu->pManager->currentCharacterIDs;
    u8 *reserve = &g_Menu->unk1E14[reserve_slot];
    u8 old_party = party[party_slot];
    u8 old_reserve = *reserve;
    party[party_slot] = old_reserve;
    *reserve = old_party;
    if (party[0] != 255 || party[1] != 255 || party[2] != 255)
        return 1;
    party[party_slot] = old_party;
    *reserve = old_reserve;
    return 0;
}
