typedef u32 FieldMoveSlotWord __attribute__((__may_alias__));
typedef u16 FieldMoveSlotHalf __attribute__((__may_alias__));
typedef s16 FieldMoveSlotCoord __attribute__((__may_alias__));

static uintptr_t field_move_slot_address(volatile ActorData *actor) {
    return (uintptr_t)actor + 0x90u + 8u * actor->curScriptIndex;
}

void func_800976A8(void) {
    volatile ActorData *actor = g_FieldScriptVMCurActor;
    volatile FieldMoveSlotWord *word = (volatile FieldMoveSlotWord *)field_move_slot_address(actor);
    u32 state = *word;

    if ((state & (3u << 23)) == 0) {
        *word = state | (1u << 23);
        uintptr_t position = (uintptr_t)&actor->position;
        s32 x = *(const volatile FieldMoveSlotCoord *)(position + 2);
        s32 y = *(const volatile FieldMoveSlotCoord *)(position + 6);
        s32 z = *(const volatile FieldMoveSlotCoord *)(position + 10);
        actor->unkD0.vx = x;
        actor->unkD0.vy = y;
        actor->unkD0.vz = z;
    }

    actor = *(ActorData *volatile *)&g_FieldScriptVMCurActor;
    volatile FieldMoveSlotHalf *value = (volatile FieldMoveSlotHalf *)field_move_slot_address(actor);
    if (*value == 0xffffu) {
        u16 argument = (u16)FieldScriptVMGetArgument(8);
        actor = *(ActorData *volatile *)&g_FieldScriptVMCurActor;
        value = (volatile FieldMoveSlotHalf *)field_move_slot_address(actor);
        *value = argument;
    }

    s32 target = FieldScriptVMGetArgument(8);
    if (func_80097A50(target) == 0) {
        g_FieldScriptVMCurActor->scriptInstructionPointer += 10;
    }
}
