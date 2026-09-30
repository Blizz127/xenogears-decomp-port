#include "common.h"
#include "system/memory.h"
#include "system/math.h"
#include "field/main.h"
#include "field/actor.h"
#include "field/camera.h"
#include "field/particles.h"

extern s32 D_800ADB34;
extern s32 g_FieldSystemMode;
extern u8 D_8006FDC8[]; // "PARTICLE  "
extern s32 D_80050100;
extern u8 D_800AF474[8];
extern s32 D_800B00B4;

static inline SetVector(SVECTOR* pVec, short value) {
    pVec->vx = value;
    pVec->vy = value;
    pVec->vz = value;
}

/* The heap lives in low-address RAM in the native port. Keep its address in
 * the retail four-byte bank slot so typed and raw script accesses agree. */
static inline ParticlePrimitive* ParticleBankPrimitives(ParticleBank* pBank) {
    return (ParticlePrimitive*)(uintptr_t)pBank->pPrimitives;
}

void FieldInitializeParticles(void) {
    int i;

    for (i = 0; i < NUM_PARTICLES; i++) {
        g_FieldParticleStatuses[i] = 0;
        g_FieldParticleActorIDs[i] = -1;
    }
}

void FieldParticlesFree(int index) {
    int i;
    ParticleBank* pCurBank;

    if (g_FieldParticleStatuses[index] == 1) {
        for (i = 0, pCurBank = g_FieldParticleBanks[index]; i < NUM_PARTICLE_BANKS; i++, pCurBank++) {
            if (pCurBank->max != 0) {
                HeapFree(ParticleBankPrimitives(pCurBank));
            }            
        }
        HeapFree(g_FieldParticleBanks[index]);
    }
    
    g_FieldParticleStatuses[index] = 0;
    g_FieldParticleActorIDs[index] = -1;
}

void FieldParticlesStop(int index) {
    int i;
    ParticleBank* pCurBank;

    if (g_FieldParticleStatuses[index] != 1) {
        return;
    }

    for (i = 0, pCurBank = g_FieldParticleBanks[index]; i < NUM_PARTICLE_BANKS; i++, pCurBank++) {
        if (pCurBank->max != 0) {
            pCurBank->ewait = 0;
        }
    }
}

void FieldParticlesStopBanks(int index) {
    int i, j;
    ParticleBank* pCurBank;

    if (g_FieldParticleStatuses[index] != 1) {
        return;
    }

    pCurBank = g_FieldParticleBanks[index];
    for (i = 0; i < NUM_PARTICLE_BANKS; i++, pCurBank++) {
        if (pCurBank->max == 0) 
            continue;
        
        pCurBank->ewait = 0;
        for (j = 0; j < pCurBank->max; j++) {
            ParticlePrimitive* pPrim = &ParticleBankPrimitives(pCurBank)[j];
            pPrim->ewait = 1;
        }
    }
}

void FieldParticlesFreeAll(void) {
    int i;
    
    for (i = 0; i < NUM_PARTICLES; i++) {
        FieldParticlesFree(i);
    }
    FieldRenderSync();
}

void FieldInitializeDefaultParticleBanks(short targetActorID) {
    int i;
    int j;
    SVECTOR* pVec;

    g_FieldParticleBankIndex = 0;

    for (i = 0; i < NUM_PARTICLE_BANKS; i++) {
        g_FieldDefaultParticleBanks[i].targetActorID = targetActorID;
        g_FieldDefaultParticleBanks[i].unk0 = 0x0;
        g_FieldDefaultParticleBanks[i].swait = 0x0;
        g_FieldDefaultParticleBanks[i].ewait = 0x80;
        g_FieldDefaultParticleBanks[i].max = 0x0;
        SetVector(&g_FieldDefaultParticleBanks[i].pos, 0x0);
        pVec = &g_FieldDefaultParticleBanks[i].epos;
        pVec->vy = -1000;
        pVec->vx = 0;
        pVec->vz = 0;
        g_FieldDefaultParticleBanks[i].speed = 0x8000;
        g_FieldDefaultParticleBanks[i].unk50 = 0x800;
        g_FieldDefaultParticleBanks[i].speedMultiplier = 0x1;
        SetVector(&g_FieldDefaultParticleBanks[i].gravity, 0x0);
        g_FieldDefaultParticleBanks[i].erange = 0x100;
        g_FieldDefaultParticleBanks[i].pewait = 0x1C;
        g_FieldDefaultParticleBanks[i].srange = 0x0;   
        g_FieldDefaultParticleBanks[i].flags = 0x0;
        g_FieldDefaultParticleBanks[i].rotAngle = 0x0;
        g_FieldDefaultParticleBanks[i].pswait = 0x1;
        g_FieldDefaultParticleBanks[i].shape = 0x0;
        SetVector(&g_FieldDefaultParticleBanks[i].scale, 456);
        SetVector(&g_FieldDefaultParticleBanks[i].scaleDelta, 0x20);
        g_FieldDefaultParticleBanks[i].color.r = 128;
        g_FieldDefaultParticleBanks[i].color.g = 32;
        g_FieldDefaultParticleBanks[i].color.b = 0;
        g_FieldDefaultParticleBanks[i].colorDelta.r = -4;
        g_FieldDefaultParticleBanks[i].colorDelta.g = -1;
        g_FieldDefaultParticleBanks[i].colorDelta.b = 0;

        for (j = 0; j < 8; j++) {
            g_FieldDefaultParticleBanks[i].directions[j].x = 0x0;
            g_FieldDefaultParticleBanks[i].directions[j].z = 0x0;
        } 
    }
}

