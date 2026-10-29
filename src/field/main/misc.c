#include "common.h"
#include "main/game.h"
#include "system/math.h"
#include "field/main.h"
#include "field/camera.h"
#include "field/actor.h"
#include "field/script_vm.h"
#include "field/text_box.h"
#include "field/particles.h"
#include "field/effects.h"
#ifdef XENO_PC_PORT
#include <stdio.h>
#endif

void FieldScriptMemoryWriteU16(int, int);

extern int rcos(int);
extern int rsin(int);

extern s32 D_800AFD1C;
extern void func_80072254(s32);
extern void SpriteSetSpecialAnimFile(SpriteData*, void*);

void FieldSetScreenDimensions(void) {
    g_FieldRenderContexts[0].dispEnv.screen.x = 0;
    g_FieldRenderContexts[0].dispEnv.screen.y = 10;
    g_FieldRenderContexts[0].dispEnv.screen.w = 0x100;
    g_FieldRenderContexts[0].dispEnv.screen.h = 0xd8;
    g_FieldRenderContexts[1].dispEnv.screen.x = 0;
    g_FieldRenderContexts[1].dispEnv.screen.y = 10;
    g_FieldRenderContexts[1].dispEnv.screen.w = 0x100;
    g_FieldRenderContexts[1].dispEnv.screen.h = 0xd8;
}

extern u8 D_800B2358[]; // Is pause disabled?

void func_80086DE0(void) {
    D_800B2358[0] = SCRIPT_READ_U8_REL(1);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
}

extern s16 D_800ADB54;

