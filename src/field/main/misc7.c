#include "common.h"
#ifdef XENO_PC_PORT
#include <stdio.h>
#include <stdlib.h>
#endif
#include "main/game.h"
#include "system/math.h"
#include "field/main.h"
#include "field/actor.h"
#include "field/camera.h"
#include "field/script_vm.h"
#include "field/text_box.h"
/* Typed array views of g_Scene: retail schedules stores into it like stores
 * to an array-typed extern, so raw-offset accesses go through these. */
extern s16 g_Scene_s16[] asm("g_Scene");
extern s32 g_Scene_s32[] asm("g_Scene");
extern u16 g_Scene_u16[] asm("g_Scene");

extern FieldActor* D_800B06B8;
extern s32 D_800AFD1C;
extern s32 g_PlayerActorIndex;
extern s32 D_800B21D8[];
extern s16 D_800AFB54;
extern s16 func_8007B1C4(s16 x, s16 z, s32 walkmeshId, s16* out, s32* state);
extern int FieldScriptArgument1(int index, int mask);
extern int FieldScriptArgument2(int index, int mask);
extern int FieldScriptArgument3(int index, int mask);
extern int FieldScriptArgument4(int index, int mask);
extern u32 FieldScriptVMGetActorIndex(int bytecodeOffset);
long FieldGetVec3Magnitude(long x, long y, long z);
long FieldGetVec2Magnitude(long x, long y);
long FieldGetVec1Magnitude(long x);
s32 func_80099AC0(s32 useStoredAngle);
s32 func_80097A50(s32 targetValue);
extern s32 func_8007B694(s32* arg0);
extern long VectorNormal(VECTOR* v0, VECTOR* v1);
extern int FieldScriptVMGetArgument(int index);

void func_800972F4(void) {
    D_800B00C0 = 1;
    g_FieldScriptVMCurActor->scriptInstructionPointer++;
}

