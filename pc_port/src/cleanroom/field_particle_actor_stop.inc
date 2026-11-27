void FieldParticleActorStop(int actorIndex, int mode)
{
    const volatile short *actorIDs = g_FieldParticleActorIDs;
    int slot;

    for (slot = 0; slot < NUM_PARTICLES; ++slot) {
        if (actorIDs[slot] == actorIndex) {
            ParticleBank *const volatile *bankSlot = &g_FieldParticleBanks[slot];
            volatile ParticleBank *bank = *bankSlot;

            bank->ewait = 0;
            bank = *bankSlot;
            bank->swait = 0;

            if (mode == 0) {
                FieldParticlesStop(slot);
            } else {
                FieldParticlesStopBanks(slot);
            }
        }
    }
}
