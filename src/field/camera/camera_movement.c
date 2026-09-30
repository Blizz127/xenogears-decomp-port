#include "common.h"
#include "main/game.h"
#include "system/math.h"
#include "field/main.h"
#include "field/camera.h"
#include "field/actor.h"
#include "field/script_vm.h"

extern s16 g_CamEyeMovementDuration;
extern s16 g_CamAtMovementDuration;
extern VECTOR g_CamAtMovementFrom;
extern VECTOR g_CamAtMovementTo;
extern VECTOR g_CamEyeMovementFrom;
extern VECTOR g_CamEyeMovementTo;
extern VECTOR g_CameraEye;
extern VECTOR g_CameraEye2;
extern VECTOR g_CameraAt;
extern VECTOR g_CameraAt2;
extern u16 g_CamMovementFlags;
extern VECTOR g_CamAtMovementCurrent;
extern VECTOR g_CamAtMovementDelta;
extern VECTOR g_CamEyeMovementCurrent;
extern VECTOR g_CamEyeMovementDelta;
extern s16 g_FieldCameraMode;
extern s32 D_800AF930[];
extern s16 D_800AF936[];
extern s16 D_800AF938[];
extern s16 D_800AF93A[];
extern s32 D_800B21D8;
extern s16 g_FieldCameraModes[] asm("g_FieldCameraMode");


void FieldScriptWaitForCameraMovement(void) {
    int targetFlags;
    int camMovementFlags;
    
    targetFlags = FieldScriptVMGetArgument(1);
    camMovementFlags = FIELD_CAMERA_AT_MOVEMENT_ACTIVE | FIELD_CAMERA_EYE_MOVEMENT_ACTIVE;
    
    // Is camera target movement done?
    if (g_CamAtMovementDuration == 0) {
        camMovementFlags &= ~FIELD_CAMERA_AT_MOVEMENT_ACTIVE;
    }
    
    // Is camera position movement done?
    if (g_CamEyeMovementDuration == 0) {
        camMovementFlags &= FIELD_CAMERA_AT_MOVEMENT_ACTIVE;
    }
    
    D_800B00C0 = 1;

    // Advance instruction pointer only when certain camera movements are done
    if (!(camMovementFlags & targetFlags)) {
        g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
    }
}


void func_8008FABC(void) {
    s16 angleY;

    angleY = FieldScriptArgument1(1, SCRIPT_READ_U8_REL(3));
    g_Scene.sceneAngle.vy = angleY;
    g_Scene.unk7C = angleY;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 4;
}

/* FE6D — snapshot current SCRZ×scale / DIP / yaw into the scripted-cam scratch. */
void func_8008FB28(void) {
    D_800AF93A[0] = 0x1000;
    D_800AF938[0] = *(u16*)((u8*)&g_Scene + 0x56);
    D_800AF936[0] = *(u16*)((u8*)&g_Scene + 0x6C);
    D_800AF930[0] = (*(s32*)((u8*)&g_Scene + 0x68) * *(s16*)((u8*)&g_Scene + 0x6E)) >> 12;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 1;
}

void func_8008FB98(void) {
#ifdef XENO_PC_PORT
    u16 angleY;
    u16 dip;
#else
    register u16 angleY asm("$3");
    register u16 dip asm("$4");
#endif

    g_FieldCameraMode = 1;
    g_FieldScriptMaxInstructionCount += 4;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 1;

    D_800AF93A[0] = 0x1000;
    D_800AF930[0] = ((s32)g_Scene.sceneScrZ * (s16)g_Scene.sceneScale) >> 12;
    angleY = g_Scene.sceneAngle.vy;
    dip = g_Scene.sceneDIP;
    g_CamInterpolation.atStepDistance = 0xC;
    g_CamInterpolation.eyeStepDistance = 0xC;
    D_800AF938[0] = angleY;
    g_Scene.unk48 |= 0x8000;
    D_800AF936[0] = dip;
}

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/field/nonmatchings/camera/camera_movement", func_8008FC4C);
#else
void func_8008FC4C(void) {
#ifdef XENO_PC_PORT
    ActorData* actor;
#else
    register ActorData* actor asm("$3");
#endif
    s16* cameraMode;
    s16 mode;
    s32 sceneFlags;
    s32 value;

    cameraMode = g_FieldCameraModes;
    mode = *cameraMode;
    if (mode == 1) {
        goto mode1;
    }
    if (mode >= 2) {
        return;
    }
    if (mode != 0) {
        return;
    }

    sceneFlags = *(volatile s32*)&g_Scene.unk48;
    actor = g_FieldScriptVMCurActor;
    *(volatile s32*)&g_Scene.unk48 = sceneFlags & 0x7FFF;
    goto advance;

mode1:
    value = FieldScriptVMGetArgument(1);
    if (value == 0) {
        *cameraMode = 0;
        sceneFlags = *(volatile s32*)&g_Scene.unk48;
        actor = g_FieldScriptVMCurActor;
        *(volatile s32*)&g_Scene.unk48 = sceneFlags & 0x7FFF;
        actor->scriptInstructionPointer += 3;
        D_800B21D8 = 2;
    } else {
        *cameraMode = 2;
        g_CamInterpolation.atStepDistance = value;
        g_CamInterpolation.eyeStepDistance = value;
    }

    actor = g_FieldScriptVMCurActor;
advance:
    actor->scriptInstructionPointer += 3;
}
#endif /* XENO_PC_PORT */

