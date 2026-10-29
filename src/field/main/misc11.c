#include "common.h"
#include "main/game.h"
#include "system/math.h"
#include "field/main.h"
#include "field/camera.h"
#include "field/actor.h"
#include "field/script_vm.h"
#include "field/text_box.h"
#include "field/particles.h"

extern s32 D_800AFD1C;
extern s32 g_PlayerActorIndex;
extern FieldActor* D_800B06B8;

extern u8 D_800B2190;
extern u8 D_800B2191;
extern u8 D_800B2192;
extern u8 D_800B2194;
extern u8 D_800B2195;
extern u8 D_800B2196;
extern s16 D_800B2198;
extern s16 D_800B219A;
extern s16 D_800B218E;
extern void func_80073E38(void);

/* Opcode 0xE6 — SETUP_FOG: near RGB, far RGB, near/far Z, enable depth-cue. */
void func_80091944(void) {
    D_800B2190 = FieldScriptVMGetArgument(1);
    D_800B2191 = FieldScriptVMGetArgument(3);
    D_800B2192 = FieldScriptVMGetArgument(5);
    D_800B2194 = FieldScriptVMGetArgument(7);
    D_800B2195 = FieldScriptVMGetArgument(9);
    D_800B2196 = FieldScriptVMGetArgument(0xB);
    D_800B2198 = FieldScriptVMGetArgument(0xD);
    D_800B219A = FieldScriptVMGetArgument(0xF);
    D_800B218E = 1;
    func_80073E38();
    g_FieldScriptVMCurActor->scriptInstructionPointer += 0x11;
}

/* Typed array view of g_Scene: retail schedules these stores like stores to
 * an array-typed extern. */
extern s16 g_Scene_s16[] asm("g_Scene");

void func_80091A08(void) {
    g_Scene_s16[0x4C / 2] = FieldScriptVMGetInstructionArgumentS16(1);
    g_Scene_s16[0x4E / 2] = FieldScriptVMGetInstructionArgumentS16(3);
    g_Scene_s16[0x50 / 2] = FieldScriptVMGetInstructionArgumentS16(5);
    g_Scene_s16[0x52 / 2] = -FieldScriptVMGetInstructionArgumentS16(7);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 9;
}

extern u8 D_800B219C;
extern u8 D_800B219D;
extern u8 D_800B219E;

void func_80091A78(void) {
#ifdef XENO_PC_PORT
    D_800B219C = FieldScriptVMGetArgument(1);
    D_800B219D = FieldScriptVMGetArgument(3);
    D_800B219E = FieldScriptVMGetArgument(5);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 7;
#else
    (*(s8*)&D_800B219C) = FieldScriptVMGetArgument(1);
    (*(s8*)&D_800B219D) = FieldScriptVMGetArgument(3);
    (*(s8*)&D_800B219E) = FieldScriptVMGetArgument(5);
    *(u16 *)((s8*)(g_FieldScriptVMCurActor) + 0xCC) = (u16) (*(u16 *)((s8*)(g_FieldScriptVMCurActor) + 0xCC) + 7);
#endif
}

void func_80091AD4(void) {
}

extern s16 D_800AF5E8[];
extern s16 D_800AF6E8[];
extern s16 D_800AF728[];
extern u16 D_800AF768;

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/field/nonmatchings/main/misc11", func_80091ADC);
#else
void func_80091ADC(s16 x, s16 y, s16 w, s16 h, s16 extra, s16 mode, s32 doMove) {
    u16 idx = D_800AF768 & 0x1F;
    s16* rect = (s16*)((u8*)D_800AF5E8 + idx * 8);
    rect[0] = x;
    rect[1] = y;
    rect[2] = w;
    rect[3] = h;
    D_800AF6E8[idx] = extra;
    D_800AF728[idx] = mode;
    if (doMove) {
        ClearImage((RECT*)rect, 0, 0, 0);
    } else {
        MoveImage((RECT*)rect, D_800AF6E8[idx], (s16)mode);
    }
    D_800AF768++;
}
#endif /* XENO_PC_PORT */

