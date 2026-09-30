#include "common.h"
#include "field/main.h"
#include "field/actor.h"
#include "field/camera.h"
#include "field/text_box.h"
#include "system/math.h"
#include "psyq/libgpu.h"
#include "psyq/libgte.h"
#ifdef XENO_PC_PORT
#include <assert.h>
#include <stdlib.h>
#include "test_input.h"

#ifdef XENO_PC_PORT
/* PsyX GTE register/intrinsic access for the func_80075B44 sprite-fog branch
 * (gte_ldrgb/gte_dpcs/C2_RGB2). Port build only. */
#include <psx/inline_c.h>
#include <psx/gtereg.h>
#endif
#else
/* <assert.h> is unavailable under the matching build's -nostdinc MIPS
 * preprocessor. The assert(...) calls below mark unimplemented/invariant
 * checks in functions not yet byte-matched, so a no-op assert compiles
 * safely there. */
#define assert(x) ((void)0)
/* Matching build uses the PSY-Q inline GTE macros where retail inlines GTE
 * operations (the port uses PsyCross' <psx/inline_c.h> above). */
#include "psyq/inline_c.h"
#include "psyq/gtemac.h"
#endif

#ifdef XENO_PC_PORT
static int s_walkFieldDumpFrame;
static int s_walkFieldDumpEnabled = -1;

static int XenoWalkFieldDumpEnabled(void) {
    if (s_walkFieldDumpEnabled < 0) {
        const char* env = getenv("XENO_WALK_ANIM_DUMP");
        s_walkFieldDumpEnabled =
            (env != NULL && env[0] != '\0' && env[0] != '0');
    }
    return s_walkFieldDumpEnabled;
}

static int XenoWalkFieldDumpFrame(void) {
    return XenoWalkFieldDumpEnabled() ? s_walkFieldDumpFrame : 60;
}

static void XenoWalkFieldDumpAdvance(void) {
    if (XenoWalkFieldDumpEnabled() && s_walkFieldDumpFrame < 60) {
        s_walkFieldDumpFrame++;
    }
}

static int XenoFieldDiagEnabled(void) {
    static int s_enabled = -1;

    if (s_enabled < 0) {
        const char* env = getenv("XENO_FIELD_DIAG");
        s_enabled = (env != NULL && env[0] != '\0' && env[0] != '0');
    }
    return s_enabled;
}

/* Mirror of misc3.c's model-build gate (retail-shaped default ON;
 * XENO_FIELD_NO_MODEL_BUILD=1 opts out). The draw path below must agree with
 * FieldLoad: when the build was skipped, the model double-buffer slots are
 * empty and func_8002C700's prim procs would write at NULL. */
static int PcPortModelBuildEnabled(void) {
    static int s_enabled = -1;

    if (s_enabled < 0) {
        const char* env = getenv("XENO_FIELD_NO_MODEL_BUILD");
        s_enabled = !(env != NULL && env[0] != '\0' && env[0] != '0');
    }
    return s_enabled;
}
#endif

void FieldMatrixResetTranslation(MATRIX *matrix) {
    matrix->t[2] = 0;
    matrix->t[1] = 0;
    matrix->t[0] = 0;
}

void FieldComputeSceneMatrices(void) {
    VECTOR scale;
    MATRIX _unused;
    MATRIX sp40;
    long flag;

    RotMatrix(&g_Scene.camRotation, &g_Scene.camRotationMatrix);
    FieldMatrixResetTranslation(&g_Scene.camRotationMatrix);
    CompMatrix(&g_Scene.camRotationMatrix, &g_Scene.viewMatrix, &sp40);
    FieldMatrixCopy(&g_Scene.viewMatrix, &sp40);
    RotMatrix(&g_Scene.worldRotation, &g_Scene.worldRotationMatrix);
    FieldMatrixResetTranslation(&g_Scene.worldRotationMatrix);
    RotMatrix(&g_Scene.worldRotation, &g_Scene.worldToScreenMatrix);
    MulMatrix2(&g_Scene.viewMatrix, &g_Scene.worldToScreenMatrix);
    SetRotMatrix(&g_Scene.viewMatrix);
    SetTransMatrix(&g_Scene.viewMatrix);
    RotTrans(&g_Scene.worldTranslation, &g_Scene.worldToScreenMatrix.t, &flag);
    scale.vx = g_WorldScale;
    scale.vy = g_WorldScale;
    scale.vz = g_WorldScale;
    ScaleMatrix(&g_Scene.worldToScreenMatrix, &scale);
    SetRotMatrix(&g_Scene.worldToScreenMatrix);
    SetTransMatrix(&g_Scene.worldToScreenMatrix);
}

/* Rebuild an actor's transform matrix from its rotation angles, then apply the
 * actor's 3D scale (ActorData scaleX/Y/Z; the asm loads them with lh, hence the
 * signed casts). Called by the EX-0x03 SET_CURRENT_ACTOR_SCALE opcode handler
 * func_8008D0F4 after it stores the new scale.
 * asm/field/nonmatchings/main/misc2/func_80072254.s */
#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/field/nonmatchings/main/misc2", func_80072254);
#else
void func_80072254(s32 actorIndex) {
    VECTOR scale;
    FieldActor* pActor;

    pActor = &g_FieldActors[actorIndex];
    scale.vx = (s16)((ActorData*)(uintptr_t)pActor->pActorData)->scaleX;
    scale.vy = (s16)((ActorData*)(uintptr_t)pActor->pActorData)->scaleY;
    scale.vz = (s16)((ActorData*)(uintptr_t)pActor->pActorData)->scaleZ;

    pActor = &g_FieldActors[actorIndex];
    RotMatrix((SVECTOR*)&pActor->rotation, &pActor->transformMatrix);

    pActor = &g_FieldActors[actorIndex];
    ScaleMatrix(&pActor->transformMatrix, &scale);
}
#endif /* XENO_PC_PORT */

void FieldMatrixCreateWorldToScreen(void) {
    FieldComputeSceneMatrices();
    SetRotMatrix(&g_Scene.worldToScreenMatrix);
    SetTransMatrix(&g_Scene.worldToScreenMatrix);

    if (g_FieldSystemMode == SYSTEM_MODE_PC_HDD) {
        func_802815B0();
    }
}

extern u8 D_800ADC1C[];

/* Count how many consecutive camera sectors, scanning forward from `index`,
 * are blocked by `mask`. Stops at the first unblocked sector (returning the
 * run length so far) or returns 0 if all 8 sectors are blocked.
 * func_80072398 is the identical scan in the opposite direction; retail emits
 * the two as byte-identical code apart from the index step (+1 vs -1).
 * Retail: asm/field/main/misc2.s 8007234C-80072394. */
s32 func_8007234C(u32 mask, u32 index) {
    s32 i = 0;
    s32 found = 0;
    while (i < 8) {
        u8 bit = D_800ADC1C[index & 7];
        index++;
        if (!(mask & bit)) {
            return found;
        }
        found++;
        i++;
    }
    return 0;
}

extern u8 D_800ADC1C[];

/* Backward twin of func_8007234C. Retail: asm/field/main/misc2.s
 * 80072398-800723E0. */
s32 func_80072398(u32 mask, u32 index) {
    s32 i = 0;
    s32 found = 0;
    while (i < 8) {
        u8 bit = D_800ADC1C[index & 7];
        index--;
        if (!(mask & bit)) {
            return found;
        }
        found++;
        i++;
    }
    return 0;
}

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/field/nonmatchings/main/misc2", func_800723E4);
#else
void func_800723E4(void* arg0, void* arg1, void* arg2) {
    VECTOR delta;
    VECTOR normal0;
    VECTOR normal1;
    s32 denom;
    s32 scale;

    delta.vx = *(s16*)((u8*)arg0 + 0x04) - *(s16*)((u8*)arg0 + 0x00);
    delta.vy = 0;
    delta.vz = *(s16*)((u8*)arg0 + 0x06) - *(s16*)((u8*)arg0 + 0x02);
    VectorNormal(&delta, &normal0);

    delta.vx = *(s16*)((u8*)arg1 + 0x04) - *(s16*)((u8*)arg1 + 0x00);
    delta.vy = 0;
    delta.vz = *(s16*)((u8*)arg1 + 0x06) - *(s16*)((u8*)arg1 + 0x02);
    VectorNormal(&delta, &normal1);

    denom = ((normal1.vx * normal0.vz) - (normal1.vz * normal0.vx)) >> 12;
    if (denom == 0) {
        scale = 0;
    } else {
        s32 num = ((*(s16*)((u8*)arg1 + 0x02) - *(s16*)((u8*)arg0 + 0x02)) *
                   normal0.vx) -
                  ((*(s16*)((u8*)arg1 + 0x00) - *(s16*)((u8*)arg0 + 0x00)) *
                   normal0.vz);
        scale = num / denom;
    }

    *(s16*)((u8*)arg2 + 0x00) =
        *(u16*)((u8*)arg1 + 0x00) + ((scale * normal1.vx) >> 12);
    *(s16*)((u8*)arg2 + 0x02) =
        *(u16*)((u8*)arg1 + 0x02) + ((scale * normal1.vz) >> 12);
}
#endif /* XENO_PC_PORT */

/* ---- func_8007254C: camera/scene default initialization ---------------------
 * Called from FieldLoad. Zeros camera vectors, sets default scene angles,
 * interpolation step distances, and scale factors.
 *
 * ASM uses $s0 = &g_CamInterpolation as a base for $s0-relative negative
 * offsets to reach adjacent camera globals (g_CameraEye, g_CameraAt, etc.).
 * The port's stub layout does NOT preserve PSX-relative positions, so each
 * global is accessed by its own symbol name instead. */
extern VECTOR g_CameraEye;
extern VECTOR g_CameraAt;
extern VECTOR g_CameraUp;
extern VECTOR g_CameraEye2;
extern VECTOR g_CameraAt2;
extern VECTOR D_800AF8D0; /* second up vector, pairs with eye2/at2 */
extern s32 D_800AF8E0, D_800AF8E4, D_800AF8E8;
extern s16 g_FieldCameraMode;
extern u16 g_CamMovementFlags;
extern s16 g_CamAtMovementDuration, g_CamEyeMovementDuration;
extern void func_80070594(MATRIX*);

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/field/nonmatchings/main/misc2", func_8007254C);
#else
void func_8007254C(void) {
    /* g_CamInterpolation fields */
    g_CamInterpolation.atStepDistance = 8;
    g_CamInterpolation.eyeStepDistance = 8;
    g_CamInterpolation.targetAngleY = 0;
    g_CamInterpolation.curAngleY = 0;

    /* g_Scene defaults */
    *(s32*)((u8*)&g_Scene + 0x5C) = 0x400000;
    *(s32*)((u8*)&g_Scene + 0x60) = 0x08000000;
    *(s16*)((u8*)&g_Scene + 0x56) = 0x800;
    *(s32*)((u8*)&g_Scene + 0xA8) = 0;
    *(s32*)((u8*)&g_Scene + 0xA4) = 0;
    *(s32*)((u8*)&g_Scene + 0xA0) = 0;
    *(s32*)((u8*)&g_Scene + 0xB4) = 0;
    *(s32*)((u8*)&g_Scene + 0xB0) = 0;
    *(s32*)((u8*)&g_Scene + 0xAC) = 0;
    *(s16*)((u8*)&g_Scene + 0x98) = 0;
    *(s16*)((u8*)&g_Scene + 0x9C) = 0;
    *(s32*)((u8*)&g_Scene + 0x48) = 0;
    *(s8*)((u8*)&g_Scene + 0x64) = 0;
    *(s8*)((u8*)&g_Scene + 0x65) = 0;
    *(s16*)((u8*)&g_Scene + 0x66) = 0;
    *(s16*)((u8*)&g_Scene + 0x54) = 0;
    *(s16*)((u8*)&g_Scene + 0x58) = 0;
    *(s32*)((u8*)&g_Scene + 0x7C) = 0x800;
    *(s16*)((u8*)&g_Scene + 0x40) = 0;
    *(s16*)((u8*)&g_Scene + 0x42) = 0;
    *(s16*)((u8*)&g_Scene + 0x44) = 0;

    func_80070594(&g_Scene.camRotationMatrix);

    /* Camera vectors — asm 80072640-800726B0, $s0 = &g_CamInterpolation
     * (0x800AF984). Negative-offset decode: -0x104 eye.vx, -0xF4..-0xEC at,
     * -0xE4/-0xE0/-0xDC up, -0xD4..-0xCC eye2, -0xC4..-0xBC at2,
     * -0xB4/-0xB0/-0xAC D_800AF8D0, -0xA4..-0x9C D_800AF8E0 shake. */
    g_CameraEye.vx = 0;  g_CameraEye.vy = 0;  g_CameraEye.vz = 0;
    g_CameraAt.vx = 0;   g_CameraAt.vy = 0;   g_CameraAt.vz = 0;
    g_CameraUp.vx = 0;
    g_CameraUp.vy = 0x10000000;  /* asm 2B68: sw $v0, -0xE0($s0) = 0x800AF8A4 */
    g_CameraUp.vz = 0;
    g_CameraEye2.vx = 0;
    g_CameraEye2.vy = 0;
    g_CameraEye2.vz = 0;
    *(s32*)((u8*)&g_Scene + 0x68) = 0x200;
    *(s16*)((u8*)&g_Scene + 0x6C) = 0x1E;
    *(s16*)((u8*)&g_Scene + 0x6E) = 0x1000;
    g_CameraAt2.vx = 0;
    g_CameraAt2.vy = 0;
    g_CameraAt2.vz = 0;
    D_800AF8D0.vx = 0;
    D_800AF8D0.vy = 0x10000000;  /* asm 2B6C: sw $v0, -0xB0($s0) = 0x800AF8D4 */
    D_800AF8D0.vz = 0;
    D_800AF8E0 = 0;  /* asm 2B98-2BA0: shake offsets zeroed */
    D_800AF8E4 = 0;
    D_800AF8E8 = 0;

    /* Retail 800726B4..800726D0: reset the remaining scene words and
     * leave scripted camera mode before initializing the next field. */
    *(s16*)((u8*)&g_Scene + 0x70) = 0;
    *(s16*)((u8*)&g_Scene + 0x80) = 0;
    *(s16*)((u8*)&g_Scene + 0x8C) = 0;
    g_FieldCameraMode = 0;
    g_CamMovementFlags = 0;
    g_CamAtMovementDuration = 0;
    g_CamEyeMovementDuration = 0;
}
#endif /* XENO_PC_PORT */

/* ---- func_800726E8: camera heading transition handler -----------------------
 * Called from func_80073230. g_Scene+0x56 is the camera heading (0xFFF wrap,
 * 8 sectors of 0x200); +0x64/+0x65 are per-sector block masks tested against
 * D_800ADC1C, +0x5C/+0x60/+0x66/+0x7C drive an 8-frame heading interpolation.
 * Two held-button blocks (D_800AFE9C bits 4/8) start a manual ±0x200 rotate,
 * each refused while g_Scene+0x48 bit15 is set (script owns the camera) or a
 * transition is in flight, and refused into a masked sector.
 * Retail: asm/field/main/misc2.s 800726E8-80072A38.
 *
 * Matching notes -- every `mask & D_800ADC1C[...]` below is written mask-first
 * on purpose. GCC 2.7.2 emits `and $d, <op1>, <op0>`, so writing the array
 * subscript first flips the operand order against retail; and because the
 * mask operand is evaluated first it is its address, not g_Scene+0x56's, that
 * wins the one callee-saved register ($s0) in block 2. Likewise block 2 spells
 * the g_Scene+0x56 read out at each use rather than hoisting it into a local:
 * CSE folds the repeats within the block, but a local would put the wrong
 * address in $s0. */
extern u8 D_800ADC1C[];
extern u16 D_800AFE9C;
extern s32 func_80072398(u32 mask, u32 index);
extern void func_80284EA4(void);

void func_800726E8(void) {
    u8 scene64 = *(u8*)((u8*)&g_Scene + 0x64);
    u8 scene65;
    u16 scene56;
    s32 modeIdx;

    if (scene64 == 0xFF) goto tail;
    scene65 = *(u8*)((u8*)&g_Scene + 0x65);
    if (scene65 == 0xFF) goto tail;

    /* Both automatic blocks are skipped while a transition is active
     * (asm 80072728: bnez -> .L80072850, the first pad block). */
    if (*(s16*)((u8*)&g_Scene + 0x66) != 0) goto pad_rotate;

    /* Automatic block 1: current sector masked against scene64. */
    scene56 = *(u16*)((u8*)&g_Scene + 0x56) & 0xFFF;
    if (scene64 & D_800ADC1C[scene56 >> 9]) {
        s32 scene5C = *(s32*)((u8*)&g_Scene + 0x5C);
        if (scene5C != (s32)0xFFC00000 && scene5C != 0x400000) {
            *(s32*)((u8*)&g_Scene + 0x5C) = 0x400000;
            *(s32*)((u8*)&g_Scene + 0x7C) += 0x200;
        }
        *(s16*)((u8*)&g_Scene + 0x66) = 8;
    }

    /* Automatic block 2: current sector masked against scene65 rotates
     * toward the nearer side (func_8007234C vs func_80072398). Retail keeps
     * &g_Scene+0x65 in $s0 and reloads the byte through it either side of the
     * func_8007234C call; the heading is re-read from its own %hi/%lo pair. */
    if (*(u8*)((u8*)&g_Scene + 0x65) &
        D_800ADC1C[(*(u16*)((u8*)&g_Scene + 0x56) & 0xFFF) >> 9]) {
        s32 r1 = func_8007234C(*(u8*)((u8*)&g_Scene + 0x65),
                               (*(u16*)((u8*)&g_Scene + 0x56) & 0xFFF) >> 9);
        if (func_80072398(*(u8*)((u8*)&g_Scene + 0x65),
                          (*(u16*)((u8*)&g_Scene + 0x56) & 0xFFF) >> 9) < r1) {
            *(s32*)((u8*)&g_Scene + 0x5C) = 0xFFC00000;
            *(s32*)((u8*)&g_Scene + 0x7C) -= 0x200;
        } else {
            *(s32*)((u8*)&g_Scene + 0x5C) = 0x400000;
            *(s32*)((u8*)&g_Scene + 0x7C) += 0x200;
        }
        *(s16*)((u8*)&g_Scene + 0x66) = 8;
    }

pad_rotate:
    /* Manual rotate, negative direction (asm .L80072850). */
    if ((D_800AFE9C & 4) &&
        !(*(s32*)((u8*)&g_Scene + 0x48) & 0x8000) &&
        *(s16*)((u8*)&g_Scene + 0x66) == 0) {
        modeIdx = ((*(s16*)((u8*)&g_Scene + 0x56) - 0x200) & 0xFFF) >> 9;
        if (!(*(u8*)((u8*)&g_Scene + 0x65) & D_800ADC1C[modeIdx])) {
            *(s32*)((u8*)&g_Scene + 0x5C) = 0xFFC00000;
            *(s16*)((u8*)&g_Scene + 0x66) = 8;
            *(s32*)((u8*)&g_Scene + 0x7C) -= 0x200;
        }
    }

    /* Manual rotate, positive direction (asm .L800728F4). */
    if (D_800AFE9C & 8) {
        if (!(*(s32*)((u8*)&g_Scene + 0x48) & 0x8000)) {
            /* asm 80072930: bnez -> .L800729AC, straight into the
             * countdown body. */
            if (*(s16*)((u8*)&g_Scene + 0x66) != 0) goto countdown;
            modeIdx = ((*(s16*)((u8*)&g_Scene + 0x56) + 0x200) & 0xFFF) >> 9;
            if (!(*(u8*)((u8*)&g_Scene + 0x65) & D_800ADC1C[modeIdx])) {
                *(s32*)((u8*)&g_Scene + 0x5C) = 0x400000;
                *(s16*)((u8*)&g_Scene + 0x66) = 8;
                *(s32*)((u8*)&g_Scene + 0x7C) += 0x200;
            }
        }
    }

tail:
    if (*(s16*)((u8*)&g_Scene + 0x66) == 0) goto set56;
countdown:
    {
        s32 new60 = *(s32*)((u8*)&g_Scene + 0x60) + *(s32*)((u8*)&g_Scene + 0x5C);
        s16 v;
        *(s32*)((u8*)&g_Scene + 0x60) = new60;
        *(s16*)((u8*)&g_Scene + 0x56) = (s16)(new60 >> 16);
        v = (s16)(*(u16*)((u8*)&g_Scene + 0x66) - 1);
        *(s16*)((u8*)&g_Scene + 0x66) = v;
        if (v != 0) goto done;
    }
set56:
    *(u16*)((u8*)&g_Scene + 0x56) = (u16)*(s32*)((u8*)&g_Scene + 0x7C);
done:
    if (g_FieldSystemMode == 0) {
        func_80284EA4();
    }
}

