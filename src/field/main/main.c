#include "common.h"
#include "psyq/libgpu.h"
#include "psyq/libetc.h"
#include "system/memory.h"
#include "system/archive.h"
#include "system/controller.h"
#include "field/actor.h"

extern int g_FrameDeltaTime;

void FieldInitializeControllers(void) {
    HeapChangeCurrentUser(HEAP_USER_YOSI, NULL);
    ArchiveSetIndex(4, 0);
    FieldInitializeControllersAndMouse();
}

void FieldRenderSync(void) {
    DrawSync(0);
    Vsync(0);
}

/* ---- FieldLoadUITextures: load UI texture CLUT from archive ------------------
 * Loads archive 0xA7, unpacks 8 TIM images via FieldLoadTIMWithClut,
 * stores combined CLUT to VRAM at D_800B004C RECT, reads back into
 * D_800AFC08 via StoreImage for later use by func_80074108.
 *
 * Guard: D_8004F344 — non-zero skips archive load (textures already loaded).
 * Texture descriptors: D_800ADC44[8] — 0xC bytes/entry (ROM data).
 * Temp buffer D_8005A4A0 is HeapAlloc'd, freed at end. */
extern s32 D_8004F344;
extern void* D_8005A4A0;
extern u16 D_800ADC44[8 * 6];
extern u16 D_800C2692, D_800C2690, D_800C38FE, D_800C38FC;
extern u16 D_800B004C, D_800B004E, D_800B0050, D_800B0052;
extern u16 D_800AFC08[0x80];
extern void ResolveArchiveEntryPointers(void* pData);

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/field/nonmatchings/main/main", FieldLoadUITextures);
#else
void FieldLoadUITextures(void) {
    u16* pTable;
    u16* pScan;
    u16* pTableB;
    s32 byteOff;
    u32* pDataPtr;
    s32 i;

#ifdef XENO_PC_PORT
    /* Boot streams archive 0xA7 into D_8005A4A0 and sets D_8004F344=1.
     * Movie teardown / heap reset can leave that pointer non-canonical
     * (observed: HeapUnpinBlock(0xf8000248e20000b0) SIGSEGV on title
     * entry). Reload from the archive whenever the cached buffer is not
     * a host heap pointer. */
    if (D_8005A4A0 == NULL ||
        ((uintptr_t)D_8005A4A0 >> 47) != 0) {
        D_8004F344 = 0;
        D_8005A4A0 = NULL;
    }
#endif

    if (D_8004F344 == 0) {
        s32 size = ArchiveDecodeAlignedSize(0xA7);
        void* pBuf = HeapAlloc(size, 1);
        D_8005A4A0 = pBuf;
        HeapPinBlock(pBuf);
        ArchiveReadFileToBuffer(0xA7, (u32*)pBuf, 0, 0x80);
        ArchiveCdDataSync(0);
    }

    pTable = D_800ADC44;
    pTableB = D_800ADC44 + 5;
    if (D_8005A4A0 != NULL) {
        HeapUnpinBlock(D_8005A4A0);
    }
    D_8004F344 = 0;
    D_800C2692 = 0;
    D_800C2690 = 0;
    D_800C38FE = 0;
    D_800C38FC = 0;
    ResolveArchiveEntryPointers(D_8005A4A0);

    byteOff = 0;
    pDataPtr = (u32*)((u8*)D_8005A4A0 + 4);
    pScan = pTable;

    for (i = 0; i < 8; i++) {
        FieldLoadTIMWithClut((u_long*)*pDataPtr,
            pScan[0],
            *(s16*)((u8*)pTable + byteOff + 2),
            *(s16*)((u8*)pTable + byteOff + 4),
            *(s16*)((u8*)pTable + byteOff + 6),
            *(s16*)((u8*)pTable + byteOff + 8),
            pTableB[0]);
        DrawSync(0);
        pScan += 6;
        pTableB += 6;
        byteOff += 0xC;
        pDataPtr += 1;
    }

    D_800B004C = 0;
    D_800B004E = 0xFB;
    D_800B0050 = 0x10;
    D_800B0052 = 1;
    StoreImage((RECT*)&D_800B004C, (u_long*)D_800AFC08);
    DrawSync(0);
#ifdef XENO_PC_PORT
    /* PsyCross StoreImage/GR_ReadVRAM doesn't write back to the caller's
     * buffer (VRAM data exists but the memcpy path is broken). Copy directly
     * from PsyCross's host VRAM array to D_800AFC08 as a compatibility shim.
     * Remove once GR_ReadVRAM is fixed upstream. */
    {
        extern unsigned short vram[];  /* PsyCross host VRAM (PsyX_render.cpp) */
        s32 vy = D_800B004E;
        s32 vw = D_800B0050;
        for (i = 0; i < vw && i < 0x80; i++) {
            D_800AFC08[i] = vram[i + vy * 1024];
        }
    }
#endif
    HeapFree(D_8005A4A0);
}
#endif /* XENO_PC_PORT */

