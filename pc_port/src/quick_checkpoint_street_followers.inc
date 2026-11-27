/* Bledavik street's three-character on-foot party uses a shared movement
 * history. Party order differs from field actor order. Never serialize an
 * actor address or infer membership from a resident's position. */
typedef struct {
    CaveActors state;
    u8 slots[2], characters[2];
    s16 sprite_facing[2];
    u16 moving[2], use_trail[2];
} StreetFollowers;

_Static_assert(sizeof(StreetFollowers) <= PC_PORT_QUICK_PARTY_BYTES,
               "street party checkpoint layout");

static int street_party_ready(void)
{
    if ((g_GameSceneMapNum & 0x3fff) != 102 || g_FieldScriptMemoryU16[0] != 62 ||
        g_FieldScriptMemoryU16[1] != 2 || !g_FieldActors || D_800ADBFC <= 0 ||
        D_800AFB54 != 3 || D_800AFB44[0] <= 0 || D_800AFB44[1] <= 0 ||
        D_800B21CC || D_800B21CE || D_800B21CF || D_800B234E || D_800B21D0 ||
        g_GamePartyMembers[0] != 0 || g_GamePartyMembers[1] != 3 || g_GamePartyMembers[2] != 2 ||
        D_8005A444[0] != g_PlayerActorIndex)
        return 0;
    for (int i = 0; i < 3; ++i) {
        int slot = D_8005A444[i];
        if (slot < 0 || slot >= D_800ADBFC || slot >= g_FieldNumActors || slot >= 255)
            return 0;
        const FieldActor *field = &g_FieldActors[slot];
        const ActorData *actor = actor_data(slot);
        if (!actor || !field->pSpriteData || (field->status & ACTOR_STATUS_INVISIBLE) ||
            (field->status & 0xf80) != 0x200 || actor->characterId != g_GamePartyMembers[i] ||
            (actor->scriptFlags.flags & 0x1800) || (i && !(actor->scriptFlags.flags & 0x01000000u)))
            return 0;
    }
    return 1;
}

static int street_values_valid(const StreetFollowers *saved)
{
    if (!trail_valid(saved->state.trail, saved->state.cursor, saved->state.last_movement, 3))
        return 0;
    for (int i = 0; i < 2; ++i) {
        const CavePose *pose = &saved->state.poses[i];
        if (saved->slots[i] != D_8005A444[i + 1] || saved->characters[i] != g_GamePartyMembers[i + 1] ||
            !contact_valid(pose->layer, pose->triangles) ||
            (saved->moving[i] & ~0x800u) || (saved->use_trail[i] & ~0x1000u))
            return 0;
    }
    return 1;
}

int PcPort_StreetFollowersCapture(uint8_t out[PC_PORT_QUICK_PARTY_BYTES])
{
    StreetFollowers saved = {0};
    if (!out || !street_party_ready())
        return 0;
    for (int i = 0; i < 2; ++i) {
        int slot = D_8005A444[i + 1];
        const ActorData *actor = actor_data(slot);
        const u8 *sprite = (const u8 *)(uintptr_t)g_FieldActors[slot].pSpriteData;
        saved.slots[i] = slot;
        saved.characters[i] = actor->characterId;
        transfer_pose(&saved.state.poses[i], slot, 0);
        memcpy(&saved.sprite_facing[i], sprite + 0x80, sizeof saved.sprite_facing[i]);
        saved.moving[i] = actor->scriptFlags.flags & 0x800;
        saved.use_trail[i] = actor->flags & 0x1000;
    }
    transfer_trail(saved.state.trail, saved.state.cursor, &saved.state.last_movement, 0);
    if (!street_values_valid(&saved))
        return 0;
    memset(out, 0, PC_PORT_QUICK_PARTY_BYTES);
    memcpy(out, &saved, sizeof saved);
    return 1;
}

int PcPort_StreetFollowersRestore(const uint8_t in[PC_PORT_QUICK_PARTY_BYTES])
{
    StreetFollowers saved;
    if (!in || !street_party_ready())
        return 0;
    memcpy(&saved, in, sizeof saved);
    if (!street_values_valid(&saved))
        return 0;
    for (size_t i = sizeof saved; i < PC_PORT_QUICK_PARTY_BYTES; ++i)
        if (in[i])
            return 0;
    for (int i = 0; i < 2; ++i) {
        int slot = D_8005A444[i + 1];
        ActorData *actor = actor_data(slot);
        transfer_pose(&saved.state.poses[i], slot, 1);
        actor->scriptFlags.flags = (actor->scriptFlags.flags & ~0x800u) | saved.moving[i];
        actor->flags = (actor->flags & ~0x1000u) | saved.use_trail[i];
        func_800223B0((void *)(uintptr_t)g_FieldActors[slot].pSpriteData, saved.sprite_facing[i]);
    }
    transfer_trail(saved.state.trail, saved.state.cursor, &saved.state.last_movement, 1);
    return 1;
}

int PcPort_StreetFollowersRegroup(void)
{
    if (!street_party_ready())
        return 0;
    /* Old slots lack follower poses. Start the normal follower trail at the
     * saved player and use the live actor bindings from this scene. */
    ActorData *leader = actor_data(g_PlayerActorIndex);
    if (!contact_valid(leader->walkmeshId, leader->walkmeshTriIds))
        return 0;
    copy_follower_placement(g_PlayerActorIndex, D_8005A444[1]);
    copy_follower_placement(g_PlayerActorIndex, D_8005A444[2]);
    func_8009B338();
    return 1;
}
