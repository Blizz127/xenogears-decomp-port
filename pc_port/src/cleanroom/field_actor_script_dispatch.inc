/* Independent host implementations of actor-to-actor field script commands. */
extern u32 FieldScriptVMGetActorIndex(int);
extern u16 FieldScriptGetBytecodeOffset(int, int);

enum FieldDispatchCommand {
    FIELD_DISPATCH_START,
    FIELD_DISPATCH_WAIT_START,
    FIELD_DISPATCH_WAIT_END
};

static unsigned char *field_dispatch_slot(ActorData *actor, unsigned index) {
    uintptr_t base = (uintptr_t)actor + __builtin_offsetof(ActorData, scripts);
    return (unsigned char *)(base + index * sizeof(ActorScriptSlot));
}

static unsigned field_dispatch_status(const unsigned char *slot) {
    u32 word;
    __builtin_memcpy(&word, slot + 4, sizeof(word));
    return word;
}

static void field_dispatch_bits(unsigned char *slot, u32 mask, u32 bits) {
    u32 next = (field_dispatch_status(slot) & ~mask) | bits;
    __builtin_memcpy(slot + 4, &next, sizeof(next));
}

static unsigned char *field_dispatch_current_slot(void) {
    ActorData *actor = g_FieldScriptVMCurActor;
    return field_dispatch_slot(actor, actor->curScriptIndex);
}

static unsigned field_dispatch_command_byte(void) {
    const unsigned char *script = g_FieldScriptVMCurScriptData;
    return script[(unsigned)g_FieldScriptVMCurActor->scriptInstructionPointer + 2u];
}

static void field_dispatch_advance(void) {
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

static void field_dispatch_clear(ActorData *target) {
    field_dispatch_bits(field_dispatch_current_slot(), 3u << 16, 0);
    field_dispatch_bits(field_dispatch_slot(target, g_FieldScriptVMCurActor->unkCF), 1u << 22, 0);
}

static void field_dispatch_start(enum FieldDispatchCommand command, ActorData *target, u32 id) {
    unsigned index;
    if (FieldActorGetScriptStatus(target, field_dispatch_command_byte() & 31u) == -1) {
        field_dispatch_advance();
        return;
    }
    for (index = 0; index < 8; ++index) {
        unsigned char *slot = field_dispatch_slot(target, index);
        u16 entry;
        if ((field_dispatch_status(slot) & (31u << 18)) != (15u << 18))
            continue;
        entry = FieldScriptGetBytecodeOffset((int)id, field_dispatch_command_byte() & 31u);
        __builtin_memcpy(slot, &entry, sizeof(entry));
        field_dispatch_bits(slot, 15u << 18, (field_dispatch_command_byte() >> 5) << 18);
        if (command == FIELD_DISPATCH_START) {
            slot[3] = field_dispatch_command_byte() & 31u;
            field_dispatch_advance();
            return;
        }
        field_dispatch_bits(field_dispatch_slot(target, g_FieldScriptVMCurActor->unkCF), 1u << 22, 1u << 22);
        if (command == FIELD_DISPATCH_WAIT_START)
            slot[3] = field_dispatch_command_byte() & 31u;
        g_FieldScriptVMCurActor->unkCF = index;
        field_dispatch_bits(field_dispatch_current_slot(), 3u << 16, 1u << 16);
        if (command == FIELD_DISPATCH_WAIT_END)
            slot[3] = field_dispatch_command_byte() & 31u;
        return;
    }
}

static void field_actor_dispatch(enum FieldDispatchCommand command) {
    u32 id;
    u32 actor_address;
    u32 actor_flags;
    unsigned phase, remembered;
    int idle;
    ActorData *target;
    uintptr_t record;
    if (FieldScriptVMGetActorIndex(1) == 255u) {
        field_dispatch_advance();
        return;
    }
    id = FieldScriptVMGetActorIndex(1);
    record = (uintptr_t)g_FieldActors + (intptr_t)(s32)(id * (u32)sizeof(FieldActor));
    __builtin_memcpy(&actor_address, (const void *)(record + __builtin_offsetof(FieldActor, pActorData)), 4);
    target = (ActorData *)(uintptr_t)actor_address;
    __builtin_memcpy(&actor_flags, (const unsigned char *)target + 4, 4);
    if (actor_flags & (1u << 20)) {
        field_dispatch_clear(target);
        field_dispatch_advance();
        return;
    }
    if (command == FIELD_DISPATCH_START) {
        field_dispatch_start(command, target, id);
        return;
    }
    phase = (field_dispatch_status(field_dispatch_current_slot()) >> 16) & 3u;
    if (!phase) {
        field_dispatch_start(command, target, id);
        return;
    }
    if (phase == 3 || (phase == 2 && command == FIELD_DISPATCH_WAIT_START))
        return;
    remembered = g_FieldScriptVMCurActor->unkCF;
    if (phase == 1 && target->curScriptIndex == remembered)
        idle = 1;
    else
        idle = (field_dispatch_status(field_dispatch_slot(target, remembered)) & (15u << 18)) == (15u << 18);
    if (!idle) {
        D_800B00C0 = 1;
        return;
    }
    if (command == FIELD_DISPATCH_WAIT_START) {
        field_dispatch_advance();
        field_dispatch_clear(target);
    } else if (phase == 1) {
        field_dispatch_bits(field_dispatch_current_slot(), 3u << 16, 2u << 16);
    } else {
        field_dispatch_clear(target);
        field_dispatch_advance();
    }
}

void func_8009EB78(void) { field_actor_dispatch(FIELD_DISPATCH_START); }
void func_8009ED68(void) { field_actor_dispatch(FIELD_DISPATCH_WAIT_START); }
void func_8009F0A0(void) { field_actor_dispatch(FIELD_DISPATCH_WAIT_END); }
