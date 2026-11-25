void FieldInitializeDefaultParticleBanks(s32 actorIndex)
{
    int i;

    g_FieldParticleBankIndex = 0;
    for (i = 0; i < NUM_PARTICLE_BANKS; ++i) {
        ParticleBank *bank = &g_FieldDefaultParticleBanks[i];
        int j;

        bank->unk0 = 0;
        bank->swait = 0;
        bank->ewait = 128;
        bank->max = 0;
        bank->speed = 32768;
        bank->pos.vx = 0;
        bank->pos.vy = 0;
        bank->pos.vz = 0;
        bank->epos.vx = 0;
        bank->epos.vy = -1000;
        bank->epos.vz = 0;
        bank->gravity.vx = 0;
        bank->gravity.vy = 0;
        bank->gravity.vz = 0;
        bank->speedMultiplier = 1;
        bank->srange = 0;
        bank->erange = 256;
        bank->flags = 0;
        for (j = 0; j < 8; ++j) {
            bank->directions[j].x = 0;
            bank->directions[j].z = 0;
        }
        bank->unk50 = 2048;
        bank->targetActorID = (short)actorIndex;
        bank->shape = 0;
        bank->pswait = 1;
        bank->pewait = 28;
        bank->scale.vx = 456;
        bank->scale.vy = 456;
        bank->scale.vz = 456;
        bank->scaleDelta.vx = 32;
        bank->scaleDelta.vy = 32;
        bank->scaleDelta.vz = 32;
        bank->color.r = 128;
        bank->color.g = 32;
        bank->color.b = 0;
        bank->colorDelta.r = -4;
        bank->colorDelta.g = -1;
        bank->colorDelta.b = 0;
        bank->rotAngle = 0;
    }
}
