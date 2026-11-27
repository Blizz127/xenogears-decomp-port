typedef s16 FieldTriggerHalf __attribute__((__may_alias__));
typedef u32 FieldTriggerWord __attribute__((__may_alias__));

static volatile ActorData *fieldTriggerActor(void)
{
    return *(ActorData *volatile *)&g_FieldScriptVMCurActor;
}

static const volatile u8 *fieldTriggerScript(void)
{
    return *(void *volatile *)&g_FieldScriptVMCurScriptData;
}

static s16 fieldTriggerCoordinate(const volatile int *fixed)
{
    return ((const volatile FieldTriggerHalf *)fixed)[1];
}

static int fieldTriggerPoint(s16 x, s16 z)
{
    return (s32)(((u32)(u16)z << 16) + (u32)(s32)x);
}

static int fieldTriggerContains(int useHeight)
{
    const volatile FieldTriggerZone *zone;
    const volatile ActorData *player;
    int x[4], z[4], px, pz;
    const volatile FieldActor *actors;
    const volatile u8 *code;
    u32 id, cursor;
    int playerId;

    if (useHeight) {
        cursor = fieldTriggerActor()->scriptInstructionPointer;
        code = fieldTriggerScript();
        playerId = *(volatile int *)&g_PlayerActorIndex;
        actors = g_FieldActors;
        id = code[cursor + 1];
        zone = &(*(FieldTriggerZone *volatile *)&g_pFieldTriggerZones)[id];
        player = (const void *)(uintptr_t)actors[playerId].pActorData;
        int y = fieldTriggerCoordinate(&player->position.vy);
        int floor = zone->y0;
        if (floor >= y || y - player->height >= floor)
            return 0;
        z[0] = zone->z0;
        x[0] = zone->x0;
        z[1] = zone->z1;
        x[1] = zone->x1;
        pz = fieldTriggerCoordinate(&player->position.vz);
        px = fieldTriggerCoordinate(&player->position.vx);
        z[2] = zone->z2;
        x[2] = zone->x2;
    } else {
        playerId = *(volatile int *)&g_PlayerActorIndex;
        const volatile ActorData *actor = fieldTriggerActor();
        actors = g_FieldActors;
        cursor = actor->scriptInstructionPointer;
        code = fieldTriggerScript();
        player = (const void *)(uintptr_t)actors[playerId].pActorData;
        pz = fieldTriggerCoordinate(&player->position.vz);
        px = fieldTriggerCoordinate(&player->position.vx);
        id = code[cursor + 1];
        zone = &(*(FieldTriggerZone *volatile *)&g_pFieldTriggerZones)[id];
        z[0] = zone->z0;
        x[0] = zone->x0;
        x[1] = zone->x1;
        x[2] = zone->x2;
        z[1] = zone->z1;
        z[2] = zone->z2;
    }
    z[3] = zone->z3;
    x[3] = zone->x3;
    int corners[4];
    for (int i = 0; i < 4; i++)
        corners[i] = fieldTriggerPoint((s16)x[i], (s16)z[i]);
    int target = fieldTriggerPoint((s16)px, (s16)pz);
    for (int i = 0; i < 4; i++)
        if (NormalClip(corners[i], corners[(i + 1) & 3], target) < 0)
            return 0;
    return 1;
}

static volatile FieldTriggerWord *fieldTriggerStackFlags(volatile ActorData *actor)
{
    return (volatile void *)((volatile u8 *)actor + __builtin_offsetof(ActorData, flags130) - 4);
}

static void fieldCheckTrigger(int useHeight)
{
    volatile int *budget = &g_FieldScriptMaxInstructionCount;
    if (fieldTriggerContains(useHeight)) {
        volatile ActorData *actor = fieldTriggerActor();
        actor->scriptInstructionPointer = (u16)(actor->scriptInstructionPointer + 4);
    } else {
        int target = FieldScriptVMGetInstructionArgument(2);
        u32 before = (u32)*budget;
        fieldTriggerActor()->scriptInstructionPointer = (u16)target;
        *budget = (s32)(before + 1u);
    }
}

static void fieldHandleTrigger(int useHeight)
{
    if (fieldTriggerContains(useHeight)) {
        volatile ActorData *actor = fieldTriggerActor();
        unsigned int depth = (*fieldTriggerStackFlags(actor) >> 6) & 7u;
        if (depth != 4) {
            /* Values 5..7 address the adjoining actor fields in saved state. */
            volatile u8 *stack = (volatile u8 *)actor + __builtin_offsetof(ActorData, scriptPointersStack);
            *(volatile FieldTriggerHalf *)(stack + 2 * depth) = (s16)(u16)(actor->scriptInstructionPointer + 4);
            int target = FieldScriptVMGetInstructionArgument(2);
            actor = fieldTriggerActor();
            actor->scriptInstructionPointer = (u16)target;
            volatile FieldTriggerWord *state = fieldTriggerStackFlags(actor);
            u32 flags = *state;
            *state = (flags & ~0x1c0u) | ((flags + 0x40u) & 0x1c0u);
            return;
        }
    }
    volatile ActorData *actor = fieldTriggerActor();
    volatile int *budget = &g_FieldScriptMaxInstructionCount;
    u32 before = (u32)*budget;
    u16 cursor = actor->scriptInstructionPointer;
    *budget = (s32)(before + 1u);
    actor->scriptInstructionPointer = (u16)(cursor + 4);
}

void FieldScriptCheckTriggerZone(void)    { fieldCheckTrigger(1); }
void FieldScriptCheckTriggerZone2D(void)  { fieldCheckTrigger(0); }
void FieldScriptHandleTriggerZone(void)   { fieldHandleTrigger(1); }
void FieldScriptHandleTriggerZone2D(void) { fieldHandleTrigger(0); }