/* ---- func_80072A38: camera position update from input vector ----------------
 * Takes a VECTOR* ($a0/$s0) and a flag ($a1/$s2). Calls func_8007CD80 to
 * look up a camera target; if not found (-1), computes via func_800723E4
 * + rsin/rcos. Sets g_CameraAt2 and g_CameraEye2. Calls func_80073684
 * for eye/at adjustment. Handles g_Scene+0x48 camera transition flags. */
extern s32 D_800ADBA8;
extern u8 D_800B21CD;
extern s32 func_8007CD80(void* a0, void* a1, void* a2);
extern s32 rsin(s32 angle);
extern s32 rcos(s32 angle);
extern void func_80073684(VECTOR* pEye, VECTOR* pAt);

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/field/nonmatchings/main/misc2", func_80072A38);
#else
void func_80072A38(VECTOR* pCamInput, s32 flag) {
    VECTOR vecArg;
    u8 cd80Segment[0x10];
    SVECTOR cd80Clipped;
    s32 result;
    s32 sinVal, cosVal;
    s32 sceneAngle, sceneScrZ, scene6E;

    /* Build a 2-element vector from pCamInput */
    vecArg.vx = pCamInput->vx;
    vecArg.vy = 0;
    vecArg.vz = pCamInput->vz;

    result = func_8007CD80(&vecArg, cd80Segment, &cd80Clipped);

    if (result == -1) {
        /* Camera target not found — compute via func_800723E4 */
        SVECTOR segmentEnds;
        SVECTOR intersection;

        segmentEnds.vx = *(s16*)(cd80Segment + 0x00);
        segmentEnds.vy = *(s16*)(cd80Segment + 0x04);
        segmentEnds.vz = *(s16*)(cd80Segment + 0x08);
        segmentEnds.pad = *(s16*)(cd80Segment + 0x0C);
        func_800723E4(&segmentEnds, &cd80Clipped, &intersection);

        g_CameraAt2.vx = (s32)intersection.vx << 16;
        g_CameraAt2.vz = (s32)intersection.vy << 16;

        if (!D_800B21CD) {
            if (!D_800ADBA8) {
                g_CameraAt2.vy = flag << 16;
                D_800ADBA8 = 1;
            }
        } else {
            g_CameraAt2.vy = pCamInput->vy + 0xFFE00000;
        }
    } else {
        /* Camera target found — copy directly */
        g_CameraAt2.vx = pCamInput->vx;
        D_800ADBA8 = 0;
        g_CameraAt2.vy = pCamInput->vy;
        g_CameraAt2.vy = pCamInput->vy + 0xFFE00000;
        g_CameraAt2.vz = pCamInput->vz;
    }

    /* Compute camera eye offset using sin/cos of scene angle */
    sceneAngle = *(s16*)((u8*)&g_Scene + 0x6C);
    sceneScrZ = *(s32*)((u8*)&g_Scene + 0x68);
    scene6E = *(s16*)((u8*)&g_Scene + 0x6E);

    /* Angle calculation: (91*angle)>>3 + 0xC00, then rsin. The expression below
     * is the compiler's strength-reduced 91*angle ((3a<<3 - a)<<2 - a); verified
     * byte-exact against retail asm func_80072A38.s 80072B80-80072BA0 (this is a
     * matching function). Earlier "angle*23/8" comment was stale (23a is only the
     * inner (3a<<3 - a) subterm before the final <<2 - a). */
    {
        s32 angleCalc = sceneAngle;
        s32 sinArg = ((angleCalc * 2 + angleCalc) * 8 - angleCalc) * 4 - angleCalc;
        sinArg = sinArg >> 3;
        sinArg += 0xC00;
        sinVal = rsin(sinArg);
    }

    {
        s32 sinResult = (s32)(sinVal * sceneScrZ) << 5;
        s32 cosMult;
        s32 angleCalc;
        s32 sinArg;
        sinResult = -sinResult >> 16;
        cosMult = (s32)(sinResult * scene6E);
        angleCalc = sceneAngle;
        sinArg = ((angleCalc * 2 + angleCalc) * 8 - angleCalc) * 4 - angleCalc;
        sinArg = sinArg >> 3;
        cosMult = (s32)cosMult << 4;
        g_CameraEye2.vy = cosMult + g_CameraAt2.vy;
    }

    /* asm 80072C40/80072C48: lw -0x13C($s0)=at2.vx, sw -0x14C($s0)=eye2.vx
     * ($s0 = g_Scene+0x6C = 0x800AF9FC) — unconditional, before func_80073684. */
    g_CameraEye2.vx = g_CameraAt2.vx;

    {
        s32 angleCalc = sceneAngle;
        s32 cosArg = ((angleCalc * 2 + angleCalc) * 8 - angleCalc) * 4 - angleCalc;
        cosArg = cosArg >> 3;
        cosArg += 0xC00;
        cosVal = rcos(cosArg);
    }

    {
        s32 cosResult = (s32)(cosVal * sceneScrZ) << 5;
        s32 cosMult;
        cosResult = cosResult >> 16;
        cosMult = (s32)(cosResult * scene6E);
        cosMult = (s32)cosMult << 4;
        g_CameraEye2.vz = cosMult + g_CameraAt2.vz;
    }

    /* Eye/at adjustment */
    func_80073684(&g_CameraEye2, &g_CameraAt2);

    /* Camera transition flag handling (g_Scene + 0x48) */
    {
        s32 scene48 = *(s32*)((u8*)&g_Scene + 0x48);
        if (scene48 & 0x1) {
            s16 scene8C = *(s16*)((u8*)&g_Scene + 0x8C);
            s16 new8C;
            if (scene8C != 0) {
                s32 scene90 = *(s32*)((u8*)&g_Scene + 0x90);
                s32 scene94 = *(s32*)((u8*)&g_Scene + 0x94);
                s32 newVal = scene90 + scene94;
                *(s32*)((u8*)&g_Scene + 0x90) = newVal;
                *(s16*)((u8*)&g_Scene + 0x6E) = (s16)(newVal >> 16);
            }
            new8C = scene8C - 1;
            *(s16*)((u8*)&g_Scene + 0x8C) = new8C;
            if (new8C == 0) {
                *(s32*)((u8*)&g_Scene + 0x48) = scene48 & 0xFFFE;
            }
        }

        /* Bit 3: scroll transition */
        scene48 = *(s32*)((u8*)&g_Scene + 0x48);
        if (scene48 & 0x8) {
            s32 scene74 = *(s32*)((u8*)&g_Scene + 0x74);
            s32 scene78 = *(s32*)((u8*)&g_Scene + 0x78);
            s32 newVal = scene74 + scene78;
            s16 scene70;
            *(s32*)((u8*)&g_Scene + 0x74) = newVal;
            *(s16*)((u8*)&g_Scene + 0x6C) = (s16)(newVal >> 16);
            scene70 = *(s16*)((u8*)&g_Scene + 0x70);
            scene70--;
            *(s16*)((u8*)&g_Scene + 0x70) = scene70;
            if (scene70 == 0) {
                *(s32*)((u8*)&g_Scene + 0x48) = scene48 & 0xFFF7;
            }
        }
    }
}
#endif /* XENO_PC_PORT */

/* ---- func_80072D74: camera interpolation/smoothing --------------------------
 * Called every frame at end of func_80073230. Interpolates g_CameraEye/At
 * toward g_CameraEye2/At2 using g_CamInterpolation step distances.
 * Also handles screen-Z transitions, shake offsets, and countdowns. */
extern s32 D_800B21D8;
extern s32 D_800AF8E0, D_800AF8E4, D_800AF8E8;
extern int rand(void);

/* Helper: interpolate one 32-bit component toward target.
 * If difference is small enough (diff^2 < threshold), snap. Otherwise step. */
static inline void cam_lerp(s32* pCur, s32 target, s32 step, s32 threshold) {
    s32 cur = *pCur;
    s32 diff;
    s32 diff16;
    if ((cur >> 16) == (target >> 16)) return;
    diff = target - cur;
    diff16 = diff >> 16;
    if (diff16 * diff16 < threshold) return;
    *pCur = cur + diff / step;
}

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/field/nonmatchings/main/misc2", func_80072D74);
#else
void func_80072D74(void) {
    s32 scene48 = *(s32*)((u8*)&g_Scene + 0x48);
    s32 eyeStepSq, atStepSq;

    /* Screen-Z transition (bit 4) */
    if (scene48 & 0x10) {
        s16 z8C = *(s16*)((u8*)&g_Scene + 0x88);
        s16 newZ8C;
        if (z8C != 0) {
            s32 val = *(s32*)((u8*)&g_Scene + 0x84) + *(s32*)((u8*)&g_Scene + 0x88);
            *(s32*)((u8*)&g_Scene + 0x84) = val;
            *(s32*)((u8*)&g_Scene + 0x68) = val >> 16;
        }
        newZ8C = z8C - 1;
        *(s16*)((u8*)&g_Scene + 0x88) = newZ8C;
        if (newZ8C < 0) {
            *(s32*)((u8*)&g_Scene + 0x48) = scene48 & 0xFFEF;
            *(s16*)((u8*)&g_Scene + 0x88) = 0;
        }
    }

    /* D_800B21D8 countdown → reset interpolation steps */
    if (D_800B21D8 != 0) {
        g_CamInterpolation.atStepDistance = 1;
        g_CamInterpolation.eyeStepDistance = 1;
        D_800B21D8--;
    }

    /* Compute step^2 thresholds */
    atStepSq = g_CamInterpolation.atStepDistance * g_CamInterpolation.atStepDistance;
    eyeStepSq = g_CamInterpolation.eyeStepDistance * g_CamInterpolation.eyeStepDistance;

    /* Interpolate eye components (uses eyeStepSq threshold, eyeStepDistance divisor) */
    cam_lerp(&g_CameraEye.vx, g_CameraEye2.vx, g_CamInterpolation.eyeStepDistance, eyeStepSq);
    cam_lerp(&g_CameraEye.vz, g_CameraEye2.vz, g_CamInterpolation.eyeStepDistance, eyeStepSq);
    cam_lerp(&g_CameraEye.vy, g_CameraEye2.vy, g_CamInterpolation.eyeStepDistance, eyeStepSq);

    /* Interpolate at components (uses atStepSq threshold, atStepDistance divisor) */
    cam_lerp(&g_CameraAt.vx,  g_CameraAt2.vx,  g_CamInterpolation.atStepDistance, atStepSq);
    cam_lerp(&g_CameraAt.vz,  g_CameraAt2.vz,  g_CamInterpolation.atStepDistance, atStepSq);
    cam_lerp(&g_CameraAt.vy,  g_CameraAt2.vy,  g_CamInterpolation.atStepDistance, atStepSq);

    /* Retail 80073068..800731FC: reset the shake accumulators, then either
     * integrate the per-frame shake velocities (g_Scene+0xAC/B0/B4) when a
     * shake is active (g_Scene+0x9A != 0), or cancel it (g_Scene+0x9C),
     * then generate the next per-axis random offsets. */
    D_800AF8E0 = 0;
    D_800AF8E4 = 0;
    D_800AF8E8 = 0;

    if (*(s16*)((u8*)&g_Scene + 0x98) != 0) {
        if (*(s16*)((u8*)&g_Scene + 0x9A) != 0) {
            *(s32*)((u8*)&g_Scene + 0xA0) += *(s32*)((u8*)&g_Scene + 0xAC);
            *(s32*)((u8*)&g_Scene + 0xA4) += *(s32*)((u8*)&g_Scene + 0xB0);
            *(s32*)((u8*)&g_Scene + 0xA8) += *(s32*)((u8*)&g_Scene + 0xB4);
        } else if (*(s16*)((u8*)&g_Scene + 0x9C) != 0) {
            *(s32*)((u8*)&g_Scene + 0xA8) = 0;
            *(s32*)((u8*)&g_Scene + 0xA4) = 0;
            *(s32*)((u8*)&g_Scene + 0xA0) = 0;
            *(s16*)((u8*)&g_Scene + 0x98) = 0;
            *(s16*)((u8*)&g_Scene + 0x9C) = 0;
        }

        D_800AF8E0 = rand() * *(s16*)((u8*)&g_Scene + 0xA2);
        D_800AF8E4 = rand() * *(s16*)((u8*)&g_Scene + 0xA6);
        D_800AF8E8 = rand() * *(s16*)((u8*)&g_Scene + 0xAA);

        /* Shake offset clamping (negative → zero + clear flag) */
        if (D_800AF8E0 < 0) { D_800AF8E0 = 0; *(s32*)((u8*)&g_Scene + 0xA0) = 0; }
        if (D_800AF8E4 < 0) { D_800AF8E4 = 0; *(s32*)((u8*)&g_Scene + 0xA4) = 0; }
        if (D_800AF8E8 < 0) { D_800AF8E8 = 0; *(s32*)((u8*)&g_Scene + 0xA8) = 0; }
    }

    /* g_Scene + 0x9A countdown */
    {
        s16 val = *(s16*)((u8*)&g_Scene + 0x9A);
        if (val > 0) {
            *(s16*)((u8*)&g_Scene + 0x9A) = val - 1;
        }
    }
}
#endif /* XENO_PC_PORT */

/* ---- func_80073230: per-frame camera update (FieldComputeSceneMatrices) ------
 * Called every frame. Dispatches on g_FieldCameraMode:
 *   0: direct camera from actor position (calls func_80072A38)
 *   1: camera movement interpolation
 *   2: transition mode
 * Always calls func_80072D74 at end, then masks g_Scene+0x56. */
extern s16 g_FieldCameraMode;
extern s32 D_800ADBAC;
extern s32 D_800ADBB0;
extern FieldCameraMovement g_CamAtMovementFrom;
extern FieldCameraMovement g_CamAtMovementTo;
extern FieldCameraMovement g_CamEyeMovementFrom;
extern FieldCameraMovement g_CamEyeMovementTo;
extern u16 g_CamMovementFlags;
extern VECTOR g_CamAtMovementCurrent;
extern VECTOR g_CamAtMovementDelta;
extern VECTOR g_CamEyeMovementCurrent;
extern VECTOR g_CamEyeMovementDelta;
extern s16 g_CamAtMovementDuration;
extern s16 g_CamEyeMovementDuration;
extern u16 D_800B233E;
extern s16 D_800AFB54;
extern void func_800726E8(void);
extern s16 func_8007B1C4(s16 a0, s16 a1, s32 a2, void* a3, void* a4);
extern void func_80072D74(void);

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/field/nonmatchings/main/misc2", func_80073230);
#else
void func_80073230(void) {
    s16 camMode = g_FieldCameraMode;

    if (camMode == 1) {
        /* Mode 1: camera movement interpolation */
        u16 flags = g_CamMovementFlags;
        D_800ADBAC = 0;
        D_800ADBB0 = 0;

        if (flags & 0x1) {
            /* At movement */
            s16 dur = g_CamAtMovementDuration;
            if (dur != 0) {
                g_CamAtMovementCurrent.vx += g_CamAtMovementDelta.vx;
                g_CamAtMovementCurrent.vy += g_CamAtMovementDelta.vy;
                g_CamAtMovementCurrent.vz += g_CamAtMovementDelta.vz;
            }
            g_CamAtMovementDuration = dur - 1;
            if (g_CamAtMovementDuration == 0) {
                g_CamMovementFlags = flags & 0xFFFE;
            }
            g_CameraAt2.vx = g_CamAtMovementCurrent.vx;
            g_CameraAt2.vy = g_CamAtMovementCurrent.vy;
            g_CameraAt2.vz = g_CamAtMovementCurrent.vz;
        }

        flags = g_CamMovementFlags;
        if (flags & 0x2) {
            /* Eye movement */
            s16 dur = g_CamEyeMovementDuration;
            if (dur != 0) {
                g_CamEyeMovementCurrent.vx += g_CamEyeMovementDelta.vx;
                g_CamEyeMovementCurrent.vy += g_CamEyeMovementDelta.vy;
                g_CamEyeMovementCurrent.vz += g_CamEyeMovementDelta.vz;
            }
            g_CamEyeMovementDuration = dur - 1;
            if (g_CamEyeMovementDuration == 0) {
                g_CamMovementFlags = flags & 0xFFFD;
            }
            g_CameraEye2.vx = g_CamEyeMovementCurrent.vx;
            g_CameraEye2.vy = g_CamEyeMovementCurrent.vy;
            g_CameraEye2.vz = g_CamEyeMovementCurrent.vz;
        }
    } else if (camMode == 0 || camMode == 2) {
        /* Mode 0 or 2: direct camera from actor position */
        VECTOR camInput;
        u8* pActorData;

        if (camMode == 0) {
            if ((D_800ADBAC & 3) == 0) {
                if (g_CamInterpolation.atStepDistance < 9)
                    g_CamInterpolation.atStepDistance = 8;
                else
                    g_CamInterpolation.atStepDistance -= 2;
                if (g_CamInterpolation.eyeStepDistance < 9)
                    g_CamInterpolation.eyeStepDistance = 8;
                else
                    g_CamInterpolation.eyeStepDistance -= 2;
            }
            D_800ADBAC++;
        } else {
            /* Mode 2 */
            D_800ADBAC = 0;
            D_800ADBB0++;
            if (D_800ADBB0 >= 0x41) {
                g_FieldCameraMode = 0;
            }
        }

        func_800726E8();

        /* Get camera position from current actor */
        {
            u16 actorIdx = D_800B233E;
            u8* pActor = (u8*)g_FieldActors + actorIdx * 0x5C;
            u8* pData = (u8*)(uintptr_t)*(u32*)(pActor + 0x4C);
            camInput.vx = *(s32*)(pData + 0x20);
            camInput.vy = *(s32*)(pData + 0x24);
            camInput.vz = *(s32*)(pData + 0x28);
            pActorData = pData;
        }

        func_80072A38(&camInput, *(s16*)(pActorData + 0x72));

        /* Camera-eye terrain clamp (asm 800733C4-8007340C): probe the top
         * walkmesh layer at the eye's (x,z); if the surface Y there is above
         * the eye, snap eye.vy up to it. Retail loads $a0 from
         * g_CameraEye2+0x2 (vx>>16) and passes two DISTINCT out buffers
         * (sp+0x38 surface point, sp+0x28 face normal). */
        if (!(g_Scene.unk48 & 0x4000)) {
            s16 meshPoint[4];
            s32 meshNormal[4];
            func_8007B1C4(
                (s16)(g_CameraEye2.vx >> 16),
                (s16)(g_CameraEye2.vz >> 16),
                D_800AFB54 - 1,
                meshPoint, meshNormal);
            if (meshPoint[1] < (s16)(g_CameraEye2.vy >> 16)) {
                g_CameraEye2.vy = (s32)meshPoint[1] << 16;
            }
        }

        /* Mode 2: distance check to end transition */
        if (g_FieldCameraMode == 2) {
            s32 atDist = FieldGetVec2Magnitude(
                (s16)(g_CameraAt2.vx >> 16) - (s16)(g_CameraAt.vx >> 16),
                (s16)(g_CameraAt2.vz >> 16) - (s16)(g_CameraAt.vz >> 16));
            s32 eyeDist = FieldGetVec2Magnitude(
                (s16)(g_CameraEye2.vx >> 16) - (s16)(g_CameraEye.vx >> 16),
                (s16)(g_CameraEye2.vz >> 16) - (s16)(g_CameraEye.vz >> 16));
            if (atDist < 0x80 && eyeDist < 0x80) {
                g_FieldCameraMode = 0;
            }
        }
    }

    /* Always called: final camera adjustment */
    func_80072D74();

    /* Mask g_Scene + 0x56 (clear upper nibble) */
    *(u16*)((u8*)&g_Scene + 0x56) &= 0x0FFF;
}
#endif /* XENO_PC_PORT */

