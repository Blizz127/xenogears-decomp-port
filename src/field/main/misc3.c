#include "common.h"
#include "psyq/libgpu.h"
#include "psyq/libgte.h"
#include "field/actor.h"
#include "field/main.h"
#include "field/camera.h"
#include "field/effects.h"
#include "system/memory.h"
#include "system/controller.h"

#ifdef XENO_PC_PORT
#include <stdlib.h>

/* Retail-shaped default: FieldLoad's per-actor model build (double-buffer
 * alloc + GPU command-list build) always runs, as on PSX. Required since the
 * func_800748E8 translation fix (asm mvmva cv=0): correctly translated model
 * prims project and write packets, so the buffers must exist or the prim
 * procs write at NULL. XENO_FIELD_NO_MODEL_BUILD=1 is a diagnostic-only
 * opt-out: it is NOT retail behavior (no model actors are drawn); the
 * mirrored gate in misc2.c func_800748E8 skips the model draw so the
 * empty double-buffer slots are never dereferenced. */
static int PcPortModelBuildEnabled(void) {
    static int s_enabled = -1;

    if (s_enabled < 0) {
        const char* env = getenv("XENO_FIELD_NO_MODEL_BUILD");
        s_enabled = !(env != NULL && env[0] != '\0' && env[0] != '0');
    }
    return s_enabled;
}
#endif

extern VECTOR g_CameraEye;
extern VECTOR g_CameraAt;
extern VECTOR g_CameraUp;
extern void func_80030A30(s32 lightId, void* pLight);
extern void func_80030B14(MATRIX* pMatrix);

static inline void FieldLoadLightRecord(u8** ppLightData, u8* pLight) {
    u8* pData = *ppLightData;

    *(s32*)(pLight + 0x0) = *(s16*)(pData + 0x0);
    pData += 2;
    *(s32*)(pLight + 0x4) = *(s16*)(pData + 0x0);
    pData += 2;
    *(s32*)(pLight + 0x8) = *(s16*)(pData + 0x0);
    pData += 4;
    *(s16*)(pLight + 0xC) = *(u16*)(pData + 0x0) << 3;
    pData += 2;
    *(s16*)(pLight + 0xE) = *(u16*)(pData + 0x0) << 3;
    pData += 2;
    *(s16*)(pLight + 0x10) = *(u16*)(pData + 0x0) << 3;
    pData += 4;

    *ppLightData = pData;
}

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/field/nonmatchings/main/misc3", func_8006FDEC);
#else
void func_8006FDEC(void* pLightData) {
    u8* pData = pLightData;
    u8* pScene = (u8*)&g_Scene;
    u8* pLight0 = pScene + 0x138;
    u8* pLight1 = pScene + 0x14C;
    u8* pLight2 = pScene + 0x160;
    s32 flag;

    FieldMatrixLookAt(&g_Scene.viewMatrix, &g_CameraEye, &g_CameraAt, &g_CameraUp);
    RotMatrix(&g_Scene.worldRotation, &g_Scene.worldToScreenMatrix);
    MulMatrix2(&g_Scene.viewMatrix, &g_Scene.worldToScreenMatrix);

    FieldLoadLightRecord(&pData, pLight0);
    func_80030A30(0, pLight0);

    FieldLoadLightRecord(&pData, pLight1);
    func_80030A30(1, pLight1);

    FieldLoadLightRecord(&pData, pLight2);
    /* Retail copies the two 0x14-byte light records with aligned word loops
     * (four loads/stores plus a tail), not with memcpy. */
    {
        typedef struct { u32 w[5]; } LightRecord20;

        *(LightRecord20*)pLight1 = *(LightRecord20*)pLight0;
        *(LightRecord20*)pLight2 = *(LightRecord20*)pLight0;
    }
    func_80030A30(2, pLight2);

    *(s16*)(pScene + 0x174) = *(u16*)(pData + 0x0) << 4;
    *(s16*)(pScene + 0x176) = *(u16*)(pData + 0x2) << 4;
    *(s16*)(pScene + 0x178) = *(u16*)(pData + 0x4) << 4;

    SetRotMatrix(&g_Scene.viewMatrix);
    SetTransMatrix(&g_Scene.viewMatrix);
    RotTrans(&g_Scene.worldTranslation, (VECTOR*)(pScene + 0xE8), &flag);
    func_80030B14(&g_Scene.worldToScreenMatrix);
    SetRotMatrix(&g_Scene.worldToScreenMatrix);
    SetTransMatrix(&g_Scene.worldToScreenMatrix);
}
#endif /* XENO_PC_PORT */

void FieldLZSSDecompress(void* _unused, void* pCompressed, void* pDecompressed) {
    LZSSDecompress(pCompressed, pDecompressed);
}

extern void WorkListsFreeAllEntries(void);
extern void func_80025044(void);
extern void func_800250E0(int context);
extern void GfxFreeWorkBuffers(void);
extern void func_8008083C(int actorIndex);
extern void func_8002CBBC(u8* modelData);
extern void func_800306D0(u8* pAnimInfo);
extern void FieldDistortionFree(void);
extern void func_80027D40(void*);
extern void GfxLineScrollFree(void* pLineScroll);
extern void func_801E7FD4(void);
extern void FieldRenderSyncAndFlush(void);
extern void func_800A83B4(void);
extern s16 D_800B00B2;
extern s32 D_800B007C;
extern s16 D_800AFEA8;
extern s32 D_800B2264;
extern void* D_800ADB20;
extern void* D_800ADBF0;
extern void* D_800AFB18;
extern void* D_800AFB14;

extern void* g_Heap;

// XENO_PC_PORT: defensive backstop for the model-data inner-field frees
// below. Root-caused: an earlier version of this loop read pModel's 4-byte
// PSX pointer slots (+0x8/+0x14) via a native 8-byte `*(void**)` read, which
// on a 64-bit host pulls in the *next* 4-byte slot's bytes as garbage upper
// bits (pModel's slots must be read as `(void*)(uintptr_t)*(u32*)(...)`,
// exactly as FieldLoad's own actor-init loop and misc8.c:303 already do --
// see the inline comment at the func_8002CB54 call site above). That bug is
// now fixed at the source, but this walk of the heap's own free-list is kept
// as a cheap, always-safe backstop against any other stale/corrupt pointer,
// since retail's own asm has no equivalent check at all.
//
// Both call sites below are already inside `#ifdef XENO_PC_PORT`, so the
// definition is guarded too: without this guard the matching build still
// emitted the (uncalled) body, leaving misc3 112 bytes LONGER than retail --
// the only place in the field overlay where the build exceeds retail.
#ifdef XENO_PC_PORT
static int IsLiveHeapBlock(void* ptr) {
    HeapBlock* pCur;

    if (ptr == NULL) {
        return 0;
    }

    pCur = (HeapBlock*)g_Heap - 1;
    while (pCur->userTag != HEAP_USER_END) {
        if (pCur->userTag != HEAP_USER_NONE && (void*)(pCur + 1) == ptr) {
            return 1;
        }
        pCur = (HeapBlock*)(uintptr_t)pCur->pNext - 1;
    }
    return 0;
}
#endif