void FieldScriptSetCameraInterpolationStep(void) {
    g_CamInterpolation.atStepDistance = FieldScriptVMGetArgument(1);
    g_CamInterpolation.eyeStepDistance = FieldScriptVMGetArgument(3);
    
    if (g_CamInterpolation.atStepDistance == 0) {
        g_CamInterpolation.atStepDistance = 1;
    }
    if (g_CamInterpolation.eyeStepDistance == 0) {
        g_CamInterpolation.eyeStepDistance = 1;
    }
    g_FieldScriptMaxInstructionCount += 1;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
}

// Reset initial camera target movement to current camera target
void FieldScriptResetCameraTargetMovement(void) {
    g_CamAtMovementFrom.vx = g_CameraAt2.vx;
    g_CamAtMovementFrom.vy = g_CameraAt2.vy;
    g_CamAtMovementFrom.vz = g_CameraAt2.vz;
    g_FieldScriptMaxInstructionCount++;
    g_FieldScriptVMCurActor->scriptInstructionPointer++;
}

void FieldScriptSetCameraTargetMovementFrom(void) {
    g_CamAtMovementFrom.vx = CONV_FROM_GTE(FieldScriptArgument1(ARG(1), SCRIPT_READ_U8_REL(7)));
    g_CamAtMovementFrom.vz = CONV_FROM_GTE(FieldScriptArgument2(ARG(2), SCRIPT_READ_U8_REL(7))); 
    g_CamAtMovementFrom.vy = CONV_FROM_GTE(FieldScriptArgument3(ARG(3), SCRIPT_READ_U8_REL(7)));
    g_FieldScriptMaxInstructionCount += 1;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 8;
}

// Set initial camera target movement to position of an actor
void FieldScriptSetCameraTargetMovementDestToActor(void) {
    ActorData* pActor = (ActorData*)(uintptr_t)g_FieldActors[func_8009CD7C(1)].pActorData;
    g_CamAtMovementTo.vx = pActor->position.vx;
    g_CamAtMovementTo.vy = pActor->position.vy;
    g_CamAtMovementTo.vz = pActor->position.vz;  
    g_FieldScriptMaxInstructionCount += 1;    
    g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
}

void FieldScriptSetCameraTargetMovementDest(void) {
    g_CamAtMovementTo.vx = CONV_FROM_GTE(FieldScriptArgument1(ARG(1), SCRIPT_READ_U8_REL(7)));
    g_CamAtMovementTo.vz = CONV_FROM_GTE(FieldScriptArgument2(ARG(2), SCRIPT_READ_U8_REL(7)));
    g_CamAtMovementTo.vy = CONV_FROM_GTE(FieldScriptArgument3(ARG(3), SCRIPT_READ_U8_REL(7)));
    g_FieldScriptMaxInstructionCount += 1;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 8;
}

// Reset initial camera position movement to current camera position
void FieldScriptResetCameraPosMovement(void) {
    g_CamEyeMovementFrom.vx = g_CameraEye2.vx;
    g_CamEyeMovementFrom.vy = g_CameraEye2.vy;
    g_CamEyeMovementFrom.vz = g_CameraEye2.vz;
    g_FieldScriptMaxInstructionCount += 1;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 1;
}

