#include "common.h"
#include "main/game.h"
#include "field/main.h"
#include "field/actor.h"
#include "field/script_vm.h"
#include "field/effects.h"
#include "system/memory.h"
#include "system/archive.h"
#ifdef XENO_PC_PORT
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#else
/* <assert.h> is unavailable under the matching build's -nostdinc MIPS
 * preprocessor. The assert(0) below marks an unimplemented path in a function
 * not yet byte-matched, so a no-op assert compiles safely there. */
#define assert(x) ((void)0)
#endif
#include "system/debug.h"
#include "psyq/libgpu.h"
#include "psyq/libcd.h"

extern s32 D_800AFFEC;
extern s32 D_800AFD1C; // Current actor index
extern s32 D_800B00C0;

extern void func_800379C8(char*, ...);
extern char D_8006FD44; // "STACKERR ACT=%d\n"
u_short FieldScriptGetBytecodeOffset(int scriptIndex, int routineIndex);

// Store instruction pointer + 5 on stack
void func_800A1730(void) {
    if (g_FieldScriptVMCurActor->flags12C_0x6 != SCRIPT_MAX_STACK_SIZE) {
        g_FieldScriptVMCurActor->scriptPointersStack[g_FieldScriptVMCurActor->flags12C_0x6] = g_FieldScriptVMCurActor->scriptInstructionPointer + 5;
        g_FieldScriptVMCurActor->scriptInstructionPointer = FieldScriptVMGetInstructionArgument(1);
        g_FieldScriptVMCurActor->flags12C_0x6++;
        return;
    }

    // Error
    if (g_FieldSystemMode == 0) {
        func_800379C8(&D_8006FD44, D_800AFD1C);
    }
    D_800B00C0 = 1;
}

// Store instruction pointer + 3 on stack
void func_800A17F4(void) {
    if (g_FieldScriptVMCurActor->flags12C_0x6 != SCRIPT_MAX_STACK_SIZE) {
        g_FieldScriptVMCurActor->scriptPointersStack[g_FieldScriptVMCurActor->flags12C_0x6] = g_FieldScriptVMCurActor->scriptInstructionPointer + 3;
        g_FieldScriptVMCurActor->scriptInstructionPointer = FieldScriptVMGetInstructionArgument(1);
        g_FieldScriptVMCurActor->flags12C_0x6++;
        return;
    }
    
    // Error
    if (g_FieldSystemMode == 0) {
        func_800379C8(&D_8006FD44, D_800AFD1C);
    }
    D_800B00C0 = 1;
}

// Restore instruction pointer from stack
void func_800A18B8(void) {
    // Error, invalid stack value
    if (!g_FieldScriptVMCurActor->flags12C_0x6) {
        if (g_FieldSystemMode == 0) {
            func_800379C8(&D_8006FD44, D_800AFD1C);
        }
        g_FieldScriptVMCurActor->scripts[g_FieldScriptVMCurActor->curScriptIndex].flags_0x12 = 0xFF;
        g_FieldScriptVMCurActor->scripts[g_FieldScriptVMCurActor->curScriptIndex].scriptId = 0xFF;
        D_800AFFEC = 1;
        D_800B00C0 = 1;
        return;
    }
    
    g_FieldScriptVMCurActor->flags12C_0x6--;
    g_FieldScriptVMCurActor->scriptInstructionPointer = g_FieldScriptVMCurActor->scriptPointersStack[g_FieldScriptVMCurActor->flags12C_0x6];
}

void func_800A19B0(void) {
    int i;
    
    for (i = 0; i < ACTOR_MAX_NUM_SCRIPTS; i++) {
        g_FieldScriptVMCurActor->scripts[i].waitTimer = 0;
        g_FieldScriptVMCurActor->scripts[i].state = SCRIPT_STATE_IDLE;
        g_FieldScriptVMCurActor->scripts[i].flags_0x12 = 0xF;
        g_FieldScriptVMCurActor->scripts[i].currentIP = 0xFFFF;
        g_FieldScriptVMCurActor->scripts[i].isInUse = 0;
        g_FieldScriptVMCurActor->scripts[i].scriptId = 0xFF;
        g_FieldScriptVMCurActor->scripts[i].flags_0 = 0xFFFF;
        g_FieldScriptVMCurActor->scripts[i].flags_0x17 = 0x0;
    }

    g_FieldScriptVMCurActor->curScriptIndex = 0;
    g_FieldScriptVMCurActor->unkCF = 0;
    g_FieldScriptVMCurActor->dialogFlags = 0;
    D_800B00C0 = 1;
    g_FieldScriptVMCurActor->flags12C_0x6 = 0;
}

// Yield / Stop, but change IP conditionally
void func_800A1A8C(void) {
    int i;

    for (i = 0; i < ACTOR_MAX_NUM_SCRIPTS; i++) {
        if (g_FieldScriptVMCurActor->scripts[i].flags_0x12 == 7) {
            g_FieldScriptVMCurActor->scripts[i].currentIP = FieldScriptGetBytecodeOffset(D_800AFD1C, 1);
        }
    }

    g_FieldScriptVMCurActor->scripts[g_FieldScriptVMCurActor->curScriptIndex].flags_0x12 = 0xFF;
    g_FieldScriptVMCurActor->scripts[g_FieldScriptVMCurActor->curScriptIndex].scriptId = 0xFF;
    D_800B00C0 = 1;
}

// Yield / Stop Handler
void func_800A1B70(void) {
    g_FieldScriptVMCurActor->scripts[g_FieldScriptVMCurActor->curScriptIndex].flags_0x12 = 0xFF;
    g_FieldScriptVMCurActor->scripts[g_FieldScriptVMCurActor->curScriptIndex].scriptId = 0xFF;
    D_800AFFEC = 1;
    D_800B00C0 = 1;
}

