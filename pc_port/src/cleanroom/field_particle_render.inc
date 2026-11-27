void FieldMatrixCopyTransform(MATRIX *, MATRIX *);

typedef u32 FieldRenderMatrixWord __attribute__((__may_alias__));

#include "packed_matrix_column_scale.inc"

static void fieldParticleCopyCamera(MATRIX *out, const MATRIX *in)
{
    const volatile FieldRenderMatrixWord *source = (const void *)in;
    volatile FieldRenderMatrixWord *dest = (void *)out;
    for (int at = 0; at < 8; at += 4) {
        u32 a = source[at];
        u32 b = source[at + 1];
        u32 c = source[at + 2];
        u32 d = source[at + 3];
        dest[at] = a;
        dest[at + 1] = b;
        dest[at + 2] = c;
        dest[at + 3] = d;
    }
}

static POLY_FT4 *fieldParticleCurrentPoly(ParticlePrimitive *particle)
{
    return &particle->poly[*(const volatile int *)&g_FieldCurRenderContextIndex];
}

void FieldParticleRender(ParticlePrimitive *particle, MATRIX *camera, s16 angle,
                         s32 sort, VECTOR *extraScale, s32 type)
{
    SVECTOR rotation = { 0, 0, angle, 0 };
    MATRIX local, composed, scaledView;
    RotMatrix(&rotation, &local);
    local.t[0] = particle->position.vx >> 12;
    local.t[1] = particle->position.vy >> 12;
    local.t[2] = particle->position.vz >> 12;
    if (type == 3) {
        fieldParticleCopyCamera(&scaledView, camera);
        packedMatrixScaleColumns(&scaledView, extraScale);
        camera = &scaledView;
    }
    CompMatrix(camera, &local, &composed);
    FieldMatrixCopyTransform(&composed, &local);

    VECTOR scale = {particle->scale.vx, particle->scale.vy, particle->scale.vz, 0};
    packedMatrixScaleColumns(&composed, &scale);
    fieldParticleCurrentPoly(particle)->r0 = particle->color.r;
    fieldParticleCurrentPoly(particle)->g0 = particle->color.g;
    fieldParticleCurrentPoly(particle)->b0 = particle->color.b;

    SetTransMatrix(&composed);
    if (type == 3)
        packedMatrixScaleColumns(&composed, extraScale);
    SetRotMatrix(&composed);

    POLY_FT4 *poly = fieldParticleCurrentPoly(particle);
    long flag = 0;
    /* This service writes 32-bit packed SXY coordinates on either host ABI.
     * Its final two outputs share one native long, as in the field call. */
    s32 z = (s32)RotAverage4(&particle->vertices[0], &particle->vertices[1],
                             &particle->vertices[2], &particle->vertices[3],
                             (void *)&poly->x0, (void *)&poly->x1,
                             (void *)&poly->x2, (void *)&poly->x3,
                             &flag, &flag);
    s32 depth = z >> ((u32)D_80050100 & 31u);
    u32 bucket = (u32)flag;
    switch (sort) {
    case PARTICLE_SORT_TOP:    bucket = 1; break;
    case PARTICLE_SORT_MID:    bucket = (u32)depth - 16u; break;
    case PARTICLE_SORT_NORMAL: bucket = (u32)depth; break;
    case PARTICLE_SORT_BACK:   bucket = (u32)depth + 16u; break;
    }
    if (bucket > 0 && bucket < 4096) {
        poly = fieldParticleCurrentPoly(particle);
        u32 *head = FIELD_CTX_OT1(g_FieldCurRenderContext) + bucket;
        addPrim(head, poly);
    }
}