void FieldScriptSetCameraPosMovementFrom(void) {
    g_CamEyeMovementFrom.vx = CONV_FROM_GTE(FieldScriptArgument1(ARG(1), SCRIPT_READ_U8_REL(7)));
    g_CamEyeMovementFrom.vz = CONV_FROM_GTE(FieldScriptArgument2(ARG(2), SCRIPT_READ_U8_REL(7))); 
    g_CamEyeMovementFrom.vy = CONV_FROM_GTE(FieldScriptArgument3(ARG(3), SCRIPT_READ_U8_REL(7)));
    g_FieldScriptMaxInstructionCount += 1;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 8;
}

void FieldScriptSetCameraPosMovementDestToActor(void) {
    ActorData* pActor = (ActorData*)(uintptr_t)g_FieldActors[func_8009CD7C(1)].pActorData;
    g_CamEyeMovementTo.vx = pActor->position.vx;
    g_CamEyeMovementTo.vy = pActor->position.vy;
    g_CamEyeMovementTo.vz = pActor->position.vz;  
    g_FieldScriptMaxInstructionCount += 1;    
    g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
}

void FieldScriptSetCameraPosMovementDest(void) {
    g_CamEyeMovementTo.vx = CONV_FROM_GTE(FieldScriptArgument1(ARG(1), SCRIPT_READ_U8_REL(7)));
    g_CamEyeMovementTo.vz = CONV_FROM_GTE(FieldScriptArgument2(ARG(2), SCRIPT_READ_U8_REL(7)));
    g_CamEyeMovementTo.vy = CONV_FROM_GTE(FieldScriptArgument3(ARG(3), SCRIPT_READ_U8_REL(7)));
    g_FieldScriptMaxInstructionCount += 1;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 8;
}