extern s32 g_GameSceneMapNum;

void func_800777DC(void) {
    ArchiveCdDataSync(0);
    while (func_8001B484((g_GameSceneMapNum & 0xFFF) << 1, 0) != 0) {
    }
}

void FieldUpdateDeltaTime(void) {
    g_FrameDeltaTime = Vsync(1);
}

void func_80077844(short* dst, int a, int b, int c, int d, int e, int f, int g, int h, int i) {
    dst[0] = a;
    dst[1] = b;
    dst[2] = c;
    dst[3] = d;
    dst[4] = e;
    dst[5] = f;
    dst[6] = g;
    dst[7] = h;
    dst[8] = i;
}

extern s32 D_800B2264;
extern u8 D_800B2394;
extern s32 D_8004F370;
extern void* D_800ADB20;
extern void* D_800ADB30;
extern s16 D_800B234A;
extern u8 D_800B225C, D_800B225D, D_800B225E;
extern u8 g_FieldEffects[];
extern void* D_8005A420[];
extern void* D_8005A450[];
extern void* D_801E8644;
#ifdef XENO_PC_PORT
/* Retail 0x801E8670 is a table of PSX words; a void* declaration strides
 * eight bytes on the 64-bit host, so slots 1.. read the wrong entries and
 * the object scale table below was never written for them. */
extern u32 D_801E8670[];
#else
extern void* D_801E8670[];
#endif
extern void FieldRenderSyncAndFlush(void);
extern void func_801E738C(s32 arg0);
extern void func_801E742C(s32 arg0, s32 arg1, void* arg2, void* arg3,
                          s32 arg4, s32 arg5, s32 arg6, s32 arg7, void* arg8);
extern void func_800A90B4(s32 arg0);