void func_80073684(VECTOR* pEye, VECTOR* pAt) {
    MATRIX matSceneRotation;
    VECTOR distance;
    VECTOR transformed;

    PushMatrix();
    RotMatrix(&g_Scene.sceneAngle, &matSceneRotation);
    distance.vx = pAt->vx - pEye->vx;
    distance.vy = pAt->vy - pEye->vy;
    distance.vz = pAt->vz - pEye->vz;
    ApplyMatrixLV(&matSceneRotation, &distance, &transformed);
    pEye->vx = transformed.vx + pAt->vx;
    pEye->vz = transformed.vz + pAt->vz;
    PopMatrix();
}

void func_80073734(void* arg0) {
    *(s32*)((u8*)arg0 + 0x0) = *(s16*)((u8*)arg0 + 0x2);
    *(s32*)((u8*)arg0 + 0x4) = *(s16*)((u8*)arg0 + 0x6);
    *(s32*)((u8*)arg0 + 0x8) = *(s16*)((u8*)arg0 + 0xA);
}

// Computes a LookAt / View Matrix
// Eye: The position of the camera
// At: The point we're looking at
void FieldMatrixLookAt(MATRIX* pMatLookAt, VECTOR* pEye, VECTOR* pAt, VECTOR* pUp) {
    VECTOR vecWork;
    VECTOR zAxis;
    VECTOR xAxis;
    VECTOR yAxis;
    SVECTOR translation;

    // Forward direction (X-Axis) is defined by the line going from the eye and
    // the point we're looking at
    vecWork.vx = CONV_TO_GTE(pAt->vx - pEye->vx);
    vecWork.vy = CONV_TO_GTE(pAt->vy - pEye->vy);
    vecWork.vz = CONV_TO_GTE(pAt->vz - pEye->vz);
    
    yAxis.vx = pUp->vx;
    yAxis.vy = pUp->vy;
    yAxis.vz = pUp->vz;
    yAxis.vx >>= 0x10;
    yAxis.vy >>= 0x10;
    yAxis.vz >>= 0x10;
    
    VectorNormal(&vecWork, &zAxis);

    // Given a vector pointing in the upwards direction, we use the cross product
    // to get our right direction (Z-Axis)
    OuterProduct12(&yAxis, &zAxis, &vecWork);
    VectorNormal(&vecWork, &xAxis);

    // Finally, knowing our two other basis vectors a cross product between them
    // gives the remaining basis vector for the Y-Axis
    OuterProduct12(&zAxis, &xAxis, &vecWork);
    VectorNormal(&vecWork, &yAxis);
    
    pMatLookAt->m[0][0] = xAxis.vx;
    pMatLookAt->m[0][1] = xAxis.vy;
    pMatLookAt->m[0][2] = xAxis.vz;
    pMatLookAt->m[1][0] = yAxis.vx;
    pMatLookAt->m[1][1] = yAxis.vy;
    pMatLookAt->m[1][2] = yAxis.vz;
    pMatLookAt->m[2][0] = zAxis.vx;
    pMatLookAt->m[2][1] = zAxis.vy;
    pMatLookAt->m[2][2] = zAxis.vz;
    
    translation.vx = CONV_TO_GTE(pEye->vx) * 3;
    translation.vy = CONV_TO_GTE(pEye->vy) * 3;
    translation.vz = CONV_TO_GTE(pEye->vz) * 3;

    // We need to translate the eye position back to the origin first,
    // and combine the translation and rotation operation into a single matrix.
    ApplyMatrix(pMatLookAt, &translation, &vecWork);
    pMatLookAt->t[0] = -vecWork.vx;
    pMatLookAt->t[1] = -vecWork.vy;
    pMatLookAt->t[2] = -vecWork.vz;
}

int FieldMathInterpolateAngle(int from, int to, int delta) {
    int difference;
    int nextAngle;

    difference = PSX_ANGLE(from - to);

    // If we have less than 180 degrees difference we subtract the delta
    if (difference < PSX_DEGREES(180)) {
        nextAngle = from - delta;
        difference = PSX_ANGLE(nextAngle - to);
        if (difference >= PSX_DEGREES(180)) {
            nextAngle = to;
        }
        return PSX_ANGLE(nextAngle);
    } 
    
    // If our difference is greater than 180 degrees we add our delta
    nextAngle = from + delta;
    difference = PSX_ANGLE(nextAngle - to);
    if (difference < PSX_DEGREES(180)) {
        nextAngle = to;
    }
    return PSX_ANGLE(nextAngle);
}

// If 0, the angle will be updated in steps rather than directly set to the target
extern s32 D_800ADC18;

int FieldMathUpdateAngle(int curAngle, int targetAngle, int delta) {
    int result;
    if (D_800ADC18 == 0) {
        result = FieldMathInterpolateAngle(curAngle, targetAngle, delta);
    } else {
        result = PSX_ANGLE(targetAngle);
    }
    return result;
}

/* ---- func_800739C0: scene matrix computation (FieldComputeSceneMatrices2) -----
 * Called every frame from func_8007554C. Computes camera angles from eye/at
 * vectors, calls func_80073230 (camera update), builds view matrix via
 * FieldMatrixLookAt + FieldMatrixCreateWorldToScreen, updates per-actor
 * facing angles.
 *
 * ASM uses $s0 = &g_CameraAt+0x8 as base; +0xF8 = g_Scene (viewMatrix),
 * +0x8 = g_CameraUp. Scratchpad stack switches (0x1F8003FC) are skipped
 * on host — functions called directly with normal stack. */
extern s16 D_800B2184[];
extern MATRIX D_800AFC30;
extern s32 D_800AFC44, D_800AFC48, D_800AFC4C;
extern s32 D_800AF8E0, D_800AF8E4, D_800AF8E8;
extern MATRIX D_800AF85C;
extern s32 D_800B00B4;
extern u8 D_800ADB05;
extern s16 D_800B21B4;
extern s32 D_800ADBFC;
extern u8 D_8006FB04;
extern void func_8008110C(void);
extern void FieldMatrixCreateWorldToScreen(void);
/* ratan2 prototype comes from psyq/libgte.h (already included above); a local
 * extern here conflicted with it (s32/int vs the real long params). */
extern void func_800223B0(s32 actorIdx, s32 angle);
extern void func_80021FE0(void* pSpriteData, s16 angle);

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/field/nonmatchings/main/misc2", func_800739C0);
#else
void func_800739C0(void) {
    s32 yawCur, yawTarget;
    s32 camDist;
    s32 pitch;

    func_8008110C();
    RotMatrix((SVECTOR*)D_800B2184, &D_800AFC30);

    D_800AFC4C = 0;
    D_800AFC48 = 0;
    D_800AFC44 = 0;

    /* Yaw from current eye/at -> +0xA (curAngleY), asm 80073A10-80073A4C:
     * sh to g_CamInterpolation+0xA. */
    yawCur = ratan2(g_CameraAt.vz - g_CameraEye.vz,
                    g_CameraAt.vx - g_CameraEye.vx);
    g_CamInterpolation.curAngleY = (s16)(yawCur - 0x400);

    /* Yaw from target eye2/at2 -> +0x8 (targetAngleY), asm 80073A34-80073A80:
     * sh to g_CamInterpolation+0x8. This is the term OP_UPDATE_CHARACTER
     * subtracts (Noah: camera2Tan). The port had these two stores swapped. */
    yawTarget = ratan2(g_CameraAt2.vz - g_CameraEye2.vz,
                       g_CameraAt2.vx - g_CameraEye2.vx);
    g_CamInterpolation.targetAngleY = (s16)(yawTarget - 0x400);

    /* Horizontal distance */
    camDist = FieldGetVec2Magnitude(
        (g_CameraAt.vx - g_CameraEye.vx) >> 16,
        (g_CameraAt.vz - g_CameraEye.vz) >> 16);

    /* Pitch */
    pitch = ratan2(camDist, (g_CameraAt.vy - g_CameraEye.vy) >> 16);
    D_800B00B4 = pitch;

    /* Camera update (ASM uses scratchpad stack — skipped on host) */
    func_80073230();

    /* Build camera vectors with D_800AF8E0/E4/E8 offsets */
    {
        VECTOR eyeVec, atVec;
        eyeVec.vx = g_CameraEye.vx + D_800AF8E0;
        eyeVec.vy = g_CameraEye.vy + D_800AF8E4;
        eyeVec.vz = g_CameraEye.vz + D_800AF8E8;
        atVec.vx = g_CameraAt.vx + D_800AF8E0;
        atVec.vy = g_CameraAt.vy + D_800AF8E4;
        atVec.vz = g_CameraAt.vz + D_800AF8E8;

        if (D_800ADC18 == 0) {
            /* Compute new view matrix */
            FieldMatrixLookAt((MATRIX*)&g_Scene, &eyeVec, &atVec, &g_CameraUp);
            /* Copy g_Scene.viewMatrix to D_800AF85C */
            D_800AF85C = g_Scene.viewMatrix;
        } else {
            /* Restore from D_800AF85C */
            g_Scene.viewMatrix = D_800AF85C;
            FieldMatrixLookAt((MATRIX*)&g_Scene, &eyeVec, &atVec, &g_CameraUp);
        }
    }

    /* World-to-screen matrix (ASM uses scratchpad stack — skipped on host) */
    FieldMatrixCreateWorldToScreen();

    /* Per-actor angle updates */
    {
        s32 actorIdx;
        for (actorIdx = 0; actorIdx < D_800ADBFC; actorIdx++) {
            u8* pActor = (u8*)g_FieldActors + actorIdx * 0x5C;
            u16 status = *(u16*)(pActor + 0x58);
            u8* pData;

            if ((status & 0xF40) == 0 || (status & 0x20))
                continue;

            pData = (u8*)(uintptr_t)*(u32*)(pActor + 0x4C);
            if (pData == NULL)
                continue;

            {
                s32 flags = *(s32*)(pData + 0x04);
                if (flags & 0x100000)
                    continue;
                if ((flags & 0x600) == 0x200)
                    continue;
            }

            {
                /* Facing smoothing toward the commanded angle (+0x106 ->
                 * +0x108). Retail asm 80073CD8-80073D54: SKIP when
                 * scriptFlags bit 0x8000 is set (the port had this guard
                 * inverted, freezing every walking actor's facing).
                 * - pflags(+0x14) bit 0x200000 set AND scriptFlags 0x1800
                 *   clear: snap toward the octant packed in pflags bits
                 *   11-13 at fixed speed 0x200 (asm .L80073D38).
                 * - otherwise: smooth toward +0x106 at speed D_800B21B4
                 *   (flags +0x04 bit 0x2000) or the actor's own +0x11E
                 *   (asm .L80073D08). */
                s32 dataFlags = *(s32*)(pData + 0x00);
                if (!(dataFlags & 0x8000)) {
                    s32 pflags = *(s32*)(pData + 0x14);
                    if ((pflags & 0x200000) && !(dataFlags & 0x1800)) {
                        s16 targAng = (s16)((((pflags >> 11) - 2) & 7) << 9);
                        *(s16*)(pData + 0x108) =
                            (s16)FieldMathUpdateAngle(
                                *(s16*)(pData + 0x108), targAng, 0x200);
                    } else {
                        s32 flags4 = *(s32*)(pData + 0x04);
                        s16 speed = (flags4 & 0x2000)
                                        ? D_800B21B4
                                        : *(s16*)(pData + 0x11E);
                        *(s16*)(pData + 0x108) =
                            (s16)FieldMathUpdateAngle(
                                *(s16*)(pData + 0x108),
                                *(s16*)(pData + 0x106), speed);
                    }
                }

                /* Facing direction push to the sprite. Retail asm 80073D94
                 * reads the CURRENT camera yaw (lhu g_CamInterpolation+0xA)
                 * unsigned; the port read +0x8. */
                if (!D_800ADB05) {
                    s32 flags = *(s32*)(pData + 0x04);
                    if (!(flags & 0x1000000)) {
                        s32 camAng = *(u16*)((u8*)&g_CamInterpolation + 0xA);
                        s32 actorAng = *(u16*)(pData + 0x108);
                        func_800223B0(*(s32*)(pActor + 0x04),
                                     (camAng + actorAng) << 16 >> 16);
                    } else {
                        func_80021FE0(*(s32*)(pActor + 0x04),
                                     *(s16*)(pData + 0x108));
                    }
                }
            }
        }
    }

    /* System mode 0: call func_80281B00 */
    if (g_FieldSystemMode == 0) {
        func_80281B00(&D_8006FB04);
    }
}
#endif /* XENO_PC_PORT */

extern s16 D_800B218E;
extern void func_800AA9DC(void* pModelData);

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/field/nonmatchings/main/misc2", func_80073E38);
#else
void func_80073E38(void) {
    s32 i;
    u8* pActor;

    for (i = 0; i < g_FieldNumActors; i++) {
        pActor = (u8*)g_FieldActors + i * 0x5C;

        if (!(*(u16*)(pActor + 0x58) & 0x40)) {
            u8* pModelData = (u8*)(uintptr_t)*(u32*)(pActor + 0x00);
            u16 status;

            func_800AA9DC(pModelData);

            status = *(u16*)(pActor + 0x58);
            if (D_800B218E != 0) {
                *(s16*)(pModelData + 0x12) = (status & 0x10) ? 5 : 4;
            } else if (status & 0x0C) {
                *(s16*)(pModelData + 0x12) = 1;
            } else if (status & 0x4000) {
                *(s16*)(pModelData + 0x12) = 3;
            } else if (status & 0x10) {
                *(s16*)(pModelData + 0x12) = 2;
            } else {
                *(s16*)(pModelData + 0x12) = 0;
            }
        }
    }
}
#endif /* XENO_PC_PORT */

#ifdef XENO_PC_PORT
static void FieldClearOTagR4(void* ot, s32 count) {
    u32* tags = ot;
    s32 i;

    if (count == 0) {
        return;
    }

    tags[0] = 0x00FFFFFF;
    for (i = 1; i < count; i++) {
        tags[i] = (u32)(uintptr_t)&tags[i - 1] & 0x00FFFFFF;
    }
}
#endif

void FieldClearAndSwapOTagInternal(void) {
    if (g_FieldSystemMode == SYSTEM_MODE_PC_HDD) {
#ifndef XENO_PC_PORT
        asm("break 0x400");   /* MIPS trap; host assembler can't emit it. PC-HDD
                                 dev path only -- never taken in the CD_ROM port. */
#endif
    }

    g_FieldCurRenderContextIndex = (g_FieldCurRenderContextIndex + 1) % 2;
    g_FieldCurRenderContext = &g_FieldRenderContexts[g_FieldCurRenderContextIndex];
#ifdef XENO_PC_PORT
    FieldClearOTagR4((u8*)g_FieldCurRenderContext + 0x80D4, 0x8);
    ClearOTagR(g_FieldCurRenderContext->ot3, 0x8);
#else
    ClearOTagR(g_FieldCurRenderContext->ot3, 0x8);
#endif
}

void FieldClearAndSwapOTag(void) {
    FieldClearAndSwapOTagInternal();
#ifdef XENO_PC_PORT
    FieldClearOTagR4((u8*)g_FieldCurRenderContext + 0xCC, 0x1000);
    if (g_FieldRenderContextUseOT2) {
        FieldClearOTagR4((u8*)g_FieldCurRenderContext + 0x40D0, 0x1000);
    }
#else
    ClearOTagR((u_long*)((u8*)g_FieldCurRenderContext + 0xCC), 0x1000);
    if (g_FieldRenderContextUseOT2) {
        ClearOTagR((u_long*)((u8*)g_FieldCurRenderContext + 0x40D0), 0x1000);
    }
#endif
}

void FieldMatrixCopy(MATRIX* dest, MATRIX* source) {
    FieldMatrixCopyTransform(dest, source);
    FieldMatrixCopyTranslation(dest, source);
}

void FieldMatrixCopyTranslation(MATRIX* dest, MATRIX* source) {
    dest->t[0] = source->t[0];
    dest->t[1] = source->t[1];
    dest->t[2] = source->t[2];
}

void FieldMatrixCopyTransform(MATRIX* dest, MATRIX* source) {
    dest->m[0][0] = source->m[0][0];
    dest->m[0][1] = source->m[0][1];
    dest->m[0][2] = source->m[0][2];
    dest->m[1][0] = source->m[1][0];
    dest->m[1][1] = source->m[1][1];
    dest->m[1][2] = source->m[1][2];
    dest->m[2][0] = source->m[2][0];
    dest->m[2][1] = source->m[2][1];
    dest->m[2][2] = source->m[2][2];
}

/* ---- func_80074108: background/camera draw dispatch -------------------------
 * Full decompile from ASM (371 lines). Three phases:
 *   1. CLUT/color table populate: 8-iteration loop filling D_800AFD24
 *   2. Camera matrix setup: LookAt, angle update, matrix chain for bg layer
 *   3. Quad rendering: multiple FieldRenderQuad loops for background/tiles */