void FieldParticlesTickAndRender(void) {
    MATRIX matWorldToScreen;
    int timer;
    int maxElements;
    int bInUse;
    int primIndex;
    ParticleBank* pCurBank;
    int i, j;

    if (D_800ADB34 == 0) {
        matWorldToScreen = g_Scene.worldToScreenMatrix;

        for (i = 0; i < NUM_PARTICLES; i++) {
            bInUse = 0;
            if (g_FieldParticleStatuses[i] == 1) {
                for (j = 0, pCurBank = g_FieldParticleBanks[i]; j < NUM_PARTICLE_BANKS; pCurBank++, j++) {
                    timer = 0;
                    if (pCurBank->max == 0)
                        continue;
                    
                    if (pCurBank->swait == 0) {
                        for (primIndex = 0; primIndex < pCurBank->max; primIndex++) {
                            if (ParticleBankPrimitives(pCurBank)[primIndex].active == 0) {
                                // If the particle duration on our bank is non-zero, start the particle
                                if (pCurBank->ewait != 0) {
                                    FieldParticleStart(pCurBank, &ParticleBankPrimitives(pCurBank)[primIndex], &timer);
                                    FieldParticleUpdateAndRender(pCurBank, &ParticleBankPrimitives(pCurBank)[primIndex], &matWorldToScreen);
                                    bInUse = 1;
                                }
                            } else {
                                FieldParticleUpdateAndRender(pCurBank, &ParticleBankPrimitives(pCurBank)[primIndex], &matWorldToScreen);
                                bInUse = 1;
                            }
                        }
                        
                        // Tick down the particle duration of the bank
                        if (pCurBank->ewait) {
                            if (pCurBank->ewait != 0x7FFF) {
                                 pCurBank->ewait--;
                            }
                            bInUse = 1;
                        }
                    } else {
                        bInUse = 1;
                        pCurBank->swait--;
                    }
                }
                
                if (bInUse == 0) {
                    FieldParticlesFree(i);
                }
            }
        }
        
        if (g_FieldSystemMode == 0) {
            func_80281B00(&D_8006FDC8);
        }
    }
}

int FieldParticlesRandRange(int range) {
    return ((rand() * range) + 1) >> 15;
}

int FieldParticlesFindFreeIndex(void) {
    int i;

    for (i = 0; i < NUM_PARTICLES; i++) {
        if (g_FieldParticleStatuses[i] == 0) {
            return i;
        }
    }
    return -1;
}

void FieldParticleActorStop(int actorIndex, int mode) {
    int i;
    
    for (i = 0; i < NUM_PARTICLES; i++) {
        if (g_FieldParticleActorIDs[i] == actorIndex) {
            if (mode == 0) {
                g_FieldParticleBanks[i]->ewait = 0;
                g_FieldParticleBanks[i]->swait = 0;
                FieldParticlesStop(i);
            } else {
                g_FieldParticleBanks[i]->ewait = 0;
                g_FieldParticleBanks[i]->swait = 0;
                FieldParticlesStopBanks(i);
            }
        }      
    }
}

