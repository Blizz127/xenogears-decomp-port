void FieldInitializeParticlePrimitive(void *, s32, s32);

void FieldParticlesFree(int slot)
{
    if (*(volatile u8 *)&g_FieldParticleStatuses[slot] == 1) {
        ParticleBank *volatile *owner = &g_FieldParticleBanks[slot];
        const volatile ParticleBank *bank = *owner;

        for (int i = 0; i < NUM_PARTICLE_BANKS; ++i) {
            if (bank[i].max != 0)
                HeapFree((void *)(uintptr_t)bank[i].pPrimitives);
        }
        HeapFree(*owner);
    }
    *(volatile u8 *)&g_FieldParticleStatuses[slot] = 0;
    *(volatile s16 *)&g_FieldParticleActorIDs[slot] = -1;
}

void FieldParticlesStopBanks(int slot)
{
    if (*(const volatile u8 *)&g_FieldParticleStatuses[slot] != 1)
        return;

    volatile ParticleBank *banks = *(ParticleBank *volatile *)&g_FieldParticleBanks[slot];
    for (int i = 0; i < NUM_PARTICLE_BANKS; ++i) {
        volatile ParticleBank *bank = &banks[i];
        int count = bank->max;
        if (count == 0)
            continue;
        bank->ewait = 0;
        for (int j = 0; j < count; ++j) {
            u8 *records = (u8 *)(uintptr_t)bank->pPrimitives;
            size_t offset = (size_t)j * sizeof(ParticlePrimitive) + __builtin_offsetof(ParticlePrimitive, ewait);
            *(volatile u16 *)(records + offset) = 1;
            count = bank->max;
        }
    }
}

/* The observed 16-byte copies first read four words, then write the four words.
 * The allocation and default bank table both have word alignment. */
typedef u32 FieldParticleCopyWord __attribute__((__may_alias__));

static void fieldCopyDefaultParticleBanks(ParticleBank *banks)
{
    const u8 *source = (const u8 *)&g_FieldDefaultParticleBanks;
    u8 *target = (u8 *)banks;
    for (size_t offset = 0; offset < sizeof(g_FieldDefaultParticleBanks); offset += 16) {
        const volatile FieldParticleCopyWord *in = (const void *)(source + offset);
        volatile FieldParticleCopyWord *out = (void *)(target + offset);
        u32 a = in[0];
        u32 b = in[1];
        u32 c = in[2];
        u32 d = in[3];
        out[0] = a;
        out[1] = b;
        out[2] = c;
        out[3] = d;
    }
}

int FieldInitializeParticleBanks(int actorIndex)
{
    int slot = FieldParticlesFindFreeIndex();
    if (slot == -1)
        return -1;

    HeapChangeCurrentUser(8, NULL);
    *(volatile int *)&g_FieldParticleCurActor = actorIndex;
    *(volatile u8 *)&g_FieldParticleStatuses[slot] = 1;
    *(volatile s16 *)&g_FieldParticleActorIDs[slot] = (s16)actorIndex;
    ParticleBank *banks = HeapAlloc(sizeof(g_FieldDefaultParticleBanks), 0);
    *(ParticleBank *volatile *)&g_FieldParticleBanks[slot] = banks;
    fieldCopyDefaultParticleBanks(banks);

    for (int i = 0; i < NUM_PARTICLE_BANKS; ++i) {
        volatile ParticleBank *bank = &banks[i];
        int count = bank->max;
        if (!count)
            continue;
        void *memory = HeapAlloc((u32)count * (u32)sizeof(ParticlePrimitive), 0);
        count = bank->max;
        bank->pPrimitives = (u32)(uintptr_t)memory;
        for (int j = 0; j < count; ++j) {
            ParticlePrimitive *particles = (void *)(uintptr_t)bank->pPrimitives;
            *(volatile s16 *)&particles[j].active = 0;
            int shape = bank->shape;
            particles = (void *)(uintptr_t)bank->pPrimitives;
            u16 flags = bank->flags;
            FieldInitializeParticlePrimitive(&particles[j], shape, ((flags >> 8) + 1) & 3);
            count = bank->max;
        }
    }
    return 1;
}
