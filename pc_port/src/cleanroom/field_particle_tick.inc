void FieldParticleStart(ParticleBank *, ParticlePrimitive *, int *);
void FieldParticleUpdateAndRender(ParticleBank *, ParticlePrimitive *, MATRIX *);
void func_80281B00(void *);

typedef u32 FieldParticleMatrixWord __attribute__((__may_alias__));

static void fieldParticleViewMatrix(MATRIX *target)
{
    const volatile FieldParticleMatrixWord *source = (const void *)&g_Scene.worldToScreenMatrix;
    volatile FieldParticleMatrixWord *dest = (void *)target;
    for (int i = 0; i < 6; i += 3) {
        u32 a = source[i];
        u32 b = source[i + 1];
        u32 c = source[i + 2];
        dest[i] = a;
        dest[i + 1] = b;
        dest[i + 2] = c;
    }
    u32 a = source[6];
    u32 b = source[7];
    dest[6] = a;
    dest[7] = b;
}

void FieldParticlesTickAndRender(void)
{
    if (*(volatile int *)&D_800ADB34 != 0)
        return;
    MATRIX view;
    fieldParticleViewMatrix(&view);
    for (int slot = 0; slot < NUM_PARTICLES; ++slot) {
        if (*(const volatile u8 *)&g_FieldParticleStatuses[slot] != 1)
            continue;

        ParticleBank *banks = *(ParticleBank *volatile *)&g_FieldParticleBanks[slot];
        int retain = 0;
        for (int i = 0; i < NUM_PARTICLE_BANKS; ++i) {
            ParticleBank *bank = &banks[i];
            volatile ParticleBank *state = bank;
            int started = 0;
            int count = state->max;
            if (!count)
                continue;
            unsigned int wait = state->swait;
            if (wait) {
                state->swait = (u16)(wait - 1);
                retain = 1;
                continue;
            }
            for (int j = 0; j < count; ++j) {
                u8 *array = (void *)(uintptr_t)state->pPrimitives;
                size_t primitiveOffset = (size_t)j * sizeof(ParticlePrimitive);
                ParticlePrimitive *primitive = (void *)(array + primitiveOffset);
                if (*(volatile s16 *)(array + primitiveOffset) != 0) {
                    FieldParticleUpdateAndRender(bank, primitive, &view);
                    retain = 1;
                } else if (state->ewait != 0) {
                    FieldParticleStart(bank, primitive, &started);
                    array = (void *)(uintptr_t)state->pPrimitives;
                    FieldParticleUpdateAndRender(bank, (void *)(array + primitiveOffset), &view);
                    retain = 1;
                }
                count = state->max;
            }
            unsigned int duration = state->ewait;
            if (duration) {
                if (duration != 32767)
                    state->ewait = (u16)(duration - 1);
                retain = 1;
            }
        }
        if (!retain)
            FieldParticlesFree(slot);
    }
    if (*(const volatile int *)&g_FieldSystemMode == 0)
        func_80281B00(D_8006FDC8);
}