void FieldScriptVMHandlerConditionalJmp(void) {
    int nValue2;
    int nValue1;
    int nArgumentsType;
    int nCondCheck;
    int nCondResult;
    int nConditionType;

    nValue2 = 0;
    nValue1 = 0;
    nArgumentsType = ((u_char*)g_FieldScriptVMCurScriptData)[g_FieldScriptVMCurActor->scriptInstructionPointer + 5] & 0xF0;
    switch (nArgumentsType) {
    case 0x0:
        nValue1 = FieldScriptVMGetVariableValue(FieldScriptVMGetInstructionArgument(1) & 0xFFFF);
        nValue2 = FieldScriptVMGetVariableValue(FieldScriptVMGetInstructionArgument(3) & 0xFFFF);
        if (FieldScriptVMGetVariableSign(FieldScriptVMGetInstructionArgument(1) & 0xFFFF) != FIELD_SCRIPT_VM_VAR_SIGNED) {
            nValue2 = (u_short)nValue2;
        } else {
            nValue2 = (short)nValue2;
        }
        break;
    case 0x40:
        nValue1 = FieldScriptVMGetVariableValue(FieldScriptVMGetInstructionArgument(1) & 0xFFFF);
        nValue2 = FieldScriptVMGetInstructionArgumentS16(3);
        if (FieldScriptVMGetVariableSign(FieldScriptVMGetInstructionArgument(1) & 0xFFFF) != FIELD_SCRIPT_VM_VAR_SIGNED) {
            nValue2 = (u_short)nValue2;
        }
        break;
    case 0x80:
        nValue1 = FieldScriptVMGetInstructionArgumentS16(1);
        nValue2 = FieldScriptVMGetVariableValue(FieldScriptVMGetInstructionArgument(3) & 0xFFFF);
        if (FieldScriptVMGetVariableSign(FieldScriptVMGetInstructionArgument(3) & 0xFFFF) != FIELD_SCRIPT_VM_VAR_SIGNED) {
            nValue1 = (u_short)nValue1;
        }
        break;
    case 0xC0:
        nValue1 = FieldScriptVMGetInstructionArgumentS16(1);
        nValue2 = FieldScriptVMGetInstructionArgumentS16(3);
        break;
    }
    
    nCondResult = 0;
    nConditionType = ((u_char*)g_FieldScriptVMCurScriptData)[g_FieldScriptVMCurActor->scriptInstructionPointer + 5] & 0xF;
    switch (nConditionType) {
        case FIELD_SCRIPT_VM_COND_EQUAL:
            if (nValue1 == nValue2) nCondResult++;
            break;
        case FIELD_SCRIPT_VM_COND_NOT_EQUAL2:
            if (nValue1 != nValue2) nCondResult++;
            break;
        case FIELD_SCRIPT_VM_COND_LT:
            nCondCheck = nValue2 < nValue1;
            if (nCondCheck) nCondResult++;
            break;
        case FIELD_SCRIPT_VM_COND_LT2:
            nCondCheck = nValue1 < nValue2;
            if (nCondCheck) nCondResult++;
            break;
        case FIELD_SCRIPT_VM_COND_GTE:
            if (nValue1 >= nValue2) nCondResult++;
            break;
        case FIELD_SCRIPT_VM_COND_GTE2:
            if (nValue2 >= nValue1) nCondResult++;
            break;
        case FIELD_SCRIPT_VM_COND_AND:
            nCondCheck = nValue1 & nValue2;
            if (nCondCheck) nCondResult++;
            break;
        case FIELD_SCRIPT_VM_COND_NOT_EQUAL:
            if (nValue1 != nValue2) nCondResult++;
            break;
        case FIELD_SCRIPT_VM_COND_OR:
            nCondCheck = nValue1 | nValue2;
            if (nCondCheck) nCondResult++;
            break;
        case FIELD_SCRIPT_VM_COND_AND2:
            nCondCheck = nValue1 & nValue2;
            if (nCondCheck) nCondResult++;
            break;
        case FIELD_SCRIPT_VM_COND_NAND:
            nCondCheck = ~nValue1 & nValue2;
            if (nCondCheck) nCondResult++;
            break;
    }
    
    if (nCondResult == 1) {
        g_FieldScriptVMCurActor->scriptInstructionPointer = g_FieldScriptVMCurActor->scriptInstructionPointer + 8;
    } else {
        g_FieldScriptVMCurActor->scriptInstructionPointer = FieldScriptVMGetInstructionArgument(6);
    }
}

void FieldScriptVMHandlerJmp(void) {
    g_FieldScriptVMCurActor->scriptInstructionPointer = FieldScriptVMGetInstructionArgument(1);
}

void func_800A1E9C(void) {
    g_FieldScriptMaxInstructionCount += 0x20;
    g_FieldScriptVMCurActor->scriptInstructionPointer++;
}

#ifdef XENO_PC_PORT
/* DIAGNOSTIC / TEST TOOLING -- XENO_VM_TRACE=<actor index>|all.
 * Prints one line per DISTINCT instruction pointer the VM dispatches for the
 * selected actor(s): the base opcode byte, and for the 0xFE prefix the
 * sub-opcode and its first two operand bytes.  Wait opcodes re-dispatch the
 * same ip every frame and print once, so the output is the executed
 * instruction sequence, not a per-frame flood.  Capped per actor so a hot
 * script loop cannot fill a log.
 *
 * Three refinements make a STALLED script readable, which first-seen alone
 * cannot do -- a loop prints its body once and then goes silent, so the cycle
 * it is spinning in is invisible:
 *   XENO_VM_TRACE_REPEAT=1   print EVERY dispatch, not just first-seen, so the
 *                            repeating cycle and its branch decisions show up.
 *   XENO_VM_TRACE_MAX=<n>    per-actor line cap (default 4000); repeat mode
 *                            needs a bigger budget to cover several cycles.
 *   XENO_VM_TRACE_FIELD=<n>  only trace while that field is loaded, so the
 *                            budget is not burned on the boot/prologue fields
 *                            before the one under investigation.
 * A monotonic sequence number is printed so lines from different actors can be
 * interleaved back into dispatch order.
 * Removal: delete this function and its call in FieldScriptVMRun. */
static void PcPort_FieldVmTrace(u_short ip, u_char op) {
    static int s_mode = -1;        /* -1 unparsed, -2 off, -3 all, else actor */
    static int s_repeat;           /* 1 = print every dispatch (no dedup) */
    static int s_max;              /* per-actor line cap */
    static int s_field;            /* -1 = any field, else only this field */
    static unsigned s_seq;
    static u_char* s_seen[256];    /* per-actor first-seen bitset, 64K ips */
    static int s_lines[256];
    static const u_char* s_seenScript[256];
    extern s32 g_GameSceneMapNum;
    const u_char* script;
    int actor;

    if (s_mode == -1) {
        const char* e = getenv("XENO_VM_TRACE");
        const char* r = getenv("XENO_VM_TRACE_REPEAT");
        const char* m = getenv("XENO_VM_TRACE_MAX");
        const char* f = getenv("XENO_VM_TRACE_FIELD");
        s_repeat = (r != NULL && r[0] != '\0' && r[0] != '0');
        s_max = (m != NULL && m[0] != '\0') ? atoi(m) : 4000;
        s_field = (f != NULL && f[0] != '\0') ? atoi(f) : -1;
        if (e == NULL || e[0] == '\0') {
            s_mode = -2;
        } else if (e[0] == 'a') {
            s_mode = -3;
        } else {
            s_mode = atoi(e) & 0xFF;
        }
    }
    if (s_mode == -2) {
        return;
    }
    if (s_field >= 0 && (int)g_GameSceneMapNum != s_field) {
        return;
    }
    actor = D_800AFD1C & 0xFF;
    if (s_mode >= 0 && actor != s_mode) {
        return;
    }
    script = (const u_char*)g_FieldScriptVMCurScriptData;
    /* A new script buffer (map change) resets the actor's first-seen set. */
    if (s_seen[actor] == NULL || s_seenScript[actor] != script) {
        if (s_seen[actor] == NULL) {
            s_seen[actor] = calloc(0x10000 / 8, 1);
            if (s_seen[actor] == NULL) {
                return;
            }
        } else {
            int i;
            for (i = 0; i < 0x10000 / 8; i++) {
                s_seen[actor][i] = 0;
            }
        }
        s_seenScript[actor] = script;
        s_lines[actor] = 0;
    }
    if (s_lines[actor] >= s_max) {
        return;
    }
    if (!s_repeat && (s_seen[actor][ip >> 3] & (1u << (ip & 7)))) {
        return;
    }
    s_seen[actor][ip >> 3] |= (u_char)(1u << (ip & 7));
    s_lines[actor]++;
    s_seq++;
    if (op == 0xFE) {
        printf("[vm-trace] #%u actor=%d ip=%u op=FE%02X args=%02x %02x %02x %02x\n",
               s_seq, actor, (unsigned)ip, script[ip + 1], script[ip + 2],
               script[ip + 3], script[ip + 4], script[ip + 5]);
    } else {
        printf("[vm-trace] #%u actor=%d ip=%u op=%02X args=%02x %02x %02x %02x\n",
               s_seq, actor, (unsigned)ip, op, script[ip + 1], script[ip + 2],
               script[ip + 3], script[ip + 4]);
    }
    fflush(stdout);
}
#endif