void FieldScriptResetCameraMovements(void) {
    g_CamAtMovementFrom.vx = g_CameraAt2.vx;
    g_CamAtMovementFrom.vy = g_CameraAt2.vy;
    g_CamAtMovementFrom.vz = g_CameraAt2.vz;
    
    g_CamAtMovementTo.vx = g_CameraAt2.vx;
    g_CamAtMovementTo.vy = g_CameraAt2.vy;
    g_CamAtMovementTo.vz = g_CameraAt2.vz;

    g_CamEyeMovementFrom.vx = g_CameraEye2.vx;
    g_CamEyeMovementFrom.vy = g_CameraEye2.vy;
    g_CamEyeMovementFrom.vz = g_CameraEye2.vz;
    
    g_CamEyeMovementTo.vx = g_CameraEye2.vx;
    g_CamEyeMovementTo.vy = g_CameraEye2.vy;
    g_CamEyeMovementTo.vz = g_CameraEye2.vz;
    
    g_FieldScriptMaxInstructionCount += 1;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 1;
}

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/field/nonmatchings/camera/camera_movement", FieldScriptStartCameraMovement);
#else
void FieldScriptStartCameraMovement(void) {
    s32 duration;
    s32 speed;
    s32 distance;
    s32 deltaX;
    s32 deltaY;
    s32 deltaZ;
    s32 fromX;
    s32 fromY;
    s32 fromZ;
    VECTOR direction;
    VECTOR normal;

    switch (SCRIPT_READ_U8_REL(1) & 0xF) {
    // Linearly move the camera target over a fixed number of frames.
    case 0:
        deltaZ = FieldScriptVMGetArgument(2);
        g_CamAtMovementDuration = deltaZ;
        if ((deltaZ << 16) == 0) {
            g_CamAtMovementDuration = deltaZ + 1;
            g_CamInterpolation.atStepDistance = 1;
        }

        deltaX = (g_CamAtMovementTo.vx - g_CamAtMovementFrom.vx) / g_CamAtMovementDuration;
        deltaY = (g_CamAtMovementTo.vy - g_CamAtMovementFrom.vy) / g_CamAtMovementDuration;
        deltaZ = (g_CamAtMovementTo.vz - g_CamAtMovementFrom.vz) / g_CamAtMovementDuration;
        fromX = g_CamAtMovementFrom.vx;
        fromY = g_CamAtMovementFrom.vy;
        fromZ = g_CamAtMovementFrom.vz;
        g_CamAtMovementCurrent.vx = fromX;
        g_CamAtMovementCurrent.vy = fromY;
        g_CamAtMovementCurrent.vz = fromZ;
        g_CamMovementFlags |= FIELD_CAMERA_AT_MOVEMENT_ACTIVE;
        g_CamAtMovementDelta.vx = deltaX;
        g_CamAtMovementDelta.vy = deltaY;
        g_CamAtMovementDelta.vz = deltaZ;

        if (SCRIPT_READ_U8_REL(1) & 0x80) {
            g_CameraAt.vx = g_CamAtMovementFrom.vx;
            g_CameraAt.vy = g_CamAtMovementFrom.vy;
            g_CameraAt.vz = g_CamAtMovementFrom.vz;
        }
        break;

    // Move the camera target toward its destination at a fixed speed.
    case 2:
        direction.vx = (g_CamAtMovementFrom.vx - g_CamAtMovementTo.vx) >> 16;
        direction.vy = (g_CamAtMovementFrom.vy - g_CamAtMovementTo.vy) >> 16;
        direction.vz = (g_CamAtMovementFrom.vz - g_CamAtMovementTo.vz) >> 16;
        VectorNormal(&direction, &normal);

        distance = FieldGetVec3Magnitude(
            (g_CamAtMovementFrom.vx - g_CamAtMovementTo.vx) >> 16,
            (g_CamAtMovementFrom.vy - g_CamAtMovementTo.vy) >> 16,
            (g_CamAtMovementFrom.vz - g_CamAtMovementTo.vz) >> 16);
        speed = FieldScriptVMGetArgument(2);

        duration = distance / speed;
        deltaX = normal.vx * speed;
        deltaY = normal.vy * speed;
        deltaZ = normal.vz * speed;
        g_CamAtMovementCurrent.vx = g_CamAtMovementFrom.vx;
        g_CamAtMovementCurrent.vy = g_CamAtMovementFrom.vy;
        g_CamAtMovementCurrent.vz = g_CamAtMovementFrom.vz;
        g_CamMovementFlags |= FIELD_CAMERA_AT_MOVEMENT_ACTIVE;
        g_CamAtMovementDelta.vx = -deltaX << 4;
        g_CamAtMovementDelta.vy = -deltaY << 4;
        g_CamAtMovementDelta.vz = -deltaZ << 4;
        g_CamAtMovementDuration = duration;

        if (SCRIPT_READ_U8_REL(1) & 0x80) {
            g_CameraAt.vx = g_CamAtMovementFrom.vx;
            g_CameraAt.vy = g_CamAtMovementFrom.vy;
            g_CameraAt.vz = g_CamAtMovementFrom.vz;
        }
        break;

    // Move the camera eye toward its destination at a fixed speed.
    case 3:
        direction.vx = (g_CamEyeMovementFrom.vx - g_CamEyeMovementTo.vx) >> 16;
        direction.vy = (g_CamEyeMovementFrom.vy - g_CamEyeMovementTo.vy) >> 16;
        direction.vz = (g_CamEyeMovementFrom.vz - g_CamEyeMovementTo.vz) >> 16;
        VectorNormal(&direction, &normal);

        distance = FieldGetVec3Magnitude(
            (g_CamEyeMovementFrom.vx - g_CamEyeMovementTo.vx) >> 16,
            (g_CamEyeMovementFrom.vy - g_CamEyeMovementTo.vy) >> 16,
            (g_CamEyeMovementFrom.vz - g_CamEyeMovementTo.vz) >> 16);
        speed = FieldScriptVMGetArgument(2);

        duration = distance / speed;
        deltaX = normal.vx * speed;
        deltaY = normal.vy * speed;
        deltaZ = normal.vz * speed;
        g_CamEyeMovementCurrent.vx = g_CamEyeMovementFrom.vx;
        g_CamEyeMovementCurrent.vy = g_CamEyeMovementFrom.vy;
        g_CamEyeMovementCurrent.vz = g_CamEyeMovementFrom.vz;
        g_CamMovementFlags |= FIELD_CAMERA_EYE_MOVEMENT_ACTIVE;
        g_CamEyeMovementDelta.vx = -deltaX << 4;
        g_CamEyeMovementDelta.vy = -deltaY << 4;
        g_CamEyeMovementDelta.vz = -deltaZ << 4;
        g_CamEyeMovementDuration = duration;

        if (SCRIPT_READ_U8_REL(1) & 0x80) {
            g_CameraEye.vx = g_CamEyeMovementFrom.vx;
            g_CameraEye.vy = g_CamEyeMovementFrom.vy;
            g_CameraEye.vz = g_CamEyeMovementFrom.vz;
        }
        break;

    // Linearly move the camera eye over a fixed number of frames.
    case 1:
        duration = FieldScriptVMGetArgument(2);
        g_CamEyeMovementDuration = duration;
        if ((duration << 16) == 0) {
            g_CamEyeMovementDuration = duration + 1;
            g_CamInterpolation.eyeStepDistance = 1;
        }

        deltaX = (g_CamEyeMovementTo.vx - g_CamEyeMovementFrom.vx) / g_CamEyeMovementDuration;
        deltaY = (g_CamEyeMovementTo.vy - g_CamEyeMovementFrom.vy) / g_CamEyeMovementDuration;
        deltaZ = (g_CamEyeMovementTo.vz - g_CamEyeMovementFrom.vz) / g_CamEyeMovementDuration;
        g_CamEyeMovementCurrent.vx = g_CamEyeMovementFrom.vx;
        g_CamEyeMovementCurrent.vy = g_CamEyeMovementFrom.vy;
        g_CamEyeMovementCurrent.vz = g_CamEyeMovementFrom.vz;
        g_CamMovementFlags |= FIELD_CAMERA_EYE_MOVEMENT_ACTIVE;
        g_CamEyeMovementDelta.vx = deltaX;
        g_CamEyeMovementDelta.vy = deltaY;
        g_CamEyeMovementDelta.vz = deltaZ;

        if (SCRIPT_READ_U8_REL(1) & 0x80) {
            g_CameraEye.vx = g_CamEyeMovementFrom.vx;
            g_CameraEye.vy = g_CamEyeMovementFrom.vy;
            g_CameraEye.vz = g_CamEyeMovementFrom.vz;
        }
        break;
    }

    g_FieldScriptVMCurActor->scriptInstructionPointer += 4;
}
#endif /* XENO_PC_PORT */