static inline void FieldSetArchiveQueueEntry(s32 index, u16 archiveIndex, void* pData) {
    u8* entry = &D_800B2394 + index * 8;

    *(u16*)entry = archiveIndex;
    *(u32*)(entry + 4) = (u32)(unsigned long)pData;
}

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/field/nonmatchings/main/main", func_80077884);
#else
void func_80077884(void) {
    s32 i;
    s16* archiveIds;
    s32 size;

    if (D_800B2264 == 0) {
        return;
    }

    func_8008A520();
    ArchiveSetIndex(4, 0);
    func_800A90B4(0);

#ifdef XENO_PC_PORT
    /* Retail's D_8004F370==0 branch sizes this buffer as the fixed-map gap
     * [0x801DC008, D_800ADB30) -- meaningless host-pointer arithmetic in the
     * port (bogus ~5MB -> HeapAlloc failure -> GameHandleError(130) spin,
     * MAP3 repro).  Size by the archive's decoded size instead -- the same
     * derivation retail's own else-branch uses for the same buffer.  Same
     * pattern as the menu-overlay fix in misc4.c (func_80078E90). */
    size = ArchiveDecodeAlignedSize(0x6B9);
#else
    if (D_8004F370 == 0) {
        size = ((u32)(unsigned long)D_800ADB30 & 0xFFFFFF) + (s32)0xFFE23FF8;
    } else {
        size = ArchiveDecodeAlignedSize(0x6B9);
    }
#endif
    D_800ADB20 = HeapAlloc(size, 1);

    func_800A90B4(1);

    archiveIds = (s16*)((u8*)&D_800B2264 - 0x88);
    for (i = 0; i < D_800B2264; i++) {
        s32 archiveIndex = (u16)archiveIds[i] + 0x6BB;
        D_8005A450[i] = HeapAlloc(ArchiveDecodeAlignedSize(archiveIndex), 1);
        FieldSetArchiveQueueEntry(i * 2 + 1, archiveIndex, D_8005A450[i]);
    }

    for (i = 0; i < D_800B2264; i++) {
        s32 archiveIndex = (u16)archiveIds[i] + 0x6BA;
        D_8005A420[i] = HeapAlloc(ArchiveDecodeAlignedSize(archiveIndex), 0);
        FieldSetArchiveQueueEntry(i * 2, archiveIndex, D_8005A420[i]);
    }

    FieldSetArchiveQueueEntry(D_800B2264 * 2, 0x6B9, D_800ADB20);
    FieldSetArchiveQueueEntry(D_800B2264 * 2 + 1, 0, NULL);
    func_8008A520();
    func_80029AFC((StreamDataQueueEntry*)&D_800B2394, 0, 0);
}
#endif /* XENO_PC_PORT */

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/field/nonmatchings/main/main", func_80077AB4);
#else
void func_80077AB4(void) {
    s32 i;
    u8* work;
    s32 top;

    work = (u8*)&D_800B2264 - 0x78;
    if (D_800B2264 == 0) {
        return;
    }

    func_8008A520();
    FieldRenderSyncAndFlush();
    func_801E738C(D_800B234A);
    D_801E8644 = (u8*)&D_800B2264 - 0x28;
    SetBackColor(D_800B225C, D_800B225D, D_800B225E);

    top = 0xFC;
    for (i = 0; i < D_800B2264; i++) {
        s16* vec = (s16*)(work + i * 8);
        s32 effect = ((u8*)g_FieldEffects)[0x1E7 + i];
        s32 x = (s16)(0x240 - ((i + effect) << 6));

        vec[0] = 0;
        vec[1] = 0;
        vec[2] = 0;
        func_801E742C(i, 0, D_8005A420[i], D_8005A450[i],
                      x, 0x100, 0, top, vec);
        HeapFree(D_8005A450[i]);
#ifdef XENO_PC_PORT
        /* Native owner guard: func_801E742C normally publishes the instantiated
         * archive-0x6B9 object here. Retail assumes that owner succeeded; the
         * fail-closed branch remains an explicit, audited port divergence. */
        if (D_801E8670[i] == 0) {
            top += 1;
            continue;
        }
#endif
        *(s32*)(work + 0x20 + i * 4) = *(s16*)((u8*)(uintptr_t)D_801E8670[i] + 0x1C);
        top += 1;
    }

    HeapChangeCurrentUser(HEAP_USER_YOSI, NULL);
}
#endif /* XENO_PC_PORT */

void func_80077C60(void) {
    func_80077884();
    func_80077AB4();
}

extern void* g_PartyDataBuffers[];

void FieldPartyAllocateSkinDataBuffers(void) {
    HeapChangeCurrentUser(HEAP_USER_YOSI, NULL);
    g_PartyDataBuffers[0] = HeapAlloc(0x14000, 0);
    g_PartyDataBuffers[1] = HeapAlloc(0x14000, 0);
    g_PartyDataBuffers[2] = HeapAlloc(0x14000, 0);
    HeapPinBlock(g_PartyDataBuffers[0]);
    HeapPinBlock(g_PartyDataBuffers[1]);
    HeapPinBlock(g_PartyDataBuffers[2]);
}