void FieldScriptVMRun(int maxInstructionCount) {
    int nInstructionCount;
    u_short nInstructionPointer;
    u_char nHandlerIndex;

    D_800B00C0 = 0;
    g_FieldScriptMaxInstructionCount = maxInstructionCount;

    for (nInstructionCount = 0; nInstructionCount < g_FieldScriptMaxInstructionCount; nInstructionCount++) {
        /* TODO: delete this asm() when we hit 100% matching.
         *
         * The current working theory is that there was some debugging code injected here, most likely a break
         * instruction, and they just changed the define to an empty string for the release build. This caused
         * the loop optimization for this function to break, which is why you see a noop in the jump's delay
         * slot. From all testing done with code structure and compiler arguments, this appears to be the only
         * way to make the function match.
         */
        asm(ASM_BREAKPOINT);
        if (nInstructionCount > 0x400) {
            if (g_FieldSystemMode == SYSTEM_PC_HARDDRIVE) {
                func_800379C8(&D_8006FD84, D_800AFD1C); // Error printing
            }
            return;
        }
        nInstructionPointer = g_FieldScriptVMCurActor->scriptInstructionPointer;
        nHandlerIndex = ((u_char *)g_FieldScriptVMCurScriptData)[nInstructionPointer];
#ifdef XENO_PC_PORT
        PcPort_FieldVmTrace(nInstructionPointer, nHandlerIndex);
#endif
        g_FieldScriptVMHandlers[nHandlerIndex]();

        if (D_800AFFEC == 0) {
            g_FieldScriptMaxInstructionCount = 0xFFFF;
        }

        // Various exit checks
        if ((D_800ADB1C != 0 && (D_800ADBE0 == 0 ||  D_800ADBE4 == 0 ||  D_800ADBEC == 0))) {
            return;
        }

        if (D_800B00C0 == 1 && D_800AFFEC == D_800B00C0) {
            return;
        }

    }

}

extern s32 D_800ADB68;
extern s32 D_800ADB74;
extern s32 D_800ADBFC;
extern s32 D_800ADBE0;
extern s32 D_800ADBE4;
extern s32 D_800ADBEC;
extern s32 D_800ADB1C;
extern s32 D_800C4268;
extern s32 D_8005A444[];
extern u8 D_800B21CC;
extern FieldActor* D_800B06B8;

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/field/nonmatchings/scripts/virtual_machine", func_800A2030);
#else
void func_800A2030(void) {
    s32 numActors;
    s32 actorIndex;

    if (D_800ADB74 == 1) {
        numActors = 1;
    } else {
        numActors = D_800ADBFC;
    }

    D_800ADB68 = 0;
    D_800C4268 = 0;

    for (actorIndex = 0; actorIndex < numActors; actorIndex++) {
        FieldActor* fieldActor = &g_FieldActors[actorIndex];
        ActorData* actor = (ActorData*)(uintptr_t)fieldActor->pActorData;
        s32 slot;
        s32 bestPriority;

        if ((fieldActor->status & 0x0F00) == 0) {
            continue;
        }

        if (*(u32*)((u8*)actor + 0x04) & 0x00100000) {
            continue;
        }

        if (D_800ADB1C != 0) {
            if (D_800ADBE0 == 0 || D_800ADBE4 == 0 || D_800ADBEC == 0) {
                return;
            }
        }

        actor->flags &= 0xFEFFFFFF;
        D_800B06B8 = fieldActor;
        D_800AFD1C = actorIndex;
        g_FieldScriptVMCurActor = actor;

        if (D_800B21CC != 0) {
            s32 i;
            s32 shouldSkip = 0;

            for (i = 0; i < 3; i++) {
                if (D_8005A444[i] != 0xFF && D_8005A444[i] == actorIndex) {
                    shouldSkip = 1;
                    break;
                }
            }
            if (shouldSkip) {
                continue;
            }
        }

        bestPriority = 0xF;
        for (slot = 0; slot < 8; slot++) {
            u32 scriptWord = *(u32*)((u8*)g_FieldScriptVMCurActor + 0x90 + slot * 8);
            s32 priority = (scriptWord >> 18) & 0xF;

            if (priority <= bestPriority) {
                bestPriority = priority;
                *(u8*)((u8*)g_FieldScriptVMCurActor + 0xCE) = slot;
            }
        }

        if (bestPriority == 0xF) {
            u16 ip = FieldScriptGetBytecodeOffset(actorIndex, 1);

            *(u16*)((u8*)g_FieldScriptVMCurActor + 0x8C) = ip;
            *(u8*)((u8*)g_FieldScriptVMCurActor + 0xCE) = 0;
            *(u32*)((u8*)g_FieldScriptVMCurActor + 0x90) =
                (*(u32*)((u8*)g_FieldScriptVMCurActor + 0x90) & 0xFFC3FFFF) | 0x001C0000;
        }

        {
            u8 curScript = *(u8*)((u8*)g_FieldScriptVMCurActor + 0xCE);
            u16 ip = *(u16*)((u8*)g_FieldScriptVMCurActor + 0x8C + curScript * 8);

            D_800AFFEC = 1;
            *(u16*)((u8*)g_FieldScriptVMCurActor + 0xCC) = ip;

            /* asm 800A2228-800A2250: gate is ActorData+0x0 bit0 (scriptFlags_0x0 /
             * isDisabled), NOT flags+0x4. Clear's OP_27 sets this bit so ambient
             * routine-1 auto-restart cannot keep running the screen-tint pulse. */
            if (!g_FieldScriptVMCurActor->scriptFlags.fields.scriptFlags_0x0) {
                FieldScriptVMRun(8);
            }

            curScript = *(u8*)((u8*)g_FieldScriptVMCurActor + 0xCE);
            *(u16*)((u8*)g_FieldScriptVMCurActor + 0x8C + curScript * 8) =
                *(u16*)((u8*)g_FieldScriptVMCurActor + 0xCC);
        }
    }
}
#endif /* XENO_PC_PORT */