extern u16 D_800AFD24[0x80];
extern u16 D_800AFC08[0x80];
extern u16 D_800ADC24[8];
extern u8 D_800B0050[];  /* base symbol; RECT is at D_800B0050 - 4 */
extern VECTOR g_CameraEye;
extern VECTOR g_CameraAt;
extern u16 D_800B233E;
extern s16 D_800ADB48;
extern s16 D_800ADB4A;
extern u8 D_800B21D1;
extern s32 D_8004F378;
extern u8 D_800B0F7C[];
extern u8 D_800B06BC[];
extern u8 D_800B0FEC[];
extern u8 D_800B1E00[];
extern CameraInterpolation g_CamInterpolation;
extern void func_80070594(MATRIX* dest);
/* Square0 prototype comes from psyq/libgte.h (already included above); a local
 * extern here conflicted with it (void vs the real VECTOR* return) and was
 * unused (Square0 is never called in this file). */
extern s32 FieldGetVec2Magnitude(s32 dx, s32 dy);
extern void func_8007AC58(u_long* ot, void* pQuad, MATRIX* pMat, s32 renderCtx);
extern void SetTransMatrix(MATRIX* m);

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/field/nonmatchings/main/misc2", func_80074108);
#else
void func_80074108(void) {
    s32 i, j;
    u16* pDst;
    u16* pSrc;
    u16* pFlags;
    u8 sceneFlag;
    s32 idx;
    s32 newAngle;
    s32 unused6C;
    s32 unused64;
    u8* pRenderCtx;
    s32 renderCtxIdx;
    u8* pQuadData;
    u32 mask24;
    u32 maskHi;
    MATRIX matView;
    MATRIX matTemp;
    MATRIX matComposite;
    MATRIX matWork;
    SVECTOR svRot;

    /* ======== PHASE 1: CLUT / color table population (loop 0..7) ======== */
    pDst = D_800AFD24;
    pSrc = D_800AFC08;
    pFlags = D_800ADC24;
    sceneFlag = *(u8*)((u8*)&g_Scene + 0x65);
    idx = 0;

    for (i = 0; i < 8; i++) {
        if (sceneFlag & pFlags[i]) {
            for (j = 0; j < 0x10; j++) {
                pDst[idx + j] = 0;
            }
        } else {
            for (j = 0; j < 0x10; j++) {
                pDst[idx + j] = pSrc[j];
            }
            pSrc += 0x10;
        }
        idx += 0x10;
    }

    /* Upload CLUT via LoadImage. RECT is at D_800B0050-4, w field at D_800B0050+0. */
    {
        u8* pRect = (u8*)D_800B0050 - 4;
        *(u16*)(pRect + 4) = 0x80;  /* sh 0x80, 0($v1) where $v1 = D_800B0050 */
#ifdef XENO_PC_PORT
        if (*(s16*)(pRect + 6) != 0)  /* guard: stubbed h=0 would crash LoadImage */
#endif
        LoadImage((RECT*)pRect, (u_long*)D_800AFD24);
    }

    /* ======== PHASE 2: Camera / matrix setup ======== */
    SetGeomScreen(0x80);
    SetGeomOffset(0x10A, 0xA6);

    /* Build view matrix: FieldMatrixLookAt(matView, eye, at, up)
     * eye = {0, g_CameraEye.vy - g_CameraAt.vy, -mag<<16}
     * at  = {0, 0, 0}
     * up  = (VECTOR*)((u8*)&g_CameraEye + 0x20) = &g_CameraUp (0x800AF880+0x20
     *       = 0x800AF8A0; adjacency holds in the host blob layout) */
    {
        s32 dx = (g_CameraEye.vx - g_CameraAt.vx) >> 16;
        s32 dy = (g_CameraEye.vz - g_CameraAt.vz) >> 16;
        s32 mag = FieldGetVec2Magnitude(dx, dy);
        VECTOR eye;
        VECTOR at;
        eye.vx = 0; eye.vy = g_CameraEye.vy - g_CameraAt.vy; eye.vz = (-mag) << 16;
        at.vx = 0;  at.vy = 0;                         at.vz = 0;
        FieldMatrixLookAt(&matView, &eye, &at, (VECTOR*)((u8*)&g_CameraEye + 0x20));
    }

    /* Setup matWork for composite matrix chain */
    func_80070594(&matWork);
    SetRotMatrix(&matWork);
    SetTransMatrix(&matWork);

    /* Actor-based camera angle */
    {
        u16 actorIdx = D_800B233E;
        s16 curAngle = D_800ADB48;
        u8* pActor = (u8*)g_FieldActors + actorIdx * 0x5C;
        s16 actorAngle = *(s16*)(*(u32*)(pActor + 0x4C) + 0x106);
        s32 targetAngle = actorAngle + (*(u16*)((u8*)&g_CamInterpolation + 0xA) + 0x400);
        D_800ADB4A = (s16)targetAngle;
        /* Retail 800742F8--80074308 sign-extends the low half of the
         * actor/camera angle sum (sll 16; sra 16) before interpolation.
         * Shifting the sum first collapsed almost every positive target to
         * zero, rotating the room-background camera away from the painting. */
        newAngle = FieldMathUpdateAngle(curAngle, (s16)targetAngle, 0x40);
    }
    D_800ADB48 = (s16)newAngle;

    /* Build Y-rotation matrix, chain into view */
    FieldMatrixResetTranslation(&matTemp);
    svRot.vx = 0; svRot.vy = (s16)newAngle; svRot.vz = 0;
    RotMatrix(&svRot, &matTemp);
    MulMatrix2(&matView, &matTemp);
    unused6C = 0x1000;
    CompMatrix(&matWork, &matTemp, &matComposite);

    /* ---- First quad loop: asm runs entry 0x14 only (D_800B0F7C) ---- */
    if (!D_800B21D1 && D_800ADC18 == 0 && D_8004F378 == 0) {
        pQuadData = D_800B0F7C;
        pRenderCtx = (u8*)g_FieldCurRenderContext;
        renderCtxIdx = g_FieldCurRenderContextIndex;
        for (i = 0; i < 1; i++) {
            FieldRenderQuad((u_long*)(pRenderCtx + 0x80D4),
                           pQuadData, &matComposite, renderCtxIdx);
            pQuadData += 0x70;
        }
    }

    /* ---- Second matrix chain ---- */
    func_80070594(&matTemp);
    MulMatrix2(&matView, &matTemp);
    unused6C = 0x1000;
    CompMatrix(&matWork, &matTemp, &matComposite);
    MulMatrix0(&matWork, &matTemp, (MATRIX*)((u8*)&g_Scene + 0xF4));
    SetRotMatrix(&matWork);
    SetTransMatrix(&matWork);

    /* ---- Third matrix chain ---- */
    func_80070594(&matTemp);
    MulMatrix2((MATRIX*)&g_Scene, &matTemp);
    unused6C = 0x1000;
    CompMatrix(&matWork, &matTemp, &matComposite);
    FieldMatrixCopy(&matWork, &matComposite);

    /* ---- Rotation for trigger-zone quad loop ---- */
    svRot.vx = 0x400; svRot.vy = 0; svRot.vz = 0;
    {
        MATRIX matRot;
        RotMatrix(&svRot, &matRot);

        if (!D_800B21D1 && D_800ADC18 == 0 && D_8004F378 == 0) {
            u8* pTrigger = (u8*)(uintptr_t)g_pFieldTriggerZones;
            pRenderCtx = (u8*)g_FieldCurRenderContext;
            renderCtxIdx = g_FieldCurRenderContextIndex;

            /* Trigger-zone loop: asm starts at D_800B06BC + 0x700 and runs
             * entries 0x10..0x13, the D_800B0DBC extra-quad area. */
            for (i = 0; i < 4; i++) {
                func_80070594(&matTemp);
                unused64 = *(s16*)(pTrigger + 0x40 + i * 4);
                unused6C = *(s16*)(pTrigger + 0x42 + i * 4);
                CompMatrix(&matWork, &matTemp, &matComposite);
                FieldMatrixCopyTransform(&matComposite, &matRot);
                func_8007AC58((u_long*)(pRenderCtx + 0x80D4),
                             D_800B06BC + 0x700 + i * 0x70, &matComposite, renderCtxIdx);
            }

            /* Third quad loop: D_800B06BC, 0x10 iterations */
            pQuadData = D_800B06BC;
            for (i = 0; i < 0x10; i++) {
                FieldRenderQuad((u_long*)(pRenderCtx + 0x80D4),
                               pQuadData, &matWork, renderCtxIdx);
                pQuadData += 0x70;
            }
        }
    }

    /* ---- Fourth quad loop: D_800B0FEC, 4 iterations ---- */
    if (!D_800B21D1 && D_800ADC18 == 0 && D_8004F378 == 0) {
        pQuadData = D_800B0FEC;
        pRenderCtx = (u8*)g_FieldCurRenderContext;
        renderCtxIdx = g_FieldCurRenderContextIndex;
        for (i = 0; i < 4; i++) {
            FieldRenderQuad((u_long*)(pRenderCtx + 0x80D4),
                           pQuadData, &matWork, renderCtxIdx);
            pQuadData += 0x70;
        }
    }

    /* ---- Final: D_800B1E00 word swap + GTE reset ---- */
    {
        s32* pEntry = (s32*)(D_800B1E00 + g_FieldCurRenderContextIndex * 0xC0);
        u8* pRC = (u8*)g_FieldCurRenderContext;
        s32 valA = pEntry[0];
        s32 valB = *(s32*)(pRC + 0x80D4);
        pEntry[0] = (valA & 0xFF000000) | (valB & 0x00FFFFFF);
        *(s32*)(pRC + 0x80D4) = (valB & 0xFF000000) | (pEntry[0] & 0x00FFFFFF);
    }

    SetGeomOffset(0xA0, 0x70);
    SetGeomScreen(*(s32*)((u8*)&g_Scene + 0x68));
}
#endif /* XENO_PC_PORT */


#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/field/nonmatchings/main/misc2", func_8007469C);
#else
s32 func_8007469C(void) {
    s32 i;
    u8* pActor;

    if (g_FieldNumActors <= 0) {
        return 0;
    }

    pActor = (u8*)g_FieldActors;
    for (i = 0; i < g_FieldNumActors; i++, pActor += 0x5C) {
        u16 status = *(u16*)(pActor + 0x58);

        if (!(status & 0x40) && (status & 0x8000)) {
            return 1;
        }
    }

    return 0;
}
#endif /* XENO_PC_PORT */

extern u16 D_800AFE9C;
extern u16 D_800AFEA0;
extern u16 D_800C2694;
extern u16 D_800C38F8;
extern u16 D_800C3900;
extern u16 D_800C3908;
extern u16 D_800ADB00;
extern s32 D_800ADBDC;
extern u16 g_C1ButtonState;
extern u16 g_C2ButtonState;
extern u16 g_C1ButtonStateReleased;
extern u16 g_C2ButtonStateReleased;
extern u16 g_C1ButtonStatePressedOnce;
extern u16 g_C2ButtonStatePressedOnce;
extern s32 ControllerPopState(void);
extern void ControllerResetState(void);
extern u8 D_80065848;
extern void func_8007AE78(s32 arg0, void* arg1);

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/field/nonmatchings/main/misc2", FieldPollControllers);
#else
void FieldPollControllers(void) {
    D_800AFE9C = 0;
    D_800AFEA0 = 0;
    D_800C2694 = 0;
    D_800C38F8 = 0;
    D_800C3900 = 0;
    D_800C3908 = 0;

    while (ControllerPopState() != 0) {
        u16 mask = g_FieldControl.controllerBtnMask;

        D_800AFE9C |= g_C1ButtonState & mask;
        D_800AFEA0 |= g_C2ButtonState;
        D_800C2694 |= g_C1ButtonStateReleased & mask;
        D_800C38F8 |= g_C2ButtonStateReleased;
        D_800C3900 |= g_C1ButtonStatePressedOnce & mask;
        D_800C3908 |= g_C2ButtonStatePressedOnce;
    }

    D_800AFE9C &= D_800ADB00;
    D_800C3900 &= D_800ADB00;
    D_800C2694 &= D_800ADB00;

#ifdef XENO_PC_PORT
    /* DIAGNOSTIC / TEST TOOLING (remove with PcPort_FieldTestInput* in
     * pc_port/src/test_input.c): scripted field input from
     * XENO_FIELD_TEST_INPUT, merged at retail's post-drain accumulator seam so
     * button-gated scenes (e.g. the Map 4 prologue's OP31 Circle wait) can be
     * driven headlessly. Unset => no effect; real pads are still ORed in. */
    {
        extern int PcPort_FieldTestInputInit(void);
        extern void PcPort_FieldTestInputMerge(u16*, u16*, u16*);

        PcPort_FieldTestInputInit();
        PcPort_FieldTestInputMerge(&D_800AFE9C, &D_800C3900, &D_800C2694);
    }

    /* Headless field-test: synthesize Circle-released when a text box is
     * waiting on confirm (window flags bit 0x8). Real pads still work; this
     * only ORs the bit so well/dialog scripts can finish without a human. */
    {
        const char* ft = getenv("XENO_FIELD_TEST");
        if (ft && ft[0] == '1') {
            s32 i;
            for (i = 0; i < 4; i++) {
                u8* pWin = (u8*)&g_FieldTextBoxes[i] + 0x18;
                if (g_FieldTextBoxes[i].visibility == 0 && (*(u16*)(pWin + 0x10) & 0x8)) {
                    D_800C2694 |= 0x20;
                    break;
                }
            }
        }
    }
#endif

    ControllerResetState();
    func_8007AE78(1, &D_80065848);

    if (D_800ADC18 != 0) {
        D_800AFE9C = 0;
        D_800AFEA0 = 0;
        D_800C2694 = 0;
        D_800C38F8 = 0;
        D_800C3900 = 0;
        D_800C3908 = 0;
    }

    if (D_800ADBDC == 0) {
        D_800C2694 &= 0xFF7F;
    }
}
#endif /* XENO_PC_PORT */

extern s32 D_80059578;
extern s32 D_800595C0;
extern s32 D_80050104;
extern s16 D_800B21AE;
extern s16 D_800B21B0;
extern s16 D_800B21B2;
extern s32 D_800B21BC;
extern s32 D_800B21C0;
extern s32 D_800B21C4;
extern u8 D_800B2190;
extern u8 D_800B2191;
extern u8 D_800B2192;
extern u8 D_800B2194;
extern u8 D_800B2195;
extern u8 D_800B2196;
extern s16 D_800B2198;
extern s16 D_800B219A;
extern void func_8002C6E0(u8 a0, u8 a1, u8 a2);
extern void func_80048AB0(s32 a0, s32 a1, s32 a2);
extern s32 func_800AAA74(void* modelData);
extern s32 func_8002C700(void* a0, void* a1, void* a2, s32 a3);
/* PC-HDD timing marker block written by the tail of func_800748E8 when
 * g_FieldSystemMode == 0 (retail 800751C8). */