void FieldPartyFreeSkinDataBuffers(void) {
    HeapUnpinBlock(g_PartyDataBuffers[0]);
    HeapUnpinBlock(g_PartyDataBuffers[1]);
    HeapUnpinBlock(g_PartyDataBuffers[2]);
    HeapFree(g_PartyDataBuffers[0]);
    HeapFree(g_PartyDataBuffers[1]);
    HeapFree(g_PartyDataBuffers[2]);
}

extern s32 D_800ADB9C;
extern int g_FieldSystemMode;
extern void FieldClearAndSwapOTag(void);
extern void FieldPollControllers(void);
extern void func_80281B00(void* arg0);
extern u8 D_8006FB80;
extern void func_800A31E8(void);

void func_80077DAC(void) {
    D_800ADB9C = Vsync(1);
    FieldClearAndSwapOTag();
    FieldPollControllers();

    if (g_FieldSystemMode == 0) {
        func_80281B00(&D_8006FB80);
    }

    func_800A31E8();
}

extern FieldActor* volatile g_FieldActors;
extern s32 D_800ADBD0;
extern s16 D_800B2344;
extern s32 g_PlayerActorIndex;

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/field/nonmatchings/main/main", func_80077E10);
#else
s32 func_80077E10(void) {
    if (D_800ADBD0 == 1 && D_800B2344 == 0) {
        /* FieldActor is 0x5C on the PSX.  Its +0x4C pActorData member is a
         * 32-bit PSX pointer slot; reconstruct it before reading ActorData. */
        ActorData* pActorData =
            (ActorData*)(uintptr_t)g_FieldActors[g_PlayerActorIndex].pActorData;
        u32 flags = pActorData->scriptFlags.flags;
        return -(s32)((flags & 0x800) != 0);
    }
    return 0;
}
#endif /* XENO_PC_PORT */

/* ---- FieldMain: field game-state driver (Phase C gateway) -------------------
 * Functional decompile (port-first; not yet byte-matched). Control flow mirrors
 * asm/field/nonmatchings/main/main/FieldMain.s 1:1 via labels/gotos; every call
 * is preserved. The native dependency audit identifies any call that still
 * resolves to generated storage. g_FieldSystemMode comes from D_80010000
 * (-1 in retail rodata) exactly as
 * the original does -> SYSTEM_MODE_CD_ROM, which skips the mode-0-only `break 1`
 * and the raw-0x80280000 dev read. Port-only instrumentation is guarded. */
#ifdef XENO_PC_PORT
extern int printf(const char*, ...);
#define FM_LOG(...) printf("[FieldMain] " __VA_ARGS__)
#else
#define FM_LOG(...) ((void)0)
#endif

extern int g_FieldSystemMode;
extern int g_FieldCurRenderContextIndex;
#ifndef SYSTEM_MODE_PC_HDD
#define SYSTEM_MODE_PC_HDD 0
#define SYSTEM_MODE_CD_ROM 1
#endif
extern int D_80010000;
extern void *g_pGameState;
extern u8 g_GameState[];
extern s32 g_PlayerActorIndex;
extern FieldActor *volatile g_FieldActors; /* 0x5C stride; +0x4C = pActorData */
extern u8 g_FieldEffects[];
extern u8 g_FieldDefaultParticleBanks[];
extern s32 g_GamePartySkinsInitialized;
extern s32 g_GamePartyMemberSkins[];

/* word globals */
extern s32 D_8004F370, D_8004F2F8, D_8004F324, D_8004F320, D_8004F308, D_8004F338;
extern s32 D_8004F348, D_8004F334, D_8004F310, D_8004F31C, D_8004F354, D_8004F358;
extern s32 D_8004F378, D_8004F37C, D_8004F380;
extern s32 D_800ADBE8, D_800ADBE4, D_800ADBE0, D_800ADBDC, D_800ADBD8, D_800ADBD4, D_800ADBD0;
extern s32 D_800ADBEC, D_800ADBC4, D_800ADB90, D_800ADB68, D_800ADB64, D_800ADB70, D_800ADB18;
extern s32 D_800ADC10, D_800ADC04, D_800ADB60, D_800ADB34, D_800ADB7C;
extern s32 D_800AFC78;
/* pointer globals */
extern void *D_80059560, *D_800595AC, *D_8006251C, *D_80062524, *D_800ADB30, *D_8005A4E0, *D_80062528;
/* halfword globals */
extern u16 D_8006F94E, D_8006F954, D_8006F950, D_8006F956, D_800C3900, D_800AFE9C, D_800C3908, D_800B236C;
extern s16 D_800B2290;
/* byte globals */
extern u8 D_800594D0, D_8005954C, D_800B2358, D_800B2355, D_800ADB04, D_800B21D0, D_80059171, D_80059179;
extern s32 D_80059488;