// Changes current actor to the top of the field actor array and runs a script routine on it
extern FieldActor* D_800B06B8;
extern s32 D_800ADB1C;
void func_800A22AC(int scriptRoutineIndex) {
    int i;
    ActorData* pNewActor;

    // Change current actor to the first actor in the list
    D_800B06B8 = g_FieldActors;
    g_FieldScriptVMCurActor = (ActorData*)(uintptr_t)D_800B06B8->pActorData;
    pNewActor = HeapAlloc(0x138, 0x1);
    *pNewActor = *(ActorData*)(uintptr_t)D_800B06B8->pActorData;

    // Reset all event slots
    for (i = 0; i < ACTOR_MAX_NUM_SCRIPTS; i++) {
        g_FieldScriptVMCurActor->scripts[i].waitTimer = 0;
        g_FieldScriptVMCurActor->scripts[i].state = SCRIPT_STATE_IDLE;
        g_FieldScriptVMCurActor->scripts[i].flags_0x12 = 0xF;
        g_FieldScriptVMCurActor->scripts[i].currentIP = 0xFFFF;
        g_FieldScriptVMCurActor->scripts[i].isInUse = 0x0;
        g_FieldScriptVMCurActor->scripts[i].scriptId = 0xFF;
        g_FieldScriptVMCurActor->scripts[i].flags_0 = 0xFFFF;
        g_FieldScriptVMCurActor->scripts[i].flags_0x17 = 0x0;
    }
    
    D_800AFD1C = 0;
    D_800ADB1C = 0;
    D_800AFFEC = 0;

    // Run entry point routine
    g_FieldScriptVMCurActor->scriptInstructionPointer = FieldScriptGetBytecodeOffset(0, scriptRoutineIndex);
    FieldScriptVMRun(0xFFFF);
    D_800ADB1C = 1;

    // Running VM bytecode will likely have changed some fields, so they are copied back into the top
    // of the actor array.
    *(ActorData*)(uintptr_t)D_800B06B8->pActorData = *pNewActor;
    HeapFree(pNewActor);
}


extern s32 D_800ADB8C;
extern void func_800A22AC(int scriptRoutineIndex);
#ifdef XENO_PC_PORT
extern s16 D_8006BE2C[];
extern s32 D_800B2268;
extern s32 D_8005A444[];
extern s32 D_8006F990[];
extern void func_800AD4D4(s32 partySlot);
extern void func_800ACFD0(s32 partySlot);
extern void func_800821F4(void* sprite, s16 animation, void* actor);
extern void FieldParticleActorStop(s32 actorIndex, s32 mode);
extern void func_8009FEE4(s32 partySlot);
extern void func_800A0524(s32 source, s32 destination);

/* Retail 800ACFD0..800AD4D4. Unlike boarding, disembarking selects the
 * on-foot actor through the party table, not the current script actor. */
void func_800ACFD0(s32 partySlot) {
    FieldActor* party;
    FieldActor* gear;
    u8* partyData;
    u8* data;
    u32* destination;
    u32 sprite;

    ((u8*)g_pGameState)[0x22b1 + partySlot] = 0;
    party = &g_FieldActors[D_8006F990[partySlot]];
    gear = &g_FieldActors[D_8005A444[partySlot]];
    sprite = party->pSpriteData;
    party->pSpriteData = gear->pSpriteData;
    gear->pSpriteData = sprite;
    party->status = (((u16)party->status & 0xf07fu) | 0x200u) & 0xffdfu;
    partyData = (u8*)(uintptr_t)party->pActorData;
    *(u32*)partyData &= ~1u;
    func_800A0524(D_8006F990[partySlot], D_8005A444[partySlot]);

    /* The copy routine is a callback boundary: reload the actor table. */
    gear = &g_FieldActors[D_8005A444[partySlot]];
    data = (u8*)(uintptr_t)gear->pActorData;
    destination = (u32*)(uintptr_t)gear->pSpriteData;
    destination[0] = *(u32*)(data + 0x20);
    destination[1] = *(u32*)(data + 0x24);
    destination[2] = *(u32*)(data + 0x28);
    *(u32*)data = (*(u32*)data | 0x400u) & ~0x300u;
    partyData = (u8*)(uintptr_t)g_FieldActors[D_8006F990[partySlot]].pActorData;
    *(u32*)partyData &= ~0x1800u;
    *(u32*)data &= ~0x1800u;
    *(u16*)(partyData + 0x108) = *(u16*)(data + 0x108);
    *(u16*)(partyData + 0x106) = *(u16*)(data + 0x106);
    *(u16*)(partyData + 0xe8) = *(u16*)(partyData + 0xe6);
    *(u16*)(data + 0xe8) = *(u16*)(data + 0xe6);
    party = &g_FieldActors[D_8006F990[partySlot]];
    func_800821F4((void*)(uintptr_t)party->pSpriteData, 6, party);
    gear = &g_FieldActors[D_8005A444[partySlot]];
    data = (u8*)(uintptr_t)gear->pActorData;
    func_800821F4((void*)(uintptr_t)gear->pSpriteData,
                 *(s16*)(data + 0xe6), gear);
    func_8009FEE4(partySlot);
    FieldParticleActorStop(D_8006F990[partySlot], 0);
}

/* Retail 800AD4D4..800AD898. Boarding swaps sprite ownership with the
 * selected gear actor, preserves its position and updates both actor states.
 * Keep the two animation calls and subsequent particle/save updates ordered. */
void func_800AD4D4(s32 partySlot) {
    FieldActor* current = &g_FieldActors[D_800AFD1C];
    FieldActor* gear = &g_FieldActors[D_8005A444[partySlot]];
    u32 sprite = current->pSpriteData;
    u8* data;
    u8* partyData;
    u32* destination;
    current->pSpriteData = gear->pSpriteData;
    gear->pSpriteData = sprite;
    data = (u8*)(uintptr_t)gear->pActorData;
    destination = (u32*)(uintptr_t)gear->pSpriteData;
    destination[0] = *(u32*)(data + 0x20);
    destination[1] = *(u32*)(data + 0x24);
    destination[2] = *(u32*)(data + 0x28);
    current->status = (u16)current->status | 0x20;
    *(u32*)data = (*(u32*)data | 0x200u) & ~0x500u;
    ((u8*)g_pGameState)[0x22b1 + partySlot] = 1;
    partyData = (u8*)(uintptr_t)g_FieldActors[D_8006F990[partySlot]].pActorData;
    *(u32*)partyData &= ~0x1800u;
    *(u32*)data &= ~0x1800u;
    *(u16*)(partyData + 0xe8) = *(u16*)(partyData + 0xe6);
    *(u16*)(data + 0xe8) = *(u16*)(data + 0xe6);
    {
        FieldActor* actor = &g_FieldActors[D_8006F990[partySlot]];
        u8* actorData = (u8*)(uintptr_t)actor->pActorData;
        func_800821F4((void*)(uintptr_t)actor->pSpriteData,
                     *(s16*)(actorData + 0xe6), actor);
    }
    /* Retail reloads the actor table after the first animation callback. */
    {
        FieldActor* actor = &g_FieldActors[D_8005A444[partySlot]];
        u8* actorData = (u8*)(uintptr_t)actor->pActorData;
        func_800821F4((void*)(uintptr_t)actor->pSpriteData,
                     *(s16*)(actorData + 0xe6), actor);
    }
    FieldParticleActorStop(D_8006F990[partySlot], 0);
    func_8009FEE4(partySlot);
}

/* Retail 0x800AD978 reconciles each changed party slot with its field actor.
 * The boolean comparison is the two-way retail branch at
 * 0x800A8F78-0x800A8FB4: unequal presence calls the add/swap path, while an
 * equal state calls the removal/refresh path. */