extern u8 D_8006FB10;
/* func_800748E8's model-offset block (retail 80075004-80075020). */
extern void func_80030B14(void* pMatrix);
extern void func_80030C40(s16 a, s16 b, s16 c);
extern s16 D_800AFB04;
extern s16 D_800AFB06;
extern s16 D_800AFB08;
extern s32 D_800ADB58;
extern s32 D_800ADB5C;
extern void func_800305D8(void* pModelOffset);
#ifdef XENO_PC_PORT
extern s32 D_800B2264;
#endif

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/field/nonmatchings/main/misc2", func_800748E8);
#else
void func_800748E8(void) {
    VECTOR scale;
    MATRIX work;
    s32 actorIndex;
#ifdef XENO_PC_PORT
    static s32 s_modelDiagFrames;
    s32 modelConsidered = 0;
    s32 modelEmitted = 0;
    s32 modelHidden = 0;
    s32 modelStatus20 = 0;
#endif

    D_80059578 = 0;
    D_800595C0 = 0;

    if (D_800B218E != 0) {
        func_8002C6E0(D_800B2190, D_800B2191, D_800B2192);
        SetFarColor(D_800B2194, D_800B2195, D_800B2196);
        func_80048AB0(D_800B2198, D_800B219A, *(s32*)((u8*)&g_Scene + 0x68));
    }

    scale.vx = g_WorldScale;
    scale.vy = g_WorldScale;
    scale.vz = g_WorldScale;
    ScaleMatrix((MATRIX*)((u8*)&g_Scene + 0xF4), &scale);
    CompMatrix(&g_Scene.worldToScreenMatrix, &D_800AFC30, &work);

    D_80050104 = 0;
    D_800B21C0 += D_800B21B2;
    D_800B21BC += D_800B21AE;
    D_800B21C4 += D_800B21B0;

    for (actorIndex = 0; actorIndex < g_FieldNumActors; actorIndex++) {
        u8* actor = (u8*)g_FieldActors + actorIndex * 0x5C;
        u32 status;

        *(u32*)(actor + 0x2C) = *(u32*)(actor + 0x0C);
        *(u32*)(actor + 0x30) = *(u32*)(actor + 0x10);
        *(u32*)(actor + 0x34) = *(u32*)(actor + 0x14);
        *(u32*)(actor + 0x38) = *(u32*)(actor + 0x18);
        *(u32*)(actor + 0x3C) = *(u32*)(actor + 0x1C);
        *(u32*)(actor + 0x40) = *(u32*)(actor + 0x20);
        *(u32*)(actor + 0x44) = *(u32*)(actor + 0x24);
        *(u32*)(actor + 0x48) = *(u32*)(actor + 0x28);

        status = *(u16*)(actor + 0x58);
        if (status & 0x40) {
            continue;
        }

        {
            u8* actorData = (u8*)(uintptr_t)*(u32*)(actor + 0x4C);
            u8* modelData = (u8*)(uintptr_t)*(u32*)actor;
            u8* env = (u8*)&D_800B218E;
            u8 mode;
            MATRIX modelMatrix;
            MATRIX branchMatrix;
            VECTOR modelPosition;
            SVECTOR branchRotation;
            SVECTOR row;
            s32 branchMatrixReady;
            s32 hiddenByModel;

            assert(modelData != NULL);
            branchMatrixReady = 0;

            if (actorIndex < D_800ADBFC) {
                u8 parentActorIndex;

                assert(actorData != NULL);
                mode = *(u32*)(actorData + 0x12C) & 3;

                /* Retail 80074AFC-80074BE8: modes 1/2/3 apply the angle at
                 * ActorData+0x70 around X/Y/Z respectively, append that to
                 * the actor transform, then compose it with world-to-screen.
                 * Mode 0 continues through the optional-matrix/parent paths. */
                if (mode != 0) {
                    branchRotation.vx = 0;
                    branchRotation.vy = 0;
                    branchRotation.vz = 0;

                    if (mode == 1) {
                        branchRotation.vx = *(u16*)(actorData + 0x70);
                    } else if (mode == 2) {
                        branchRotation.vy = *(u16*)(actorData + 0x70);
                    } else {
                        branchRotation.vz = *(u16*)(actorData + 0x70);
                    }

                    RotMatrix(&branchRotation, &branchMatrix);
                    MulMatrix2((MATRIX*)(actor + 0x0C), &branchMatrix);
                    branchMatrix.t[0] = *(s32*)(actor + 0x20);
                    branchMatrix.t[1] = *(s32*)(actor + 0x24);
                    branchMatrix.t[2] = *(s32*)(actor + 0x28);
                    CompMatrix(&g_Scene.worldToScreenMatrix, &branchMatrix,
                               &modelMatrix);
                    branchMatrixReady = 1;
                } else {
                    u16 additionalMatrix = *(u16*)(actorData + 0x128);

                    /* Retail 80074BEC-80074C70: a non-FFFF value selects a
                     * model-resource matrix by high-nibble table selector and
                     * low-12-bit index. Adjust childMatrix in place, compose it
                     * through world-to-screen and the actor transform, then
                     * retain the adjusted child transform for descendants. */
                    if (additionalMatrix != 0xFFFF) {
                        MATRIX* childMatrix = (MATRIX*)(actor + 0x2C);

                        func_801E72CC(childMatrix, childMatrix,
                                      additionalMatrix >> 12,
                                      additionalMatrix & 0xFFF);
                        CompMatrix(&g_Scene.worldToScreenMatrix, childMatrix,
                                   &branchMatrix);
                        CompMatrix(&branchMatrix, (MATRIX*)(actor + 0x0C),
                                   &modelMatrix);
                        CompMatrix(childMatrix, (MATRIX*)(actor + 0x0C),
                                   childMatrix);
                        branchMatrixReady = 1;
                    } else {
                        /* Retail 80074C74-80074D2C: 0xFF means no parent;
                         * otherwise compose through the named parent's child
                         * matrix and update this actor's child matrix in place. */
                        parentActorIndex = *(u8*)(actorData + 0x75);
                        if (parentActorIndex != 0xFF) {
                            FieldActor* parentActor = &g_FieldActors[parentActorIndex];

                            CompMatrix(&g_Scene.worldToScreenMatrix,
                                       &parentActor->childMatrix, &branchMatrix);
                            CompMatrix(&branchMatrix, (MATRIX*)(actor + 0x0C),
                                       &modelMatrix);
                            CompMatrix(&parentActor->childMatrix,
                                       (MATRIX*)(actor + 0x0C),
                                       (MATRIX*)(actor + 0x2C));
                            branchMatrixReady = 1;
                        }
                    }
                }
            }

            if (!branchMatrixReady) {
                if ((env[0x44] & 0x7F) == 0) {
                    *(s32*)(actor + 0x20) += *(s16*)(env + 0x20);
                    *(s32*)(actor + 0x24) += *(s16*)(env + 0x24);
                    *(s32*)(actor + 0x28) += *(s16*)(env + 0x22);
                }

                if ((env[0x44] & 0x7F) == 1) {
                    *(s32*)(actor + 0x20) += *(s16*)(env + 0x20);
                    *(s32*)(actor + 0x24) += *(s16*)(env + 0x24);
                    *(s32*)(actor + 0x28) += *(s16*)(env + 0x22);
                }

                assert((env[0x44] & 0x80) == 0);
            }

            if (*(s16*)(modelData + 0x12) == 1) {
                /* Retail 80074F98-80075020: modelMatrix := work, then the
                 * model-offset/scale helpers. fp == 1, s3 == &modelMatrix
                 * (sp+0x78), the source is sp+0x58 (work), sp+0x28 is scale. */
                modelMatrix = work;
                ScaleMatrix(&modelMatrix, &scale);
                func_80030B14(&modelMatrix);
                func_80030C40(D_800AFB04, D_800AFB06, D_800AFB08);
            }

            D_80050104 = 0;
#ifdef XENO_PC_PORT
            modelConsidered++;
#endif
            if (status & 0x20) {
#ifdef XENO_PC_PORT
                modelStatus20++;
#endif
                continue;
            }

            /* Retail 8007504C-80075088: when the actor is flagged 0x2000 and
             * carries a model-offset pointer, hand it to func_800305D8 with the
             * actor index published in D_800ADB58 and D_800ADB5C cleared. */
            if ((status & 0x2000) != 0 && *(u32*)(modelData + 0x14) != 0) {
                D_800ADB58 = actorIndex;
                D_800ADB5C = 0;
                func_800305D8((void*)(uintptr_t)*(u32*)(modelData + 0x14));
            }

            if (!branchMatrixReady) {
                /*
                 * func_800748E8 is handwritten/nonmatching GTE code. The original
                 * asm (80074E14-80074ED8) builds modelMatrix.R = work.R x actorR
                 * COLUMN by COLUMN: for each j it gathers actor matrix column j
                 * with stride-6 halfword loads (lhu +0x0/+0x6/+0xC from bases
                 * actor+0xC/+0xE/+0x10), transforms it with mvmva cv=3
                 * (= ApplyMatrixSV), and stores the result as modelMatrix column j
                 * (sh +0x0/+0x6/+0xC from dests +0x0/+0x2/+0x4). The previous C
                 * read overlapping SVECTORs and stored rows, producing a
                 * near-degenerate rotation.
                 */
                {
                    s16* actorR = (s16*)(actor + 0x0C); /* row-major 3x3 */
                    SVECTOR col;
                    s32 j;

                    for (j = 0; j < 3; j++) {
                        col.vx = actorR[0 + j];
                        col.vy = actorR[3 + j];
                        col.vz = actorR[6 + j];
                        gte_ApplyMatrixSV(&work, &col, &row);
                        modelMatrix.m[0][j] = row.vx;
                        modelMatrix.m[1][j] = row.vy;
                        modelMatrix.m[2][j] = row.vz;
                    }
                }

                modelPosition.vx = *(s16*)(actor + 0x20);
                modelPosition.vy = *(s16*)(actor + 0x24);
                modelPosition.vz = *(s16*)(actor + 0x28);
#ifdef XENO_PC_PORT
                ApplyMatrixLV(&work, &modelPosition, (VECTOR*)modelMatrix.t);
                /*
                 * Original func_800748E8 asm (53EC-542C) loads work.t into the GTE
                 * translation registers (ctc2 $5/$6/$7) and transforms the actor
                 * position with mvmva cv=0, which INCLUDES the translation:
                 * modelMatrix.t = work.R * pos + work.t. ApplyMatrixLV is the
                 * rotation-only (cv=3) library op, so add work.t explicitly here.
                 */
                modelMatrix.t[0] += work.t[0];
                modelMatrix.t[1] += work.t[1];
                modelMatrix.t[2] += work.t[2];
#else
                /* Retail 80074F80-80074FE4 inlines ApplyMatrixLV: ctc2 TR from
                 * work.t, then gte_ldlv0 (lhu/or/mtc2 $0 + lwc2 $1), then
                 * MVMVA 1,0,0,0,0 (cv=TR, so the translation IS included -
                 * the explicit work.t add above is a port-only workaround) and
                 * swc2 $25/$26/$27 (== gte_stlvnl). */
                gte_SetTransMatrix(&work);
                gte_ldlv0(&modelPosition);
                gte_rt();
                gte_stlvnl((VECTOR*)modelMatrix.t);
#endif
                /* Retail 80074F3C-80074F94: the low two status bits select
                 * an alternate model-matrix construction. Mode 0 keeps the
                 * ordinary matrix above. Mode 1 composes the scaled scene
                 * matrix with the actor transform. Modes 2/3 copy and scale
                 * the actor rotation. All nonzero modes then append the
                 * camera-rotation matrix before rejoining at 80074F98. */
                if ((status & 3) == 1) {
                    MulMatrix0(&g_Scene.unkF4, (MATRIX*)(actor + 0x0C),
                               &modelMatrix);
                    MulMatrix2(&g_Scene.camRotationMatrix, &modelMatrix);
                } else if ((status & 3) != 0) {
                    FieldMatrixCopyTransform(&modelMatrix,
                                             (MATRIX*)(actor + 0x0C));
                    ScaleMatrix(&modelMatrix, &scale);
                    MulMatrix2(&g_Scene.camRotationMatrix, &modelMatrix);
                }
            }
            gte_SetRotMatrix(&modelMatrix);
            gte_SetTransMatrix(&modelMatrix);

            hiddenByModel = func_800AAA74(modelData);
            if (hiddenByModel != 0 && (env[0x44] & 0x80) == 0) {
#ifdef XENO_PC_PORT
                modelHidden++;
#endif
                continue;
            }

            /*
             * Original asm .L800750EC (800750EC-80075128): after the
             * func_800AAA74 visibility check (which leaves the GTE loaded with
             * D_800B00E8 for its own bbox test), retail re-issues a full inline
             * SetRotMatrix+SetTransMatrix from modelMatrix (lw 0x0..0x1C($s4)
             * -> ctc2 $0..$7) before func_8002C700 -- the prim procs' rtpt must
             * run with R = work.R x actorR, TR = work.R x pos + work.t.
             */
            gte_SetRotMatrix(&modelMatrix);
            gte_SetTransMatrix(&modelMatrix);

#ifdef XENO_PC_PORT
            /* Opt-out scaffolding: without the FieldLoad model build the
             * double-buffer slots read below are 0, so skip the draw (the
             * pre-model-build port behavior) instead of writing at NULL. */
            if (!PcPortModelBuildEnabled()) {
                continue;
            }
#endif
            {
                u8* renderContext = (u8*)g_FieldCurRenderContext;
                s32 renderIndex = g_FieldCurRenderContextIndex;
                void* modelPacket = (void*)(uintptr_t)*(u32*)(modelData + 0x04);
                void* modelWork = (void*)(uintptr_t)*(u32*)(modelData + 0x08 + renderIndex * 4);
                void* ot = renderContext + ((status & 0x8000) ? 0x40D0 : 0xCC);
                func_8002C700(modelPacket, modelWork, ot, *(s16*)(modelData + 0x12));
#ifdef XENO_PC_PORT
                modelEmitted++;
#endif
            }
        }
    }

#ifdef XENO_PC_PORT
    if (XenoFieldDiagEnabled() && s_modelDiagFrames < 8) {
        ActorData* actor3 = NULL;
        printf("[field-diag] models frame=%d considered=%d emitted=%d hidden=%d status20=%d objects=%d otEmits=%d\n",
               (int)s_modelDiagFrames, (int)modelConsidered, (int)modelEmitted,
               (int)modelHidden, (int)modelStatus20, (int)D_800B2264,
               (int)D_80059578);
        printf("[field-diag] camera eye=(%d,%d,%d) at=(%d,%d,%d)\n",
               (int)(g_CameraEye.vx >> 16), (int)(g_CameraEye.vy >> 16),
               (int)(g_CameraEye.vz >> 16),
               (int)(g_CameraAt.vx >> 16), (int)(g_CameraAt.vy >> 16),
               (int)(g_CameraAt.vz >> 16));
        if (g_FieldNumActors > 3) {
            actor3 = (ActorData*)(uintptr_t)g_FieldActors[3].pActorData;
            if (actor3 != NULL) {
                printf("[field-diag] actor3 ip=%u flags4=%08x status=%04x pos=(%d,%d,%d)\n",
                       (unsigned)actor3->scriptInstructionPointer,
                       (unsigned)actor3->flags,
                       (unsigned)(u16)g_FieldActors[3].status,
                       (int)*(s16*)((u8*)actor3 + 0x22),
                       (int)*(s16*)((u8*)actor3 + 0x26),
                       (int)*(s16*)((u8*)actor3 + 0x2A));
            }
        }
        s_modelDiagFrames++;
    }
#endif

    if (g_FieldSystemMode == 0) {
        func_80281B00(&D_8006FB10);
    }
}
#endif /* XENO_PC_PORT */

extern s32 D_8004F380;
extern s32 D_800B2264;
extern u8 D_800B225C;
extern u8 D_800B225D;
extern u8 D_800B225E;
extern u8 D_800B221C;
extern u8 D_8006FB1C;
extern void func_80086BA8(void);
/* SetBackColor prototype comes from psyq/libgte.h (already included above); a
 * local extern here conflicted with it (int vs the real long params). */
extern void func_801E7D14(void* sceneData, void* arg1, void* prim, s32 renderContextIndex, s32 arg4);

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/field/nonmatchings/main/misc2", func_8007520C);
#else
void func_8007520C(void) {
    if (D_8004F380 != 0) {
        return;
    }

    if (D_800B2264 != 0) {
        func_80086BA8();
        SetBackColor(D_800B225C, D_800B225D, D_800B225E);
        func_801E7D14((u8*)&g_Scene + 0xD4,
                      &D_800B221C,
                      (u8*)g_FieldCurRenderContext + 0xCC,
                      g_FieldCurRenderContextIndex,
                      1);
    }

    if (g_FieldSystemMode == 0) {
        func_80281B00(&D_8006FB1C);
    }
}
#endif /* XENO_PC_PORT */

extern u8 D_800ADB05;
extern void func_800250E0(s32 context);
extern void GfxSetCurrentOT(u_long* ot);
extern void func_80024FF4(MATRIX* matrix);
extern void func_8001D468(void);
extern void WorkListUpdate(void);
extern void TimerWorkListUpdate(void);
extern void func_80075B44(void* ot, s32 renderContextIndex);
extern void AnimScriptTick(void* pSpriteData);
extern void func_800764B4(void* ot, s32 renderContextIndex);
extern u8 D_8006FB28;

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/field/nonmatchings/main/misc2", func_800752C8);
#else
void func_800752C8(void) {
    s32 i;
    u8* pActor;

    if (D_800ADB05 == 1) {
        return;
    }

    func_800250E0(g_FieldCurRenderContextIndex);
    GfxSetCurrentOT((u_long*)((u8*)g_FieldCurRenderContext + 0xCC));
    func_80024FF4((MATRIX*)((u8*)&g_Scene + 0xD4));
    func_8001D468();
    WorkListUpdate();
    TimerWorkListUpdate();
    func_80075B44((u8*)g_FieldCurRenderContext + 0xCC, g_FieldCurRenderContextIndex);

    pActor = (u8*)g_FieldActors;
    for (i = 0; i < D_800ADBFC; i++, pActor += 0x5C) {
        u32 status = *(u32*)(pActor + 0x58);
        u8* pActorData = (u8*)(uintptr_t)*(u32*)(pActor + 0x4C);
        s32 shouldTick = 0;

        if ((status & 0x60) == 0x40) {
            u32 flags = *(u32*)(pActorData + 0x4);
            if ((flags & 0x600) != 0x200 && (flags & 0x1000) == 0 &&
                (*(u32*)pActorData & 0x1) == 0) {
                shouldTick = 1;
            }
        } else if (*(u32*)(pActorData + 0x4) & 0x1000000) {
            shouldTick = 1;
        }

        if (shouldTick) {
            AnimScriptTick((void*)(uintptr_t)*(u32*)(pActor + 0x4));
#ifdef XENO_PC_PORT
            if (XenoWalkFieldDumpFrame() < 60) {
                u8* sprite = (u8*)(uintptr_t)*(u32*)(pActor + 0x4);

                if (sprite != NULL) {
                    u8* scriptPc = (u8*)(uintptr_t)*(u32*)(sprite + 0x64);
                    s32 moving = *(s32*)(pActorData + 0x40) |
                                 *(s32*)(pActorData + 0x44) |
                                 *(s32*)(pActorData + 0x48);
                    s16 curAnim = *(s16*)(pActorData + 0xE8);

                    if (moving != 0 || curAnim == 1 || curAnim == 2) {
                        printf("[walk-anim] field f=%d actor=%d pos=%d,%d "
                               "anim=%d sprAnim=%d pose=%d wait=%d op=0x%02x\n",
                               XenoWalkFieldDumpFrame(), i,
                               (int)*(s16*)(pActorData + 0x22),
                               (int)*(s16*)(pActorData + 0x2A),
                               (int)curAnim, (int)*(s8*)(sprite + 0xAF),
                               (int)*(s16*)(sprite + 0x34),
                               (int)*(s16*)(sprite + 0x9E),
                               scriptPc != NULL ? (unsigned)scriptPc[0] : 0u);
                    }
                }
            }
#endif
        }
    }
#ifdef XENO_PC_PORT
    XenoWalkFieldDumpAdvance();
#endif

    func_800764B4((u8*)g_FieldCurRenderContext + 0xCC, g_FieldCurRenderContextIndex);

    if (g_FieldSystemMode == 0) {
        func_80281B00(&D_8006FB28);
    }
}
#endif /* XENO_PC_PORT */

void FieldAddPrimitives(u_long* ot, u_long* pPrimList, int size) {
#ifdef XENO_PC_PORT
    /* Diagnostic-only counters/statics: on the matching build these have no
     * output-section mapping (the retail binary never had them), so any
     * .sbss/.scommon storage for them here is discarded at link time. Confine
     * the whole diagnostic apparatus to the port build. */
    extern s32 g_FieldDiagSubmittedThisFrame;
    static s32 s_loggedSubmits;
    u32 tag = *(u32*)pPrimList;

    g_FieldDiagSubmittedThisFrame++;
    if (XenoFieldDiagEnabled() && s_loggedSubmits < 8) {
        printf("[field-diag] submit[%d] srcOff=0x%lx size=%d tag=%08x w1=%08x w2=%08x w3=%08x\n",
               (int)s_loggedSubmits,
               (unsigned long)((uintptr_t)pPrimList - (uintptr_t)g_FieldCurRenderContext),
               (int)size,
               (unsigned int)tag,
               (unsigned int)((u32*)pPrimList)[1],
               (unsigned int)((u32*)pPrimList)[2],
               (unsigned int)((u32*)pPrimList)[3]);
        s_loggedSubmits++;
    }
#endif
    AddPrims(ot, (u8*)pPrimList + size * 4, pPrimList);
}

extern s16 D_800B00B2;
extern s32 D_800ADB50;
extern s32 D_800B007C;
extern s16 D_800B21D4;
extern s32 func_800273C4(void* ctx, SVECTOR* eye, SVECTOR* at, MATRIX* mtx,
                         void* ot, s32 renderCtxIndex);

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/field/nonmatchings/main/misc2", func_80075484);
#else
void func_80075484(void) {
    if (D_800B00B2 != 0 && D_800ADB50 == 0) {
        SVECTOR eye;
        SVECTOR at;
        void* prim;

        eye.vx = (s16)(g_CameraEye.vx >> 16);
        eye.vy = (s16)(g_CameraEye.vy >> 16);
        eye.vz = (s16)(g_CameraEye.vz >> 16);
        at.vx = (s16)(g_CameraAt.vx >> 16);
        at.vy = (s16)(g_CameraAt.vy >> 16);
        at.vz = (s16)(g_CameraAt.vz >> 16);

        /* asm: a3 = g_CameraEye + 0x1E4 == &g_Scene.worldToScreenMatrix */
        prim = (u8*)g_FieldCurRenderContext + D_800B21D4 * 4 + 0x40CC;
        func_800273C4((void*)D_800B007C, &eye, &at, &g_Scene.worldToScreenMatrix,
                      prim, g_FieldCurRenderContextIndex);
    }
}
#endif /* XENO_PC_PORT */