extern int ControllerGetType();
extern int func_80078BC8();
extern int func_80077E10();
extern int func_8001B484();
extern int ArchiveDataSync(void);
extern void FieldRenderSyncAndFlush();
extern void func_80085890(), func_802811EC(), func_80078D44(), func_80077DAC(), func_8007554C();
extern void func_800A5924(), func_8001B66C(), func_80085B20(), func_800A3F4C(), func_8003A89C();
extern void func_8007FFE8(), func_800A30FC(), func_800A5C40(), func_800798BC(), func_800ABA98();
extern void func_800A7C58(), func_800799D4(), func_800ACE90(), func_80078B5C(), func_800A91F0();
extern void func_800A31E8(), func_800864F0(), func_80085988(), func_8007954C();
extern void FieldInitializeDefaultParticleBanks(), FieldInitializeControllers(), FieldLoadUITextures();
extern void GamePartySyncSkinData(), GamePartySyncStreamedData(), GraphicsDrawPauseLetters();
extern void FieldPollControllers(), SoundMuteAllSpuChannels(), SoundEnableAllSpuChannels();
extern void GameCheckAndHandleSoftReset(), FieldParticlesFreeAll(), FieldFree();
extern void FieldScriptMemoryWriteU16();
#ifdef XENO_PC_PORT
extern void PcPort_QuickCheckpointSetFieldActive(int active);
extern int PcPort_QuickCheckpointPoll(void);
extern void PcPort_QuickCheckpointCommitLoad(void);
extern void PcPort_QuickCheckpointRestorePlayer(void);
extern void ChangeGameState(unsigned int state);
extern void MainLoop(int errorCode) __attribute__((noreturn));
#endif

/* g_FieldActors[idx].pActorData->flags. pActorData is a u32 slot (== pointer
 * size on MIPS, so matching-safe). On the 64-bit port the u32 is widened to a
 * host pointer via uintptr_t before dereferencing. sizeof(FieldActor)==0x5C on
 * both builds, so the raw offset 0x4C is correct. The native null return is a
 * fail-closed ownership guard; retail assumes func_80080F44 published a valid
 * actor-data pointer, so the guard remains an audited divergence. */
#ifdef XENO_PC_PORT
#define FIELD_ACTOR_FLAGS(idx) \
    ({ u32 _p = *(u32*)((u8*)g_FieldActors + (idx) * 0x5C + 0x4C); \
       _p ? *(s32*)(uintptr_t)_p : 0; })
#else
#define FIELD_ACTOR_FLAGS(idx) \
    (*(s32*)(uintptr_t)(*(u32*)((u8*)g_FieldActors + (idx) * 0x5C + 0x4C)))