void func_800AD978(s32 mode) {
    s32 i;

    if (!D_800B2268) {
        return;
    }

    for (i = 0; i < 3; i++) {
        s32 present;

        if (D_8005A444[i] == 0xFF || D_8006BE2C[i] != 1) {
            continue;
        }

        D_800AFD1C = D_8006F990[i];
        present = ((u8*)g_pGameState)[0x22B1 + i] != 0;
        if (present != (mode != 0)) {
            func_800AD4D4(i);
        } else {
            func_800ACFD0(i);
        }
    }
}

/* Retail 0x800ACE24 converts the three pre-script party snapshots into
 * changed/not-changed flags, then refreshes party presentation state. */
void func_800ACE24(void) {
    s32 i;

    for (i = 0; i < 3; i++) {
        D_8006BE2C[i] =
            D_8006BE2C[i] == ((u8*)g_pGameState)[0x22B1 + i] ? 0 : 1;
    }
    func_800AD978(0);
}
#else
extern void func_800ACE24(void);
#endif

void func_800A2488(void) {
    D_800ADB8C = 1;
    func_800A22AC(3);
    func_800ACE24();
    D_800ADB8C = 0;
}

extern s32 g_GamePartySkinsInitialized;
extern s32 D_800ADBFC;
extern s32 D_800AFC74;
extern s32 D_800B2268;
extern s32 D_8005A444[];
extern s32 D_8006F990[];
extern void* g_PartyDataBuffers[];
extern void func_800A3474(void);
extern void func_8002303C(void*, s32, s32);
extern void func_80076AC0(s32, s32, void*, s32, s32, s32, s32);

extern s32 D_800B2264;
extern u8 D_800B21D2;
extern s32 D_800B21BC;
extern s32 D_800B21C0;
extern s32 D_800B21C4;
extern s32 g_FieldNumActors;
extern s32 g_PlayerActorIndex;
extern void func_800AD898(void);
extern void func_801E8330(s32 slot, s32 arg1, s32 objId);

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/field/nonmatchings/scripts/virtual_machine", func_800A24C4);
#else
void func_800A24C4(void) {
    u8* playerData;
    s32 i;

    if (g_GamePartySkinsInitialized == 0) {
        return;
    }

    /* asm 800A24E4-800A26F8: the party-skin per-frame update. */
    func_800A22AC(2);
    func_800AD898();

    /* Refresh the registered object-slot sprites through the field archive
     * 0x6B9 native-owner entry func_801E8330. */
    for (i = 0; i < D_800B2264; i++) {
        func_801E8330(i & 0xFFFF, 0,
                      *(s16*)((u8*)&D_800B2264 - 0x80 + i * 2));
    }

    playerData = (u8*)(uintptr_t)
        *(u32*)((u8*)g_FieldActors + g_PlayerActorIndex * 0x5C + 0x4C);
    if (*(u8*)(playerData + 0x74) != 0xFF) {
        *(s32*)(playerData + 0x24) -= 8;
    }

    /* Apply the global scroll-drift accumulators (D_800B21BC/C0/C4, the
     * func_800748E8 counters) to eligible actors: real actors need
     * (+0x12C & 3) == 0; slots past D_800ADBFC skip that gate.  Mode byte
     * D_800B21D2 & 0x7F: 0 or 1 applies, else not (retail tests ==0 then
     * ==1 as two sequential blocks; a 0 never double-applies because the
     * second test re-reads the unchanged byte). */
    for (i = 0; i < g_FieldNumActors; i++) {
        u8* actor = (u8*)g_FieldActors + i * 0x5C;
        u32 mode;

        if (i < D_800ADBFC) {
            u8* actorData = (u8*)(uintptr_t)*(u32*)(actor + 0x4C);

            if (*(u32*)(actorData + 0x12C) & 0x3) {
                continue;
            }
        }

        mode = D_800B21D2 & 0x7F;
        if (mode == 0 || mode == 1) {
            *(s32*)(actor + 0x20) += D_800B21BC;
            *(s32*)(actor + 0x24) += D_800B21C0;
            *(s32*)(actor + 0x28) += D_800B21C4;
        }
    }
}
#endif /* XENO_PC_PORT */