void FieldScriptFadeOut(void) {
    int duration;
    
    FieldFadeInitializePrimitives(0);
    duration = FieldScriptVMGetArgument(1);
    FieldFadeToBlack(duration);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

void FieldScriptFadeIn(void) {
    int duration = FieldScriptVMGetArgument(1);
    FieldFadeToWhite(duration);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

void func_800973A4(void) {
    if (func_8001B484(FieldScriptVMGetInstructionArgument(2) & 0xFFFF, SCRIPT_READ_U8_REL(1)) == 0) {
        g_FieldScriptVMCurActor->scriptInstructionPointer += 4;
    }
}

void func_80097410(void) {
    g_FieldScriptVMCurActor->scriptInstructionPointer += FieldScriptVMGetArgument(1) * 3 + 3;
}

int FieldGetPlayerActorDirection(void) {
    int halfDirection = PSX_DEGREES(22.5);
    return (PSX_ANGLE_TO_DIRECTION_8(((ActorData*)(uintptr_t)g_FieldActors[g_PlayerActorIndex].pActorData)->rotation.vy + halfDirection) + 2) 
        & MASK_8DIR_MOVEMENT_NUM_DIRECTIONS;
}

void func_8009749C(void) {
    ActorData* actor = g_FieldScriptVMCurActor;
    ActorData* target;

    actor->scripts[actor->curScriptIndex].flags_0x17 = 2;

    target = (ActorData*)(uintptr_t)g_FieldActors[SCRIPT_READ_U8_REL(1)].pActorData;
    actor->unkD0.vx = target->position.vx >> 16;
    actor->unkD0.vz = target->position.vz >> 16;
    actor->unkD0.vy = target->position.vy >> 16;

    if (g_FieldScriptVMCurActor->scripts[g_FieldScriptVMCurActor->curScriptIndex].flags_0 == 0xFFFF) {
        g_FieldScriptVMCurActor->scripts[g_FieldScriptVMCurActor->curScriptIndex].flags_0 = FieldScriptVMGetArgument(5);
    }

    if (func_80097A50(FieldScriptVMGetArgument(5)) == 0) {
        g_FieldScriptVMCurActor->scriptInstructionPointer += 7;
    }
}

/* VM opcode 0x55: set the current script's movement target to another actor.
 * The target actor index is the instruction byte at +1; its integer position
 * becomes the current actor's movement target before func_80097A50 advances
 * the interpolation. */
void func_800975C0(void) {
    ActorData* actor = g_FieldScriptVMCurActor;
    ActorData* target;

    actor->scripts[actor->curScriptIndex].flags_0x17 = 2;

    target = (ActorData*)(uintptr_t)g_FieldActors[SCRIPT_READ_U8_REL(1)].pActorData;
    actor->unkD0.vx = target->position.vx >> 16;
    actor->unkD0.vz = target->position.vz >> 16;
    actor->unkD0.vy = target->position.vy >> 16;

    {
        ActorData* pActor = g_FieldScriptVMCurActor;
        s32 result;

        pActor->scripts[pActor->curScriptIndex].flags_0 = 0xFFFF;
        result = func_80097A50(0xFFFF);
        if (result == 0) {
            g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
        }
    }
}

/* Movement-mode variant: capture the current actor position once, retain the
 * script-supplied duration in its slot, then let func_80097A50 advance it. */
void func_800976A8(void) {
    if (g_FieldScriptVMCurActor->scripts[g_FieldScriptVMCurActor->curScriptIndex].flags_0x17 == 0) {
        g_FieldScriptVMCurActor->scripts[g_FieldScriptVMCurActor->curScriptIndex].flags_0x17 = 1;
        g_FieldScriptVMCurActor->unkD0.vx = g_FieldScriptVMCurActor->position.vx >> 16;
        g_FieldScriptVMCurActor->unkD0.vy = g_FieldScriptVMCurActor->position.vy >> 16;
        g_FieldScriptVMCurActor->unkD0.vz = g_FieldScriptVMCurActor->position.vz >> 16;
    }

    if (g_FieldScriptVMCurActor->scripts[g_FieldScriptVMCurActor->curScriptIndex].flags_0 == 0xFFFF) {
        g_FieldScriptVMCurActor->scripts[g_FieldScriptVMCurActor->curScriptIndex].flags_0 = FieldScriptVMGetArgument(8);
    }

    if (func_80097A50(FieldScriptVMGetArgument(8)) == 0) {
        g_FieldScriptVMCurActor->scriptInstructionPointer += 10;
    }
}

/* Capture current integer position into unkD0 once, force duration 0xFFFF,
 * then wait via func_80097A50 (mode flags_0x17 = 1). */
void func_800977A4(void) {
    ActorData* actor = g_FieldScriptVMCurActor;

    if (actor->scripts[actor->curScriptIndex].flags_0x17 == 0) {
        actor->scripts[actor->curScriptIndex].flags_0x17 = 1;
        actor->unkD0.vx = actor->position.vx >> 16;
        actor->unkD0.vy = actor->position.vy >> 16;
        actor->unkD0.vz = actor->position.vz >> 16;
    }

    {
        ActorData* pActor = g_FieldScriptVMCurActor;
        s32 result;

        pActor->scripts[pActor->curScriptIndex].flags_0 = 0xFFFF;
        result = func_80097A50(0xFFFF);
        if (result == 0) {
            g_FieldScriptVMCurActor->scriptInstructionPointer += 8;
        }
    }
}

/* Map001-reachable walk wait (opcode 0x46): mode flags_0x17 = 3, duration from
 * arg(5), IP += 8 when func_80097A50 reports done. */
void func_80097864(void) {
    ActorData* actor = g_FieldScriptVMCurActor;

    if (actor->scripts[actor->curScriptIndex].flags_0x17 == 0) {
        actor->scripts[actor->curScriptIndex].flags_0x17 = 3;
        actor->unkD0.vx = actor->position.vx >> 16;
        actor->unkD0.vy = actor->position.vy >> 16;
        actor->unkD0.vz = actor->position.vz >> 16;
    }

    if (g_FieldScriptVMCurActor->scripts[g_FieldScriptVMCurActor->curScriptIndex].flags_0 == 0xFFFF) {
        g_FieldScriptVMCurActor->scripts[g_FieldScriptVMCurActor->curScriptIndex].flags_0 =
            FieldScriptVMGetArgument(5);
    }

    if (func_80097A50(FieldScriptVMGetArgument(5)) == 0) {
        g_FieldScriptVMCurActor->scriptInstructionPointer += 8;
    }
}

/* Duration-from-arg(8) wait wrapper; IP += 0xA on completion. */
void func_80097954(void) {
    if (g_FieldScriptVMCurActor->scripts[g_FieldScriptVMCurActor->curScriptIndex].flags_0 == 0xFFFF) {
        g_FieldScriptVMCurActor->scripts[g_FieldScriptVMCurActor->curScriptIndex].flags_0 = FieldScriptVMGetArgument(8);
    }

    if (func_80097A50(FieldScriptVMGetArgument(8)) == 0) {
        g_FieldScriptVMCurActor->scriptInstructionPointer += 0xA;
    }
}

void func_800979F0(void) {
    ActorData* pActor = g_FieldScriptVMCurActor;
    s32 result;
    pActor->scripts[pActor->curScriptIndex].flags_0 = 0xFFFF;
    result = func_80097A50(0xFFFF);
    if (result == 0) {
        g_FieldScriptVMCurActor->scriptInstructionPointer += 8;
    }
}

/* Transcribed from asm/field/nonmatchings/main/misc7/func_80097A50.s
 * (0x80097A50-0x80098034). Mode is bits 23-24 of the flags word at
 * actor+0x90+curScriptIndex*8 (not actor->scripts[] at the 0x90 label).
 * Countdown is the low half of that word. move.vy is stored then forced 0. */
#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/field/nonmatchings/main/misc7", func_80097A50);
#else
s32 func_80097A50(s32 targetValue) {
    FieldActor* refFieldActor;
    ActorData* refActorData;
    u8* refSpriteData;
    ActorData* actor;
    u8* actorBytes;
    u32* pFlagsWord;
    s32 mode;
    u16 countdown;
    u16 speed;
    s32 targetX;
    s32 targetY;
    s32 targetZ;
    s32 combinedSolidRange;
    s32 selfX;
    s32 selfY;
    s32 selfZ;
    s32 stepMagnitude;
    s32 distance;
    s32 hasMove;
    s32 speedShifted;
    s32 finalAngle;
    VECTOR fromTarget;
    VECTOR normal;
    VECTOR moveVec;

#ifdef NPC_EVENT_MUTANT_ALWAYS_DONE
    return 0;
#endif
#ifdef NPC_EVENT_MUTANT_NO_IP
    D_800B00C0 = 1;
    return -1;
#endif

    refFieldActor = (FieldActor*)((u8*)g_FieldActors + D_800AFD1C * 0x5C);
    refActorData = (ActorData*)(uintptr_t)refFieldActor->pActorData;
    refSpriteData = (u8*)(uintptr_t)refFieldActor->pSpriteData;
    actor = g_FieldScriptVMCurActor;
    actorBytes = (u8*)actor;
    targetX = 0;
    targetY = 0;
    targetZ = 0;
    combinedSolidRange = 0;
    hasMove = -1;

    speed = *(u16*)(actorBytes + 0x76);
    if (refActorData->flags & 0x2000) {
        *(s32*)(refSpriteData + 0x18) = 0x08000000 / speed;
    } else {
        *(s32*)(refSpriteData + 0x18) = 0x04000000 / speed;
    }
    stepMagnitude = FieldGetVec1Magnitude(*(s32*)(refSpriteData + 0x18) >> 15) + 1;

    pFlagsWord = (u32*)(actorBytes + 0x90 + actorBytes[0xCE] * 8);
    mode = (s32)((*pFlagsWord >> 23) & 3);
    switch (mode) {
    case 0:
        targetX = FieldScriptArgument1(1, SCRIPT_READ_U8_REL(5));
        targetZ = FieldScriptArgument2(3, SCRIPT_READ_U8_REL(5));
        targetY = FieldScriptArgument3(6, SCRIPT_READ_U8_REL(5));
        break;

    case 1:
        targetX = FieldScriptArgument1(1, SCRIPT_READ_U8_REL(5)) + actor->unkD0.vx;
        targetZ = FieldScriptArgument2(3, SCRIPT_READ_U8_REL(5)) + actor->unkD0.vz;
        targetY = FieldScriptArgument3(6, SCRIPT_READ_U8_REL(5)) + actor->unkD0.vy;
        break;

    case 2: {
        u32 otherIdx = FieldScriptVMGetActorIndex(1);
        ActorData* other;

        if (otherIdx == 0xFF) {
            return 0;
        }
        otherIdx = FieldScriptVMGetActorIndex(1);
        other = (ActorData*)(uintptr_t)((FieldActor*)((u8*)g_FieldActors + otherIdx * 0x5C))->pActorData;
        combinedSolidRange = FieldGetVec1Magnitude(other->solidRange + actor->solidRange);
        targetX = actor->unkD0.vx;
        targetZ = actor->unkD0.vz;
        targetY = FieldScriptArgument1(2, SCRIPT_READ_U8_REL(4));
        break;
    }

    case 3: {
        s32 angle = FieldScriptVMGetArgument(1) & 0xFFF;

        targetX = (actor->unkD0.vx + (rsin(angle) << 5)) >> 12;
        targetZ = actor->unkD0.vz + ((-(rcos(angle) << 5)) >> 12);
        targetY = actor->unkD0.vy + FieldScriptArgument1(3, SCRIPT_READ_U8_REL(7));
        break;
    }

    default:
        break;
    }

    selfX = *(s16*)(actorBytes + 0x22);
    selfY = *(s16*)(actorBytes + 0x26);
    selfZ = *(s16*)(actorBytes + 0x2A);

    fromTarget.vx = selfX - targetX;
    fromTarget.vy = selfY - targetY;
    fromTarget.vz = selfZ - targetZ;
    VectorNormal(&fromTarget, &normal);

    speedShifted = *(s32*)(refSpriteData + 0x18) >> 8;
    moveVec.vx = -((normal.vx * speedShifted) >> 4);
    moveVec.vy = -((normal.vy * speedShifted) >> 4);
    moveVec.vz = -((normal.vz * speedShifted) >> 4);

    actor->move.vx = moveVec.vx;
    actor->move.vy = moveVec.vy;
#ifndef FIELD_97A50_MUTANT_KEEP_MOVE_Y
    actor->move.vy = 0;
#endif
    actor->move.vz = moveVec.vz;

    distance = FieldGetVec3Magnitude(targetX - selfX, targetY - selfY, targetZ - selfZ);

    if (*(s16*)(actorBytes + 0x42) == 0 && *(s16*)(actorBytes + 0x4A) == 0) {
        hasMove = 0;
    }

    actor->scriptFlags.flags |= 0x400000;

    pFlagsWord = (u32*)(actorBytes + 0x90 + actorBytes[0xCE] * 8);
    countdown = *(u16*)pFlagsWord;
    if (countdown != 0 && stepMagnitude + combinedSolidRange < distance) {
        if (hasMove == -1) {
            finalAngle = (s16)func_8007B694((s32*)&moveVec);
            finalAngle |= 0x8000;
            actor->rotation.vx = finalAngle;
            actor->rotation.vy = finalAngle;
        }
        actor->unkEC = (s16)((actor->position.vy + moveVec.vy) >> 16);
        actor->scriptFlags.flags |= 0x40000;
        D_800B00C0 = 1;
        *(u16*)pFlagsWord = (u16)(countdown - 1);
        return -1;
    }

    if (hasMove == -1) {
        if (targetValue != 0) {
            if ((actor->scriptFlags.flags & 0x8000) == 0) {
                finalAngle = actor->rotation.vy | 0x8000;
            } else {
                finalAngle = actor->unk11C | 0x8000;
            }
        } else {
            finalAngle = (s16)func_8007B694((s32*)&moveVec);
            finalAngle |= 0x8000;
        }
        actor->rotation.vx = finalAngle;
        actor->rotation.vy = finalAngle;
    }

    actor->unkEC = (s16)((actor->position.vy + moveVec.vy) >> 16);
    pFlagsWord = (u32*)(actorBytes + 0x90 + actorBytes[0xCE] * 8);
    *pFlagsWord &= 0xFE7FFFFFu;
    *(u16*)pFlagsWord = 0xFFFF;
    return 0;
}
#endif /* XENO_PC_PORT */

/* asm 80098038-800980F8, opcode 0x53. Unlike 0x52, the caller supplies the
 * countdown seed and the useStoredAngle flag as an explicit script argument
 * rather than the fixed 0xFFFF sentinel. */
void func_80098038(void) {
    g_FieldScriptVMCurActor->scripts[g_FieldScriptVMCurActor->curScriptIndex].flags_0x17 = 2;

    if (g_FieldScriptVMCurActor->scripts[g_FieldScriptVMCurActor->curScriptIndex].flags_0 == 0xFFFF) {
        g_FieldScriptVMCurActor->scripts[g_FieldScriptVMCurActor->curScriptIndex].flags_0 = FieldScriptVMGetArgument(2);
    }

    if (func_80099AC0(FieldScriptVMGetArgument(2)) == 0) {
        g_FieldScriptVMCurActor->scriptInstructionPointer += 4;
    }
}

/* asm 800980FC-80098180, opcode 0x52. Thin wrapper: always (re)start the
 * per-script movement countdown at the 0xFFFF sentinel and wait via
 * func_80099AC0(0xFFFF) -- do not implement against the stub, which returns
 * 0 unconditionally and would fake instant movement completion. */
void func_800980FC(void) {
    ActorData* pActor = g_FieldScriptVMCurActor;
    s32 result;

    pActor->scripts[pActor->curScriptIndex].flags_0x17 = 2;
    pActor->scripts[pActor->curScriptIndex].flags_0 = 0xFFFF;
    result = func_80099AC0(0xFFFF);
    if (result == 0) {
        g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
    }
}

void func_80098184(void) {
    ActorData* actor = g_FieldScriptVMCurActor;

    if (actor->scripts[actor->curScriptIndex].flags_0x17 == 0) {
        actor->scripts[actor->curScriptIndex].flags_0x17 = 3;
        actor->unkD0.vx = actor->position.vx >> 16;
        actor->unkD0.vy = actor->position.vy >> 16;
        actor->unkD0.vz = actor->position.vz >> 16;
    }

    if (g_FieldScriptVMCurActor->scripts[g_FieldScriptVMCurActor->curScriptIndex].flags_0 == 0xFFFF) {
        g_FieldScriptVMCurActor->scripts[g_FieldScriptVMCurActor->curScriptIndex].flags_0 = FieldScriptVMGetArgument(3);
    }

    if (func_80099AC0(FieldScriptVMGetArgument(3)) == 0) {
        g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
    }
}

void func_80098274(void) {
    ActorData* actor = g_FieldScriptVMCurActor;

    if (actor->scripts[actor->curScriptIndex].flags_0x17 == 0) {
        actor->scripts[actor->curScriptIndex].flags_0x17 = 1;
        actor->unkD0.vx = actor->position.vx >> 16;
        actor->unkD0.vy = actor->position.vy >> 16;
        actor->unkD0.vz = actor->position.vz >> 16;
    }

    if (g_FieldScriptVMCurActor->scripts[g_FieldScriptVMCurActor->curScriptIndex].flags_0 == 0xFFFF) {
        g_FieldScriptVMCurActor->scripts[g_FieldScriptVMCurActor->curScriptIndex].flags_0 = FieldScriptVMGetArgument(6);
    }

    if (func_80099AC0(FieldScriptVMGetArgument(6)) == 0) {
        g_FieldScriptVMCurActor->scriptInstructionPointer += 8;
    }
}

void func_80098370(void) {
    ActorData* actor = g_FieldScriptVMCurActor;

    if (actor->scripts[actor->curScriptIndex].flags_0x17 == 0) {
        actor->scripts[actor->curScriptIndex].flags_0x17 = 1;
        actor->unkD0.vx = actor->position.vx >> 16;
        actor->unkD0.vy = actor->position.vy >> 16;
        actor->unkD0.vz = actor->position.vz >> 16;
    }

    {
        ActorData* pActor = g_FieldScriptVMCurActor;
        s32 result;

        pActor->scripts[pActor->curScriptIndex].flags_0 = 0xFFFF;
        result = func_80099AC0(0xFFFF);
        if (result == 0) {
            g_FieldScriptVMCurActor->scriptInstructionPointer += 6;
        }
    }
}

/* asm 80098430-800984E8, opcode 0x4B. Transcribed from
 * asm/field/nonmatchings/main/misc7/func_80098430.s:
 *   lw 0x90(actor+idx*8); and 0xFE7FFFFF; sw (0x80098450-0x8009845C)
 *   lhu 0x90; if == 0xFFFF, jal FieldScriptVMGetArgument(6) and sh back
 *     after reloading g_FieldScriptVMCurActor (0x80098470-0x800984A4)
 *   always jal GetArgument(6) then func_80099AC0; on 0, lhu 0xCC += 8
 *     (0x800984A8-0x800984D8). */
void func_80098430(void) {
    ActorData* actor = g_FieldScriptVMCurActor;

#ifndef FIELD_98430_MUTANT_SKIP_MASK
    actor->scripts[actor->curScriptIndex].flags_0x17 = 0;
#endif
    if (actor->scripts[actor->curScriptIndex].flags_0 == 0xFFFF) {
        g_FieldScriptVMCurActor->scripts[g_FieldScriptVMCurActor->curScriptIndex].flags_0 = FieldScriptVMGetArgument(6);
    }

    if (func_80099AC0(FieldScriptVMGetArgument(6)) == 0) {
        g_FieldScriptVMCurActor->scriptInstructionPointer += 8;
    }
}

extern s16 D_800B21AE;
extern s16 D_800B21B0;
extern s16 D_800B21B2;
extern u8 D_800B21D2;

#ifndef XENO_PC_PORT
extern s32 g_FieldScriptVMCurScriptData_s32 asm("g_FieldScriptVMCurScriptData");
#endif
void func_800984EC(void) {
#ifdef XENO_PC_PORT
    u8 mask1 = SCRIPT_READ_U8_REL(7);
    D_800B21AE = (s16)FieldScriptArgument1(1, mask1);
    mask1 = SCRIPT_READ_U8_REL(7);
    D_800B21B0 = (s16)FieldScriptArgument2(3, mask1);
    mask1 = SCRIPT_READ_U8_REL(7);
    D_800B21B2 = (s16)FieldScriptArgument3(5, mask1);
    D_800B21D2 |= 0x80;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 8;
#else
    /* Raw-offset actor reads: a struct-field access lets gcc hoist the final
     * load above the D_800B21D2 store, which retail does not do. */
    D_800B21AE = FieldScriptArgument1(1, *(u8*)(*(u16*)((u8*)g_FieldScriptVMCurActor + 0xCC) + g_FieldScriptVMCurScriptData_s32 + 7));
    D_800B21B0 = FieldScriptArgument2(3, *(u8*)(*(u16*)((u8*)g_FieldScriptVMCurActor + 0xCC) + g_FieldScriptVMCurScriptData_s32 + 7));
    D_800B21B2 = FieldScriptArgument3(5, *(u8*)(*(u16*)((u8*)g_FieldScriptVMCurActor + 0xCC) + g_FieldScriptVMCurScriptData_s32 + 7));
    D_800B21D2 |= 0x80;
    *(u16*)((u8*)g_FieldScriptVMCurActor + 0xCC) = *(u16*)((u8*)g_FieldScriptVMCurActor + 0xCC) + 8;
#endif
}

extern u8 D_8006FD1C[];

void func_800985BC(void) {
    if (g_FieldSystemMode == 0) {
        s32 v = FieldScriptVMGetVariableValue(FieldScriptVMGetInstructionArgument(1) & 0xFFFF);
        func_800379C8(D_8006FD1C, v, v);
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

void func_8009861C(void) {
#ifdef XENO_PC_PORT
    u8 mask;
    s32 arg2, arg3, arg4, arg5, addr;
    s32 dx, dz, dist;
    mask = SCRIPT_READ_U8_REL(0xB);
    arg2 = FieldScriptArgument2(3, mask);
    mask = SCRIPT_READ_U8_REL(0xB);
    arg3 = FieldScriptArgument3(5, mask);
    mask = SCRIPT_READ_U8_REL(0xB);
    arg4 = FieldScriptArgument4(7, mask);
    mask = SCRIPT_READ_U8_REL(0xB);
    arg5 = FieldScriptArgument5(9, mask);
    addr = FieldScriptVMGetInstructionArgument(1);
    dx = arg5 - arg2;
    dz = arg4 - arg3;
    dist = FieldGetVec2Magnitude(dx, dz);
    FieldScriptMemoryWriteU16((u16)addr, (u16)dist);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 0xC;
#else
    s32 x1 = FieldScriptArgument2(3, SCRIPT_READ_U8_REL_RAW(0xB));
    s32 z1 = FieldScriptArgument3(5, SCRIPT_READ_U8_REL_RAW(0xB));
    s32 x2 = FieldScriptArgument4(7, SCRIPT_READ_U8_REL_RAW(0xB));
    s32 z2 = FieldScriptArgument5(9, SCRIPT_READ_U8_REL_RAW(0xB));
    s32 addr = SCRIPT_IMM_ARG(1);
    FieldScriptMemoryWriteU16(addr, FieldGetVec2Magnitude(x2 - x1, z2 - z1));
    g_FieldScriptVMCurActor->scriptInstructionPointer += 0xC;
#endif
}

void func_80098738(void) {
#ifdef XENO_PC_PORT
    u8 mask;
    s32 a2, a3, a4, a5, a6, a7, addr, dist;
    mask = SCRIPT_READ_U8_REL(0xF);
    a2 = FieldScriptArgument2(3, mask);
    mask = SCRIPT_READ_U8_REL(0xF);
    a3 = FieldScriptArgument3(5, mask);
    mask = SCRIPT_READ_U8_REL(0xF);
    a4 = FieldScriptArgument3(7, mask);
    mask = SCRIPT_READ_U8_REL(0xF);
    a5 = FieldScriptArgument4(9, mask);
    mask = SCRIPT_READ_U8_REL(0xF);
    a6 = FieldScriptArgument5(0xB, mask);
    mask = SCRIPT_READ_U8_REL(0xF);
    a7 = FieldScriptArgument5(0xD, mask);
    addr = FieldScriptVMGetInstructionArgument(1);
    dist = FieldGetVec3Magnitude(a6 - a2, a7 - a3, a5 - a4);
    FieldScriptMemoryWriteU16((u16)addr, (u16)dist);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 0x10;
#else
    s32 x1 = FieldScriptArgument2(3, SCRIPT_READ_U8_REL_RAW(0xF));
    s32 y1 = FieldScriptArgument3(5, SCRIPT_READ_U8_REL_RAW(0xF));
    s32 z1 = FieldScriptArgument3(7, SCRIPT_READ_U8_REL_RAW(0xF));
    s32 x2 = FieldScriptArgument4(9, SCRIPT_READ_U8_REL_RAW(0xF));
    s32 y2 = FieldScriptArgument5(0xB, SCRIPT_READ_U8_REL_RAW(0xF));
    s32 z2 = FieldScriptArgument5(0xD, SCRIPT_READ_U8_REL_RAW(0xF));
    s32 addr = SCRIPT_IMM_ARG(1);
    FieldScriptMemoryWriteU16(addr, FieldGetVec3Magnitude(x2 - x1, z2 - z1, y2 - y1));
    g_FieldScriptVMCurActor->scriptInstructionPointer += 0x10;
#endif
}

extern s32 FieldMathInterpolateAngle(s32, s32, s32);

void func_800988B8(void) {
    u8 mask2 = SCRIPT_READ_U8_REL(9);
    s32 arg2 = FieldScriptArgument2(3, mask2);
    u8 mask3 = SCRIPT_READ_U8_REL(9);
    s32 arg3 = FieldScriptArgument3(5, mask3);
    u8 mask4 = SCRIPT_READ_U8_REL(9);
    s32 arg4 = FieldScriptArgument4(7, mask4);
    s32 result = FieldMathInterpolateAngle(arg2, arg3, arg4);
    u16 addr = (u16)FieldScriptVMGetInstructionArgument(1);
    FieldScriptMemoryWriteU16(addr, result);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 0xA;
}

void func_8009899C(void) {
    FieldScriptMemoryWriteU16(SCRIPT_IMM_ARG(1), g_FieldScriptVMCurActor->rotation.vy & 0xFFF);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

void func_800989F0(void) {
    s32 actorIdx = FieldScriptVMGetActorIndex(1);
    if (actorIdx != 0xFF) {
        ActorData* pModel =
            (ActorData*)(uintptr_t)g_FieldActors[actorIdx].pActorData;
        u16 addr = (u16)FieldScriptVMGetInstructionArgument(2);
        u16 value = *(u16*)((u8*)pModel + 0x106) & 0xFFF;
        FieldScriptMemoryWriteU16(addr, value);
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 4;
}

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/field/nonmatchings/main/misc7", func_80098A7C);
#else
void func_80098A7C(void) {
    /* Instruction-faithful to retail 80098A7C-80098BFC: the sprite pointer is
     * latched before the three argument calls, the field-actor slot is
     * re-derived from D_800AFD1C afterwards, the arg mask byte is re-read
     * per call, and `x << 16` wraps as u32 (retail sll). */
    ActorData* actor;
    FieldActor* pFieldActor;
    SpriteData* pSpriteData;
    s32 value;

    actor = g_FieldScriptVMCurActor;
    pSpriteData = (SpriteData*)(uintptr_t)g_FieldActors[D_800AFD1C].pSpriteData;
    actor->scriptFlags.flags |= 0x10000;
    actor->flags |= 0x200000;

    value = FieldScriptArgument1(1, SCRIPT_READ_U8_REL(7));
    g_FieldScriptVMCurActor->position.vx = (s32)((u32)value << 16);
    value = FieldScriptArgument2(3, SCRIPT_READ_U8_REL(7));
    g_FieldScriptVMCurActor->position.vz = (s32)((u32)value << 16);
    value = FieldScriptArgument3(5, SCRIPT_READ_U8_REL(7));

    actor = g_FieldScriptVMCurActor;
    actor->position.vy = (s32)((u32)value << 16);
    pFieldActor = &g_FieldActors[D_800AFD1C];
    pFieldActor->transformMatrix.t[0] = (s16)(actor->position.vx >> 16);
    pFieldActor->transformMatrix.t[1] = (s16)(actor->position.vy >> 16);
    pFieldActor->transformMatrix.t[2] = (s16)(actor->position.vz >> 16);
    pSpriteData->position.x = actor->position.vx;
    pSpriteData->position.y = actor->position.vy;
    pSpriteData->position.z = actor->position.vz;
    actor->scriptInstructionPointer += 8;
}
#endif /* XENO_PC_PORT */

void func_80098C00(void) {
    g_FieldScriptVMCurActor->scripts[g_FieldScriptVMCurActor->curScriptIndex].flags_0 = 0xFFFF;
    func_80098CAC(0);
}

void func_80098C3C(void) {
    if (g_FieldScriptVMCurActor->scripts[g_FieldScriptVMCurActor->curScriptIndex].flags_0 == 0xFFFF) {
        g_FieldScriptVMCurActor->scripts[g_FieldScriptVMCurActor->curScriptIndex].flags_0 = FieldScriptVMGetArgument(0xB);
    }
    func_80098CAC(1);
}

extern s32 D_800AFD1C;
extern s32 D_800B00C0;

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/field/nonmatchings/main/misc7", func_80098CAC);
#else
extern void func_800821F4(void* pSpriteData, s16 animIndex, void* pFieldActor);
extern void func_80081F80(void* pSpriteData, s16 angle, void* pFieldActor);

void func_80098CAC(s32 arg0) {
    /* Instruction-faithful to retail 80098CAC-80099210.  Retail `div` has no
     * break guard: moveSpeed == 0 and the INT_MIN / -1 cases use the R3000
     * results (MIPS_DIV); position/step arithmetic wraps as u32. */
    FieldActor* pFieldActor;
    SpriteData* pSprite;
    ActorData* actor;
    u8* script;
    u16 ip;
    s32 speed;
    s32 animationId;
    s32 x, y, z;
    s32 quotient;
    s32 dvx, dvy, dvz;
    s32 prevX, prevY, prevZ;
    s32 value;

    pFieldActor = &g_FieldActors[D_800AFD1C];
    pSprite = (SpriteData*)(uintptr_t)pFieldActor->pSpriteData;
    if (((ActorData*)(uintptr_t)pFieldActor->pActorData)->flags & 0x2000) {
        speed = MIPS_DIV(0x8000000, g_FieldScriptVMCurActor->moveSpeed) >> 16;
    } else {
        speed = MIPS_DIV(0x4000000, g_FieldScriptVMCurActor->moveSpeed) >> 16;
    }
    if (speed == 0) {
        speed = 1;
    }

    actor = g_FieldScriptVMCurActor;
    script = (u8*)g_FieldScriptVMCurScriptData;
    ip = actor->scriptInstructionPointer;
    actor->scriptFlags.flags |= 0x10000;
    animationId = 1;
    if (script[ip + 1] == 0) {
        x = (s32)((u32)FieldScriptArgument1(2, script[ip + 8]) << 16);
        z = (s32)((u32)FieldScriptArgument2(4, SCRIPT_READ_U8_REL(8)) << 16);
        y = (s32)((u32)FieldScriptArgument3(6, SCRIPT_READ_U8_REL(8)) << 16);
        actor = g_FieldScriptVMCurActor;
        quotient = MIPS_DIV((s32)FieldGetVec3Magnitude(
                                (s32)((u32)x - (u32)actor->position.vx) >> 16,
                                (s32)((u32)y - (u32)actor->position.vy) >> 16,
                                (s32)((u32)z - (u32)actor->position.vz) >> 16),
                            speed);
        actor = g_FieldScriptVMCurActor;
        actor->unk102 = (s16)quotient;
        if ((s16)quotient == 0) {
            actor->unk102 = (s16)((u32)quotient + 1);
        }

        actor = g_FieldScriptVMCurActor;
        dvx = MIPS_DIV((s32)((u32)x - (u32)actor->position.vx), actor->unk102);
        dvy = MIPS_DIV((s32)((u32)y - (u32)actor->position.vy), actor->unk102);
        dvz = MIPS_DIV((s32)((u32)z - (u32)actor->position.vz), actor->unk102);
        actor->unkD0.vy = dvy;
        actor->unkD0.vx = dvx;
        actor->unkD0.vz = dvz;
        if ((x >> 16) != (s16)(actor->position.vx >> 16) ||
            (z >> 16) != (s16)(actor->position.vz >> 16)) {
            value = ratan2(dvz >> 16, (s16)(actor->unkD0.vx >> 16));
            g_FieldScriptVMCurActor->rotation.vx = (s16)(0u - (u32)value);
            g_FieldScriptVMCurActor->rotation.vy = (s16)(0u - (u32)value);
        }
        g_FieldScriptVMCurActor->scriptInstructionPointer += 9;
    } else {
        if (actor->unk102 <= 0 || actor->scripts[actor->curScriptIndex].flags_0 == 0) {
            actor->scriptInstructionPointer = (u16)(ip - 9);
            if (actor->scripts[actor->curScriptIndex].flags_0 != 0) {
                prevX = actor->position.vx;
                prevY = actor->position.vy;
                prevZ = actor->position.vz;
                value = FieldScriptArgument1(2, SCRIPT_READ_U8_REL(8));
                g_FieldScriptVMCurActor->position.vx = (s32)((u32)value << 16);
                value = FieldScriptArgument2(4, SCRIPT_READ_U8_REL(8));
                g_FieldScriptVMCurActor->position.vz = (s32)((u32)value << 16);
                value = FieldScriptArgument3(6, SCRIPT_READ_U8_REL(8));
                actor = g_FieldScriptVMCurActor;
                actor->position.vy = (s32)((u32)value << 16);
                actor->moveModified.vx = (s32)((u32)actor->position.vx - (u32)prevX);
                actor->moveModified.vy = (s32)((u32)actor->position.vy - (u32)prevY);
                actor->moveModified.vz = (s32)((u32)actor->position.vz - (u32)prevZ);
            }
            actor = g_FieldScriptVMCurActor;
            animationId = actor->defaultAnimationId;
            if (arg0 == 0) {
                actor->scriptInstructionPointer += 0xB;
            } else {
                actor->scriptInstructionPointer += 0xD;
            }
            actor = g_FieldScriptVMCurActor;
            actor->scripts[actor->curScriptIndex].flags_0 = 0xFFFF;
        } else {
            actor->position.vx = (s32)((u32)actor->position.vx + (u32)actor->unkD0.vx);
            actor->position.vz = (s32)((u32)actor->position.vz + (u32)actor->unkD0.vz);
            actor->position.vy = (s32)((u32)actor->position.vy + (u32)actor->unkD0.vy);
            actor->moveModified.vy = actor->unkD0.vy;
            actor->moveModified.vz = actor->unkD0.vz;
            actor->moveModified.vx = actor->unkD0.vx;
            D_800B00C0 = 1;
            actor->scripts[actor->curScriptIndex].flags_0--;
        }

        actor = g_FieldScriptVMCurActor;
        actor->unk102 = (s16)((u16)actor->unk102 - 1);
        pFieldActor = &g_FieldActors[D_800AFD1C];
        pFieldActor->transformMatrix.t[0] = (s16)(actor->position.vx >> 16);
        pFieldActor->transformMatrix.t[1] = (s16)(actor->position.vy >> 16);
        pFieldActor->transformMatrix.t[2] = (s16)(actor->position.vz >> 16);
        pSprite->position.x = actor->position.vx;
        pSprite->position.y = actor->position.vy;
        pSprite->position.z = actor->position.vz;
    }

    actor = g_FieldScriptVMCurActor;
    if (actor->unkAnimationId != 0xFF) {
        animationId = actor->unkAnimationId;
    }
    if (actor->curAnimationId != animationId && !(actor->scriptFlags.flags & 0x2000000)) {
        actor->curAnimationId = animationId;
        func_800821F4(pSprite, animationId, D_800B06B8);
    }
    func_80081F80(pSprite, g_FieldScriptVMCurActor->rotation.vx, D_800B06B8);
}
#endif /* XENO_PC_PORT */

/*
 * asm 80099214-8009997C, opcode 0x57. Actor jump/lerp with gravity (not a
 * camera opcode). Mode from SCRIPT_READ_U8_REL(1) & 3:
 *   0: init from absolute x/z/y + step count (arg4); bit 0x80 resolves Y via
 *      walkmesh (func_8007B1C4).
 *   1: init from absolute x/z + speed (arg4) → step count via XZ magnitude.
 *   2: arc/jump: derive step count from gravity + Y delta via SquareRoot0.
 *   3: if byte1==0xF, refresh walkmesh tris and clear 0x10000/0x200000;
 *      else tick: advance by unkD0 + sprite step.vy/gravity until unk102
 *      reaches unkE0, then rewind IP by 0xB to re-read init args, snap, and
 *      skip init+continue (IP += 0xD). Always yields via D_800B00C0=1.
 */
#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/field/nonmatchings/main/misc7", func_80099214);
#else
void func_80099214(void) {
    /* Instruction-faithful to retail 80099214-8009997C.  Retail `div` has no
     * break guard (step count 0 / -1 and INT_MIN / -1 use the R3000 results
     * via MIPS_DIV); `mult` keeps the low word; `x << 16`, position and step
     * arithmetic wrap as u32.  out/state mirror the retail stack rows
     * (sp+0x58 stride 8, sp+0x18 stride 16) handed to func_8007B1C4. */
    ActorData* actor;
    FieldActor* pFieldActor;
    u8* sprite;
    u8* script;
    u16 ip;
    u8 modeByte;
    s32 mode;
    s32 x, y, z;
    s32 stepCount;
    s32 walkmeshId;
    s32 negDuration;
    s32 yDelta;
    s32 dx, dz;
    s32 value;
    s32 i;
    s16 out[4][4];
    s32 state[4][4];

    actor = g_FieldScriptVMCurActor;
    script = (u8*)g_FieldScriptVMCurScriptData;
    ip = actor->scriptInstructionPointer;
    sprite = (u8*)(uintptr_t)g_FieldActors[D_800AFD1C].pSpriteData;
    actor->scriptFlags.flags |= 0x10000;
    modeByte = script[ip + 1];
    mode = modeByte & 3;

    if (mode == 1) {
        x = FieldScriptArgument1(2, script[ip + 0xA]);
        z = FieldScriptArgument2(4, SCRIPT_READ_U8_REL(0xA));
        actor = g_FieldScriptVMCurActor;
        dx = (s32)(((u32)x << 16) - (u32)actor->position.vx);
        dz = (s32)(((u32)z << 16) - (u32)actor->position.vz);
        stepCount = FieldScriptArgument4(8, SCRIPT_READ_U8_REL(0xA));
        stepCount = MIPS_DIV((s32)FieldGetVec2Magnitude(dx >> 16, dz >> 16), stepCount);
    } else if (mode == 0) {
        stepCount = FieldScriptArgument4(8, script[ip + 0xA]);
    } else if (mode == 2) {
        FieldScriptArgument1(2, script[ip + 0xA]);
        FieldScriptArgument2(4, SCRIPT_READ_U8_REL(0xA));
        y = FieldScriptArgument3(6, SCRIPT_READ_U8_REL(0xA));
        negDuration = (s32)(0u - (u32)FieldScriptArgument4(8, SCRIPT_READ_U8_REL(0xA)));
        value = *(s16*)(sprite + 0x1E);
        yDelta = (s32)(((u32)y << 16) - (u32)g_FieldScriptVMCurActor->position.vy);
        value = SquareRoot0((s32)((u32)value * ((u32)negDuration << 1)));
        *(s32*)(sprite + 0x10) = (s32)(0u - ((u32)value << 16));
        SquareRoot0(negDuration);
        value = (s32)((u32)negDuration - (u32)(yDelta >> 16));
        if (value < 0) {
            value = (s32)(0u - (u32)value);
        }
        stepCount = SquareRoot0(value);
        if (stepCount < 0) {
            stepCount = (s32)(0u - (u32)stepCount);
        }
    } else {
        if (modeByte == 0xF) {
            /* Cleanup: refresh walkmesh triangle ids (loop bound re-read). */
            for (i = 0; i < D_800AFB54 - 1; i++) {
                actor = g_FieldScriptVMCurActor;
                value = func_8007B1C4((s16)(actor->position.vx >> 16), (s16)(actor->position.vz >> 16),
                                      i, out[i], state[i]);
                g_FieldScriptVMCurActor->walkmeshTriIds[i] = (u16)value;
            }
            actor = g_FieldScriptVMCurActor;
            actor->scriptFlags.flags &= 0xFFFEFFFF;
            actor->flags &= 0xFFDFFFFF;
            actor->scriptInstructionPointer += 2;
            D_800B00C0 = 1;
            return;
        }

        if (actor->unk102 < (s16)actor->unkE0) {
            actor->position.vx = (s32)((u32)actor->position.vx + (u32)actor->unkD0.vx);
            actor->position.vz = (s32)((u32)actor->position.vz + (u32)actor->unkD0.vz);
            actor->position.vy = (s32)((u32)actor->position.vy + *(u32*)(sprite + 0x10));
            *(u32*)(sprite + 0x10) = *(u32*)(sprite + 0x10) + *(u32*)(sprite + 0x1C);
            if (actor->unkD0.vx != 0 || actor->unkD0.vz != 0) {
                if (!(actor->scriptFlags.flags & 0x8000)) {
                    value = func_8007B694((s32*)&actor->unkD0) | 0x8000;
                    g_FieldScriptVMCurActor->rotation.vx = (s16)value;
                    g_FieldScriptVMCurActor->rotation.vy = (s16)value;
                }
            }
        } else {
            /* Rewind to the paired init opcode to re-read the snap target. */
            actor->scriptInstructionPointer = (u16)(ip - 0xB);
            x = FieldScriptArgument1(2, script[(u16)(ip - 0xB) + 0xA]);
            z = FieldScriptArgument2(4, SCRIPT_READ_U8_REL(0xA));
            if (SCRIPT_READ_U8_REL(1) & 0x80) {
                walkmeshId = FieldScriptArgument3(6, SCRIPT_READ_U8_REL(0xA));
                value = func_8007B1C4(x, z, walkmeshId, out[walkmeshId], state[walkmeshId]);
                g_FieldScriptVMCurActor->walkmeshTriIds[walkmeshId] = (u16)value;
                y = out[walkmeshId][1];
            } else {
                y = FieldScriptArgument3(6, SCRIPT_READ_U8_REL(0xA));
            }
            actor = g_FieldScriptVMCurActor;
            *(s32*)(sprite + 0x10) = 0;
            actor->position.vx = (s32)((u32)x << 16);
            actor->position.vy = (s32)((u32)y << 16);
            actor->position.vz = (s32)((u32)z << 16);
            actor->scriptFlags.flags &= 0xFFFEFFFF;
            actor->scriptInstructionPointer += 0xD;
            actor->flags &= 0xFFDFFFFF;
        }

        actor = g_FieldScriptVMCurActor;
        pFieldActor = &g_FieldActors[D_800AFD1C];
        pFieldActor->transformMatrix.t[0] = (s16)(actor->position.vx >> 16);
        pFieldActor->transformMatrix.t[1] = (s16)(actor->position.vy >> 16);
        pFieldActor->transformMatrix.t[2] = (s16)(actor->position.vz >> 16);
        *(s32*)(sprite + 0x0) = actor->position.vx;
        *(s32*)(sprite + 0x4) = actor->position.vy;
        *(s32*)(sprite + 0x8) = actor->position.vz;
        actor->unk102 = (s16)((u16)actor->unk102 + 1);
        D_800B00C0 = 1;
        return;
    }

    /* Shared init (retail 800992E4). */
    if (stepCount == 0) {
        stepCount = 1;
    }
    x = FieldScriptArgument1(2, SCRIPT_READ_U8_REL(0xA));
    z = FieldScriptArgument2(4, SCRIPT_READ_U8_REL(0xA));
    if (SCRIPT_READ_U8_REL(1) & 0x80) {
        walkmeshId = FieldScriptArgument3(6, SCRIPT_READ_U8_REL(0xA));
        func_8007B1C4(x, z, walkmeshId, out[walkmeshId], state[walkmeshId]);
        y = out[walkmeshId][1];
        g_FieldScriptVMCurActor->walkmeshId = (s16)walkmeshId;
    } else {
        y = FieldScriptArgument3(6, SCRIPT_READ_U8_REL(0xA));
    }

    actor = g_FieldScriptVMCurActor;
    value = (s32)(*(u32*)(sprite + 0x1C) * (u32)stepCount);
    value = (s32)((u32)value + ((u32)value >> 31)) >> 1;
    value = (s32)(0u - (u32)value);
    *(s32*)(sprite + 0x10) = value;
    *(s32*)(sprite + 0x10) = (s32)((u32)value +
        (u32)MIPS_DIV((s32)(((u32)y << 16) - (u32)actor->position.vy), stepCount));
    dx = MIPS_DIV((s32)(((u32)x << 16) - (u32)actor->position.vx), (s32)((u32)stepCount + 1));
    dz = MIPS_DIV((s32)(((u32)z << 16) - (u32)actor->position.vz), (s32)((u32)stepCount + 1));
    actor->unkD0.vy = 0;
    actor->unkE0 = (u16)stepCount;
    actor->unk102 = 0;
    actor->scriptInstructionPointer += 0xB;
    actor->unkD0.vx = dx;
    actor->unkD0.vz = dz;
    D_800B00C0 = 1;
}
#endif /* XENO_PC_PORT */

void func_80099980(void) {
    g_FieldScriptVMCurActor->scripts[g_FieldScriptVMCurActor->curScriptIndex].flags_0x17 = 0;
    g_FieldScriptVMCurActor->scripts[g_FieldScriptVMCurActor->curScriptIndex].flags_0 = 0xFFFF;
    if (func_80099AC0(0xFFFF) == 0) {
        g_FieldScriptVMCurActor->scriptInstructionPointer += 0x6;
    }
}

long FieldGetVec3Magnitude(long x, long y, long z) {
    VECTOR vec;
    VECTOR vecSquared;

    vec.vx = x;
    vec.vy = y;
    vec.vz = z;

    Square0(&vec, &vecSquared);
    return SquareRoot0(vecSquared.vx + vecSquared.vy + vecSquared.vz);
}

long FieldGetVec2Magnitude(long x, long y) {
    VECTOR vec;
    VECTOR vecSquared;

    vec.vx = x;
    vec.vy = y;
    vec.vz = 0;
    Square0(&vec, &vecSquared);
    /* Retail ADDU wraps before the SDK lookup, including 0x80000000. */
    return SquareRoot0((s32)((u32)vecSquared.vx + (u32)vecSquared.vy));
}

long FieldGetVec1Magnitude(long x) {
    VECTOR vec;
    VECTOR vecSquared;

    vec.vx = x;
#ifdef XENO_PC_PORT
    vec.vy = 0;
    vec.vz = 0;
#endif
    Square0(&vec, &vecSquared);
    return SquareRoot0(vecSquared.vx);
}

extern int FieldScriptArgument1(int index, int mask);
extern int FieldScriptArgument2(int index, int mask);
extern int FieldScriptArgument5(int index, int mask);
extern int FieldScriptArgument6(int index, int mask);

/*
 * asm 80099AC0-80099EF4. Movement-toward-target tick: advances a per-script
 * "steps remaining" counter (scripts[curScriptIndex].flags_0, a 16-bit
 * countdown reusing the ActorScriptSlot bitfield word at +0x90 -- verified
 * against func_80099980's asm, which writes the identical field at the same
 * offset) while g_FieldScriptVMCurActor closes on a target point selected by
 * scripts[curScriptIndex].flags_0x17 (0=absolute, 1=relative to unkD0,
 * 2=another actor, 3=random point on a circle). Returns -1 while still
 * approaching (and holds the VM via D_800B00C0=1), 0 once arrived (or if a
 * case-1 target actor index is invalid).
 */
#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/field/nonmatchings/main/misc7", func_80099AC0);
#else
s32 func_80099AC0(s32 useStoredAngle) {
#ifdef NPC_EVENT_MUTANT_ALWAYS_DONE
    return 0;
#endif
#ifdef NPC_EVENT_MUTANT_NO_IP
    D_800B00C0 = 1;
    return -1;
#endif
    FieldActor* refFieldActor = &g_FieldActors[D_800AFD1C];
    ActorData* refActorData = (ActorData*)(uintptr_t)refFieldActor->pActorData;
    u8* refSpriteData = (u8*)(uintptr_t)refFieldActor->pSpriteData;
    ActorData* actor = g_FieldScriptVMCurActor;
    ActorScriptSlot* slot = &actor->scripts[actor->curScriptIndex];
    s32 selfX, selfZ, targetX = 0, targetZ = 0, combinedSolidRange = 0;
    s32 dx, dz, distance, stepMagnitude;
    s32 deltaVec[3];
    s16 finalAngle;

    /*
     * asm 80099AD8-80099B80: per-frame movement quantum, cached in the
     * reference actor's sprite data at +0x18 (an opaque SpriteData sub-field;
     * SpriteData itself has no declared layout in actor.h, matching the raw
     * pSpriteData+offset convention already used elsewhere, e.g. misc6.c).
     */
    if (refActorData->flags & 0x2000) {
        *(s32*)(refSpriteData + 0x18) = 0x08000000 / (s16)actor->moveSpeed;
    } else if (*(s32*)(refSpriteData + 0x18) == 0) {
        *(s32*)(refSpriteData + 0x18) = 0x04000000 / (s16)actor->moveSpeed;
    }
    stepMagnitude = FieldGetVec1Magnitude(*(s32*)(refSpriteData + 0x18) >> 15) + 1;

    /* asm 80099B98/80099BAC: upper halfword of self position.vx/vz. */
    selfX = *(s16*)((u8*)actor + 0x22);
    selfZ = *(s16*)((u8*)actor + 0x2A);

    switch (slot->flags_0x17) {
    case 0:
        targetX = FieldScriptArgument1(ARG(1), SCRIPT_READ_U8_REL(5));
        targetZ = FieldScriptArgument2(ARG(2), SCRIPT_READ_U8_REL(5));
        break;

    case 1:
        targetX = FieldScriptArgument1(ARG(1), SCRIPT_READ_U8_REL(5)) + actor->unkD0.vx;
        targetZ = FieldScriptArgument2(ARG(2), SCRIPT_READ_U8_REL(5)) + actor->unkD0.vz;
        break;

    case 2: {
#ifdef NPC_EVENT_MUTANT_MODE2_CIRCLE
        s32 angle = FieldScriptVMGetArgument(1) & 0xFFF;
        targetX = actor->unkD0.vx + ((rsin(angle) << 12) >> 12);
        targetZ = actor->unkD0.vz - ((rcos(angle) << 12) >> 12);
#else
        u32 otherIdx = FieldScriptVMGetActorIndex(1);
        ActorData* other;

        if (otherIdx == 0xFF) {
            return 0;
        }
        otherIdx = FieldScriptVMGetActorIndex(1);
        other = (ActorData*)(uintptr_t)g_FieldActors[otherIdx].pActorData;

        combinedSolidRange = FieldGetVec1Magnitude(other->solidRange + actor->solidRange);
        targetX = *(s16*)((u8*)other + 0x22);
        targetZ = *(s16*)((u8*)other + 0x2A);

        if (SCRIPT_READ_U8_REL(1) == g_PlayerActorIndex) {
            actor->scriptFlags.flags |= 0x200000;
        }
#endif
        break;
    }

    case 3: {
        s32 angle = FieldScriptVMGetArgument(1) & 0xFFF;
        targetX = actor->unkD0.vx + ((rsin(angle) << 12) >> 12);
        targetZ = actor->unkD0.vz - ((rcos(angle) << 12) >> 12);
        break;
    }
    }

    dx = targetX - selfX;
    dz = targetZ - selfZ;
    distance = FieldGetVec2Magnitude(dx, dz);
    deltaVec[0] = dx;
    deltaVec[1] = 0;
    deltaVec[2] = dz;

    actor->scriptFlags.flags |= 0x400000;

    if (slot->flags_0 != 0 && stepMagnitude + combinedSolidRange < distance) {
#ifdef XENO_PC_PORT
        /* DIAGNOSTIC (XENO_MOVE_DIAG=1): a scripted walk that never arrives is
         * what hangs the field-12 Alice scene -- Fei animates in place while
         * this tick keeps returning -1. This function does NOT translate the
         * actor; it only faces the target and raises scriptFlags 0x400000 for
         * the mover. So the question is whether the POSITION changes between
         * ticks: if `self` is pinned while `d` never shrinks, the mover is
         * refusing the move (the missing-floor theory) rather than the script
         * asking for something impossible. Removal: delete this block. */
        {
            static int s_on = -1;
            static s32 s_lastX, s_lastZ;
            static unsigned long s_ticks;

            if (s_on < 0) {
                const char* e = getenv("XENO_MOVE_DIAG");
                s_on = (e != NULL && e[0] != '\0' && e[0] != '0');
            }
            if (s_on) {
                int moved = (selfX != s_lastX) || (selfZ != s_lastZ);
                if ((s_ticks++ % 30) == 0 || moved) {
                    printf("[xeno-port][move] walk tick #%lu actor=%d "
                           "self=(%d,%d) target=(%d,%d) d=%d step=%d "
                           "solid=%d countdown=%u moved=%d\n",
                           s_ticks, (int)D_800AFD1C, (int)selfX, (int)selfZ,
                           (int)targetX, (int)targetZ, (int)distance,
                           (int)stepMagnitude, (int)combinedSolidRange,
                           (unsigned)slot->flags_0, moved);
                    fflush(stdout);
                }
                s_lastX = selfX;
                s_lastZ = selfZ;
            }
        }
#endif
        slot->flags_0 = slot->flags_0 - 1;
        finalAngle = (s16)func_8007B694(deltaVec);
        D_800B00C0 = 1;
        actor->rotation.vx = finalAngle;
        actor->rotation.vy = finalAngle;
        return -1;
    }

    /* asm 80099DF4+: arrived (or flags_0==0 short-circuit). Snap the final
     * facing angle, then reset the per-script movement state. */
    if (useStoredAngle != 0) {
        if ((actor->scriptFlags.flags & 0x8000) == 0) {
            finalAngle = actor->rotation.vy | 0x8000;
        } else {
            finalAngle = actor->unk11C | 0x8000;
        }
    } else {
        finalAngle = (s16)func_8007B694(deltaVec);
    }
    actor->rotation.vx = finalAngle;
    actor->rotation.vy = finalAngle;

    slot->flags_0 = 0xFFFF;
    slot->flags_0x17 = 0;
    actor->scriptFlags.flags &= 0xFDDFF7FF;
    return 0;
}
#endif /* XENO_PC_PORT */

void FieldScriptVMWriteCurCharacterID(void) {
    FieldScriptMemoryWriteU16(SCRIPT_IMM_ARG(1), g_FieldScriptVMCurActor->characterId);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

/* Retail 0x80099F48-0x80099FC4: write the character id belonging to the
 * player actor (not necessarily the VM's current actor), then consume the
 * three-byte instruction. */
void FieldScriptVMWritePartyLeaderCharacterID(void) {
#ifdef FIELD_VM_AUDIT_MUTANT_LEADER_CURRENT_ACTOR
    FieldScriptMemoryWriteU16(
        SCRIPT_IMM_ARG(1), g_FieldScriptVMCurActor->characterId);
#else
    ActorData* player =
        (ActorData*)(uintptr_t)g_FieldActors[g_PlayerActorIndex].pActorData;
    FieldScriptMemoryWriteU16(SCRIPT_IMM_ARG(1), player->characterId);
#endif
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

void FieldScriptVMHandlerGetActorDirection(void) {
    int delta = PSX_DEGREES(22.5);

    // The direction is likely offset by two due to how g_FieldAngleToDirectionLUT is set up.
    FieldScriptMemoryWriteU16(
        SCRIPT_IMM_ARG(1),
        (PSX_ANGLE_TO_DIRECTION_8(g_FieldScriptVMCurActor->rotation.vy + delta) + 2) & MASK_8DIR_MOVEMENT_NUM_DIRECTIONS
    );
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

void FieldScriptVMHandlerGetActorPosition(void) {
    int nActorIndex = FieldScriptVMGetActorIndex(1);
    if (nActorIndex != ACTOR_ID_INVALID) {
        FieldScriptMemoryWriteU16(SCRIPT_IMM_ARG_ALIGNED(1), g_FieldActors[nActorIndex].childMatrix.t[0]);
        FieldScriptMemoryWriteU16(SCRIPT_IMM_ARG_ALIGNED(2), g_FieldActors[nActorIndex].childMatrix.t[2]);
        FieldScriptMemoryWriteU16(SCRIPT_IMM_ARG_ALIGNED(3), g_FieldActors[nActorIndex].childMatrix.t[1]);
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 8;
}

void func_8009A0FC(void) {
    g_FieldScriptVMCurActor->defaultAnimationId = SCRIPT_READ_U8_REL(1);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
}

void FieldScriptVMHandlerPlayAnimation(void) {
    unsigned char animationID;
    
    g_FieldScriptVMCurActor->flags &= 0xFEFFFFFF;
    animationID = *(u8*)&g_FieldScriptVMCurScriptData[g_FieldScriptVMCurActor->scriptInstructionPointer + 1];
    g_FieldScriptVMCurActor->unkAnimationId = animationID;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
}

void func_8009A174(void) {
    FieldScriptVMHandlerPlayAnimation();
    g_FieldScriptVMCurActor->flags &= 0xFFFEFFFF;
}

void func_8009A1AC(void) {
    if (g_FieldScriptVMCurActor->flags & 0x10000) {
        g_FieldScriptVMCurActor->unkAnimationId = 0xFF;
        g_FieldScriptVMCurActor->scriptInstructionPointer += 1;
    }
}

extern s32 D_8005A444[];

void func_8009A1E4(void) {
#ifdef XENO_PC_PORT
    void* pActor = g_FieldScriptVMCurActor;
    u8* pScriptData = (u8*)g_FieldScriptVMCurScriptData;
    u8 arg = *(u8*)(pScriptData + *(u16*)((u8*)pActor + 0xCC) + 1);
    s32 targetIdx = D_8005A444[arg];
    if (targetIdx != 0xFF) {
        ActorData* pTargetModel =
            (ActorData*)(uintptr_t)g_FieldActors[targetIdx].pActorData;
        s32 dx = *(s32*)((u8*)pTargetModel + 0x28) - *(s32*)((u8*)pActor + 0x28);
        s32 dz = *(s32*)((u8*)pTargetModel + 0x20) - *(s32*)((u8*)pActor + 0x20);
        s32 angle = -ratan2(dx, dz);
        angle |= 0x8000;
        *(u16*)((u8*)pActor + 0x104) = (u16)angle;
        *(u16*)((u8*)pActor + 0x106) = (u16)angle;
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
#else
    /* Retail reloads g_FieldScriptVMCurActor after the ratan2 call and again for
     * the IP advance, so the global is read at each use instead of being cached. */
    s32 targetIdx = D_8005A444[SCRIPT_READ_U8_REL(1)];
    if (targetIdx != 0xFF) {
        ActorData* target = (ActorData*)(uintptr_t)g_FieldActors[targetIdx].pActorData;
        s16 angle = -ratan2(target->position.vz - g_FieldScriptVMCurActor->position.vz,
                            target->position.vx - g_FieldScriptVMCurActor->position.vx) | 0x8000;
        g_FieldScriptVMCurActor->rotation.vx = angle;
        g_FieldScriptVMCurActor->rotation.vy = angle;
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
#endif
}

/* Opcode 0x6F (OP_ROTATE_TO_ACTOR), asm 8009A2A8-8009A348 line-verified.
 * Turn the current actor to face the target actor named by script byte +1,
 * then advance the IP by 2. The angle is ratan2(dz, dx) negated with the
 * 0x8000 "facing changed" latch, stored into both rotation.vx (+0x104) and
 * rotation.vy (+0x106); when the target id resolves to ACTOR_ID_INVALID the
 * rotation is left untouched but the IP still advances (retail reaches the
 * shared .L8009A324 tail either way). The stub previously bound here never
 * advanced the IP, so talk scripts spun on this opcode forever. */
void func_8009A2A8(void) {
    s32 targetIndex = FieldScriptVMGetActorIndex(1);

    if (targetIndex != ACTOR_ID_INVALID) {
        ActorData* target =
            (ActorData*)(uintptr_t)g_FieldActors[targetIndex].pActorData;
        s32 dz = target->position.vz - g_FieldScriptVMCurActor->position.vz;
        s32 dx = target->position.vx - g_FieldScriptVMCurActor->position.vx;
        s16 angle = (s16)((-ratan2(dz, dx)) | 0x8000);

        g_FieldScriptVMCurActor->rotation.vx = angle;
        g_FieldScriptVMCurActor->rotation.vy = angle;
    }

    g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
}

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/field/nonmatchings/main/misc7", func_8009A34C);
#else
void func_8009A34C(void) {
    /* Instruction-faithful to retail 8009A34C-8009A41C: the divisor is the
     * re-read halfword at g_Scene+0x8C (1..255, never zero); the tween delta
     * `-((scale - target) << 16)` wraps as u32 (retail subu/sll/negu). */
    s32 duration;
    s32 targetAngle;
    s32 scale;
    s32 delta;

    duration = SCRIPT_READ_U8_REL(3);
    *(s16*)((u8*)&g_Scene + 0x8C) = duration;
    if (duration == 0) {
        *(s16*)((u8*)&g_Scene + 0x8C) = duration + 1;
        D_800B21D8[0] = (s32)((u32)D_800B21D8[0] + 2);
    }

    targetAngle = FieldScriptVMGetArgument(1);
    scale = *(s16*)((u8*)&g_Scene + 0x6E);
    delta = MIPS_DIV((s32)(0u - (((u32)scale - (u32)targetAngle) << 16)), *(s16*)((u8*)&g_Scene + 0x8C));
    *(s32*)((u8*)&g_Scene + 0x90) = (s32)((u32)scale << 16);
    g_Scene.unk48 |= 1;
    *(s32*)((u8*)&g_Scene + 0x94) = delta;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 4;
}
#endif /* XENO_PC_PORT */

extern s16 D_800AEA64[];

/* Start DIP (scene+0x6C) lerp to target over `duration` frames. */
void func_8009A420(s32 targetDip, s32 duration) {
    s32 curDip;
    s32 delta;

    if (duration == 0) {
        D_800B21D8[0] = 2;
        duration = 1;
    }

    curDip = *(s16*)((u8*)&g_Scene + 0x6C);
    delta = -((curDip - targetDip) << 16) / duration;
    *(s16*)((u8*)&g_Scene + 0x70) = duration;
    *(s32*)((u8*)&g_Scene + 0x74) = curDip << 16;
    g_Scene.unk48 |= 0x8;
    *(s32*)((u8*)&g_Scene + 0x78) = delta;
}

/* Opcode 0xA5 — DIP lerp wrapper: arg1 = target, byte3&0x7F = duration. */
void func_8009A490(void) {
    s32 target = FieldScriptArgument1(1, SCRIPT_READ_U8_REL(3));
    s32 duration = SCRIPT_READ_U8_REL(3) & 0x7F;

    func_8009A420(target, duration);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 4;
}

int FieldGetCameraDirection(void) {
    int halfDirection = PSX_DEGREES(22.5);
    return (7 - PSX_ANGLE_TO_DIRECTION_8(g_CamInterpolation.targetAngleY - halfDirection)) 
        & MASK_8DIR_MOVEMENT_NUM_DIRECTIONS;
}

void FieldScriptWriteCameraDirection(void) {
    FieldScriptMemoryWriteU16(SCRIPT_IMM_ARG(1), FieldGetCameraDirection() & 0xFFFF);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}


void func_8009A58C(void) {
    if ((g_Scene.unk48 & SCRIPT_READ_U8_REL(1)) == 0) {
        g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
    } else {
        D_800B00C0 = 1;
    }
}

void func_8009A5E0(void) {
    if ((g_Scene.unk48 & SCRIPT_READ_U8_REL(1)) == 0) {
        g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
    } else {
        D_800B00C0 = 1;
    }
}

void FieldScriptSetDollySet(void) {
    g_Scene.dollySet = FieldScriptVMGetArgument(1);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

void FieldScriptSetDollyStop(void) {
    g_Scene.dollyStop = FieldScriptVMGetArgument(1);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

void FieldScriptCos(void) {
    int nValue;
    int angle;
    int nFactor;
    int address;

    address = SCRIPT_IMM_ARG(1);
    angle = FieldScriptArgument2(ARG(2), SCRIPT_READ_U8_REL(7));
    nFactor = FieldScriptArgument3(ARG(3),  SCRIPT_READ_U8_REL(7)); // ?
    nValue = rcos(angle) * nFactor;
    FieldScriptMemoryWriteU16(address, nValue >> 12);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 8;
}

void FieldScriptSin(void) {
    int nValue;
    int angle;
    int nFactor;
    int address;

    address = SCRIPT_IMM_ARG(1);
    angle = FieldScriptArgument2(ARG(2), SCRIPT_READ_U8_REL(7));
    nFactor = FieldScriptArgument3(ARG(3),  SCRIPT_READ_U8_REL(7));
    nValue = rsin(angle) * nFactor;
    FieldScriptMemoryWriteU16(address, nValue >> 12);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 8;
}

void FieldScriptAtan2(void) {
    int x;
    int y;
    int address;
    
    address = SCRIPT_IMM_ARG(1);
    y = FieldScriptArgument2(ARG(2), SCRIPT_READ_U8_REL(7));
    x = FieldScriptArgument3(ARG(3), SCRIPT_READ_U8_REL(7));
    
    FieldScriptMemoryWriteU16(
        address & 0xFFFF, 
        (s16) ratan2(y, x)
    );
    g_FieldScriptVMCurActor->scriptInstructionPointer += 8;
}

int FieldScriptGetCurActorDirection(void) {
    // The direction is likely offset by two due to how g_FieldAngleToDirectionLUT is set up.
    int halfDirection = PSX_DEGREES(22.5);
    return (PSX_ANGLE_TO_DIRECTION_8(g_FieldScriptVMCurActor->rotation.vy + halfDirection) + 2) & MASK_8DIR_MOVEMENT_NUM_DIRECTIONS;
}

extern s32 D_800ADB1C; // Is current actor a 2D actor (0) maybe?
void FieldSetCurrentActorRotation(int rotation) {
    short rotationValue2D;
    short rotationValue3D;

    if (D_800ADB1C == 0) {
        rotationValue3D = rotation | 0x8000;
        g_FieldScriptVMCurActor->rotation.vx = rotationValue3D;
        g_FieldScriptVMCurActor->rotation.vy = rotationValue3D;
        g_FieldScriptVMCurActor->rotation.vz = rotationValue3D;
    }
    rotationValue2D = rotation | 0x8000;
    g_FieldScriptVMCurActor->rotation.vx = rotationValue2D;
    g_FieldScriptVMCurActor->rotation.vy = rotationValue2D;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

void FieldScriptSetActorRotation(int angle) {
    ActorData* pActor;
    int actorIndex;
    int rotAngle3D;
    int rotAngle2D;

    actorIndex = FieldScriptVMGetActorIndex(1);
    if (actorIndex != ACTOR_ID_INVALID) {
        pActor = (ActorData*)(uintptr_t)g_FieldActors[FieldScriptVMGetActorIndex(1)].pActorData;
        if (D_800ADB1C == 0) {
            rotAngle3D = angle | 0x8000;
            pActor->rotation.vx = rotAngle3D;
            pActor->rotation.vy = rotAngle3D;
            pActor->rotation.vz = rotAngle3D;
            
        }
        rotAngle2D = angle | 0x8000;
        pActor->rotation.vx = rotAngle2D;
        pActor->rotation.vy = rotAngle2D;
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 4;
}

void func_8009AA00(void) {
#ifdef XENO_PC_PORT
    s32 idx1 = FieldScriptVMGetActorIndex(1);
    if (idx1 == 0xFF) goto advance;
    {
        s32 idx2 = FieldScriptVMGetActorIndex(2);
        if (idx2 == 0xFF) goto advance;
    }
    {
        ActorData* pModel2 = (ActorData*)(uintptr_t)
            g_FieldActors[FieldScriptVMGetActorIndex(2)].pActorData;
        ActorData* pModel1 = (ActorData*)(uintptr_t)
            g_FieldActors[FieldScriptVMGetActorIndex(1)].pActorData;
        s32 dx = *(s32*)((u8*)pModel2 + 0x28) - *(s32*)((u8*)pModel1 + 0x28);
        s32 dz = *(s32*)((u8*)pModel2 + 0x20) - *(s32*)((u8*)pModel1 + 0x20);
        s32 angle = -ratan2(dx, dz);
        angle |= 0x8000;
        if (D_800ADB1C == 0) {
            *(u16*)((u8*)pModel1 + 0x104) = (u16)angle;
            *(u16*)((u8*)pModel1 + 0x106) = (u16)angle;
            *(u16*)((u8*)pModel1 + 0x108) = (u16)angle;
        }
        *(u16*)((u8*)pModel1 + 0x104) = (u16)angle;
        *(u16*)((u8*)pModel1 + 0x106) = (u16)angle;
    }
advance:
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
#else
    if (FieldScriptVMGetActorIndex(1) != 0xFF && FieldScriptVMGetActorIndex(2) != 0xFF) {
        ActorData* target = (ActorData*)(uintptr_t)g_FieldActors[FieldScriptVMGetActorIndex(2)].pActorData;
        ActorData* self = (ActorData*)(uintptr_t)g_FieldActors[FieldScriptVMGetActorIndex(1)].pActorData;
        s16 angle = -ratan2(target->position.vz - self->position.vz,
                            target->position.vx - self->position.vx) | 0x8000;
        if (D_800ADB1C == 0) {
            self->rotation.vx = angle;
            self->rotation.vy = angle;
            self->rotation.vz = angle;
        }
        self->rotation.vx = angle;
        self->rotation.vy = angle;
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
#endif
}

void func_8009AB08(int rotation) {
    short rotationValue;

    rotationValue = PSX_ANGLE(rotation - g_CamInterpolation.targetAngleY) | 0x8000;
    g_FieldScriptVMCurActor->rotation.vx = rotationValue;
    g_FieldScriptVMCurActor->rotation.vy = rotationValue;

    // 3D Actor?
    if (D_800ADB1C == 0) {
        g_FieldScriptVMCurActor->rotation.vz = rotationValue;
    }
    
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}


extern u16 g_FieldAngleToDirectionLUT[
    // PSX_DEGREES(270) | 0x8000,
    // PSX_DEGREES(315) | 0x8000,
    // PSX_DEGREES(0) | 0x8000,
    // PSX_DEGREES(45) | 0x8000,
    // PSX_DEGREES(90) | 0x8000,
    // PSX_DEGREES(135) | 0x8000,
    // PSX_DEGREES(180) | 0x8000,
    // PSX_DEGREES(225) | 0x8000,
];

// Rotate current actor N turns clockwise, where 1 turn is 45 degrees
void FieldScriptRotateActorClockwise(void) {
    int numTurns = FieldScriptVMGetArgument(1);
    FieldSetCurrentActorRotation(g_FieldAngleToDirectionLUT[
        numTurns + FieldScriptGetCurActorDirection() & MASK_8DIR_MOVEMENT_NUM_DIRECTIONS
    ]);
}

// Rotate current actor N turns counter-clockwise, where 1 turn is 45 degrees
void FieldScriptRotateActorCounterClockwise(void) {
    int numTurns = FieldScriptVMGetArgument(1);
    FieldSetCurrentActorRotation(g_FieldAngleToDirectionLUT[
        FieldScriptGetCurActorDirection() - numTurns & MASK_8DIR_MOVEMENT_NUM_DIRECTIONS
    ]);
}

void FieldScriptSetActorDirection(void) {
    int directionIndex = FieldScriptVMGetArgument(2);
    FieldScriptSetActorRotation(g_FieldAngleToDirectionLUT[directionIndex]);
}

/* Opcode 0x69 — set actor rotation from 8-way index, camera-relative. */
void func_8009AC34(void) {
    s32 directionIndex = FieldScriptVMGetArgument(2);
    s32 angle = (g_FieldAngleToDirectionLUT[directionIndex] - (u16)g_CamInterpolation.targetAngleY) & 0xFFF;

    FieldScriptSetActorRotation(angle);
}

void FieldScriptVMHandlerSetCurActorRotation(void) {
    int directionIndex = FieldScriptVMGetArgument(1);
    FieldSetCurrentActorRotation(g_FieldAngleToDirectionLUT[directionIndex]);
}

void func_8009ACB4(void) {
    int directionIndex = FieldScriptVMGetArgument(1);
    func_8009AB08(g_FieldAngleToDirectionLUT[directionIndex]);
}

extern u16 D_800AEA44[];

/* Opcode 0xAB — write camera-relative cardinal facing into actor rotation. */
void func_8009ACEC(void) {
    s32 directionIndex = SCRIPT_READ_U8_REL(1);
    s16 rotation = ((D_800AEA44[directionIndex] - (u16)g_CamInterpolation.targetAngleY) & 0xFFF) | 0x8000;

    g_FieldScriptVMCurActor->rotation.vx = rotation;
    g_FieldScriptVMCurActor->rotation.vy = rotation;
    if (D_800ADB1C == 0) {
        g_FieldScriptVMCurActor->rotation.vz = rotation;
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
}

extern u16 D_800AEA54[];

void func_8009AD6C(void) {
    int directionIndex = SCRIPT_READ_U8_REL(1);
    short rotation = D_800AEA54[directionIndex] | 0x8000;

    g_FieldScriptVMCurActor->rotation.vx = rotation;
    g_FieldScriptVMCurActor->rotation.vy = rotation;
    if (D_800ADB1C == 0) {
        g_FieldScriptVMCurActor->rotation.vz = rotation;
    }

    g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
}

void func_8009ADDC(void) {
    g_Scene.unk48 |= 0x4000;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 1;
}

void func_8009AE0C(void) {
    g_Scene.unk48 &= 0xBFFF;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 1;
}

/* Start SCRZ (scene+0x68) lerp to target over `duration` frames. */

void func_8009AE3C(s32 targetScrZ, s32 duration) {
#ifdef XENO_PC_PORT
    s32 curScrZ;
    s32 delta;
    s32* pFlags;

    if (duration != 0) {
        curScrZ = *(s32*)((u8*)&g_Scene + 0x68);
        delta = -((curScrZ - targetScrZ) << 16) / duration;
        pFlags = (s32*)((u8*)&g_Scene + 0x48);
        *(s16*)((u8*)&g_Scene + 0x80) = duration;
        *pFlags |= 0x10;
        *(s32*)((u8*)&g_Scene + 0x84) = curScrZ << 16;
        *(s32*)((u8*)&g_Scene + 0x88) = delta;
    } else {
        *(s32*)((u8*)&g_Scene + 0x68) = targetScrZ;
        *(s16*)((u8*)&g_Scene + 0x80) = 0;
        D_800B21D8[0] += 2;
    }

    pFlags = (s32*)((u8*)&g_Scene + 0x48);
    *pFlags &= ~0x2000;
#else
    if (duration != 0) {
        g_Scene_s16[0x80 / 2] = duration;
        g_Scene_s32[0x48 / 4] = (s32) (g_Scene_s32[0x48 / 4] | 0x10);
        g_Scene_s32[0x84 / 4] = (s32) (g_Scene_s32[0x68 / 4] << 0x10);
        g_Scene_s32[0x88 / 4] = (s32) ((s32) -((g_Scene_s32[0x68 / 4] - targetScrZ) << 0x10) / duration);
    } else {
        g_Scene_s32[0x68 / 4] = targetScrZ;
        g_Scene_s16[0x80 / 2] = 0;
        D_800B21D8[0] += 2;
    }
    g_Scene_s32[0x48 / 4] = (s32) (g_Scene_s32[0x48 / 4] & 0xDFFF);
#endif
}

extern s32 D_8005A444[];
extern s16 D_800B2348;
#ifdef XENO_PC_PORT
/* Retail passes the full target words (a0=$s3, a1=$s4 at 8009B0E0). */
extern void func_8009E574(s32 x, s32 z);
#else
extern void func_8009E574(s16 x, s16 z);
#endif

/* Retail 0x8009AEE0-0x8009B15C. Move one of the three field-party actors
 * toward an integer X/Z target. The return value is the retail poll contract:
 * -1 while ordinary movement is still in progress, 0 once the member is
 * absent, hidden, arrived, or placed through the retail stuck fallback. */
#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/field/nonmatchings/main/misc7", func_8009AEE0);
#else
s32 func_8009AEE0(s32 partySlot, s32 targetX, s32 targetZ, s32 direction) {
#ifdef FIELD_VM_AUDIT_MUTANT_PARTY_MOVE_ALWAYS_DONE
    return 0;
#else
    /* Instruction-faithful to retail 0x8009AEE0-0x8009B15C.  Move one of the
     * three field-party actors toward an integer X/Z target; returns -1 while
     * ordinary movement is still in progress, 0 once the member is absent,
     * hidden, arrived, or placed through the stuck fallback.  Retail `div`
     * has no break guard (moveSpeed == 0 -> MIPS_DIV); deltas wrap as u32;
     * the stuck counter compare is the signed `lh 0x6E; slti 0x41`; and
     * func_8009E574 receives the full target words (retail a0=$s3, a1=$s4). */
    s32 actorIndex;
    FieldActor* fieldActor;
    ActorData* actor;
    u8* sprite;
    s32 stepMagnitude;
    s32 distance;
    s32 delta[3];
    s32 facing;
    ActorData* savedActor;
    s32 savedActorIndex;

    actorIndex = D_8005A444[partySlot];
    if (actorIndex == ACTOR_ID_INVALID) {
        return 0;
    }

    fieldActor = &g_FieldActors[actorIndex];
    if (fieldActor->status & 0x20) {
        return 0;
    }

    sprite = (u8*)(uintptr_t)fieldActor->pSpriteData;
    actor = (ActorData*)(uintptr_t)fieldActor->pActorData;
    if (*(s32*)(sprite + 0x18) == 0) {
        *(s32*)(sprite + 0x18) = MIPS_DIV(0x04000000, actor->moveSpeed);
    }

    stepMagnitude = (s32)((u32)FieldGetVec1Magnitude(*(s32*)(sprite + 0x18) >> 15) + 1);
    delta[0] = (s32)((u32)targetX - (u32)(s16)(actor->position.vx >> 16));
    delta[1] = 0;
    delta[2] = (s32)((u32)targetZ - (u32)(s16)(actor->position.vz >> 16));
    distance = FieldGetVec2Magnitude(delta[0], delta[2]);
    actor->scriptFlags.flags |= 0x400000;

    if (stepMagnitude < distance) {
        if (actor->prevPosition.x == (s16)(actor->position.vx >> 16) &&
            actor->prevPosition.y == (s16)(actor->position.vy >> 16) &&
            actor->prevPosition.z == (s16)(actor->position.vz >> 16)) {
            actor->unk6E++;
        } else {
            actor->unk6E = 0;
        }

        facing = func_8007B694(delta);
        actor->rotation.vx = (s16)facing;
        actor->rotation.vy = (s16)facing;
        if ((s16)actor->unk6E < 0x41 && D_800B2348 == 0) {
            return -1;
        }

        /* The placement helper consumes the global current-actor context.
         * Retail temporarily lends it the party actor (index re-read from
         * D_8005A444), then restores both publications. */
        savedActor = g_FieldScriptVMCurActor;
        savedActorIndex = D_800AFD1C;
        g_FieldScriptVMCurActor = actor;
        D_800AFD1C = D_8005A444[partySlot];
        func_8009E574(targetX, targetZ);
        g_FieldScriptVMCurActor = savedActor;
        D_800AFD1C = savedActorIndex;
    }

    if ((actor->scriptFlags.flags & 0x8000) == 0) {
        if (direction == 0xFF) {
            facing = (u16)actor->rotation.vy | 0x8000;
        } else {
            facing = g_FieldAngleToDirectionLUT[direction] | 0x8000;
        }
    } else {
        facing = (u16)actor->unk11C | 0x8000;
    }
    actor->rotation.vx = (s16)facing;
    actor->rotation.vy = (s16)facing;
    actor->position.vx = (s32)((u32)targetX << 16);
    actor->position.vz = (s32)((u32)targetZ << 16);
    actor->unk6E = 0;
    actor->scriptFlags.flags &= 0xFDDFF7FF;
    return 0;
#endif
}
#endif /* XENO_PC_PORT */

extern u8 D_800B21CF[];

/* FE44 — enable player interact gate. */
void func_8009B15C(void) {
    D_800B21CF[0] = 1;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 1;
}

extern u8 D_800B21CF[];
extern u8 D_800B21CC;
extern s32 D_800B2368;
extern s32 D_800B2364;
extern s32 D_800B2360;
extern u8 D_800B21CE;
extern s32 g_PlayerActorIndex;
extern void func_80081C54(s32);

void func_8009B184(void) {
    s32 i;
    D_800B21CF[0] = 0;
    D_800B21CC = 0;
    D_800B2368 = 0;
    D_800B2364 = 0;
    D_800B2360 = 0;
    D_800B21CE = 0;
    for (i = 0; i < 0x20; i++) {
        func_80081C54(g_PlayerActorIndex);
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 1;
}

extern s32 D_8005A444[];
extern s32 func_8009AEE0(s32, s32, s32, s32);
extern u8 D_800B21CC;
extern s16 D_800B2348;
extern void func_8009B338(void);

void func_8009B210(void) {
#ifdef XENO_PC_PORT
    ActorData* pModel =
        (ActorData*)(uintptr_t)g_FieldActors[D_8005A444[0]].pActorData;
    s16 posX = *(s16*)((u8*)pModel + 0x22);
    s16 posZ = *(s16*)((u8*)pModel + 0x2A);
    s32 mask = 0;
    s32 r;
    r = func_8009AEE0(0, posX, posZ, 0xFF);
    mask |= (r == 0) ? 1 : 0;
    r = func_8009AEE0(1, posX, posZ, 0xFF);
    if (r == 0) mask |= 2;
    r = func_8009AEE0(2, posX, posZ, 0xFF);
    if (r == 0) mask |= 4;
    /* Retail writes one in the third call's branch delay slot on both paths. */
    D_800B00C0 = 1;
    if (mask == 7) {
        g_FieldScriptVMCurActor->scriptInstructionPointer += 1;
        D_800B21CC = 0;
        D_800B2348 = 0;
        func_8009B338();
    } else {
        D_800B21CC = 1;
        g_FieldScriptVMCurActor->scriptInstructionPointer -= 1;
    }
#else
    /* Plain-pointer IP updates: as struct fields they are reordered across the
     * D_800B21CC/D_800B2348 stores (gcc 2.7.2 struct/non-struct alias rule). */
    ActorData* actor = (ActorData*)(uintptr_t)g_FieldActors[D_8005A444[0]].pActorData;
    s16 x = *(s16*)((u8*)actor + 0x22);
    s16 z = *(s16*)((u8*)actor + 0x2A);
    s32 blocked = func_8009AEE0(0, x, z, 0xFF) == 0;
    if (func_8009AEE0(1, x, z, 0xFF) == 0) {
        blocked |= 2;
    }
    if (func_8009AEE0(2, x, z, 0xFF) == 0) {
        blocked |= 4;
    }
    D_800B00C0 = 1;
    if (blocked == 7) {
        *(u16*)((u8*)g_FieldScriptVMCurActor + 0xCC) += 1;
        D_800B21CC = 0;
        D_800B2348 = 0;
        func_8009B338();
        return;
    }
    D_800B21CC = 1;
    *(u16*)((u8*)g_FieldScriptVMCurActor + 0xCC) -= 1;
#endif
}

extern s32 D_800B2360;
extern s32 D_800B2364;
extern s32 D_800B2368;
extern u8 D_800B21CE;

void func_8009B338(void) {
    s32 i;
    D_800B2368 = 0;
    D_800B2364 = 0;
    D_800B2360 = 0;
    D_800B21CE = 0;
    for (i = 0; i < 0x20; i++) {
        func_80081C54(g_PlayerActorIndex);
    }
}

/* Retail FE23, 0x8009B398-0x8009B664: move all three field-party members to
 * script-provided targets and poll until each helper reports completion. */
#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/field/nonmatchings/main/misc7", func_8009B398);
#else
void func_8009B398(void) {
    /* Instruction-faithful to retail 8009B398-8009B660: the argument mask
     * byte is re-read before every FieldScriptArgumentN call and the three
     * arguments of each func_8009AEE0 call are evaluated into temporaries in
     * retail call order (ArgumentN, ArgumentN+1, GetArgument). */
    s32 completeMask;
    s32 targetX;
    s32 targetZ;
    s32 direction;
    s32 i;

    if (FieldScriptArgument1(1, SCRIPT_READ_U8_REL(0xD)) == 0x7FFF) {
#ifdef FIELD_VM_AUDIT_MUTANT_PARTY_GROUP_IP_1
        g_FieldScriptVMCurActor->scriptInstructionPointer += 1;
#else
        g_FieldScriptVMCurActor->scriptInstructionPointer += 0x14;
#endif
        D_800B21CE = 1;
        for (i = 0; i < 3; i++) {
            s32 actorIndex = D_8005A444[i];
            if (actorIndex != ACTOR_ID_INVALID) {
                ActorData* actor =
                    (ActorData*)(uintptr_t)g_FieldActors[actorIndex].pActorData;
                s16 facing = (s16)((u16)actor->rotation.vy | 0x8000);
                actor->rotation.vx = facing;
                actor->rotation.vy = facing;
            }
        }
        return;
    }

    targetX = FieldScriptArgument1(1, SCRIPT_READ_U8_REL(0xD));
    targetZ = FieldScriptArgument2(3, SCRIPT_READ_U8_REL(0xD));
    direction = FieldScriptVMGetArgument(0xE);
    completeMask = func_8009AEE0(0, targetX, targetZ, direction) == 0;

    targetX = FieldScriptArgument3(5, SCRIPT_READ_U8_REL(0xD));
    targetZ = FieldScriptArgument4(7, SCRIPT_READ_U8_REL(0xD));
    direction = FieldScriptVMGetArgument(0x10);
    if (func_8009AEE0(1, targetX, targetZ, direction) == 0) {
        completeMask |= 2;
    }

    targetX = FieldScriptArgument5(9, SCRIPT_READ_U8_REL(0xD));
    targetZ = FieldScriptArgument6(0xB, SCRIPT_READ_U8_REL(0xD));
    direction = FieldScriptVMGetArgument(0x12);
    if (func_8009AEE0(2, targetX, targetZ, direction) == 0) {
        completeMask |= 4;
    }

    D_800B00C0 = 1;
    if (completeMask == 7) {
        D_800B2348 = 0;
#ifdef FIELD_VM_AUDIT_MUTANT_PARTY_GROUP_IP_1
        g_FieldScriptVMCurActor->scriptInstructionPointer += 1;
#else
        g_FieldScriptVMCurActor->scriptInstructionPointer += 0x14;
#endif
        D_800B21CC = 0;
    } else {
        D_800B21CC = 1;
        g_FieldScriptVMCurActor->scriptInstructionPointer -= 1;
    }
    D_800B21CE = 1;
}
#endif /* XENO_PC_PORT */

void func_8009B664(void) {
    FieldScriptMemoryWriteU16(SCRIPT_IMM_ARG(1), g_Scene.sceneScrZ);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

void func_8009B6AC(void) {
    func_8009AE3C(FieldScriptVMGetArgument(1), FieldScriptVMGetArgument(3));
    g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
}

/* Start yaw tween toward a relative 8-way facing over `duration` frames. */
void func_8009B708(s32 facingOffset, s32 duration) {
    s32 direction;
    s32 delta;
    s32 tableIndex;

    direction = FieldGetCameraDirection() & 0xFFFF;
    if (duration == 0) {
        D_800B21D8[0] += 2;
        duration = 1;
    }

    tableIndex = (direction << 3) + facingOffset;
    delta = ((s32)D_800AEA64[tableIndex] << 25) / duration;
    *(s16*)((u8*)&g_Scene + 0x66) = duration;
    *(s32*)((u8*)&g_Scene + 0x7C) = ((facingOffset + 4) & 7) << 9;
    *(s32*)((u8*)&g_Scene + 0x5C) = delta;
}

/* Nudge yaw ±0x200 (45°) over `duration` frames. a0==0 → +0x200, else −0x200. */
#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/field/nonmatchings/main/misc7", func_8009B7A8);
#else
void func_8009B7A8(s32 negative, s32 duration) {
    /* Retail 8009B7A8-8009B820.  duration is never zero at the div (retail
     * substitutes 1) and the dividends are +-0x2000000, so the quotient is
     * defined; the g_Scene+0x7C target wraps as u32 (retail addiu). */
    s32 delta;
    s32 target;

    if (duration == 0) {
        D_800B21D8[0] = (s32)((u32)D_800B21D8[0] + 2);
        duration = 1;
    }

    if (negative == 0) {
        delta = 0x2000000 / duration;
        target = (s32)(*(u32*)((u8*)&g_Scene + 0x7C) + 0x200);
    } else {
        delta = (s32)0xFE000000 / duration;
        target = (s32)(*(u32*)((u8*)&g_Scene + 0x7C) - 0x200);
    }

    *(s32*)((u8*)&g_Scene + 0x7C) = target;
    *(s32*)((u8*)&g_Scene + 0x5C) = delta;
    *(s16*)((u8*)&g_Scene + 0x66) = duration;
}
#endif /* XENO_PC_PORT */

void func_8009B824(void) {
    if (*(s16*)((u8*)&g_Scene + 0x66) == 0) {
        func_8009B7A8(0, FieldScriptVMGetArgument(1));
        g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
    }
    D_800B00C0 = 1;
}

void func_8009B884(void) {
    if (*(s16*)((u8*)&g_Scene + 0x66) == 0) {
        func_8009B7A8(0, FieldScriptVMGetArgument(1));
        g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
    }
    D_800B00C0 = 1;
}

/* Opcode 0xB6 — scripted camera yaw to 8-way facing (instant if D_800ADB1C==0). */

void func_8009B8E4(void) {
#ifdef XENO_PC_PORT
    s32 facing = FieldScriptVMGetArgument(1);
    s32 duration = FieldScriptVMGetArgument(3);

    if (D_800ADB1C == 0) {
        s32 angle = ((facing + 4) & 7) << 9;
        *(s16*)((u8*)&g_Scene + 0x56) = angle;
        *(s32*)((u8*)&g_Scene + 0x7C) = angle;
        g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
        D_800B00C0 = 1;
    } else if (*(s16*)((u8*)&g_Scene + 0x66) == 0) {
        func_8009B708(facing, duration);
        g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
        D_800B00C0 = 1;
    } else {
        D_800B00C0 = 1;
    }
#else
    s32 temp_a1;
    s32 temp_s0;
    s16 temp_v0;

    temp_s0 = FieldScriptVMGetArgument(1);
    temp_a1 = FieldScriptVMGetArgument(3);
    if (D_800ADB1C == 0) {
        temp_v0 = (temp_s0 + 4) & 7;
        g_Scene_s16[0x56 / 2] = (s16) (temp_v0 << 9);
        g_Scene_s32[0x7C / 4] = (s32) (temp_v0 << 9);
        g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
    } else if (g_Scene_s16[0x66 / 2] == 0) {
        func_8009B708(temp_s0, temp_a1);
        *(u16 *)((s8*)(g_FieldScriptVMCurActor) + 0xCC) = (u16) (*(u16 *)((s8*)(g_FieldScriptVMCurActor) + 0xCC) + 5);
    }
    D_800B00C0 = 1;
#endif
}

void func_8009B9A0(void) {
    if (g_Scene_s16[0x66 / 2] == 0) {
        g_Scene_s16[0xB8 / 2] = FieldGetCameraDirection();
        g_Scene_s32[0xBC / 4] = g_Scene.sceneScrZ;
        g_Scene_u16[0xC0 / 2] = g_Scene.sceneDIP;
        g_FieldScriptVMCurActor->scriptInstructionPointer += 1;
    }
}

/* Opcode 0xA0 — restore camera yaw/SCRZ/DIP saved by func_8009B9A0 over 0x20 frames. */
void func_8009BA0C(void) {
    if (*(s16*)((u8*)&g_Scene + 0x66) == 0) {
        func_8009B708(*(s16*)((u8*)&g_Scene + 0xB8), 0x20);
        func_8009AE3C(*(s32*)((u8*)&g_Scene + 0xBC), 0x20);
        func_8009A420(*(s16*)((u8*)&g_Scene + 0xC0), 0x20);
        g_FieldScriptVMCurActor->scriptInstructionPointer += 1;
    }
}

void func_8009BA7C(void) {
#ifdef XENO_PC_PORT
    s32 angle;
    s32 scrZ;

    *(s16*)((u8*)&g_Scene + 0x6C) = FieldScriptVMGetArgument(3);

    angle = ((FieldScriptVMGetArgument(1) + 4) & 7) << 9;
    *(s32*)((u8*)&g_Scene + 0x7C) = angle;
    *(s16*)((u8*)&g_Scene + 0x56) = angle;
    *(s32*)((u8*)&g_Scene + 0x60) = angle << 16;

    scrZ = FieldScriptVMGetArgument(5);
    *(s32*)((u8*)&g_Scene + 0x68) = scrZ;
    SetGeomScreen(scrZ);

    g_FieldScriptVMCurActor->scriptInstructionPointer += 7;
#else
    s32 t;
    s16 angle;
    s32 scrZ;

    *(s16*)((u8*)&g_Scene + 0x6C) = FieldScriptVMGetArgument(3);

    t = (FieldScriptVMGetArgument(1) + 4) & 7;
    angle = t << 9;
    *(s32*)((u8*)&g_Scene + 0x7C) = angle;
    *(s16*)((u8*)&g_Scene + 0x56) = t << 9;
    *(s32*)((u8*)&g_Scene + 0x60) = angle << 16;

    scrZ = FieldScriptVMGetArgument(5);
    *(s32*)((u8*)&g_Scene + 0x68) = scrZ;
    SetGeomScreen(scrZ);

    g_FieldScriptVMCurActor->scriptInstructionPointer += 7;
#endif
}