int FieldInitializeParticleBanks(int actorIndex) {
    int i, j;
    ParticleBank* pCurrent;
    ParticleBank* pBanks;

    i = FieldParticlesFindFreeIndex();
    if (i == -1) {
        return -1;
    }
    
    HeapChangeCurrentUser(HEAP_USER_YOSI, 0);
    g_FieldParticleCurActor = actorIndex;
    g_FieldParticleStatuses[i] = 1;
    g_FieldParticleActorIDs[i] = actorIndex;
    pBanks = HeapAlloc(sizeof(ParticleBank) * NUM_PARTICLE_BANKS, 0);
    g_FieldParticleBanks[i] = pBanks;

    // Cast to a ParticleBanks struct wrapping an array of ParticleBank objects
    // in order to match the original code.
    *(ParticleBanks*)pBanks = *(ParticleBanks*)g_FieldDefaultParticleBanks;
    
    for (i = 0, pCurrent = &pBanks[i]; i < NUM_PARTICLE_BANKS; i++, pCurrent++) {
        if (pCurrent->max == 0)
            continue;
        
        pCurrent->pPrimitives = (u32)(uintptr_t)HeapAlloc(pCurrent->max * sizeof(ParticlePrimitive), 0);
        for (j = 0; j < pCurrent->max; j++) {
            ParticleBankPrimitives(pCurrent)[j].active = 0;
            FieldInitializeParticlePrimitive(
                &ParticleBankPrimitives(pCurrent)[j],
                pCurrent->shape, 
                ((((s16)pCurrent->flags >> 8) + 1) & 3)
            );
        }
    }
    
    return 1;
}

int FieldParticleUpdateColor(int color, int delta) {
    int newColor;

    if (delta < 0) {
        newColor = color + delta;
        if (newColor < 0) {
            newColor = 0;
        }
    } else {
        newColor = color + delta;
        if (newColor >= 256) {
            newColor = 255;
        }
    }
    
    return newColor;
}

void FieldParticleRender(ParticlePrimitive* pParticle, MATRIX* pMatWorldToScreen, s16 angleZ, s32 sortFlag, VECTOR* pScale, s32 typeFlag) {
    MATRIX transformMatrix;
    MATRIX matLocalToWorld;
    MATRIX matWorldToScreen;
    SVECTOR rotationAngle;
    VECTOR scale;
    u_long value;
    long zDepth;
    POLY_FT4* pPoly;

    // Set up our transform matrices
    rotationAngle.vx = 0;
    rotationAngle.vy = 0;
    rotationAngle.vz = angleZ;
    RotMatrix(&rotationAngle, &matLocalToWorld);
    matLocalToWorld.t[0] = pParticle->position.vx >> 12;
    matLocalToWorld.t[1] = pParticle->position.vy >> 12;
    matLocalToWorld.t[2] = pParticle->position.vz >> 12;
    
    if (typeFlag == 3) {
        matWorldToScreen = *pMatWorldToScreen;
        ScaleMatrix(&matWorldToScreen, pScale);
        CompMatrix(&matWorldToScreen, &matLocalToWorld, &transformMatrix);
        FieldMatrixCopyTransform(&transformMatrix, &matLocalToWorld);
        scale.vx = pParticle->scale.vx;
        scale.vy = pParticle->scale.vy;
        scale.vz = pParticle->scale.vz;
        ScaleMatrix(&transformMatrix, &scale);
        setRGB0(&pParticle->poly[g_FieldCurRenderContextIndex], pParticle->color.r, pParticle->color.g, pParticle->color.b);
        SetTransMatrix(&transformMatrix);
        ScaleMatrix(&transformMatrix, pScale);
    } else {
        CompMatrix(pMatWorldToScreen, &matLocalToWorld, &transformMatrix);
        FieldMatrixCopyTransform(&transformMatrix, &matLocalToWorld);
        scale.vx = pParticle->scale.vx;
        scale.vy = pParticle->scale.vy;
        scale.vz = pParticle->scale.vz;
        ScaleMatrix(&transformMatrix, &scale);
        setRGB0(&pParticle->poly[g_FieldCurRenderContextIndex], pParticle->color.r, pParticle->color.g, pParticle->color.b);
        SetTransMatrix(&transformMatrix);
    }
    SetRotMatrix(&transformMatrix);

    // Transform vertices and set the coordinates of our POLY_FT4 primitive
    pPoly = &pParticle->poly[g_FieldCurRenderContextIndex];
    zDepth = RotAverage4(
        &pParticle->vertices[0], &pParticle->vertices[1], &pParticle->vertices[2], &pParticle->vertices[3], 
        (long*)&pPoly->x0, (long*)&pPoly->x1, (long*)&pPoly->x2, (long*)&pPoly->x3, 
        &value, &value
    ) >> D_80050100;

    switch (sortFlag) {
        case PARTICLE_SORT_TOP:
            value = 1;
            break;
        case PARTICLE_SORT_MID:
            value = zDepth - 16;
            break;
        case PARTICLE_SORT_NORMAL:
            value = zDepth;
            break;
        case PARTICLE_SORT_BACK:
            value = zDepth + 16;
            break;        
    }

    // Render the POLY_FT4 primitive as long as out OT Z-index is valid
    if ((value - 1) < (FIELD_OT_MAX_SIZE - 1)) {
        /* Retail indexes packed words at context +0xCC. Keeping the table
         * base separate lets addPrim use the retail constant displacement. */
        u32* ot = (u32*)g_FieldCurRenderContext;
        addPrim(ot + value + 0x33, &pParticle->poly[g_FieldCurRenderContextIndex]);
    }
}