void func_800A2714(void) {
    ActorData* pActor;
    FieldActor* pFieldActors;
    int i;
    void* pData;

    if (g_GamePartySkinsInitialized) {
        // Read animation files
        for (i = 0; i < D_800ADBFC; i++) {
            ArchiveSetIndex(0x4, 0x0);
            pActor = (ActorData*)(uintptr_t)g_FieldActors[i].pActorData;
            if (pActor->unk124 != -1) {
                g_FieldScriptVMCurActor = pActor;
                pData = HeapAlloc(ArchiveDecodeAlignedSize(pActor->unk124, pActor) + 8, 0x0);
                g_FieldScriptVMCurActor->unk120 = (u32)(uintptr_t)pData;
                ArchiveReadFileToBuffer(g_FieldScriptVMCurActor->unk124, pData, 0, CdlModeSpeed); // Read from disc into buffer
                ArchiveCdDataSync(0);
            }
        }

        // Set Special animation file
        for (i = 0; i < D_800ADBFC; i++) {
            pFieldActors = g_FieldActors;
            pActor = (ActorData*)(uintptr_t)pFieldActors[i].pActorData;
            if (pActor->unk124 != -1) {
                SpriteSetSpecialAnimFile((void*)(uintptr_t)pFieldActors[i].pSpriteData, pActor->unk120);
            }
        }
        
        func_800A3C8C();
        
        if (g_FieldEffects.distortion.isActive) {
            FieldDistortionInitialize(1);
        }
        
        FieldScriptMemoryWriteU16(0x10, 0x0);
        FieldScriptWritePartyMemberIDs();

        // Apply rotation and scale
        for (i = 0; i < D_800ADBFC; i++) {
            func_80072254(i);
        }
    }
}

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/field/nonmatchings/scripts/virtual_machine", func_800A28D4);
#else
void func_800A28D4(void) {
    int i;

    if (g_GamePartySkinsInitialized) {
        func_800A3474();

        for (i = 0; i < D_800ADBFC; i++) {
            FieldActor* pFieldActor = &g_FieldActors[i];
            ActorData* pActor = (ActorData*)(uintptr_t)pFieldActor->pActorData;
            u8 skinId = *(u8*)((u8*)pActor + 0x126);
            u8 spriteId = *(u8*)((u8*)pActor + 0x127);
            u32 flags130 = *(u32*)((u8*)pActor + 0x130);
            u32 flags134 = *(u32*)((u8*)pActor + 0x134);

            if (!(skinId & 0x80)) {
                func_80076AC0(i, spriteId, g_PartyDataBuffers[skinId],
                              (flags130 >> 28) & 0x3, flags134 & 0xF,
                              skinId, (flags134 >> 4) & 0x1);
            } else {
                u8* pSpriteData = (u8*)g_FieldSpriteData;
                u32* pOffsets = (u32*)pSpriteData;
                u8* pAnimData = pSpriteData + pOffsets[(skinId & 0x7F) + 1];
                u32 mode;

                func_80076AC0(i, spriteId, pAnimData, (flags130 >> 28) & 0x3,
                              flags134 & 0xF, skinId, (flags134 >> 4) & 0x1);

                mode = *(u16*)((u8*)pActor + 0x12E) & 0x3;
                /* Retail 800A2A74/800A2AE4 keeps a SEPARATE copy of the
                 * func_8002303C call + state stores per mode - a1 is 2 on the
                 * mode==1 arm (800A2A74, `ori $a1,$zero,0x2`) and 3 on the
                 * mode==2 arm (800A2AE4, `ori $a1,$zero,0x3` loaded in the
                 * branch delay slot at 800A2A68). Splitting the previously
                 * merged `mode == 1 || mode == 2` block is behaviour-preserving:
                 * each arm below is that mode's exact projection of the old
                 * shared body. */
                if (mode == 1) {
                    u8* pSprite = (u8*)(uintptr_t)pFieldActor->pSpriteData;
                    u8* pBase;
                    u8* pState;

                    func_8002303C(pSprite, 2, 0);
                    pBase = (u8*)(uintptr_t)*(u32*)(pSprite + 0x7C);
                    pState = (u8*)(uintptr_t)*(u32*)(pBase + 0x18);
                    *(u16*)(pState + 0x4) = (*(u32*)((u8*)pActor + 0x12C) >> 18) & 0x3FF;
                    *(u16*)(pState + 0x6) = flags130 & 0x1FF;
                } else if (mode == 2) {
                    u8* pSprite = (u8*)(uintptr_t)pFieldActor->pSpriteData;
                    u8* pBase;
                    u8* pState;

                    func_8002303C(pSprite, 3, 0);
                    pBase = (u8*)(uintptr_t)*(u32*)(pSprite + 0x7C);
                    pState = (u8*)(uintptr_t)*(u32*)(pBase + 0x18);
                    *(u16*)(pState + 0x4) = (*(u32*)((u8*)pActor + 0x12C) >> 18) & 0x3FF;
                    *(u16*)(pState + 0x6) = flags130 & 0x1FF;
                    *(u16*)(pState + 0x8) = (flags130 >> 9) & 0x3FF;
                    *(u16*)(pState + 0xA) = (flags130 >> 19) & 0x1FF;
                }
            }
        }

        if (D_800B2268) {
            for (i = 0; i < 3; i++) {
                s32 actorIndex = D_8005A444[i];

                if (actorIndex != 0xFF) {
                    s32 swapActorIndex = D_8006F990[i];
                    ActorData* pSwapActor;

                    if (((u8*)g_pGameState)[0x22B1 + i]) {
                        u32 savedSpriteData = g_FieldActors[actorIndex].pSpriteData;

                        g_FieldActors[actorIndex].pSpriteData = g_FieldActors[swapActorIndex].pSpriteData;
                        g_FieldActors[swapActorIndex].pSpriteData = savedSpriteData;

                        pSwapActor = (ActorData*)(uintptr_t)g_FieldActors[swapActorIndex].pActorData;
                        pSwapActor->flags |= 0x200;
                        pSwapActor->flags &= ~0x500;
                        g_FieldActors[swapActorIndex].status |= 0x20;
                    } else {
                        pSwapActor = (ActorData*)(uintptr_t)g_FieldActors[swapActorIndex].pActorData;
                        pSwapActor->flags |= 0x400;
                        pSwapActor->flags &= ~0x300;
                    }
                }
            }
        }

        return;
    }

    FieldScriptMemoryWriteU16(0x10, 0);
    FieldScriptWritePartyMemberIDs();

    for (i = 0; i < D_800ADBFC; i++) {
        D_800AFD1C = i;
        D_800B06B8 = &g_FieldActors[i];
        g_FieldScriptVMCurActor = (ActorData*)(uintptr_t)D_800B06B8->pActorData;
        g_FieldScriptVMCurActor->scriptInstructionPointer = FieldScriptGetBytecodeOffset(i, 2);
        if (((u8*)g_FieldScriptVMCurScriptData)[g_FieldScriptVMCurActor->scriptInstructionPointer] == 0) {
            g_FieldScriptVMCurActor->flags |= 0x4000000;
        }

        D_800AFD1C = i;
        D_800B06B8 = &g_FieldActors[i];
        g_FieldScriptVMCurActor = (ActorData*)(uintptr_t)D_800B06B8->pActorData;
        g_FieldScriptVMCurActor->scriptInstructionPointer = FieldScriptGetBytecodeOffset(i, 0);
    }

    for (i = 0; i < D_800ADBFC; i++) {
        D_800AFD1C = i;
        D_800AFC74 = 0;
        D_800AFFEC = 0;
        D_800B06B8 = &g_FieldActors[i];
        g_FieldScriptVMCurActor = (ActorData*)(uintptr_t)D_800B06B8->pActorData;
        FieldScriptVMRun(0xFFFF);

        if (D_800AFC74 == 0) {
            u8* pSpriteData = (u8*)g_FieldSpriteData;
            u8* pAnimData = pSpriteData + *(s32*)(pSpriteData + 4);
#ifdef XENO_PC_PORT
            /* Diagnostic-only static; see the matching-build note in
             * FieldAddPrimitives (misc2.c) -- confined to the port build so no
             * .sbss storage for it is emitted for the matching target. */
            {
                static s32 s_loggedFirstAttach;
                if (!s_loggedFirstAttach) {
                    printf("[field-diag] func_80076AC0 first: actor=%d actorPtr=%p actorSpriteSlot=%p curSprite=%p args=(%d,%d,%p,%d,%d,%d,%d)\n",
                           i, &g_FieldActors[i], &g_FieldActors[i].pSpriteData,
                           (void*)(uintptr_t)g_FieldActors[i].pSpriteData,
                           i, 0, pAnimData, 0, 0, 0x80, 0);
                    s_loggedFirstAttach = 1;
                }
            }
#endif
            func_80076AC0(i, 0, pAnimData, 0, 0, 0x80, 0);
            g_FieldScriptVMCurActor->flags |= 0x800;
        }
    }
}
#endif /* XENO_PC_PORT */

void FieldScriptVMHandlerNop(void) {
    g_FieldScriptVMCurActor->scriptInstructionPointer++;
}

// Script files contains a sections of sign bits for variables. 
// This function is a bit of a convoluted way to check if a bit in section of data is set or not.
int FieldScriptVMGetVariableSign(int index) {
    return -((g_FieldCurScriptFile->signBits[index >> 6] & (1 << ((index >> 1) & 0x1F))) != 0);
}

int FieldScriptVMGetVariableValue(int index) {
    if (!(g_FieldCurScriptFile->signBits[index >> 6] & (1 << ((index >> 1) & 0x1F)))) {
        return ((short*)&g_FieldScriptMemory)[index >> 1];
    } else {
        return ((u_short*)&g_FieldScriptMemory)[index >> 1];
    }
}

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/field/nonmatchings/scripts/virtual_machine", FieldScriptMemoryWriteU16);
#else
void FieldScriptMemoryWriteU16(s32 address, u16 value) {
    s32 index = address >> 1;

    ((u16*)&g_FieldScriptMemory)[index] = value;
}
#endif /* XENO_PC_PORT */

// scriptIndex here refers to the index of the script, which will (always?) correspond to an entity index
// routineIndex is an index into the offset table, which points to a bytecode routine in that script.
u_short FieldScriptGetBytecodeOffset(int scriptIndex, int routineIndex) {
    int nOffset;
    u_short* pScriptData;

    pScriptData = (u_short*)((u8*)g_FieldCurScriptFile + 0x84);
    nOffset = (scriptIndex * (SCRIPT_OFFSET_TABLE_SIZE / sizeof(u_short)) + routineIndex);
    return *(pScriptData + nOffset);
}