// FieldMain teardown counterpart to FieldLoad: frees every heap block the
// outgoing field allocated (per-actor model/sprite/shadow data, trigger
// zones, script file, model/sprite section buffers, line-scrolls, font,
// party-skin overlay) so func_800A5C40's persistent-block re-allocation and
// FieldLoad's fresh allocations have room. Actors with status&0x40 keep
// their model data (shared/static model, not owned by this actor slot).
#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/field/nonmatchings/main/misc3", FieldFree);
#else
void FieldFree(void) {
    s32 i;
    s32 renderCtxStep;

    ResetGraph(1);
    WorkListsFreeAllEntries();
    renderCtxStep = 0;
    do {
        func_80025044();
        DrawSync(0);
        renderCtxStep++;
        func_800250E0((g_FieldCurRenderContextIndex + renderCtxStep) & 1);
        func_80025044();
        DrawSync(0);
        GfxFreeWorkBuffers();
    } while (renderCtxStep < 2);

    for (i = 0; i < g_FieldNumActors; i++) {
        FieldActor* pActor = &g_FieldActors[i];

        func_8008083C(i);
        if (!(pActor->status & 0x40) && pActor->pModelData != 0) {
            u8* pModelData = (u8*)(uintptr_t)pActor->pModelData;
            /* PSX 4-byte pointer slots -- read via u32 + truncating cast,
             * never as a native 8-byte pointer (see IsLiveHeapBlock above). */
            u8* modelData = (u8*)(uintptr_t)*(u32*)(pModelData + 0x4);
            void* pDoubleBuffer = (void*)(uintptr_t)*(u32*)(pModelData + 0x8);
            void* pAnimInfo = (void*)(uintptr_t)*(u32*)(pModelData + 0x14);

            /* status&0x2000 gates whether +0x14 (pAnimInfo) was ever written
             * (func_80080A74/func_80080F44, misc8.c) -- matches retail.
             * IsLiveHeapBlock is a port-only host-pointer backstop (retail's
             * asm has no equivalent check), so it is excluded from the
             * matching build. */
#ifdef XENO_PC_PORT
            if ((pActor->status & 0x2000) && IsLiveHeapBlock(pAnimInfo)) {
#else
            if (pActor->status & 0x2000) {
#endif
                func_800306D0(pAnimInfo);
            }
            func_8002CBBC(modelData);
#ifdef XENO_PC_PORT
            if (IsLiveHeapBlock(pDoubleBuffer)) {
#else
            {
#endif
                HeapFree(pDoubleBuffer);
            }
            HeapFree((void*)(uintptr_t)pActor->pModelData);
        }
    }

    FieldDistortionFree();
    HeapFree(g_FieldActors);
    HeapFree(g_pFieldTriggerZones);
    HeapFree(D_800ADBF0);
    HeapFree(g_FieldCurScriptFile);
    HeapFree(D_800AFB18);
    HeapFree(D_800AFB14);
    HeapFree(g_FieldSpriteData);

    if (D_800B00B2 != 0) {
        func_80027D40((void*)(uintptr_t)D_800B007C);
    }

    /* Same D_800AFEA8 header layout as func_800920D8 (misc11.c): s16 count
     * at +0, then two parallel u32[] arrays (line-scroll ptr, its heap
     * buffer) at +4 and +0x84. */
    if (D_800AFEA8 > 0) {
        u32* pLineScrolls = (u32*)((u8*)&D_800AFEA8 + 4);
        u32* pLineScrollBuffers = (u32*)((u8*)&D_800AFEA8 + 0x84);

        for (i = 0; i < D_800AFEA8; i++) {
            GfxLineScrollFree((void*)(uintptr_t)pLineScrolls[i]);
            HeapFree((void*)(uintptr_t)pLineScrollBuffers[i]);
            HeapFree((void*)(uintptr_t)pLineScrolls[i]);
        }
    }
    FontFree();
    D_800AFEA8 = 0;

    if (D_800B2264 != 0) {
        func_801E7FD4();
        HeapFree(D_800ADB20);
        FieldRenderSyncAndFlush();
    }
    D_800B2264 = 0;

    HeapFreeBlocksWithFlag(3);
    func_800A83B4();
}
#endif /* XENO_PC_PORT */

void FieldLoadTIMWithClut(u_long *pTimData, short x, short y, short clutX, short clutY, short clutWidth, short clutHeight) {
    TIM_IMAGE* pTIM;
    TIM_IMAGE tim;   

    OpenTIM(pTimData);
    pTIM = ReadTIM(&tim);

    if (pTIM) {
        if (tim.caddr) {
            if (clutY != -1) {
                tim.crect->x = clutX;
                tim.crect->y = clutY;
            }

            if (clutWidth)
                tim.crect->w = clutWidth;
            
            if (clutHeight)
                tim.crect->h = clutHeight;

            LoadImage(tim.crect, tim.caddr);
        }

        if (tim.paddr) {
            tim.prect->x = x;
        }
        tim.prect->y = y;
        LoadImage(tim.prect, tim.paddr);
    }
}

extern s32 D_800ADB60;
extern void* D_800ADC14;
extern s32 g_GameSceneMapNum;
extern int* ArchiveAllocStreamFile(int numEntries, int allocMode);
extern int func_80029EB0(s32 archiveIndex, void* pStreamFile, s32 arg2, s32 arg3, s32 arg4, s32 arg5, s32 arg6, s32 arg7, s32 arg8, s32 arg9);

void func_80070488(void) {
    if (D_800ADB60 == 0) {
        D_800ADB60 = 1;
        D_800ADC14 = ArchiveAllocStreamFile(4, 1);
        func_80029EB0(((g_GameSceneMapNum & 0xFFF) << 1) + 0xB9,
                      D_800ADC14,
                      0,
                      0,
                      0,
                      0,
                      0,
                      0,
                      0,
                      0);
    }
}

void func_80070508(void) {
    if (D_800ADB60 == 1) {
        ArchiveCdDataSync(0);
        DrawSync(0);
        HeapFree(D_800ADC14);
        D_800ADB60 = 0;
    }
    func_80078C5C();
}

void func_80070560(s32* dest, s16* src) {
    dest[0] = src[0] << 16;
    dest[1] = src[1] << 16;
    dest[2] = src[2] << 16;
}

void func_80070594(MATRIX* dest) {
    SVECTOR rotation;

    rotation.vx = 0;
    rotation.vy = 0;
    rotation.vz = 0;
    RotMatrix(&rotation, dest);
    dest->t[2] = 0;
    dest->t[1] = 0;
    dest->t[0] = 0;
}

extern void func_8028125C(void);
extern void func_8007254C(void);
extern void func_800864B4(void);
extern void FieldInitializeParticles(void);
extern void func_800ABD18(void);

extern MATRIX D_800B00E8;
extern s16 D_800ADB00, D_800AFE9C, D_800AFEA0, D_800C2694;
extern s16 D_800C38F8, D_800C3900, D_800C3908;
extern u8 D_800B2356;
extern s16 D_800B2346, D_800B234A, D_800B234C, D_800C3A38;
extern s32 D_800C3A60, D_800C3A5C;
extern s32 D_800B21BC, D_800B21C0, D_800B21C4, D_800B2264, D_800B21B8;
extern s32 D_800B2268;
extern u8 D_800B21D1, D_800B21D0, D_800B21CE;
extern s16 D_800B21B0, D_800B21B2, D_800B21AE;
extern u8 D_800B21CD, D_800B219F, D_800B219E, D_800B219D, D_800B219C;
extern u8 D_800B21CC, D_800B21CF, D_800B21D2;
extern s32 D_800ADB74, D_800ADB90, D_800ADB24, D_800ADB98, D_800ADB94;
extern s32 D_800ADB70, D_800ADB2C, D_800ADB68, D_800ADB88, D_800ADBA8;
extern s32 D_800B0064, D_800ADB18, D_800ADB50, D_800AFE84, D_800AFD04;
extern u8 D_800B02C8;
extern s32 D_800ADBD4, D_800ADBD0;
extern s16 D_800B22E0, D_800B233C, D_800B236C;
extern s32 D_800B14A4, D_800ADB38, D_800ADB3C, D_800AFD14, D_800B0048;
extern s16 D_800B21AC, D_800ADC08, D_800AF93A;
extern s16 D_800B21D4, D_800B21A4, D_800B21A2, D_800B21A0;
extern s16 D_800B21AA, D_800B21A8, D_800B21A6, D_800B21B4, D_800B218C;
extern s16 D_800B2184, D_800B2186, D_800B2188;
extern s32 D_800ADBC4, D_800ADB6C, D_800ADBEC, D_800B21D8;
extern s16 D_800B2290;
extern s16 D_800B2270[];
extern s16 D_800B22A0[];
extern s16 D_800B22E2[];
extern s32 D_8005A444[], D_8006F990[];
extern s32 D_80050100, D_800B229C, D_800B2298;
extern u8 D_800B2192, D_800B2191, D_800B2190;
extern u8 D_800B2196, D_800B2195, D_800B2194;
extern s16 D_800B2198, D_800B219A, D_800B21D6;
extern s16 D_800AFEA8, D_800B2174;
extern s32 D_800B2180, D_800B217C;
extern u16 D_800B218E;
extern s16 D_800ADB02, D_800B2344, D_800B2342, D_800B2348, D_800B234E;
extern u16 D_800B233E;
extern u8 D_800B2355, D_800ADB04, D_800ADB05, D_800B2357, D_800B2354;
extern s32 D_800ADBB4, D_800B2350, D_800ADB84, D_800ADB7C, D_800ADB8C, D_800ADB64;
extern s16 D_800ADB54;
extern u8 D_800B2358;
extern s32 D_800ADC18, D_800ADC0C, g_FieldParticleCurActor;
extern s32 g_PlayerActorIndex;
extern s32 g_GamePartySkinsInitialized;
extern void* g_pGameState;
extern MATRIX D_800AF85C;

#ifdef XENO_PC_PORT
/* The direct-field harness has no retail exit-transition frame to publish
 * the first field's music request.  That is a process-start condition, not
 * a per-field condition: once FieldMain has been entered, subsequent natural
 * field transitions own D_8004F324 and must be left to the retail scripts.
 * Keep this state private to the port process; there is deliberately no map
 * table or synthetic music selection here. */
int PcPort_FieldTestMusicBootstrapAllowed(void) {
    static int s_bootstrapped;
    const char* ft;

    ft = getenv("XENO_FIELD_TEST");
    if (s_bootstrapped || ft == NULL || ft[0] != '1') {
        return 0;
    }
    s_bootstrapped = 1;
    return 1;
}
#endif

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/field/nonmatchings/main/misc3", func_800705DC);
#else
void func_800705DC(void) {
    SVECTOR rotation;
    s32 i;
    u16* scriptDst;
    u16* scriptClear;
    u16* gameStateSrc;

    if (g_FieldSystemMode == SYSTEM_MODE_PC_HDD) {
        func_8028125C();
    }

    FieldSetClipDimensions(0, 0, 0x140, 0xE0);
    ControllerResetState();

    D_800ADB00 = -1;
    D_800AFE9C = 0;
    D_800AFEA0 = 0;
    D_800C2694 = 0;
    D_800C38F8 = 0;
    D_800C3900 = 0;
    D_800C3908 = 0;
    D_800B2356 = 5;
    D_800B2346 = 3;
    D_800B234A = 0x40;
    D_800B234C = 0xFF;
    D_800C3A38 = 0xFF;
    D_800C3A60 = 0;
    D_800C3A5C = 0;
    D_800B21BC = 0;
    D_800B21C0 = 0;
    D_800B21C4 = 0;
    D_800B2264 = 0;
    D_800B21B8 = 0;
    D_800B21D1 = 0;
    D_800B21D0 = 0;

    g_FieldEffects.distortion.v6 = 0;
    g_FieldEffects.distortion.v5 = 0;
    g_FieldEffects.distortion.v4 = 0;
    g_FieldEffects.distortion.v3 = 0;
    g_FieldEffects.distortion.v2 = 0;
    g_FieldEffects.distortion.v1 = 0;
    g_FieldEffects.distortion.unk3C = 0;
    g_FieldEffects.distortion.unk38 = 0;
    D_800B2268 = 0;
    D_800B21CE = 0;
    D_800B21B0 = 0;
    D_800B21B2 = 0;
    D_800B21AE = 0;
    g_FieldEffects.distortion.isActive = 0;
    D_800B21CD = 0;
    D_800B219F = 0;
    D_800B219E = 0;
    D_800B219D = 0;
    D_800B219C = 0;
    D_800B21CC = 0;
    D_800B21CF = 0;
    D_800B21D2 = 0;
    D_800ADB74 = 0;
    g_FieldRenderContextUseOT2 = 0;
    D_800ADB90 = 0;
    D_800ADB24 = 0;
    D_800ADB98 = 0;
    D_800ADB94 = 0;
    D_800ADB70 = 0;
    D_800ADB2C = 0;
    D_800ADB68 = 0;
    D_800ADB88 = 0;
    D_800ADBA8 = 0;
    D_800B0064 = 0;
    D_800ADB18 = 0;
    D_800ADB50 = 0;
    D_800AFE84 = 0;
    D_800AFD04 = 0;
    D_800B02C8 = 0;
    D_800ADBD4 = 0;
    D_800ADBD0 = 0;
    D_800B22E0 = 0;
    D_800B233C = 0;
    D_800B14A4 = 0;
    D_800B236C = 0;
    D_800ADB38 = 0;
    D_800ADB3C = 0;
    D_800AFD14 = 0x20;
    D_800B0048 = 2;
    D_800B21AC = 0x3FF;
    D_800ADC18 = 4;
    g_FieldParticleCurActor = 0;
    D_800ADB02 = 0;
    D_800B233E = 0;
    D_800B2344 = 0;
    D_800B2342 = 0;
    D_800B2348 = 0;
    D_800B234E = 0;
    D_800B2355 = 0;
    D_800ADB04 = 0;
    D_800ADB05 = 0;
    D_800B2357 = 0;
    D_800B2354 = 0;
    D_800ADBB4 = 0;
    D_800B2350 = 0;
    D_800ADB54 = 0;
    D_800B2358 = 0;
    D_800ADB84 = 0;
    D_800ADB7C = 0;
    D_800ADB8C = 0;
    D_800ADB64 = 0xFF;

    rotation.vx = 0;
    rotation.vy = 0;
    rotation.vz = 0;
    RotMatrix(&rotation, &D_800B00E8);

    for (i = 0; i < 3; i++) {
        D_800B22E2[i] = -1;
    }

    D_800ADC08 = 1;
    D_800AF93A = 0x1000;
    D_800B21D4 = 0x720;
    D_800B21A4 = 0x100;
    D_800B21A2 = 0x100;
    D_800B21A0 = 0x100;
    D_800B21AA = 0x200;
    D_800B21A8 = 0x200;
    D_800B21A6 = 0x200;
    D_800B21B4 = 0x80;
    D_800ADBC4 = 0xFF;
    D_800B218C = 0x1000;
    D_800B2184 = 0;
    D_800B2186 = 0;
    D_800B2188 = 0;
    D_800ADB6C = -1;

    for (i = 0; i < 16; i++) {
        D_800B2270[i] = 0x1D;
    }

    D_800B2290 = 0x1D;
    /* Field-test harness: direct map entry skips the exit-transition flow
     * (FieldMain's D_800ADBD0 block) that requests the next map's music, so
     * no music request ever lands. Stand in for it with that block's own
     * body (main.c: func_8001B66C -> D_8004F308 = -1 -> func_80085B20),
     * using the field's just-initialized default music id -- the same
     * stand-in role the XENO_FIELD_ENTRANCE spawn write plays for the
     * missing transition. Everything downstream (the per-frame
     * func_80085C90 poller, bank+song load, manager create/start) is the
     * real retail flow. */
#ifdef XENO_PC_PORT
    {
        if (PcPort_FieldTestMusicBootstrapAllowed()) {
            extern s32 D_8004F308, D_8004F324, D_8004F364;
            extern void func_8001B66C(void);
            extern void func_80085B20(s32 musicIdx, s32 arg1);
            /* Retail boots with D_8004F364=1 ("field common WDS bank
             * resident") because the new-game flow loads that bank before any
             * field entry. The port now performs that boot closure itself:
             * port_main's PcPort_LoadRetailBootSoundBanks() replays retail
             * func_80019578's archive 0/1 files 2..5, and one of them IS this
             * common bank (id 0x26, 0x25CF0 bytes, pinned SPU 0x12000) -- the
             * same load also sets D_80059560, which FieldMain adopts into
             * D_8006251C (main.c). Re-loading it here was therefore a
             * DUPLICATE: SoundLoadWdsFile -> SoundSpuMemoryAllocateBlockAtAddress
             * (0x25CF0, 0x12000) returned 0 (gap == 0, region already
             * resident) and fired SoundHandleError(0x1F), which aborted the
             * port in PsyX_SPUAL_Write. Assert retail's invariant instead of
             * re-loading -- the per-frame C90 poller then skips the
             * common-bank leg exactly as retail does. */
            D_8004F364 = 1;
            func_8001B66C();
            D_8004F308 = -1;
            D_8004F324 = D_800B2290;
            func_80085B20(D_800B2290, 1);
        }
    }
#endif
    D_800ADBEC = -1;
    D_800B21D8 = 2;
    g_FieldControl.controllerBtnMask = 0xFFFF;
    D_800B2192 = 0x80;
    D_800B2191 = 0x80;
    D_800B2190 = 0x80;
    D_800B2196 = 0xFF;
    D_800B2195 = 0xFF;
    D_800B2194 = 0xFF;
    D_800B2198 = 0x15E0;
    D_800B219A = 0x300C;
    g_FieldCurRenderContextIndex = 0;
    D_800ADC0C = 0;
    D_800AFEA8 = 0;
    g_PlayerActorIndex = 0;
    g_FieldControl.unkAngle = 0;
    D_800B2174 = 0;
    g_FieldControl.isRandomEncountersEnabled = 0;
    D_800B2180 = 0;
    D_800B217C = 0;
    D_800B218E = 0;
    D_800B21D6 = 8;

    if (!g_GamePartySkinsInitialized) {
        for (i = 0; i < 3; i++) {
            D_8005A444[i] = 0xFF;
            D_8006F990[i] = 0xFF;
        }
    }

    for (i = 0; i < 32; i++) {
        D_800B22A0[i] = -1;
    }

    D_800B229C = 0;
    D_800B2298 = 0;
    D_80050100 = 2;

    scriptDst = (u16*)&g_FieldScriptMemory;
    scriptClear = (u16*)((u8*)&g_FieldScriptMemory + 0x400);
    gameStateSrc = (u16*)((u8*)g_pGameState + 0x1930);
    for (i = 0; i < 0x200; i++) {
        scriptDst[i] = gameStateSrc[i];
        scriptClear[i] = 0;
    }

    SetGeomScreen(0x200);
    func_80070594(&g_Scene.viewMatrix);
    func_80070594(&D_800AF85C);
    func_80070594(&g_Scene.worldToScreenMatrix);
    func_80070594(&g_Scene.worldRotationMatrix);

    g_Scene.worldRotation.vx = 0;
    g_Scene.worldRotation.vy = 0;
    g_Scene.worldRotation.vz = 0;
    g_Scene.worldTranslation.vx = 0;
    g_Scene.worldTranslation.vy = 0;
    g_Scene.worldTranslation.vz = 0;
    g_WorldScale = 0x3000;
    RotMatrix(&g_Scene.worldRotation, &g_Scene.worldToScreenMatrix);

    g_FieldCurRenderContext = g_FieldRenderContexts;
    func_8007254C();
    func_80070C84();
    func_800864B4();
    FieldInitializeParticles();
    func_800ABD18();
}
#endif /* XENO_PC_PORT */

extern u16 D_800B06A4[];
extern u16 D_800B06A6[];
extern s32 D_800ADB0C;

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/field/nonmatchings/main/misc3", func_80070C84);
#else
void func_80070C84(void) {
    s32 i;
    for (i = 0; i < 3; i++) {
        D_800B06A4[i * 3] = 0xFF;
        D_800B06A6[i * 3] = 0xFF;
    }
    D_800ADB0C = 0;
}
#endif /* XENO_PC_PORT */

/* ---- FieldLoad: parse the raw map file and build the field runtime state -----
 * Port-first functional decompile (control flow mirrors
 * asm/field/nonmatchings/main/misc3/FieldLoad.s 1:1). The map file has already
 * been streamed into the buffer pointed to by D_8005A4E0 (an ActorFile header
 * followed by LZSS-compressed sections). This routine:
 *   - copies the 0x100-byte TIM/CLUT header table out of the map,
 *   - HeapAllocs + FieldLZSSDecompresses each section (TIMs, cluts, model data,
 *     sprite data, scripts, dialogs, triggers, walkmesh),
 *   - sets the globals g_FieldActors / g_FieldNumActors / g_FieldSpriteData /
 *     g_pFieldTriggerZones / g_FieldCurScriptFile,
 *   - runs the per-actor (0x5C-byte) init loop.
 * Section offsets/sizes are read from the map header by raw byte offset to stay
 * faithful to the asm (see the ActorFile struct in field/actor.h for the map).
 * Every jal is preserved in order; the native dependency audit identifies any
 * callee still supplied by generated storage. */

extern void* D_8005A4E0;

/* Data buffers / scratch globals touched by FieldLoad. Native ownership and
 * exact sizes remain tracked by the generated-data audit. */
extern u8 D_800B1F78[];     /* 0x100-byte header table copied out of the map */
extern u8 D_800B06BC[];     /* 4x4 grid of Quads (0x70 each) */
extern u8 D_800B0DBC[];     /* 5 Quads (0x70 each) */
extern void* D_800AFB14;    /* decompressed model-data buffer (0x114 section) */
extern void* D_800AFB18;    /* decompressed sprite/model-2 buffer (0x134 section) */
/* One contiguous scratch block in the retail binary: D_800AFB20 is the base,
 * D_800AFB24 == D_800AFB20[1], and D_800AFB54 == ((s16*)D_800AFB20)[0x1A].
 * Declared as a single array so the native storage stays contiguous. */
extern u32 D_800AFB20[];    /* fixup base pointer table (>= 0x38 bytes) */
#define D_800AFB24 (D_800AFB20[1])
#define D_800AFB54 (((s16*)D_800AFB20)[0x1A])
extern s32 D_800AFD10;
extern s32 D_800ADBFC;      /* == g_FieldNumActors snapshot (script actor count) */
extern s32 D_800AFC74;
extern void* D_800ADBF0;    /* decompressed dialogs buffer (0x128 section) */
extern u8 D_800658DC[];     /* field battle encounter records and weights */
extern s32 D_8004F330;
extern s32 D_8004F334;

/* Geometry / camera work-area symbols used by the render-setup tail. Each is a
 * distinct absolute data symbol in the asm (auto-stubbed in the port). */
extern u8  D_800B223C[];    /* ZoomFadeEffect-ish work area (see main.h) */
extern s16 D_800B0080, D_800B0082, D_800B0084, D_800B0086, D_800B0088;
extern s16 D_800B008A, D_800B008C, D_800B008E;
extern s32 D_800B0090, D_800B0094, D_800B0098;
extern s8  D_800B00A0, D_800B00A1, D_800B00A2, D_800B00A4, D_800B00A5;
extern s8  D_800B00A6, D_800B00A8, D_800B00A9, D_800B00AA;
extern s16 D_800B00AC, D_800B00AE, D_800B00B0, D_800B00B2;
extern s8  D_800B225C, D_800B225D, D_800B225E;
extern u8  D_800AFC30[];
extern u32 D_800AFC44, D_800AFC48, D_800AFC4C;
extern s32 D_800ADB1C;
extern s32 D_800B007C;
extern u16 D_800B233E;
extern VECTOR g_CameraAt2;
extern u32 D_8006FAF4;      /* stack-init source (dead copy at entry) */
extern void func_8006FDEC(void* pLightData);

/* Externs for callees that don't yet have a C signature. */
extern void func_800705DC(void);
extern void func_8007A7F4(Quad* pPart, int x, int y, int tex);
extern void func_8007A5C4(void);
extern void func_80077844(short* dst, int a, int b, int c,
                         int d, int e, int f, int g, int h, int i);
extern void func_80077C60(void);
extern void func_8007469C(void);
extern void func_80080F44(s32 actorIndex);
extern void func_802812A4(void);
extern void func_800A28D4(void);
extern void func_800A2714(void);
extern void func_80073E38(void);
extern void func_80077268(void);
extern void func_800303C8(void* modelData, int a1);
extern void func_8002CB54(void* modelData, u32* out1, u32* out2);
extern void func_8002C8CC(void* a0, void* a1, int a2);
extern void func_8002C644(void* a0);
extern int  func_8002C3E8(void* a0);
extern void* func_8002709C(s32 a0, s32 a1, s32 a2, s32 a3,
                           s32 clutX, s32 clutY, s32 abr, s32 scrollSign,
                           s16* pCoords, u8* pColors,
                           s32 skyScale, s32 fadeDiv, s32 fadeSub);
extern void func_800223B0(void* a0, s16 a1);
extern void FieldTextBoxInitialize(void);
extern void FieldLoadTIM(u_long* pTimData);
extern void GfxLoadClutsAccelerated(void* pClutData, s32 x, s32 y);
extern void GfxAllocateWorkBuffers(int workBufferSize, unsigned int allocFlag);

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/field/nonmatchings/main/misc3", FieldLoad);
#else
void FieldLoad(void) {
    u8* pMap;
    u32* pTimTable;   /* decompressed TIM-package section */
    u32* pClutTable;  /* decompressed CLUT section */
    s32 numEntries;
    s32 i, j;
    s32 numActors;
    s32 timEntryCount = 0;
    s32 clutEntryCount = 0;

    /* Entry: copies a 4-word blob out of D_8006FAF4 to the stack (dead; the
     * slots are never read again) then does field-camera / controller setup. */
    (void)D_8006FAF4;
    func_800705DC();
#ifdef XENO_PC_PORT
    printf("[field-diag] FieldLoad begin field=%d mapBuf=%p\n", (int)g_GameSceneMapNum, D_8005A4E0);
#endif

    /* Copy the 0x100-byte TIM/CLUT header table out of the map header. The asm
     * chooses an aligned (lw/sw) or unaligned (lwl/lwr) copy loop; both move the
     * same 0x100 bytes. */
    pMap = (u8*)D_8005A4E0;
    memcpy(D_800B1F78, pMap, 0x100);

    /* Build the 4x4 grid of compass/background quads, then 5 extra quads. */
    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            s32 idx = i * 4 + j;
            func_8007A7F4((Quad*)(D_800B06BC + idx * 0x70), j, i, 0);
        }
    }
    func_8007A7F4((Quad*)(D_800B0DBC + 0x00), 4, 4, 1);
    func_8007A7F4((Quad*)(D_800B0DBC + 0x70), 5, 5, 1);
    func_8007A7F4((Quad*)(D_800B0DBC + 0xE0), 6, 6, 1);
    func_8007A7F4((Quad*)(D_800B0DBC + 0x150), 7, 7, 1);
    func_8007A7F4((Quad*)(D_800B0DBC + 0x1C0), 8, 8, 1);
    func_8007A5C4();

    /* --- TIM-package section (size 0x10C, offset 0x130): load each TIM ------- */
    pMap = (u8*)D_8005A4E0;
    pTimTable = (u32*)HeapAlloc(*(u32*)(pMap + 0x10C) + 0x10, 1);
    FieldLZSSDecompress(NULL,
                        (u8*)D_8005A4E0 + *(u32*)((u8*)D_8005A4E0 + 0x130),
                        pTimTable);
    numEntries = (s32)pTimTable[0];
    timEntryCount = numEntries;
    for (i = 0; i < numEntries; i++) {
        FieldLoadTIM((u_long*)((u8*)pTimTable + pTimTable[1 + i]));
    }

    /* --- CLUT section (size 0x11C, offset 0x140): accelerated CLUT upload ---- */
    pMap = (u8*)D_8005A4E0;
    pClutTable = (u32*)HeapAlloc(*(u32*)(pMap + 0x11C) + 0x10, 0);
    FieldLZSSDecompress(NULL,
                        (u8*)D_8005A4E0 + *(u32*)((u8*)D_8005A4E0 + 0x140),
                        pClutTable);
    numEntries = (s32)pClutTable[0];
    clutEntryCount = numEntries;
    numEntries <<= 3; /* iterate the header table in 8-byte strides */
    for (i = 0; i < numEntries; i += 8) {
        /* D_800B1F78 header holds, per entry: [+0]=u16 idx, [+2]=u16, [+4]=s16 flag */
        s16 flag = *(s16*)(D_800B1F78 + i + 6);
        if (flag == 0) {
            /* clut table entries start after the count word */
            GfxLoadClutsAccelerated((u8*)pClutTable + pClutTable[1 + (i >> 3)],
                                    *(u16*)(D_800B1F78 + i + 0),
                                    *(u16*)(D_800B1F78 + i + 2));
        }
    }
    DrawSync(0);
    HeapFree(pTimTable);
    HeapFree(pClutTable);

    /* --- Model-data section (size 0x114, offset 0x138) ---------------------- */
    D_800AFB14 = HeapAlloc(*(u32*)((u8*)D_8005A4E0 + 0x114) + 0x10, 0);
    FieldLZSSDecompress(NULL,
                        (u8*)D_8005A4E0 + *(u32*)((u8*)D_8005A4E0 + 0x138),
                        D_800AFB14);
    {
        /* buffer layout: [0]=count, [1..]=per-entry byte offsets into the buffer */
        u32* pOff = (u32*)D_800AFB14 + 1;
        numEntries = (s32)*(u32*)D_800AFB14;
        for (i = 0; i < numEntries; i++) {
            func_8002C3E8((u8*)D_800AFB14 + *pOff);
            pOff++;
            numEntries = (s32)*(u32*)D_800AFB14; /* asm reloads count each iter */
        }
    }

    /* Retail 80071054..6C decompresses the battle encounter section at the
     * destination base. The +0x10 is on the unused size argument, not a2. */
    FieldLZSSDecompress(NULL,
                        (u8*)((u8*)D_8005A4E0 + *(u32*)((u8*)D_8005A4E0 + 0x148)),
                        D_800658DC);

    /* --- Scripts section (size 0x120, offset 0x144) ------------------------- */
    g_FieldCurScriptFile =
        (ScriptsFile*)HeapAlloc(*(u32*)((u8*)D_8005A4E0 + 0x120) + 0x10, 0);
    FieldLZSSDecompress(NULL,
                        (u8*)D_8005A4E0 + *(u32*)((u8*)D_8005A4E0 + 0x144),
                        g_FieldCurScriptFile);
    D_800ADBFC = (s32)g_FieldCurScriptFile->numScripts;
    g_FieldScriptVMCurScriptData =
        (u8*)g_FieldCurScriptFile + 0x84 + (g_FieldCurScriptFile->numScripts << 6);
#ifdef XENO_PC_PORT
    printf("[field-diag] assets before VM: tim=%d clut=%d scripts=%d scriptData=%p\n",
           (int)timEntryCount, (int)clutEntryCount, (int)D_800ADBFC, g_FieldScriptVMCurScriptData);
    {
        u8* spawn = (u8*)g_FieldScriptVMCurScriptData;
        int e;

        printf("[field-diag] spawn marker=0x%02x modelsInPackage=%d\n",
               spawn[0], (int)*(u32*)D_800AFB14);
        if (spawn[0] == 0xFF) {
            for (e = 0; e < 12; e++) {
                int off = e * 7;
                short sx = (short)(spawn[off + 1] | (spawn[off + 2] << 8));
                short sz = (short)(spawn[off + 3] | (spawn[off + 4] << 8));
                printf("[field-diag] spawn[%d] x=%d z=%d wm=%u rot=%u face=%u\n",
                       e, (int)sx, (int)sz, (unsigned)spawn[off + 5],
                       (unsigned)spawn[off + 6], (unsigned)spawn[off + 7]);
            }
        }
    }
#endif

    /* --- Triggers section (size 0x12C, offset 0x150) ------------------------ */
    g_pFieldTriggerZones =
        (FieldTriggerZone*)HeapAlloc(*(u32*)((u8*)D_8005A4E0 + 0x12C) + 0x10, 0);
    FieldLZSSDecompress(NULL,
                        (u8*)D_8005A4E0 + *(u32*)((u8*)D_8005A4E0 + 0x150),
                        g_pFieldTriggerZones);

    /* --- Dialogs section (size 0x128, offset 0x14C) ------------------------- */
    D_800ADBF0 = HeapAlloc(*(u32*)((u8*)D_8005A4E0 + 0x128) + 0x10, 0);
    FieldLZSSDecompress(NULL,
                        (u8*)D_8005A4E0 + *(u32*)((u8*)D_8005A4E0 + 0x14C),
                        D_800ADBF0);

    /* --- Second model/sprite section (size 0x110, offset 0x134) ------------- */
    D_800AFB18 = HeapAlloc(*(u32*)((u8*)D_8005A4E0 + 0x110) + 0x10, 0);
    FieldLZSSDecompress(NULL,
                        (u8*)D_8005A4E0 + *(u32*)((u8*)D_8005A4E0 + 0x134),
                        D_800AFB18);
    /* Fixup pass over the decompressed model-2 buffer. The buffer begins with a
     * count word (numWalkMeshes) then per-layer block_size words. The first 4
     * are scaled to per-layer triangle counts (block_size / 0xE == (w>>1)/7)
     * and stored into D_800AFB44[0..3]; the rest are relocated (add the buffer
     * base) into the D_800AFB20 pointer table.
     *
     * Retail asm (FieldLoad 800711B0) stores these counts to `&D_800AFB18 +
     * 0x2C`, which in the game's contiguous layout is the ADDRESS D_800AFB44
     * (0x800AFB18 + 0x2C == 0x800AFB44) -- NOT an offset into the decompressed
     * buffer. The port must express that as D_800AFB44 explicitly (= the
     * D_800AFB20 pointer table + 0x24, i.e. D_800AFB20[9]); writing into the
     * buffer instead left D_800AFB44 zero, which disabled the walkmesh
     * point-to-triangle locator func_8007B1C4 (its loop bound is this count). */
    {
        const u32 kDiv = 0x92492493u; /* fixed-point reciprocal used for /7 */
        u8* pBufBase = (u8*)D_800AFB18;
        u32* pSrc;
        u32* pDst = &D_800AFB20[9];    /* = D_800AFB44[]: per-layer tri counts */
        u32* pBaseTab = D_800AFB20;

        D_800AFB54 = (s16)*(u32*)pBufBase;  /* count */
        pSrc = (u32*)pBufBase + 1;

        for (i = 0; i < 4; i++) {
            u32 w = *pSrc++;
            u64 prod = (u64)(w >> 1) * (u64)kDiv;
            *pDst++ = (u32)(prod >> 32) >> 2;
        }

        pBaseTab[0] = (u32)(uintptr_t)(pBufBase + *pSrc++);   /* D_800AFB20 = base + off */
        if (D_800AFB54 > 0) {
            u32* pA = &pBaseTab[1];        /* a0 walks D_800AFB24[k] from +0x4  */
            u32* pB = &pBaseTab[1 + 4];    /* a1 walks D_800AFB34[k] from +0x14 */
            s32 k = 0;
            do {
                /* Relocate the per-layer triangle/vertex block offsets against
                 * the DECOMPRESSED BUFFER BASE (D_800AFB18), not FB20[0].
                 * Retail (FieldLoad 80071244) reads the relocation base from
                 * -0xC($a2) where $a2 = &D_800AFB20+4, i.e. *(&D_800AFB20-8) ==
                 * *(&D_800AFB18) == pBufBase. The earlier hand-port added
                 * pBaseTab[0] (== pBufBase + off0), over-shifting every
                 * triangle/vertex base by off0 (~0x268) and turning all
                 * walkmesh triangle indices into garbage. */
                pA[0] = (u32)(uintptr_t)(pBufBase + *pSrc++);
                pB[0] = (u32)(uintptr_t)(pBufBase + *pSrc++);
                k++;
                pA++;
                pB++;
            } while (k < *(s16*)((u8*)pBaseTab + 0x34)); /* lh 0x30(a2), a2 moved +4 */
        }
        D_800AFD10 = (s32)((D_800AFB24 - D_800AFB20[0]) >> 2);
    }

    /* --- Sprite-data section (size 0x118, offset 0x13C) --------------------- */
    g_FieldSpriteData = HeapAlloc(*(u32*)((u8*)D_8005A4E0 + 0x118) + 0x10, 0);
    FieldLZSSDecompress(NULL,
                        (u8*)D_8005A4E0 + *(u32*)((u8*)D_8005A4E0 + 0x13C),
                        g_FieldSpriteData);
#ifdef XENO_PC_PORT
    printf("[field-diag] sprite package loaded: spriteData=%p\n", g_FieldSpriteData);
#endif

    /* Reset scene world-rotation flags and load light data (header + 0x154). */
    *(s16*)((u8*)&g_Scene + 0x4C) = 1;
    *(s16*)((u8*)&g_Scene + 0x4E) = 1;
    *(s16*)((u8*)&g_Scene + 0x50) = 1;
    *(s16*)((u8*)&g_Scene + 0x52) = 1;
    func_8006FDEC((u8*)D_8005A4E0 + 0x154);

    /* --- Allocate + zero g_FieldActors, then per-actor init loop ------------ */
    {
        u16 nActors;
        u32* pClear;
        s32 nWords;

        pMap = (u8*)D_8005A4E0;
        nActors = *(u16*)(pMap + 0x18C);       /* header numEntitites */
        /* entity records begin at header + 0x190; s5 walks them */
        /* nWords = nActors * 0x5C / 4 = nActors * 0x17 -> see asm shift math.
         * FieldActor now uses u32 pointer slots so sizeof==0x5C on both the MIPS
         * matching target and the 64-bit port; the faithful nWords path is correct
         * everywhere. */
        nWords = ((((nActors << 1) + nActors) << 3) - nActors); /* nActors*0x17 */
        g_FieldNumActors = nActors;
        pClear = (u32*)HeapAlloc(nWords << 2, 0);
        g_FieldActors = (FieldActor*)pClear;
        for (i = 0; i < nWords; i++) {
            pClear[i] = 0;
        }
#ifdef XENO_PC_PORT
        printf("[field-diag] actors allocated: count=%d actorArray=%p\n", (int)g_FieldNumActors, g_FieldActors);
#endif
    }

    numActors = g_FieldNumActors;
    if (numActors > 0) {
        u16* pEntry = (u16*)((u8*)D_8005A4E0 + 0x190);
#ifdef XENO_PC_PORT
        int modelBuilt = 0;
#endif
        for (i = 0; i < numActors; i++) {
            FieldActor* pActor = &g_FieldActors[i];
            u16 status;

            pActor->status = *pEntry; pEntry += 1;   /* 0x58 */
            pActor->rotation.x = *(pEntry + 0);       /* 0x50 */
            pActor->rotation.y = *(pEntry + 1);       /* 0x52 */
            pActor->rotation.z = *(pEntry + 2);       /* 0x54 */
            pEntry += 3;
            /* three 32-bit position pairs (0x20/0x40, 0x24/0x44, 0x28/0x48).
             * Retail loads the header shorts with lh (sign-extend). A u16
             * promotion zero-extends negative map coords (MAP16 trees sit at
             * ~-1100 and were stored as +64431). */
            *(s32*)((u8*)pActor + 0x20) = (s32)(s16)pEntry[0];
            *(s32*)((u8*)pActor + 0x40) = (s32)(s16)pEntry[0];
            *(s32*)((u8*)pActor + 0x24) = (s32)(s16)pEntry[1];
            *(s32*)((u8*)pActor + 0x44) = (s32)(s16)pEntry[1];
            *(s32*)((u8*)pActor + 0x28) = (s32)(s16)pEntry[2];
            *(s32*)((u8*)pActor + 0x48) = (s32)(s16)pEntry[2];
            pEntry += 3;

            status = (u16)g_FieldActors[i].status;
            if ((status & 0x40) == 0) {
                void* pModel = HeapAlloc(0x24, 0);
                u32* pOffTab;
                u16 spriteId;

                g_FieldActors[i].pModelData = (u32)(uintptr_t)pModel;
                spriteId = *pEntry;
                pOffTab = (u32*)((u8*)D_800AFB14 + (spriteId << 2));
                /* pModel holds PSX 4-byte pointer slots (+0x4 modelData, +0x8/+0xC
                 * the model's double-buffer halves). Store/load them as truncated
                 * u32 (host RAM is linked below 4 GiB, so it round-trips): an 8-byte
                 * host-pointer store here would clobber the neighbouring slot. */
                *(u32*)((u8*)pModel + 0x4) =
                    (u32)((u8*)D_800AFB14 + pOffTab[1] + 0x10);
#ifdef XENO_PC_PORT
                /* Retail-shaped model build (default ON; opt out with
                 * XENO_FIELD_NO_MODEL_BUILD=1). Historically skipped as a
                 * stopgap while the D_8004FE50 build subsystem was unported;
                 * the buildProcs for prims 0x04/0x05/0x0C/0x0D and the
                 * host-callable func_8002C8CC dispatch are now in place.
                 * Unproven paths fail loudly (prim 0x08 buildProc -> abort;
                 * 0xC4/0xC8 inline commands -> "[stub]" logs). Opting out
                 * skips the block (empty model control blocks), which also
                 * skips model prim emission in func_800748E8. */
                if (PcPortModelBuildEnabled())
#endif
                {
                func_8002CB54((void*)(u32)*(u32*)((u8*)pModel + 0x4),
                              (u32*)((u8*)pModel + 0x8),
                              (u32*)((u8*)pModel + 0xC));
                {
                    /* asm: a0 = pModel[0x4] (modelData, the model with header),
                     * a1 = pModel[0x8] (out1), a2 = (status & 0xC) >> 2. */
                    u16 st = (u16)g_FieldActors[i].status;
                    func_8002C8CC((void*)(u32)*(u32*)((u8*)pModel + 0x4),
                                  (void*)(u32)*(u32*)((u8*)pModel + 0x8),
                                  (st & 0xC) >> 2);
                }
                {
                    void* pSrcModel = (void*)(u32)*(u32*)((u8*)pModel + 0x4);
                    memcpy((void*)(u32)*(u32*)((u8*)pModel + 0xC),
                           (void*)(u32)*(u32*)((u8*)pModel + 0x8),
                           *(s32*)((u8*)pSrcModel + 0x34));
                }
                if (g_FieldActors[i].status & 0x2000) {
                    HeapChangeCurrentUser(HEAP_USER_KAZM, NULL);
                    func_800303C8((void*)(u32)*(u32*)((u8*)pModel + 0x4), 0);
                    /* asm: sw v0, 0x14(s0) captures HeapChangeCurrentUser's
                     * return (prior user tag); the shared header types it void,
                     * so preserve the store without the (unused) value. */
                    HeapChangeCurrentUser(HEAP_USER_YOSI, NULL);
                    *(s32*)((u8*)pModel + 0x14) = 0;
                }
                func_8002C644((void*)(u32)*(u32*)((u8*)pModel + 0x4));
#ifdef XENO_PC_PORT
                modelBuilt++;
                {
                    u8* hdr = (u8*)(uintptr_t)*(u32*)((u8*)pModel + 0x4);
                    printf("[field-diag] model-build actor=%d spriteId=%u groups=%u "
                           "status=%04x faPos=(%d,%d,%d)\n",
                           i, (unsigned)spriteId,
                           hdr != NULL ? (unsigned)*(u16*)(hdr + 0x6) : 0u,
                           (unsigned)(u16)g_FieldActors[i].status,
                           (int)*(s32*)((u8*)&g_FieldActors[i] + 0x20),
                           (int)*(s32*)((u8*)&g_FieldActors[i] + 0x24),
                           (int)*(s32*)((u8*)&g_FieldActors[i] + 0x28));
                }
#endif
                }
            } else {
                g_FieldActors[i].status = status | 0x20;
                g_FieldActors[i].rotation.x = 0;
                g_FieldActors[i].rotation.y = 0;
                g_FieldActors[i].rotation.z = 0;
            }
            pEntry += 1;    /* asm: s5 += 2, delay slot of func_80080F44 */
            func_80080F44(i);
        }
#ifdef XENO_PC_PORT
        printf("[field-diag] model-build count=%d / actors=%d\n",
               modelBuilt, (int)numActors);
#endif
    }

    /* --- PC-HDD dev-only path guard ---------------------------------------- */
    if (g_FieldSystemMode == 0) {
        func_802812A4();
    }
    FieldTextBoxInitialize();
    FieldFadeInitialize();

    HeapUnpinBlock(D_8005A4E0);
    HeapFree(D_8005A4E0);
    HeapChangeCurrentUser(HEAP_USER_MIYA, NULL);
    GfxAllocateWorkBuffers(0x3C00, 0);
    WorkListsReset();
    HeapChangeCurrentUser(HEAP_USER_YOSI, NULL);

    /* Geometry / camera work-area setup (D_800B223C.. and the D_800B00xx block).
     * The asm emits absolute stores to a set of distinct data symbols; each is a
     * separate (auto-stubbed) symbol in the port, so we store to them by name. */
    /* Retail 80071640 stores 0x800 at sp+0x1C (matrix element 6).
     * Both call delay slots store zero at sp+0x24 (matrix element 8). */
    func_80077844((short*)D_800B223C, 0x800, 0, 0, 0x800, 0, 0, 0x800, 0, 0);
    func_80077844((short*)(D_800B223C - 0x20), 0x1F8, -0xFC1, -0x1F8, 0, 0, 0, 0, 0, 0);

    D_800B225E = 0x1E;
    D_800B225D = 0x1E;
    D_800B225C = 0x1E;
    D_800B0084 = 0x140;
    D_800B008E = 0;
    D_800B00A9 = 0;
    D_800B00A8 = 0;
    D_800B00A6 = 0;
    D_800B00A5 = 0;
    D_800B00A4 = 0;
    D_800B00A2 = 0;
    D_800B00A1 = 0;
    D_800B00A0 = 0;                 /* sb zero, 0x1C(s2) */
    D_800B0090 = 0;                 /* sw zero, 0xC(s2) */
    D_800B0098 = 0x1000;
    D_800B00B0 = 0;
    D_800B00AE = 0;
    D_800B00AC = 0;
    D_800B008C = 0;
    D_800B008A = 0;
    D_800B0088 = 0;
    D_800B0086 = 0;
    D_800B0082 = 0;
    D_800B0080 = 0;
    D_800B00B2 = 0;
    D_800B0094 = 0;
    D_800B00AA = 0x20;
    D_800ADB1C = 0;
    {
        s32 actorDataCount = 0;
        s32 spriteDataCount = 0;
        s32 activeCount = 0;

        for (i = 0; i < g_FieldNumActors; i++) {
            FieldActor* pActor = &g_FieldActors[i];
            if ((pActor->status & 0x20) == 0) {
                activeCount++;
            }
            if (pActor->pActorData != 0) {
                actorDataCount++;
            }
            if (pActor->pSpriteData != 0) {
                spriteDataCount++;
            }
        }
#ifdef XENO_PC_PORT
        printf("[field-diag] before VM: active=%d actorData=%d/%d spriteData=%d/%d D_800AFC74=%d\n",
               (int)activeCount, (int)actorDataCount, (int)g_FieldNumActors,
               (int)spriteDataCount, (int)g_FieldNumActors, (int)D_800AFC74);
#endif
    }
    func_800A28D4();
    {
        s32 actorDataCount = 0;
        s32 spriteDataCount = 0;
        s32 activeCount = 0;

        for (i = 0; i < g_FieldNumActors; i++) {
            FieldActor* pActor = &g_FieldActors[i];
            if ((pActor->status & 0x20) == 0) {
                activeCount++;
            }
            if (pActor->pActorData != 0) {
                actorDataCount++;
            }
            if (pActor->pSpriteData != 0) {
                spriteDataCount++;
            }
        }
#ifdef XENO_PC_PORT
        printf("[field-diag] after VM: active=%d actorData=%d/%d spriteData=%d/%d D_800AFC74=%d objects=%d player=%d\n",
               (int)activeCount, (int)actorDataCount, (int)g_FieldNumActors,
               (int)spriteDataCount, (int)g_FieldNumActors, (int)D_800AFC74,
               (int)D_800B2264, (int)g_PlayerActorIndex);
        for (i = 0; i < g_FieldNumActors && i < 24; i++) {
            FieldActor* pActor = &g_FieldActors[i];
            ActorData* ad = (ActorData*)(uintptr_t)pActor->pActorData;
            printf("[field-diag] actor[%d] status=%04x flags4=%08x faPos=(%d,%d,%d) "
                   "adPos=(%d,%d,%d) model=%p sprite=%p ip=%u\n",
                   i, (unsigned)(u16)pActor->status,
                   ad != NULL ? (unsigned)ad->flags : 0u,
                   (int)*(s32*)((u8*)pActor + 0x20),
                   (int)*(s32*)((u8*)pActor + 0x24),
                   (int)*(s32*)((u8*)pActor + 0x28),
                   ad != NULL ? (int)*(s16*)((u8*)ad + 0x22) : 0,
                   ad != NULL ? (int)*(s16*)((u8*)ad + 0x26) : 0,
                   ad != NULL ? (int)*(s16*)((u8*)ad + 0x2A) : 0,
                   (void*)(uintptr_t)pActor->pModelData,
                   (void*)(uintptr_t)pActor->pSpriteData,
                   ad != NULL ? (unsigned)ad->scriptInstructionPointer : 0u);
        }
#endif
    }

    D_800ADB1C = 1;
    RotMatrix((SVECTOR*)(D_800B223C - 0xB8), (MATRIX*)D_800AFC30);
    D_800AFC4C = 0;
    D_800AFC48 = 0;
    D_800AFC44 = 0;
    if (D_800B00B2 != 0) {
        /* FieldLoad.s: a0..a3 = B0080/82/lh(*&B0084)/B0086; stack =
         * B0088/8A/8C/8E, &B0084+0xC (=&B0090), &B0084+0x1C (=&B00A0),
         * B00AC/AE/B0. */
        D_800B007C = (s32)func_8002709C(
            D_800B0080, D_800B0082, D_800B0084, D_800B0086,
            D_800B0088, D_800B008A, D_800B008C, D_800B008E,
            (s16*)&D_800B0090,
            (u8*)&D_800B00A0,
            D_800B00AC, D_800B00AE, D_800B00B0);
    }

    HeapChangeCurrentUser(HEAP_USER_YOSI, NULL);
    {
        u16 nA2 = D_800B233E;
        s32 stride = (((nA2 << 1) + nA2) << 3) - nA2;   /* nA2 * 0x17 */
        u32* pA = (u32*)((u8*)g_FieldActors + (stride << 2));
        g_CameraAt2.vx = (s32)pA[8] << 16;              /* +0x20 */
        g_CameraAt2.vy = (s32)pA[9] << 16;              /* +0x24 */
        g_CameraAt2.vz = (s32)pA[10] << 16;             /* +0x28 */
    }

    /* Per-actor matrix setup loop (RotMatrix + copy transform blocks). */
    numActors = g_FieldNumActors;
    if (numActors > 0) {
        for (i = 0; i < numActors; i++) {
            FieldActor* pActor = &g_FieldActors[i];
            RotMatrix((SVECTOR*)((u8*)pActor + 0x50), (MATRIX*)((u8*)pActor + 0xC));
            /* copy 0xC..0x28 -> 0x2C..0x48 (two MATRIX-ish blocks) */
            *(s32*)((u8*)pActor + 0x2C) = *(s32*)((u8*)pActor + 0x0C);
            *(s32*)((u8*)pActor + 0x30) = *(s32*)((u8*)pActor + 0x10);
            *(s32*)((u8*)pActor + 0x34) = *(s32*)((u8*)pActor + 0x14);
            *(s32*)((u8*)pActor + 0x38) = *(s32*)((u8*)pActor + 0x18);
            *(s32*)((u8*)pActor + 0x3C) = *(s32*)((u8*)pActor + 0x1C);
            *(s32*)((u8*)pActor + 0x40) = *(s32*)((u8*)pActor + 0x20);
            *(s32*)((u8*)pActor + 0x44) = *(s32*)((u8*)pActor + 0x24);
            *(s32*)((u8*)pActor + 0x48) = *(s32*)((u8*)pActor + 0x28);
        }
    }

    func_80077C60();
    func_800A2714();
    D_8004F334 = -1;
    D_8004F330 = -1;
    func_80073E38();
    func_80077268();

    if (D_800B00B2 != 0) {
        g_FieldRenderContextUseOT2 = 1;
    } else {
        func_8007469C();
        g_FieldRenderContextUseOT2 = 1;
    }

    /* Final per-script-actor animation init loop. */
    numActors = D_800ADBFC;
    if (numActors > 0) {
        for (i = 0; i < numActors; i++) {
            FieldActor* pActor = &g_FieldActors[i];
            u16 status = *(u16*)((u8*)pActor + 0x58);
            if (status & 0x40) {
#ifndef FIELD_AUDIT_MUTANT_SKIP_FINAL_ACTOR_ANIM
                /* pActorData (offset 0x4C) is populated by the retail-derived
                 * func_80080F44 path above. Keep the final direction update in
                 * every production build; the guard exists only for the focused
                 * omission mutant. */
                void* pModel = (void*)(uintptr_t)*(u32*)((u8*)pActor + 0x4C);
                u32 flag = *(u32*)((u8*)pModel + 0x4);
                if (flag & 0x1000000) {
                    func_80021FE0((void*)(uintptr_t)*(u32*)((u8*)pActor + 0x4),
                                  *(s16*)((u8*)pModel + 0x108));
                } else {
                    s16 v = (s16)(g_CamInterpolation.curAngleY +
                                  *(u16*)((u8*)pModel + 0x108));
                    func_800223B0((void*)(uintptr_t)*(u32*)((u8*)pActor + 0x4), v);
                }
#endif
            }
        }
    }
}
#endif /* XENO_PC_PORT */