/* ---- func_8007554C: per-frame field render pipeline ------------------------
 * Port-first functional decompile (control flow mirrors the asm). This is the
 * main render function called every frame from FieldMain's loop (via
 * func_80078D44's fade loops and the per-frame dispatch). It:
 *   - Vsync for frame timing,
 *   - updates field/actor/particle state,
 *   - renders fade/background/particles/distortion/actors,
 *   - swaps render contexts, clears the framebuffer, uploads DrawEnv/DispEnv,
 *   - adds primitives to the OT and calls DrawOTag to present the frame.
 * Many callees are still stubs in the port; the key path is DrawOTag at the end. */
extern s32 D_800ADB9C, D_800ADBA0, D_800ADC18, D_800ADBB4;
extern s32 g_FieldRenderContextUseOT2;
extern s16 D_800B21D4;
extern u8 D_800B219C, D_800B219D, D_800B219E;
extern s32 D_800B0048;
extern s32 D_800B217C;
extern u32 D_800AF87C;
extern u8 D_800AFC58[];
extern void func_800739C0(void);
extern void func_80086908(void);
extern void func_80281B00(void*);
extern void FieldFadeUpdateAndDraw(void* arg0, int arg1);
extern void func_80074108(void);
extern void func_800748E8(void);
extern void func_800752C8(void);
extern void func_8007520C(void);
extern void func_80075484(void);
extern void func_80281450(void);
extern void func_80281400(void);
extern void func_800A84C0(void);
extern void func_800ABEC8(void);
extern void func_800805F4(void);
extern void func_8008004C(void* arg0, int arg1);
extern void func_80025044(void);
extern void func_800920D8(void);
extern char D_8006FB34, D_8006FB40, D_8006FB4C, D_8006FB58, D_8006FB64;
extern void HeapTickDelayedFree(void);
#ifdef XENO_PC_PORT
/* Diagnostic-only globals; see the matching-build note in FieldAddPrimitives. */
s32 g_FieldDiagSubmittedThisFrame;
static s32 g_FieldDiagFrameCount;
#endif

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/field/nonmatchings/main/misc2", func_8007554C);
#else
void func_8007554C(void) {
    s32 s1;
    s32 s0 = 0x80D4; /* offset into RenderContext for fade draw args */
#ifdef XENO_PC_PORT
    s32 diagFrame = g_FieldDiagFrameCount++;

    PcPort_TestInputAdvanceFrame();
    g_FieldDiagSubmittedThisFrame = 0;
#endif
    D_800ADB9C = Vsync(1);
    s1 = Vsync(-1);
    func_800739C0();
    func_80086908();

    if (g_FieldSystemMode == 0) {
        func_80281B00(&D_8006FB34);
    }

    /* Fade update + draw (args: renderContext + 0x80D4, renderContextIndex) */
    FieldFadeUpdateAndDraw((u8*)g_FieldCurRenderContext + s0,
                           g_FieldCurRenderContextIndex);

    if (g_FieldSystemMode == 0) {
        func_80281B00(&D_8006FB40);
    }

    func_80074108();

    /* Scratchpad stack save (0x1F8003FC area — PSX-specific; harmless on host) */
    {
        /* The asm stores $sp to the scratchpad top and allocates 4 bytes.
         * On the host this is a no-op (no scratchpad); skip it. */
    }

    func_800748E8();
    func_800752C8();
    FieldParticlesTickAndRender();

    if (g_FieldSystemMode == 0) {
        func_80281450();
    }

    FieldDistortionDraw();

    /* Restore scratchpad stack (no-op on host) */
    func_800A84C0();
    func_80075484();
    func_8007520C();
    func_800ABEC8();

    if (g_FieldSystemMode == 0) {
        func_80281400();
        func_80281B00(&D_8006FB4C);
    }

    D_800ADBA0 = Vsync(1);
    DrawSync(0);
    func_800805F4();

    /* Render context swap */
    func_8008004C((u8*)g_FieldCurRenderContext + s0,
                  g_FieldCurRenderContextIndex);

    Vsync(0);
    HeapTickDelayedFree();

    /* Framebuffer clear / display setup */
    if (D_800ADC18 == 0) {
        /* Normal path: clear the framebuffer area */
        ClearImage(&g_FieldCurRenderContext->drawEnvs[0].clip, 0, 0, 0);
    } else if (D_800B0048 == D_800ADC18) {
        /* MoveImage path: scroll the framebuffer */
        RECT rect;
        rect.x = 0x2C0; rect.y = 0x100; rect.w = 0x140; rect.h = 0xE0;
        MoveImage(&rect, 0, g_FieldCurRenderContextIndex << 8);
    } else {
        ClearImage(&g_FieldCurRenderContext->drawEnvs[0].clip, 0, 0, 0);
    }

    PutDispEnv(&g_FieldCurRenderContext->dispEnv);
    PutDrawEnv(&g_FieldCurRenderContext->drawEnvs[0]);

    if (g_FieldSystemMode == 0) {
        D_800ADB9C = Vsync(1);
    }

    func_80025044();

    if (g_FieldSystemMode == 0) {
        func_80281B00(&D_8006FB58);
    }

    func_800920D8();

    /* Conditional VRAM upload */
    if (D_800ADBB4 != 0) {
        LoadImage((RECT*)D_800AFC58, (void*)(uintptr_t)D_800AF87C);
        D_800ADBB4 = 0;
    }

    if (g_FieldSystemMode == 0) {
        func_80281B00(&D_8006FB64);
    }

    /* Add primitives to OT and draw. All offsets are BYTE offsets (MIPS addu). */
    {
        u8* ctx = (u8*)g_FieldCurRenderContext;

        if (D_800ADC18 == 0) {
            if (g_FieldRenderContextUseOT2) {
                FieldAddPrimitives((u_long*)(ctx + D_800B21D4 * 4 + 0xCC),
                                   (u_long*)(ctx + 0x40D0), D_800B21D4);
            }
            FieldAddPrimitives((u_long*)(ctx + 0x80F0),
                               (u_long*)(ctx + 0xCC), D_800B21D4);
        }
    }

    /* The actual draw call (byte offset 0x80F0 into RenderContext = ot1) */
#ifdef XENO_PC_PORT
    if (XenoFieldDiagEnabled() && diagFrame < 8) {
        printf("[field-diag] frame=%d D_800ADC18=%d useOT2=%d primSubmits=%d DrawOTag=1\n",
               (int)diagFrame, (int)D_800ADC18, (int)g_FieldRenderContextUseOT2,
               (int)g_FieldDiagSubmittedThisFrame);
    }
    /* DIAGNOSTIC (remove with the title-backdrop probes): histogram the
     * packet codes reachable from the OT DrawOTag is about to walk, so a
     * submitted-but-invisible layer (Map 490 horizon FT4s) can be told apart
     * from one that never reached the list. Host prims sit below 16 MiB, so
     * the 24-bit links are direct pointers. */
    if (XenoFieldDiagEnabled() && diagFrame >= 4 && diagFrame < 7) {
        u32* head = (u32*)((u8*)g_FieldCurRenderContext + 0x80F0);
        u32 link = *head;
        int hist[256] = {0};
        int total = 0, steps = 0, ft4s = 0;
        char buf[512];
        int n = 0, c;
        while ((link & 0xFFFFFF) != 0xFFFFFF && steps < 200000) {
            u32* pkt = (u32*)(uintptr_t)(link & 0xFFFFFF);
            u32 len = link >> 24;
            if (pkt == NULL) break;
            if (len != 0) {
                u8 code = ((u8*)pkt)[7];
                hist[code]++;
                total++;
                if ((code & 0xFC) == 0x2C && ft4s < 3) {
                    s16* v = (s16*)pkt;
                    printf("[field-diag] ot-ft4 #%d rgb=%02x%02x%02x xy0=(%d,%d) xy3=(%d,%d) "
                           "uv0=%02x,%02x clut=%04x tpage=%04x\n", ft4s,
                           ((u8*)pkt)[4], ((u8*)pkt)[5], ((u8*)pkt)[6],
                           v[4], v[5], v[16], v[17],
                           ((u8*)pkt)[12], ((u8*)pkt)[13],
                           *(u16*)((u8*)pkt + 14), *(u16*)((u8*)pkt + 22));
                    ft4s++;
                }
            }
            link = *pkt;
            steps++;
        }
        for (c = 0; c < 256; c++) {
            if (hist[c] && n < 440) {
                n += snprintf(buf + n, sizeof(buf) - n, " %02x:%d", c, hist[c]);
            }
        }
        printf("[field-diag] ot-walk frame=%d steps=%d prims=%d codes:%s\n",
               (int)diagFrame, steps, total, buf);
    }
#endif
    DrawOTag((u_long*)((u8*)g_FieldCurRenderContext + 0x80F0));

    /* Frame timing wait loop */
    {
        s32 target = s1 + D_800B217C + 2;
        while (Vsync(-1) < target) {
        }
    }
}
#endif /* XENO_PC_PORT */

extern s32 D_800ADB78;
extern void func_800A2030(void);
extern void func_800805F4(void);
extern void func_8008004C(void* pCtx, s32 idx);

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/field/nonmatchings/main/misc2", func_80075910);
#else
void func_80075910(void) {
    RECT rect;
    FieldPollControllers();
    func_800A2030();
    func_800805F4();
    func_8008004C((u8*)g_FieldCurRenderContext + 0x80D4, g_FieldCurRenderContextIndex);
    DrawSync(0);
    Vsync(0);
    rect.w = 0x140;
    rect.h = 0xE0;
    rect.x = 0;
    rect.y = (D_800ADB78 + 1 & 1) << 8;
    MoveImage(&rect, g_FieldCurRenderContextIndex << 8, 0);
    PutDispEnv((DISPENV*)((u8*)g_FieldCurRenderContext + 0xB8));
    PutDrawEnv((DRAWENV*)g_FieldCurRenderContext);
    DrawOTag(g_FieldCurRenderContext->ot3 + 7);
}
#endif /* XENO_PC_PORT */

/* Retail 800759E4..80075B08: construct three matrix rows from the given
 * axis and two normalized cross products. Translation/padding are retained. */
#ifdef XENO_PC_PORT
/* Field rodata 8006FB70 (raw 0x80), read as one 16-byte VECTOR. */
const VECTOR D_8006FB70 = { 0, 0, 0x1000, 0 };
#else
extern VECTOR D_8006FB70;
#endif

void func_800759E4(MATRIX *matrix,VECTOR *axis) {
    VECTOR up = D_8006FB70;
    VECTOR right;
    VECTOR temporary;
    OuterProduct12(&up,axis,&temporary);
    VectorNormal(&temporary,&right);
    OuterProduct12(&right,axis,&temporary);
    VectorNormal(&temporary,&up);
    matrix->m[0][0] = right.vx;
    matrix->m[0][1] = right.vy;
    matrix->m[0][2] = right.vz;
    matrix->m[1][0] = axis->vx;
    matrix->m[1][1] = axis->vy;
    matrix->m[1][2] = axis->vz;
    matrix->m[2][0] = up.vx;
    matrix->m[2][1] = up.vy;
    matrix->m[2][2] = up.vz;
}

void func_80075B08(void* sprite, u8* color) {
    if (D_800B218E == 0) {
        SpriteSetColor(sprite, color[0], color[1], color[2]);
    }
}

extern s32 D_8004F37C;
extern s32 D_80050100;
extern s32 D_800B2268;
extern u8 D_800B2357;
extern void func_8001E298(void* pSpriteData, void* ot);
/* Model base color bytes (set by func_8002C6E0 in the fog prologue); retail's
 * sprite-fog branch lwc2's the word they form into GTE RGBC. Needed by both
 * builds. */
extern u8 D_80059598;
extern u8 D_80059599;
extern u8 D_8005959A;
#ifdef XENO_PC_PORT
extern u32 D_801E8670[];
extern s32 D_800B220C[];

/* Retail 80076300-80076464 updates the object overlay instead of drawing
 * a substitute sprite. Slots advance only after this branch processes an
 * object; ordinary actors and the global skip do not consume slots. */
static u32 FieldUpdateObjectActor(u8* actor, u32 status, u32 slot) {
    u8* object;
    u8* node;
    u16 flags;
    u32 product;
    if (D_8004F380 != 0) return slot;
    object = (u8*)(uintptr_t)D_801E8670[slot];
    flags = *(u16*)(object + 0x4A);
    if ((*(u32*)actor & 0x10000) ||
        (*(u32*)(actor + 0x14) & 0x200002) ||
        (*(u32*)(actor + 4) & 0x800)) flags |= 1;
    else flags &= 0xFFFE;
    *(u16*)(object + 0x4A) = flags;
    object[0x34] = (status & 0x20) ? 0 : 1;
    node = (u8*)(uintptr_t)*(u32*)(object + 4);
    if (*(u32*)(actor + 4) & 0x20000) {
        u16 angle = *(u16*)(node + 0x56) - 0xC00;
        *(u16*)(actor + 0x108) = angle;
        *(u16*)(actor + 0x106) = angle;
    } else {
        *(u16*)(node + 0x56) = *(u16*)(actor + 0x108) + 0xC00;
    }
    /* MULT/MFLO followed by SRA: wrap before the signed shift. The scale
     * table is retail 0x800B220C (D_800B220C, written by func_80077AB4 /
     * func_800A0FD8 from obj+0x1C). It is addressed through its own alias:
     * the packed block g_FieldBss_800B20A8 starts at retail 0x800B2078
     * (g_FieldEffects), so "+0x164" from it is 0x800B21DC, the spriteId<<1
     * table -- which scaled every MAP16 object by garbage. */
    product = (u32)(s32)*(s16*)(actor + 0xF4) * (u32)D_800B220C[slot];
    *(s16*)(object + 0x1C) = (s32)product >> 12;
    *(s16*)(object + 0x60) = *(s16*)(actor + 0x26);
    *(s32*)(node + 0x5C) = *(s16*)(actor + 0x22);
    *(s32*)(node + 0x64) = *(s16*)(actor + 0x2A);
    *(u32*)(actor + 4) &= ~0x200u;
    return slot + 1;
}

extern void func_8001E2F8(void* sprite, void* ot, s16 splitY);
extern void func_8001E368(void* sprite, void* ot, s16 splitY);

/* Retail 80076118-800762FC, after the common transform and OT adjustment.
 * Return values classify diagnostic counters only: hidden/plain/special. */