void FieldScriptWritePartyMemberIDs(void) {
    FieldScriptMemoryWriteU16(0x3E, g_GamePartyMembers[0]);
    FieldScriptMemoryWriteU16(0x40, g_GamePartyMembers[1]);
    FieldScriptMemoryWriteU16(0x42, g_GamePartyMembers[2]);
}

extern u16 D_8005941C;
extern u8 D_80059418, D_80059420, D_80059484, D_800594D0;
extern s32 D_8004F2F4, D_8004F318, D_8004F324, D_8004F328;
extern s32 g_GameSceneMapNum, g_PlayerActorIndex;
extern u8 D_800B02C8;
extern u16 D_800AFC6C, D_800AFE9C;
extern int FieldGetPlayerActorDirection(void);
extern int FieldGetCameraDirection(void);
extern void func_8009FEE4(s32 arg0);
#ifdef XENO_PC_PORT
extern u32 g_PcPortOpcode56Func8009FEE4ConditionCount;
#endif

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/field/nonmatchings/scripts/virtual_machine", func_800A30FC);
#else
void func_800A30FC(void) {
    s32 i;
    u16* src;
    u16* dst;

    *(s16*)((u8*)g_pGameState + 0x231A) = g_GameSceneMapNum;
    *(s16*)((u8*)g_pGameState + 0x2322) = D_8004F324;
    *(s16*)((u8*)g_pGameState + 0x2320) = *(u16*)((u8*)g_pGameState + 0x1932);
    *(s16*)((u8*)g_pGameState + 0x231C) = *(u16*)((u8*)g_pGameState + 0x1938) << 9;

    FieldScriptMemoryWriteU16(0x44, D_8005941C);
    FieldScriptMemoryWriteU16(0x46, D_800594D0);
    FieldScriptMemoryWriteU16(0x6, FieldGetPlayerActorDirection() & 0xFFFF);
    FieldScriptMemoryWriteU16(0x8, FieldGetCameraDirection() & 0xFFFF);
    FieldScriptMemoryWriteU16(0x24, *(s16*)((u8*)&g_Scene + 0x6C));
    FieldScriptMemoryWriteU16(0x3C, g_GameSceneMapNum);
    FieldScriptWritePartyMemberIDs();

    src = (u16*)&g_FieldScriptMemory;
    dst = (u16*)((u8*)g_pGameState + 0x1930);
    for (i = 0; i < 0x200; i++) {
        *dst++ = *src++;
    }
}
#endif /* XENO_PC_PORT */

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/field/nonmatchings/scripts/virtual_machine", func_800A31E8);
#else
void func_800A31E8(void) {
    s32 i;

    if (D_800B02C8 == 1) {
        return;
    }

    D_800AFC6C |= D_800AFE9C;

    for (i = 0; i < 3; i++) {
        ((u8*)g_pGameState)[0x1D34 + i] = g_GamePartyMembers[i];
    }

    func_800A30FC();

    D_8004F2F4 = 0;
    D_8004F318 += 1;

    for (i = 0; i < 3; i++) {
        if (((u8*)g_pGameState)[0x22B1 + i] == 1) {
#ifdef XENO_PC_PORT
            /* F14 instrumentation: opcode 0x56 samples this counter around its
             * outgoing-state snapshot so the conditional stub path is never
             * silently mistaken for a complete snapshot. */
            g_PcPortOpcode56Func8009FEE4ConditionCount += 1;
#endif
            func_8009FEE4(i);
        }
    }

    if (D_8004F318 >= 0x1F) {
        D_8004F318 = 0;
        if (!(D_8004F328 & 0x80)) {
            s32 value = FieldScriptVMGetVariableValue(0xA);
            s32 seconds = value & 0xFF;
            s32 minutes = (value >> 8) & 0xFF;

            if (!(D_8004F328 & 0x4)) {
                seconds += 1;
                if (seconds >= 0x3D) {
                    seconds = 0;
                    minutes += 1;
                }
            } else if (seconds == 0) {
                if (minutes != 0) {
                    seconds = 0x3B;
                    minutes -= 1;
                }
            } else {
                seconds -= 1;
            }
            FieldScriptMemoryWriteU16(0xA, (minutes << 8) | (seconds & 0xFF));
        }
    }

    FieldScriptMemoryWriteU16(0xC, D_80059418 | (D_80059420 << 8));
    FieldScriptMemoryWriteU16(0xE, D_80059484);
    FieldScriptMemoryWriteU16(0x1E, *(s16*)((u8*)g_FieldActors[g_PlayerActorIndex].pActorData + 0x22));
    FieldScriptMemoryWriteU16(0x20, *(s16*)((u8*)g_FieldActors[g_PlayerActorIndex].pActorData + 0x2A));
    FieldScriptMemoryWriteU16(0x22, *(s16*)((u8*)g_FieldActors[g_PlayerActorIndex].pActorData + 0x26));
}
#endif /* XENO_PC_PORT */

extern u8 D_8005A4E4[];
extern s32 D_8005A408[];
extern u8* D_800AFC50;
extern u8 D_800B007C[];
extern u32 D_800AFB20[];
extern VECTOR g_CameraEye;
extern char D_8006FD9C;
extern void func_80021EBC(void*, void*);

extern void* memcpy(void*, const void*, size_t);

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/field/nonmatchings/scripts/virtual_machine", func_800A3474);
#else
void func_800A3474(void) {
    int i;

    D_800AFC50 = D_8005A4E4;
    g_FieldNumActors = D_8005A4E4[0];
    D_800AFC50 = D_8005A4E4 + 4;

    memcpy(D_800B007C, D_800AFC50, 0x38);
    D_800AFC50 += 0x38;
    memcpy((u8*)&g_Scene + 0xC4, D_800AFC50, 0x74);
    D_800AFC50 += 0x74;
    memcpy((void*)(uintptr_t)D_800AFB20[0], D_800AFC50, 0x400);
    D_800AFC50 += 0x400;
    memcpy((u8*)&g_FieldEffects, D_800AFC50, 0x2E4);
    D_800AFC50 += 0x2E4;
    memcpy((u8*)&g_CameraEye, D_800AFC50, 0x1C8);
    D_800AFC50 += 0x1C8;

    for (i = 0; i < D_800ADBFC; i++) {
        u8* actor = (u8*)&g_FieldActors[i];
        u8* actorData = (u8*)(uintptr_t)*(u32*)(actor + 0x4C);
        u8* actorSave = D_800AFC50;
        u32 saved118;

        memcpy(actor + 0x50, D_800AFC50, 0x8);
        D_800AFC50 += 0x8;
        *(u16*)(actor + 0x58) = *(u16*)D_800AFC50;
        D_800AFC50 = actorSave + 0x3C;

        saved118 = *(u32*)(actorData + 0x118);
        memcpy(actorData, D_800AFC50, 0x138);
        D_800AFC50 += 0x138;
        *(u32*)(actorData + 0x118) = saved118;

        if (*(u32*)(actorData + 0x134) & 0x80) {
            *(u32*)(actorData + 0x110) = (u32)(uintptr_t)HeapAlloc(0xC, 0);
            memcpy((void*)(uintptr_t)*(u32*)(actorData + 0x110), D_800AFC50, 0xC);
            D_800AFC50 += 0xC;
        }

        if (*(u32*)(actorData + 0x12C) & 0x1000) {
            *(u32*)(actorData + 0x114) = (u32)(uintptr_t)HeapAlloc(0x10, 0);
            memcpy((void*)(uintptr_t)*(u32*)(actorData + 0x114), D_800AFC50, 0x10);
            D_800AFC50 += 0x10;
        }
    }

    memcpy(&g_FieldScriptMemory, D_800AFC50, 0x800);
    D_800AFC50 += 0x800;
}
#endif /* XENO_PC_PORT */