void FieldParticleUpdateAndRender(ParticleBank* pParticleBank, ParticlePrimitive* pPrim, MATRIX* pMatWorldToScreen) {
    VECTOR sp18;
    SVECTOR rotationVec;
    MATRIX matrix;
    MATRIX transformMatrix;
    VECTOR sp70;
    VECTOR sp80;
    VECTOR sp90;
    VECTOR scaleVec;
    long flag;
    FieldActor* pFieldActors;
    int var_s5;
    int delta;

    if (pPrim->swait != 0) {
        // Tick down the particle's start delay
        pPrim->swait--;
        if (pPrim->swait != 0) {
            return;
        }
        
        // Once the start delay is zero, initialize stuff
        matrix.t[2] = 0;
        matrix.t[1] = 0;
        matrix.t[0] = 0;
        
        var_s5 = 0;
        switch (PARTICLE_FLAG_TYPE(pParticleBank->flags)) {
            case 3:
                rotationVec.vx = 0;
                rotationVec.vy = ((ActorData*)(uintptr_t)g_FieldActors[pParticleBank->targetActorID].pActorData)->rotation.vz;
                rotationVec.vz = 0;
                RotMatrix(&rotationVec, &matrix);
                pFieldActors = g_FieldActors;
                sp70.vx = CONV_TO_GTE(((ActorData*)(uintptr_t)pFieldActors[pParticleBank->targetActorID].pActorData)->position.vx);
                sp70.vy = CONV_TO_GTE(((ActorData*)(uintptr_t)pFieldActors[pParticleBank->targetActorID].pActorData)->position.vy);
                sp70.vz = CONV_TO_GTE(((ActorData*)(uintptr_t)pFieldActors[pParticleBank->targetActorID].pActorData)->position.vz);
                pParticleBank->unk50 = ((ActorData*)(uintptr_t)pFieldActors[pParticleBank->targetActorID].pActorData)->scaleX;
                var_s5 = 1;
                break;
            case 0:
                rotationVec.vx = 0;
                rotationVec.vy = ((ActorData*)(uintptr_t)g_FieldActors[pParticleBank->targetActorID].pActorData)->rotation.vz;
                rotationVec.vz = 0;
                RotMatrix(&rotationVec, &matrix);
                pFieldActors = g_FieldActors;
                sp70.vx = CONV_TO_GTE(((ActorData*)(uintptr_t)pFieldActors[pParticleBank->targetActorID].pActorData)->position.vx);
                sp70.vy = CONV_TO_GTE(((ActorData*)(uintptr_t)pFieldActors[pParticleBank->targetActorID].pActorData)->position.vy);
                sp70.vz = CONV_TO_GTE(((ActorData*)(uintptr_t)pFieldActors[pParticleBank->targetActorID].pActorData)->position.vz);
                pParticleBank->unk50 = 0x1000;
                break;
            case 1:
                func_801E72CC(&matrix, &transformMatrix, pParticleBank->unk72, pParticleBank->unk74);
                SetRotMatrix(&matrix);
                SetTransMatrix(&matrix);
                rotationVec.vx = pParticleBank->pos.vx;
                rotationVec.vy = pParticleBank->pos.vy;
                rotationVec.vz = pParticleBank->pos.vz;
                RotTrans(&rotationVec, &sp70, &flag);
                pParticleBank->unk50 = 0x1000;
                break;
            case 2:
                matrix = g_FieldActors[pParticleBank->targetActorID].childMatrix;
                SetRotMatrix(&matrix);
                SetTransMatrix(&matrix);
                rotationVec.vx = pParticleBank->pos.vx;
                rotationVec.vy = pParticleBank->pos.vy;
                rotationVec.vz = pParticleBank->pos.vz;
                RotTrans(&rotationVec, &sp70, &flag);
                pParticleBank->unk50 = 0x1000;
                break;
        }
        
        matrix.t[2] = 0;
        matrix.t[1] = 0;
        matrix.t[0] = 0;
        SetRotMatrix(&matrix);
        SetTransMatrix(&matrix);
        rotationVec.vx = pPrim->velocity.vx;
        rotationVec.vy = pPrim->velocity.vy;
        rotationVec.vz = pPrim->velocity.vz;
        ApplyRotMatrix(&rotationVec, &sp18);
        VectorNormal(&sp18, &pPrim->velocity);
        pPrim->velocity.vx = ((pPrim->velocity.vx * pParticleBank->speed) >> 0xC) * pParticleBank->speedMultiplier;
        pPrim->velocity.vy = ((pPrim->velocity.vy * pParticleBank->speed) >> 0xC) * pParticleBank->speedMultiplier;
        pPrim->velocity.vz = ((pPrim->velocity.vz * pParticleBank->speed) >> 0xC) * pParticleBank->speedMultiplier;
        
        if (var_s5 == 1) {
            pPrim->position.vx = (pPrim->position.vx * pParticleBank->unk50) >> 0xC;
            pPrim->position.vy = (pPrim->position.vy * pParticleBank->unk50) >> 0xC;
            pPrim->position.vz = (pPrim->position.vz * pParticleBank->unk50) >> 0xC;
        }
        
        SetRotMatrix(&matrix);
        SetTransMatrix(&matrix);
        rotationVec.vx = pPrim->position.vx;
        rotationVec.vy = pPrim->position.vy;
        rotationVec.vz = pPrim->position.vz;
        RotTrans(&rotationVec, &sp18, &flag);
        
        if (var_s5 == 1) {
            delta = PSX_DEGREES(90);
            rotationVec.vx = D_800B00B4 - delta;
            rotationVec.vy = -g_CamInterpolation.curAngleY;
            rotationVec.vz = 0;
            RotMatrixZYX(&rotationVec, &transformMatrix);
            SetRotMatrix(&transformMatrix);
            SetTransMatrix(&transformMatrix);
            sp80.vx = 0;
            sp80.vy = sp18.vy;
            sp80.vz = 0;
            ApplyRotMatrixLV(&sp80, &sp90);
            sp18.vx += sp90.vx;
            sp18.vz += sp90.vz;
            sp18.vy = sp90.vy;
            pPrim->position.vx = (sp70.vx + sp18.vx) * (0x1000000 / pParticleBank->unk50);
            pPrim->position.vy = (sp70.vy + sp18.vy) * (0x1000000 / pParticleBank->unk50);
            pPrim->position.vz = (sp70.vz + sp18.vz) * (0x1000000 / pParticleBank->unk50);
            return;
        }
        
        pPrim->position.vx = ((sp70.vx + sp18.vx) << 12);
        pPrim->position.vy = ((sp70.vy + sp18.vy) << 12);
        pPrim->position.vz = ((sp70.vz + sp18.vz) << 12);    
        return;
    } 
    
    pPrim->velocity.vx += pPrim->gravity.vx;
    pPrim->velocity.vy += pPrim->gravity.vy;
    pPrim->velocity.vz += pPrim->gravity.vz;
    pPrim->position.vx += pPrim->velocity.vx;
    pPrim->position.vy += pPrim->velocity.vy;
    pPrim->position.vz += pPrim->velocity.vz;
    pPrim->scale.vx += pPrim->scaleDelta.vx;
    pPrim->scale.vy += pPrim->scaleDelta.vy;
    pPrim->scale.vz += pPrim->scaleDelta.vz;
    pPrim->color.r = FieldParticleUpdateColor(pPrim->color.r, pPrim->colorDelta.r);
    pPrim->color.g = FieldParticleUpdateColor(pPrim->color.g, pPrim->colorDelta.g);
    pPrim->color.b = FieldParticleUpdateColor(pPrim->color.b, pPrim->colorDelta.b);
    
    scaleVec.vx = pParticleBank->unk50;
    scaleVec.vy = pParticleBank->unk50;
    scaleVec.vz = pParticleBank->unk50;

    if (pPrim->ewait != 1) {
        FieldParticleRender(pPrim, pMatWorldToScreen, pPrim->zRotation, 
            PARTICLE_FLAG_SORT(pParticleBank->flags), 
            &scaleVec, 
            PARTICLE_FLAG_TYPE(pParticleBank->flags)
        );
    }
    
    pPrim->ewait--;
    if (pPrim->ewait == 0) {
        pPrim->active = 0;
    }
}