// Write the current interpolated value
void FieldScriptWriteCurCameraTarget(void) {
    FieldScriptMemoryWriteU16(SCRIPT_IMM_ARG(1), CONV_TO_GTE(g_CameraAt.vx));
    FieldScriptMemoryWriteU16(SCRIPT_IMM_ARG(2), CONV_TO_GTE(g_CameraAt.vz));
    FieldScriptMemoryWriteU16(SCRIPT_IMM_ARG(3), CONV_TO_GTE(g_CameraAt.vy));
    g_FieldScriptMaxInstructionCount++;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 7;
}

// Write the current interpolated value
void FieldScriptWriteCurCameraPosition(void) {
    FieldScriptMemoryWriteU16(SCRIPT_IMM_ARG(1), CONV_TO_GTE(g_CameraEye.vx));
    FieldScriptMemoryWriteU16(SCRIPT_IMM_ARG(2), CONV_TO_GTE(g_CameraEye.vz));
    FieldScriptMemoryWriteU16(SCRIPT_IMM_ARG(3), CONV_TO_GTE(g_CameraEye.vy));
    g_FieldScriptMaxInstructionCount++;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 7;
}

// Write the target / desired value
void FieldScriptWriteCameraTweenTarget(void) {
    FieldScriptMemoryWriteU16(SCRIPT_IMM_ARG(1), CONV_TO_GTE(g_CameraAt2.vx));
    FieldScriptMemoryWriteU16(SCRIPT_IMM_ARG(2), CONV_TO_GTE(g_CameraAt2.vz));
    FieldScriptMemoryWriteU16(SCRIPT_IMM_ARG(3), CONV_TO_GTE(g_CameraAt2.vy));
    g_FieldScriptMaxInstructionCount++;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 7;
}

// Write the target / desired value
void FieldScriptWriteCameraTweenPosition(void) {
    FieldScriptMemoryWriteU16(SCRIPT_IMM_ARG(1), CONV_TO_GTE(g_CameraEye2.vx));
    FieldScriptMemoryWriteU16(SCRIPT_IMM_ARG(2), CONV_TO_GTE(g_CameraEye2.vz));
    FieldScriptMemoryWriteU16(SCRIPT_IMM_ARG(3), CONV_TO_GTE(g_CameraEye2.vy));
    g_FieldScriptMaxInstructionCount++;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 7;
}

/* Camera mode 1 exposes its yaw, pitch, and distance through script
 * variables.  A zero type byte means read the live value into the supplied
 * variable; any nonzero type byte writes the supplied immediate value back
 * into the camera-mode state. */