extern s32 D_800B2268;
extern void func_80021D50(void* pSprite, void* pSave);

/* Transcribed from asm/field/nonmatchings/scripts/virtual_machine/func_800A3C8C.s
 * (0x800A3C8C-0x800A3F48). Restore g_FieldNumActors from D_8005A4E4[0], memcpy
 * 0x74 bytes at D_8005A4E4+0x3C into g_Scene+0xC4, count D_8005A408 vs
 * g_pGameState[0x22B1+i] mismatches, skip to actor records at +0x95C. Per
 * actor: skip 0xC header; if unk124!=-1 and unkEA!=0xFF patch save+0x20;
 * func_80021D50 unless flags&0x1000000 or (D_800B2268 && scriptFlags&0x600
 * && mismatch); advance 0x168, +0xC if flags134&0x80, +0x10 if +0x12C&0x1000.
 * Frame 0x30. */
#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/field/nonmatchings/scripts/virtual_machine", func_800A3C8C);
#else
void func_800A3C8C(void) {
    s32 i;
    s32 mismatch;
    FieldActor* pField;
    ActorData* pActor;
    u8* pSave;

    D_800AFC50 = D_8005A4E4;
    g_FieldNumActors = D_8005A4E4[0];
    D_800AFC50 = D_8005A4E4 + 0x3C;
    memcpy((u8*)&g_Scene + 0xC4, D_800AFC50, 0x74);

    mismatch = 0;
    for (i = 0; i < 3; i++) {
        if (D_8005A408[i] != ((u8*)g_pGameState)[0x22B1 + i]) {
            mismatch += 1;
        }
    }
    D_800AFC50 += 0x920;

    for (i = 0; i < D_800ADBFC; i++) {
        pField = &g_FieldActors[i];
        pActor = (ActorData*)(uintptr_t)pField->pActorData;
        pSave = D_800AFC50;
        D_800AFC50 = pSave + 0xC;
        if (pActor->unk124 != -1) {
            if (pActor->unkAnimationId != (s16)0xFF) {
#ifdef FIELD_A3C8C_MUTANT_SKIP_ANIM_PATCH
                /* skip save+0x20 patch */
#else
                *(s16*)(pSave + 0x20) = pActor->unkAnimationId;
#endif
            }
        }
        if ((pActor->flags & 0x1000000) == 0) {
            if (D_800B2268 == 0 || (pActor->scriptFlags.flags & 0x600) == 0
                || mismatch == 0) {
                func_80021D50((void*)(uintptr_t)pField->pSpriteData, D_800AFC50);
            }
        }
        if (*(u32*)((u8*)pActor + 0x134) & 0x80) {
            D_800AFC50 = pSave + 0xC + 0x174;
        } else {
            D_800AFC50 = pSave + 0xC + 0x168;
        }
        if (*(u32*)((u8*)pActor + 0x12C) & 0x1000) {
            D_800AFC50 += 0x10;
        }
    }
}
#endif /* XENO_PC_PORT */


#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/field/nonmatchings/scripts/virtual_machine", func_800A3F4C);
#else
void func_800A3F4C(void) {
    int i;

    D_800AFC50 = D_8005A4E4;
    D_8005A4E4[0] = (u8)g_FieldNumActors;
    D_800AFC50 = D_8005A4E4 + 4;

    memcpy(D_800AFC50, D_800B007C, 0x38);
    D_800AFC50 += 0x38;
    memcpy(D_800AFC50, (u8*)&g_Scene + 0xC4, 0x74);
    D_800AFC50 += 0x74;
    memcpy(D_800AFC50, (void*)(uintptr_t)D_800AFB20[0], 0x400);
    D_800AFC50 += 0x400;
    memcpy(D_800AFC50, (u8*)&g_FieldEffects, 0x2E4);
    D_800AFC50 += 0x2E4;
    memcpy(D_800AFC50, (u8*)&g_CameraEye, 0x1C8);
    D_800AFC50 += 0x1C8;

    for (i = 0; i < D_800ADBFC; i++) {
        u8* actor = (u8*)&g_FieldActors[i];
        u8* actorData = (u8*)(uintptr_t)*(u32*)(actor + 0x4C);
        u8* spriteData = (u8*)(uintptr_t)*(u32*)(actor + 0x04);
        u8* actorSave = D_800AFC50;

        memcpy(D_800AFC50, actor + 0x50, 0x8);
        D_800AFC50 += 0x8;
        *(u16*)(actorSave + 0x8) = *(u16*)(actor + 0x58);
        *(u16*)(actorSave + 0xA) = 0;
        D_800AFC50 = actorSave + 0xC;

        func_80021EBC(spriteData, D_800AFC50);
        D_800AFC50 += 0x30;

        memcpy(D_800AFC50, actorData, 0x138);
        D_800AFC50 += 0x138;

        if (*(u32*)(actorData + 0x134) & 0x80) {
            memcpy(D_800AFC50, (void*)(uintptr_t)*(u32*)(actorData + 0x110), 0xC);
            D_800AFC50 += 0xC;
        }

        if (*(u32*)(actorData + 0x12C) & 0x1000) {
            memcpy(D_800AFC50, (void*)(uintptr_t)*(u32*)(actorData + 0x114), 0x10);
            D_800AFC50 += 0x10;
        }
    }

    memcpy(D_800AFC50, &g_FieldScriptMemory, 0x800);
    D_800AFC50 += 0x800;

    for (i = 0; i < 3; i++) {
        D_8005A408[i] = ((u8*)g_pGameState)[0x22B1 + i];
    }

    if (g_FieldSystemMode == 0) {
        func_800379C8(&D_8006FD9C, D_800AFC50 - D_8005A4E4, D_800AFC50 - D_8005A4E4);
    }
}
#endif /* XENO_PC_PORT */

void func_800A4748(void) {
    func_800A476C(0x2c0,0x100);
}

void func_800A476C(int x, int y) {
    RECT rect;

    rect.w = 0x140;
    rect.y = 0;
    rect.x = 0;
    rect.h = 0xe0;
    SetGeomScreen(0x200);
#ifdef XENO_PC_PORT
    /* PsyCross DrawOTag writes the GL backbuffer; retail GPU already has
     * those pixels in VRAM at (0,0). Pull them into CPU vram[] before the
     * offscreen snapshot (title/menu backdrop at 704,256). */
    {
        extern void GR_StoreFrameBufferImmediate(int x, int y, int w, int h);
        GR_StoreFrameBufferImmediate(rect.x, rect.y, rect.w, rect.h);
    }
#endif
    MoveImage(&rect, x, y);
    FieldRenderSync();
}
