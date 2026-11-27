/* Game-side GTE names follow the resident entry points. The include shim
 * translates them to the corresponding mathematical PsyCross functions. */
static int fieldParticleRadialOffset(int angleValue, unsigned int radius)
{
    return (s32)((u32)angleValue * radius) >> 12;
}

void FieldParticleStart(ParticleBank *source, ParticlePrimitive *destination, int *delay)
{
    const volatile ParticleBank *bank = source;
    volatile ParticlePrimitive *particle = destination;
    volatile int *clock = delay;

    particle->active = 1;
    unsigned int prior = (u32)*clock;
    particle->swait = (u16)(prior + bank->pswait);
    unsigned int interval = bank->pswait;
    *clock = (int)(interval + (u32)*clock);
    particle->ewait = bank->pewait;
    if (bank->flags & 1)
        particle->zRotation = (short)(rand() & 4095);
    else
        particle->zRotation = bank->rotAngle;

    unsigned int radius = (bank->flags & 0x80) ? bank->srange
                                               : (unsigned int)FieldParticlesRandRange(bank->srange);
    int angle = FieldParticlesRandRange(4095);
    int startX = fieldParticleRadialOffset(rsin(angle), radius);
    int startZ = 0;
    if (!(bank->flags & 0x40))
        startZ = fieldParticleRadialOffset(rcos(angle), radius);

    int actorIndex = bank->targetActorID;
    const volatile ActorData *actor = (const void *)(uintptr_t)g_FieldActors[actorIndex].pActorData;
    int heading = g_CamInterpolation.curAngleY + actor->rotation.vz;
    unsigned int direction = D_800AF474[((unsigned int)heading & 4095u) >> 9];
    startX += bank->pos.vx + bank->directions[direction].x;
    startZ += bank->pos.vz + bank->directions[direction].z;
    int startY = bank->pos.vy;
    particle->position.vx = startX;
    particle->position.vz = startZ;
    particle->position.vy = startY;

    radius = (unsigned int)FieldParticlesRandRange(bank->erange);
    int endX = fieldParticleRadialOffset(rsin(angle), radius) + bank->epos.vx;
    int endZ = fieldParticleRadialOffset(rcos(angle), radius) + bank->epos.vz;
    particle->velocity.vx = (s32)((u32)endX - (u32)startX);
    particle->velocity.vy = (s32)((u32)(s32)bank->epos.vy - (u32)startY);
    particle->velocity.vz = (s32)((u32)endZ - (u32)startZ);
    particle->gravity.vx = bank->gravity.vx;
    particle->gravity.vy = bank->gravity.vy;
    particle->gravity.vz = bank->gravity.vz;
    particle->scale.vx = bank->scale.vx;
    particle->scale.vy = bank->scale.vy;
    particle->scale.vz = bank->scale.vz;
    particle->scaleDelta.vx = bank->scaleDelta.vx;
    particle->scaleDelta.vy = bank->scaleDelta.vy;
    particle->scaleDelta.vz = bank->scaleDelta.vz;
    particle->color.r = bank->color.r;
    particle->color.g = bank->color.g;
    particle->color.b = bank->color.b;
    particle->colorDelta.r = bank->colorDelta.r;
    particle->colorDelta.g = bank->colorDelta.g;
    particle->colorDelta.b = bank->colorDelta.b;
}