static int FieldRenderActorSpriteTail(void* sprite, u8* actor, void* ot,
                                      s32 otIndex, s32 sceneDip) {
    u8* data = sprite;
    SVECTOR center;
    int xy;
    long p, flag;
    s32 depth;
    u32 flags;
    u32 base = (u32)(uintptr_t)ot;
    if ((u16)(*(u16*)(actor + 0xE8) + 0x22) < 2) {
        if (*(u32*)(actor + 4) & 0x02000000) return 0;
        SpriteSetColor(sprite, actor[0xFC], actor[0xFD], actor[0xFE]);
        data[0x3D] = 0xEF;
        func_8001E298(sprite, (void*)(uintptr_t)(base + ((u32)otIndex << 2) - 0x40));
        center.vx = 0; center.vy = 300; center.vz = 0;
        depth = RotTransPers(&center, &xy, &p, &flag) >> (D_80050100 & 31);
        SpriteSetColor(sprite, actor[0xFF], actor[0x100], actor[0x101]);
        data[0x3D] = 0xF7;
        func_8001E298(sprite, (void*)(uintptr_t)(base + ((u32)depth << 2)));
        return 2;
    }
    data[0x3D] = 0;
    if (*(u32*)(actor + 4) & 0x02000000) return 0;
    flags = *(u32*)(actor + 0x134);
    if ((flags & 0x60) == 0) {
        func_80075B08(sprite, actor + 0xFC);
        func_8001E298(sprite, (void*)(uintptr_t)(base + ((u32)otIndex << 2)));
        return 1;
    }
    if (flags & 0x20) {
        func_80075B08(sprite, actor + 0xFC);
        center.vx = 0;
        center.vy = (u32)(*(s16*)(actor + 0xEE) - sceneDip / 3) << 1;
        center.vz = 0;
        depth = RotTransPers(&center, &xy, &p, &flag) >> (D_80050100 & 31);
        if (depth >= 2) depth -= 2;
        func_8001E2F8(sprite, (void*)(uintptr_t)(base + ((u32)depth << 2)),
                      *(s16*)(actor + 0xEE));
    }
    /* Retail re-reads flags after the first renderer; both bits may run. */
    if (*(u32*)(actor + 0x134) & 0x40) {
        func_80075B08(sprite, actor + 0xFF);
        func_8001E368(sprite, (void*)(uintptr_t)(base + ((u32)otIndex << 2)),
                      *(s16*)(actor + 0xEE));
    }
    return 2;
}
#endif

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/field/nonmatchings/main/misc2", func_80075B44);
#else
void func_80075B44(void* ot, s32 renderContextIndex) {
    MATRIX baseSpriteMatrix;
    s32 cameraDirection = FieldGetCameraDirection() & 0xFFFF;
    s32 sceneDip = *(s16*)((u8*)&g_Scene + 0x6C);
    s32 centerYOffset = -(((sceneDip / 3) << 1));
    s32 actorIndex;
#ifdef XENO_PC_PORT
    static s32 s_diagFrames;
    s32 diagActive = 0;
    s32 diagStatus20 = 0;
    s32 diagFlagNeg = 0;
    s32 diagGlobalSkip = 0;
    s32 diagActorFlagSkip = 0;
    s32 diagPlain = 0;
    s32 diagSpecial = 0;
    s32 diagObjectSkip = 0;
    u32 objectSlot = 0;
#endif

    (void)renderContextIndex;

    FieldMatrixCopyTransform(&baseSpriteMatrix, (MATRIX*)((u8*)&g_Scene + 0x20));

    for (actorIndex = 0; actorIndex < D_800ADBFC; actorIndex++) {
        u8* pActor = (u8*)g_FieldActors + actorIndex * 0x5C;
        u32 status = *(u32*)(pActor + 0x58);
        u8* pActorData;
        u8* pSpriteData;
        u8* pSpriteBase;
        MATRIX actorMatrix;
        VECTOR actorTranslation;
        SVECTOR center;
        long screenXY;
        long p;
        long flag;
        s32 otIndex;
        u32 actorFlags4;
        VECTOR scale;

        if ((status & 0x40) == 0) {
            continue;
        }
#ifdef XENO_PC_PORT
        diagActive++;
#endif

        pActorData = (u8*)(uintptr_t)*(u32*)(pActor + 0x4C);
        pSpriteData = (u8*)(uintptr_t)*(u32*)(pActor + 0x4);

        *(u32*)(pActor + 0x2C) = *(u32*)(pActor + 0x0C);
        *(u32*)(pActor + 0x30) = *(u32*)(pActor + 0x10);
        *(u32*)(pActor + 0x34) = *(u32*)(pActor + 0x14);
        *(u32*)(pActor + 0x38) = *(u32*)(pActor + 0x18);
        *(u32*)(pActor + 0x3C) = *(u32*)(pActor + 0x1C);
        *(u32*)(pActor + 0x40) = *(u32*)(pActor + 0x20);
        *(u32*)(pActor + 0x44) = *(u32*)(pActor + 0x24);
        *(u32*)(pActor + 0x48) = *(u32*)(pActor + 0x28);

        actorFlags4 = *(u32*)(pActorData + 0x04);
        if (actorFlags4 & 0x2000) {
#ifdef XENO_PC_PORT
            objectSlot = FieldUpdateObjectActor(pActorData, status, objectSlot);
            diagObjectSkip++;
#endif
            continue;
        }

        {
#ifdef XENO_PC_PORT
            SVECTOR column;
            SVECTOR transformed;

            /* Retail 80075CB4-80075D94 gathers the three actor-matrix
             * columns at halfword offsets 0/6/12, 2/8/14, and 4/10/16,
             * transforms each column, then stores the result as a column. */
            column.vx = *(s16*)(pActor + 0x0C);
            column.vy = *(s16*)(pActor + 0x12);
            column.vz = *(s16*)(pActor + 0x18);
            ApplyMatrixSV(&g_Scene.worldToScreenMatrix, &column, &transformed);
            actorMatrix.m[0][0] = transformed.vx;
            actorMatrix.m[1][0] = transformed.vy;
            actorMatrix.m[2][0] = transformed.vz;

            column.vx = *(s16*)(pActor + 0x0E);
            column.vy = *(s16*)(pActor + 0x14);
            column.vz = *(s16*)(pActor + 0x1A);
            ApplyMatrixSV(&g_Scene.worldToScreenMatrix, &column, &transformed);
            actorMatrix.m[0][1] = transformed.vx;
            actorMatrix.m[1][1] = transformed.vy;
            actorMatrix.m[2][1] = transformed.vz;

            column.vx = *(s16*)(pActor + 0x10);
            column.vy = *(s16*)(pActor + 0x16);
            column.vz = *(s16*)(pActor + 0x1C);
            ApplyMatrixSV(&g_Scene.worldToScreenMatrix, &column, &transformed);
            actorMatrix.m[0][2] = transformed.vx;
            actorMatrix.m[1][2] = transformed.vy;
            actorMatrix.m[2][2] = transformed.vz;
#else
            /* Retail 80075CB4-80075D94 gathers the three actor-matrix COLUMNS
             * (halfwords 0/6/0xC relative to pActor+0xC, +0xE, +0x10),
             * transforms each with the world->screen rotation and stores the
             * result back as a column. The previous row-wise form here read
             * contiguous halfwords and produced a different matrix. */
            SVECTOR column;
            SVECTOR transformed;

            column.vx = *(s16*)(pActor + 0x0C);
            column.vy = *(s16*)(pActor + 0x12);
            column.vz = *(s16*)(pActor + 0x18);
            gte_ApplyMatrixSV(&g_Scene.worldToScreenMatrix, &column, &transformed);
            actorMatrix.m[0][0] = transformed.vx;
            actorMatrix.m[1][0] = transformed.vy;
            actorMatrix.m[2][0] = transformed.vz;

            column.vx = *(s16*)(pActor + 0x0E);
            column.vy = *(s16*)(pActor + 0x14);
            column.vz = *(s16*)(pActor + 0x1A);
            gte_ApplyMatrixSV(&g_Scene.worldToScreenMatrix, &column, &transformed);
            actorMatrix.m[0][1] = transformed.vx;
            actorMatrix.m[1][1] = transformed.vy;
            actorMatrix.m[2][1] = transformed.vz;

            column.vx = *(s16*)(pActor + 0x10);
            column.vy = *(s16*)(pActor + 0x16);
            column.vz = *(s16*)(pActor + 0x1C);
            gte_ApplyMatrixSV(&g_Scene.worldToScreenMatrix, &column, &transformed);
            actorMatrix.m[0][2] = transformed.vx;
            actorMatrix.m[1][2] = transformed.vy;
            actorMatrix.m[2][2] = transformed.vz;
#endif
        }

        {
            VECTOR actorPosition;

            actorPosition.vx = *(s16*)(pActor + 0x20);
            actorPosition.vy = *(s16*)(pActor + 0x24);
            actorPosition.vz = *(s16*)(pActor + 0x28);
#ifdef XENO_PC_PORT
            ApplyMatrixLV(&g_Scene.worldToScreenMatrix, &actorPosition, &actorTranslation);
            actorTranslation.vx += g_Scene.worldToScreenMatrix.t[0];
            actorTranslation.vy += g_Scene.worldToScreenMatrix.t[1];
            actorTranslation.vz += g_Scene.worldToScreenMatrix.t[2];
#else
            /* Retail 80075DE4 inlines ApplyMatrixLV: ctc2 TR, then
             * lhu/or/mtc2 $0 + lwc2 $1 (== gte_ldlv0), MVMVA 1,0,0,0,0
             * (cv=TR, so the translation IS included), then
             * swc2 $25/$26/$27 (== gte_stlvnl). */
            gte_SetTransMatrix(&g_Scene.worldToScreenMatrix);
            gte_ldlv0(&actorPosition);
            gte_rt();
            gte_stlvnl(&actorTranslation);
#endif
        }

        actorMatrix.t[0] = actorTranslation.vx;
        actorMatrix.t[1] = actorTranslation.vy;
        actorMatrix.t[2] = actorTranslation.vz;
        gte_SetRotMatrix(&actorMatrix);
        gte_SetTransMatrix(&actorMatrix);

        center.vx = 0;
        center.vy = centerYOffset;
        center.vz = 0;
        {
            /* Retail 80075E38-80075E70 inlines RotTransPers here (lwc2 $0/$1
             * from sp+0x18, RTPS, then swc2 for SXY2/DP/flags/OTZ) instead of
             * calling it - the surrounding sites stay as calls. */
            long otz;

            gte_RotTransPers(&center, &screenXY, &p, &flag, &otz);
            otIndex = otz >> D_80050100;
        }

        if ((((s16)(screenXY >> 16) + 9) >= 0x143) ||
            (((s16)screenXY + 0x27) >= 0x18F)) {
            *(u32*)(pActorData + 0x04) |= 0x200;
        } else {
            *(u32*)(pActorData + 0x04) &= ~0x200;
        }

        if (D_8004F37C != 0 || (status & 0x20) || flag < 0) {
#ifdef XENO_PC_PORT
            if (D_8004F37C != 0) {
                diagGlobalSkip++;
            }
            if (status & 0x20) {
                diagStatus20++;
            }
            if (flag < 0) {
                diagFlagNeg++;
            }
#endif
            continue;
        }

        scale.vx = (*(s16*)(pActorData + 0xF4) * 3) >> 2;
        scale.vy = (*(s16*)(pActorData + 0xF6) * 3) >> 2;
        scale.vz = (*(s16*)(pActorData + 0xF8) * 3) >> 2;
        if ((*(s16*)(pActorData + 0xE4) == 7) && (D_800B2268 != 0)) {
            scale.vx = (scale.vx * 5) >> 2;
            scale.vy = (scale.vy * 5) >> 2;
            scale.vz = (scale.vz * 5) >> 2;
        }

        pSpriteBase = (u8*)(uintptr_t)*(u32*)(pSpriteData + 0x20);
        /* Retail 80075F98 inlines this rotation-only copy here (only the
         * baseSpriteMatrix init at 80075B94 remains a call), so write the nine
         * halfword moves out explicitly rather than calling the helper. */
        {
            MATRIX* pDst = (MATRIX*)(pSpriteBase + 0x0C);

            pDst->m[0][0] = baseSpriteMatrix.m[0][0];
            pDst->m[0][1] = baseSpriteMatrix.m[0][1];
            pDst->m[0][2] = baseSpriteMatrix.m[0][2];
            pDst->m[1][0] = baseSpriteMatrix.m[1][0];
            pDst->m[1][1] = baseSpriteMatrix.m[1][1];
            pDst->m[1][2] = baseSpriteMatrix.m[1][2];
            pDst->m[2][0] = baseSpriteMatrix.m[2][0];
            pDst->m[2][1] = baseSpriteMatrix.m[2][1];
            pDst->m[2][2] = baseSpriteMatrix.m[2][2];
        }
        ScaleMatrix((MATRIX*)(pSpriteBase + 0x0C), &scale);

        if (*(u32*)(pActorData + 0x14) & 0x200000) {
            s32 facingDelta = (cameraDirection - (((*(u32*)(pActorData + 0x14) >> 11) - 2) & 0x7)) & 0x7;

            if (facingDelta != 0) {
                if (facingDelta < 4) {
                    center.vx = 0;
                    center.vy = -0x80;
                    center.vz = 0;
                    otIndex = RotTransPers(&center, &screenXY, &p, &flag) >> D_80050100;
                } else if (facingDelta >= 5 && facingDelta < 8) {
                    center.vx = 0;
                    center.vy = 0x80;
                    center.vz = 0;
                    otIndex = RotTransPers(&center, &screenXY, &p, &flag) >> D_80050100;
                }
            }
        }

        if (D_800B2357 == 0 && D_800B218E != 0) {
#ifdef XENO_PC_PORT
            /* Retail 0x800760AC: fog the actor's sprite.  RGBC <- the model
             * base color word (retail lwc2's the u32 at 0x80059598; the port
             * composes it from the three separate BSS bytes), DPCS depth-cues
             * it toward the far color using IR0 left by this actor's most
             * recent RotTransPers (the center transform above, or the
             * facing-delta re-issue), and the fogged RGB2 tints the sprite.
             * This is the fog-on counterpart of func_80075B08's fog-off
             * SpriteSetColor; the DQA/DQB + far-color state is refreshed each
             * frame by the ported func_800748E8 fog prologue. */
            u32 rgbc = (u32)D_80059598 | ((u32)D_80059599 << 8) |
                       ((u32)D_8005959A << 16);
            u32 fogged;

            gte_ldrgb(&rgbc);
            gte_dpcs();
            fogged = (u32)C2_RGB2;
            SpriteSetColor(pSpriteData, (u8)fogged, (u8)(fogged >> 8),
                           (u8)(fogged >> 16));
#else
            /* Retail 800760AC: lwc2 $6 (RGBC) straight from the model base
             * color word, DPCS depth-cues it against the far color using the
             * IR0 left by this actor's RotTransPers, swc2 $22 (RGB2) into a
             * stack slot, then SpriteSetColor with that slot's three bytes.
             * Retail reloads g_FieldActors[actorIndex] for the sprite pointer. */
            u32 fogged;

            gte_ldrgb((u32*)((u8*)&D_80059598));
            gte_dpcs();
            gte_strgb(&fogged);
            SpriteSetColor((u8*)(uintptr_t)g_FieldActors[actorIndex].pSpriteData,
                           (u8)fogged, (u8)(fogged >> 8), (u8)(fogged >> 16));
#endif
        }

        if (otIndex >= 2) {
            otIndex -= 2;
        }

#ifdef XENO_PC_PORT
        switch (FieldRenderActorSpriteTail(pSpriteData, pActorData, ot, otIndex, sceneDip)) {
        case 0: diagActorFlagSkip++; break;
        case 1:
            diagPlain++;
            if (XenoFieldDiagEnabled() && s_diagFrames < 4) {
                printf("[field-diag] func_80075B44 draw actor=%d sprite=%p otIndex=%d ot=%p status=%08x flags4=%08x flag=%08lx screen=%08lx\n",
                       (int)actorIndex, pSpriteData, (int)otIndex, ot,
                       (unsigned int)status, (unsigned int)actorFlags4,
                       (unsigned long)flag, (unsigned long)screenXY);
            }
            break;
        case 2: diagSpecial++; break;
        }
#else
        /* Retail .L80076118: actor "double render" (the 0xE8 counter wrapping
         * through 0x22). Sets the sprite colour from pActorData+0xFC, links at
         * otIndex-16, re-transforms a (0, 0x12C, 0) centre and links again
         * with the +0xFF colour. Skipped entirely when flags4 has 0x02000000. */
        if (((*(u16*)(pActorData + 0xE8) + 0x22) & 0xFFFF) < 2) {
            if ((actorFlags4 & 0x02000000) == 0) {
                SpriteSetColor(pSpriteData, *(u8*)(pActorData + 0xFC),
                               *(u8*)(pActorData + 0xFD),
                               *(u8*)(pActorData + 0xFE));
                *(u8*)(pSpriteData + 0x3D) = 0xEF;
                func_8001E298(pSpriteData, (u8*)ot + otIndex * 4 - 0x40);
                center.vx = 0;
                center.vy = 0x12C;
                center.vz = 0;
                otIndex = RotTransPers(&center, &screenXY, &p, &flag) >> D_80050100;
                SpriteSetColor(pSpriteData, *(u8*)(pActorData + 0xFF),
                               *(u8*)(pActorData + 0x100),
                               *(u8*)(pActorData + 0x101));
                *(u8*)(pSpriteData + 0x3D) = 0xF7;
                func_8001E298(pSpriteData, (u8*)ot + otIndex * 4);
            }
            continue;
        }

        /* Retail .L800761E0: the ordinary sprite path. */
        *(u8*)(pSpriteData + 0x3D) = 0;
        if (actorFlags4 & 0x02000000) {
            continue;
        }

        if ((*(u32*)(pActorData + 0x134) & 0x60) == 0) {
            func_80075B08(pSpriteData, pActorData + 0xFC);
            func_8001E298(pSpriteData, (u8*)ot + otIndex * 4);
            continue;
        }

        /* Retail .L80076234: rotated variants keyed on (0x134 >> 5) bits 0/1. */
        if ((*(u32*)(pActorData + 0x134) >> 5) & 1) {
            func_80075B08(pSpriteData, pActorData + 0xFC);
            center.vx = 0;
            center.vy = (*(s16*)(pActorData + 0xEE) - sceneDip / 3) * 2;
            center.vz = 0;
            otIndex = RotTransPers(&center, &screenXY, &p, &flag) >> D_80050100;
            if (otIndex >= 2) {
                otIndex -= 2;
            }
            func_8001E2F8(pSpriteData, (u8*)ot + otIndex * 4,
                          *(s16*)(pActorData + 0xEE));
        }

        if (((*(u32*)(pActorData + 0x134) >> 5) & 2) != 0) {
            func_80075B08(pSpriteData, pActorData + 0xFF);
            func_8001E368(pSpriteData, (u8*)ot + otIndex * 4,
                          *(s16*)(pActorData + 0xEE));
            continue;
        }
#endif
    }

#ifdef XENO_PC_PORT
    if (XenoFieldDiagEnabled() && s_diagFrames < 8) {
        printf("[field-diag] func_80075B44 frame=%d active=%d plain=%d status20=%d flagNeg=%d globalSkip=%d actorFlagSkip=%d special=%d objectSkip=%d\n",
               (int)s_diagFrames, (int)diagActive, (int)diagPlain,
               (int)diagStatus20, (int)diagFlagNeg, (int)diagGlobalSkip,
               (int)diagActorFlagSkip, (int)diagSpecial, (int)diagObjectSkip);
    }
    s_diagFrames++;
#endif
}
#endif /* XENO_PC_PORT */

extern s32 D_8004F37C;
extern s32 D_800500F8;
extern s32 D_800500FC;

#ifdef XENO_PC_PORT
static s32 s_ActiveActorSkipCount = 0;

/* Retail submits this quad directly and relies on the PSX GPU's raster clip.
 * PsyCross can still turn a completely off-screen active-actor quad into a
 * visible, stretched FT4 after its draw-environment offset is applied.  Do
 * not duplicate a vertex-in-viewport test here: a quad can legitimately cross
 * an edge with every vertex outside.  Reject only the conservative case where
 * all four vertices are beyond the same raster edge. */