void func_80086E1C(void) {
    RECT rect;
    switch (FieldScriptVMGetArgument(1)) {
        case 0:
            rect.w = 0x500;
            rect.x = 0;
            rect.y = 0;
            rect.h = 0x200;
            ClearImage(&rect, 0x0, 0x0, 0x0);
            DrawSync(0);
            Vsync(0);
            SetDefDrawEnv(g_FieldRenderContexts[0].drawEnvs, 0, 0, 0x280, 0xE0);
            SetDefDrawEnv(g_FieldRenderContexts[1].drawEnvs, 0, 0x100, 0x280, 0xE0);
            SetDefDispEnv(&g_FieldRenderContexts[0].dispEnv, 0, 0x100, 0x280, 0xE0);
            SetDefDispEnv(&g_FieldRenderContexts[1].dispEnv, 0, 0, 0x280, 0xE0);
            FieldSetScreenDimensions();
            break;
        case 1:
            D_800ADB54 = 0x1;
            break;
        case 2:
            D_800ADB54 = 0x0;
            break;
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

// Set model animation for actor
void func_80086F7C(void) {
    g_FieldScriptVMCurActor->modelAnimation = (FieldScriptVMGetArgument(1) << 0xC) | FieldScriptVMGetArgument(3);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
}

void func_80086FD0(void) {
    switch (SCRIPT_READ_U8_REL(1)) {
        case 0:
            // Initialize Sprite List
            func_800AAC08();
            g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
            return;
        case 2:
            // Free Sprite List
            func_800AABD8();
            g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
            return;
        case 1:
            // Set X, Y of Sprite and add it to OT for drawing
            func_800AAE4C(
                FieldScriptVMGetArgument(2), // Sprite Index
                FieldScriptVMGetArgument(4), // X
                FieldScriptVMGetArgument(6), // Y
                FieldScriptVMGetArgument(8)  // Sprite type
            );
            g_FieldScriptVMCurActor->scriptInstructionPointer += 0xA;
            return;
        case 3:
            // Set Sprite Color
            func_800AADC8(
                FieldScriptVMGetArgument(2), // Sprite Index
                FieldScriptVMGetArgument(4), // Red
                FieldScriptVMGetArgument(6), // Green
                FieldScriptVMGetArgument(8)  // Blue
            );
            g_FieldScriptVMCurActor->scriptInstructionPointer += 0xA;
            return;
    }
}

#ifndef XENO_PC_PORT
/* func_80087148: in-struct store (pointer indexing) so the actor load hoists
 * above the sh; the second argument call stays inline in the |= so it is
 * evaluated before g_pGameState is loaded. */
#endif
void func_80087148(void) {
#ifdef XENO_PC_PORT
    s32 a = FieldScriptVMGetArgument(1);
    s32 b = FieldScriptVMGetArgument(3);
    *(u16*)((u8*)g_pGameState + a * 0x20 + 0x16DA) |= b;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
#else
    s32 a = FieldScriptVMGetArgument(1);
    ((u16*)((u8*)g_pGameState + (a << 5)))[0x16DA / 2] |= FieldScriptVMGetArgument(3);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
#endif
}

extern s32 D_800C3A48;
extern s32 D_800AF87C;
extern s16 D_800AFC58;
extern s16 D_800AFC5A;
extern s16 D_800AFC5C;
extern s16 D_800AFC5E;
extern s32 D_800ADBB4;

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/field/nonmatchings/main/misc", func_800871B0);
#else
/* Retail 800871B0: the 0x20 instruction-budget bump happens only in
 * sub-ops 0/1, sub-op 1 offsets by arg2 << 9, and sub-op 3 advances IP by 2. */
void func_800871B0(void) {
    u8 subOp = SCRIPT_READ_U8_REL(1);

    switch (subOp) {
        case 0: {
            s32 arg2;
            s32 arg4;
            s32 size;
            g_FieldScriptMaxInstructionCount += 0x20;
            arg2 = FieldScriptVMGetArgument(2);
            arg4 = FieldScriptVMGetArgument(4);
            size = (s32)((u32)arg4 << 9);
            D_800C3A48 = (s32)HeapAlloc(size, 0);
            D_800AF87C = (s32)HeapAlloc(size, 0);
            D_800AFC58 = 0;
            D_800AFC5A = (s16)arg2;
            D_800AFC5C = 0x100;
            D_800AFC5E = (s16)arg4;
            StoreImage(&D_800AFC58, (void*)(uintptr_t)D_800C3A48);
            D_800B00C0 = 1;
            g_FieldScriptVMCurActor->scriptInstructionPointer += 6;
            break;
        }
        case 1: {
            s32 arg2;
            s32 arg4;
            u32 ofs;
            g_FieldScriptMaxInstructionCount += 0x20;
            arg2 = FieldScriptVMGetArgument(2);
            arg4 = FieldScriptVMGetArgument(4);
            ofs = (u32)arg2 << 9;
            func_80026F44(0x100, arg4,
                (void*)(uintptr_t)((u32)D_800AF87C + ofs),
                (void*)(uintptr_t)((u32)D_800C3A48 + ofs));
            D_800ADBB4 = 1;
            g_FieldScriptVMCurActor->scriptInstructionPointer += 6;
            break;
        }
        case 2:
            HeapFree((void*)(uintptr_t)D_800C3A48);
            HeapFree((void*)(uintptr_t)D_800AF87C);
            D_800B00C0 = 1;
            g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
            break;
        case 3:
            g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
            D_800B00C0 = 1;
            break;
    }
}
#endif /* XENO_PC_PORT */

extern u8 D_800B225F[];

void func_800873C4(void) {
    D_800B225F[FieldScriptVMGetArgument(1)] = FieldScriptVMGetArgument(3);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
}

void func_80087420(void) {
    int arg1, arg2, arg3, arg4, arg5, arg6;
    int value1, value2;

    arg1 = FieldScriptVMGetArgument(ARG(1));
    arg2 = FieldScriptVMGetArgument(ARG(2));
    arg3 = FieldScriptVMGetArgument(ARG(3));
    arg4 = FieldScriptVMGetArgument(ARG(4));
    arg5 = FieldScriptVMGetArgument(ARG(5));
    arg6 = FieldScriptVMGetArgument(ARG(6));

    value1 = CONV_TO_GTE(
        CONV_FROM_GTE(arg5) / arg3 * arg1
    );
    value2 = CONV_TO_GTE(
        CONV_FROM_GTE(arg6) / arg4 * arg2
    );
    
    FieldScriptMemoryWriteU16(
        SCRIPT_IMM_ARG(7), 
        value1
    );
    FieldScriptMemoryWriteU16(
        SCRIPT_IMM_ARG(8), 
        value2
    );
    
    g_FieldScriptVMCurActor->scriptInstructionPointer += 0x11;
}

void func_8008752C(void) {
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

void func_8008754C(void) {
    g_pGameState->unk22B6 |= 0x4000;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 1;
}

void FieldScriptCopyGear(void) {
    int sourceGearIndex = FieldScriptVMGetArgument(1);
    g_pGameState->gears[FieldScriptVMGetArgument(3)] = g_pGameState->gears[sourceGearIndex];
    g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
}

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/field/nonmatchings/main/misc", func_8008764C);
#else
void func_8008764C(void) {
    s32 srcIdx = FieldScriptVMGetArgument(1);
    s32 dstIdx = FieldScriptVMGetArgument(3);
    u8* pGS = (u8*)g_pGameState;
    u8* pSrc = pGS + srcIdx * 164 + 0x26C;
    u8* pDst = pGS + dstIdx * 164 + 0x26C;
    s32 i;
    /* Copy 164 bytes (10*16 + 4) in 16-byte chunks */
    for (i = 0; i < 160; i += 16) {
        *(s32*)(pDst + i)      = *(s32*)(pSrc + i);
        *(s32*)(pDst + i + 4)  = *(s32*)(pSrc + i + 4);
        *(s32*)(pDst + i + 8)  = *(s32*)(pSrc + i + 8);
        *(s32*)(pDst + i + 12) = *(s32*)(pSrc + i + 12);
    }
    *(s32*)(pDst + 160) = *(s32*)(pSrc + 160);
    /* Copy 32 bytes at offset 0x16C0 using unaligned access */
    pSrc = pGS + srcIdx * 32 + 0x16C0;
    pDst = pGS + dstIdx * 32 + 0x16C0;
    for (i = 0; i < 32; i += 4) {
        *(s32*)(pDst + i) = *(s32*)(pSrc + i);
    }
    /* Set party flags.  Retail (differential-tested) tests the DESTINATION
     * slot index (the second operand, kept in $t1), not the source. */
    if (dstIdx == 9) {
        *(u16*)(pGS + 0x22B6) |= 0x2000;
    }
    if (dstIdx == 10) {
        *(u16*)(pGS + 0x22B6) |= 0x1000;
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
}
#endif /* XENO_PC_PORT */

extern u8 D_80050622;

void func_80087800(void) {
    FieldScriptMemoryWriteU16(
        SCRIPT_IMM_ARG(1), 
        D_80050622
    );
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

extern s32 D_800ADBDC;
extern s32 D_800ADBE4;
extern s32 D_800ADB2C;
extern s32 D_8004F308;
extern u8 D_8005061C, D_8005061D, D_8005061E, D_8005061F, D_80050620, D_80050621;
extern s32 D_800ADB88;
extern s32 D_800ADBE8;

void func_80087848(void) {
    if (D_800ADBDC == 0 || D_800ADBE4 == 0 || D_800ADB2C != 0 || D_8004F308 == -1) {
        D_800B00C0 = 1;
        g_FieldScriptVMCurActor->scriptInstructionPointer -= 1;
        return;
    }
    func_800379B4(0);
    D_8005061C = (u8)FieldScriptVMGetArgument(1);
    D_8005061D = (u8)FieldScriptVMGetArgument(3);
    D_8005061E = (u8)FieldScriptVMGetArgument(5);
    D_8005061F = (u8)FieldScriptVMGetArgument(7);
    D_80050620 = (u8)FieldScriptVMGetArgument(9);
    D_80050621 = (u8)FieldScriptVMGetArgument(11);
    D_800ADB88 = 1;
    D_800ADBE8 = 0;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 0xD;
}

void func_80087960(void) {
    FieldScriptMemoryWriteU16(SCRIPT_IMM_ARG(1), *(u16*)((u8*)g_pGameState + 0x1844));
    FieldScriptMemoryWriteU16(SCRIPT_IMM_ARG(2), *(u16*)((u8*)g_pGameState + 0x1846));
    g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
}

void func_800879D0(void) {
    FieldScriptMemoryWriteU16(SCRIPT_IMM_ARG(1), g_pGameState->unk184E);
    FieldScriptMemoryWriteU16(SCRIPT_IMM_ARG(2), g_pGameState->unk1852);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
}

extern u8 D_800B2357[];

void func_80087A40(void) {
    D_800B2357[0] = SCRIPT_READ_U8_REL(1);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
}

extern u8 D_800B2354[];

void func_80087A7C(void) {
    D_800B2354[0] = SCRIPT_READ_U8_REL(1);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
}

void func_80087AB8(void) {
#ifdef XENO_PC_PORT
    ActorData* pActor = g_FieldScriptVMCurActor;
    void* pScript = g_FieldScriptVMCurScriptData;
    u8 scriptByte = *(u8*)(pScript + pActor->scriptInstructionPointer + 9);
    u8* pGS = (u8*)g_pGameState;
    s32 arg1 = FieldScriptArgument1(1, scriptByte);
    *(s16*)(pGS + 0x184E) = (s16)arg1;
    {
        s32 arg3 = FieldScriptArgument2(3, scriptByte);
        *(s16*)(pGS + 0x1852) = (s16)arg3;
    }
    *(s16*)(pGS + 0x1854) = 0;
    *(s16*)(pGS + 0x1850) = 0;
    *(s16*)(pGS + 0x1856) = 1;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 6;
#else
    s32 result;
    ActorData *actor;
    g_pGameState->unk184E = FieldScriptArgument1(1, SCRIPT_READ_U8_REL(9));
    result = FieldScriptArgument2(3, SCRIPT_READ_U8_REL(9));
    actor = g_FieldScriptVMCurActor;
    *(s16*)((u8*)g_pGameState + 0x1852) = result;
    *(s16*)((u8*)g_pGameState + 0x1854) = 0;
    *(s16*)((u8*)g_pGameState + 0x1850) = 0;
    *(s16*)((u8*)g_pGameState + 0x1856) = 1;
    actor->scriptInstructionPointer += 6;
#endif
}

void func_80087B5C(void) {
#ifdef XENO_PC_PORT
    u8* pGS = (u8*)g_pGameState;
    u16 arg1 = (u16)FieldScriptVMGetInstructionArgument(1);
    FieldScriptMemoryWriteU16(arg1, *(u16*)(pGS + 0x182C));
    arg1 = (u16)FieldScriptVMGetInstructionArgument(3);
    FieldScriptMemoryWriteU16(arg1, *(u16*)(pGS + 0x182E));
    arg1 = (u16)FieldScriptVMGetInstructionArgument(5);
    FieldScriptMemoryWriteU16(arg1, *(u16*)(pGS + 0x1830));
    arg1 = (u16)FieldScriptVMGetInstructionArgument(7);
    FieldScriptMemoryWriteU16(arg1, *(u16*)(pGS + 0x1832));
    g_FieldScriptVMCurActor->scriptInstructionPointer += 9;
#else
    FieldScriptMemoryWriteU16(FieldScriptVMGetInstructionArgument(1) & 0xFFFF, *(u16 *)((s8*)(g_pGameState) + 0x182C));
    FieldScriptMemoryWriteU16(FieldScriptVMGetInstructionArgument(3) & 0xFFFF, *(u16 *)((s8*)(g_pGameState) + 0x182E));
    FieldScriptMemoryWriteU16(FieldScriptVMGetInstructionArgument(5) & 0xFFFF, *(u16 *)((s8*)(g_pGameState) + 0x1830));
    FieldScriptMemoryWriteU16(FieldScriptVMGetInstructionArgument(7) & 0xFFFF, *(u16 *)((s8*)(g_pGameState) + 0x1832));
    *(u16 *)((s8*)(g_FieldScriptVMCurActor) + 0xCC) = (u16) (*(u16 *)((s8*)(g_FieldScriptVMCurActor) + 0xCC) + 9);
#endif
}

extern s32 D_8004F300;

void func_80087C0C(void) {
    D_8004F300 = 1;
    g_FieldScriptVMCurActor->scriptInstructionPointer++;
}

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/field/nonmatchings/main/misc", func_80087C34);
#else
void func_80087C34(void) {
    u8* pGS = (u8*)g_pGameState;
    u8 mask;
    mask = SCRIPT_READ_U8_REL(9);
    *(s16*)(pGS + 0x182C) = (s16)FieldScriptArgument1(1, mask);
    mask = SCRIPT_READ_U8_REL(9);
    *(s16*)(pGS + 0x182E) = (s16)FieldScriptArgument2(3, mask);
    mask = SCRIPT_READ_U8_REL(9);
    *(s16*)(pGS + 0x1830) = (s16)FieldScriptArgument3(5, mask);
    mask = SCRIPT_READ_U8_REL(9);
    *(s16*)(pGS + 0x1832) = (s16)FieldScriptArgument4(7, mask);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 10;
}
#endif /* XENO_PC_PORT */

void func_80087D30(void) {
    FieldScriptMemoryWriteU16(SCRIPT_IMM_ARG(1), *(u16*)((u8*)g_pGameState + 0x1834));
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

#ifndef XENO_PC_PORT
/* func_80087D80: the 0x1834 store must be MEM_IN_STRUCT (pointer indexing =
 * INDIRECT_REF(PLUS_EXPR)) so the scheduler may hoist the g_FieldScriptVMCurActor
 * load above it; the TU idiom *(u16*)((u8*)g_pGameState + 0x1834) is a non-struct
 * store and pins that load below the sh (size 104 vs 96). */
#endif
void func_80087D80(void) {
#ifdef XENO_PC_PORT
    *(u16*)((u8*)g_pGameState + 0x1834) = FieldScriptArgument1(1, SCRIPT_READ_U8_REL(3));
    g_FieldScriptVMCurActor->scriptInstructionPointer += 4;
#else
    ((u16*)g_pGameState)[0x1834 / 2] = FieldScriptArgument1(1, SCRIPT_READ_U8_REL(3));
    g_FieldScriptVMCurActor->scriptInstructionPointer += 4;
#endif
}

extern u8 D_800B2355;
extern u8 D_800B2356;

void func_80087DE0(void) {
    int arg = FieldScriptVMGetArgument(2);
    if (SCRIPT_READ_U8_REL(1) == 0) {
        D_800B2355 = arg;
    } else {
        D_800B2356 = arg;
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 4;
}

extern s16 D_800B234C[];

void func_80087E5C(void) {
    D_800B234C[0] = FieldScriptVMGetArgument(1);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

extern s32 D_8005A444[];
extern s32 D_800ADBFC;
extern s32 g_PlayerActorIndex;
extern s16 D_800B233E;
extern s16 D_800B234E;

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/field/nonmatchings/main/misc", func_80087E98);
#else
void func_80087E98(void) {
    s32 actorIdx = FieldScriptVMGetActorIndex(1);
    if (actorIdx != 0xFF) {
        s32 i;
        u32 mask = 0xFEFFBFFF;
        if (actorIdx == D_8005A444[0]) {
            D_800B234E = 0;
        } else {
            D_800B234E = 1;
        }
        g_PlayerActorIndex = actorIdx;
        D_800B233E = (s16)actorIdx;
        for (i = 0; i < D_800ADBFC; i++) {
            u8* pFieldActor = (u8*)g_FieldActors + i * 92;
            u32 pSub = *(u32*)(pFieldActor + 0x4C);
            *(u32*)(pSub) &= mask;
        }
        {
            u8* pFieldActor = (u8*)g_FieldActors + actorIdx * 92;
            u32 pSub = *(u32*)(pFieldActor + 0x4C);
            *(u32*)(pSub) |= 0x4000;
        }
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
}
#endif /* XENO_PC_PORT */

extern u16 D_800B2348[]; /* retail materialises its address (la), as for an array */

void func_80087FA4(void) {
    D_800B2348[0]++;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 1;
}

void func_80087FD4(void) {
    func_800A8BA4();
    g_FieldScriptVMCurActor->scriptInstructionPointer += 1;
}

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/field/nonmatchings/main/misc", func_8008800C);
#else
/* Retail 8008800C: mtx.t is zeroed before func_801E72CC(&mtx, &scratch,
 * arg1, arg2) (scratch = retail sp+0x30); args 3..5 are read only after
 * SetRotMatrix/SetTransMatrix and form the vector RotTransSV transforms
 * (flag is its own word, retail sp+0x60). */
void func_8008800C(void) {
    MATRIX mtx;
    MATRIX scratch;
    SVECTOR vec;
    SVECTOR out;
    long flag;
    s32 arg1;
    s32 arg2;

    mtx.t[2] = 0;
    mtx.t[1] = 0;
    mtx.t[0] = 0;
    arg1 = FieldScriptArgument1(1, SCRIPT_READ_U8_REL(0xB));
    arg2 = FieldScriptArgument2(3, SCRIPT_READ_U8_REL(0xB));
    func_801E72CC(&mtx, &scratch, arg1, arg2);
    SetRotMatrix(&mtx);
    SetTransMatrix(&mtx);
    vec.vx = (s16)FieldScriptArgument3(5, SCRIPT_READ_U8_REL(0xB));
    vec.vy = (s16)FieldScriptArgument4(7, SCRIPT_READ_U8_REL(0xB));
    vec.vz = (s16)FieldScriptArgument5(9, SCRIPT_READ_U8_REL(0xB));
    RotTransSV(&vec, &out, &flag);
    FieldScriptMemoryWriteU16(FieldScriptVMGetInstructionArgument(0xC) & 0xFFFF, out.vx);
    FieldScriptMemoryWriteU16(FieldScriptVMGetInstructionArgument(0xE) & 0xFFFF, out.vy);
    FieldScriptMemoryWriteU16(FieldScriptVMGetInstructionArgument(0x10) & 0xFFFF, out.vz);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 0x12;
}
#endif /* XENO_PC_PORT */

/* Field-script opcode: snapshot 20 GameState per-slot entries (+0x9DC->+0x9D8
 * word, +0x9B2->+0x9B0 half, 0xA4 stride), then IP += 1. As a no-op stub this
 * never advanced the IP -> the VM desynced MAP3's script and fed garbage actor
 * index 128 to func_8009EB78 (the boot SEGV) -- same class as the FE07 fix. */
#ifndef XENO_PC_PORT
void func_80088198(void) {
    u8 *entry;
    void *p;
    s32 i;

    i = 0;
    p = g_pGameState;
    do {
        i++;
        *(s32 *)((entry = (u8 *)p) + 0x9D8) = *(s32 *)((u8 *)p + 0x9DC);
        *(u16 *)(entry + 0x9B0) = *(u16 *)((u8 *)p + 0x9B2);
        p += 0xA4;
    } while (i < 0x14);
    entry = (u8 *)g_FieldScriptVMCurActor + 0xCC;
    *(u16 *)((u8 *)g_FieldScriptVMCurActor + 0xCC) = *(u16 *)entry + 1;
}
#else
void func_80088198(void) {
    u8* p = (u8*)g_pGameState;
    int i;

    for (i = 0; i < 0x14; i++) {
        *(u32*)(p + 0x9D8) = *(u32*)(p + 0x9DC);
        *(u16*)(p + 0x9B0) = *(u16*)(p + 0x9B2);
        p += 0xA4;
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 1;
}
#endif /* XENO_PC_PORT */

extern void func_800A915C(void);
extern void func_800A91F0(void);

void func_800881E8(void) {
    u8 val = SCRIPT_READ_U8_REL(1);
    if (val == 0) {
        func_800A915C();
    } else {
        func_800A91F0();
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
}

extern s32 D_8004F308;

void func_8008825C(void) {
    if (D_8004F308 == -1) {
        g_FieldScriptVMCurActor->scriptInstructionPointer -= 1;
    } else {
        g_FieldScriptVMCurActor->scriptInstructionPointer += 1;
    }
    D_800B00C0 = 1;
}

#ifndef XENO_PC_PORT
/* func_800882B8: both arms are FieldScriptMemoryWriteU16(SCRIPT_IMM_ARG(2), x)
 * calls (cross-jumped into one); the gearId load must sit inside the call so it
 * is evaluated after the argument call. */
#endif
void func_800882B8(void) {
#ifdef XENO_PC_PORT
    s32 charId = func_8008CF3C(FieldScriptVMGetArgument(1));
    u16 dest;
    if (charId != 0xFF) {
        u8* pChar = (u8*)g_pGameState + charId * 164;
        dest = (u16)FieldScriptVMGetInstructionArgument(3);
        FieldScriptMemoryWriteU16(dest, pChar[0x30C]);
    } else {
        dest = (u16)FieldScriptVMGetInstructionArgument(3);
        FieldScriptMemoryWriteU16(dest, 0xFF);
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
#else
    s32 charId = func_8008CF3C(FieldScriptVMGetArgument(1));
    if (charId != 0xFF) {
        FieldScriptMemoryWriteU16(SCRIPT_IMM_ARG(2), g_pGameState->characters[charId].gearId);
    } else {
        FieldScriptMemoryWriteU16(SCRIPT_IMM_ARG(2), 0xFF);
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
#endif
}

void FieldScriptSetCharacterGear(void) {
    int characterId = FieldScriptVMGetArgument(1);
    g_pGameState->characters[characterId].gearId = FieldScriptVMGetArgument(3);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
}

void func_800883D4(void) {
#ifdef XENO_PC_PORT
    s32 charId = func_8008CF3C(FieldScriptVMGetArgument(2));
    if (charId != 0xFF) {
        ActorData* pActor = g_FieldScriptVMCurActor;
        void* pScript = g_FieldScriptVMCurScriptData;
        u8 scriptByte = *(u8*)(pScript + pActor->scriptInstructionPointer + 1);
        u8* pGS = (u8*)g_pGameState;
        u16 flags = *(u16*)(pGS + 0x2318);
        u32 mask = 1u << charId;
        if (scriptByte == 0) {
            *(u16*)(pGS + 0x2318) = (u16)(flags | mask);
        } else {
            *(u16*)(pGS + 0x2318) = (u16)(flags & ~mask);
        }
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 4;
#else
    s32 bit = func_8008CF3C(FieldScriptVMGetArgument(2));
    if (bit != 0xFF) {
        if (SCRIPT_READ_U8_REL(1) == 0) {
            *(u16*)((u8*)g_pGameState + 0x2318) |= 1 << bit;
        } else {
            *(u16*)((u8*)g_pGameState + 0x2318) &= ~(1 << bit);
        }
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 4;
#endif
}

extern s16 D_800B236C;

void func_8008848C(void) {
    D_800B236C = SCRIPT_READ_U8_REL(1) ^ 1;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
}

extern s16 D_800B21AC[];

void func_800884CC(void) {
    D_800B21AC[0] = FieldScriptVMGetArgument(1);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

extern s32 D_8005A444[];

void func_80088508(void) {
#ifdef XENO_PC_PORT
    s32 arg5 = FieldScriptVMGetArgument(5);
    s32 slot = D_8005A444[arg5];
    u16 arg1, arg3;
    g_FieldScriptMaxInstructionCount += 4;

    if (slot == 0xFF) {
        arg1 = (u16)FieldScriptVMGetInstructionArgument(1);
        FieldScriptMemoryWriteU16(arg1, 0);
        arg3 = (u16)FieldScriptVMGetInstructionArgument(3);
        FieldScriptMemoryWriteU16(arg3, 0);
    } else {
        u32 pFieldActor = (u32)((u8*)g_FieldActors + slot * 92);
        u32 pActorData = *(u32*)(pFieldActor + 0x04);
        u32 pSub;
        u16 val;
        arg1 = (u16)FieldScriptVMGetInstructionArgument(1);
        pSub = *(u32*)(pActorData + 0x7C);
        val = *(u16*)(pSub + 0x0C);
        FieldScriptMemoryWriteU16(arg1, val);
        arg3 = (u16)FieldScriptVMGetInstructionArgument(3);
        FieldScriptMemoryWriteU16(arg3, (u16)slot);
        pSub = *(u32*)(pActorData + 0x7C);
        val = *(u16*)(pSub + 0x0C);
        if (val != 1) {
            *(u16*)(pSub + 0x0C) = 0;
        }
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 7;
#else
    s32 slot = D_8005A444[FieldScriptVMGetArgument(5)];
    u8* pSprite;
    u8* pSub;

    g_FieldScriptMaxInstructionCount += 4;
    if (slot != 0xFF) {
        pSprite = (u8*)(uintptr_t)g_FieldActors[slot].pSpriteData;
        FieldScriptMemoryWriteU16(SCRIPT_IMM_ARG(1), *(u16*)(*(u8**)(pSprite + 0x7C) + 0xC));
        FieldScriptMemoryWriteU16(SCRIPT_IMM_ARG(2), slot);
        pSub = *(u8**)(pSprite + 0x7C);
        if (*(u16*)(pSub + 0xC) != 1) {
            *(u16*)(pSub + 0xC) = 0;
        }
    } else {
        FieldScriptMemoryWriteU16(SCRIPT_IMM_ARG(1), 0);
        FieldScriptMemoryWriteU16(SCRIPT_IMM_ARG(2), 0);
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 7;
#endif
}

#ifndef XENO_PC_PORT
/* func_8008861C: the bank index is read as a plain (non-struct) s32 so the
 * struct-field stores below do not invalidate it (one lw per iteration), and
 * the +0x32 store goes through a computed pointer (addu off,base then 0x32(v0))
 * while the +0x30 store folds into sym+0x30(off).  D_800B2384 is declared only
 * after this site (as ParticleBankHandle), so this body reaches it through an
 * asm alias with the same 16-byte (non-gp-small) shape. */
struct func_8008861C_handle { int bankIndex; u8 unk4[0xC]; };
extern struct func_8008861C_handle D_800B2384__8861C asm("D_800B2384");

#endif
void func_8008861C(void) {
#ifdef XENO_PC_PORT
    s32 i;
    u8* pBanks = (u8*)g_FieldDefaultParticleBanks;
    for (i = 0; i < 8; i++) {
        u8* pEntry = pBanks + i * 112;
        *(u16*)(pEntry + 0x32) = 0;
        *(u16*)(pEntry + 0x30) = 0;
    }
#else
    s32 i;
    s32 off;
    u8* p;

    i = 0;
    do {
        off = i * 4 + *(s32*)&D_800B2384__8861C * 0x78;
        i++;
        p = (u8*)g_FieldDefaultParticleBanks + off;
        *(s16*)(p + 0x32) = 0;
        *(s16*)((u8*)g_FieldDefaultParticleBanks + 0x30 + off) = 0;
    } while (i < 8);
#endif
}

extern s32 D_800B2374;
extern s32 D_800B2378;
extern s32 D_800B237C;
extern s32 D_800B2380;
extern void FieldInitializeDefaultParticleBanks(s32 actorIdx);

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/field/nonmatchings/main/misc", func_80088674);
#else
void func_80088674(void) {
    s32 actorIdx = FieldScriptVMGetArgument(1);
    if (actorIdx == 0xFF) { actorIdx = 0; }
    D_800B2374 = FieldScriptVMGetArgument(1);
    D_800B2378 = FieldScriptVMGetArgument(3);
    D_800B237C = FieldScriptVMGetArgument(5);
    D_800B2380 = FieldScriptVMGetArgument(7);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 9;
    FieldInitializeDefaultParticleBanks(actorIdx);
    switch (D_800B2378) {
        case 0: D_800B2378 = 0; break;
        case 1: D_800B2378 = 0x10; break;
        case 2: D_800B2378 = 0x20; break;
        case 3: D_800B2378 = 0x30; break;
    }
    g_FieldScriptMaxInstructionCount += 4;
}
#endif /* XENO_PC_PORT */

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/field/nonmatchings/main/misc", func_80088790);
#else
void func_80088790(void) {
    s32 actorIdx = FieldScriptVMGetActorIndex(1);
    if (actorIdx == 0xFF) { actorIdx = 0; }
    D_800B2374 = actorIdx;
    D_800B2378 = FieldScriptVMGetArgument(2);
    D_800B237C = FieldScriptVMGetArgument(4);
    D_800B2380 = FieldScriptVMGetArgument(6);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 8;
    FieldInitializeDefaultParticleBanks(actorIdx);
    switch (D_800B2378) {
        case 0: D_800B2378 = 0; break;
        case 1: D_800B2378 = 0x10; break;
        case 2: D_800B2378 = 0x20; break;
        case 3: D_800B2378 = 0x30; break;
    }
    g_FieldScriptMaxInstructionCount += 4;
}
#endif /* XENO_PC_PORT */

#ifndef XENO_PC_PORT
/* func_800888A4: retail ActorData+0x12C/+0x130 bitfields differ from the header
 * (0x12C: 2-bit mode at 16, 10-bit angle at 18; 0x130: 9-bit speed at 0), so a
 * local overlay struct is used.  The angle word is written with an explicit
 * mask (dir << 24, not a CSE of the sprite value dir << 6), the other two fields
 * as bitfields, and the actor pointer is taken into a local right after the
 * call: its longer life is what gives retail its a2/a1 register split. */
struct func_800888A4_actor {
    u8 pad[0x12C];
    u32 f12C_0 : 16;
    u32 f12C_16 : 2;
    u32 f12C_18 : 10;
    u32 f12C_28 : 4;
    u32 f130_0 : 9;
    u32 f130_9 : 23;
};

#endif
void func_800888A4(void) {
#ifdef XENO_PC_PORT
    u32 actorIdx = (u32)D_800AFD1C;
    u8* pFieldActor = (u8*)g_FieldActors + actorIdx * 92;
    u32 pActorData = *(u32*)(pFieldActor + 0x04);
    s32 arg1 = FieldScriptVMGetArgument(1);
    s32 dirBits = arg1 & 0xF;
    s32 arg3 = FieldScriptVMGetArgument(3);
    s32 speed = (arg3 >> 4 << 8) | (arg3 & 0xF);
    u8* pActor = (u8*)g_FieldScriptVMCurActor;
    u32 pSub;

    func_8002303C(pActorData, 2, 0);

    pSub = *(u32*)(pActorData + 0x7C);
    pSub = *(u32*)(pSub + 0x18);
    *(s16*)(pSub + 0x04) = (s16)(dirBits << 6);

    {
        u32 flags12C = *(u32*)(pActor + 0x12C);
        flags12C = (flags12C & 0xF003FFFF) | (dirBits << 24);
        *(u32*)(pActor + 0x12C) = flags12C;
    }

    pSub = *(u32*)(pActorData + 0x7C);
    pSub = *(u32*)(pSub + 0x18);
    *(s16*)(pSub + 0x06) = (s16)speed;

    {
        u32 flags130 = *(u32*)(pActor + 0x130);
        u32 flags12C = *(u32*)(pActor + 0x12C);
        flags130 = (flags130 & 0xFFFFFE00) | (speed & 0x1FF);
        flags12C = (flags12C & 0xFFFCFFFF) | 0x10000;
        *(u32*)(pActor + 0x130) = flags130;
        *(u32*)(pActor + 0x12C) = flags12C;
    }

    *(u16*)(pActor + 0xCC) += 5;
#else
    u8* pSprite;
    s32 dir;
    s32 speed;
    struct func_800888A4_actor* actor;

    pSprite = (u8*)(uintptr_t)g_FieldActors[D_800AFD1C].pSpriteData;
    dir = FieldScriptVMGetArgument(1) & 0xF;
    speed = ((FieldScriptVMGetArgument(1) >> 4) << 8) + FieldScriptVMGetArgument(3);
    func_8002303C(pSprite, 2, 0);
    actor = (struct func_800888A4_actor*)g_FieldScriptVMCurActor;
    ((s16*)*(u8**)(*(u8**)(pSprite + 0x7C) + 0x18))[2] = dir << 6;
    ((u32*)actor)[0x12C / 4] = (((u32*)actor)[0x12C / 4] & 0xF003FFFF) | (dir << 24);
    ((s16*)*(u8**)(*(u8**)(pSprite + 0x7C) + 0x18))[3] = speed;
    actor->f130_0 = speed;
    actor->f12C_16 = 1;
    ((ActorData*)actor)->scriptInstructionPointer += 5;
#endif
}

#ifndef XENO_PC_PORT
/* func_800889BC: same shape as func_800888A4 with two angle/speed pairs
 * (0x130: speed 0..8, angle2 9..18, speed2 19..27); the angle words use explicit
 * masks, the speeds and mode are bitfields, actor local taken after the call. */
struct func_800889BC_actor {
    u8 pad[0x12C];
    u32 f12C_0 : 16;
    u32 f12C_16 : 2;
    u32 f12C_18 : 10;
    u32 f12C_28 : 4;
    u32 f130_0 : 9;
    u32 f130_9 : 10;
    u32 f130_19 : 9;
    u32 f130_28 : 4;
};

#endif
void func_800889BC(void) {
#ifdef XENO_PC_PORT
    u32 actorIdx = (u32)D_800AFD1C;
    u8* pFieldActor = (u8*)g_FieldActors + actorIdx * 92;
    u32 pActorData = *(u32*)(pFieldActor + 0x04);
    /* Retail 80088A04..80088A58: arguments are read in order
     * (1),(1),(3),(5),(5),(7); speed = ((arg1b>>4)<<8) + arg3 and
     * speed2 = ((arg5b>>4)<<8) + arg7. */
    s32 arg1 = FieldScriptVMGetArgument(1);
    s32 dirBits = arg1 & 0xF;
    s32 arg1b = FieldScriptVMGetArgument(1);
    s32 speed = ((s32)arg1b >> 4 << 8) + FieldScriptVMGetArgument(3);
    s32 arg5 = FieldScriptVMGetArgument(5);
    s32 dirBits2 = arg5 & 0xF;
    s32 arg5b = FieldScriptVMGetArgument(5);
    s32 speed2 = ((s32)arg5b >> 4 << 8) + FieldScriptVMGetArgument(7);
    u8* pActor = (u8*)g_FieldScriptVMCurActor;
    u32 pSub;

    func_8002303C((void*)pActorData, 3, 0);

    pSub = *(u32*)(pActorData + 0x7C);
    pSub = *(u32*)(pSub + 0x18);
    *(s16*)(pSub + 0x04) = (s16)(dirBits << 6);

    {
        u32 flags12C = *(u32*)(pActor + 0x12C);
        *(u32*)(pActor + 0x12C) = (flags12C & 0xF003FFFF) | (dirBits << 24);
    }

    pSub = *(u32*)(pActorData + 0x7C);
    pSub = *(u32*)(pSub + 0x18);
    *(s16*)(pSub + 0x06) = (s16)speed;

    {
        u32 flags130 = *(u32*)(pActor + 0x130);
        *(u32*)(pActor + 0x130) = (flags130 & 0xFFFFFE00) | (speed & 0x1FF);
    }

    pSub = *(u32*)(pActorData + 0x7C);
    pSub = *(u32*)(pSub + 0x18);
    *(s16*)(pSub + 0x08) = (s16)(dirBits2 << 6);

    {
        u32 flags130 = *(u32*)(pActor + 0x130);
        *(u32*)(pActor + 0x130) = (flags130 & 0xFFF801FF) | (dirBits2 << 15);
    }

    pSub = *(u32*)(pActorData + 0x7C);
    pSub = *(u32*)(pSub + 0x18);
    *(s16*)(pSub + 0x0A) = (s16)speed2;

    {
        u32 flags130 = *(u32*)(pActor + 0x130);
        u32 flags12C = *(u32*)(pActor + 0x12C);
        *(u32*)(pActor + 0x130) = (flags130 & 0xF007FFFF) | ((speed2 & 0x1FF) << 19);
        *(u32*)(pActor + 0x12C) = (flags12C & 0xFFFCFFFF) | 0x20000;
    }

    *(u16*)(pActor + 0xCC) += 9;
#else
    u8* pSprite;
    s32 dir;
    s32 speed;
    s32 dir2;
    s32 speed2;
    struct func_800889BC_actor* actor;

    pSprite = (u8*)(uintptr_t)g_FieldActors[D_800AFD1C].pSpriteData;
    dir = FieldScriptVMGetArgument(1) & 0xF;
    speed = ((FieldScriptVMGetArgument(1) >> 4) << 8) + FieldScriptVMGetArgument(3);
    dir2 = FieldScriptVMGetArgument(5) & 0xF;
    speed2 = ((FieldScriptVMGetArgument(5) >> 4) << 8) + FieldScriptVMGetArgument(7);
    func_8002303C(pSprite, 3, 0);
    actor = (struct func_800889BC_actor*)g_FieldScriptVMCurActor;
    ((s16*)*(u8**)(*(u8**)(pSprite + 0x7C) + 0x18))[2] = dir << 6;
    ((u32*)actor)[0x12C / 4] = (((u32*)actor)[0x12C / 4] & 0xF003FFFF) | (dir << 24);
    ((s16*)*(u8**)(*(u8**)(pSprite + 0x7C) + 0x18))[3] = speed;
    actor->f130_0 = speed;
    ((s16*)*(u8**)(*(u8**)(pSprite + 0x7C) + 0x18))[4] = dir2 << 6;
    ((u32*)actor)[0x130 / 4] = (((u32*)actor)[0x130 / 4] & 0xFFF801FF) | (dir2 << 15);
    ((s16*)*(u8**)(*(u8**)(pSprite + 0x7C) + 0x18))[5] = speed2;
    actor->f130_19 = speed2;
    ((ActorData*)actor)->scriptInstructionPointer += 9;
    actor->f12C_16 = 2;
#endif
}

// Start of particle handlers
extern s32 D_800ADB40;
extern s32 D_800ADB8C;
extern s32 D_800B2374;
extern s32 D_800B2378;
extern s32 D_800B237C;
extern s32 D_800B2380;

typedef struct {
    /* 0x0 */ int bankIndex;
    /* 0x4 */ u8 unk4[0xC];
} ParticleBankHandle;

extern ParticleBankHandle D_800B2384;
extern s32 g_FieldScriptMaxInstructionCount;

#ifndef XENO_PC_PORT
/* func_80088B68: retail is a switch on the argument (beq 1 / beq 2 / j); the
 * bank flags update is the struct form. */
#endif
void func_80088B68(void) {
#ifdef XENO_PC_PORT
    s32 arg1 = FieldScriptVMGetArgument(1);
    u32 flagBit = 0;
    if (arg1 == 1) {
        flagBit = 0x80;
    } else if (arg1 == 2) {
        flagBit = 0x40;
    }
    {
        u32 bankIdx = D_800B2384.bankIndex;
        u32 stride = (bankIdx * 16 - bankIdx) * 8;
        u8* pBank = (u8*)g_FieldDefaultParticleBanks + stride;
        u16 flags = *(u16*)(pBank + 0x2A);
        *(u16*)(pBank + 0x2A) = (u16)(flags | flagBit);
    }
    g_FieldScriptMaxInstructionCount += 4;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 7;
#else
    s32 flagBit = 0;
    switch (FieldScriptVMGetArgument(1)) {
    case 1:
        flagBit = 0x80;
        break;
    case 2:
        flagBit = 0x40;
        break;
    }
    g_FieldDefaultParticleBanks[D_800B2384.bankIndex].flags |= flagBit;
    g_FieldScriptMaxInstructionCount += 4;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 7;
#endif
}

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/field/nonmatchings/main/misc", func_80088C1C);
#else
void func_80088C1C(void) {
    u32 bankIdx = D_800B2384.bankIndex;
    u32 stride = (bankIdx * 16 - bankIdx) * 8;
    u8* pBank = (u8*)g_FieldDefaultParticleBanks + stride;

    s32 arg1 = FieldScriptVMGetArgument(1);
    *(s16*)(pBank + 0x24) = (s16)arg1;

    arg1 = FieldScriptVMGetArgument(3);
    {
        u16 flags = *(u16*)(pBank + 0x2A);
        *(u16*)(pBank + 0x2A) = (u16)(flags | (s32)((u32)arg1 << 8));
    }

    arg1 = FieldScriptVMGetArgument(5);
    *(s16*)(pBank + 0x76) = (s16)arg1;

    g_FieldScriptMaxInstructionCount += 4;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 7;
}
#endif /* XENO_PC_PORT */

void func_80088CF8() {
    FieldScriptSetParticleBankDirections(0);
}

void func_80088D18() {
    FieldScriptSetParticleBankDirections(4);
}

void FieldScriptSetParticleBankDirections(int index) {
    g_FieldDefaultParticleBanks[D_800B2384.bankIndex].directions[index].x = FieldScriptArgument1(1, SCRIPT_READ_U8_REL(0x11));
    g_FieldDefaultParticleBanks[D_800B2384.bankIndex].directions[index].z = FieldScriptArgument2(3, SCRIPT_READ_U8_REL(0x11));
    g_FieldDefaultParticleBanks[D_800B2384.bankIndex].directions[index + 1].x = FieldScriptArgument3(5, SCRIPT_READ_U8_REL(0x11));
    g_FieldDefaultParticleBanks[D_800B2384.bankIndex].directions[index + 1].z = FieldScriptArgument4(7, SCRIPT_READ_U8_REL(0x11));
    g_FieldDefaultParticleBanks[D_800B2384.bankIndex].directions[index + 2].x = FieldScriptArgument5(9, SCRIPT_READ_U8_REL(0x11));
    g_FieldDefaultParticleBanks[D_800B2384.bankIndex].directions[index + 2].z = FieldScriptArgument6(0xB, SCRIPT_READ_U8_REL(0x11));
    g_FieldDefaultParticleBanks[D_800B2384.bankIndex].directions[index + 3].x = FieldScriptArgument7(0xD, SCRIPT_READ_U8_REL(0x11));
    g_FieldDefaultParticleBanks[D_800B2384.bankIndex].directions[index + 3].z = FieldScriptArgument8(0xF, SCRIPT_READ_U8_REL(0x11));
    g_FieldScriptMaxInstructionCount += 4;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 0x12;
}

void FieldScriptInitializeParticleBank(void) {
    D_800B2384.bankIndex = FieldScriptVMGetArgument(1);
    g_FieldDefaultParticleBanks[D_800B2384.bankIndex].speedMultiplier = 1;
    g_FieldDefaultParticleBanks[D_800B2384.bankIndex].targetActorID = D_800B2374;
    D_800ADB40 = g_FieldDefaultParticleBanks[D_800B2384.bankIndex].targetActorID;
    g_FieldDefaultParticleBanks[D_800B2384.bankIndex].unk0 = 0;
    g_FieldDefaultParticleBanks[D_800B2384.bankIndex].rotAngle = 0;
    g_FieldDefaultParticleBanks[D_800B2384.bankIndex].max = FieldScriptVMGetArgument(3);
    g_FieldDefaultParticleBanks[D_800B2384.bankIndex].swait = FieldScriptVMGetArgument(5);
    g_FieldDefaultParticleBanks[D_800B2384.bankIndex].ewait = FieldScriptVMGetArgument(7);
    func_8008861C();
    g_FieldScriptMaxInstructionCount += 4;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 9;
}

void FieldScriptSetParticleBankPosition(void) {
    g_FieldDefaultParticleBanks[D_800B2384.bankIndex].pos.vx = FieldScriptArgument1(1, SCRIPT_READ_U8_REL(0xD));
    g_FieldDefaultParticleBanks[D_800B2384.bankIndex].pos.vy  = FieldScriptArgument2(3, SCRIPT_READ_U8_REL(0xD));
    g_FieldDefaultParticleBanks[D_800B2384.bankIndex].pos.vz = FieldScriptArgument3(5, SCRIPT_READ_U8_REL(0xD));
    g_FieldDefaultParticleBanks[D_800B2384.bankIndex].epos.vx = FieldScriptArgument4(7, SCRIPT_READ_U8_REL(0xD));
    g_FieldDefaultParticleBanks[D_800B2384.bankIndex].epos.vy = FieldScriptArgument5(9, SCRIPT_READ_U8_REL(0xD));
    g_FieldDefaultParticleBanks[D_800B2384.bankIndex].epos.vz = FieldScriptArgument6(0xB, SCRIPT_READ_U8_REL(0xD));
    g_FieldScriptMaxInstructionCount += 4;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 0xE;
}

void FieldScriptSetParticleBankPhysics(void) {
    g_FieldDefaultParticleBanks[D_800B2384.bankIndex].speed = FieldScriptArgument1(1, SCRIPT_READ_U8_REL(0xD));
    g_FieldDefaultParticleBanks[D_800B2384.bankIndex].gravity.vx = FieldScriptArgument2(3, SCRIPT_READ_U8_REL(0xD));
    g_FieldDefaultParticleBanks[D_800B2384.bankIndex].gravity.vy = FieldScriptArgument3(5, SCRIPT_READ_U8_REL(0xD));
    g_FieldDefaultParticleBanks[D_800B2384.bankIndex].gravity.vz = FieldScriptArgument4(7, SCRIPT_READ_U8_REL(0xD));
    g_FieldDefaultParticleBanks[D_800B2384.bankIndex].srange = FieldScriptArgument5(9, SCRIPT_READ_U8_REL(0xD));
    g_FieldDefaultParticleBanks[D_800B2384.bankIndex].erange = FieldScriptArgument6(0xB, SCRIPT_READ_U8_REL(0xD));
    g_FieldScriptMaxInstructionCount += 4;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 0xE;
}

void FieldScriptSetParticleBankParameters(void) {
    int flags;
    int flags_2;

    g_FieldDefaultParticleBanks[D_800B2384.bankIndex].pswait = FieldScriptVMGetArgument(1);
    g_FieldDefaultParticleBanks[D_800B2384.bankIndex].pewait = FieldScriptVMGetArgument(3);
    g_FieldDefaultParticleBanks[D_800B2384.bankIndex].shape = FieldScriptVMGetArgument(5);
    flags = FieldScriptVMGetArgument(7);
    flags_2 = (FieldScriptVMGetArgument(9) * 2);
    g_FieldDefaultParticleBanks[D_800B2384.bankIndex].flags = flags | flags_2 | D_800B2378;
    g_FieldDefaultParticleBanks[D_800B2384.bankIndex].unk72 = D_800B237C;
    g_FieldDefaultParticleBanks[D_800B2384.bankIndex].unk74 = D_800B2380;
    g_FieldScriptMaxInstructionCount += 4;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 0xB;
}

void FieldScriptSetParticleBankScale(void) {
    g_FieldDefaultParticleBanks[D_800B2384.bankIndex].scale.vx = FieldScriptArgument1(1, SCRIPT_READ_U8_REL(0x9));
    g_FieldDefaultParticleBanks[D_800B2384.bankIndex].scale.vy = FieldScriptArgument2(3, SCRIPT_READ_U8_REL(0x9));
    g_FieldDefaultParticleBanks[D_800B2384.bankIndex].scale.vz = 0;
    g_FieldDefaultParticleBanks[D_800B2384.bankIndex].scaleDelta.vx = FieldScriptArgument3(5, SCRIPT_READ_U8_REL(0x9));
    g_FieldDefaultParticleBanks[D_800B2384.bankIndex].scaleDelta.vy = FieldScriptArgument4(7, SCRIPT_READ_U8_REL(0x9));
    g_FieldDefaultParticleBanks[D_800B2384.bankIndex].scaleDelta.vz = 0;
    g_FieldScriptMaxInstructionCount += 4;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 0xA;
}

void FieldScriptSetParticleBankColor(void) {
    g_FieldDefaultParticleBanks[D_800B2384.bankIndex].color.r  = FieldScriptArgument1(1, SCRIPT_READ_U8_REL(0xD));
    g_FieldDefaultParticleBanks[D_800B2384.bankIndex].color.g = FieldScriptArgument2(3, SCRIPT_READ_U8_REL(0xD));
    g_FieldDefaultParticleBanks[D_800B2384.bankIndex].color.b = FieldScriptArgument3(5, SCRIPT_READ_U8_REL(0xD));
    g_FieldDefaultParticleBanks[D_800B2384.bankIndex].colorDelta.r = FieldScriptArgument4(7, SCRIPT_READ_U8_REL(0xD));
    g_FieldDefaultParticleBanks[D_800B2384.bankIndex].colorDelta.g = FieldScriptArgument5(9, SCRIPT_READ_U8_REL(0xD));
    g_FieldDefaultParticleBanks[D_800B2384.bankIndex].colorDelta.b = FieldScriptArgument6(0xB, SCRIPT_READ_U8_REL(0xD));
    g_FieldScriptMaxInstructionCount += 4;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 0xE;
}

void FieldScriptParticlesInitialize(void) {
    g_FieldScriptMaxInstructionCount += 4;
    if (D_800ADB8C == 0) {
        FieldInitializeParticleBanks(D_800AFD1C);
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 1;
}

void FieldScriptStopParticleActor(void) {
    g_FieldScriptMaxInstructionCount += 4;
    FieldParticleActorStop(D_800AFD1C, SCRIPT_READ_U8_REL(0x1));
    g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
}
// End of particle handlers

extern s32 D_8005A444[];
extern s32 D_800AFD1C;

void func_80089B54(void) {
    s32 i;
    for (i = 0; i < 3; i++) {
        if (D_8005A444[i] == D_800AFD1C) {
            u16 arg = (u16)FieldScriptVMGetInstructionArgument(1);
            FieldScriptMemoryWriteU16(arg, i);
            g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
            return;
        }
    }
    {
        u16 arg = (u16)FieldScriptVMGetInstructionArgument(1);
        FieldScriptMemoryWriteU16(arg, 0xFF);
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

extern s16 D_800B22E8[];
extern s16 D_800B22EA[];
extern s16 D_800B22EC[];
extern s16 D_800B2300[];
extern s16 D_800B2302[];
extern s16 D_800B2304[];
extern s32 D_800B2318[];

void func_80089BF0(void) {
    s32 idx;
    s32 arg1 = FieldScriptArgument1(1, SCRIPT_READ_U8_REL(0x11));
    idx = arg1 * 4;
    D_800B22E8[idx] = (s16)FieldScriptArgument2(3, SCRIPT_READ_U8_REL(0x11));
    D_800B22EA[idx] = (s16)FieldScriptArgument3(5, SCRIPT_READ_U8_REL(0x11));
    D_800B22EC[idx] = (s16)FieldScriptArgument4(7, SCRIPT_READ_U8_REL(0x11));
    D_800B2300[idx] = (s16)FieldScriptArgument5(9, SCRIPT_READ_U8_REL(0x11));
    D_800B2302[idx] = (s16)FieldScriptArgument6(0xB, SCRIPT_READ_U8_REL(0x11));
    D_800B2304[idx] = (s16)FieldScriptArgument7(0xD, SCRIPT_READ_U8_REL(0x11));
    D_800B2318[arg1] = FieldScriptArgument8(0xF, SCRIPT_READ_U8_REL(0x11));
    g_FieldScriptVMCurActor->scriptInstructionPointer += 0x12;
}

extern s16 D_800B2324[];
extern s16 D_800B2326[];
extern s16 D_800B2328[];
extern s16 D_800B22E2[];

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/field/nonmatchings/main/misc", func_80089DCC);
#else
void func_80089DCC(void) {
    u8 mask;
    s32 arg1, arg2, idx, actorIdx;
    mask = SCRIPT_READ_U8_REL(9);
    arg1 = FieldScriptArgument1(1, mask);
    mask = SCRIPT_READ_U8_REL(9);
    arg2 = FieldScriptArgument2(3, mask);
    idx = arg1 * 4;
    D_800B2324[idx] = (s16)arg2;
    mask = SCRIPT_READ_U8_REL(9);
    D_800B2328[idx] = (s16)FieldScriptArgument3(5, mask);
    mask = SCRIPT_READ_U8_REL(9);
    D_800B2326[idx] = (s16)FieldScriptArgument4(7, mask);
    actorIdx = FieldScriptVMGetActorIndex(0xA);
    if (actorIdx == 0xFF) {
        D_800B22E2[arg1] = -1;
    } else {
        D_800B22E2[arg1] = (s16)actorIdx;
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 0xB;
}
#endif /* XENO_PC_PORT */

extern s16 D_800B22E0[];

void func_80089F18(void) {
    D_800B22E0[0] = FieldScriptVMGetArgument(1);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

extern u8 D_800B21D2[];

void func_80089F54(void) {
    D_800B21D2[0] = FieldScriptVMGetArgument(1) - 0x80;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

extern s32 D_800AFE84;

void func_80089F94(void) {
    D_800AFE84 = SCRIPT_READ_U8_REL(1);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
}

extern s16 D_800B0080, D_800B0082, D_800B0084, D_800B0086, D_800B0088, D_800B008A, D_800B008C, D_800B008E;

void func_80089FD0(void) {
#ifdef XENO_PC_PORT
    D_800B0080 = (s16)FieldScriptVMGetInstructionArgument(1);
    D_800B0082 = (s16)FieldScriptVMGetInstructionArgument(3);
    {
        s16 arg5 = (s16)FieldScriptVMGetInstructionArgument(5);
        D_800B0084 = arg5;
        if ((s16)arg5 == 0) {
            D_800B0084 = arg5 + 1;
        }
    }
    D_800B0086 = (s16)FieldScriptVMGetInstructionArgument(7);
    D_800B0088 = 0;
    D_800B008A = (s16)FieldScriptVMGetInstructionArgument(9);
    D_800B008C = (s16)FieldScriptVMGetInstructionArgument(11);
    D_800B008E = (s16)FieldScriptVMGetInstructionArgument(13);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 15;
#else
    s16 temp_v0;

    D_800B0080 = FieldScriptVMGetInstructionArgument(1);
    D_800B0082 = FieldScriptVMGetInstructionArgument(3);
    temp_v0 = FieldScriptVMGetInstructionArgument(5);
    D_800B0084 = temp_v0;
    if ((temp_v0 << 0x10) == 0) {
        D_800B0084 = temp_v0 + 1;
    }
    D_800B0086 = FieldScriptVMGetInstructionArgument(7);
    D_800B0088 = 0;
    D_800B008A = FieldScriptVMGetInstructionArgument(9);
    D_800B008C = FieldScriptVMGetInstructionArgument(0xB);
    (*(s16*)&D_800B008E) = FieldScriptVMGetInstructionArgument(0xD);
    *(u16 *)((s8*)(g_FieldScriptVMCurActor) + 0xCC) = (u16) (*(u16 *)((s8*)(g_FieldScriptVMCurActor) + 0xCC) + 0xF);
#endif
}

extern s32 D_800B0090, D_800B0094, D_800B0098;

void func_8008A08C(void) {
#ifdef XENO_PC_PORT
    u8 mask = SCRIPT_READ_U8_REL(7);
    D_800B0090 = FieldScriptArgument1(1, mask);
    mask = SCRIPT_READ_U8_REL(7);
    D_800B0098 = FieldScriptArgument2(3, mask);
    mask = SCRIPT_READ_U8_REL(7);
    D_800B0094 = FieldScriptArgument3(5, mask);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 8;
#else
    D_800B0090 = FieldScriptArgument1(1, SCRIPT_READ_U8_REL_RAW(7));
    D_800B0098 = FieldScriptArgument2(3, SCRIPT_READ_U8_REL_RAW(7));
    D_800B0094 = FieldScriptArgument3(5, SCRIPT_READ_U8_REL_RAW(7));
    *(u16*)((u8*)g_FieldScriptVMCurActor + 0xCC) += 8;
#endif
}

extern u8 D_800B00A0, D_800B00A1, D_800B00A2, D_800B00A4, D_800B00A5, D_800B00A6, D_800B00A8, D_800B00A9, D_800B00AA;
extern s16 D_800B00AC, D_800B00AE, D_800B00B0, D_800B00B2;

#ifndef XENO_PC_PORT
void func_8008A148(void) {
    D_800B00A0 = FieldScriptVMGetArgument(1);
    D_800B00A1 = FieldScriptVMGetArgument(3);
    D_800B00A2 = FieldScriptVMGetArgument(5);
    D_800B00A4 = FieldScriptVMGetArgument(7);
    D_800B00A5 = FieldScriptVMGetArgument(9);
    D_800B00A6 = FieldScriptVMGetArgument(0xB);
    D_800B00A8 = FieldScriptVMGetArgument(0xD);
    D_800B00A9 = FieldScriptVMGetArgument(0xF);
    D_800B00AA = FieldScriptVMGetArgument(0x11);
    D_800B00AC = FieldScriptVMGetArgument(0x13);
    D_800B00AE = FieldScriptVMGetArgument(0x15);
    D_800B00B0 = FieldScriptVMGetArgument(0x17);
    /* scriptInstructionPointer; the port keeps the typed field below */
    *(u16 *)((u8 *)g_FieldScriptVMCurActor + 0xCC) += 0x19;
    D_800B00B2 = 1;
}
#else
void func_8008A148(void) {
    D_800B00A0 = (u8)FieldScriptVMGetArgument(1);
    D_800B00A1 = (u8)FieldScriptVMGetArgument(3);
    D_800B00A2 = (u8)FieldScriptVMGetArgument(5);
    D_800B00A4 = (u8)FieldScriptVMGetArgument(7);
    D_800B00A5 = (u8)FieldScriptVMGetArgument(9);
    D_800B00A6 = (u8)FieldScriptVMGetArgument(11);
    D_800B00A8 = (u8)FieldScriptVMGetArgument(13);
    D_800B00A9 = (u8)FieldScriptVMGetArgument(15);
    D_800B00AA = (u8)FieldScriptVMGetArgument(17);
    D_800B00AC = (s16)FieldScriptVMGetArgument(19);
    D_800B00AE = (s16)FieldScriptVMGetArgument(21);
    D_800B00B0 = (s16)FieldScriptVMGetArgument(23);
    D_800B00B2 = 1;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 25;
}
#endif /* XENO_PC_PORT */

extern s32 D_800ADB88;

void func_8008A244(void) {
    if (D_800ADB88 == 0) {
        g_FieldScriptVMCurActor->scriptInstructionPointer += 1;
    } else {
        g_FieldScriptVMCurActor->scriptInstructionPointer -= 1;
    }
    D_800B00C0 = 1;
}

extern s32 D_800B06A0;

void func_8008A2A0(void) {
    FieldScriptMemoryWriteU16(SCRIPT_IMM_ARG(1), D_800B06A0);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

extern s32 D_800B1F74;
extern s32 func_8008A558(void);
extern void func_800A915C(void);
extern s32 FieldLoadTIMWithClut(void* data, s32 ofsX, s32 ofsY, s32 width, s32 height, s32 clutX, s32 clutY);

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/field/nonmatchings/main/misc", func_8008A2E8);
#else
/* Retail 8008A2E8: sub-op 0 hands the HeapAlloc result straight to the
 * archive read; sub-op 1 calls FieldLoadTIMWithClut(buf, x=arg2, y=arg3,
 * clutX=0, clutY, 0, 0) (real misc3.c prototype takes shorts). */
void func_8008A2E8(void) {
    u8 subOp;
    if (func_8008A558() == -1) {
        D_800B00C0 = 1;
        g_FieldScriptVMCurActor->scriptInstructionPointer -= 1;
        return;
    }
    subOp = SCRIPT_READ_U8_REL(1);
    if (subOp == 0) {
        s32 fileId = FieldScriptArgument1(5, SCRIPT_READ_U8_REL(0xD));
        void* buf;
        ArchiveSetIndex(4, 0);
        fileId += 0x7FB;
        buf = HeapAlloc(ArchiveDecodeAlignedSize(fileId), 0);
        D_800B1F74 = (s32)(uintptr_t)buf;
        ArchiveReadFileToBuffer(fileId, buf, 0, 0x80);
        g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
    } else if (subOp == 1) {
        s32 arg4 = FieldScriptArgument4(8, SCRIPT_READ_U8_REL(0xA));
        s32 clutY;
        s32 arg2;
        s32 arg3;
        if (arg4 == 0xFF) {
            clutY = -1;
        } else {
            clutY = arg4 + 0xE8;
        }
        arg2 = FieldScriptArgument2(4, SCRIPT_READ_U8_REL(0xA));
        arg3 = FieldScriptArgument3(6, SCRIPT_READ_U8_REL(0xA));
        if (arg3 >= 0x100 && arg2 >= 0x2C0) {
            func_800A915C();
        }
        ((void (*)(u_long*, short, short, short, short, short, short))FieldLoadTIMWithClut)(
            (u_long*)(uintptr_t)D_800B1F74, (short)arg2, (short)arg3, 0, (short)clutY, 0, 0);
        g_FieldScriptVMCurActor->scriptInstructionPointer += 0xB;
    } else {
        HeapFree((void*)(uintptr_t)D_800B1F74);
        g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
    }
    D_800B00C0 = 1;
}
#endif /* XENO_PC_PORT */

void func_8008A4E0(void) {}
void func_8008A4E8(void) {}
void func_8008A4F0(void) {}
void func_8008A4F8(void) {}
void func_8008A500(void) {}
void func_8008A508(void) {}
void func_8008A510(void) {}
void func_8008A518(void) {}

void func_8008A520(void) {
    while (func_8008A558()) {
        Vsync(0);
    }
}

extern s32 D_800ADB2C;
extern s32 D_800ADB90;
extern s32 D_800ADB1C;

s32 func_8008A558(void) {
    if (D_800ADB2C != 0) {
        return -1;
    }
    if (ArchiveDataSync() != 0) {
        return -1;
    }
    ArchiveCdDataSync(0);
    return 0;
}

void func_8008A5A0(void) {
    if (SCRIPT_READ_U8_REL(1) == 0) {
        func_8003633C(0);
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 1;
}

extern s16 D_800B21D4[];

void func_8008A604(void) {
    D_800B21D4[0] = FieldScriptVMGetArgument(1);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

void func_8008A640(void) {
    s32 charId = func_8008CF3C(FieldScriptVMGetArgument(3));
    if (charId != 0xFF) {
        s32 arg1 = FieldScriptVMGetArgument(1);
        u8* pChar = (u8*)g_pGameState + charId * 164;
        u8 currentVal = *(pChar + 0x2E3);
        s32 newVal = arg1 - currentVal;
        if (newVal < 0) newVal = 0;
        *(pChar + 0x2E4) = (u8)newVal;
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
}

void func_8008A6E0(void) {
    s32 charId = func_8008CF3C(FieldScriptVMGetArgument(3));
    u16 dest;
    s32 val;
    if (charId != 0xFF) {
        u8* pChar = (u8*)g_pGameState + charId * 164;
        val = (s32)pChar[0x2E3] + (s32)pChar[0x2E4];
        dest = (u16)FieldScriptVMGetInstructionArgument(1);
        FieldScriptMemoryWriteU16(dest, val);
    } else {
        dest = (u16)FieldScriptVMGetInstructionArgument(1);
        FieldScriptMemoryWriteU16(dest, 0);
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
}

s32 func_8008A790(s32 value, s32* outIndex) {
    s32 i;
    for (i = 0; i < MAX_PARTY_MEMBERS; i++) {
        if (g_GamePartyMembers[i] == value) {
            return -1;
        }
        if (g_GamePartyMembers[i] == 0xFF) {
            *outIndex = i;
            return 0;
        }
    }
    return -1;
}

/* Skin-archive kickoff for a joining member (asm 8008A7DC): stage the
 * character skin (field maps, archive charId+5) or gear skin (gear maps,
 * GameCharacterGetGearID+0x10+5; 0xFF -> gear 0x10) into a heap staging
 * buffer via an async archive read, record slot/charId/buffer in
 * D_800ADBCC/C8/C0, and mark the load in flight (D_800ADBC4 = 1). */
extern s32 D_800ADB1C;
extern s32 D_800ADBC4;
extern s32 D_800ADBC8;
extern s32 D_800ADBCC;
extern void* D_800ADBC0;
extern s32 g_GameSceneMapNum;
extern s32 g_GamePartyMemberSkins[];
extern s32 GameCharacterGetGearID(s32 charId);

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/field/nonmatchings/main/misc", func_8008A7DC);
#else
void func_8008A7DC(s32 charId, s32 slot) {
    s32 fileId;
    void* buf;

    D_800ADBCC = slot;
    D_800ADBC8 = charId;
    ArchiveSetIndex(4, 0);
    if (D_800ADB1C == 0) {
        ArchiveCdDataSync(0);
    }
    if ((g_GameSceneMapNum & 0xC000) == 0) {
        fileId = (s32)((u32)charId + 5);
        g_GamePartyMemberSkins[slot] = charId;
        buf = HeapAlloc(ArchiveDecodeAlignedSize(fileId), 0);
        D_800ADBC0 = buf;
    } else {
        s32 gearId = GameCharacterGetGearID(charId);

        if (gearId == 0xFF) {
            gearId = 0;
        }
        /* addiu wraps: no host signed-overflow UB for any callee value. */
        gearId = (s32)((u32)gearId + 0x10);
        fileId = (s32)((u32)gearId + 5);
        buf = HeapAlloc(ArchiveDecodeAlignedSize(fileId), 0);
        D_800ADBC0 = buf;
        g_GamePartyMemberSkins[slot] = gearId;
    }
    ArchiveReadFileToBuffer(fileId, buf, 0, 0x80);
    if (D_800ADB1C == 0) {
        ArchiveCdDataSync(0);
    }
    D_800ADBC4 = 1;
}
#endif /* XENO_PC_PORT */

void func_8008A93C(void) {
    g_FieldScriptVMCurActor->unkAnimationId = ~SCRIPT_READ_U8_REL(1);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
}

void func_8008A974(void) {
    func_8008A93C();
    g_FieldScriptVMCurActor->flags &= 0xFFFEFFFF;
}

void func_8008A9AC(void) {
    if (func_8008A558() == 0) {
        D_800ADB90 = 0;
        SpriteSetSpecialAnimFile(
            (SpriteData*)(uintptr_t)g_FieldActors[D_800AFD1C].pSpriteData,
            (void*)(uintptr_t)g_FieldScriptVMCurActor->unk120);
        g_FieldScriptVMCurActor->scriptInstructionPointer += 1;
    } else {
        g_FieldScriptVMCurActor->scriptInstructionPointer -= 1;
    }
    D_800B00C0 = 1;
}

void func_8008AA60(void) {
    if (g_FieldScriptVMCurActor->unk124 != -1) {
        HeapFree((void*)(uintptr_t)g_FieldScriptVMCurActor->unk120);
        g_FieldScriptVMCurActor->unk124 = -1;
    }
    D_800B00C0 = 1;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 1;
}

extern void* D_800AFD08;
extern s32 D_800AFD0C;
extern s32 D_800AFD18;
extern s32 D_80062518[];
#ifdef XENO_PC_PORT
extern void* D_80062524;
extern void* D_800595AC;
#else
extern s32 D_80062524[];
extern s32 D_800595AC;
#endif
extern s32 D_8004F370;
extern void SoundLoadWdsFile(void* pData, s32 a1);
extern void SoundFreeWdsEntry(s32 handle);
extern void func_8003BDFC(s32 a0);

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/field/nonmatchings/main/misc", func_8008AACC);
#else
/* Retail 8008AACC: the slot table receives SoundLoadWdsFile's return value
 * (32-bit retail word); bit-7 file ids use directory 0x2C/1 with (id & 0x7F)
 * + 0x1F unless D_8004F370 == 1, which loads file 4 + 2 from 0x1C/0.
 * PC divergence kept: the slot-3 publish reads the host-width shadow
 * D_80062524 (retail re-reads D_80062518[3] = the entry just stored). */
void func_8008AACC(void) {
    if (func_8008A558() != 0) {
        D_800B00C0 = 1;
        g_FieldScriptVMCurActor->scriptInstructionPointer -= 1;
        return;
    }
    if (SCRIPT_READ_U8_REL(1) == 1) {
        void* entry = ((void* (*)(void*, s32))SoundLoadWdsFile)(D_800AFD08, 0);
        D_80062518[D_800AFD18] = (s32)(uintptr_t)entry;
        func_8003BDFC(0x10);
        HeapFree(D_800AFD08);
        if (D_800AFD18 == 3) {
            D_800595AC = D_80062524;
        }
        D_800B00C0 = 1;
        g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
    } else {
        s32 slot = FieldScriptVMGetArgument(2);
        s32 entry = D_80062518[slot];
        s32 arg4;
        D_800AFD18 = slot;
        SoundFreeWdsEntry(entry);
        arg4 = FieldScriptVMGetArgument(4);
        D_800AFD0C = arg4;
        if ((arg4 & 0x80) && D_8004F370 != 1) {
            D_800AFD0C = (arg4 & 0x7F) + 0x1F;
            ArchiveSetIndex(0x2C, 1);
        } else {
            if (arg4 & 0x80) {
                D_800AFD0C = 4;
            }
            ArchiveSetIndex(0x1C, 0);
            D_800AFD0C += 2;
        }
        D_800AFD08 = HeapAlloc(ArchiveDecodeAlignedSize(D_800AFD0C), 0);
        ArchiveReadFileToBuffer(D_800AFD0C, D_800AFD08, 0, 0x80);
        ArchiveSetIndex(4, 0);
        g_FieldScriptVMCurActor->scriptInstructionPointer += 6;
    }
}
#endif /* XENO_PC_PORT */

void func_8008ACE8(void) {
    s32 arg;
    s32 archiveIndex;
    s32 archiveSize;
    void* pBuffer;

    if (D_800ADB90 == 0 && D_800ADB2C == 0) {
        if (func_8008A558() != 0) {
            D_800B00C0 = 1;
            g_FieldScriptVMCurActor->scriptInstructionPointer -= 1;
            return;
        }

        if (g_FieldScriptVMCurActor->unk124 != -1) {
            HeapFree((void*)(uintptr_t)g_FieldScriptVMCurActor->unk120);
            g_FieldScriptVMCurActor->unk124 = -1;
        }

        arg = FieldScriptVMGetArgument(1);
        archiveIndex = arg;
        ArchiveSetIndex(4, 0);
        archiveIndex += 0x77A;
        archiveSize = ArchiveDecodeAlignedSize(archiveIndex);
        g_FieldScriptVMCurActor->unk124 = archiveIndex;
        pBuffer = HeapAlloc(archiveSize + 8, 0);
        g_FieldScriptVMCurActor->unk120 = (u32)(uintptr_t)pBuffer;
        ArchiveReadFileToBuffer(
            archiveIndex,
            pBuffer,
            0,
            0x80);
        if (D_800ADB1C == 0) {
            ArchiveCdDataSync(0);
        }
        D_800ADB90 = 1;
        g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
    } else {
        g_FieldScriptVMCurActor->scriptInstructionPointer -= 1;
    }
    D_800B00C0 = 1;
}

void func_8008AE5C(void) {
    if (SCRIPT_READ_U8_REL(1) == 0) {
        g_FieldScriptVMCurActor->flags |= 0x20000;
    } else {
        g_FieldScriptVMCurActor->flags &= ~0x20000;
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
}

extern s16 D_800B221C[];

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/field/nonmatchings/main/misc", func_8008AEC8);
#else
void func_8008AEC8(void) {
    u8 mask;
    s32 arg1, arg2;

    mask = SCRIPT_READ_U8_REL(9);
    arg1 = FieldScriptArgument1(1, mask);
    mask = SCRIPT_READ_U8_REL(9);
    arg2 = FieldScriptArgument2(3, mask);

    {
        /* Retail (differential-tested): operand 1 is the record index, operand
         * 2 the first halfword; the old port arm had the two swapped. */
        s32 idx = arg1 * 3; /* stride 6 = 3 halfwords */
        D_800B221C[idx] = (s16)arg2;
        mask = SCRIPT_READ_U8_REL(9);
        D_800B221C[idx + 1] = (s16)FieldScriptArgument3(5, mask);
        mask = SCRIPT_READ_U8_REL(9);
        D_800B221C[idx + 2] = (s16)FieldScriptArgument4(7, mask);
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 10;
}
#endif /* XENO_PC_PORT */

extern s16 D_800B223C[];

#ifndef XENO_PC_PORT
/* func_8008AFD8: plain array stores; the first operand is the index, the second
 * the value (the port body has these swapped). */
#endif
void func_8008AFD8(void) {
#ifdef XENO_PC_PORT
    u8 mask;
    s32 arg1, arg2;

    mask = SCRIPT_READ_U8_REL(9);
    arg1 = FieldScriptArgument1(1, mask);
    mask = SCRIPT_READ_U8_REL(9);
    arg2 = FieldScriptArgument2(3, mask);

    D_800B223C[arg2] = (s16)arg1;
    mask = SCRIPT_READ_U8_REL(9);
    D_800B223C[arg2 + 3] = (s16)FieldScriptArgument3(5, mask);
    mask = SCRIPT_READ_U8_REL(9);
    D_800B223C[arg2 + 6] = (s16)FieldScriptArgument4(7, mask);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 10;
#else
    s32 idx = FieldScriptArgument1(1, SCRIPT_READ_U8_REL(9));
    D_800B223C[idx] = FieldScriptArgument2(3, SCRIPT_READ_U8_REL(9));
    D_800B223C[idx + 3] = FieldScriptArgument3(5, SCRIPT_READ_U8_REL(9));
    D_800B223C[idx + 6] = FieldScriptArgument4(7, SCRIPT_READ_U8_REL(9));
    g_FieldScriptVMCurActor->scriptInstructionPointer += 10;
#endif
}

extern u8 D_800B225C;
extern u8 D_800B225D;
extern u8 D_800B225E;

void func_8008B0E8(void) {
#ifdef XENO_PC_PORT
    D_800B225C = FieldScriptVMGetArgument(1);
    D_800B225D = FieldScriptVMGetArgument(3);
    D_800B225E = FieldScriptVMGetArgument(5);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 7;
#else
    (*(s8*)&D_800B225C) = FieldScriptVMGetArgument(1);
    (*(s8*)&D_800B225D) = FieldScriptVMGetArgument(3);
    (*(s8*)&D_800B225E) = FieldScriptVMGetArgument(5);
    *(u16 *)((s8*)(g_FieldScriptVMCurActor) + 0xCC) = (u16) (*(u16 *)((s8*)(g_FieldScriptVMCurActor) + 0xCC) + 7);
#endif
}

extern s16 D_800B21B4[];

void func_8008B144(void) {
    D_800B21B4[0] = FieldScriptVMGetArgument(1);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

extern s32 D_800ADB1C;
extern u16 D_800B21E4[];
extern void func_801E8330(s32 slot, s32 unused, s32 anim);

/* Field-script opcode: when D_800ADB1C is set, drive an object's anim via the
 * field archive 0x6B9 entry func_801E8330 and record the anim into
 * D_800B21E4[arg1]. Always IP += 5. */

void func_8008B180(void) {
#ifdef XENO_PC_PORT
    if (D_800ADB1C != 0) {
        s32 arg1 = FieldScriptVMGetArgument(1);
        s32 arg3 = FieldScriptVMGetArgument(3);
        func_801E8330(arg1 & 0xFFFF, 0, arg3);
        arg1 = FieldScriptVMGetArgument(1);
        arg3 = FieldScriptVMGetArgument(3);
        D_800B21E4[arg1] = (u16)arg3;
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
#else
    s32 temp_s0_2;
    s32 temp_s0;

    if (D_800ADB1C != 0) {
        temp_s0 = FieldScriptVMGetArgument(1) & 0xFFFF;
        func_801E8330(temp_s0, 0, FieldScriptVMGetArgument(3));
        temp_s0_2 = FieldScriptVMGetArgument(1);
        D_800B21E4[temp_s0_2] = FieldScriptVMGetArgument(3);
    }
    *(u16 *)((s8*)(g_FieldScriptVMCurActor) + 0xCC) = (u16) (*(u16 *)((s8*)(g_FieldScriptVMCurActor) + 0xCC) + 5);
#endif
}

void func_8008B210(void) {
    g_FieldScriptVMCurActor->unk11E = FieldScriptVMGetArgument(1);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

void func_8008B248(void) {
    FieldFadeSetParameters(1,
        FieldScriptVMGetArgument(9),
        FieldScriptVMGetArgument(3),
        FieldScriptVMGetArgument(5),
        FieldScriptVMGetArgument(7),
        FieldScriptVMGetArgument(1));
    g_FieldScriptVMCurActor->scriptInstructionPointer += 0xB;
}

#ifdef XENO_PC_PORT
/* Defined in src/field/effects/distortion.c alongside the only writer that
 * clears distortion.isActive; see the FE27 case 1 diagnostic below. */
extern unsigned long g_XenoDistortionDrawCalls;
extern int PcPort_DistortionDiagEnabled(void);
#endif

void func_8008B2F0(void) {
    FieldDistortionInitialize(0);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 0xF;
}

void func_8008B328(void) {
    switch (SCRIPT_READ_U8_REL(1)) {
        case 0:
            FieldDistortionSetTarget(0, 0, 0, 0, 0, 0, FieldScriptVMGetArgument(2));
            g_FieldEffects.distortion.isFinished = 1;
            g_FieldScriptVMCurActor->scriptInstructionPointer += 4;
            break;
        case 1:
#ifdef XENO_PC_PORT
            /* DIAGNOSTIC (XENO_DISTORTION_DIAG=1): this wait hangs field 14
             * after the dream battle.  Print the three inputs that decide it
             * plus the draw counter, rate-limited so a long wait stays
             * readable.  Removal: delete this block. */
            if (g_FieldEffects.distortion.isActive != 0 &&
                PcPort_DistortionDiagEnabled()) {
                static unsigned long s_waits;
                if ((s_waits++ % 60) == 0) {
                    printf("[xeno-port][distortion] FE27 wait #%lu "
                           "isActive=%d isFinished=%d duration=%d "
                           "v1=%d v2=%d draws=%lu\n",
                           s_waits, (int)g_FieldEffects.distortion.isActive,
                           (int)g_FieldEffects.distortion.isFinished,
                           (int)g_FieldEffects.distortion.duration,
                           (int)g_FieldEffects.distortion.v1,
                           (int)g_FieldEffects.distortion.v2,
                           g_XenoDistortionDrawCalls);
                    fflush(stdout);
                }
            }
#endif
            if (g_FieldEffects.distortion.isActive == 0) {
                g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
            } else {
                g_FieldScriptVMCurActor->scriptInstructionPointer -= 1;
            }
            break;
        case 2:
            g_FieldEffects.distortion.isActive = 0;
            g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
            break;
        case 3:
            FieldDistortionFree();
            g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
            break;
    }

    D_800B00C0 = 1;
}

extern s16 D_800B2184, D_800B2186, D_800B2188;

void func_8008B45C(void) {
#ifdef XENO_PC_PORT
    u8 mask = SCRIPT_READ_U8_REL(7);
    D_800B2184 = (s16)FieldScriptArgument1(1, mask);
    mask = SCRIPT_READ_U8_REL(7);
    D_800B2188 = (s16)FieldScriptArgument2(3, mask);
    mask = SCRIPT_READ_U8_REL(7);
    D_800B2186 = (s16)FieldScriptArgument3(5, mask);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 8;
#else
    D_800B2184 = FieldScriptArgument1(1, SCRIPT_READ_U8_REL_RAW(7));
    D_800B2188 = FieldScriptArgument2(3, SCRIPT_READ_U8_REL_RAW(7));
    D_800B2186 = FieldScriptArgument3(5, SCRIPT_READ_U8_REL_RAW(7));
    *(u16*)((u8*)g_FieldScriptVMCurActor + 0xCC) += 8;
#endif
}

void func_8008B518(void) {
    u8 mask = SCRIPT_READ_U8_REL(7);
    g_Scene.camRotation.vx = (s16)FieldScriptArgument1(1, mask);
    mask = SCRIPT_READ_U8_REL(7);
    g_Scene.camRotation.vz = (s16)FieldScriptArgument2(3, mask);
    mask = SCRIPT_READ_U8_REL(7);
    g_Scene.camRotation.vy = (s16)FieldScriptArgument3(5, mask);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 8;
}

/* Retail FE1B, 0x8008B5D4-0x8008B894. Offset every polygon coordinate in the
 * current actor-model packet page and publish those changed coordinate bytes
 * to the alternate double-buffer page. The model's command stream supplies
 * the primitive count (header high halfword), shape (low-byte bit 3), and two
 * 32-bit descriptor words per primitive. C4/C8 control records consume only
 * their four-byte header and do not advance either packet cursor. */
#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/field/nonmatchings/main/misc", func_8008B5D4);
#else
void func_8008B5D4(void) {
    u32 modelData = g_FieldActors[D_800AFD1C].pModelData;
    s32 page = g_FieldCurRenderContextIndex;
    u8* cur = (u8*)(uintptr_t)*(u32*)(uintptr_t)(modelData + ((u32)page << 2) + 8);
    u32 modelHeader = *(u32*)(uintptr_t)(modelData + 4);
    u8* alt = (u8*)(uintptr_t)*(u32*)(uintptr_t)(modelData + (((u32)page + 1) & 1) * 4 + 8);
    u8* command = (u8*)(uintptr_t)*(u32*)(uintptr_t)(modelHeader + 0x10);
    s16 xOffset = FieldScriptVMGetInstructionArgumentS16(1);
    s16 yOffset = FieldScriptVMGetInstructionArgumentS16(3);
    s32 commandCount = *(u16*)(uintptr_t)(modelHeader + 6);

    /* Retail byte order per primitive: offset the current page's X/Y bytes,
     * then copy them to the alternate page (the copies interleave loads from
     * cur with stores to alt, so overlapping pages behave as on retail). */
    while (commandCount > 0) {
        u32 header = *(u32*)command;
        u8 opcode = (u8)header;
        s32 primitiveCount = (s32)(header >> 16);
        s32 i;
        command += 4;

        if (opcode != 0xC4 && opcode != 0xC8) {
            if (!(header & 8)) {
                for (i = 0; i < primitiveCount; i++) {
                    cur[0x0C] = (u8)(cur[0x0C] + xOffset);
                    cur[0x1C] = (u8)(cur[0x1C] + xOffset);
                    cur[0x14] = (u8)(cur[0x14] + xOffset);
                    cur[0x15] = (u8)(cur[0x15] + yOffset);
                    cur[0x0D] = (u8)(cur[0x0D] + yOffset);
                    cur[0x1D] = (u8)(cur[0x1D] + yOffset);
#ifndef FIELD_VM_AUDIT_MUTANT_PRIM_SKIP_ALT_COPY
                    alt[0x0C] = cur[0x0C];
                    alt[0x14] = cur[0x14];
                    alt[0x1C] = cur[0x1C];
                    alt[0x0D] = cur[0x0D];
                    alt[0x15] = cur[0x15];
                    alt[0x1D] = cur[0x1D];
#endif
                    command += 8;
#ifdef FIELD_VM_AUDIT_MUTANT_PRIM_TRIANGLE_STRIDE_28
                    cur += 0x28;
                    alt += 0x28;
#else
                    cur += 0x20;
                    alt += 0x20;
#endif
                }
            } else {
                for (i = 0; i < primitiveCount; i++) {
                    cur[0x0C] = (u8)(cur[0x0C] + xOffset);
                    cur[0x1C] = (u8)(cur[0x1C] + xOffset);
                    cur[0x14] = (u8)(cur[0x14] + xOffset);
                    cur[0x0D] = (u8)(cur[0x0D] + yOffset);
                    cur[0x24] = (u8)(cur[0x24] + xOffset);
                    cur[0x1D] = (u8)(cur[0x1D] + yOffset);
                    cur[0x15] = (u8)(cur[0x15] + yOffset);
                    cur[0x25] = (u8)(cur[0x25] + yOffset);
#ifndef FIELD_VM_AUDIT_MUTANT_PRIM_SKIP_ALT_COPY
                    alt[0x0C] = cur[0x0C];
                    alt[0x14] = cur[0x14];
                    alt[0x1C] = cur[0x1C];
                    alt[0x24] = cur[0x24];
                    alt[0x0D] = cur[0x0D];
                    alt[0x15] = cur[0x15];
                    alt[0x1D] = cur[0x1D];
                    alt[0x25] = cur[0x25];
#endif
                    command += 8;
                    cur += 0x28;
                    alt += 0x28;
                }
            }
        }
        commandCount--;
    }

    g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
}
#endif /* XENO_PC_PORT */

/* --- Party-join chain (the Citan-join machinery) ------------------------
 * add-member opcodes (func_8008BC80 arg-variant / func_8008BDD8 immediate)
 * -> skin-archive kickoff (func_8008A7DC, D_800ADBC4=1 "loading")
 * -> wait/complete opcode (func_8008B894: sync, LZSS into the party
 *    buffer, activate via func_8008B978, D_800ADBC4=0xFF re-arm). */

extern s32 D_800ADB1C;
extern s32 D_800ADB2C;
extern s32 D_800ADBC4;
extern s32 D_800ADBC8;
extern s32 D_800ADBCC;
extern void* D_800ADBC0;
extern s32 D_800ADBFC;
extern void* D_800B06B8;
extern s32 D_800AFD1C;
extern s32 D_800AFFEC;
extern s32 D_800B00C0;
extern s32 D_8006F990[];
extern s32 g_GamePartyMembers[];
extern void* g_PartyDataBuffers[];
extern s32 g_PlayerActorIndex;
extern s32 func_8008A558(void);
extern s32 func_8008A790(s32 value, s32* outIndex);
extern void func_8008A7DC(s32 charId, s32 slot);
extern s32 GameCharacterGetGearID(s32 charId);
extern void LZSSDecompress(void* src, void* dst);
extern s32 ArchiveDataSync(void);
extern void func_80080A74(s32 actorIndex);
extern void FieldActorCopyPlacement(s32 actorIndex, s32 srcActorIndex);
extern void func_80077268(void);
extern u_short FieldScriptGetBytecodeOffset(int scriptIndex, int routineIndex);
extern void FieldScriptVMRun(s32 maxInstructionCount);

/* Wait-for-skin-load opcode (asm 8008B894): while the archive read is in
 * flight, retry (IP-1 + yield); once synced, LZSS-decompress the staged
 * skin into g_PartyDataBuffers[slot], free the staging buffer, activate the
 * joined member (func_8008B978), re-arm D_800ADBC4=0xFF, IP+1 + yield. */
void func_8008B894(void) {
    if (D_800ADBC4 != 0xFF) {
        if (ArchiveDataSync() == 0) {
            ArchiveCdDataSync(0);
            LZSSDecompress(D_800ADBC0, g_PartyDataBuffers[D_800ADBCC]);
            HeapFree(D_800ADBC0);
            func_8008B978(D_800ADBC8);
            D_800ADBC4 = 0xFF;
            D_800B00C0 = 1;
            g_FieldScriptVMCurActor->scriptInstructionPointer += 1;
            return;
        }
    }
    D_800B00C0 = 1;
    g_FieldScriptVMCurActor->scriptInstructionPointer -= 1;
}

/* Activate a joined member (asm 8008B978): set the character's roster bit
 * (gamestate+0x1D30), and for mid-game joins (D_800ADB1C) find the actor
 * declaring this character (script-0 starting with `0x16 <charId>`),
 * re-init it (func_80080A74), place it at the player, run its script 0,
 * settle (func_80077268), and on gear maps re-run the party slot's gear
 * actor too.  The VM context is saved/restored around the nested runs. */
#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/field/nonmatchings/main/misc", func_8008B978);
#else
void func_8008B978(s32 charId) {
    ActorData* savedCur = g_FieldScriptVMCurActor;
    void* savedB06B8 = D_800B06B8;
    s32 savedB00C0;
    s32 savedAFD1C;
    s32 savedMax;
    u16 savedIP;
    s32 i;

    /* Retail sllv: shift count is the low five bits. */
    *(u16*)((u8*)g_pGameState + 0x1D30) |= (u16)(1u << (D_800ADBC8 & 31));
    if (D_800ADB1C == 0) {
        return;
    }

    savedIP = savedCur->scriptInstructionPointer;
    savedB00C0 = D_800B00C0;
    savedAFD1C = D_800AFD1C;
    savedMax = g_FieldScriptMaxInstructionCount;

    for (i = 0; i < D_800ADBFC; i++) {
        u16 off0 = FieldScriptGetBytecodeOffset(i, 0);
        u8* pc = (u8*)g_FieldScriptVMCurScriptData + off0;
        u16 off0c;

        if (pc[0] != 0x16 || pc[1] != charId) {
            continue;
        }

        D_800B06B8 = &g_FieldActors[i];
        g_FieldScriptVMCurActor =
            (ActorData*)(uintptr_t)g_FieldActors[i].pActorData;
        func_80080A74(i);
        ((ActorData*)(uintptr_t)g_FieldActors[i].pActorData)
            ->scriptInstructionPointer = off0;
        D_800AFD1C = i;
        {
            u16 off0b = FieldScriptGetBytecodeOffset(i, 0);
            ActorData* cur = g_FieldScriptVMCurActor;
            D_800AFFEC = 0;
            cur->scriptInstructionPointer = off0b;
        }
        /* Retail 8008BADC reads the offset a second time (kept in $s1 for
         * the gear-map path). */
        off0c = FieldScriptGetBytecodeOffset(i, 0);
        FieldActorCopyPlacement(i, g_PlayerActorIndex);
        FieldScriptVMRun(0xFFFF);
        func_80077268();
        *(u16*)((u8*)g_pGameState + 0x1D30) |= (u16)(1u << (D_800ADBC8 & 31));

        if (g_GameSceneMapNum & 0xC000) {
            s32 gi = D_8006F990[D_800ADBCC];
            u16 gearOff;

            D_800B06B8 = &g_FieldActors[gi];
            g_FieldScriptVMCurActor =
                (ActorData*)(uintptr_t)g_FieldActors[gi].pActorData;
            func_80080A74(gi);
            ((ActorData*)(uintptr_t)g_FieldActors[i].pActorData)
                ->scriptInstructionPointer = off0c;
            D_800AFD1C = D_8006F990[D_800ADBCC];
            gearOff = FieldScriptGetBytecodeOffset(D_8006F990[D_800ADBCC], 0);
            D_800AFFEC = 0;
            g_FieldScriptVMCurActor->scriptInstructionPointer = gearOff;
            FieldScriptVMRun(0xFFFF);
        }
        break;
    }

    D_800B06B8 = savedB06B8;
    g_FieldScriptVMCurActor = savedCur;
    D_800B00C0 = savedB00C0;
    D_800AFD1C = savedAFD1C;
    g_FieldScriptMaxInstructionCount = savedMax;
    savedCur->scriptInstructionPointer = savedIP;
}
#endif /* XENO_PC_PORT */

/* Add-party-member opcode, 2-byte-argument variant (asm 8008BC80).
 * Busy (a load pending / transition / stream): retry (IP-1 + yield).
 * arg == 0xFF: no-op, IP+5.  Free slot: clear the member's gamestate byte
 * (+0x22B1+slot), write the roster, kick the skin load, IP+3.  Already a
 * member / party full: set the roster bit (+0x1D30), IP+5. */
#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/field/nonmatchings/main/misc", func_8008BC80);
#else
void func_8008BC80(void) {
    s32 pending = D_800ADBC4;
    s32 charId;
    s32 slot;

    if (pending == 0xFF && D_800ADB2C == 0 && func_8008A558() == 0) {
        ArchiveCdDataSync(0);
        charId = FieldScriptVMGetArgument(1);
        if (charId == pending) {
            g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
            return;
        }
        if (func_8008A790(charId, &slot) == 0) {
            *((u8*)g_pGameState + 0x22B1 + slot) = 0;
            g_GamePartyMembers[slot] = charId;
            func_8008A7DC(charId, slot);
            g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
        } else {
            /* Retail sllv: shift count is the low five bits. */
            *(u16*)((u8*)g_pGameState + 0x1D30) |= (u16)(1u << (charId & 31));
            g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
        }
        return;
    }
    D_800B00C0 = 1;
    g_FieldScriptVMCurActor->scriptInstructionPointer -= 1;
}
#endif /* XENO_PC_PORT */

/* Add-party-member opcode, immediate-byte variant (asm 8008BDD8): charId is
 * the raw byte at IP+1; success IP+2, already-member IP+4, busy retry. */
#ifndef XENO_PC_PORT
/* func_8008BDD8: one script-byte read feeds both the party-slot store and the
 * func_8008A7DC call (the port reads it twice); gearRide/unk1D30 struct forms;
 * busy path uses the struct IP so its load hoists above the D_800B00C0 store. */
#endif
void func_8008BDD8(void) {
#ifdef XENO_PC_PORT
    s32 charId;
    s32 slot;

    if (D_800ADBC4 == 0xFF && D_800ADB2C == 0 && func_8008A558() == 0) {
        ArchiveCdDataSync(0);
        charId = SCRIPT_READ_U8_REL(1);
        if (func_8008A790(charId, &slot) == 0) {
            *((u8*)g_pGameState + 0x22B1 + slot) = 0;
            g_GamePartyMembers[slot] = SCRIPT_READ_U8_REL(1);
            func_8008A7DC(SCRIPT_READ_U8_REL(1), slot);
            g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
        } else {
            *(u16*)((u8*)g_pGameState + 0x1D30) |= 1 << SCRIPT_READ_U8_REL(1);
            g_FieldScriptVMCurActor->scriptInstructionPointer += 4;
        }
        return;
    }
    D_800B00C0 = 1;
    g_FieldScriptVMCurActor->scriptInstructionPointer -= 1;
#else
    s32 slot;
    s32 charId;

    if (D_800ADBC4 == 0xFF && D_800ADB2C == 0 && func_8008A558() == 0) {
        ArchiveCdDataSync(0);
        if (func_8008A790(SCRIPT_READ_U8_REL(1), &slot) == 0) {
            g_pGameState->gearRide[slot] = 0;
            charId = SCRIPT_READ_U8_REL(1);
            g_GamePartyMembers[slot] = charId;
            func_8008A7DC(charId, slot);
            g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
        } else {
            g_pGameState->unk1D30 |= 1 << SCRIPT_READ_U8_REL(1);
            g_FieldScriptVMCurActor->scriptInstructionPointer += 4;
        }
        return;
    }
    D_800B00C0 = 1;
    g_FieldScriptVMCurActor->scriptInstructionPointer -= 1;
#endif
}

extern s32 D_8005A448[];
extern void* D_8005A418[];
extern void* g_FieldSpriteData;

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/field/nonmatchings/main/misc", func_8008BF38);
#else
/* Retail 8008BF38.  D_8005A448 is &D_8005A444[1] on retail but a separate
 * stub on PC, so every party-slot access goes through D_8005A444 + 1.
 * Copy loop: 16-byte blocks (4 loads, then 4 stores) until src == src0 +
 * 0x14000.  Sprite-skin branch passes act[0x127], (w130 >> 28) & 3,
 * w134 & 0xF, the byte act[0x126] and (w134 >> 4) & 1, re-reading the
 * actor-data pointer as retail does. */
void func_8008BF38(s32 partyId) {
    s32 slot = D_8005A444[partyId + 1];

    if (slot == 0xFF) {
        g_GamePartyMemberSkins[partyId] = g_GamePartyMemberSkins[partyId + 1];
        g_GamePartyMembers[partyId] = g_GamePartyMembers[partyId + 1];
        D_8005A444[partyId] = D_8005A444[partyId + 1];
        g_GamePartyMembers[partyId + 1] = slot;
        g_GamePartyMemberSkins[partyId + 1] = slot;
        D_8005A444[partyId + 1] = slot;
    } else {
        u32* src = (u32*)D_8005A418[partyId];
        u32* dst = (u32*)g_PartyDataBuffers[partyId];
        u32* end = (u32*)((u8*)src + 0x14000);
        u8* act;
        do {
            u32 w0 = src[0];
            u32 w1 = src[1];
            u32 w2 = src[2];
            u32 w3 = src[3];
            dst[0] = w0;
            dst[1] = w1;
            dst[2] = w2;
            dst[3] = w3;
            src += 4;
            dst += 4;
        } while (src != end);

        g_GamePartyMemberSkins[partyId] = g_GamePartyMemberSkins[partyId + 1];
        g_GamePartyMembers[partyId] = g_GamePartyMembers[partyId + 1];
        D_8005A444[partyId] = D_8005A444[partyId + 1];

        act = (u8*)(uintptr_t)g_FieldActors[slot].pActorData;
        if (!(act[0x126] & 0x80)) {
            func_80076AC0(slot, partyId, g_PartyDataBuffers[partyId], 1, 0, partyId, 1);
        } else {
            u8* spriteData = (u8*)g_FieldSpriteData;
            u32 animOffset = *(u32*)(spriteData + (act[0x126] & 0x7F) * 4 + 4);
            u32 w130 = *(u32*)(act + 0x130);
            s32 arg1 = act[0x127];
            s32 arg4 = *(u32*)(act + 0x134) & 0xF;
            s32 arg5 = ((u8*)(uintptr_t)g_FieldActors[slot].pActorData)[0x126];
            s32 arg6 = (*(u32*)((u8*)(uintptr_t)g_FieldActors[slot].pActorData + 0x134) >> 4) & 1;
            func_80076AC0(slot, arg1, spriteData + animOffset, (w130 >> 28) & 3, arg4, arg5, arg6);
        }
        g_GamePartyMembers[partyId + 1] = 0xFF;
        g_GamePartyMemberSkins[partyId + 1] = 0xFF;
        D_8005A444[partyId + 1] = 0xFF;
    }
}
#endif /* XENO_PC_PORT */

extern void* D_800B06B8;
extern void func_80080A74(s32 actorIndex);

/* Retail 8008C180..8008C334 keeps the calling FieldActor and ActorData
 * independently. The constructor can republish the target ActorData owner. */
#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/field/nonmatchings/main/misc", func_8008C180);
#else
void func_8008C180(s32 partyId) {
    s32 slot = D_8005A444[partyId];
    if (slot != 0xFF) {
        FieldActor* fieldActor = &g_FieldActors[slot];
        void* savedFieldActor = D_800B06B8;
        ActorData* savedActor = g_FieldScriptVMCurActor;
        s32 savedIndex = D_800AFD1C;
        u16 savedIP = savedActor->scriptInstructionPointer;
        ActorData* target;
        u32 targetFlags;

        D_800B06B8 = fieldActor;
        g_FieldScriptVMCurActor = (ActorData*)(uintptr_t)fieldActor->pActorData;
        func_80080A74(slot);

        slot = D_8005A444[partyId];
        fieldActor = &g_FieldActors[slot];
        D_800AFD1C = slot;
        fieldActor->status = (fieldActor->status & 0xF07F) | 0x200;
        func_80076AC0(slot, 0, g_PartyDataBuffers[0], 1, 0, 0, 1);

        target = g_FieldScriptVMCurActor;
        target->scriptFlags.flags |= 1;
        targetFlags = target->flags;
        D_800B00C0 = 0;
        g_FieldScriptVMCurActor = savedActor;
        D_800B06B8 = savedFieldActor;
        D_800AFD1C = savedIndex;
        /* Retail SH at 8008C2C4 addresses the target still held in a0. */
        target->scriptInstructionPointer = savedIP;
        target->flags = targetFlags | 0x100000;
        target->scriptFlags.flags |= 0x20000;
        target->flags |= 0x400;
        D_8005A444[partyId] = 0xFF;
    }
    g_GamePartyMembers[partyId] = 0xFF;
    g_GamePartyMemberSkins[partyId] = 0xFF;
}
#endif /* XENO_PC_PORT */

extern s32 D_800ADBC4;
extern s32 g_GamePartyMemberSkins[];
extern void* g_PartyDataBuffers[];
extern s32 D_8005A444[];
extern s32 g_PlayerActorIndex;
extern s32 FieldCharacterIdToPartyId(s32 characterId);
extern s32 func_8008CF3C(s32);
extern void func_8008BF38(s32 partyId);
extern void func_8008C180(s32 partyId);

typedef struct {
    u32 words[4];
} PartyBufferBlock;

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/field/nonmatchings/main/misc", func_8008C334);
#else
void func_8008C334(void) {
    s32 partyId;
    PartyBufferBlock* src;
    PartyBufferBlock* dst;
    PartyBufferBlock* end;
    ActorData* player;

    if (D_800ADBC4 != 0xFF) {
        D_800B00C0 = 1;
        g_FieldScriptVMCurActor->scriptInstructionPointer--;
        return;
    }

    DrawSync(0);
    partyId = FieldCharacterIdToPartyId(func_8008CF3C(SCRIPT_READ_U8_REL(1)));
    if (partyId == -1) {
        g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
        return;
    }

    if (D_800ADB1C == 0) {
        switch (partyId) {
        case 0:
            if (g_GamePartyMembers[1] == 0xFF) {
                g_GamePartyMembers[0] = 0xFF;
                g_GamePartyMemberSkins[0] = 0xFF;
                g_pGameState->gearRide[0] = 0;
                break;
            }

            src = g_PartyDataBuffers[1];
            dst = g_PartyDataBuffers[0];
            end = src + (0x14000 / sizeof(*src));
            do {
                u32 w0 = src->words[0];
                u32 w1 = src->words[1];
                u32 w2 = src->words[2];
                u32 w3 = src->words[3];
                dst->words[0] = w0;
                dst->words[1] = w1;
                dst->words[2] = w2;
                dst->words[3] = w3;
                src++;
                dst++;
            } while (src != end);

            g_GamePartyMembers[0] = g_GamePartyMembers[1];
            g_GamePartyMemberSkins[0] = g_GamePartyMemberSkins[1];
            g_GamePartyMembers[1] = 0xFF;
            g_GamePartyMemberSkins[1] = 0xFF;
            g_pGameState->gearRide[0] = g_pGameState->gearRide[1];

            if (g_GamePartyMembers[2] == 0xFF) {
                break;
            }

            src = g_PartyDataBuffers[2];
            dst = g_PartyDataBuffers[1];
            end = src + (0x14000 / sizeof(*src));
            do {
                u32 w0 = src->words[0];
                u32 w1 = src->words[1];
                u32 w2 = src->words[2];
                u32 w3 = src->words[3];
                dst->words[0] = w0;
                dst->words[1] = w1;
                dst->words[2] = w2;
                dst->words[3] = w3;
                src++;
                dst++;
            } while (src != end);

            g_GamePartyMembers[1] = g_GamePartyMembers[2];
            g_GamePartyMemberSkins[1] = g_GamePartyMemberSkins[2];
            g_GamePartyMembers[2] = 0xFF;
            g_GamePartyMemberSkins[2] = 0xFF;
            g_pGameState->gearRide[1] = g_pGameState->gearRide[2];
            break;

        case 1:
            if (g_GamePartyMembers[2] == 0xFF) {
                g_GamePartyMembers[1] = 0xFF;
                g_GamePartyMemberSkins[1] = 0xFF;
                g_pGameState->gearRide[1] = 0;
                break;
            }

            src = g_PartyDataBuffers[2];
            dst = g_PartyDataBuffers[1];
            end = src + (0x14000 / sizeof(*src));
            do {
                u32 w0 = src->words[0];
                u32 w1 = src->words[1];
                u32 w2 = src->words[2];
                u32 w3 = src->words[3];
                dst->words[0] = w0;
                dst->words[1] = w1;
                dst->words[2] = w2;
                dst->words[3] = w3;
                src++;
                dst++;
            } while (src != end);

            g_GamePartyMembers[1] = g_GamePartyMembers[2];
            g_GamePartyMemberSkins[1] = g_GamePartyMemberSkins[2];
            g_GamePartyMembers[2] = 0xFF;
            g_GamePartyMemberSkins[2] = 0xFF;
            g_pGameState->gearRide[1] = g_pGameState->gearRide[2];
            g_pGameState->gearRide[2] = 0;
            break;

        case 2:
            g_GamePartyMembers[2] = 0xFF;
            g_GamePartyMemberSkins[2] = 0xFF;
            g_pGameState->gearRide[2] = 0;
            break;
        default:
            goto advance;
        }
    } else {
        switch (partyId) {
        case 0:
            g_pGameState->gearRide[0] = g_pGameState->gearRide[1];
            g_pGameState->gearRide[1] = g_pGameState->gearRide[2];
            g_pGameState->gearRide[2] = 0;
            func_8008C180(0);
            func_8008BF38(0);
            func_8008BF38(1);
            break;
        case 1:
            g_pGameState->gearRide[1] = g_pGameState->gearRide[2];
            g_pGameState->gearRide[2] = 0;
            func_8008C180(1);
            func_8008BF38(1);
            break;
        case 2:
            g_pGameState->gearRide[2] = 0;
            func_8008C180(2);
            break;
        }

        if (g_GamePartyMembers[0] != 0xFF) {
            if (D_8005A444[0] != 0xFF) {
                g_PlayerActorIndex = D_8005A444[0];
                player = (ActorData*)(uintptr_t)g_FieldActors[D_8005A444[0]].pActorData;
                player->scriptFlags.flags = (player->scriptFlags.flags | 0x4400) & ~0x80;
            } else {
                g_PlayerActorIndex = 0;
            }
        } else {
            g_PlayerActorIndex = 0;
        }
    }

advance:
    g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
}
#endif /* XENO_PC_PORT */

void func_8008C7D8(void) {
    ActorData* pActor = g_FieldScriptVMCurActor;
    u32 flags12C = *(u32*)((u8*)pActor + 0x12C);
    if (flags12C & 0x1000) {
        HeapFree((void*)(uintptr_t)*(u32*)((u8*)pActor + 0x114));
        *(u32*)((u8*)g_FieldScriptVMCurActor + 0x12C) &= ~0x1000u;
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 1;
}

extern s32 D_8004F36C;
extern s32 D_8004F324;
extern void *D_80062528;
extern void func_8003A89C();

// FE 0E — field music cue: sends a command to the music manager once a song is
// loaded (D_8004F36C); skips when no music is selected (id 0xFF) or the field is
// not yet live (D_800ADB1C == 0); otherwise retries until the song load completes.
void func_8008C84C(void) {
    s32 arg1;

    if (D_8004F36C != 0) {
        arg1 = FieldScriptVMGetArgument(1);
        func_8003A89C(D_80062528, arg1, FieldScriptVMGetArgument(3));
        g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
    } else if (D_8004F324 == 0xFF) {
        g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
    } else if (D_800ADB1C == 0) {
        g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
    } else {
        g_FieldScriptVMCurActor->scriptInstructionPointer -= 1;
    }
    D_800B00C0 = 1;
}

extern void func_8003A948(void* soundHandle, s32 a1, s32 a2);

void func_8008C938(void) {
    if (D_8004F36C != 0) {
        s32 arg1, arg2;
        arg1 = FieldScriptArgument1(1, SCRIPT_READ_U8_REL(5));
        arg2 = FieldScriptArgument2(3, SCRIPT_READ_U8_REL(5));
        func_8003A948(D_80062528, arg1, arg2);
        g_FieldScriptVMCurActor->scriptInstructionPointer += 6;
    } else if (D_8004F324 == 0xFF) {
        g_FieldScriptVMCurActor->scriptInstructionPointer += 6;
    } else if (D_800ADB1C == 0) {
        g_FieldScriptVMCurActor->scriptInstructionPointer += 6;
    } else {
        g_FieldScriptVMCurActor->scriptInstructionPointer -= 1;
    }
    D_800B00C0 = 1;
}

void func_8008CA60(void) {
    if (D_8004F36C != 0) {
        s32 arg1 = FieldScriptVMGetArgument(1);
        s32 arg3 = FieldScriptVMGetArgument(3);
        func_8003A838(D_80062528, arg1, arg3);
        g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
    } else if (D_8004F324 == 0xFF) {
        g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
    } else if (D_800ADB1C == 0) {
        g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
    } else {
        g_FieldScriptVMCurActor->scriptInstructionPointer -= 1;
    }
    D_800B00C0 = 1;
}

extern void func_8003A9BC(void* soundHandle, s32 a1, s32 a2);

void func_8008CB4C(void) {
    if (D_8004F36C != 0) {
        s32 arg1, arg2;
        arg1 = FieldScriptArgument1(1, SCRIPT_READ_U8_REL(5));
        arg2 = FieldScriptArgument2(3, SCRIPT_READ_U8_REL(5));
        func_8003A9BC(D_80062528, arg1, arg2);
        g_FieldScriptVMCurActor->scriptInstructionPointer += 6;
    } else if (D_8004F324 == 0xFF) {
        g_FieldScriptVMCurActor->scriptInstructionPointer += 6;
    } else if (D_800ADB1C == 0) {
        g_FieldScriptVMCurActor->scriptInstructionPointer += 6;
    } else {
        g_FieldScriptVMCurActor->scriptInstructionPointer -= 1;
    }
    D_800B00C0 = 1;
}

extern void func_8003AAC4(void* soundHandle, s32 soundId);

void func_8008CC74(void) {
    ActorData* pActor;
    if (D_8004F36C != 0) {
        s32 arg1 = FieldScriptVMGetArgument(1);
        func_8003AAC4(D_80062528, arg1);
        g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
    } else if (D_8004F324 == 0xFF) {
        g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
    } else if (D_800ADB1C == 0) {
        g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
    } else {
        g_FieldScriptVMCurActor->scriptInstructionPointer -= 1;
    }
    D_800B00C0 = 1;
}

/* Extended VM opcode 0x13 (Noah OPX_13): set the actor's pending sound
 * fields (+0x10A id, +0x10C param, +0x10D state=0) and release any SPU
 * voice channel still bound to this actor (func_800863E8). Id 0 means
 * "none" -> state 0xFF. asm nonmatchings/main/misc/func_8008CD48.s. */
extern void func_800863E8(s32 actorIdx);

void func_8008CD48(void) {
    /* The retail opcode addresses these script-owned bytes directly. The
     * ActorData member layout includes the overlapping rotation vector, so
     * spell out the retail offsets here. */
    *(s16*)((u8*)g_FieldScriptVMCurActor + 0x10A) = (s16)FieldScriptVMGetArgument(1);
    *((u8*)g_FieldScriptVMCurActor + 0x10D) = 0;
    *((u8*)g_FieldScriptVMCurActor + 0x10C) = (u_char)FieldScriptVMGetArgument(3);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
    func_800863E8(D_800AFD1C);
    if (*(u16*)((u8*)g_FieldScriptVMCurActor + 0x10A) == 0) {
        *((u8*)g_FieldScriptVMCurActor + 0x10D) = 0xFF;
    }
}

extern void func_800863E8(s32 actorIdx);
extern s32 D_800AFD1C;

#ifndef XENO_PC_PORT
/* func_8008CDD4: retail offsets 0x10A (u16), 0x10C, 0x10D (u8).  ActorData
 * names these unk10A/unk10C/unk10D but its 8-byte SVECTOR rotation at 0x104
 * pushes them to 0x10C/0x10E/0x10F, so the body indexes the actor directly.
 * The 0x80 store is through u8 (ori 0x80, not li -128). */
#endif
void func_8008CDD4(void) {
#ifdef XENO_PC_PORT
    ActorData* pActor = g_FieldScriptVMCurActor;
    s32 arg1 = FieldScriptVMGetArgument(1);
    pActor->unk10A = (short)arg1;
    pActor->unk10D = 0x80;
    {
        s32 arg3 = FieldScriptVMGetArgument(3);
        g_FieldScriptVMCurActor->unk10C = (u_char)arg3;
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
    func_800863E8(D_800AFD1C);
    if (g_FieldScriptVMCurActor->unk10A == 0) {
        g_FieldScriptVMCurActor->unk10D = 0xFF;
    }
#else
    ((u16*)g_FieldScriptVMCurActor)[0x10A / 2] = FieldScriptVMGetArgument(1);
    ((u8*)g_FieldScriptVMCurActor)[0x10D] = 0x80;
    ((u8*)g_FieldScriptVMCurActor)[0x10C] = FieldScriptVMGetArgument(3);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
    func_800863E8(D_800AFD1C);
    if (((u16*)g_FieldScriptVMCurActor)[0x10A / 2] == 0) {
        ((u8*)g_FieldScriptVMCurActor)[0x10D] = 0xFF;
    }
#endif
}

void func_8008CE64(void) {
    s32 v = func_8008CF3C(FieldScriptVMGetArgument(1));
    if (v != 0xFF) {
        g_pGameState->FrMask &= ~(1 << v);
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

void func_8008CED0(void) {
    s32 v = func_8008CF3C(FieldScriptVMGetArgument(1));
    if (v != 0xFF) {
        g_pGameState->FrMask |= 1 << v;
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

s32 func_8008CF3C(s32 a0) {
#ifdef XENO_PC_PORT
    switch (a0) {
    case 0xFF:
        return g_GamePartyMembers[2];
    case 0xFE:
        return g_GamePartyMembers[1];
    case 0xFD:
        return g_GamePartyMembers[0];
    case 0xFC:
        return 0xFF;
    }
    return a0;
#else
    register s32 result asm("$2");
    if (a0 == 0xFF) result = g_GamePartyMembers[2];
    else if (a0 == 0xFE) result = g_GamePartyMembers[1];
    else if (a0 == 0xFD) result = g_GamePartyMembers[0];
    else { result = 0xFF; if (a0 != 0xFC) result = a0; }
    return result;
#endif
}

void func_8008CF9C(void) {
    g_FieldScriptVMCurActor->faceId = func_8008CF3C(FieldScriptVMGetArgument(1));
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

extern s16 D_800B21A0, D_800B21A2, D_800B21A4, D_800B21A6, D_800B21A8, D_800B21AA;

void func_8008CFEC(void) {
#ifdef XENO_PC_PORT
    D_800B21A0 = (s16)FieldScriptVMGetInstructionArgument(1);
    D_800B21A4 = (s16)FieldScriptVMGetInstructionArgument(3);
    D_800B21A2 = (s16)FieldScriptVMGetInstructionArgument(5);
    D_800B21A6 = (s16)FieldScriptVMGetInstructionArgument(7);
    D_800B21AA = (s16)FieldScriptVMGetInstructionArgument(9);
    D_800B21A8 = (s16)FieldScriptVMGetInstructionArgument(11);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 13;
#else
    D_800B21A0 = FieldScriptVMGetInstructionArgument(1);
    D_800B21A4 = FieldScriptVMGetInstructionArgument(3);
    D_800B21A2 = FieldScriptVMGetInstructionArgument(5);
    D_800B21A6 = FieldScriptVMGetInstructionArgument(7);
    (*(s16*)&D_800B21AA) = FieldScriptVMGetInstructionArgument(9);
    D_800B21A8 = FieldScriptVMGetInstructionArgument(0xB);
    *(u16 *)((s8*)(g_FieldScriptVMCurActor) + 0xCC) = (u16) (*(u16 *)((s8*)(g_FieldScriptVMCurActor) + 0xCC) + 0xD);
#endif
}

void func_8008D078(void) {
    if (FieldScriptVMGetArgument(1) == 0) {
        *(s32*)((u8*)g_FieldScriptVMCurActor + 0x4) &= ~0x800;
    } else {
        *(s32*)((u8*)g_FieldScriptVMCurActor + 0x4) |= 0x800;
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

/* Extended field-script opcode EX-0x03 (SET_CURRENT_ACTOR_SCALE), reached via
 * the 0xFE prefix -> FieldScriptVM2Run. Sets the current actor's sprite scale
 * (SpriteData+0x2C) and 3D scale (scaleX/Y/Z), then advances the script IP by 3.
 * Decompiled from func_8008D0F4.s. This was previously INCLUDE_ASM, which on the
 * native port became a no-op logging stub that never advanced scriptInstructionPointer;
 * the VM then re-dispatched the operand bytes as a bogus top-level dialog opcode,
 * opening a garbage text box (the Lahan-intro "garbled dialog"). The IP += 3 here
 * is the load-bearing fix. */
void func_8008D0F4(void) {
#ifdef XENO_PC_PORT
    s32 scale = FieldScriptVMGetArgument(1);
    *(s16*)((u8*)(uintptr_t)g_FieldActors[D_800AFD1C].pSpriteData + 0x2C) = (scale * 3) >> 2;
    g_FieldScriptVMCurActor->scaleX = scale;
    g_FieldScriptVMCurActor->scaleY = scale;
    g_FieldScriptVMCurActor->scaleZ = scale;
    func_80072254(D_800AFD1C);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
#else
    s32 scale = FieldScriptVMGetArgument(1);
    ((SpriteData *)(uintptr_t)g_FieldActors[D_800AFD1C].pSpriteData)->scale = (u32)(scale * 3) >> 2;
    g_FieldScriptVMCurActor->scaleX = scale;
    g_FieldScriptVMCurActor->scaleY = scale;
    g_FieldScriptVMCurActor->scaleZ = scale;
    func_80072254(D_800AFD1C);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
#endif
}

extern s32 D_800AFD1C;

#ifndef XENO_PC_PORT
/* func_8008D180: struct-field stores (SpriteData.scale, ActorData.scaleX/Y/Z)
 * keep D_800AFD1C live in a0 for the call; func_80072254 takes one argument
 * (the draft passed the live a1/a2 too). */
#endif
void func_8008D180(void) {
#ifdef XENO_PC_PORT
    s32 arg1 = FieldScriptVMGetArgument(1);
    s32 arg3 = FieldScriptVMGetArgument(3);
    s32 arg5 = FieldScriptVMGetArgument(5);
    u32 actorIdx = (u32)D_800AFD1C;
    u8* pFieldActor = (u8*)g_FieldActors + actorIdx * 92;
    u32 pActorData = *(u32*)(pFieldActor + 0x04);
    u8* pActor = (u8*)g_FieldScriptVMCurActor;

    *(s16*)(pActorData + 0x2C) = 0xC00;
    *(s16*)(pActor + 0xF4) = (s16)arg1;
    *(s16*)(pActor + 0xF6) = (s16)arg3;
    *(s16*)(pActor + 0xF8) = (s16)arg5;
    func_80072254(actorIdx);
    *(u16*)(pActor + 0xCC) += 7;
#else
    s32 arg1 = FieldScriptVMGetArgument(1);
    s32 arg3 = FieldScriptVMGetArgument(3);
    s32 arg5 = FieldScriptVMGetArgument(5);
    ((SpriteData*)(uintptr_t)g_FieldActors[D_800AFD1C].pSpriteData)->scale = 0xC00;
    g_FieldScriptVMCurActor->scaleX = arg1;
    g_FieldScriptVMCurActor->scaleY = arg3;
    g_FieldScriptVMCurActor->scaleZ = arg5;
    func_80072254(D_800AFD1C);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 7;
#endif
}

extern s16 D_800B218C[];

/* FE3A — write party frame mask. */
void func_8008D230(void) {
    D_800B218C[0] = FieldScriptVMGetArgument(1);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

#ifndef XENO_PC_PORT
/* func_8008D26C: a struct-field store (SpriteData.field_0x82) lets the actor
 * load hoist above the sh; the raw *(s16*)(p + 0x82) store pins it below. */
#endif
void func_8008D26C(void) {
#ifdef XENO_PC_PORT
    *(s16*)((u8*)(uintptr_t)g_FieldActors[D_800AFD1C].pSpriteData + 0x82) = FieldScriptVMGetArgument(1) << 1;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
#else
    ((SpriteData*)(uintptr_t)g_FieldActors[D_800AFD1C].pSpriteData)->field_0x82 = FieldScriptVMGetArgument(1) << 1;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
#endif
}

void func_8008D2D8(void) {
}

void func_8008D2E0(s32 a0, s32 a1) {
    *(u8*)(a1 + (s32)g_FieldScriptVMCurScriptData + 1) = a0 >> 8;
    *(u8*)((u8*)g_FieldScriptVMCurScriptData + a1) = a0;
}

s32 func_8008D30C(s32 a0, s32 a1) {
    s32 dx = g_FieldActors[a0].transformMatrix.t[0] - g_FieldActors[a1].transformMatrix.t[0];
    s32 dz = g_FieldActors[a0].transformMatrix.t[2] - g_FieldActors[a1].transformMatrix.t[2];
    return -(FieldGetVec2Magnitude(dx, dz) >= 0x10);
}

static inline void CopySpriteFieldS32(FieldActor* actors, int dstId, int srcId, int offset) {
    void* dst = (void*)(uintptr_t)actors[dstId].pSpriteData;
    void* src = (void*)(uintptr_t)actors[srcId].pSpriteData;
    *(s32*)((u8*)dst + offset) = *(s32*)((u8*)src + offset);
}

static inline void CopySpriteFieldU16(FieldActor* actors, int dstId, int srcId, int offset) {
    void* dst = (void*)(uintptr_t)actors[dstId].pSpriteData;
    void* src = (void*)(uintptr_t)actors[srcId].pSpriteData;
    *(u16*)((u8*)dst + offset) = *(u16*)((u8*)src + offset);
}

static inline void CopyActorFieldS32(FieldActor* base, int dstId, int srcId, int offset) {
    *(s32*)((u8*)&base[dstId] + offset) = *(s32*)((u8*)&base[srcId] + offset);
}

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/field/nonmatchings/main/misc", FieldActorCopyPlacement);
#else
void FieldActorCopyPlacement(int dstActorId, int srcActorId) {
    FieldActor* actors = g_FieldActors;
    ActorData* dstActor;
    ActorData* srcActor;
    int i;

    srcActor = (ActorData*)(uintptr_t)actors[srcActorId].pActorData;
    dstActor = (ActorData*)(uintptr_t)actors[dstActorId].pActorData;

    for (i = 0; i < 4; i++) {
        dstActor->walkmeshTriIds[i] = srcActor->walkmeshTriIds[i];
    }
    {
      s16 walkmeshId = srcActor->walkmeshId;
      dstActor->walkmeshId = walkmeshId;
    }
    dstActor->curTriNormal.vx = srcActor->curTriNormal.vx;
    dstActor->curTriNormal.vy = srcActor->curTriNormal.vy;
    dstActor->curTriNormal.vz = srcActor->curTriNormal.vz;
    dstActor->position.vx = srcActor->position.vx;
    dstActor->position.vy = srcActor->position.vy;
    dstActor->position.vz = srcActor->position.vz;
    dstActor->unkEC = srcActor->unkEC;
    dstActor->curYPos = srcActor->curYPos;
    dstActor->curWalkmeshTriMaterial = srcActor->curWalkmeshTriMaterial;

    CopySpriteFieldU16(g_FieldActors, dstActorId, srcActorId, offsetof(SpriteData, field_0x84));
    CopySpriteFieldS32(g_FieldActors, dstActorId, srcActorId, offsetof(SpriteData, position.x));
    CopySpriteFieldS32(g_FieldActors, dstActorId, srcActorId, offsetof(SpriteData, position.y));
    CopySpriteFieldS32(g_FieldActors, dstActorId, srcActorId, offsetof(SpriteData, position.z));

    CopyActorFieldS32(g_FieldActors, dstActorId, srcActorId, offsetof(FieldActor, transformMatrix.t[0]));
    CopyActorFieldS32(g_FieldActors, dstActorId, srcActorId, offsetof(FieldActor, transformMatrix.t[1]));
    CopyActorFieldS32(g_FieldActors, dstActorId, srcActorId, offsetof(FieldActor, transformMatrix.t[2]));
}
#endif /* XENO_PC_PORT */

extern s32 D_8005A444[];
extern u8 D_800B219F[];

void func_8008D570(void) {
    s32 i;
    for (i = 0; i < 3; i++) {
        if (D_8005A444[i] == D_800AFD1C) {
            D_800B219F[0] &= ~(1 << i);
        }
    }
}

extern u8 D_800B21CD[];

/* FE26 — write distortion/effect flag byte. */
void func_8008D5C8(void) {
    D_800B21CD[0] = SCRIPT_READ_U8_REL(1);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
}

/* FE07/opcode 0x107 -- set (arg 1) or clear (arg 0) the current actor's
 * 0x400 flag; any other arg leaves it untouched.  IP += 2.  As a no-op stub
 * this handler never advanced the IP, so the VM's next dispatch executed the
 * FE-prefix's extension byte as a BASE opcode -- desyncing the actor's whole
 * script stream (MAP3's boot SEGV: garbage actor index 128 fed to
 * func_8009EB78 several bogus opcodes later). */
#ifndef XENO_PC_PORT
/* func_8008D604: retail is a switch (beqz / beq 1 / j default) whose two
 * cases share one store through the cached actor pointer. */
#endif
void func_8008D604(void) {
#ifdef XENO_PC_PORT
    u8 arg = SCRIPT_READ_U8_REL(1);

    if (arg == 0) {
        g_FieldScriptVMCurActor->flags &= ~0x400;
    } else if (arg == 1) {
        g_FieldScriptVMCurActor->flags |= 0x400;
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
#else
    switch (SCRIPT_READ_U8_REL(1)) {
    case 0:
        g_FieldScriptVMCurActor->flags &= ~0x400;
        break;
    case 1:
        g_FieldScriptVMCurActor->flags |= 0x400;
        break;
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
#endif
}

void func_8008D684(void) {
    int mask;
    unsigned int slotIndex;

    slotIndex = (unsigned int) (SCRIPT_IMM_ARG(1)) >> 4;
    mask = 1 << (FieldScriptVMGetInstructionArgument(1) & 0xF);
    FieldScriptMemoryWriteU16(slotIndex, FieldScriptVMGetVariableValue(slotIndex) | mask);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

void func_8008D700(void) {
    int mask;
    unsigned int slotIndex;

    slotIndex = (unsigned int) (SCRIPT_IMM_ARG(1)) >> 4;
    mask = 1 << (FieldScriptVMGetInstructionArgument(1) & 0xF);
    FieldScriptMemoryWriteU16(slotIndex, FieldScriptVMGetVariableValue(slotIndex) & ~mask);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

void func_8008D780(void) {
    short destination;
    int mask;
    unsigned int slotIndex;

    slotIndex = (unsigned int) (SCRIPT_IMM_ARG(1)) >> 4;
    mask = 1 << (FieldScriptVMGetInstructionArgument(1) & 0xF);
    if (FieldScriptVMGetVariableValue(slotIndex) & mask) {
        g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
    } else {
        destination = FieldScriptVMGetInstructionArgument(3);
        g_FieldScriptVMCurActor->scriptInstructionPointer = destination;
    }
}

void func_8008D808(s32 arg0, s32 arg1, s32 arg2) {
    /* Retail 8008D808 reloads g_FieldScriptVMCurActor->scriptInstructionPointer
     * (0xCC) and g_FieldScriptVMCurScriptData for every byte store and every
     * func_8008D2E0 call (127 instructions); caching them collapses the body. */
    SCRIPT_READ_U8_REL(0x00) = 0x57;
    SCRIPT_READ_U8_REL(0x01) = 0x81;
    func_8008D2E0(arg0, g_FieldScriptVMCurActor->scriptInstructionPointer + 0x02);
    func_8008D2E0(arg1, g_FieldScriptVMCurActor->scriptInstructionPointer + 0x04);
    func_8008D2E0(arg2, g_FieldScriptVMCurActor->scriptInstructionPointer + 0x06);
    func_8008D2E0(0xC, g_FieldScriptVMCurActor->scriptInstructionPointer + 0x08);
    SCRIPT_READ_U8_REL(0x0A) = 0xFF;
    SCRIPT_READ_U8_REL(0x0B) = 0x57;
    SCRIPT_READ_U8_REL(0x0C) = 0x8F;
    SCRIPT_READ_U8_REL(0x0D) = 0x26;
    SCRIPT_READ_U8_REL(0x0E) = 0x01;
    SCRIPT_READ_U8_REL(0x0F) = 0x80;
    SCRIPT_READ_U8_REL(0x10) = 0x57;
    SCRIPT_READ_U8_REL(0x11) = 0x0F;
}

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/field/nonmatchings/main/misc", func_8008DA04);
#else
void func_8008DA04(s32 arg0, s32 arg1) {
    u8* pScript = g_FieldScriptVMCurScriptData;
    u8* pIP = pScript + g_FieldScriptVMCurActor->scriptInstructionPointer;
    pIP[0x0C] = 0x4B;
    func_8008D2E0(arg0, g_FieldScriptVMCurActor->scriptInstructionPointer + 0x0D);
    func_8008D2E0(arg1, g_FieldScriptVMCurActor->scriptInstructionPointer + 0x0F);
    pIP = pScript + g_FieldScriptVMCurActor->scriptInstructionPointer;
    pIP[0x11] = 0xFF;
    pIP[0x12] = 0xFF;
    pIP[0x13] = 0x80;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 0xC;
}
#endif /* XENO_PC_PORT */

void func_8008DAFC(void) {
    g_FieldScriptVMCurActor->parentActorId = 0xFF;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 1;
}

extern s16 D_800B233E;

void func_8008DB2C(void) {
#ifdef XENO_PC_PORT
    D_800B233E = FieldScriptVMGetActorIndex(1);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
#else
    D_800B233E = FieldScriptVMGetActorIndex(1);
    *(u16 *)((s8*)(g_FieldScriptVMCurActor) + 0xCC) = (u16) (*(u16 *)((s8*)(g_FieldScriptVMCurActor) + 0xCC) + 2);
#endif
}


int FieldPartyMemberIncreaseGearHp(int partyMemberIndex, unsigned int amount) {
    int gearId;
    unsigned int maxHp;
    unsigned int newHp;

    gearId = GameCharacterGetGearID(g_GamePartyMembers[partyMemberIndex]);
    if (gearId != CHARACTER_ID_NONE) {
        maxHp = g_pGameState->gears[gearId].maxHp;
        newHp = g_pGameState->gears[gearId].hp + amount;
        g_pGameState->gears[gearId].hp = newHp;
        if (maxHp < newHp) {
            g_pGameState->gears[gearId].hp = maxHp;
        }
    }
}

int FieldPartyMemberDecreaseGearHp(int partyMemberIndex, unsigned int amount) {
    int gearId;
    int newHp;

    gearId = GameCharacterGetGearID(g_GamePartyMembers[partyMemberIndex]);
    if (gearId != CHARACTER_ID_NONE) {
        newHp = g_pGameState->gears[gearId].hp - amount;
        if (newHp <= 0) {
            newHp = 1;
        }
        g_pGameState->gears[gearId].hp = newHp;
    }
}

extern s16 g_FieldNumPartyMembersMasks[4];

void FieldScriptVMHandlerIncreasePartyGearHp(void) {
    int mask;
    int amount;
    int i;

    amount = FieldScriptArgument1(1, SCRIPT_READ_U8_REL(3));
    mask = g_FieldNumPartyMembersMasks[SCRIPT_READ_U8_REL(3) & 0x3];

    for (i = 0; i < MAX_PARTY_MEMBERS; i++) {
        if ((g_GamePartyMembers[i] != CHARACTER_ID_NONE) && (mask & 1)) {
            FieldPartyMemberIncreaseGearHp(i, amount);
        }
        mask >>= 1;
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 4;
}

void FieldScriptVMHandlerDecreasePartyGearHp(void) {
    int mask;
    int amount;
    int i;

    amount = FieldScriptArgument1(1, SCRIPT_READ_U8_REL(3));
    mask = g_FieldNumPartyMembersMasks[SCRIPT_READ_U8_REL(3) & 0x3];

    for (i = 0; i < MAX_PARTY_MEMBERS; i++) {
        if ((g_GamePartyMembers[i] != CHARACTER_ID_NONE) && (mask & 1)) {
            FieldPartyMemberDecreaseGearHp(i, amount);
        }
        mask >>= 1;
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 4;
}

void FieldScriptSetParentActor(void) {
    int actorIndex = FieldScriptVMGetActorIndex(1);
    if (actorIndex != ACTOR_ID_INVALID) {
        g_FieldScriptVMCurActor->parentActorId = actorIndex;
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
}

void FieldScriptWriteActorFlags1(void) {
    int actorIndex = FieldScriptVMGetActorIndex(1);

    if (actorIndex != ACTOR_ID_INVALID) {
        u8* pActorData = (u8*)(uintptr_t)g_FieldActors[actorIndex].pActorData;
        FieldScriptMemoryWriteU16(SCRIPT_IMM_ARG(1), *(u32*)pActorData);
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

void FieldScriptWriteActorFlags2(void) {
    int actorIndex = FieldScriptVMGetActorIndex(1);

    if (actorIndex != ACTOR_ID_INVALID) {
        u8* pActorData = (u8*)(uintptr_t)g_FieldActors[actorIndex].pActorData;
        FieldScriptMemoryWriteU16(SCRIPT_IMM_ARG(1), *(u16*)(pActorData + 2));
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

void FieldScriptWriteActorFlags3(void) {
    int actorIndex = FieldScriptVMGetActorIndex(1);

    if (actorIndex != ACTOR_ID_INVALID) {
        u8* pActorData = (u8*)(uintptr_t)g_FieldActors[actorIndex].pActorData;
        FieldScriptMemoryWriteU16(SCRIPT_IMM_ARG(1), *(u32*)(pActorData + 4));
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

void FieldScriptWriteActorFlags4(void) {
    int actorIndex = FieldScriptVMGetActorIndex(1);

    if (actorIndex != ACTOR_ID_INVALID) {
        u8* pActorData = (u8*)(uintptr_t)g_FieldActors[actorIndex].pActorData;
        FieldScriptMemoryWriteU16(SCRIPT_IMM_ARG(1), *(u16*)(pActorData + 6));
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

// TODO: These two handlers could use more semantic names since they're likely used
// in certain situations.
void FieldScriptVMConditionalJump6(unsigned short flag) {
    unsigned short argument = SCRIPT_IMM_ARG(1);
    if (argument & flag) {
        g_FieldScriptVMCurActor->scriptInstructionPointer += 6;
    } else {
        g_FieldScriptVMCurActor->scriptInstructionPointer = FieldScriptVMGetInstructionArgument(4);
    }
}

void FieldScriptVMConditionalJump5(unsigned short flag) {
    unsigned short argument = SCRIPT_IMM_ARG(1);
    if (argument & flag) {
        g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
    } else {
        g_FieldScriptVMCurActor->scriptInstructionPointer = SCRIPT_IMM_ARG(2);
    }
}

void FieldScriptWriteActorDistance(void) {
    ActorData* pActorA;
    ActorData* pActorB;
    FieldActor* actors;
    int actorIndexA;
    int actorIndexB;
    int distance;

    distance = 0;
    actorIndexA = FieldScriptVMGetActorIndex(3);
    actorIndexB = FieldScriptVMGetActorIndex(4);
    if ((actorIndexA != ACTOR_ID_INVALID) && (actorIndexB != ACTOR_ID_INVALID)) {
        /* Snapshot the volatile publication once, as the retail load does. */
        actors = g_FieldActors;
        pActorA = (ActorData*)(uintptr_t)actors[actorIndexA].pActorData;
        pActorB = (ActorData*)(uintptr_t)actors[actorIndexB].pActorData;
        distance = FieldGetVec2Magnitude(
            CONV_TO_GTE(pActorA->position.vx) - CONV_TO_GTE(pActorB->position.vx),
            CONV_TO_GTE(pActorA->position.vz) - CONV_TO_GTE(pActorB->position.vz)
        );
    }
#ifdef FIELD_VM_AUDIT_MUTANT_DISTANCE_ZERO
    distance = 0;
#endif
    FieldScriptMemoryWriteU16(
        SCRIPT_IMM_ARG(1),
        distance
    );
    g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
}

void func_8008E298(void) {
    FieldScriptVMConditionalJump6(((u16*)g_FieldActors[FieldScriptVMGetActorIndex(3)].pActorData)[0]);
}

void func_8008E2EC(void) {
    FieldScriptVMConditionalJump6(((u16*)g_FieldActors[FieldScriptVMGetActorIndex(3)].pActorData)[1]);
}

void func_8008E340(void) {
    FieldScriptVMConditionalJump6(((u16*)g_FieldActors[FieldScriptVMGetActorIndex(3)].pActorData)[2]);
}

void func_8008E394(void) {
    FieldScriptVMConditionalJump6(((u16*)g_FieldActors[FieldScriptVMGetActorIndex(3)].pActorData)[3]);
}

void func_8008E3E8(void) {
    FieldScriptVMConditionalJump5(((u16*)g_FieldScriptVMCurActor)[0]);
}

void func_8008E414(void) {
    FieldScriptVMConditionalJump5(((u16*)g_FieldScriptVMCurActor)[1]);
}

void func_8008E440(void) {
    FieldScriptVMConditionalJump5(((u16*)g_FieldScriptVMCurActor)[2]);
}

void func_8008E46C(void) {
    FieldScriptVMConditionalJump5(((u16*)g_FieldScriptVMCurActor)[3]);
}

void func_8008E498(int a0) {
    FieldScriptMemoryWriteU16(SCRIPT_IMM_ARG(1), a0 & 0xFFFF);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

void func_8008E4EC(void) {
    func_8008E498(((u16*)g_FieldScriptVMCurActor)[0]);
}

void func_8008E518(void) {
    func_8008E498(((u16*)g_FieldScriptVMCurActor)[1]);
}

void func_8008E544(void) {
    func_8008E498(((u16*)g_FieldScriptVMCurActor)[2]);
}

void func_8008E570(void) {
    func_8008E498(((u16*)g_FieldScriptVMCurActor)[3]);
}

void func_8008E59C(void) {
    u32 mask;

    mask = FieldScriptVMGetInstructionArgument(2) & 0xFFFF;
    switch (SCRIPT_READ_U8_REL(1)) {
        case 0:
            g_FieldScriptVMCurActor->scriptFlags.flags |= mask;
            break;

        case 1:
            g_FieldScriptVMCurActor->scriptFlags.flags |= mask << 16;
            break;

        case 2:
            g_FieldScriptVMCurActor->flags |= mask;
            break;

        case 3:
            g_FieldScriptVMCurActor->flags |= mask << 16;
            break;

        case 4:
            g_FieldScriptVMCurActor->scriptFlags.flags &= ~mask;
            break;

        case 5:
            g_FieldScriptVMCurActor->scriptFlags.flags &= ~(mask << 16);
            break;

        case 6:
            g_FieldScriptVMCurActor->flags &= ~mask;
            break;

        case 7:
            g_FieldScriptVMCurActor->flags &= ~(mask << 16);
            break;
    }

    g_FieldScriptVMCurActor->scriptInstructionPointer += 4;
}

/* func_8008E718 -- step-counter / formation-cooldown REFRESH. Decompiled from
 * asm/field/nonmatchings/main/misc/func_8008E718.s (was INCLUDE_ASM/stubbed).
 * Called by func_80079288 (roll, when the step counter hits 0) and by the
 * encounter-setup opcode func_8008E85C below. Reloads the step counter from the
 * encounter base and assigns each active formation a unique random cooldown.
 * See docs/ai_context/ACTIVE_HANDOFF.md. */
extern s32 D_800B2294, D_800B2298, D_800B229C;
extern s16 D_800B22A0[];

#ifndef XENO_PC_PORT
/* func_8008E718: D_800B2294..D_800B22A0 is one block (the count is re-read as
 * timers[-2] through the timers base, the clear loop runs off &D_800B2294+0x4A),
 * so it is reached through a struct alias.  The clear loop is written ascending:
 * gcc reverses it (check_dbra_loop) into the retail countdown, which is why the
 * counter init lands after the hoisted 0xFFFF.  roll is an int with an explicit
 * & 0xFFFF (sra, masked value stored). */
typedef struct {
    s32 unk0;
    s32 max;
    s32 count;
    u16 timers[32];
} func_8008E718_s;
extern func_8008E718_s D_800B2294__s asm("D_800B2294");

#endif
void func_8008E718(void) {
#ifdef XENO_PC_PORT
    u16 *timers = (u16 *)D_800B22A0;
    s32  i, j, roll;

    D_800B2294 = D_800B2298;

    if (D_800B229C == 0) {
        D_800B2298 = 0;
        return;
    }

    for (i = 31; i >= 0; i--) {
        timers[i] = 0xFFFF;
    }

    if (D_800B229C > 0) {
        for (i = 0; i < D_800B229C; i++) {
            do {
                roll = ((rand() * (D_800B2298 + 1)) >> 15) & 0xFFFF;
                for (j = 0; j < 32; j++) {
                    if (timers[j] == (u16)roll) break;
                }
            } while (j < 32);
            timers[i] = (u16)roll;
        }
    }

    if (D_800B229C > 0) {
        for (i = 0; i < D_800B229C; i++) {
            timers[i] = (u16)(timers[i] + 1);
        }
    }
#else
    s32 i, j;
    s32 roll;

    D_800B2294__s.unk0 = D_800B2294__s.max;
    if (D_800B2294__s.count == 0) {
        D_800B2294__s.max = 0;
        return;
    }
    for (i = 0; i < 32; i++) {
        D_800B2294__s.timers[i] = 0xFFFF;
    }
    for (i = 0; i < D_800B2294__s.count; i++) {
    retry:
        roll = ((rand() * (D_800B2294__s.max + 1)) >> 15) & 0xFFFF;
        for (j = 0; j < 32; j++) {
            if (D_800B2294__s.timers[j] == roll) {
                goto retry;
            }
        }
        D_800B2294__s.timers[i] = roll;
    }
    for (i = 0; i < D_800B2294__s.count; i++) {
        D_800B2294__s.timers[i]++;
    }
#endif
}

void func_8008E85C(void) {
    D_800B2298 = FieldScriptVMGetArgument(1);
    D_800B229C = FieldScriptVMGetArgument(3);
    if (D_800B229C >= 0x21) {
        D_800B229C = 0x20;
    }
    func_8008E718();
    g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
}

void func_8008E8C8(void) {
    ActorData* pActor = g_FieldScriptVMCurActor;
    u8 subOp = SCRIPT_READ_U8_REL(1);

    switch (subOp) {
        case 0: {
            u32 flags0 = *(u32*)(pActor);
            if (flags0 & 0x8000) {
                *(u32*)(pActor) = flags0 & 0xFFFF7FFF;
            }
            pActor = g_FieldScriptVMCurActor;
            if (*(u32*)((u8*)pActor + 0x04) & 0x80000) {
                s32 idx = D_800AFD1C;
                u8* pFieldActor = (u8*)(uintptr_t)((u32)(idx * 92) + (u32)(uintptr_t)g_FieldActors);
                u32 pSub = *(u32*)(pFieldActor + 0x04);
                *(u32*)(pSub + 0x18) = 0;
                *(u32*)(pSub + 0x14) = 0;
                *(u32*)(pSub + 0x0C) = 0;
                *(u32*)((u8*)pActor + 0x04) &= 0xFFF7FFFF;
            }
            break;
        }
        case 1: {
            u32 flags0 = *(u32*)(pActor);
            u16 val = *(u16*)((u8*)pActor + 0x106);
            *(u32*)(pActor) = flags0 | 0x8000;
            *(u16*)((u8*)pActor + 0x11C) = val;
            break;
        }
        case 2: {
            u32 flags4 = *(u32*)((u8*)pActor + 0x04);
            *(u32*)((u8*)pActor + 0x04) = flags4 | 0x80000;
            break;
        }
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
}

extern s32 D_800ADB7C;

/* FE61: wait for the FE60 transition body (func_800A7C58) to publish
 * D_800ADB7C, then consume the flag and advance.  Title map 490 sits here
 * while the attract STR plays. */
void func_8008E9F8(void) {
#ifdef TITLE_CHAIN_MUTANT_FE61_NEVER_RELEASES
    /* Deliberate mutant (pc_port/tests/run_title_newgame_chain.sh): the wait
     * never consumes the flag, so the title script spins forever. */
    g_FieldScriptVMCurActor->scriptInstructionPointer -= 1;
    D_800B00C0 = 1;
    return;
#endif
    if (D_800ADB7C == 0) {
        g_FieldScriptVMCurActor->scriptInstructionPointer -= 1;
    } else {
        D_800ADB7C = 0;
        g_FieldScriptVMCurActor->scriptInstructionPointer += 1;
    }
    D_800B00C0 = 1;
}

extern s32 D_800ADBDC;
extern s16 D_800C3A20, D_800C3A22, D_800C3A24, D_800C3A26, D_800C3A28, D_800C3A2A, D_800C3A2C, D_800C3A2E;
extern s16 D_800C3A30, D_800C3A32, D_800C3A34, D_800C3A36, D_800C3A38, D_800C3A3A;
extern s32 D_800ADB80, D_800ADB74, D_800ADB70;

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/field/nonmatchings/main/misc", func_8008EA58);
#else
void func_8008EA58(void) {
    if (D_800ADBDC == 0) {
        D_800B00C0 = 1;
        g_FieldScriptVMCurActor->scriptInstructionPointer -= 1;
        return;
    }
    D_800C3A20 = (s16)FieldScriptArgument1(1, SCRIPT_READ_U8_REL(0xB));
    D_800C3A2A = (s16)FieldScriptArgument2(3, SCRIPT_READ_U8_REL(0xB));
    D_800C3A2C = (s16)FieldScriptArgument3(5, SCRIPT_READ_U8_REL(0xB));
    D_800C3A2E = (s16)FieldScriptArgument4(7, SCRIPT_READ_U8_REL(0xB));
    D_800C3A38 = (s16)FieldScriptArgument5(9, SCRIPT_READ_U8_REL(0xB));
    D_800C3A32 = 0x140;
    D_800ADB80 = 0x40;
    D_800C3A36 = 1;
    D_800C3A34 = 0x100;
    D_800C3A26 = 0;
    D_800C3A24 = 0;
    D_800C3A22 = 0;
    D_800C3A28 = 0x100;
    D_800C3A3A = 0;
    D_800C3A30 = (s16)(*(u16*)&D_800C3A30 & 0xF);
    D_800ADB74 = 0;
    D_800ADB70 = 1;
    D_800B00C0 = 1;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 0xC;
}
#endif /* XENO_PC_PORT */

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/field/nonmatchings/main/misc", func_8008EC30);
#else
void func_8008EC30(void) {
    s32 arg7, type;

    if (D_800ADBDC == 0) {
        D_800B00C0 = 1;
        g_FieldScriptVMCurActor->scriptInstructionPointer -= 1;
        return;
    }
    D_800C3A20 = (s16)FieldScriptVMGetArgument(1);
    D_800C3A2A = (s16)FieldScriptVMGetArgument(3);
    D_800C3A2E = (s16)FieldScriptVMGetArgument(5);
    arg7 = FieldScriptVMGetArgument(7);
    type = arg7 & 0xF;
    D_800C3A30 = (s16)arg7;
#ifdef TITLE_CHAIN_MUTANT_FE60_DROPS_CIRCLE_GATE
    /* Deliberate mutant (pc_port/tests/run_title_newgame_chain.sh): drop bit
     * 0x80, the flag func_800A7C58 needs before it polls Circle. */
    D_800ADB80 = arg7 & 0x40;
#else
    D_800ADB80 = arg7 & 0xC0;
#endif
    D_800C3A32 = 0x140;
    D_800C3A34 = 0x100;
    D_800C3A30 = (s16)type;
    D_800C3A2C = 1;

    switch (type) {
        case 0:
            D_800C3A22 = 0x140;
            D_800C3A24 = 0;
            D_800C3A26 = 0x140;
            D_800C3A28 = 0x100;
            D_800ADB74 = 1;
            D_800C3A36 = 0;
            break;
        case 1:
            D_800C3A26 = 0;
            D_800C3A24 = 0;
            D_800C3A22 = 0;
            D_800C3A28 = 0x100;
            D_800ADB74 = 0;
            D_800C3A36 = 0;
            break;
        case 2:
            D_800C3A26 = 0;
            D_800C3A24 = 0;
            D_800C3A22 = 0;
            D_800C3A28 = 0x100;
            D_800ADB74 = 0;
            D_800C3A36 = 1;
            break;
    }
    D_800C3A38 = 0xFF;
    D_800C3A3A = 0;
    D_800ADB70 = 1;
    D_800B00C0 = 1;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 9;
}
#endif /* XENO_PC_PORT */

extern s16 D_800C3A20, D_800C3A22, D_800C3A24, D_800C3A26, D_800C3A28, D_800C3A2A, D_800C3A2C, D_800C3A2E;
extern s16 D_800C3A30, D_800C3A32, D_800C3A34, D_800C3A36, D_800C3A38, D_800C3A3A;
extern s32 D_800ADB80, D_800ADB74, D_800ADB70;

#ifndef XENO_PC_PORT
/* func_8008EE14: the IP read must be the plain-pointer form (it stays below the
 * scalar-global stores) while the IP store is the struct form, and D_800ADB70 = 1
 * comes after it in source: sched2 then lifts that sw into the lhu delay slot
 * (an in-struct varying store never conflicts with a fixed scalar store). */

#endif
void func_8008EE14(void) {
#ifdef XENO_PC_PORT
    u16 arg9;
    D_800C3A20 = (s16)FieldScriptVMGetArgument(1);
    D_800C3A2A = (s16)FieldScriptVMGetArgument(3);
    D_800C3A2C = (s16)FieldScriptVMGetArgument(5);
    D_800C3A2E = (s16)FieldScriptVMGetArgument(7);
    arg9 = (u16)FieldScriptVMGetArgument(9);
    D_800C3A30 = (s16)arg9;
    if (arg9 == 0xFE) {
        D_800C3A3A = 1;
    } else {
        D_800C3A3A = 0;
    }
    D_800C3A22 = (s16)FieldScriptVMGetArgument(11);
    D_800C3A26 = D_800C3A22;
    D_800C3A24 = (s16)FieldScriptVMGetArgument(13);
    D_800C3A28 = D_800C3A24;
    D_800C3A32 = (s16)FieldScriptVMGetArgument(15);
    D_800C3A34 = (s16)FieldScriptVMGetArgument(17);
    D_800ADB80 = arg9 & 0x40;
    D_800C3A38 = 0xFF;
    D_800ADB74 = 2;
    D_800C3A36 = 0;
    D_800C3A30 = (s16)(arg9 & 0xF);
    D_800ADB70 = 1;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 0x13;
#else
    s32 temp_s0;
    u16 temp_v0;
    u16 temp_v0_2;
    u16 temp_v0_3;

    D_800C3A20 = FieldScriptVMGetArgument(1);
    D_800C3A2A = FieldScriptVMGetArgument(3);
    D_800C3A2C = FieldScriptVMGetArgument(5);
    D_800C3A2E = FieldScriptVMGetArgument(7);
    temp_v0 = FieldScriptVMGetArgument(9);
    temp_s0 = temp_v0 & 0xFFFF;
    D_800C3A30 = temp_v0;
    if (temp_s0 == 0xFE) {
        D_800C3A3A = 1;
    } else {
        D_800C3A3A = 0;
    }
    temp_v0_2 = FieldScriptVMGetArgument(0xB);
    D_800C3A22 = temp_v0_2;
    D_800C3A26 = temp_v0_2;
    temp_v0_3 = FieldScriptVMGetArgument(0xD);
    D_800C3A24 = temp_v0_3;
    D_800C3A28 = temp_v0_3;
    D_800C3A32 = FieldScriptVMGetArgument(0xF);
    D_800C3A34 = FieldScriptVMGetArgument(0x11);
    D_800ADB80 = temp_s0 & 0x40;
    D_800C3A38 = 0xFF;
    D_800ADB74 = 2;
    D_800C3A36 = 0;
    D_800C3A30 &= 0xF;
    g_FieldScriptVMCurActor->scriptInstructionPointer = *(u16*)((u8*)g_FieldScriptVMCurActor + 0xCC) + 0x13;
    D_800ADB70 = 1;
#endif
}

extern s32 D_800ADB3C;
extern s32 D_800ADB38;

void func_8008EF5C(void) {
    D_800ADB3C = FieldScriptVMGetArgument(1);
    D_800ADB38 = 1;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

void func_8008EFA0(void) {
    D_800ADB3C = FieldScriptVMGetArgument(1);
    D_800ADB38 = 2;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

void func_8008EFE4(void) {
#ifdef XENO_PC_PORT
    s32 arg1 = FieldScriptVMGetArgument(1);
    s32 arg3 = FieldScriptVMGetArgument(3);
    s32 arg5 = FieldScriptVMGetArgument(5);
    s32 arg7 = FieldScriptVMGetArgument(7);
    FieldSetClipDimensions((short)arg1, (short)arg3, (short)arg5, (short)arg7);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 9;
#else
    s32 temp_s0;
    s32 temp_s1;
    s32 temp_s2;

    temp_s2 = FieldScriptVMGetArgument(1);
    temp_s1 = FieldScriptVMGetArgument(3);
    temp_s0 = FieldScriptVMGetArgument(5);
    FieldSetClipDimensions(temp_s2, temp_s1, temp_s0, FieldScriptVMGetArgument(7));
    *(u16 *)((s8*)(g_FieldScriptVMCurActor) + 0xCC) = (u16) (*(u16 *)((s8*)(g_FieldScriptVMCurActor) + 0xCC) + 9);
#endif
}

void func_8008F070(void) {
    D_800ADB3C = FieldScriptVMGetArgument(1);
    D_800ADB38 = 3;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

void func_8008F0B4(void) {
#ifdef XENO_PC_PORT
    s32 actorIdx = FieldScriptVMGetActorIndex(2);
    if (actorIdx != 0xFF) {
        u8* pFieldActor = (u8*)g_FieldActors + actorIdx * 92;
        u8* pScript = g_FieldScriptVMCurScriptData;
        u8 flags = pScript[g_FieldScriptVMCurActor->scriptInstructionPointer + 1];
        u32 pSub = *(u32*)(pFieldActor + 0x4C);

        if (flags & 1) {
            *(u8*)(pSub + 0xFC) = (u8)FieldScriptVMGetArgument(3);
            *(u8*)(pSub + 0xFD) = (u8)FieldScriptVMGetArgument(5);
            *(u8*)(pSub + 0xFE) = (u8)FieldScriptVMGetArgument(7);
        }
        if (flags & 2) {
            *(u8*)(pSub + 0xFF) = (u8)FieldScriptVMGetArgument(3);
            *(u8*)(pSub + 0x100) = (u8)FieldScriptVMGetArgument(5);
            *(u8*)(pSub + 0x101) = (u8)FieldScriptVMGetArgument(7);
        }
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 9;
#else
    s32 actorIdx = FieldScriptVMGetActorIndex(2);
    if (actorIdx != 0xFF) {
        u8* pSub = (u8*)(uintptr_t)g_FieldActors[actorIdx].pActorData;
        if (SCRIPT_READ_U8_REL(1) & 1) {
            pSub[0xFC] = FieldScriptVMGetArgument(3);
            pSub[0xFD] = FieldScriptVMGetArgument(5);
            pSub[0xFE] = FieldScriptVMGetArgument(7);
        }
        if (SCRIPT_READ_U8_REL(1) & 2) {
            pSub[0xFF] = FieldScriptVMGetArgument(3);
            pSub[0x100] = FieldScriptVMGetArgument(5);
            pSub[0x101] = FieldScriptVMGetArgument(7);
        }
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 9;
#endif
}

/* Field-script opcode: conditionally (per the opcode's flag byte at IP+1) copy
 * two operand triples into the actor's color fields (unkFC..FE / unkFF..101),
 * then IP += 8. As a no-op stub it never advanced the IP -> MAP160's VM
 * desynced and fed a garbage actor index (128) to
 * FieldScriptVMHandlerEnableActorVM (misc6.c:80) -- the same FE07-class desync
 * fixed for MAP3 (func_80088198/func_8008B180). */
void func_8008F1C8(void) {
    if (SCRIPT_READ_U8_REL(1) & 0x1) {
        g_FieldScriptVMCurActor->unkFC = (u8)FieldScriptVMGetArgument(2);
        g_FieldScriptVMCurActor->unkFD = (u8)FieldScriptVMGetArgument(4);
        g_FieldScriptVMCurActor->unkFE = (u8)FieldScriptVMGetArgument(6);
    }
    if (SCRIPT_READ_U8_REL(1) & 0x2) {
        g_FieldScriptVMCurActor->unkFF = (u8)FieldScriptVMGetArgument(2);
        g_FieldScriptVMCurActor->unk100 = (u8)FieldScriptVMGetArgument(4);
        g_FieldScriptVMCurActor->unk101 = (u8)FieldScriptVMGetArgument(6);
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 8;
}

void func_8008F2D8(void) {
    func_80023290((void*)(uintptr_t)g_FieldActors[D_800AFD1C].pSpriteData, FieldScriptVMGetArgument(1));
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

extern s32 D_800C3A5C;
extern s32 D_800C3A60;

void func_8008F348(void) {
    D_800C3A5C = FieldScriptVMGetArgument(1);
    D_800C3A60 = FieldScriptVMGetArgument(3);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
}

extern s16 D_800B233C[];

void func_8008F394(void) {
    D_800B233C[0] = FieldScriptVMGetArgument(1);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

void func_8008F3D0(void) {
    func_8003A450(FieldScriptVMGetArgument(3) << 1, FieldScriptVMGetArgument(1), FieldScriptVMGetArgument(5));
    g_FieldScriptVMCurActor->scriptInstructionPointer += 7;
}

void func_8008F444(void) {
    func_8003A344(FieldScriptVMGetArgument(3) << 1, FieldScriptVMGetArgument(1));
    g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
}

void func_8008F4A0(void) {
    func_8003A55C(FieldScriptVMGetArgument(3) << 1, FieldScriptVMGetArgument(1));
    g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
}

void func_8008F4FC(void) {
    func_80085634(FieldScriptVMGetArgument(1), FieldScriptVMGetArgument(3));
    g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
}

void func_8008F558(void) {
    s32 soundId = FieldScriptVMGetArgument(1);
    s32 volume = FieldScriptVMGetArgument(5);
    s32 pan = FieldScriptVMGetArgument(3);
    s32 channel = FieldScriptVMGetArgument(7);

    func_800855C8(soundId, volume, pan, channel);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 9;
}

/* FE 64 — sound-bit gate (asm 8008F5E4-8008F664).
 * FieldScriptVM2Run leaves IP on the 0x64 sub-opcode. Poll
 * func_8003A5D0(-1) & (arg1<<8): clear → IP+=3 (skip 64+2 args);
 * set → IP-=1 (back to FE, wait). Always yields (D_800B00C0=1).
 * A stubbed FE64 left IP on 0x64 and let the primary VM desync
 * A1 r5 past SHOW (0x22), so Fei stayed hidden after A14 HideById. */
extern s32 func_8003A5D0(s32);

void func_8008F5E4(void) {
    s32 soundBits = func_8003A5D0(-1);
    s32 mask = FieldScriptVMGetArgument(1) << 8;

    if ((soundBits & mask) == 0) {
        g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
    } else {
        g_FieldScriptVMCurActor->scriptInstructionPointer -= 1;
    }
    D_800B00C0 = 1;
}

void func_8008F668(void) {
    func_80085634(FieldScriptVMGetArgument(1), 3);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

void func_8008F6AC(void) {
    func_800855C8(FieldScriptVMGetArgument(1), FieldScriptVMGetArgument(5), FieldScriptVMGetArgument(3), 3);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 7;
}

extern s32 D_800ADBDC;
extern s32 D_8004F340;
extern s32 D_800ADB1C;
extern s32 D_8004F324;
extern s32 D_8004F308;
extern s32 D_8004F354;
extern void func_80085EEC(void);
extern void func_8001B66C(void);
extern void func_80085B20(s32);

void func_8008F724(void) {
    if (D_800ADBDC == 0) {
        D_800B00C0 = 1;
    } else {
        D_8004F340 = 0;
        func_8008F7B8();
    }
}

void func_8008F76C(void) {
    if (D_800ADBDC == 0) {
        D_800B00C0 = 1;
    } else {
        D_8004F340 = -1;
        func_8008F7B8();
    }
}

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/field/nonmatchings/main/misc", func_8008F7B8);
#else
void func_8008F7B8(void) {
    s32 fieldId = FieldScriptVMGetArgument(1);

    if (D_800ADB1C == 0) {
        func_80085EEC();
        if (fieldId != D_8004F324) {
            func_8001B66C();
            D_8004F308 = -1;
        }
        D_8004F324 = fieldId;
        g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
        return;
    }

    if (func_8008A558() != 0) {
        D_800B00C0 = 1;
        return;
    }

    if (D_800ADBDC == 0) {
        D_800B00C0 = 1;
        return;
    }

    if (D_8004F354 == 1 || D_8004F308 == -1) {
        D_800B00C0 = 1;
        return;
    }

    if (fieldId != D_8004F324) {
        func_8001B66C();
        D_8004F324 = fieldId;
        D_8004F308 = -1;
        func_80085B20(fieldId);
    }

    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}
#endif /* XENO_PC_PORT */

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/field/nonmatchings/main/misc", func_8008F90C);
#else
void func_8008F90C(void) {
    u8* pScene = (u8*)&g_Scene;
    s32 arg1 = FieldScriptVMGetArgument(1);
    s32 arg3 = FieldScriptVMGetArgument(3);
    s32 arg5 = FieldScriptVMGetArgument(5);
    s32 steps = FieldScriptVMGetArgument(7);
    s32 dx, dy, dz;

    if (steps == 0) { steps = 1; }

    g_FieldScriptVMCurActor->scriptInstructionPointer += 9;

    /* Retail does the shift/subtract with wrapping MIPS arithmetic and a
     * non-trapping div (INT_MIN / -1 = INT_MIN); do it on u32 so the port has
     * no signed-overflow / shift-of-negative UB. */
    {
        u32 sceneA0 = *(u32*)(pScene + 0xA0);
        u32 sceneA4 = *(u32*)(pScene + 0xA4);
        u32 nx = ((u32)arg1 << 16) - sceneA0;
        u32 nz = ((u32)arg5 << 16) - sceneA4;
        u32 ny = ((u32)arg3 << 16) - sceneA4;
        if (steps == -1) {
            dx = (s32)(0u - nx);
            dz = (s32)(0u - nz);
            dy = (s32)(0u - ny);
        } else {
            dx = (s32)nx / steps;
            dz = (s32)nz / steps;
            dy = (s32)ny / steps;
        }
    }

    *(s16*)(pScene + 0x9A) = (s16)steps;
    *(s16*)(pScene + 0x98) = 1;
    *(s32*)(pScene + 0xAC) = dx;
    *(s32*)(pScene + 0xB0) = dz;
    *(s32*)(pScene + 0xB4) = dy;

    if (arg1 == 0 && arg5 == 0 && arg3 == 0) {
        *(s16*)(pScene + 0x9A) = (s16)(steps + 2);
        *(s16*)(pScene + 0x9C) = 1;
    } else {
        *(s16*)(pScene + 0x9C) = 0;
    }
}
#endif /* XENO_PC_PORT */