void FieldParticleStart(ParticleBank* pParticleBank, ParticlePrimitive* pPrim, int* pTimer) {
    VECTOR _unused;
    VECTOR initialPosition;
    VECTOR initialEPos;
    int angle;
    int factor;
    u16 rotAngle;
    int direction;
    u_int rotation;

    pPrim->active = 1;
    pPrim->swait = pParticleBank->pswait + *pTimer;
    *pTimer += pParticleBank->pswait;
    pPrim->ewait = pParticleBank->pewait;
    
    if (PARTICLE_FLAG_RANDROT(pParticleBank->flags)) {
        rotAngle = PSX_ANGLE(rand());
    } else {
        rotAngle = pParticleBank->rotAngle;
    }
    pPrim->zRotation = rotAngle;
    
    if (!(pParticleBank->flags & 0x80)) {
        factor = FieldParticlesRandRange(pParticleBank->srange);
    } else {
        factor = pParticleBank->srange;
    }
    
    angle = FieldParticlesRandRange(0xFFF);
    initialPosition.vx = (rsin(angle) * factor) >> 12;
    if (!(pParticleBank->flags & 0x40)) {
        initialPosition.vz = (rcos(angle) * factor) >> 12;
    } else {
        initialPosition.vz = 0;
    }

    rotation = g_CamInterpolation.curAngleY + PSX_ANGLE(((ActorData*)(uintptr_t)g_FieldActors[pParticleBank->targetActorID].pActorData)->rotation.vz);
    direction = D_800AF474[rotation >> 9];
    
    initialPosition.vx += pParticleBank->pos.vx + pParticleBank->directions[direction].x;
    initialPosition.vz += pParticleBank->pos.vz + pParticleBank->directions[direction].z;
    initialPosition.vy = pParticleBank->pos.vy;
    pPrim->position.vx = initialPosition.vx;
    pPrim->position.vz = initialPosition.vz;
    pPrim->position.vy = initialPosition.vy;
    
    factor = FieldParticlesRandRange(pParticleBank->erange);
    initialEPos.vx = pParticleBank->epos.vx + ((rsin(angle) * factor) >> 12);
    initialEPos.vz = pParticleBank->epos.vz + ((rcos(angle) * factor) >> 12);
    initialEPos.vy = pParticleBank->epos.vy;
    
    pPrim->velocity.vx = initialEPos.vx - initialPosition.vx;
    pPrim->velocity.vy = initialEPos.vy - initialPosition.vy;
    pPrim->velocity.vz = initialEPos.vz - initialPosition.vz;
    
    pPrim->gravity.vx = pParticleBank->gravity.vx;
    pPrim->gravity.vy = pParticleBank->gravity.vy;
    pPrim->gravity.vz = pParticleBank->gravity.vz;

    pPrim->scale.vx = pParticleBank->scale.vx;
    pPrim->scale.vy = pParticleBank->scale.vy;
    pPrim->scale.vz = pParticleBank->scale.vz;
    pPrim->scaleDelta.vx = pParticleBank->scaleDelta.vx;
    pPrim->scaleDelta.vy = pParticleBank->scaleDelta.vy;
    pPrim->scaleDelta.vz = pParticleBank->scaleDelta.vz;

    pPrim->color.r = pParticleBank->color.r;
    pPrim->color.g = pParticleBank->color.g;
    pPrim->color.b = pParticleBank->color.b;
    pPrim->colorDelta.r = pParticleBank->colorDelta.r;
    pPrim->colorDelta.g = pParticleBank->colorDelta.g;
    pPrim->colorDelta.b = pParticleBank->colorDelta.b;
}