static int FieldActiveQuadOutsideRaster(const u8* pPrim) {
    s32 x0 = *(const s16*)(pPrim + 0x08), y0 = *(const s16*)(pPrim + 0x0A);
    s32 x1 = *(const s16*)(pPrim + 0x10), y1 = *(const s16*)(pPrim + 0x12);
    s32 x2 = *(const s16*)(pPrim + 0x18), y2 = *(const s16*)(pPrim + 0x1A);
    s32 x3 = *(const s16*)(pPrim + 0x20), y3 = *(const s16*)(pPrim + 0x22);
    s32 maxX = D_800500F8;
    s32 maxY = D_800500FC >> 16;

    return (x0 < 0 && x1 < 0 && x2 < 0 && x3 < 0) ||
           (x0 > maxX && x1 > maxX && x2 > maxX && x3 > maxX) ||
           (y0 < 0 && y1 < 0 && y2 < 0 && y3 < 0) ||
           (y0 > maxY && y1 > maxY && y2 > maxY && y3 > maxY);
}
#endif

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/field/nonmatchings/main/misc2", func_800764B4);
#else
void func_800764B4(void* ot, s32 renderContextIndex) {
    s32 actorIndex;
    u8* pActor;

#ifdef XENO_PC_PORT
    (void)s_ActiveActorSkipCount; /* retained fallback for other unmigrated paths */
#endif

    if (D_8004F37C != 0 || D_800ADBFC <= 0) {
        return;
    }

    pActor = (u8*)g_FieldActors;
    for (actorIndex = 0; actorIndex < D_800ADBFC; actorIndex++, pActor += 0x5C) {
        u32 status = *(u32*)(pActor + 0x58);
        u8* pActorData;
        u8* pSpriteData;
        u8* pPacket;
        u32 flags4;
        VECTOR worldZ, up, cross, normVec1, normVec2, pos, trans, scale;
        MATRIX billboard;
        MATRIX actorMatrix;
        s32 j;
        s32 s0off, packetOff;
        long p, flag, otz;
        s32 otIndex;
        u8* pPrim;
        u32* pSlot;

        /* ---- actor filter chain (unchanged, .L8007656C -> .L80076A28) ---- */
        if ((status & 0x60) != 0x40) {
            continue;
        }
        pActorData = (u8*)(uintptr_t)*(u32*)(pActor + 0x4C);
        flags4 = *(u32*)(pActorData + 0x04);
        if ((flags4 & 0x102200) != 0) {
            continue;
        }
        if ((flags4 & 0x800) != 0) {
            continue;
        }
        if ((*(u32*)(pActorData + 0x00) & 0x10000) != 0) {
            continue;
        }
        if ((*(u32*)(pActorData + 0x14) & 0x200002) != 0) {
            continue;
        }

        /* ---- migrated per-actor quad render body (.L800765D8 .. .L80076A28) ---- */
        pSpriteData = (u8*)(uintptr_t)*(u32*)(pActor + 0x04);
        pPacket     = (u8*)(uintptr_t)*(u32*)(pActor + 0x08);

        /* Actor direction vector at pActorData+0x50 (three 32-bit words, GTE
           truncates to s16). Build a billboard basis via two outer products:
           op = (matrix diagonal) x (IR vector) >> 12  (== cross product). */
        up.vx = *(s32*)(pActorData + 0x50);
        up.vy = *(s32*)(pActorData + 0x54);
        up.vz = *(s32*)(pActorData + 0x58);

        worldZ.vx = 0;
        worldZ.vy = 0;
        worldZ.vz = 0x1000;
        gte_OuterProduct12(&worldZ, &up, &cross);      /* op1 @80076620: cross(worldZ, up) */
        VectorNormal(&cross, &normVec1);           /* -> row0 */
        gte_OuterProduct12(&normVec1, &up, &cross);    /* op1 @80076678: cross(normVec1, up) */
        VectorNormal(&cross, &normVec2);           /* -> row2 */

        billboard.m[0][0] = (s16)normVec1.vx;
        billboard.m[0][1] = (s16)normVec1.vy;
        billboard.m[0][2] = (s16)normVec1.vz;
        billboard.m[1][0] = (s16)up.vx;
        billboard.m[1][1] = (s16)up.vy;
        billboard.m[1][2] = (s16)up.vz;
        billboard.m[2][0] = (s16)normVec2.vx;
        billboard.m[2][1] = (s16)normVec2.vy;
        billboard.m[2][2] = (s16)normVec2.vz;

        /* actorMatrix rotation = worldToScreenRot * billboard, built COLUMN by
           COLUMN (three mvmva 1,0,3,3,0 == ApplyMatrixSV per column). */
        for (j = 0; j < 3; j++) {
            SVECTOR col, res;
            col.vx = billboard.m[0][j];
            col.vy = billboard.m[1][j];
            col.vz = billboard.m[2][j];
            gte_ApplyMatrixSV(&g_Scene.worldToScreenMatrix, &col, &res);
            actorMatrix.m[0][j] = res.vx;
            actorMatrix.m[1][j] = res.vy;
            actorMatrix.m[2][j] = res.vz;
        }

        /* actorMatrix.t = worldToScreenRot * pos.  The asm translation step
           (@80076850, mvmva 1,0,0,0,0) adds the GTE TR register, but the port's
           ApplyMatrixLV is rotation-only and the proven-working sibling
           func_80075B44 (misc2.c:1848-1853, identical cv=0 asm @80075DE4) also
           drops the TR add -- so we mirror the sibling exactly for scene
           consistency and do NOT add worldToScreenMatrix.t here. Proven at
           runtime: for pos=[-100,0,100] ApplyMatrixLV yields [0,-340,258] ==
           R*pos>>12 (TR-add would give Z=25052 -> actor pushed off-screen). */
        pos.vx = *(s16*)(pActor + 0x20);
        pos.vy = *(s16*)(pSpriteData + 0x84);
        pos.vz = *(s16*)(pActor + 0x28);
#ifdef XENO_PC_PORT
        ApplyMatrixLV(&g_Scene.worldToScreenMatrix, &pos, &trans);
#else
        /* Retail 80076850 is the same inlined ApplyMatrixLV form as
         * func_80075B44 (MVMVA 1,0,0,0,0, cv=TR included). */
        gte_SetTransMatrix(&g_Scene.worldToScreenMatrix);
        gte_ldlv0(&pos);
        gte_rt();
        gte_stlvnl(&trans);
#endif
        actorMatrix.t[0] = trans.vx;
        actorMatrix.t[1] = trans.vy;
        actorMatrix.t[2] = trans.vz;

        /* Per-actor scale = (dim*3)>>2, quartered for flagged actors.
           Bit-identical to the asm's ((v*3)<<10)>>12 / >>14 (v is s16). */
        scale.vx = (*(s16*)(pActorData + 0xF4) * 3) >> 2;
        scale.vy = (*(s16*)(pActorData + 0xF6) * 3) >> 2;
        scale.vz = (*(s16*)(pActorData + 0xF8) * 3) >> 2;
        if (D_800B2268 != 0 && (*(u32*)(pActorData + 0x00) & 0x400)) {
            scale.vx = (*(s16*)(pActorData + 0xF4) * 3) >> 4;
            scale.vy = (*(s16*)(pActorData + 0xF6) * 3) >> 4;
            scale.vz = (*(s16*)(pActorData + 0xF8) * 3) >> 4;
        }
        ScaleMatrix(&actorMatrix, &scale);

        /* Install the scaled matrix, then project the shared quad and average OTZ. */
        gte_SetRotMatrix(&actorMatrix);
        gte_SetTransMatrix(&actorMatrix);

        s0off     = renderContextIndex * 0x28;
        packetOff = s0off + 0x20;         /* s1 = idx*0x28 + 0x20 */
        pPrim     = pPacket + packetOff;  /* per-render-context packet tag word */

        otz = RotAverage4(
            (SVECTOR*)(pPacket + 0x00),
            (SVECTOR*)(pPacket + 0x08),
            (SVECTOR*)(pPacket + 0x10),
            (SVECTOR*)(pPacket + 0x18),
            (long*)(pPrim + 0x08),
            (long*)(pPrim + 0x10),
            (long*)(pPrim + 0x18),
            (long*)(pPrim + 0x20),
            &p, &flag);

#ifdef XENO_PC_PORT
        if (FieldActiveQuadOutsideRaster(pPrim)) {
            s_ActiveActorSkipCount++;
            continue;
        }
#endif

        /* Splice the packet at the head of ot[otz >> D_80050100], preserving the
           top (len/code) byte of both the packet tag and the OT slot. 24-bit link
           truncation is the port's established OT convention (g_PsxRam maps low). */
        otIndex = (s32)(otz >> D_80050100);
        pSlot   = (u32*)((u8*)ot + otIndex * 4);
        *(u32*)pPrim = (*(u32*)pPrim & 0xFF000000) | (*pSlot & 0x00FFFFFF);
        *pSlot       = (*pSlot & 0xFF000000) | ((u32)(uintptr_t)pPrim & 0x00FFFFFF);
    }
}
#endif /* XENO_PC_PORT */

void func_80076A74(void* pSpriteData) {
    u8* pAnimState = (u8*)(uintptr_t)*(u32*)((u8*)pSpriteData + 0x7C);
    u8* pActorData = (u8*)(uintptr_t)g_FieldActors[*(s16*)(pAnimState + 0x14)].pActorData;

    *(u32*)(pActorData + 0x04) |= 0x10000;
}

extern void HeapChangeCurrentUser(u_int userTag, char** pContentTypes);
extern void func_800230A8(void* pSpriteData);
extern void* func_80024294(void* pAnimPackage, s16 texX, s16 texY, s16 clutX, s16 clutY, s16 arg5, s32 arg6);
extern void* func_80024524(void* pAnimPackage, s16 texX, s16 texY, s16 clutX, s16 clutY, s16 arg5);
extern void func_80023340(void* pSpriteData, s32 arg1);
extern void func_8001F5BC(void* pSpriteData, s32 arg1, s32* outZ, s32* outX, s32* outY);
extern void func_80021C00(void* pSpriteData, u32 arg1);
extern void func_800245D8(void* pSpriteData, s16 animIndex);
extern void func_80021FE0(void* pSpriteData, s16 angle);
extern void func_80021BF8(void* pSpriteData, s32 callback);
extern void AnimScriptTick(void* pSpriteData);
extern void TimerWorkListUpdate(void);
extern s32 g_GamePartySkinsInitialized;
extern u8 D_800B1F78[];
extern s32 D_800AFC74;

static inline void FieldActorSyncSpritePosition(u8* pActor, u8* pSpriteData) {
    u8* pActorData = (u8*)(uintptr_t)*(u32*)(pActor + 0x4C);

    *(u32*)(pSpriteData + 0x00) = *(u32*)(pActorData + 0x20);
    *(u32*)(pSpriteData + 0x04) = *(u32*)(pActorData + 0x24);
    *(u32*)(pSpriteData + 0x08) = *(u32*)(pActorData + 0x28);
}

/* Retail stack+0x10 is the texture-page offset; stack+0x14 is the saved
 * skin selector (including bit7 for field-local packages). */
#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/field/nonmatchings/main/misc2", func_80076AC0);
#else
void func_80076AC0(s32 actorIndex, s32 skinIndex, void* pAnimPackage, s32 spriteMode, s32 texPageOffset, s32 skinSelector, s32 skipInitialTick) {
    u8* pActor;
    u8* pActorData;
    u8* pSpriteData;
    s32 outZ;
    s32 outX;
    s32 outY;
    s32 actorOffset = actorIndex * 0x5C;

    HeapChangeCurrentUser(8, NULL);

    pActor = (u8*)g_FieldActors + actorOffset;
    pActorData = (u8*)(uintptr_t)*(u32*)(pActor + 0x4C);

    *(u8*)(pActorData + 0x127) = skinIndex;
    *(u8*)(pActorData + 0x126) = skinSelector;
    *(u32*)(pActorData + 0x134) =
        (*(u32*)(pActorData + 0x134) & ~0xF) | (texPageOffset & 0xF);
    *(u32*)(pActorData + 0x130) =
        (*(u32*)(pActorData + 0x130) & 0xCFFFFFFF) | ((spriteMode & 0x3) << 28);
    *(u32*)(pActorData + 0x134) =
        (*(u32*)(pActorData + 0x134) & ~0x10) | ((skipInitialTick & 0x1) << 4);

    if (spriteMode == 0) {
        s16 clutX = *(u16*)(D_800B1F78 + skinIndex * 8 + 0);
        s16 clutY = *(u16*)(D_800B1F78 + skinIndex * 8 + 2);

        /* Retail duplicates the flag-guarded sprite-clear into each
         * texPageOffset arm (three func_800230A8 call sites total in this
         * function: 80076C44, 80076CB0, 80076D3C). */
        if (texPageOffset == 0) {
            if (*(u16*)(pActor + 0x5A) & 0x1) {
                func_800230A8((void*)(uintptr_t)*(u32*)(pActor + 0x04));
            }
            pSpriteData = func_80024524(pAnimPackage, 0x100, skinIndex + 0x1E0, clutX, clutY, 0x40);
        } else {
            if (*(u16*)(pActor + 0x5A) & 0x1) {
                func_800230A8((void*)(uintptr_t)*(u32*)(pActor + 0x04));
            }
            pSpriteData = func_80024294(pAnimPackage,
                                        (s16)((texPageOffset << 4) + 0x100),
                                        (s16)(skinIndex + 0x1E0),
                                        clutX,
                                        clutY,
                                        0x40,
                                        texPageOffset);
        }
    } else {
        if (*(u16*)(pActor + 0x5A) & 0x1) {
            func_800230A8((void*)(uintptr_t)*(u32*)(pActor + 0x04));
        }

        if (spriteMode == 1) {
            pSpriteData = func_80024524(pAnimPackage,
                                        0x100,
                                        (s16)(skinIndex + 0xE0),
                                        0x280,
                                        (s16)(skinIndex * 0x40 + 0x100),
                                        8);
        } else {
            pSpriteData = func_80024524(pAnimPackage,
                                        0x100,
                                        (s16)(skinIndex + 0xE3),
                                        0x2A0,
                                        (s16)(skinIndex * 0x40 + 0x100),
                                        8);
        }

        func_80023340(pSpriteData, 0x20);
    }

    *(u32*)(pActor + 0x04) = (u32)(uintptr_t)pSpriteData;
    *(u16*)(pActor + 0x5A) |= 0x1;

    func_8001F5BC(pSpriteData, 0, &outZ, &outX, &outY);
    func_80021C00(pSpriteData, 3);

    *(s16*)(pSpriteData + 0x2C) = 0xC00;
    *(s16*)(pSpriteData + 0x82) = 0x2000;

    if (g_GamePartySkinsInitialized == 0) {
        FieldActorSyncSpritePosition(pActor, pSpriteData);
        *(u32*)(pSpriteData + 0x10) = 0;
        *(u32*)(pSpriteData + 0x0C) = 0;
        *(u32*)(pSpriteData + 0x10) = 0;
        *(u32*)(pSpriteData + 0x14) = 0;
        *(u32*)(pSpriteData + 0x1C) = 0x10000;
        *(s16*)(pSpriteData + 0x84) = *(u32*)(pActor + 0x24);

        if (spriteMode == 0) {
            *(s16*)(pActorData + 0x1A) = outX * 2;
        } else {
            *(s16*)(pActorData + 0x1A) = 0x40;
        }
    }

    if (D_800B218E != 0) {
        *(u32*)(pSpriteData + 0x40) |= 0x40000;
    }

    func_800245D8(pSpriteData, 0);
    func_80021FE0(pSpriteData, 0);
    HeapChangeCurrentUser(8, NULL);

    *(s16*)((u8*)(uintptr_t)*(u32*)(pSpriteData + 0x7C) + 0x14) = actorIndex;
    func_80021BF8(pSpriteData, (s32)func_80076A74);

    if (skipInitialTick == 0) {
        AnimScriptTick(pSpriteData);
        TimerWorkListUpdate();

        if (*(u16*)((u8*)(uintptr_t)*(u32*)(pSpriteData + 0x7C) + 0x0C) == 0xFF) {
            *(s16*)(pActorData + 0xEA) = 0xFF;
            *(u32*)(pActorData + 0x04) |= 0x1000000;
            FieldActorSyncSpritePosition(pActor, pSpriteData);
        }
    }

    /* Retail 80077064..80077118 reloads g_FieldActors[actorIndex] for every
     * one of these stores/loads (the aliasing store through the actor pointer
     * prevents CSE), so index the array at each site instead of caching. */
    *(u32*)((u8*)&g_FieldActors[actorIndex] + 0x20) = *(s16*)((u8*)(uintptr_t)g_FieldActors[actorIndex].pActorData + 0x22);
    *(u32*)((u8*)&g_FieldActors[actorIndex] + 0x40) = *(s16*)((u8*)(uintptr_t)g_FieldActors[actorIndex].pActorData + 0x22);
    *(u32*)((u8*)&g_FieldActors[actorIndex] + 0x24) = *(s16*)((u8*)(uintptr_t)g_FieldActors[actorIndex].pActorData + 0x26);
    *(u32*)((u8*)&g_FieldActors[actorIndex] + 0x44) = *(s16*)((u8*)(uintptr_t)g_FieldActors[actorIndex].pActorData + 0x26);
    *(u32*)((u8*)&g_FieldActors[actorIndex] + 0x28) = *(s16*)((u8*)(uintptr_t)g_FieldActors[actorIndex].pActorData + 0x2A);
    *(u32*)((u8*)&g_FieldActors[actorIndex] + 0x48) = *(s16*)((u8*)(uintptr_t)g_FieldActors[actorIndex].pActorData + 0x2A);
    *(s16*)(pSpriteData + 0x84) = *(u32*)((u8*)&g_FieldActors[actorIndex] + 0x24);
    FieldActorSyncSpritePosition((u8*)&g_FieldActors[actorIndex], pSpriteData);
    D_800AFC74++;
}
#endif /* XENO_PC_PORT */

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/field/nonmatchings/main/misc2", func_800771B0);
#else
void func_800771B0(u32* pBuf, s32 count) {
    s32 i;
    s32 n = (count + 1) / 2 - 1;
    if (n == -1) return;
    for (i = n; i >= 0; i--) {
        *pBuf++ |= 0x80008000;
    }
}
#endif /* XENO_PC_PORT */

void FieldLoadTIM(u_long* pTimData) {
    TIM_IMAGE timImage;

    OpenTIM(pTimData);
    while (ReadTIM(&timImage) ) {
        if (timImage.caddr != NULL) {
            LoadImage(timImage.crect, timImage.caddr);
        }
        if (timImage.paddr != NULL) {
            LoadImage(timImage.prect, timImage.paddr);
        }
    }
}


extern s32 D_800ADBFC;
extern s32 g_PlayerActorIndex;
extern s32 D_800B2360;
extern s32 D_800B2364;
extern s32 D_800B2368;
extern s32 FieldCharacterIdToPartyId(s32 characterId);
extern void func_80084A40(s32 actorIndex, s16 y, void* pFieldActor, void* pActorData,
                          s32 targetState);
extern void func_80081C54(s32 actorIndex);

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/field/nonmatchings/main/misc2", func_80077268);
#else
void func_80077268(void) {
    s32 i;

    /* Retail 80077268 loads g_PlayerActorIndex and g_FieldActors twice here
     * (once for the first func_80084A40 argument, once for the actor/data), so
     * index the array per site rather than caching the pointers. */
    func_80084A40(g_PlayerActorIndex,
                  *(s16*)((u8*)(uintptr_t)g_FieldActors[g_PlayerActorIndex].pActorData + 0x26),
                  (u8*)&g_FieldActors[g_PlayerActorIndex],
                  (u8*)(uintptr_t)g_FieldActors[g_PlayerActorIndex].pActorData, 0);

    for (i = 0; i < D_800ADBFC; i++) {
        u8* actor = (u8*)g_FieldActors + i * 0x5C;
        u8* actorData = (u8*)(uintptr_t)*(u32*)(actor + 0x4C);

        if ((*(u16*)(actor + 0x58) & 0x0F80) == 0x0200) {
            s32 partyId = FieldCharacterIdToPartyId(*(s16*)(actorData + 0xE4));

            if (partyId != -1 && partyId != 0) {
                u8* spriteData = (u8*)(uintptr_t)*(u32*)(actor + 0x04);

                func_80084A40(i,
                              *(s16*)((u8*)(uintptr_t)g_FieldActors[i].pActorData + 0x26),
                              actor,
                              (u8*)(uintptr_t)g_FieldActors[i].pActorData, 0);

                /* Retail 80077368..80077450 reloads g_PlayerActorIndex and
                 * g_FieldActors and recomputes the 0x5C stride for each of
                 * these six copies, so index the array per site. */
                *(u32*)(spriteData + 0x00) = *(u32*)((u8*)(uintptr_t)g_FieldActors[g_PlayerActorIndex].pSpriteData + 0x00);
                *(u32*)(spriteData + 0x04) = *(u32*)((u8*)(uintptr_t)g_FieldActors[g_PlayerActorIndex].pSpriteData + 0x04);
                *(u32*)(spriteData + 0x08) = *(u32*)((u8*)(uintptr_t)g_FieldActors[g_PlayerActorIndex].pSpriteData + 0x08);
                *(u32*)((u8*)&g_FieldActors[i] + 0x20) = *(u32*)((u8*)&g_FieldActors[g_PlayerActorIndex] + 0x20);
                *(u32*)((u8*)&g_FieldActors[i] + 0x24) = *(u32*)((u8*)&g_FieldActors[g_PlayerActorIndex] + 0x24);
                *(u32*)((u8*)&g_FieldActors[i] + 0x28) = *(u32*)((u8*)&g_FieldActors[g_PlayerActorIndex] + 0x28);
            }
        }
    }

    D_800B2368 = 0;
    D_800B2364 = 0;
    D_800B2360 = 0;

    for (i = 0; i < 0x20; i++) {
        func_80081C54(g_PlayerActorIndex);
    }
}
#endif /* XENO_PC_PORT */

extern void func_8003747C(void* pFont);
extern void* FontLoadFont(int startX, int startY, int width, int height,
                          int maxLetters, unsigned int flags, int texpageX,
                          int texpageY, int clutX, int clutY,
                          void* pCompressedFontFile);
extern void SystemTransferPaletteToVRAM(short xDest, short yDest);

void func_80077544(void) {
    if (g_FieldSystemMode == SYSTEM_MODE_PC_HDD) {
        func_8003747C((void*)0x80270000);
        FontLoadFont(0x10, 0x10, 0x130, 0xE0, 0x400, 4,
                     0x3C0, 0x100, 0x100, 0x1FF, 0);
    }

    SystemTransferPaletteToVRAM(0x100, 0xF0);
}