void func_80090C20(void) {
    if (SCRIPT_READ_U8_REL(3) == 0) {
        FieldScriptMemoryWriteU16(
            FieldScriptVMGetInstructionArgument(1) & 0xFFFF,
            D_800AF938[0]);
    } else {
        D_800AF938[0] = FieldScriptVMGetInstructionArgument(1);
    }

    g_FieldScriptMaxInstructionCount++;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 4;
}

void func_80090CB8(void) {
    if (SCRIPT_READ_U8_REL(3) == 0) {
        FieldScriptMemoryWriteU16(
            FieldScriptVMGetInstructionArgument(1) & 0xFFFF,
            D_800AF936[0]);
    } else {
        D_800AF936[0] = FieldScriptVMGetInstructionArgument(1);
    }

    g_FieldScriptMaxInstructionCount++;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 4;
}

void func_80090D50(void) {
    if (SCRIPT_READ_U8_REL(3) == 0) {
        FieldScriptMemoryWriteU16(
            FieldScriptVMGetInstructionArgument(1) & 0xFFFF,
            D_800AF930[0]);
    } else {
        D_800AF930[0] = FieldScriptVMGetInstructionArgument(1) & 0xFFFF;
    }

    g_FieldScriptMaxInstructionCount++;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 4;
}

void func_80090DEC(void) {
    FieldScriptMemoryWriteU16(FieldScriptVMGetInstructionArgument(1) & 0xFFFF, D_800AF938[0]);
    FieldScriptMemoryWriteU16(FieldScriptVMGetInstructionArgument(3) & 0xFFFF, D_800AF936[0]);
    FieldScriptMemoryWriteU16(FieldScriptVMGetInstructionArgument(5) & 0xFFFF, D_800AF930[0]);
    g_FieldScriptMaxInstructionCount++;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 7;
}

void func_80090E70(void) {
    VECTOR camAtDest;
    VECTOR camEyeDest;
    s32 yaw;
    s32 pitch;
    s32 halfDistance;
    long distance;
    u16 yawAngle;
    int pitchAngle;

    camAtDest.vx = g_CamAtMovementTo.vx;
    camAtDest.vy = g_CamAtMovementTo.vy;
    camAtDest.vz = g_CamAtMovementTo.vz;
    camEyeDest.vx = g_CamEyeMovementTo.vx;
    camEyeDest.vy = g_CamEyeMovementTo.vy;
    camEyeDest.vz = g_CamEyeMovementTo.vz;
    
    // Compute the half distance between the destination camera position and destination
    // camera target of the camera movement
    distance = FieldGetVec3Magnitude(
        CONV_TO_GTE(g_CamEyeMovementTo.vx - g_CamAtMovementTo.vx), 
        CONV_TO_GTE(g_CamEyeMovementTo.vy - g_CamAtMovementTo.vy), 
        CONV_TO_GTE(g_CamEyeMovementTo.vz - g_CamAtMovementTo.vz)
    );
    halfDistance = distance / 2;
    
    D_800AF93A[0] = 0x1000;

    // Cursed angle math
    yawAngle = ratan2(camAtDest.vz - camEyeDest.vz, camAtDest.vx - camEyeDest.vx);
    yaw = PSX_ANGLE((-yawAngle & 0xFFFF) - PSX_DEGREES(90));

    pitchAngle = -ratan2(
        FieldGetVec2Magnitude(
            CONV_TO_GTE(camEyeDest.vx - camAtDest.vx), 
            CONV_TO_GTE(camEyeDest.vz - camAtDest.vz)
        ), 
        CONV_TO_GTE(camAtDest.vy - camEyeDest.vy)
    );
    pitch = ((pitchAngle * 360) >> 12) + 91;
    
    FieldScriptMemoryWriteU16(SCRIPT_IMM_ARG(1), yaw);
    FieldScriptMemoryWriteU16(SCRIPT_IMM_ARG(2), pitch);
    FieldScriptMemoryWriteU16(SCRIPT_IMM_ARG(3), halfDistance);
    
    g_FieldScriptMaxInstructionCount += 1;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 7;
}

