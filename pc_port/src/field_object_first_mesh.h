#ifndef XENO_FIELD_OBJECT_FIRST_MESH_H
#define XENO_FIELD_OBJECT_FIRST_MESH_H

#include <stddef.h>
#include "common.h"

/* Live registered attachment and sprite-pool owners held by file 10. */
typedef struct PcFieldGearActorTail {
    const void *trail, *emitter, *auxEntries;
    int (*ownsBlock)(const void *, size_t);
} PcFieldGearActorTail;

typedef struct PcFieldFirstMesh {
    u8 *group, *packets;
    unsigned depthWords;
    s32 (*draw)(u8 *group, u8 *packets, u32 *depth, s32 mode);
    s32 drawn;
    unsigned nodeIndex, groupIndex;
    struct PcFieldFirstMesh *next;
    /* Only the final authenticated draw can grant the actor postlude. */
    const PcFieldGearActorTail *completeActor;
    s32 actorCompleted;
    unsigned gearId; /* File 10 selected Gear for the completeActor grant. */
} PcFieldFirstMesh;

/* With the registered opening actor and cleared depth table held by its caller,
 * execute the file-12 pose, shadow and ordered, owner-checked mesh prefix. */
void PcPort_FieldDrawFirstGearMesh(MATRIX *view, MATRIX *light, u32 *depth, s32 context,
                                  PcFieldFirstMesh *first);

/* DCEC8 after the final mesh, followed by E7D14's auxiliary draw.
 * The caller holds the registered actor, authenticated owners, view and OT. */
int PcPort_FieldFinishGearActor(u8 *actor, MATRIX *view, u32 *depth, s32 context,
                               s32 ticks, const PcFieldGearActorTail *tail);

#endif
