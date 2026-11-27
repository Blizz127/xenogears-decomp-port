extern int rand(void);
void FieldParticlesFree(int index);
void FieldRenderSync(void);

static inline void SetVector(SVECTOR *target, short value)
{
    volatile SVECTOR *vector = target;
    vector->vx = value;
    vector->vy = value;
    vector->vz = value;
}

void FieldInitializeParticles(void)
{
    for (int slot = 0; slot < NUM_PARTICLES; slot++) {
        *(volatile u8 *)&g_FieldParticleStatuses[slot] = 0;
        *(volatile s16 *)&g_FieldParticleActorIDs[slot] = -1;
    }
}

void FieldParticlesStop(int index)
{
    if (*(const volatile u8 *)&g_FieldParticleStatuses[index] != 1)
        return;
    volatile ParticleBank *banks = *(ParticleBank *volatile *)&g_FieldParticleBanks[index];
    for (int i = 0; i < NUM_PARTICLE_BANKS; i++) {
        if (banks[i].max != 0)
            banks[i].ewait = 0;
    }
}

void FieldParticlesFreeAll(void)
{
    for (int slot = 0; slot < NUM_PARTICLES; slot++)
        FieldParticlesFree(slot);
    FieldRenderSync();
}

int FieldParticlesRandRange(int range)
{
    u32 value = (u32)rand() * (u32)range + 1u;
    return (value & 0x80000000u) ? -1 - (int)(~value >> 15) : (int)(value >> 15);
}

int FieldParticlesFindFreeIndex(void)
{
    for (int slot = 0; slot < NUM_PARTICLES; slot++) {
        if (*(const volatile u8 *)&g_FieldParticleStatuses[slot] == 0)
            return slot;
    }
    return -1;
}

int FieldParticleUpdateColor(int color, int delta)
{
    u32 bits = (u32)color + (u32)delta;
    int sum = (bits & 0x80000000u) ? -1 - (int)~bits : (int)bits;
    if (delta < 0)
        return sum < 0 ? 0 : sum;
    return sum > 255 ? 255 : sum;
}