void func_80091008(VECTOR* destination, VECTOR* origin, s32 angle) {
    MATRIX rotationMatrix;
    VECTOR offset;
    VECTOR rotatedOffset;
    SVECTOR rotation;

    rotation.vx = 0;
    rotation.vy = angle;
    rotation.vz = 0;
    PushMatrix();
    RotMatrix(&rotation, &rotationMatrix);

    offset.vx = origin->vx - destination->vx;
    offset.vy = origin->vy - destination->vy;
    offset.vz = origin->vz - destination->vz;
    ApplyMatrixLV(&rotationMatrix, &offset, &rotatedOffset);

    destination->vx = rotatedOffset.vx + origin->vx;
    destination->vz = rotatedOffset.vz + origin->vz;
    PopMatrix();
}

void func_800910C0(void) {
    VECTOR parameter;
    VECTOR vecResult;
    s32 angle;
    s32 temp_s1;
    s32 factor;
    s32 yAngle;

    parameter.vx = FieldScriptArgument1(1, SCRIPT_READ_U8_REL(0xD)) << 16;
    parameter.vz = FieldScriptArgument2(3, SCRIPT_READ_U8_REL(0xD)) << 16;
    parameter.vy = FieldScriptArgument3(5, SCRIPT_READ_U8_REL(0xD)) << 16;
    yAngle = FieldScriptArgument4(7, SCRIPT_READ_U8_REL(0xD));
    temp_s1 = FieldScriptArgument5(9, SCRIPT_READ_U8_REL(0xD));
    factor = FieldScriptArgument6(0xB, SCRIPT_READ_U8_REL(0xD));

    angle = ((temp_s1 * 0xB60) >> 8) + 0xC00;
    vecResult.vy = ((-((rsin(angle) * factor) << 5) >> 16) * D_800AF93A[0] * 16) + parameter.vy;
    vecResult.vz = ((((rcos(angle) * factor) << 5) >> 16) * D_800AF93A[0] * 16) + parameter.vz;
    vecResult.vx = parameter.vx;

    func_80091008(&vecResult, &parameter, yAngle);

    FieldScriptMemoryWriteU16(FieldScriptVMGetInstructionArgument(0xE) & 0xFFFF, vecResult.vx >> 16);
    FieldScriptMemoryWriteU16(FieldScriptVMGetInstructionArgument(0x10) & 0xFFFF, vecResult.vz >> 16);
    FieldScriptMemoryWriteU16(FieldScriptVMGetInstructionArgument(0x12) & 0xFFFF, vecResult.vy >> 16);

    g_FieldScriptMaxInstructionCount += 1;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 0x14;
}

void func_80091318(void) {
    VECTOR parameter;
    VECTOR vecResult;
    s32 angle;
    s32 temp_s1;
    s32 factor;
    s32 yAngle;
    u8 parameterType;

    parameterType = SCRIPT_READ_U8_REL(1);
    switch (parameterType) {
    case 0:
        parameter.vx = g_CamAtMovementFrom.vx;
        parameter.vy = g_CamAtMovementFrom.vy;
        parameter.vz = g_CamAtMovementFrom.vz;
        break;
    case 1:
        parameter.vx = g_CamAtMovementTo.vx;
        parameter.vy = g_CamAtMovementTo.vy;
        parameter.vz = g_CamAtMovementTo.vz;
        break;
    case 2:
        parameter.vx = g_CamEyeMovementFrom.vx;
        parameter.vy = g_CamEyeMovementFrom.vy;
        parameter.vz = g_CamEyeMovementFrom.vz;
        break;
    case 3:
        parameter.vx = g_CamEyeMovementTo.vx;
        parameter.vy = g_CamEyeMovementTo.vy;
        parameter.vz = g_CamEyeMovementTo.vz;
        break;
    }
    
    yAngle = FieldScriptArgument1(2, SCRIPT_READ_U8_REL(8));
    temp_s1 = FieldScriptArgument2(4, SCRIPT_READ_U8_REL(8));
    factor = FieldScriptArgument3(6, SCRIPT_READ_U8_REL(8));
    
    // 0xC00 = 270 degrees
    angle = ((temp_s1 * 0xB60) >> 8) + 0xC00;

    // (number << 5) >> 0x10 is division by 0x800 = 180 degrees
    vecResult.vy = ((-((rsin(angle) * factor) << 5) >> 0x10) * D_800AF93A[0] * 0x10) + parameter.vy;
    vecResult.vz = ((((rcos(angle) * factor) << 5) >> 0x10) * D_800AF93A[0] * 0x10) + parameter.vz;
    vecResult.vx = parameter.vx;

    // Rotate difference between parameter value and the result vector by Y angle,
    // then we add it to parameter and set our result vector to it
    func_80091008(&vecResult, &parameter, yAngle);
    
    FieldScriptMemoryWriteU16(SCRIPT_IMM_ARG(5), vecResult.vx >> 0x10);
    FieldScriptMemoryWriteU16(SCRIPT_IMM_ARG(6), vecResult.vz >> 0x10);
    FieldScriptMemoryWriteU16(SCRIPT_IMM_ARG(7), vecResult.vy >> 0x10);
    
    g_FieldScriptMaxInstructionCount += 1;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 15;
}

