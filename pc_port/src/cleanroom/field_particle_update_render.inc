#include <inline_c.h>

long VectorNormal(VECTOR *, VECTOR *);
MATRIX *RotMatrixZYX(SVECTOR *, MATRIX *);

static s32 fieldParticleVelocity(s32 unit, s32 speed, s16 multiplier)
{
    s32 scaled = (s32)((u32)unit * (u32)speed) >> 12;
    return (s32)((u32)scaled * (u32)(s32)multiplier);
}

static s32 fieldParticleScaleAxis(s32 value, s16 scale)
{
    return (s32)((u32)value * (u32)(s32)scale) >> 12;
}

static s32 fieldParticleCombine(s32 base, s32 offset, s32 factor)
{
    return (s32)(((u32)base + (u32)offset) * (u32)factor);
}

typedef s16 FieldParticlePositionHalf __attribute__((__may_alias__));

static const volatile ActorData *fieldParticleActor(const volatile FieldActor *records, int index)
{
    return (const void *)(uintptr_t)records[index].pActorData;
}

static int fieldParticleActorCoordinate(const volatile int *fixed)
{
    return ((const volatile FieldParticlePositionHalf *)fixed)[1];
}

static void fieldParticlePlace(ParticleBank *source, ParticlePrimitive *destination)
{
    volatile ParticleBank *bank = source;
    volatile ParticlePrimitive *particle = destination;
    MATRIX basis = {0}, temporary = {0};
    VECTOR base, rotated;
    SVECTOR input = {0};
    long flag = 0;
    unsigned int mode = (bank->flags >> 4) & 3;

    if (mode == 0 || mode == 3) {
        int index = bank->targetActorID;
        const volatile ActorData *actor = fieldParticleActor(g_FieldActors, index);
        SVECTOR yaw = {0, actor->rotation.vz, 0, 0};
        RotMatrix(&yaw, &basis);
        index = bank->targetActorID;
        const volatile FieldActor *records = g_FieldActors;
        base.vx = fieldParticleActorCoordinate(&fieldParticleActor(records, index)->position.vx);
        base.vy = fieldParticleActorCoordinate(&fieldParticleActor(records, bank->targetActorID)->position.vy);
        base.vz = fieldParticleActorCoordinate(&fieldParticleActor(records, bank->targetActorID)->position.vz);
        bank->unk50 = mode == 3 ? (s16)fieldParticleActor(records, bank->targetActorID)->scaleX : 4096;
    } else {
        if (mode == 1) {
            func_801E72CC(&basis, &temporary, bank->unk72, bank->unk74);
        } else {
            basis = g_FieldActors[bank->targetActorID].childMatrix;
        }
        SetRotMatrix(&basis);
        SetTransMatrix(&basis);
        input.vx = bank->pos.vx;
        input.vy = bank->pos.vy;
        input.vz = bank->pos.vz;
        RotTrans(&input, &base, &flag);
        bank->unk50 = 4096;
    }

    basis.t[0] = 0;
    basis.t[1] = 0;
    basis.t[2] = 0;
    SetRotMatrix(&basis);
    SetTransMatrix(&basis);
    input.vx = (s16)particle->velocity.vx;
    input.vy = (s16)particle->velocity.vy;
    input.vz = (s16)particle->velocity.vz;
    ApplyRotMatrix(&input, &rotated);
    /* The resident rotation service returns the saturated GTE IR components.
     * PsyCross also leaves this result in IR1..3 when it stores the wide MAC. */
    rotated.vx = (s16)MFC2(9);
    rotated.vy = (s16)MFC2(10);
    rotated.vz = (s16)MFC2(11);
    VectorNormal(&rotated, &destination->velocity);
    particle->velocity.vx = fieldParticleVelocity(particle->velocity.vx, bank->speed, bank->speedMultiplier);
    particle->velocity.vy = fieldParticleVelocity(particle->velocity.vy, bank->speed, bank->speedMultiplier);
    particle->velocity.vz = fieldParticleVelocity(particle->velocity.vz, bank->speed, bank->speedMultiplier);
    if (mode == 3) {
        particle->position.vx = fieldParticleScaleAxis(particle->position.vx, bank->unk50);
        particle->position.vy = fieldParticleScaleAxis(particle->position.vy, bank->unk50);
        particle->position.vz = fieldParticleScaleAxis(particle->position.vz, bank->unk50);
    }
    SetRotMatrix(&basis);
    SetTransMatrix(&basis);
    input.vx = (s16)particle->position.vx;
    input.vy = (s16)particle->position.vy;
    input.vz = (s16)particle->position.vz;
    RotTrans(&input, &rotated, &flag);

    if (mode == 3) {
        SVECTOR view = {(s16)((u32)D_800B00B4 - 1024u), (s16)(-(int)g_CamInterpolation.curAngleY), 0, 0};
        VECTOR vertical = {0, rotated.vy, 0, 0}, extra;
        /* Rotation leaves the translation words untouched. The console loaded
         * its previous stack occupants here; the rotation-only operation uses
         * no translation. Native temporaries have a defined initial value. */
        RotMatrixZYX(&view, &temporary);
        SetRotMatrix(&temporary);
        SetTransMatrix(&temporary);
        ApplyRotMatrixLV(&vertical, &extra);
        rotated.vx = (s32)((u32)rotated.vx + (u32)extra.vx);
        rotated.vy = extra.vy;
        rotated.vz = (s32)((u32)rotated.vz + (u32)extra.vz);
    }
    s32 factor = 4096;
    if (mode == 3) {
        s32 scale = bank->unk50;
        factor = scale ? 0x1000000 / scale : -1;
    }
    particle->position.vx = fieldParticleCombine(base.vx, rotated.vx, factor);
    if (mode == 3) {
        s32 scale = bank->unk50;
        factor = scale ? 0x1000000 / scale : -1;
    }
    particle->position.vy = fieldParticleCombine(base.vy, rotated.vy, factor);
    if (mode == 3) {
        s32 scale = bank->unk50;
        factor = scale ? 0x1000000 / scale : -1;
    }
    particle->position.vz = fieldParticleCombine(base.vz, rotated.vz, factor);
}

