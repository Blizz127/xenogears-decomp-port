#ifndef XENO_FIELD_SCRIPT_PROJECTED_BOUNDS_H
#define XENO_FIELD_SCRIPT_PROJECTED_BOUNDS_H

static inline __attribute__((always_inline))
void fieldScriptTestProjectedBounds(u32 minimum, u32 rows, u32 columns)
{
    s32 x, y;
    FieldProjectActorOriginToScreen(&x, &y);

    s32 skipBounds = *(const volatile s32 *)&D_800ADC18;
    ActorData *volatile *current = &g_FieldScriptVMCurActor;
    /* The original tests Y first and reads X only when Y is in range. */
    if (skipBounds || ((u32)*(const volatile s32 *)&y - minimum < rows &&
                       (u32)*(const volatile s32 *)&x - minimum < columns)) {
        volatile ActorData *actor = *current;
        actor->scriptInstructionPointer += 4u;
    } else {
        u16 destination = (u16)FieldScriptVMGetInstructionArgument(2);
        volatile ActorData *actor = *current;
        actor->scriptInstructionPointer = destination;
    }
    if (!skipBounds)
        *(volatile s32 *)&D_800B00C0 = 1;
}

#endif
