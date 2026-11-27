/* The script samples the signed upper halfwords of the fixed-point coordinates. */
typedef s16 FieldDoorHalfword __attribute__((__may_alias__));

static void FieldScriptTranslateDoor(int opening)
{
    extern void FieldDoorPlaySound(int, int) __asm__("func_80085634");
    extern void FieldDoorRefreshMatrix(s32) __asm__("func_80072254");
    extern s32 FieldScriptVMGetArgument(s32);
    extern int rsin(int), rcos(int);
    extern s32 D_800AFD1C;
    extern FieldActor *D_800B06B8;

    const u32 open = 1u << 20;
    const u32 moving = 1u << 5;
    ActorData *volatile *current = &g_FieldScriptVMCurActor;
    FieldActor *volatile *currentRender = &D_800B06B8;
    volatile ActorData *actor = *current;
    u32 flags = actor->scriptFlags.flags;

    if (((flags & open) != 0) == opening) {
        actor->scriptInstructionPointer += 7u;
    } else {
        volatile u32 *phase = (volatile u32 *)((volatile u8 *)actor + 0x12c);
        u32 state = *phase;
        if (!(state & moving)) {
            *phase = state | moving;
            actor->curDoorStep = 0;
            FieldDoorPlaySound(8, 3);
            if (opening) {
                actor = *current;
                s32 x = actor->position.vx;
                s32 y = actor->position.vy;
                s32 z = actor->position.vz;
                actor->unkD0.vx = x;
                actor->unkD0.vy = y;
                actor->unkD0.vz = z;
            }
        } else {
            ++actor->curDoorStep;
            actor = *current;
            int elapsed = actor->curDoorStep;
            int duration = FieldScriptVMGetArgument(3);
            if (elapsed >= duration) {
                actor = *current;
                flags = actor->scriptFlags.flags;
                actor->curDoorStep = 0;
                volatile ActorData *cursorActor;
                phase = (volatile u32 *)((volatile u8 *)actor + 0x12c);
                if (opening) {
                    actor->scriptFlags.flags = flags | open;
                    state = *phase;
                    cursorActor = *current;
                } else {
                    cursorActor = *current;
                    actor->scriptFlags.flags = flags & ~open;
                    state = *phase;
                }
                *phase = state & ~moving;
                cursorActor->scriptInstructionPointer += 7u;
            } else {
                int direction = FieldScriptVMGetArgument(5);
                if (direction == 0x1000 || direction == 0x1001) {
                    u32 distance = (u32)FieldScriptVMGetArgument(1) << 4;
                    actor = *current;
                    volatile FieldActor *render = *currentRender;
                    u32 y = (u32)actor->unkD0.vy;
                    s16 z = ((const volatile FieldDoorHalfword *)&actor->unkD0.vz)[1];
                    actor->unkD0.vy = (s32)(direction == 0x1000 ? y - distance : y + distance);
                    render->transformMatrix.t[1] = z;
                } else {
                    u32 angleOffset = (u32)FieldScriptVMGetArgument(5);
                    volatile FieldActor *render = *currentRender;
                    s32 rotation = render->rotation.y;
                    int angle = (s32)((u32)rotation + angleOffset - 1024u);
                    u32 sine = (u32)rsin(angle);
                    u32 distance = (u32)FieldScriptVMGetArgument(1);
                    actor = *current;
                    u32 x = (u32)actor->unkD0.vx;
                    u32 dx = sine * distance;
                    actor->unkD0.vx = (s32)(opening ? x + dx : x - dx);

                    u32 cosine = (u32)rcos(angle);
                    distance = (u32)FieldScriptVMGetArgument(1);
                    actor = *current;
                    render = *currentRender;
                    u32 z = (u32)actor->unkD0.vz;
                    s16 renderX = ((const volatile FieldDoorHalfword *)&actor->unkD0.vx)[1];
                    u32 dz = cosine * distance;
                    actor->unkD0.vz = (s32)(opening ? z - dz : z + dz);
                    render->transformMatrix.t[0] = renderX;
                    s16 renderZ = ((const volatile FieldDoorHalfword *)&actor->unkD0.vz)[1];
                    render->transformMatrix.t[2] = renderZ;
                }
            }
        }
    }
    FieldDoorRefreshMatrix(*(volatile s32 *)&D_800AFD1C);
}

void func_80094158(void)
{
    FieldScriptTranslateDoor(1);
}

void func_800943AC(void)
{
    FieldScriptTranslateDoor(0);
}