extern int FieldScriptArgument5(int index, int mask);
extern int FieldScriptArgument6(int index, int mask);

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/field/nonmatchings/main/misc11", func_80091BBC);
#else
void func_80091BBC(void) {
    u8 mask;
    s32 arg1, arg2, arg3, arg4, arg5, arg6;

    mask = SCRIPT_READ_U8_REL(0xD);
    arg1 = FieldScriptArgument1(1, mask);
    mask = SCRIPT_READ_U8_REL(0xD);
    arg2 = FieldScriptArgument2(3, mask);

    if (arg1 == 0 && arg2 == 0) {
        mask = SCRIPT_READ_U8_REL(0xD);
        arg3 = FieldScriptArgument3(5, mask);
        mask = SCRIPT_READ_U8_REL(0xD);
        arg4 = FieldScriptArgument4(7, mask);
        mask = SCRIPT_READ_U8_REL(0xD);
        arg5 = FieldScriptArgument5(9, mask);
        mask = SCRIPT_READ_U8_REL(0xD);
        arg6 = FieldScriptArgument6(0xB, mask);
        func_80091ADC(arg3, arg4, arg5, arg6, 0, 0, 1);
    } else {
        mask = SCRIPT_READ_U8_REL(0xD);
        arg3 = FieldScriptArgument3(5, mask);
        mask = SCRIPT_READ_U8_REL(0xD);
        arg4 = FieldScriptArgument4(7, mask);
        mask = SCRIPT_READ_U8_REL(0xD);
        arg5 = FieldScriptArgument5(9, mask);
        mask = SCRIPT_READ_U8_REL(0xD);
        arg6 = FieldScriptArgument6(0xB, mask);
        func_80091ADC(arg1, arg2, arg3, arg4, arg5, arg6, 0);
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 0xE;
}
#endif /* XENO_PC_PORT */

#ifndef XENO_PC_PORT
/* func_80091E00: struct-field flags134 store (in-struct) lets the script-data
 * load for the second operand hoist above it. */
#endif
void func_80091E00(void) {
#ifdef XENO_PC_PORT
    u8 mask = SCRIPT_READ_U8_REL(5);
    s32 arg1 = FieldScriptArgument1(1, mask);
    s32 motionType = arg1 & 3;
    u32 flags = *(u32*)((u8*)g_FieldScriptVMCurActor + 0x134);
    *(u32*)((u8*)g_FieldScriptVMCurActor + 0x134) = (flags & 0xFFFFFF9F) | (motionType << 5);
    mask = SCRIPT_READ_U8_REL(5);
    *(u16*)((u8*)g_FieldScriptVMCurActor + 0xEE) = (u16)FieldScriptArgument2(3, mask);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 6;
#else
    s32 arg1 = FieldScriptArgument1(1, SCRIPT_READ_U8_REL(5));
    g_FieldScriptVMCurActor->flags134 = (g_FieldScriptVMCurActor->flags134 & ~0x60) | ((arg1 & 3) << 5);
    g_FieldScriptVMCurActor->unkEE = FieldScriptArgument2(3, SCRIPT_READ_U8_REL(5));
    g_FieldScriptVMCurActor->scriptInstructionPointer += 6;
#endif
}

extern int FieldScriptVMGetActorIndex(int);
extern int FieldScriptArgument1(int index, int mask);
extern int FieldScriptArgument2(int index, int mask);

void func_80091E98(void) {
#ifdef XENO_PC_PORT
    u8 mask;
    FieldActor* pActor;
    void* pActorData;
    s32 motionType;

    if (FieldScriptVMGetActorIndex(1) == ACTOR_ID_INVALID) {
        g_FieldScriptVMCurActor->scriptInstructionPointer += 7;
        return;
    }
    pActor = &g_FieldActors[FieldScriptVMGetActorIndex(1)];
    pActorData = (void*)(uintptr_t)pActor->pActorData;
    mask = SCRIPT_READ_U8_REL(6);
    motionType = FieldScriptArgument1(2, mask) & 3;
    *(u32*)((u8*)pActorData + 0x134) = (*(u32*)((u8*)pActorData + 0x134) & 0xFFFFFF9F) | (motionType << 5);
    mask = SCRIPT_READ_U8_REL(6);
    *(s16*)((u8*)pActorData + 0xEE) = (s16)FieldScriptArgument2(4, mask);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 7;
#else
    ActorData* pActorData;

    if (FieldScriptVMGetActorIndex(1) != 0xFF) {
        pActorData = (ActorData*)(uintptr_t)g_FieldActors[FieldScriptVMGetActorIndex(1)].pActorData;
        pActorData->flags134 = (pActorData->flags134 & ~0x60) | ((FieldScriptArgument1(2, SCRIPT_READ_U8_REL(6)) & 3) << 5);
        pActorData->unkEE = FieldScriptArgument2(4, SCRIPT_READ_U8_REL(6));
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 7;
#endif
}

/* ---- func_80091F84: VM opcode 0xDB — write clamped value into the actor's
 * animation dispatch table.
 * asm 80091F84-80092040: arg(1) = table index, arg(3) = value, clamped to
 * 0xFFF (slti 0x1000 / ori 0xFFF). If g_FieldActors[D_800AFD1C].status has
 * bit 0x2000, stores it at ((s32*)actorData->unk118)[index] — the 0x80-byte
 * table func_80080A74 allocates under the same status gate. IP += 5
 * unconditionally (sh at 8009202C). */
void func_80091F84(void) {
    s32 index = FieldScriptVMGetArgument(1);
    s32 value = FieldScriptVMGetArgument(3);

    if (value >= 0x1000) {
        value = 0xFFF;
    }
    if (g_FieldActors[D_800AFD1C].status & 0x2000) {
        ((s32*)(uintptr_t)g_FieldScriptVMCurActor->unk118)[index] = value;
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
}

void func_80092044(void) {
    u16 addr1, addr2, addr3, addr4;
    s32 val1, val2;
    addr1 = (u16)FieldScriptVMGetInstructionArgument(1);
    val1 = FieldScriptVMGetVariableValue(addr1);
    addr2 = (u16)FieldScriptVMGetInstructionArgument(3);
    val2 = FieldScriptVMGetVariableValue(addr2);
    addr3 = (u16)FieldScriptVMGetInstructionArgument(3);
    FieldScriptMemoryWriteU16(addr3, val1);
    addr4 = (u16)FieldScriptVMGetInstructionArgument(1);
    FieldScriptMemoryWriteU16(addr4, val2);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
}

extern s16 D_800AFEA8;
extern void GfxLineScrollUpdate(void* pLineScroll);

#ifndef XENO_PC_PORT
/* func_800920D8: D_800AFEA8 is a count followed by an entry table (count re-read
 * as entries[-1] each iteration); reached through a >8-byte struct alias so it
 * is not gp-small (la + lh 0(v1)).  No separate guard: the for test is it. */
struct func_800920D8_s {
    s16 count;
    s16 pad;
    void* entries[16];
};
extern struct func_800920D8_s D_800AFEA8__s asm("D_800AFEA8");

#endif
void func_800920D8(void) {
#ifdef XENO_PC_PORT
    s32 i;
    u32* entries;

    if (D_800AFEA8 <= 0) {
        return;
    }

    entries = (u32*)((u8*)&D_800AFEA8 + 4);
    for (i = 0; i < D_800AFEA8; i++) {
        GfxLineScrollUpdate((void*)(uintptr_t)entries[i]);
    }
#else
    s32 i;

    for (i = 0; i < D_800AFEA8__s.count; i++) {
        GfxLineScrollUpdate(D_800AFEA8__s.entries[i]);
    }
#endif
}

extern s16 D_800AFFAC[];

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/field/nonmatchings/main/misc11", func_80092148);
#else
/* Retail reaches the line-scroll table through D_800AFFAC (== D_800AFEA8+0x104,
 * the s16 size[] table) and D_800AFFAC-0x80 (== D_800AFEA8+0x84, the u32
 * buffer[] slots).  On PC D_800AFFAC is a separate stub, so address both
 * through D_800AFEA8 + offset with retail 4-byte pointer slots.  The byte
 * view is an incomplete array so the host compiler does not take D_800AFEA8's
 * declared s16 size as the object size. */
extern u8 D_800AFEA8_tbl[] asm("D_800AFEA8");

void func_80092148(void) {
    s32 arg1 = FieldScriptVMGetArgument(1);
    s32 arg3 = FieldScriptVMGetArgument(3);
    u8 val = (u8)FieldScriptVMGetInstructionArgument(5);
    u8* tbl = D_800AFEA8_tbl;

    if (arg3 < ((s16*)(tbl + 0x104))[arg1]) {
        *(u8*)(uintptr_t)(((u32*)(tbl + 0x84))[arg1] + (u32)arg3) = val;
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 7;
}
#endif /* XENO_PC_PORT */

extern s32 D_800ADB8C;
extern s16 D_800AFEA8;

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/field/nonmatchings/main/misc11", func_800921E8);
#else
/* Retail table at D_800AFEA8: s16 count @+0, u32 lineScroll[32] @+4,
 * u32 buffer[32] @+0x84, s16 size[32] @+0x104 (== D_800AFFAC).  The pointer
 * tables are 4-byte guest slots (consumers func_800920D8/func_80092148 read
 * u32[idx]); a host void* store would be 8 bytes wide and overrun the next
 * table, so store the pointers as u32 slots like retail's sw. */
void func_800921E8(void) {
    u8* tbl = D_800AFEA8_tbl;
    u32 count;
    u8 initVal;
    s32 i;
    s16 x, y, modulus, width, destX, destY;

    if (D_800ADB8C == 0 && *(s16*)tbl < 0x20) {
        count = FieldScriptVMGetInstructionArgument(9) & 0xFFFF;
        ((s16*)(tbl + 0x104))[*(s16*)tbl] = (s16)count;
        ((u32*)(tbl + 0x84))[*(s16*)tbl] = (u32)(uintptr_t)HeapAlloc(count + 1, 0);
        ((u32*)(tbl + 0x4))[*(s16*)tbl] = (u32)(uintptr_t)HeapAlloc(0x18, 0);
        initVal = (u8)FieldScriptVMGetInstructionArgument(0xF);
        for (i = 0; i < (s32)count; i++) {
            *(u8*)(uintptr_t)(((u32*)(tbl + 0x84))[*(s16*)tbl] + (u32)i) = initVal;
        }

        x = FieldScriptVMGetInstructionArgument(1);
        y = FieldScriptVMGetInstructionArgument(3);
        modulus = FieldScriptVMGetInstructionArgument(5);
        width = FieldScriptVMGetInstructionArgument(7);
        destX = FieldScriptVMGetInstructionArgument(0xB);
        destY = FieldScriptVMGetInstructionArgument(0xD);

        GfxLineScrollInitialize(
            (void*)(uintptr_t)((u32*)(tbl + 0x4))[*(s16*)tbl],
            x, y, modulus, width, (s16)count, destX, destY,
            (s8*)(uintptr_t)((u32*)(tbl + 0x84))[*(s16*)tbl]);

        *(u16*)tbl = *(u16*)tbl + 1;
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 0x11;
}
#endif /* XENO_PC_PORT */

void func_800923E4(void) {
    g_FieldScriptVMCurActor->scriptInstructionPointer++;
}

void func_80092404(void) {
    g_FieldScriptVMCurActor->scriptInstructionPointer++;
}

#ifdef XENO_PC_PORT
/* FieldLoad stores packed 32-bit addresses here, including the next layer
 * pointer at +4. A native pointer load would combine those two words. */
extern u32 D_800AFB20[];
#define FIELD_COLLISION_FLAGS ((u8*)(uintptr_t)D_800AFB20[0])
#else
extern u32* D_800AFB20;
#define FIELD_COLLISION_FLAGS ((u8*)D_800AFB20)
#endif

u32 func_80092424(s32 index, s32 component) {
    switch (component) {
        case 0: return FIELD_COLLISION_FLAGS[index * 4];
        case 1: return FIELD_COLLISION_FLAGS[index * 4 + 1];
        case 2: return FIELD_COLLISION_FLAGS[index * 4 + 2];
        case 3: return FIELD_COLLISION_FLAGS[index * 4 + 3];
    }
    return 0;
}

void func_800924D4(s32 index, s32 component, s32 value) {
#ifdef XENO_PC_PORT
    u32* entry;
    u32 mask;

    switch (component) {
        case 0:
            entry = (u32*)(FIELD_COLLISION_FLAGS + index * 4);
            *entry = (*entry & ~0xFFu) | (u32)value;
            break;
        case 1:
            entry = (u32*)(FIELD_COLLISION_FLAGS + index * 4);
            mask = 0xFFFF00FF;
            *entry = (*entry & mask) | ((u32)value << 8);
            break;
        case 2:
            entry = (u32*)(FIELD_COLLISION_FLAGS + index * 4);
            mask = 0xFF00FFFF;
            *entry = (*entry & mask) | ((u32)value << 16);
            break;
        case 3:
            entry = (u32*)(FIELD_COLLISION_FLAGS + index * 4);
            mask = 0x00FFFFFF;
            *entry = (*entry & mask) | ((u32)value << 24);
            break;
    }
#else
    switch (component) {
        case 0:
            D_800AFB20[index] = (D_800AFB20[index] & 0xFFFFFF00) | value;
            break;
        case 1:
            value = (u32)value << 8; D_800AFB20[index] = (D_800AFB20[index] & 0xFFFF00FF) | value;
            break;
        case 2:
            value = (u32)value << 16; D_800AFB20[index] = (D_800AFB20[index] & 0xFF00FFFF) | value;
            break;
        case 3:
            value = (u32)value << 24; D_800AFB20[index] = (D_800AFB20[index] & 0x00FFFFFF) | value;
            break;
    }
#endif
}

extern s32 D_800B217C;
extern s16 D_800B21D6;

void func_800925A0(void) {
    s32 arg1 = FieldScriptVMGetArgument(1);
    D_800B217C = arg1;
    switch (arg1) {
        case 0: D_800B21D6 = 8; break;
        case 1: D_800B21D6 = 6; break;
        case 2: D_800B21D6 = 4; break;
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

void FieldScriptVMHandlerSetControllerBtnMask(void) {
    g_FieldControl.controllerBtnMask = FieldScriptVMGetInstructionArgument(1);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

void func_80092664(void) {
    func_800924D4(SCRIPT_READ_U8_REL(1), SCRIPT_READ_U8_REL(2), FieldScriptVMGetArgument(3));
    g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
}

void func_800926C8(void) {
    u32 cur = func_80092424(SCRIPT_READ_U8_REL(1), SCRIPT_READ_U8_REL(2));
    cur |= FieldScriptVMGetArgument(3);
    func_800924D4(SCRIPT_READ_U8_REL(1), SCRIPT_READ_U8_REL(2), cur);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
}

void func_80092768(void) {
    u32 cur = func_80092424(SCRIPT_READ_U8_REL(1), SCRIPT_READ_U8_REL(2));
    cur &= FieldScriptVMGetArgument(3);
    func_800924D4(SCRIPT_READ_U8_REL(1), SCRIPT_READ_U8_REL(2), cur);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
}

void func_80092808(void) {
#ifdef XENO_PC_PORT
    u8* actorData = (u8*)g_FieldScriptVMCurActor;
    s16 angle = *(s16*)((u8*)D_800B06B8 + 0x52);
    s32 sinTerm = rsin(angle) * 9;
    s32 cosTerm;
    s32 zMove;

    *(s16*)(actorData + 0x60) = (s16)(u16)((u32)sinTerm >> 10);

    cosTerm = rcos(angle) * 9;
    zMove = -(cosTerm * 4);
    *(s16*)(actorData + 0x64) = (s16)(zMove >> 12);

    *(u32*)(actorData + 0x4) |= 0x800;
    *(u16*)(actorData + 0xCC) += 1;
#else
    g_FieldScriptVMCurActor->unk60.vx = (u32)(rsin(D_800B06B8->rotation.y) * 9) >> 10;
    g_FieldScriptVMCurActor->unk60.vz = -(rcos(D_800B06B8->rotation.y) * 36) >> 12;
    g_FieldScriptVMCurActor->flags |= 0x800;
    g_FieldScriptVMCurActor->scriptInstructionPointer++;
#endif
}

extern FieldActor* D_800B06B8;
extern long FieldGetVec1Magnitude(long x);
extern long FieldGetVec2Magnitude(long x, long y);
extern s32 func_8007B694(s32* arg0);
extern void func_800821F4(void* pSpriteData, s16 animIndex, void* pFieldActor);
extern s32 g_GameSceneMapNum;
extern s32 D_800ADBDC;
extern s32 D_800ADBE4;
extern s32 D_800ADBEC;

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/field/nonmatchings/main/misc11", func_80092894);
#else
s32 func_80092894(s32 arg1, s32 arg2, s32 arg3, s32 arg4) {
    s32 vec[3];
    s32 targetX, targetZ;
    s32 angle;
    s32 dist;
    s32 threshold;
    s16 savedX, savedZ;
    u8* pActorData;
    u8* pSpriteData;
    u8* cur;
    u8* slot;
    u32 v;

    g_FieldControl.isRandomEncountersEnabled = -1;

    pActorData = (u8*)(uintptr_t)g_FieldActors[g_PlayerActorIndex].pActorData;
    pSpriteData = (u8*)(uintptr_t)g_FieldActors[g_PlayerActorIndex].pSpriteData;

    *(u32*)(pActorData + 0x4) |= 0x38;
    *(u32*)(pSpriteData + 0x18) = 0x80000;

    threshold = (s32)((u32)FieldGetVec1Magnitude(8) << 1);
    savedX = *(s16*)(pActorData + 0x22);
    savedZ = *(s16*)(pActorData + 0x2A);

    /* arg2 != 0: walk straight to (arg3, arg4); no exit-point or angle math. */
    targetX = arg3;
    targetZ = arg4;
    if (arg2 == 0) {
        if (D_800ADBDC == 0 || D_800ADBE4 == 0) {
            D_800B00C0 = 1;
        }
        if (D_800ADBEC != 0) {
            s32 entrance = FieldScriptVMGetArgument(4);
            s32 mapNum = FieldScriptVMGetArgument(2);
            func_80092F44();
            D_800ADBEC = 0;
            FieldScriptMemoryWriteU16(2, entrance);
            g_GameSceneMapNum = mapNum;
        }

        /* Target is 40 units ahead of the script's current actor (not the
         * player), along D_800B06B8's facing + arg1 - 0x400. */
        angle = (s32)((u32)*(s16*)((u8*)D_800B06B8 + 0x52) + (u32)arg1 - 0x400u);
        v = (u32)rsin(angle);
        cur = (u8*)g_FieldScriptVMCurActor;
        targetX = (s32)((u32)(*(s16*)(cur + 0x60) + *(s16*)(cur + 0x22)) + (u32)((s32)(v * 40u) >> 12));
        v = (u32)rcos(angle);
        cur = (u8*)g_FieldScriptVMCurActor;
        targetZ = (s32)((u32)(*(s16*)(cur + 0x64) + *(s16*)(cur + 0x2A)) + (u32)((s32)(0u - v * 40u) >> 12));
    }

    /* func_8007B694 reads the retail 3-word stack vector {dx, 0, dz}. */
    vec[0] = (s32)((u32)targetX - (u32)savedX);
    vec[1] = 0;
    vec[2] = (s32)((u32)targetZ - (u32)savedZ);
    dist = (s32)FieldGetVec2Magnitude(vec[0], vec[2]);

    if (threshold < dist) {
        /* Player hasn't reached target yet */
        if (*(s16*)(pActorData + 0x68) == *(s16*)(pActorData + 0x22) &&
            *(s16*)(pActorData + 0x6A) == *(s16*)(pActorData + 0x26) &&
            *(s16*)(pActorData + 0x6C) == *(s16*)(pActorData + 0x2A)) {
            *(u16*)(pActorData + 0x6E) = *(u16*)(pActorData + 0x6E) + 1;
        } else {
            *(u16*)(pActorData + 0x6E) = 0;
        }
        if (*(s16*)(pActorData + 0x6E) < 0x41) {
            s32 heading = func_8007B694(vec);
            *(u16*)(pActorData + 0x104) = (u16)heading;
            *(u16*)(pActorData + 0x106) = (u16)heading;
            D_800B00C0 = 1;
            if (arg2 != 0) {
                g_FieldScriptVMCurActor->scriptInstructionPointer--;
            }
            return -1;
        }
    }

    /* Player reached target (or got stuck) — finalize transition */
    {
        u16 flags = *(u16*)(pActorData + 0x106) | 0x8000;
        *(u16*)(pActorData + 0x104) = flags;
        *(u16*)(pActorData + 0x106) = flags;
    }
    *(u32*)(pSpriteData + 0x18) = 0;
    *(s16*)(pActorData + 0xE8) = 0;
    func_800821F4(pSpriteData, 0, &g_FieldActors[g_PlayerActorIndex]);
    D_800B00C0 = 1;
    *(u16*)(pActorData + 0x90 + *(u8*)(pActorData + 0xCE) * 8) = 0xFFFF;
    /* retail: lw/and/sw of the same slot after the sh; byte copies keep the
     * host compiler from reordering the u16 store and the u32 load. */
    slot = pActorData + 0x90 + *(u8*)(pActorData + 0xCE) * 8;
    __builtin_memcpy(&v, slot, 4);
    v &= 0xFE7FFFFF;
    __builtin_memcpy(slot, &v, 4);
    *(u32*)(pActorData + 0x0) &= 0xFFDFFFFF;
    if (arg2 == 1) {
        *(u32*)(pActorData + 0x4) &= ~0x38u;
    }
    *(s16*)(pActorData + 0x6E) = 0;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 6;
    return 0;
}
#endif /* XENO_PC_PORT */

extern s32 D_800B2350;
extern s32 D_800ADBDC;
extern s32 D_800ADBE4;
extern s32 D_800ADB2C;
extern s32 D_8004F308;
extern s32 D_800ADB90;

#ifndef XENO_PC_PORT
/* func_80092C20: D_800B2350 is not gp-small in retail (its address lives in s0
 * across the calls), so it is read/written through an incomplete-array alias;
 * func_800A0C4C/func_80092894 are only declared after this site. */
extern s32 D_800B2350__a[] asm("D_800B2350");
extern void func_800A0C4C(void);
extern s32 func_80092894(s32, s32, s32, s32);

#endif
void func_80092C20(void) {
#ifdef XENO_PC_PORT
    s32 arg1, arg2;
    s32 result;

    if (D_800ADBDC == 0 || D_800ADBE4 == 0 || D_800ADB2C != 0 ||
        D_8004F308 == -1 || D_800ADB90 != 0) {
        D_800B00C0 = 1;
        g_FieldScriptVMCurActor->scriptInstructionPointer--;
        return;
    }

    arg1 = FieldScriptArgument1(1, SCRIPT_READ_U8_REL(5));
    arg2 = FieldScriptArgument2(3, SCRIPT_READ_U8_REL(5));

    if (D_800B2350 == 0) {
        FieldActor* pPlayer = &g_FieldActors[g_PlayerActorIndex];
        D_800B2350 = *(s32*)(void*)(uintptr_t)pPlayer->pActorData;
    }

    func_800A0C4C();
    result = func_80092894(0, 1, arg1, arg2);
    if (result != 0) {
        return;
    }

    if (!(D_800B2350 & 0x80)) {
        FieldActor* pPlayer = &g_FieldActors[g_PlayerActorIndex];
        u32* pData = (u32*)(void*)(uintptr_t)pPlayer->pActorData;
        *pData = *pData & ~0x80u;
    }
    D_800B2350 = 0;
#else
    s32 arg1, arg2;

    if (D_800ADBDC == 0 || D_800ADBE4 == 0 || D_800ADB2C != 0 || D_8004F308 == -1 || D_800ADB90 != 0) {
        D_800B00C0 = 1;
        g_FieldScriptVMCurActor->scriptInstructionPointer--;
        return;
    }
    arg1 = FieldScriptArgument1(1, SCRIPT_READ_U8_REL(5));
    arg2 = FieldScriptArgument2(3, SCRIPT_READ_U8_REL(5));
    if (D_800B2350__a[0] == 0) {
        D_800B2350__a[0] = *(s32*)(uintptr_t)g_FieldActors[g_PlayerActorIndex].pActorData;
    }
    func_800A0C4C();
    if (func_80092894(0, 1, arg1, arg2) == 0) {
        if (!(D_800B2350__a[0] & 0x80)) {
            *(u32*)(uintptr_t)g_FieldActors[g_PlayerActorIndex].pActorData &= ~0x80;
        }
        D_800B2350__a[0] = 0;
    }
#endif
}

extern s32 D_800ADBDC;
extern s32 D_800ADBE4;
extern s32 D_800ADB2C;
extern s32 D_8004F308;
extern s32 D_800ADB90;
extern void func_800A0C4C(void);
extern s32 func_80092894(s32, s32, s32, s32);

void func_80092DFC(void) {
    if (D_800ADBDC == 0 || D_800ADBE4 == 0 || D_800ADB2C != 0 ||
        D_8004F308 == -1 || D_800ADB90 != 0) {
        D_800B00C0 = 1;
    } else {
        func_800A0C4C();
        func_80092894(0, 0, 0, 0);
    }
}

void func_80092EA0(void) {
    if (D_800ADBDC == 0 || D_800ADBE4 == 0 || D_800ADB2C != 0 ||
        D_8004F308 == -1 || D_800ADB90 != 0) {
        D_800B00C0 = 1;
    } else {
        func_800A0C4C();
        func_80092894(0x3E0, 0, 0, 0);
    }
}

extern s32 g_GameSceneMapNum;

void func_80092F44(void) {
    FieldScriptMemoryWriteU16(4, g_GameSceneMapNum & 0x3FFF);
    FieldScriptMemoryWriteU16(6, FieldGetPlayerActorDirection() & 0xFFFF);
    FieldScriptMemoryWriteU16(8, FieldGetCameraDirection() & 0xFFFF);
    FieldScriptMemoryWriteU16(0x12, (s16)(FieldScriptVMGetVariableValue(0x12) + 1));
}

extern s32 D_800ADBD8;
extern s32 D_800B0064;

void func_80092FB4(void) {
    if (D_800ADBD8 != 0) {
        g_FieldControl.isRandomEncountersEnabled = -1;
        D_800ADBD8 = 0;
        D_800B0064 = FieldScriptVMGetArgument(1);
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

extern u8 D_800B02C8;
extern void func_800A31E8(void);
extern int FieldScriptArgument1(int index, int mask);
extern int FieldScriptArgument2(int index, int mask);
extern int FieldScriptArgument3(int index, int mask);
extern int FieldScriptArgument4(int index, int mask);
void func_800931F8(void);

#ifdef XENO_PC_PORT
extern u32 g_PcPortOpcode56Func8009FEE4ConditionCount;
extern void PcPort_FieldOpcode56TransitionIntercept(
    s32 readinessBefore, u32 refreshConditionCountBefore,
    s32 opcodeActor, u16 opcodeIp);
extern void PcPort_FieldOpcode56RecordControlLock(
    s16 controlBefore, s32 actor, u16 lockIp);

/* VM opcode 0x56 -- snapshot the outgoing field and arm the four-halfword
 * field/world-map transition tuple.  Retail 0x80093014-0x800931F4.
 *
 * This is deliberately separate from the port harness's transition hold. The
 * body below performs every retail write, including clearing D_800ADBE4. The
 * port harness observes retail's complete FE54 -> 0x56 departure sequence:
 * FE54 records the pre-lock control value, and this body's final hook logs the
 * fully armed state before restoring anything. With
 * XENO_FIELD_HOLD_TRANSITION=1, the hook restores D_800ADBE4 to prevent
 * FieldMain teardown into the still-stubbed func_8007954C exit-1 arm and, only
 * after an actor/IP-adjacent FE54 snapshot, restores the pre-lock control value
 * so the artificially held field remains playable. A real F15 departure keeps
 * retail's deliberate lock -> arm -> teardown choreography. */
void func_80093014(void) {
    s32 readinessBefore;
    u32 refreshConditionCountBefore;
    s32 opcodeActor;
    u16 opcodeIp;
    s32 arg3;
    s32 heading;
    s32 mask;

    if (D_800ADBDC == 0 || D_800ADBE4 == 0 || D_800ADB2C != 0 ||
        D_8004F308 == -1 || D_800ADB90 != 0) {
        D_800B00C0 = 1;
        return;
    }

    readinessBefore = D_800ADBE4;
    opcodeActor = D_800AFD1C;
    opcodeIp = g_FieldScriptVMCurActor->scriptInstructionPointer;
    refreshConditionCountBefore =
        g_PcPortOpcode56Func8009FEE4ConditionCount;

    func_800A31E8();
    g_FieldControl.isRandomEncountersEnabled = -1;
    D_800ADBE4 = 0;

    mask = SCRIPT_READ_U8_REL(9);
    *(s16*)((u8*)g_pGameState + 0x231A) =
        (s16)FieldScriptArgument1(1, mask);

    /* Retail stores this halfword but no reader is currently identified. */
    *(s16*)((u8*)g_pGameState + 0x231E) =
        (s16)FieldScriptArgument2(3, mask);

    arg3 = FieldScriptArgument3(5, mask);
    if (arg3 == -1 || (u16)arg3 == 0xFFFF) {
        heading = *(u16*)((u8*)&g_Scene + 0x56) + 0x800;
    } else {
        heading = arg3 + 0x800;
    }
    *(s16*)((u8*)g_pGameState + 0x231C) = (s16)(heading & 0xFFF);

    /* This store is the retail jal func_800931F8 delay-slot side effect. */
    *(s16*)((u8*)g_pGameState + 0x2320) =
        (s16)FieldScriptArgument4(7, mask);
    func_800931F8();

    D_800B02C8 = 1;
    D_800B00C0 = 1;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 0xA;

    PcPort_FieldOpcode56TransitionIntercept(
        readinessBefore, refreshConditionCountBefore, opcodeActor, opcodeIp);
}
#else
/* func_80093014: (arg3 & 0xFFFF) == 0xFFFF | arg3 == -1 is a non-short-circuit
 * |, the 0x231C store is written in both arms (cross-jumped tail, g_pGameState
 * load stays per arm) through the raw form, the 0x231A/0x231E stores are pointer
 * indexed, and the IP update is raw-read/struct-store before D_800B00C0 = 1.
 * This retail definition also replaces the former NON_MATCHING draft. */
void func_80093014(void) {
    s32 arg3;

    if (D_800ADBDC == 0 || D_800ADBE4 == 0 || D_800ADB2C != 0 || D_8004F308 == -1 || D_800ADB90 != 0) {
        D_800B00C0 = 1;
        return;
    }
    func_800A31E8();
    g_FieldControl.isRandomEncountersEnabled = -1;
    D_800ADBE4 = 0;
    ((s16*)g_pGameState)[0x231A / 2] = FieldScriptArgument1(1, SCRIPT_READ_U8_REL(9));
    ((s16*)g_pGameState)[0x231E / 2] = FieldScriptArgument2(3, SCRIPT_READ_U8_REL(9));
    arg3 = FieldScriptArgument3(5, SCRIPT_READ_U8_REL(9));
    if (((arg3 & 0xFFFF) == 0xFFFF) | (arg3 == -1)) {
        *(s16*)((u8*)g_pGameState + 0x231C) = (*(u16*)((u8*)&g_Scene + 0x56) + 0x800) & 0xFFF;
    } else {
        *(s16*)((u8*)g_pGameState + 0x231C) = (arg3 + 0x800) & 0xFFF;
    }
    ((s16*)g_pGameState)[0x2320 / 2] = FieldScriptArgument4(7, SCRIPT_READ_U8_REL(9));
    func_800931F8();
    D_800B02C8 = 1;
    g_FieldScriptVMCurActor->scriptInstructionPointer = *(u16*)((u8*)g_FieldScriptVMCurActor + 0xCC) + 0xA;
    D_800B00C0 = 1;
}

#endif

void func_800931F8(void) {
}

extern s32 D_800B0048;
extern s32 D_800AFD14;
void func_800932D0(void);

void func_80093200(void) {
    if (D_800ADBDC == 0 || D_800ADBE4 == 0 || D_800ADB2C != 0 ||
        D_8004F308 == -1 || D_800ADB90 != 0) {
        D_800B00C0 = 1;
    } else {
        func_800932D0();
        func_800931F8();
        D_800B0048 = FieldScriptVMGetArgument(0);
        D_800AFD14 = FieldScriptVMGetArgument(2);
        g_FieldScriptVMCurActor->scriptInstructionPointer += 4;
    }
}

extern s32 D_800ADB70;
extern s32 D_800ADBEC;
extern s32 g_GameSceneMapNum;

// VM opcode 152, CHANGE_FIELD: request a map transition to arg(1)/entrance
// arg(3). Gated on six readiness flags (archive/CD-load and camera-cut state);
// while any is unsatisfied the instruction does not advance and the VM yields
// to retry it next frame. Once clear, it snapshots the outgoing scene via
// func_80092F44, clears the pending-load flag, writes the entrance into VM
// variable slot 2 and the new map number, then kicks off the load
// (func_800931F8 is a genuine empty function in retail).
void func_800932D0(void) {
    s32 entranceArg;
    s32 mapArg;

    if (D_800ADBDC == 0 || D_800ADBE4 == 0 || D_800ADB2C != 0 ||
        D_8004F308 == -1 || D_800ADB90 != 0 || D_800ADB70 != 0) {
        D_800B00C0 = 1;
        return;
    }

    g_FieldControl.isRandomEncountersEnabled = -1;

    if (D_800ADBEC != 0) {
        entranceArg = FieldScriptVMGetArgument(3);
        mapArg = FieldScriptVMGetArgument(1);
#ifdef XENO_PC_PORT
        { extern void PcPort_RouteLogChangeField(int, int);
          PcPort_RouteLogChangeField(mapArg, entranceArg); } /* inert unless XENO_ROUTE_LOG */
#endif
        func_80092F44();
        D_800ADBEC = 0;
        FieldScriptMemoryWriteU16(2, entranceArg);
        g_GameSceneMapNum = mapArg;
        func_800931F8();
    }

    D_800B00C0 = 1;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
}

extern s32 D_800ADB18;
extern s32 D_800ADBE0;
extern s32 D_800ADB88;
extern u8 D_8005954C;
extern u8 D_80059508;
extern u8 D_800594F8;
extern u8 D_800B2356;

void func_800933F8(void) {
    if (D_800ADBDC == 0 || D_800ADBE4 == 0 || D_800ADBEC == 0 ||
        D_800ADB2C != 0 || D_8004F308 == -1 || D_800ADB90 != 0) {
        D_800B00C0 = 1;
        g_FieldScriptVMCurActor->scriptInstructionPointer--;
        return;
    }
    D_8005954C = D_800B2356;
    D_80059508 = (u8)FieldScriptVMGetArgument(1);
    D_800594F8 = 0;
    D_800ADBDC = 0;
    D_800ADBE0 = 0;
    D_800ADB88 = 1;
    {
        s32 mapNum = FieldScriptVMGetArgument(5);
        if (mapNum != 0x7FFF) {
            s32 entrance = FieldScriptVMGetArgument(7);
#ifdef XENO_PC_PORT
            { extern void PcPort_RouteLogChangeField(int, int);
              PcPort_RouteLogChangeField(mapNum, entrance); } /* inert unless XENO_ROUTE_LOG */
#endif
            func_80092F44();
            FieldScriptMemoryWriteU16(2, entrance);
            g_GameSceneMapNum = mapNum;
            D_800ADB18 = 1;
        }
    }
    D_800B00C0 = 1;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 9;
}

extern u8 D_800B2356;
extern u8 D_8005954C;
extern u8 D_80059508;
extern u8 D_800594F8;
extern s32 D_800ADBE0;
extern s32 D_800ADB88;

void func_80093568(void) {
    if (D_800ADBDC == 0 || D_800ADBE4 == 0 || D_800ADBEC == 0 ||
        D_800ADB2C != 0 || D_8004F308 == -1 || D_800ADB90 != 0) {
        D_800B00C0 = 1;
    } else {
        D_8005954C = D_800B2356;
        D_80059508 = (u8)FieldScriptVMGetArgument(1);
        D_800594F8 = 0;
        D_800ADBDC = 0;
        D_800ADBE0 = 0;
        D_800ADB88 = 1;
        D_800B00C0 = 1;
        g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
    }
}

void func_80093664(void) {
    u8 b1 = SCRIPT_READ_U8_REL(1);
    u8 b2 = SCRIPT_READ_U8_REL(2);
    s32 result = func_80092424(b1, b2);
    u16 addr = (u16)FieldScriptVMGetInstructionArgument(3);
    FieldScriptMemoryWriteU16(addr, result);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
}

extern s32 D_8004F350;

void func_800936E4(void) {
    if (D_8004F350 == 0) {
        g_FieldScriptVMCurActor->scriptInstructionPointer += 1;
    } else {
        g_FieldScriptVMCurActor->scriptInstructionPointer -= 1;
    }
    D_800B00C0 = 1;
}

extern s32 D_8004F350;
extern s8 D_80059171;
extern s32 D_800ADB64;
extern u16 D_800B236C;

void func_80093740(void) {
    D_800B00C0 = 1;
    D_800ADB64 = 0;
    D_80059171 = D_800B236C;
    D_8004F350++;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 1;
}

void func_80093790(void) {
    D_80059171 = 1;
    D_800ADB64 = 6;
    D_800B00C0 = 1;
    D_8004F350++;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 1;
}

/* FE57: request the title menu (MenuExecute slot 2 -> func_801C62A8 ->
 * func_801C58EC).  FieldMain's opener func_800799D4 consumes D_800ADB64. */
void func_800937E0(void) {
#ifdef TITLE_CHAIN_MUTANT_FE57_WRONG_MENU
    /* Deliberate mutant (pc_port/tests/run_title_newgame_chain.sh): request
     * the field system menu instead of the title menu. */
    D_800ADB64 = 0;
#else
    D_800ADB64 = 2;
#endif
    D_800B00C0 = 1;
    D_8004F350++;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 1;
}

void func_80093824(void) {
    D_80059171 = FieldScriptVMGetArgument(1);
    D_800ADB64 = 0x3;
    D_800B00C0 = 1;
    D_8004F350++;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

void func_80093888(void) {
    s32 arg3, arg1;
    g_FieldControl.isRandomEncountersEnabled = -1;
    arg3 = FieldScriptVMGetArgument(3);
    arg1 = FieldScriptVMGetArgument(1);
    func_80092F44();
    FieldScriptMemoryWriteU16(2, arg3);
    g_GameSceneMapNum = arg1;
    func_800931F8();
    D_800ADB64 = 1;
    D_800B00C0 = 1;
    D_8004F350++;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
}

extern s32 D_800ADB64;
extern s32 D_800C3A6A;
extern s32 D_8004F350;

#ifndef XENO_PC_PORT
/* func_80093930: the actor pointer is taken into a local before the two raw
 * g_pGameState stores (which keep the D_800C3A6A store behind them); D_800C3A6A
 * is a halfword in retail (sh) though the TU declares it s32. */
#endif
void func_80093930(void) {
#ifdef XENO_PC_PORT
    s32 arg1 = FieldScriptVMGetArgument(1);
    D_800ADB64 = 1;
    D_800B00C0 = 1;
    *(s16*)((u8*)g_pGameState + 0x1932) = (s16)arg1;
    *(s16*)((u8*)g_pGameState + 0x2320) = (s16)arg1;
    D_800C3A6A = (s16)arg1;
    D_8004F350++;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
#else
    s32 arg1 = FieldScriptVMGetArgument(1);
    ActorData* actor;
    D_800ADB64 = 1;
    D_800B00C0 = 1;
    actor = g_FieldScriptVMCurActor;
    *(s16*)((u8*)g_pGameState + 0x1932) = arg1;
    *(s16*)((u8*)g_pGameState + 0x2320) = arg1;
    *(s16*)&D_800C3A6A = arg1;
    D_8004F350++;
    actor->scriptInstructionPointer += 3;
#endif
}

void func_800939A0(void) {
    D_80059171 = FieldScriptVMGetArgument(1);
    D_800ADB64 = 0x4;
    D_800B00C0 = 1;
    D_8004F350++;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

void func_80093A04(void) {
    D_80059171 = FieldScriptVMGetArgument(1);
    D_800ADB64 = 0x5;
    D_800B00C0 = 1;
    D_8004F350++;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}


// Random encounter stuff

// These are likely part of a struct
extern s8 D_800B21D0[];
extern s8 D_800B21D1[];

void func_80093A68(void) {
    g_Scene.unk48 &= 0x7FFF;
    g_FieldScriptVMCurActor->scriptInstructionPointer++;
}

void func_80093A98(void) {
    g_Scene.unk48 |= 0x8000;
    g_FieldScriptVMCurActor->scriptInstructionPointer++;
}

void func_80093AC8(void) {
    g_FieldControl.isRandomEncountersEnabled = 0;
    D_800B21D0[0] = 0;
    D_800B21D1[0] = 0;
    g_Scene.unk48 &= 0x3FFF;
    g_FieldScriptVMCurActor->scriptInstructionPointer++;
}

void func_80093B10(void) {
#ifdef XENO_PC_PORT
    /* FE54 is retail's departure input lock. Record its prior value without
     * predicting what follows; opcode 0x56 consumes it only when actor and IP
     * prove this was the immediately preceding instruction. */
    PcPort_FieldOpcode56RecordControlLock(
        g_FieldControl.isRandomEncountersEnabled,
        D_800AFD1C,
        g_FieldScriptVMCurActor->scriptInstructionPointer);
#endif
    g_FieldControl.isRandomEncountersEnabled = -1;
    D_800B21D0[0] = 1;
    D_800B21D1[0] = 1;
    g_Scene.unk48 |= 0xC000;
    if ((D_800ADBDC == 0) || (D_800ADBE4 == 0)) {
        D_800B00C0 = 1; // Stop script VM execution?
        g_FieldScriptVMCurActor->scriptInstructionPointer--;
        return;
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer++;
}

void func_80093BB0(void) {
    D_800B21D0[0] = 0x0;
    g_FieldScriptVMCurActor->scriptInstructionPointer++;
}

void func_80093BD4(void) {
    D_800B21D0[0] = 0x1;
    g_FieldScriptVMCurActor->scriptInstructionPointer++;
}

void FieldScriptEnableCompass(void) {
    D_800B21D1[0] = 0x0;
    g_FieldScriptVMCurActor->scriptInstructionPointer++;
}

void FieldScriptDisableCompass(void) {
    D_800B21D1[0] = 0x1;
    g_FieldScriptVMCurActor->scriptInstructionPointer++;
}

void FieldScriptVMHandlerDisableRandomEncounters(void) {
    g_FieldControl.isRandomEncountersEnabled = 0;
    g_FieldScriptVMCurActor->scriptInstructionPointer++;
}

void func_80093C6C(void) {
    if (D_800ADBDC == 0 || D_800ADBE4 == 0) {
        D_800B00C0 = 1;
        return;
    }
    g_FieldControl.isRandomEncountersEnabled = -1;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 1;
}
// End of random encounter stuff


// Write U8 Handler
// Arg1 + Arg3 = Offset to U8 value in script
// Arg2 = Write address
void func_80093CD0(void) {
    unsigned short offset;
    unsigned short arg1;

    arg1 = FieldScriptVMGetInstructionArgument(1);
    offset = arg1 + FieldScriptVMGetArgument(5);
    FieldScriptMemoryWriteU16(
        SCRIPT_IMM_ARG(2), 
        SCRIPT_READ_U8(offset)
    );
    g_FieldScriptVMCurActor->scriptInstructionPointer += 7;
}

// Write U16/S16 handler
// Arg1 + Arg3 = Offset in script to short to write
// Arg3: Write address
void func_80093D48(void) {
    int arg1;
    int nAddress;
    unsigned short nOffset;
    int nValue;
    int arg3;

    arg1 = FieldScriptVMGetInstructionArgument(1);
    nOffset = arg1 + FieldScriptVMGetArgument(5);
    if (SCRIPT_READ_U8_REL(7) == 0) {
        // W/O carry
        FieldScriptMemoryWriteU16(
            SCRIPT_IMM_ARG(2),
            SCRIPT_READ_U8(nOffset) | (SCRIPT_READ_U8(nOffset + 1) << 8)
        );
    } else {
        // With carry
        FieldScriptMemoryWriteU16(
            SCRIPT_IMM_ARG(2),
            (short)(SCRIPT_READ_U8(nOffset) + (SCRIPT_READ_U8(nOffset + 1) << 8))
        );
    }
    
    g_FieldScriptVMCurActor->scriptInstructionPointer += 8;
}

// The two functions seems to be related to handling room transitions
void func_80093E30(void) {
    if (!g_FieldScriptVMCurActor->scriptFlags.fields.scriptFlags_0x13) {
        if (!(g_FieldScriptVMCurActor->flags12C_0x5)) {
            g_FieldScriptVMCurActor->flags12C_0x5 = 1;
            g_FieldScriptVMCurActor->curDoorStep = 0;
            func_80085634(8, 3);
        } else {
            g_FieldScriptVMCurActor->curDoorStep++;
            if (g_FieldScriptVMCurActor->curDoorStep < 0x1F) {
                if (SCRIPT_READ_U8_REL(1) == 0) {
                    g_FieldActors[D_800AFD1C].rotation.y += 0x20;
                } else {
                    g_FieldActors[D_800AFD1C].rotation.y -= 0x20;
                }
            } else {
                g_FieldScriptVMCurActor->scriptFlags.fields.scriptFlags_0x13 = 0x1;
                g_FieldScriptVMCurActor->flags12C_0x5 = 0;
                g_FieldScriptVMCurActor->curDoorStep = 0;
                g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
            }
        }
    } else {
        g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
    }
    
    func_80072254(D_800AFD1C);
}

void func_80093FC0(void) {
    if (g_FieldScriptVMCurActor->scriptFlags.fields.scriptFlags_0x13) {
        if (!(g_FieldScriptVMCurActor->flags12C_0x5)) {
            g_FieldScriptVMCurActor->flags12C_0x5 = 1;
            g_FieldScriptVMCurActor->curDoorStep = 0;
            func_80085634(8, 3);
        } else {
            g_FieldScriptVMCurActor->curDoorStep++;
            if (g_FieldScriptVMCurActor->curDoorStep < 0x1F) {
                if (SCRIPT_READ_U8_REL(1) == 0) {
                    g_FieldActors[D_800AFD1C].rotation.y -= 0x20;
                } else {
                    g_FieldActors[D_800AFD1C].rotation.y += 0x20;
                }
            } else {
                g_FieldScriptVMCurActor->scriptFlags.fields.scriptFlags_0x13 = 0;
                g_FieldScriptVMCurActor->flags12C_0x5 = 0;
                g_FieldScriptVMCurActor->curDoorStep = 0;
                g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
            }
        }
    } else {
        g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
    }
    func_80072254(D_800AFD1C);
}

extern void func_80072254(int);
extern void func_80085634(int,int);
extern s32 D_800AFD1C;
extern FieldActor* D_800B06B8;

void func_80094158() {
    ActorData* pActor;
    int angle;
    int delta;

    if (!g_FieldScriptVMCurActor->scriptFlags.fields.scriptFlags_0x13) {
        if (!g_FieldScriptVMCurActor->flags12C_0x5) {
            g_FieldScriptVMCurActor->flags12C_0x5 = 0x1;
            g_FieldScriptVMCurActor->curDoorStep = 0;
            func_80085634(8, 3);
            g_FieldScriptVMCurActor->unkD0.vx = g_FieldScriptVMCurActor->position.vx;
            g_FieldScriptVMCurActor->unkD0.vy = g_FieldScriptVMCurActor->position.vy;
            g_FieldScriptVMCurActor->unkD0.vz = g_FieldScriptVMCurActor->position.vz;
        } else {
            g_FieldScriptVMCurActor->curDoorStep++;
            if (g_FieldScriptVMCurActor->curDoorStep < FieldScriptVMGetArgument(3)) {
                switch (FieldScriptVMGetArgument(5)) { 
                    case 0x1000:
                        g_FieldScriptVMCurActor->unkD0.vy -= FieldScriptVMGetArgument(1) * 0x10;
                        D_800B06B8->transformMatrix.t[1] = CONV_TO_GTE(g_FieldScriptVMCurActor->unkD0.vz);
                        break;
                    case 0x1001:
                        g_FieldScriptVMCurActor->unkD0.vy += FieldScriptVMGetArgument(1) * 0x10;
                        D_800B06B8->transformMatrix.t[1] = CONV_TO_GTE(g_FieldScriptVMCurActor->unkD0.vz);
                        break;
                    default:
                        delta = PSX_DEGREES(90);
                        angle = D_800B06B8->rotation.y + FieldScriptVMGetArgument(5) - delta;
                        g_FieldScriptVMCurActor->unkD0.vx += rsin(angle) * FieldScriptVMGetArgument(1);
                        g_FieldScriptVMCurActor->unkD0.vz -= rcos(angle) * FieldScriptVMGetArgument(1);
                        D_800B06B8->transformMatrix.t[0] = CONV_TO_GTE(g_FieldScriptVMCurActor->unkD0.vx);
                        D_800B06B8->transformMatrix.t[2] = CONV_TO_GTE(g_FieldScriptVMCurActor->unkD0.vz);
                        break;
                }
            } else {
                g_FieldScriptVMCurActor->scriptFlags.fields.scriptFlags_0x13 = 1;
                g_FieldScriptVMCurActor->flags12C_0x5 = 0;
                g_FieldScriptVMCurActor->curDoorStep = 0;   
                g_FieldScriptVMCurActor->scriptInstructionPointer += 7;
            }
        }
    } else {
        g_FieldScriptVMCurActor->scriptInstructionPointer += 7;
    }
    func_80072254(D_800AFD1C);
}

void func_800943AC(void) {
    ActorData* temp_a1;
    int angle;
    int delta;
    
    if (g_FieldScriptVMCurActor->scriptFlags.fields.scriptFlags_0x13) {
        if (!g_FieldScriptVMCurActor->flags12C_0x5) {
            g_FieldScriptVMCurActor->flags12C_0x5 = 0x1;
            g_FieldScriptVMCurActor->curDoorStep = 0x0;
            func_80085634(8, 3);
        } else {
            g_FieldScriptVMCurActor->curDoorStep++;
            if (g_FieldScriptVMCurActor->curDoorStep < FieldScriptVMGetArgument(3)) {
                switch (FieldScriptVMGetArgument(5)) {
                    case 0x1000:
                        g_FieldScriptVMCurActor->unkD0.vy -= FieldScriptVMGetArgument(1) * 0x10;
                        D_800B06B8->transformMatrix.t[1] = CONV_TO_GTE(g_FieldScriptVMCurActor->unkD0.vz);
                        break;
                    case 0x1001:
                        g_FieldScriptVMCurActor->unkD0.vy += FieldScriptVMGetArgument(1) * 0x10;
                        D_800B06B8->transformMatrix.t[1] = CONV_TO_GTE(g_FieldScriptVMCurActor->unkD0.vz);
                        break;
                    default:
                        delta = PSX_DEGREES(90);
                        angle = D_800B06B8->rotation.y + FieldScriptVMGetArgument(5) - delta;
                        g_FieldScriptVMCurActor->unkD0.vx -= rsin(angle) * FieldScriptVMGetArgument(1);
                        g_FieldScriptVMCurActor->unkD0.vz += rcos(angle) * FieldScriptVMGetArgument(1);
                        D_800B06B8->transformMatrix.t[0] = CONV_TO_GTE(g_FieldScriptVMCurActor->unkD0.vx);
                        D_800B06B8->transformMatrix.t[2] = CONV_TO_GTE(g_FieldScriptVMCurActor->unkD0.vz);
                        break;
                }
            } else {
                g_FieldScriptVMCurActor->scriptFlags.fields.scriptFlags_0x13 = 0x0;
                g_FieldScriptVMCurActor->flags12C_0x5 = 0x0;
                g_FieldScriptVMCurActor->curDoorStep = 0;
                g_FieldScriptVMCurActor->scriptInstructionPointer += 7;
            }
        }
    } else {
        g_FieldScriptVMCurActor->scriptInstructionPointer += 7;
    }
    func_80072254(D_800AFD1C);
}

extern s32 D_8004F318;
extern s32 D_8004F328;

void func_800945D4(void) {
    D_8004F318 = 0;
    D_8004F328 = 0xFF;
    FieldScriptMemoryWriteU16(
        0xA, 
        ((FieldScriptVMGetArgument(1) << 8) & 0xFF00) | (FieldScriptVMGetArgument(3) & 0xFF)
    );
    g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
}

void func_80094650(void) {
    D_8004F328 = SCRIPT_READ_U8_REL(1);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
}

void func_8009468C(void) {
    D_8004F318 = 0;
    D_8004F328 = 0xFF;
    g_FieldScriptVMCurActor->scriptInstructionPointer++;
}

void func_800946BC(void) {
    g_FieldScriptVMCurActor->flags12C_0 = 0x1;
    g_FieldScriptVMCurActor->unk70 = FieldScriptVMGetArgument(1);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

void func_80094710(void) {
    g_FieldScriptVMCurActor->flags12C_0 = 0x2;
    g_FieldScriptVMCurActor->unk70 = FieldScriptVMGetArgument(1);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

void func_80094764(void) {
    g_FieldScriptVMCurActor->flags12C_0 = 0x3;
    g_FieldScriptVMCurActor->unk70 = FieldScriptVMGetArgument(1);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

extern void func_80072254(int);

void func_800947B0(void) {
    FieldActor* pActor;

    if (FieldScriptVMGetActorIndex(2) != ACTOR_ID_INVALID) {
        pActor = &g_FieldActors[FieldScriptVMGetActorIndex(2)];
        switch (SCRIPT_READ_U8_REL(1)) {
            case 0:
                pActor->rotation.x += FieldScriptVMGetArgument(3);
                break;
            case 1:
                pActor->rotation.x -= FieldScriptVMGetArgument(3);
                break;
            case 2:
                pActor->rotation.y += FieldScriptVMGetArgument(3);
                break;
            case 3:
                pActor->rotation.y -= FieldScriptVMGetArgument(3);
                break;
            case 4:
                pActor->rotation.z += FieldScriptVMGetArgument(3);
                break;
            case 5:
                pActor->rotation.z -= FieldScriptVMGetArgument(3);
                break;
        }
        func_80072254(FieldScriptVMGetActorIndex(2));
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
}

// Set X, Y or Z rotaiton of Actor D_800AFD1C
void func_80094918(void) {
    switch (SCRIPT_READ_U8_REL(3)) { 
        case 0:
            g_FieldActors[D_800AFD1C].rotation.x = FieldScriptVMGetArgument(1);
            break;
        case 1:
            g_FieldActors[D_800AFD1C].rotation.y = FieldScriptVMGetArgument(1);
            break;
        case 2:
            g_FieldActors[D_800AFD1C].rotation.z = FieldScriptVMGetArgument(1);
            break;
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 4;
    func_80072254(D_800AFD1C);
}

// Increase X Rotation of Actor D_800AFD1C
void func_80094A5C(void) {
    (&g_FieldActors[D_800AFD1C])->rotation.x += FieldScriptVMGetArgument(1);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
    func_80072254(D_800AFD1C);
}

// Decrease X Rotation of Actor D_800AFD1C
void func_80094ACC(void) {
    (&g_FieldActors[D_800AFD1C])->rotation.x -= FieldScriptVMGetArgument(1);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
    func_80072254(D_800AFD1C);
}

// Increase Y Rotation of Actor D_800AFD1C
void func_80094B3C(void) {
    (&g_FieldActors[D_800AFD1C])->rotation.y += FieldScriptVMGetArgument(1);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
    func_80072254(D_800AFD1C);
}

// Decrease Y Rotation of Actor D_800AFD1C
void func_80094BAC(void) {
    (&g_FieldActors[D_800AFD1C])->rotation.y -= FieldScriptVMGetArgument(1);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
    func_80072254(D_800AFD1C);
}

// Increase Z Rotation of Actor D_800AFD1C
void func_80094C1C(void) {
    (&g_FieldActors[D_800AFD1C])->rotation.z += FieldScriptVMGetArgument(1);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
    func_80072254(D_800AFD1C);
}

// Decrease Z Rotation of Actor D_800AFD1C
void func_80094C8C(void) {
    (&g_FieldActors[D_800AFD1C])->rotation.z -= FieldScriptVMGetArgument(1);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
    func_80072254(D_800AFD1C);
}

s32 func_80094CFC(void) {
    s32 i;
    for (i = 0; i < 0x96; i++) {
        u8* p = (u8*)g_pGameState + i;
        if (p[0x1F90] == 0) {
            return i;
        }
        if (p[0x2026] == 0) {
            return i;
        }
    }
    return -1;
}

s32 func_80094D4C(void) {
    s32 i;
    for (i = 0; i < 0x64; i++) {
        u8* p = (u8*)g_pGameState + i;
        if (p[0x1D38] == 0) {
            return i;
        }
        if (p[0x1D9C] == 0) {
            return i;
        }
    }
    return -1;
}

s32 func_80094D9C(void) {
    s32 i;
    for (i = 0; i < 0xC8; i++) {
        u8* p = (u8*)g_pGameState + i;
        if (p[0x1E00] == 0) {
            return i;
        }
        if (p[0x1EC8] == 0) {
            return i;
        }
    }
    return -1;
}

s32 func_80094DEC(void) {
    s32 i;
    for (i = 0; i < 0x64; i++) {
        u8* p = (u8*)g_pGameState + i;
        if (p[0x20BC] == 0) {
            return i;
        }
        if (p[0x2120] == 0) {
            return i;
        }
    }
    return -1;
}

s32 func_80094E3C(void) {
    s32 i;
    for (i = 0; i < 0x96; i++) {
        u8* p = (u8*)g_pGameState + i;
        if (p[0x2184] == 0) {
            return i;
        }
        if (p[0x221A] == 0) {
            return i;
        }
    }
    return -1;
}

s32 func_80094E8C(s32 a0) {
    s32 i;
    for (i = 0; i < 0x96; i++) {
        u8* p = (u8*)g_pGameState + i;
        if (p[0x2026] == a0 && p[0x1F90] != 0) {
            return i;
        }
    }
    return -1;
}

s32 func_80094EDC(s32 a0) {
    s32 i;
    for (i = 0; i < 0xC8; i++) {
        u8* p = (u8*)g_pGameState + i;
        if (p[0x1EC8] == a0 && p[0x1E00] != 0) {
            return i;
        }
    }
    return -1;
}

s32 func_80094F2C(s32 a0) {
    s32 i;
    for (i = 0; i < 0x64; i++) {
        u8* p = (u8*)g_pGameState + i;
        if (p[0x1D9C] == a0 && p[0x1D38] != 0) {
            return i;
        }
    }
    return -1;
}

s32 func_80094F7C(s32 a0) {
    s32 i;
    for (i = 0; i < 0x64; i++) {
        u8* p = (u8*)g_pGameState + i;
        if (p[0x2120] == a0 && p[0x20BC] != 0) {
            return i;
        }
    }
    return -1;
}

s32 func_80094FCC(s32 a0) {
    s32 i;
    for (i = 0; i < 0x96; i++) {
        u8* p = (u8*)g_pGameState + i;
        if (p[0x221A] == a0 && p[0x2184] != 0) {
            return i;
        }
    }
    return -1;
}

u8* func_8009501C(s32 itemId) {
    switch (itemId >> 8) {
    case 0:
        return (u8*)g_pGameState + 0x2026;
    case 1:
        return (u8*)g_pGameState + 0x1D9C;
    case 2:
        return (u8*)g_pGameState + 0x1EC8;
    case 3:
        return (u8*)g_pGameState + 0x2120;
    case 4:
        return (u8*)g_pGameState + 0x221A;
    default:
        return NULL;
    }
}

u8* func_800950A0(s32 itemId) {
    switch (itemId >> 8) {
    case 0:
        return (u8*)g_pGameState + 0x1F90;
    case 1:
        return (u8*)g_pGameState + 0x1D38;
    case 2:
        return (u8*)g_pGameState + 0x1E00;
    case 3:
        return (u8*)g_pGameState + 0x20BC;
    case 4:
        return (u8*)g_pGameState + 0x2184;
    default:
        return NULL;
    }
}

s32 func_80095124(s32 itemId) {
    switch (itemId >> 8) {
    case 0:
        return func_80094E8C(itemId);
    case 1:
        return func_80094F2C(itemId - 0x100);
    case 2:
        return func_80094EDC(itemId - 0x200);
    case 3:
        return func_80094F7C(itemId - 0x300);
    case 4:
        return func_80094FCC(itemId - 0x400);
    default:
        return 0;
    }
}

s32 func_800951B8(s32 itemId) {
    switch (itemId >> 8) {
    case 0:
        return func_80094CFC();
    case 1:
        return func_80094D4C();
    case 2:
        return func_80094D9C();
    case 3:
        return func_80094DEC();
    case 4:
        return func_80094E3C();
    default:
        return 0;
    }
}

void func_8009524C(void) {
    func_80095284();
    g_FieldScriptVMCurActor->scriptInstructionPointer += 1;
}

/* ---- func_80095284: VM opcode 0x5B — halt actor movement and hold ----------
 * asm 80095284-800952FC: zeroes ActorData moveModified (+0x30..0x38) and move
 * (+0x40..0x48), ORs 0x8000 into rotation.vx/.vy (+0x104/+0x106, vy stored
 * first), zeroes three words in the FieldActor pSpriteData object (+0xC,
 * +0x14, +0x18), and sets D_800B00C0 = 1 (yields the VM turn). Does NOT
 * advance scriptInstructionPointer — the opcode re-runs every turn (hold).
 * func_8009524C (opcode 0x5A) is the halt-then-advance variant. */
#ifndef XENO_PC_PORT
/* func_80095284: actor in a local, rot read before the D_800B00C0 store, and the
 * | 0x8000 written inside the stores (a separate rot |= 0x8000 statement lets the
 * ori float up); the sprite clears use pointer indexing. */
#endif
void func_80095284(void) {
#ifdef XENO_PC_PORT
    FieldActor* fieldActor = &g_FieldActors[D_800AFD1C];
    u8* pSprite = (u8*)(uintptr_t)fieldActor->pSpriteData;
    ActorData* actor = g_FieldScriptVMCurActor;
    u16 rot = (u16)actor->rotation.vx;

    D_800B00C0 = 1;
    actor->moveModified.vx = 0;
    actor->moveModified.vy = 0;
    actor->moveModified.vz = 0;
    actor->move.vx = 0;
    actor->move.vy = 0;
    actor->move.vz = 0;
    rot |= 0x8000;
    actor->rotation.vy = rot;
    actor->rotation.vx = rot;
    *(s32*)(pSprite + 0x0C) = 0;
    *(s32*)(pSprite + 0x14) = 0;
    *(s32*)(pSprite + 0x18) = 0;
#else
    u8* pSprite;
    ActorData* actor;
    u16 rot;

    pSprite = (u8*)(uintptr_t)g_FieldActors[D_800AFD1C].pSpriteData;
    actor = g_FieldScriptVMCurActor;
    rot = actor->rotation.vx;
    D_800B00C0 = 1;
    actor->moveModified.vx = 0;
    actor->moveModified.vy = 0;
    actor->moveModified.vz = 0;
    actor->move.vx = 0;
    actor->move.vy = 0;
    actor->move.vz = 0;
    actor->rotation.vy = rot | 0x8000;
    actor->rotation.vx = rot | 0x8000;
    ((s32*)pSprite)[3] = 0;
    ((s32*)pSprite)[5] = 0;
    ((s32*)pSprite)[6] = 0;
#endif
}

void func_80095300(void) {
    g_FieldControl.unkAngle = FieldScriptVMGetArgument(1);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

// Check if an actor is within a trigger zone when considering the entire scene projected to
// 2D, top-down. If so, call a script function.
void FieldScriptHandleTriggerZone2D(void) {
    ActorData* pActor;
    long actorPosition2D;
    long triggerPos2;
    long triggerPos3;
    long triggerPos4;
    long triggerPos1;
    int triggerIndex;
    FieldTriggerZone* pTrigger;

    triggerIndex = SCRIPT_READ_U8_REL(1);
    pActor = (ActorData*)(uintptr_t)g_FieldActors[g_PlayerActorIndex].pActorData;

    // Pack Z and X position into a long for use w/ NormalClip
    actorPosition2D = (CONV_TO_GTE(pActor->position.vz) << 0x10) + CONV_TO_GTE(pActor->position.vx);
    
    // Consider trigger zone in 2D from a top-down
    triggerPos1 = (g_pFieldTriggerZones[triggerIndex].z0 << 0x10) + g_pFieldTriggerZones[triggerIndex].x0;
    triggerPos2 = (g_pFieldTriggerZones[triggerIndex].z1 << 0x10) + g_pFieldTriggerZones[triggerIndex].x1;
    triggerPos3 = (g_pFieldTriggerZones[triggerIndex].z2 << 0x10) + g_pFieldTriggerZones[triggerIndex].x2;
    triggerPos4 = (g_pFieldTriggerZones[triggerIndex].z3 << 0x10) + g_pFieldTriggerZones[triggerIndex].x3;
    
    if (NormalClip(triggerPos1, triggerPos2, actorPosition2D) >= 0 && 
        NormalClip(triggerPos2, triggerPos3, actorPosition2D) >= 0 && 
        NormalClip(triggerPos3, triggerPos4, actorPosition2D) >= 0 && 
        NormalClip(triggerPos4, triggerPos1, actorPosition2D) >= 0
    ) {
        // If we're inside the trigger zone, call script function based on argument
        if (g_FieldScriptVMCurActor->flags12C_0x6 != 0x4) {
            g_FieldScriptVMCurActor->scriptPointersStack[g_FieldScriptVMCurActor->flags12C_0x6] = g_FieldScriptVMCurActor->scriptInstructionPointer + 4;
            g_FieldScriptVMCurActor->scriptInstructionPointer = FieldScriptVMGetInstructionArgument(2);
            g_FieldScriptVMCurActor->flags12C_0x6++;;
            return;
        }
    }
    g_FieldScriptMaxInstructionCount += 1;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 4;
}

// Check if an actor is within a trigger zone, considering height of the trigger zone as well.
// If so, call a script function.
void FieldScriptHandleTriggerZone(void) {
    ActorData* pActor;
    int actorPositionY;
    long actorPosition2D;
    long triggerPos1;
    long triggerPos2;
    long triggerPos3;
    long triggerPos4;
    int triggerIndex;
    FieldTriggerZone* pTrigger;

    triggerIndex = SCRIPT_READ_U8_REL(1);
    pActor = (ActorData*)(uintptr_t)g_FieldActors[g_PlayerActorIndex].pActorData;

    actorPositionY = CONV_TO_GTE(pActor->position.vy);
    if (g_pFieldTriggerZones[triggerIndex].y0 < actorPositionY && 
        (actorPositionY - pActor->height) < g_pFieldTriggerZones[triggerIndex].y0
    ) {
        triggerPos1 = (g_pFieldTriggerZones[triggerIndex].z0 << 0x10) + g_pFieldTriggerZones[triggerIndex].x0;
        triggerPos2 = (g_pFieldTriggerZones[triggerIndex].z1 << 0x10) + g_pFieldTriggerZones[triggerIndex].x1;
        triggerPos3 = (g_pFieldTriggerZones[triggerIndex].z2 << 0x10) + g_pFieldTriggerZones[triggerIndex].x2;
        triggerPos4 = (g_pFieldTriggerZones[triggerIndex].z3 << 0x10) + g_pFieldTriggerZones[triggerIndex].x3;
        actorPosition2D = (CONV_TO_GTE(pActor->position.vz) << 0x10) + CONV_TO_GTE(pActor->position.vx);

        if (
            NormalClip(triggerPos1, triggerPos2, actorPosition2D) >= 0 && 
            NormalClip(triggerPos2, triggerPos3, actorPosition2D) >= 0 && 
            NormalClip(triggerPos3, triggerPos4, actorPosition2D) >= 0 && 
            NormalClip(triggerPos4, triggerPos1, actorPosition2D) >= 0
        ) {
            // If we're inside the trigger zone, call script function based on argument
            if (g_FieldScriptVMCurActor->flags12C_0x6 != 0x4) {
                g_FieldScriptVMCurActor->scriptPointersStack[g_FieldScriptVMCurActor->flags12C_0x6] = g_FieldScriptVMCurActor->scriptInstructionPointer + 4;
                g_FieldScriptVMCurActor->scriptInstructionPointer = FieldScriptVMGetInstructionArgument(2);
                g_FieldScriptVMCurActor->flags12C_0x6++;;
                return;
            }
        }
    }
    g_FieldScriptMaxInstructionCount += 1;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 4;
}

void FieldScriptCheckTriggerZone2D(void) {
    ActorData* pActor;
    long actorPosition2D;
    long triggerPos2;
    long triggerPos3;
    long triggerPos4;
    long triggerPos1;
    int triggerIndex;
    FieldTriggerZone* pTrigger;

    triggerIndex = SCRIPT_READ_U8_REL(1);
    pActor = (ActorData*)(uintptr_t)g_FieldActors[g_PlayerActorIndex].pActorData;

    // Pack Z and X position into a long for use w/ NormalClip
    actorPosition2D = (CONV_TO_GTE(pActor->position.vz) << 0x10) + CONV_TO_GTE(pActor->position.vx);
    
    // Consider trigger zone in 2D from a top-down
    triggerPos1 = (g_pFieldTriggerZones[triggerIndex].z0 << 0x10) + g_pFieldTriggerZones[triggerIndex].x0;
    triggerPos2 = (g_pFieldTriggerZones[triggerIndex].z1 << 0x10) + g_pFieldTriggerZones[triggerIndex].x1;
    triggerPos3 = (g_pFieldTriggerZones[triggerIndex].z2 << 0x10) + g_pFieldTriggerZones[triggerIndex].x2;
    triggerPos4 = (g_pFieldTriggerZones[triggerIndex].z3 << 0x10) + g_pFieldTriggerZones[triggerIndex].x3;
    
    if (NormalClip(triggerPos1, triggerPos2, actorPosition2D) >= 0 && 
        NormalClip(triggerPos2, triggerPos3, actorPosition2D) >= 0 && 
        NormalClip(triggerPos3, triggerPos4, actorPosition2D) >= 0 && 
        NormalClip(triggerPos4, triggerPos1, actorPosition2D) >= 0
    ) {
        g_FieldScriptVMCurActor->scriptInstructionPointer += 4;
        return;
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer = FieldScriptVMGetInstructionArgument(2);
    g_FieldScriptMaxInstructionCount += 1;
}

void FieldScriptCheckTriggerZone(void) {
    ActorData* pActor;
    int actorPositionY;
    long actorPosition2D;
    long triggerPos1;
    long triggerPos2;
    long triggerPos3;
    long triggerPos4;
    int triggerIndex;
    FieldTriggerZone* pTrigger;

    triggerIndex = SCRIPT_READ_U8_REL(1);
    pActor = (ActorData*)(uintptr_t)g_FieldActors[g_PlayerActorIndex].pActorData;

    actorPositionY = CONV_TO_GTE(pActor->position.vy);
    if (g_pFieldTriggerZones[triggerIndex].y0 < actorPositionY && 
        (actorPositionY - pActor->height) < g_pFieldTriggerZones[triggerIndex].y0
    ) {
        triggerPos1 = (g_pFieldTriggerZones[triggerIndex].z0 << 0x10) + g_pFieldTriggerZones[triggerIndex].x0;
        triggerPos2 = (g_pFieldTriggerZones[triggerIndex].z1 << 0x10) + g_pFieldTriggerZones[triggerIndex].x1;
        triggerPos3 = (g_pFieldTriggerZones[triggerIndex].z2 << 0x10) + g_pFieldTriggerZones[triggerIndex].x2;
        triggerPos4 = (g_pFieldTriggerZones[triggerIndex].z3 << 0x10) + g_pFieldTriggerZones[triggerIndex].x3;
        actorPosition2D = (CONV_TO_GTE(pActor->position.vz) << 0x10) + CONV_TO_GTE(pActor->position.vx);

        if (
            NormalClip(triggerPos1, triggerPos2, actorPosition2D) >= 0 && 
            NormalClip(triggerPos2, triggerPos3, actorPosition2D) >= 0 && 
            NormalClip(triggerPos3, triggerPos4, actorPosition2D) >= 0 && 
            NormalClip(triggerPos4, triggerPos1, actorPosition2D) >= 0
        ) {
            g_FieldScriptVMCurActor->scriptInstructionPointer += 4;
            return;
        }
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer = FieldScriptVMGetInstructionArgument(2);
    g_FieldScriptMaxInstructionCount += 1;
}

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/field/nonmatchings/main/misc11", FieldProjectActorOriginToScreen);
#else
void FieldProjectActorOriginToScreen(int* screenX, int* screenY) {
    MATRIX matrix;
    SVECTOR origin;
    int screenXY;
    long depth;
    long flag;
    u32 actorIndex;

    actorIndex = func_8009CD7C(1);
    CompMatrix(&g_Scene.worldToScreenMatrix,
               &g_FieldActors[actorIndex].childMatrix, &matrix);

    origin.vx = 0;
    origin.vy = 0;
    origin.vz = 0;
    SetRotMatrix(&matrix);
    SetTransMatrix(&matrix);
    RotTransPers(&origin, &screenXY, &depth, &flag);

    *screenY = (s16)(screenXY >> 16);
    *screenX = (s16)screenXY;
}
#endif /* XENO_PC_PORT */

extern s32 D_800ADC18;

void func_80095B3C(void) {
    int screenX;
    int screenY;

    FieldProjectActorOriginToScreen(&screenX, &screenY);
    if (D_800ADC18 != 0) {
        g_FieldScriptVMCurActor->scriptInstructionPointer += 4;
        return;
    }
    
    if ((screenY - 33) < 159U && (screenX - 33) < 255U) {
        g_FieldScriptVMCurActor->scriptInstructionPointer += 4;
    } else {
        g_FieldScriptVMCurActor->scriptInstructionPointer = FieldScriptVMGetInstructionArgument(2);
    }
    
    D_800B00C0 = 1;
}

void FieldScriptCheckActorOnScreen(void) {
    int screenX;
    int screenY;

    FieldProjectActorOriginToScreen(&screenX, &screenY);
    if (D_800ADC18 != 0) {
        g_FieldScriptVMCurActor->scriptInstructionPointer += 4;
        return;
    }
    
    if ((screenY - 1) < (SCREEN_HEIGHT - 1) && (screenX - 1) < (SCREEN_WIDTH - 1)) {
        g_FieldScriptVMCurActor->scriptInstructionPointer += 4;
    } else {
        g_FieldScriptVMCurActor->scriptInstructionPointer = FieldScriptVMGetInstructionArgument(2);
    }
    
    D_800B00C0 = 1;
}

void func_80095CC4(void) {
    s32 actorIdx = FieldScriptVMGetActorIndex(1);
    if (actorIdx != ACTOR_ID_INVALID) {
        u32 actorAddr = (u32)(FieldScriptVMGetActorIndex(1) * sizeof(FieldActor));
        FieldActor* pActor;
        void* pActorData;
        s32 arg2;
        actorAddr += (u32)(uintptr_t)g_FieldActors;
        pActor = (FieldActor*)(uintptr_t)actorAddr;
        pActorData = (void*)(uintptr_t)pActor->pActorData;
        arg2 = FieldScriptVMGetArgument(2);
        if (arg2 == *(s16*)((u8*)pActorData + 0x10)) {
            g_FieldScriptVMCurActor->scriptInstructionPointer += 6;
            return;
        }
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer = FieldScriptVMGetInstructionArgument(4);
}

extern u32 D_800AFB24[];

void func_80095D6C(void) {
    s32 actorIdx = FieldScriptVMGetActorIndex(1);
    if (actorIdx != ACTOR_ID_INVALID) {
        u32 actorAddr = (u32)(FieldScriptVMGetActorIndex(1) * sizeof(FieldActor));
        FieldActor* pActor;
        u8* pActorData;
        s16 idx1;
        s16 idx2;
        u8* tableBase;
        u8 matchVal;
        s32 arg2;
        actorAddr += (u32)(uintptr_t)g_FieldActors;
        pActor = (FieldActor*)(uintptr_t)actorAddr;
        pActorData = (u8*)(uintptr_t)pActor->pActorData;
        idx1 = *(s16*)(pActorData + 0x10);
        idx2 = *(s16*)(pActorData + idx1 * 2 + 0x8);
        tableBase = (u8*)(uintptr_t)D_800AFB24[idx1];
        matchVal = tableBase[idx2 * 14 + 0xC];
        arg2 = FieldScriptVMGetArgument(2);
        if (arg2 == matchVal) {
            g_FieldScriptVMCurActor->scriptInstructionPointer += 6;
            return;
        }
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer = FieldScriptVMGetInstructionArgument(4);
}

extern FieldActor* D_800B06B8;

void FieldScriptCheckActorDistance(void) {
    ActorData* pActor;
    ActorData* pActorOther;
    FieldActor* pFieldActor;
    int distance;
    int actorIndex;

    actorIndex = FieldScriptVMGetActorIndex(1);
    if (actorIndex != 0xFF) {
        pFieldActor = &g_FieldActors[FieldScriptVMGetActorIndex(1)];
        
        pActor = (ActorData*)(uintptr_t)D_800B06B8->pActorData;
        pActorOther = (ActorData*)(uintptr_t)pFieldActor->pActorData;
        distance = FieldGetVec3Magnitude(
            CONV_TO_GTE(pActor->position.vx) - CONV_TO_GTE(pActorOther->position.vx), 
            CONV_TO_GTE(pActor->position.vy) - CONV_TO_GTE(pActorOther->position.vy), 
            CONV_TO_GTE(pActor->position.vz) - CONV_TO_GTE(pActorOther->position.vz)
        );
        
        if (distance < FieldScriptVMGetArgument(2)) {
            g_FieldScriptVMCurActor->scriptInstructionPointer += 6;
            return;
        }
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer = FieldScriptVMGetInstructionArgument(4);
}