void FieldParticleUpdateAndRender(ParticleBank *source, ParticlePrimitive *destination, MATRIX *camera)
{
    volatile ParticleBank *bank = source;
    volatile ParticlePrimitive *particle = destination;
    unsigned int delay = particle->swait;
    if (delay) {
        particle->swait = (u16)(delay - 1u);
        if (delay == 1)
            fieldParticlePlace(source, destination);
        return;
    }

    int red = particle->color.r;
    int delta = particle->colorDelta.r;
    particle->velocity.vx = (s32)((u32)particle->velocity.vx + (u32)particle->gravity.vx);
    particle->velocity.vy = (s32)((u32)particle->velocity.vy + (u32)particle->gravity.vy);
    particle->velocity.vz = (s32)((u32)particle->velocity.vz + (u32)particle->gravity.vz);
    particle->scale.vy = (s16)(particle->scale.vy + particle->scaleDelta.vy);
    particle->scale.vx = (s16)(particle->scale.vx + particle->scaleDelta.vx);
    particle->scale.vz = (s16)(particle->scale.vz + particle->scaleDelta.vz);
    particle->position.vx = (s32)((u32)particle->position.vx + (u32)particle->velocity.vx);
    particle->position.vy = (s32)((u32)particle->position.vy + (u32)particle->velocity.vy);
    particle->position.vz = (s32)((u32)particle->position.vz + (u32)particle->velocity.vz);
    int next = FieldParticleUpdateColor(red, delta);
    int green = particle->color.g;
    delta = particle->colorDelta.g;
    particle->color.r = (u8)next;
    next = FieldParticleUpdateColor(green, delta);
    int blue = particle->color.b;
    delta = particle->colorDelta.b;
    particle->color.g = (u8)next;
    particle->color.b = (u8)FieldParticleUpdateColor(blue, delta);

    VECTOR scale;
    scale.vx = bank->unk50;
    scale.vy = bank->unk50;
    scale.vz = bank->unk50;
    if (particle->ewait != 1) {
        int sort = (bank->flags >> 1) & 3;
        int type = (bank->flags >> 4) & 3;
        FieldParticleRender(destination, camera, particle->zRotation, sort, &scale, type);
    }
    unsigned int remaining = (u16)(particle->ewait - 1);
    particle->ewait = (u16)remaining;
    if (!remaining)
        particle->active = 0;
}