#endif

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/field/nonmatchings/main/main", FieldMain);
#else
void FieldMain(void) {
    int exitCode = 0;   /* s0 at teardown (set 0/1/2/3 on the exit paths) */
    int savedSound;     /* s0 reuse: D_80059488 save/restore in the wait loops */
    int waterInit = 0;  /* s4 */
    int s5flag = 0;     /* s5: one-time D_800AFC78 latch */

    FM_LOG("entry. D_80010000=0x%08x\n", (unsigned)D_80010000);
    if (D_80010000 == -1) {
        g_FieldSystemMode = SYSTEM_MODE_CD_ROM;
    } else {
        g_FieldSystemMode = SYSTEM_MODE_PC_HDD;
    }
    FM_LOG("g_FieldSystemMode=%d (%s)\n", g_FieldSystemMode,
           g_FieldSystemMode == SYSTEM_MODE_CD_ROM ? "CD_ROM" : "PC_HDD");
    FieldRenderSyncAndFlush();
    if (g_FieldSystemMode == 0) {
        DrawSyncCallback(FieldUpdateDeltaTime);
    }
    D_8006251C = D_80059560;
    D_80062524 = D_800595AC;
    HeapChangeCurrentUser(HEAP_USER_YOSI, NULL);

    if (g_FieldSystemMode == 0 && D_8004F370 == 0) {
        /* PC-HDD dev path (not taken in the port; raw 0x80280000 is a PSX addr). */
        ArchiveSetIndex(4, 0);
        FM_LOG("dev field read: archive id 0xAD -> 0x80280000\n");
        ArchiveReadFileToBuffer(0xAD, (s32)0x80280000, 0, 0x80);
        ArchiveCdDataSync(0);
        FieldRenderSyncAndFlush();
    }

    if (!g_GamePartySkinsInitialized) {
        g_GamePartyMemberSkins[2] = 0xFF;
        g_GamePartyMemberSkins[1] = 0xFF;
        g_GamePartyMemberSkins[0] = 0xFF;
    }
    func_80085890(0);
    FieldPartyAllocateSkinDataBuffers();
    D_800ADBE8 = -1; D_800ADBE4 = -1; D_800ADBE0 = -1; D_800ADBDC = -1; D_800ADBD8 = -1;
    D_8004F358 = 0; D_8004F354 = 0;
    D_800ADC10 = 0; D_800ADB60 = 0; D_800ADB34 = 0; D_800ADB7C = 0;
    D_800ADC04 = 2;
    if (g_FieldSystemMode == 0) {
        func_802811EC();
    }
    FM_LOG("FieldInitializeControllers\n");
    FieldInitializeControllers();

    g_pGameState = &g_GameState;
    g_GameSceneMapNum = D_8006F94E;
    *(s16*)(g_GameState + 0x1932) = D_8006F954;
    *(s16*)(g_GameState + 0x1938) = D_8006F950 >> 9;
    if (D_8004F2F8 == 0) {
        D_800594D0 = 0;
        D_8004F324 = 0xFF;
    } else {
        D_8004F324 = D_8006F956;
    }
    FM_LOG("g_pGameState=%p g_GameSceneMapNum=%d\n", g_pGameState, (int)g_GameSceneMapNum);
    if (g_FieldSystemMode == SYSTEM_MODE_CD_ROM) {
        *(s16*)((u8*)g_pGameState + 0x1980) = 1;
        FieldScriptMemoryWriteU16(0x50, 1);
    }
    FM_LOG("FieldLoadUITextures\n");
    FieldLoadUITextures();
    D_8004F320 = 0;
    GamePartySyncSkinData();
    GamePartySyncStreamedData();
    D_800ADB30 = HeapAlloc(4, 1);
    if (g_FieldSystemMode == 0) {
        /* mode-0 only: the original has a `break 1` trap here. */
        FieldInitializeDefaultParticleBanks(g_PlayerActorIndex);
        *(s16*)(g_FieldDefaultParticleBanks + 0) = 1;
        *(s16*)(g_FieldDefaultParticleBanks + 6) = 0x10;
    }
    func_80078D44();
#ifdef XENO_PC_PORT
    PcPort_QuickCheckpointRestorePlayer();
    PcPort_QuickCheckpointSetFieldActive(1);
#endif
    D_800ADB04 = 1;
    FM_LOG("entering main loop\n");

    for (;;) {  /* .L80078174 */
#ifdef XENO_PC_PORT
        if (PcPort_QuickCheckpointPoll()) {
            exitCode = 4;
            goto teardown;
        }
#endif
        /* wait for a controller if none present */
        if (ControllerGetType(0) == 0) {
            savedSound = D_80059488;
            SoundMuteAllSpuChannels();
            GraphicsDrawPauseLetters(0x88, (((g_FieldCurRenderContextIndex + 1) & 1) << 8) | 0x64);
            do {
                DrawSync(0); Vsync(2); FieldPollControllers(); GameCheckAndHandleSoftReset();
            } while (ControllerGetType(0) == 0);
            SoundEnableAllSpuChannels();
            D_80059488 = savedSound;
        }
        /* paused/menu hold */
        if ((D_800C3900 & 0x800) && !(D_800AFE9C & 0x40) && !D_800B2358) {
            savedSound = D_80059488;
            SoundMuteAllSpuChannels();
            GraphicsDrawPauseLetters(0x88, (((g_FieldCurRenderContextIndex + 1) & 1) << 8) | 0x64);
            do {
                DrawSync(0); Vsync(2); FieldPollControllers(); GameCheckAndHandleSoftReset();
            } while (D_800C3900 & 0x800);
            SoundEnableAllSpuChannels();
            D_80059488 = savedSound;
        }
        if (g_FieldSystemMode == SYSTEM_MODE_CD_ROM) {
            FieldScriptMemoryWriteU16(0x50, 1);
        }
        GameCheckAndHandleSoftReset();
        func_80077DAC();
        func_8007554C();
        func_800A5924();

        /* render-context dispatch (g_FieldCurRenderContextIndex == 1) */
        if (g_FieldCurRenderContextIndex == 1 && D_800ADBDC == 0
            && func_80078BC8() == 0 && func_80077E10() == 0) {
            if (D_8004F334 != -1) {
                HeapUnpinBlock(D_8005A4E0);
                HeapFree(D_8005A4E0);
            }
            if (s5flag == 0) {
                D_800AFC78 = (s32)D_8004F324;
                s5flag = 1;
            }
            func_8007FFE8();
            if (D_800ADBD0 == 1) {
                D_8005954C = (u8)D_800B2355;
                D_800AFC78 = (s32)D_8004F324;
                if (D_8004F338 != D_800B2290) {
                    if (D_8004F338 != -1) {
                        D_8004F348 = 1;
                    }
                    func_8001B66C();
                    D_8004F308 = -1;
                    D_8004F324 = D_800B2290;
                    func_80085B20(D_800B2290, 1);
                }
                D_800ADBD0 = 0;
                D_800ADBD4 = 1;
                goto after_ctx;
            } else {
                if (D_800ADB18 == 0) {
                    g_GamePartySkinsInitialized++;
                    func_800A3F4C();
                }
                exitCode = 0;
                if (D_800ADBD4 == 1) {
                    func_8003A89C(D_80062528, 0x7F, 0);
                }
                D_800ADBD4 = 0;
                goto teardown;
            }
        }

    after_ctx:  /* .L80078494 */
        if (D_800ADBEC == 0 && D_8004F308 == 0 && D_800ADBC4 == 0xFF && D_800ADB90 == 0
            && func_8001B484((g_GameSceneMapNum & 0xFFF) << 1, 0) == 0
            && ArchiveDataSync() == 0
            && *(s16*)(g_FieldEffects + 0xA0) == 0) {
            D_800ADB04 = 0;
            func_800A30FC();
            ArchiveCdDataSync(0);
            func_800A5C40();
            ControllerResetState();
            D_800ADB04 = 1;
        }

        /* .L80078558: render-context teardown ladder (exit codes 1/2/3) */
        if (g_FieldCurRenderContextIndex == 1) {
            if (D_800ADBE4 == 0 && func_80078BC8() == 0) {
                ArchiveCdDataSync(0);
                exitCode = 1;
                if (D_8004F334 != -1) {
                    HeapUnpinBlock(D_8005A4E0);
                    HeapFree(D_8005A4E0);
                }
                goto teardown;
            }
            if (g_FieldCurRenderContextIndex == 1) {  /* re-checked in asm */
                if (D_800ADBE8 == 0 && func_80078BC8() == 0) {
                    ArchiveCdDataSync(0);
                    if (D_8004F334 != -1) {
                        HeapUnpinBlock(D_8005A4E0);
                        HeapFree(D_8005A4E0);
                    }
                    D_8004F310++;
                    exitCode = 2;
                    func_800A3F4C();
                    goto teardown;
                }
                if (g_FieldCurRenderContextIndex == 1 && D_800ADBD8 == 0 && func_80078BC8() == 0) {
                    ArchiveCdDataSync(0);
                    if (D_8004F334 != -1) {
                        HeapUnpinBlock(D_8005A4E0);
                        HeapFree(D_8005A4E0);
                    }
                    exitCode = 3;
                    func_8001B66C();
                    goto teardown;
                }
            }
        }

        /* .L800786F4: per-frame field update/render */
        if (g_FieldSystemMode == 0) {
            u16 f3908 = D_800C3908;
            if (f3908 & 0x40) D_8004F378 = (D_8004F378 + 1) & 1;
            if (f3908 & 0x10) D_8004F37C = (D_8004F37C + 1) & 1;
            if (f3908 & 0x80) D_8004F380 = (D_8004F380 + 1) & 1;
            if ((D_800AFE9C & 0x40) && (D_800C3900 & 0x100) && D_800ADBEC == -1
                && D_8004F308 == 0 && D_800ADB34 == 0) {
                g_GameSceneMapNum = 0;
                D_800ADBEC = 0;
                FieldScriptMemoryWriteU16(2, 0);
            }
        }

        /* .L80078810 */
        if (D_800ADBD8 == -1 && D_800ADBDC == -1 && D_800ADBE4 == -1
            && func_80078BC8() == 0 && D_800ADBEC == -1) {
            u16 fe9c = D_800AFE9C;
            if ((fe9c & 3) == 0) {
                waterInit = 0;
            }
            if ((fe9c & 1) && D_800ADB68 == 1 && (fe9c & 2) && waterInit == 0) {
                func_800798BC();
                waterInit = 1;
                if (D_800ADB64 == 0xFF && (FIELD_ACTOR_FLAGS(g_PlayerActorIndex) & 0x1800) == 0
                    && D_80059179 == 0) {
                    func_800ACE90();
                }
            }
        }
        if ((D_800C3900 & 0x100) && D_800ADBEC == -1 && D_800ADB68 == 1) {
            func_800ABA98();
        }
        if (D_800ADB70 && g_FieldCurRenderContextIndex == 1) {
            func_800A7C58();
            D_800ADB70 = 0;
        }
        if (D_800ADB64 != 0xFF && g_FieldCurRenderContextIndex == 0
            && (FIELD_ACTOR_FLAGS(g_PlayerActorIndex) & 0x1800) == 0) {
            func_8007FFE8();
            func_800799D4();
            D_800ADB64 = 0xFF;
        }
        if ((D_800C3900 & 0x10) && !D_800B21D0 && D_800ADB64 == 0xFF && D_800ADB68 == 1) {
            D_800ADB64 = 0x80;
            D_80059171 = (u8)D_800B236C;
        }

        /* .L80078AAC */
        func_80078B5C();
    }

teardown:  /* .L80078ABC */
    FM_LOG("teardown exitCode=%d\n", exitCode);
#ifdef XENO_PC_PORT
    PcPort_QuickCheckpointSetFieldActive(0);
#endif
    func_800798BC();
    func_800A91F0();
    func_800A31E8();
    FieldParticlesFreeAll();
    func_800864F0();
    func_8007FFE8();
    DrawSync(0);
    Vsync(0);
    FieldFree();
    FieldPartyFreeSkinDataBuffers();
    func_80085988();
    D_8004F31C = 0;
    HeapFree(D_800ADB30);
#ifdef XENO_PC_PORT
    if (exitCode == 4) {
        PcPort_QuickCheckpointCommitLoad();
        ChangeGameState(1);
        MainLoop(0);
    }
#endif
    func_8007954C(exitCode);
}
#endif /* XENO_PC_PORT */