void FieldScriptWriteCameraMovementParameter(void) {
    int mode;
    VECTOR movementParam;

    mode = SCRIPT_READ_U8_REL(1);
    switch (mode) { 
        case 0:
            movementParam.vx  = g_CamAtMovementFrom.vx;
            movementParam.vy = g_CamAtMovementFrom.vy;
            movementParam.vz = g_CamAtMovementFrom.vz;
            break;
        case 1:
            movementParam.vx  = g_CamAtMovementTo.vx;
            movementParam.vy = g_CamAtMovementTo.vy;
            movementParam.vz = g_CamAtMovementTo.vz;
            break;
        case 2:
            movementParam.vx  = g_CamEyeMovementFrom.vx;
            movementParam.vy = g_CamEyeMovementFrom.vy;
            movementParam.vz = g_CamEyeMovementFrom.vz;
            break;
        case 3:
            movementParam.vx  = g_CamEyeMovementTo.vx;
            movementParam.vy = g_CamEyeMovementTo.vy;
            movementParam.vz = g_CamEyeMovementTo.vz;
            break;
    }
    
    FieldScriptMemoryWriteU16(SCRIPT_IMM_ARG_ALIGNED(1), CONV_TO_GTE(movementParam.vx));
    FieldScriptMemoryWriteU16(SCRIPT_IMM_ARG_ALIGNED(2), CONV_TO_GTE(movementParam.vz));
    FieldScriptMemoryWriteU16(SCRIPT_IMM_ARG_ALIGNED(3), CONV_TO_GTE(movementParam.vy));

    g_FieldScriptMaxInstructionCount += 1;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 8;
}

void FieldScriptSetCameraMovementParameter(void) {
    VECTOR parameter;
    u8 srcParameter;
    u8 destParameter;

    srcParameter = SCRIPT_READ_U8_REL(1);
    switch (srcParameter) {
    case 0:
        parameter.vx = g_CamAtMovementFrom.vx;
        parameter.vy = g_CamAtMovementFrom.vy;
        parameter.vz = g_CamAtMovementFrom.vz;
        break;
    case 1:
        parameter.vx = g_CamAtMovementTo.vx;
        parameter.vy = g_CamAtMovementTo.vy;
        parameter.vz = g_CamAtMovementTo.vz;
        break;
    case 2:
        parameter.vx = g_CamEyeMovementFrom.vx;
        parameter.vy = g_CamEyeMovementFrom.vy;
        parameter.vz = g_CamEyeMovementFrom.vz;
        break;
    case 3:
        parameter.vx = g_CamEyeMovementTo.vx;
        parameter.vy = g_CamEyeMovementTo.vy;
        parameter.vz = g_CamEyeMovementTo.vz;
        break;
    }
    
    destParameter = SCRIPT_READ_U8_REL(2);
    switch (destParameter) {
    case 0:
        g_CamAtMovementFrom.vx = parameter.vx;
        g_CamAtMovementFrom.vy = parameter.vy;
        g_CamAtMovementFrom.vz = parameter.vz;
        break;
    case 1:
        g_CamAtMovementTo.vx = parameter.vx;
        g_CamAtMovementTo.vy = parameter.vy;
        g_CamAtMovementTo.vz = parameter.vz;
        break;
    case 2:
        g_CamEyeMovementFrom.vx = parameter.vx;
        g_CamEyeMovementFrom.vy = parameter.vy;
        g_CamEyeMovementFrom.vz = parameter.vz;
        break;
    case 3:
        g_CamEyeMovementTo.vx = parameter.vx;
        g_CamEyeMovementTo.vy = parameter.vy;
        g_CamEyeMovementTo.vz = parameter.vz;
        break;
    }
    
    g_FieldScriptMaxInstructionCount += 1;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}
