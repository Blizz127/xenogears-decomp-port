#include "common.h"
#include "field/actor.h"
#include "field/main.h"
#include "field/text_box.h"
#include "field/effects.h"
#include "system/memory.h"
#include "system/archive.h"
#include "system/sound.h"
#include "system/math.h"
#include "psyq/libetc.h"
#include "psyq/libcd.h"
/* Retail inlines the libgte macro form of NormalClip (6 inlined NCLIP in
 * func_8007CD80, zero `jal NormalClip`). Use the same PSY-Q inline macros in
 * the matching build; the port resolves these to PsyCross' equivalents. */
#ifdef XENO_PC_PORT
#include <psx/inline_c.h>
#include <psx/gtemac.h>
/* PsyCross gte_ldsxy3 dereferences its arguments; PSY-Q mtc2's packed SXY
 * values. Route the matching-shape macro onto NormalClip() so the live
 * path does not SEGV. Matching still uses the PSY-Q inline form. */
#undef gte_NormalClip
#define gte_NormalClip(r1, r2, r3, r4) \
    do { *(r4) = NormalClip((int)(r1), (int)(r2), (int)(r3)); } while (0)
#else
#include "psyq/inline_c.h"
#include "psyq/gtemac.h"
#endif
#ifdef XENO_PC_PORT
#include <stdint.h> /* uintptr_t for LP64 pointer widening below */
#include <stdio.h>
#include <stdlib.h> /* getenv for the env-gated party-pointer diagnostic */
#endif

/* Per-frame helper: rand seed, map-load check, angle-step timer. */
extern s32 D_8004F308, D_8004F324, D_800ADC18;
extern int func_80085C90(int);
#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/field/nonmatchings/main/misc4", func_80078B5C);
#else
void func_80078B5C(void) {
    rand();
    if (D_8004F308 == -1) {
        D_8004F308 = func_80085C90(D_8004F324);
    }
    if (D_800ADC18 != 0) {
        D_800ADC18--;
    }
}
#endif /* XENO_PC_PORT */

extern s32 D_800ADB2C;
extern s32 D_800ADB90;
extern s32 D_800ADB34;
extern s32 D_800ADBC4;

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/field/nonmatchings/main/misc4", func_80078BC8);
#else
s32 func_80078BC8(void) {
    if (D_800ADB2C != 0) {
        return -1;
    }
    if (ArchiveDataSync() != 0) {
        return -1;
    }
    if (D_8004F308 != 0) {
        return -1;
    }
    if (D_800ADB90 != 0) {
        return -1;
    }
    if (D_800ADB34 != 0) {
        return -1;
    }
    return D_800ADBC4 != 0xFF ? -1 : 0;
}
#endif /* XENO_PC_PORT */

extern s16 D_800B2344;
void func_80078C5C(void) {
    RECT rect;
    int i;
    u_int* pImage;

    if (D_800B2344 != 0) {
        g_FieldRenderContexts[0].drawEnvs[0].dtd = 0;
        g_FieldRenderContexts[1].drawEnvs[0].dtd = 0;
        pImage = HeapAlloc(0x4000, 0x0);
        rect.y = 0x1E0;
        rect.w = 0x100;
        rect.x = 0;
        rect.h = 0x20;
        StoreImage(&rect, pImage);
        DrawSync(0);

        // Process image before storing it back in VRAM
        for (i = 0; i < 0x1000; i++) {
            if (pImage[i] & 0xFFFF) {
                pImage[i] |= 0xC63;
            }
            if (pImage[i] & 0xFFFF0000) {
                pImage[i] |= 0x0C630000;
            }
        }
        
        LoadImage(&rect, pImage);
        DrawSync(0);
        HeapFree(pImage);
    }
}

/* ---- func_80078D44: field-init / initial map-load orchestrator --------------
 * Functional decompile (port-first; control flow mirrors the asm). FieldMain's
 * setup calls this: it loads the map file (func_800777DC -> func_8001B484), brings
 * up the render contexts + VRAM, sets g_FieldCurRenderContextIndex=1, then parses
 * the map via FieldLoad (which allocates g_FieldActors). The fade/zoom loops play a
 * transition; in the port their effect calls stub out and ArchiveDataSync returns
 * 0, so the loops just iterate their fixed counts. */
extern s32 D_8004F2F8, D_8004F304, D_8004F308, D_8004F310, D_8004F324, D_8004F338;
extern s32 D_8004F2FC, D_8004F348, D_800ADB60, D_800C2684, D_800B2264, D_800AFD04;
extern s32 g_GamePartySkinsInitialized;
extern u8 D_800594D0, D_8005942C;
extern void *D_800ADC14, *D_80062528, *g_GameCurLoadedWDS;
extern void func_80077544(), func_800A915C(), func_800777DC(), func_800A4748();
extern void func_800A476C(), func_800A5884(), func_80070488();
extern void func_801E7378(s32 value);
extern void func_800A5600(), func_80039C4C(), func_800399D4(), func_800A24C4();
extern void func_8001B66C(), func_80085B20(), func_80085EEC(), func_800A31E8();
extern void func_800A91F0(), func_80077DAC(), func_80078B5C(), func_8007554C();

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/field/nonmatchings/main/misc4", func_80078D44);
#else
void func_80078D44(void) {
    RECT rect;
    s32 s0, s1, s2;

    func_80077544();
    func_800A915C();
    ArchiveSetIndex(4, 0);
    func_800777DC();              /* load the map file into D_8005A4E0 */
    FieldRenderSync();
    if (D_8004F2F8 == 0) {
        FieldImageConvert24BitTo15Bit(0);
    }
    FieldInitializeRenderContexts();
    s0 = 1;
    if (D_8004F2F8 == 0) {
        rect.x = 0; rect.y = 0x100; rect.w = 0x140; rect.h = 0xE0;
        MoveImage(&rect, 0, 0);
        s0 = 1;
    }
    g_FieldCurRenderContextIndex = s0;
    func_800A4748();
    func_800A476C(0, 0x100);
    DrawSync(0);
    FieldClearAndSwapOTag();
    FieldRenderSync();
    if (D_800594D0 == 1) {
        func_800A5884(0, 0);
    } else if (D_8005942C != 1) {
        func_800A5884(1, 1);
    } else {
        func_800A5884(0, 0);
    }
    D_8004F2F8 = 1;
    ArchiveCdDataSync(0);
    ArchiveSetIndex(4, 0);
    FieldLoad();                  /* parse map -> allocates g_FieldActors */
    func_80070488();
    D_800AFD04 = 1;
    if (D_800B2264 != 0) {
        func_801E7378(1);
    }
    if (D_800594D0 == 1) {
        s2 = 0;
    } else if (D_8005942C != 1) {
        s2 = 0x20;
    } else {
        s2 = 0;
    }

    s0 = 0x800000;
    if (D_800ADB60 == 1) {
        do {
            FieldClearAndSwapOTag();
            FieldZoomFadeEffectUpdate();
            FieldDisplay();
            if (D_8005942C == 1) {
                func_800A5600(s0 >> 16);
                s0 -= 0x40000;
                if (s0 < 0) s0 = 0;
            } else if (D_800C2684 < 0x22C0) {
                D_800C2684 += s2;
            }
        } while (ArchiveDataSync() != 0);
        DrawSync(0);
        HeapFree(D_800ADC14);
        D_800ADB60 = 0;
        func_80078C5C();
    }

    if (D_8005942C == 1) {
        do {
            FieldClearAndSwapOTag();
            FieldZoomFadeEffectUpdate();
            FieldDisplay();
            func_800A5600(s0 >> 16);
            s0 -= 0x40000;
        } while (s0 >= 0);
    }

    if (D_800594D0 == 1) {
        rect.x = 0; rect.y = 0; rect.w = 0x140; rect.h = 0xE0;
        MoveImage(&rect, 0x200, 0);
    }
    if (D_8004F304 != 0) {
        func_80039C4C(D_80062528);
        func_800399D4(D_80062528);
        SoundFreeWdsEntry(g_GameCurLoadedWDS);
        D_8004F304 = 0;
    }
    func_800A24C4();
    D_8004F310 = 0;
    g_GamePartySkinsInitialized = 0;
    FieldRenderSync();
    ControllerResetState();
    D_8004F308 = 0;
    if (D_800594D0 == 1) {
        D_8004F324 = 0xE;
        func_80085EEC();
    }
    if (D_8004F338 != D_8004F324) {
        func_8001B66C();
        D_8004F308 = -1;
        if (D_8004F2FC != 0) {
            D_8004F348 = 1;
        }
        func_80085B20(D_8004F324, 1);
    } else {
        func_80085EEC();
    }
    func_800A31E8();

    if (D_800594D0 == 1) {
        s1 = 0;
        do {
            func_80077DAC();
            s1++;
            FieldZoomFadeEffectUpdate();
            func_8007554C();
            func_80078B5C();
        } while (s1 < 8);
    } else if (D_8005942C == 1) {
        FieldFadeToBlack(0x20);
    } else {
        FieldFadeToBlack(0x20);
        s0 = 0x800000;
        s1 = 0;
        do {
            func_80077DAC();
            FieldZoomFadeEffectUpdate();
            func_8007554C();
            func_80078B5C();
            if (D_800594D0 != 1) {
                func_800A5600(s0 >> 16);
                s0 -= 0x40000;
                if (s0 < 0) s0 = 0;
                if (D_800C2684 < 0x22C0) {
                    D_800C2684 += s2;
                }
            }
            s1++;
        } while (s1 < 0x20);
    }

    if (D_800B2264 != 0) {
        func_801E7378(0);
    }
    func_800A91F0();
    HeapConsolidate();
    FontFree();
    func_80077544();
    D_800AFD04 = 0;
}
#endif /* XENO_PC_PORT */

/* func_80079288 -- per-step random-encounter roll. Decompiled from
 * asm/field/nonmatchings/main/misc4/func_80079288.s (was INCLUDE_ASM ->
 * stubbed, so encounters never rolled). Called each field step a direction is
 * held (misc6.c:659). Guards -> step-counter decrement (refresh via
 * func_8008E718 at 0) -> age cooldown timers -> on an expired timer, weighted
 * roll over the 16 formation weights (D_80065ADC) -> commit + battle handoff.
 * See docs/ai_context/ACTIVE_HANDOFF.md. */
extern s32 D_8004F308, D_8004F370;
extern s32 D_800ADBDC, D_800ADBE4, D_800ADBEC, D_800ADB2C, D_800ADBD0;
extern u8  D_800ADB04;
extern s32 D_800B2294, D_800B2298, D_800B229C;
extern s16 D_800B22A0[];
extern s16 D_800B2270[];
extern s16 D_800B2290;
extern u8  D_80065ADC[16];        /* per-formation encounter weights (main-exe BSS) */
extern u8  D_80059508, D_800594F8;
extern void  func_8008E718(void);
extern void  func_80281204(s32);
extern void *LoadGameStateOverlay(unsigned int);

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/field/nonmatchings/main/misc4", func_80079288);
#else
void func_80079288(void) {
    u16 *timers = (u16 *)D_800B22A0;
    s32  cumulative[16];
    s32  i, sum, acc, roll, hit, selected;

    if (D_800ADBDC == 0) return;
    if (D_800ADBE4 == 0) return;
    if (D_800ADBEC == 0) return;
    if (D_8004F308 == -1) return;
    if (D_800B2298 == 0) return;
    if (*(s16 *)&g_FieldControl == -1) return;
    if (D_800ADB2C == 1) return;
    if (D_800ADB04 == 0) return;

    D_800B2294 -= 1;
    if (D_800B2294 == 0) {
        func_8008E718();
    }

    if (D_800B229C > 0) {
        for (i = 0; i < D_800B229C; i++) {
            if (timers[i] != 0xFFFF) {
                timers[i] = (u16)(timers[i] - 1);
            }
        }
    }

    if (D_800B229C <= 0) return;
    hit = -1;
    for (i = 0; i < D_800B229C; i++) {
        if (timers[i] == 0) { hit = i; break; }
    }
    if (hit < 0) return;
    timers[hit] = 0xFFFF;

    sum = 0;
    for (i = 0; i < 16; i++) {
        sum += D_80065ADC[i];
    }
    acc = 0;
    for (i = 0; i < 16; i++) {
        cumulative[i] = acc;
        acc += D_80065ADC[i];
    }

    roll = (rand() * (sum + 1)) >> 15;

    selected = -1;
    for (i = 15; i >= 0; i--) {
        if (D_80065ADC[i] != 0 && cumulative[i] < roll) {
            selected = i;
            break;
        }
    }
    if (selected < 0) return;

    D_80059508 = (u8)selected;
    D_800594F8 = 0;
    D_800B2290 = D_800B2270[selected];
    if (D_8004F370 == 0) {
        LoadGameStateOverlay(2);
    }
    D_800ADBDC = 0;
    D_800ADBD0 = 1;
    if (g_FieldSystemMode == 0) {
        func_80281204(selected);
    }
}
#endif /* XENO_PC_PORT */

/* Field-state exit dispatcher. */
extern s16 D_8004F384;
extern s32 D_800AFC78, D_800B0064;
extern void* g_pGameState;
extern void func_80085FB8(void);
extern int func_80085F30(void);
extern void func_8001BB50(void);
extern void ChangeGameState(unsigned int state);
extern void MainLoop(int errorCode) __attribute__((noreturn));

void func_8007954C(s32 exitCode) {
    D_8005942C = 0;

    switch (exitCode) {
    case 0: {
        void* gameState;
        s32 skin;
        s32 pending;
        u16 priorSkin;

        func_800A30FC();
        gameState = g_pGameState;
        skin = D_800AFC78;
        pending = D_8004F370;
        priorSkin = *(u16*)((u8*)gameState + 0x1932);
        D_8004F324 = skin;
        *(u16*)((u8*)gameState + 0x2322) = (u16)skin;
        *(u16*)((u8*)gameState + 0x2320) = priorSkin;
        if (pending != 0) {
            return;
        }
        ChangeGameState(2);
        break;
    }

    case 1:
        if (D_8004F384 == 1) {
            func_8001B66C();
            func_80085FB8();
            ArchiveCdDataSync(0);
            func_80085F30();
            func_8001B66C();
        }
        if (D_8004F370 != 0) {
            return;
        }
        ChangeGameState(3);
        break;

    case 2: {
        void* gameState = g_pGameState;
#ifdef XENO_PC_PORT
        s32 skin = D_8004F324;
#else
        register s32 skin __asm__("$3") = D_8004F324;
#endif
        s32 pending = D_8004F370;
        u16 priorSkin = *(u16*)((u8*)gameState + 0x1932);

        *(u16*)((u8*)gameState + 0x2322) = (u16)skin;
        *(u16*)((u8*)gameState + 0x2320) = priorSkin;
        if (pending != 0) {
            return;
        }
        ChangeGameState(4);
        g_GamePartySkinsInitialized++;
        break;
    }

    case 3:
        D_8004F310 = 0;
        g_GamePartySkinsInitialized = 0;
        if (D_8004F370 != 0) {
            return;
        }
        if (D_800B0064 & 0x80) {
            func_8001BB50();
        }
        ChangeGameState(D_800B0064 & 0x7F);
        break;

    default:
        break;
    }

    MainLoop(0);
}
void func_800796F4(void) {}

void FieldSwapRenderContext(void) {
    g_FieldCurRenderContextIndex = (g_FieldCurRenderContextIndex + 1) % 2;
    g_FieldCurRenderContext = &g_FieldRenderContexts[g_FieldCurRenderContextIndex];
    PutDispEnv(&g_FieldCurRenderContext->dispEnv);
    PutDrawEnv(&g_FieldCurRenderContext->drawEnvs[0]);
}

extern RECT D_800AFE4C;
extern TILE D_800AFE54[];
extern DR_MODE D_800AFE24[];
	
void func_80079784(int color) {
    FieldClearAndSwapOTag();
    setRGB(D_800AFE54[g_FieldCurRenderContextIndex], color * 4);
    addPrim(g_FieldCurRenderContext->ot1, &D_800AFE54[g_FieldCurRenderContextIndex]);
    addPrim(g_FieldCurRenderContext->ot1, &D_800AFE24[g_FieldCurRenderContextIndex]);
    FieldRenderSync();
    MoveImage(&D_800AFE4C, 0, g_FieldCurRenderContextIndex * 0x100);
    PutDispEnv(&g_FieldCurRenderContext->dispEnv);
    PutDrawEnv(&g_FieldCurRenderContext->drawEnvs[0]);
    DrawOTag(g_FieldCurRenderContext->ot1 + 1);
}


/* Resolve the field movement gate from the active player actor's +0x14 flags,
 * then apply the signed-halfword script override when it is not 0x00FF.
 * Transcribed from asm/field/nonmatchings/main/misc4/func_800798BC.s
 * (0x800798BC-0x80079958). Retail control flow: beqz D_800B2268 -> store 1;
 * else andi flags+0x14,0xC0 / bnez store 1 else store 0; then lh D_800B234C
 * and sb override when != 0xFF. */
extern s32 D_800B2268;
extern s16 D_800B234C;
extern u8 D_80059179;
extern s32 g_PlayerActorIndex;

void func_800798BC(void) {
    if (D_800B2268 != 0) {
        u32 flags = *(u32*)((u8*)(uintptr_t)g_FieldActors[g_PlayerActorIndex].pActorData
                            + 0x14);
#ifdef FIELD_MOVEGATE_MUTANT_C0_MASK
        if (flags & 0x40) {
#else
        if (flags & 0xC0) {
#endif
            D_80059179 = 1;
        } else {
            D_80059179 = 0;
        }
    } else {
        D_80059179 = 1;
    }

    if (D_800B234C != 0xFF) {
        D_80059179 = D_800B234C;
    }
}

void func_8007995C(short w, short h, short x, short y, int destX, int destY) {
    RECT rect;

    rect.w = w;
    rect.h = h;
    rect.x = x;
    rect.y = y;
    MoveImage(&rect, destX, destY);
    DrawSync(0);
}

void FieldRenderSyncAndFlush(void) {
    FieldRenderSync();
    EnterCriticalSection();
    FlushCache();
    ExitCriticalSection();
}

/* VRAM coordinate pairs for the party-sprite save/restore blit tables. */
typedef struct {
    u16 x;
    u16 y;
} FieldVramCoord;

extern FieldVramCoord D_800ADCB0[]; /* field-side sprite VRAM rects */
extern FieldVramCoord D_800ADCC8[]; /* menu-backup VRAM rects */
extern s16 D_8006BE2C[];            /* party-member-present flags snapshot */
extern TILE D_800AFE64;             /* fade tile, render context 1 */
extern void* D_8005945C;            /* menu shared-resource buffer */
extern void* D_800ADB20;            /* saved 0x6B9 archive buffer */
extern void* D_800ADB30;            /* menu-arg source buffer */
extern void* D_8005A4AC;            /* &g_FieldRenderContexts + 0xCC */
extern void* D_8005A4B0;            /* &g_FieldRenderContexts + 0x81C0 */
extern u8 D_80059460;               /* menuToEnter = request & 0x7F */
extern u8 D_800B02C8;
extern u8 D_800ADB05;
extern s32 D_800AFE84, D_800ADB50, D_800ADB60, D_800ADBEC;
extern s32 D_80050100, D_8004F320, D_8004F31C, D_8004F350;
extern s32 D_800ADB64;
extern s32 g_GameSceneMapNum;
extern s32 g_GamePartyMemberSkins[];
extern RenderContext g_FieldRenderContexts[2];
extern void* g_pGameState;
extern u8 D_800B21D0[];
extern u8 g_MenuDebugEnabled;
extern void* g_PartyDataBuffers[];

extern void func_800798BC(void);
extern void func_800A2488(void);
extern void func_80070488(void);
extern void func_80070508(void);
extern void func_80077544(void);
extern void FieldPartyFreeSkinDataBuffers(void);
extern void FieldPartyAllocateSkinDataBuffers(void);
extern void GamePartySyncSkinData(void);
extern void GamePartySyncStreamedData(void);
extern void FontFree(void);
extern void FieldSwapRenderContext(void);
extern void MenuMain(void);
extern void FieldScriptMemoryWriteU16(int index, int value);
extern int func_80029AFC(StreamDataQueueEntry* pEntries, int arg1, int arg2);
extern u32 LZSSDecompress(void* src, void* dst);

#ifdef XENO_PC_PORT
/* DIAGNOSTIC / TEST TOOLING (XENO_MENU_PARTY_DIAG=1).  The field menu opener
 * frees and reallocates the party skin buffers around MenuMain.  On PSX the
 * heap is deterministic, so the reallocation lands on the same addresses and
 * any pointer an actor still holds into them keeps working; on the host it need
 * not.  Dump g_PartyDataBuffers[], g_GamePartyMemberSkins[], and every field
 * actor's pSpriteData (+0x4) / pActorData (+0x4C) at each seam so a retained
 * pre-free pointer is visible rather than inferred.  Inert unless armed. */
extern s32 D_800ADBFC;      /* live actor count (party + scripted actors) */
extern s32 g_FieldNumActors;
extern s32 g_PlayerActorIndex;

static int PcPort_MenuPartyDiagArmed(void)
{
    static int armed = -1;
    if (armed < 0) {
        const char* e = getenv("XENO_MENU_PARTY_DIAG");
        armed = (e && e[0] == '1') ? 1 : 0;
    }
    return armed;
}

/* DIAGNOSTIC / TEST TOOLING (XENO_MENU_PARTY_DIAG=1).  Second half of the same
 * question: the opener blits the 6 party-sprite VRAM rects out to the backup
 * table before MenuMain and back afterwards.  The C is symmetric, but that only
 * proves the *calls* pair up -- if the host MoveImage loses the pixels, the
 * party textures come back as 0x0000 texels, which the PSX blender treats as
 * fully transparent, i.e. the sprite draws its quads and shows nothing.  Digest
 * both rect tables straight out of the VRAM mirror so that case is measured
 * rather than assumed. */
static void PcPort_MenuVramDigest(const char* tag)
{
    extern unsigned short vram[];
    enum { VRAM_W = 1024, RECT_W = 0x40, RECT_H = 0x20 };
    s32 r;

    if (!PcPort_MenuPartyDiagArmed())
        return;
    /* Retail party-page coordinates spelled out, INDEPENDENT of the migrated
     * D_800ADCB0 (asm/field/data/3DF78.data.s @ 0x3E1C0).  The table-driven
     * digest below reports whatever the build actually blits; this probe always
     * reports the real party texture pages, so a build whose table is stubbed
     * to zero can still be measured at the same place as a build whose table is
     * correct.  That is what makes the before/after comparison an A/B. */
    {
        static const FieldVramCoord kRetailPartyPages[6] = {
            { 0x0000, 0x00E0 }, { 0x0040, 0x00E0 }, { 0x0080, 0x00E0 },
            { 0x00C0, 0x00E0 }, { 0x0100, 0x00E0 }, { 0x0100, 0x01E0 },
        };

        for (r = 0; r < 6; r++) {
            unsigned int h = 2166136261u;
            int nonZero = 0;
            s32 y;

            for (y = 0; y < RECT_H; y++) {
                s32 x;
                for (x = 0; x < RECT_W; x++) {
                    unsigned short p =
                        vram[(kRetailPartyPages[r].y + y) * VRAM_W +
                             kRetailPartyPages[r].x + x];
                    h ^= p;
                    h *= 16777619u;
                    if (p != 0)
                        nonZero++;
                }
            }
            printf("[xeno-port][vram-probe] %-11s page[%d] (%u,%u) fnv=%08x "
                   "nonzero=%d/%d\n",
                   tag, (int)r, (unsigned)kRetailPartyPages[r].x,
                   (unsigned)kRetailPartyPages[r].y, h, nonZero,
                   RECT_W * RECT_H);
        }
    }

    for (r = 0; r < 6; r++) {
        const FieldVramCoord* pRects[2];
        const char* names[2];
        s32 t;

        pRects[0] = &D_800ADCB0[r];
        pRects[1] = &D_800ADCC8[r];
        names[0] = "field";
        names[1] = "backup";
        for (t = 0; t < 2; t++) {
            unsigned int h = 2166136261u;
            int nonZero = 0;
            s32 y;

            for (y = 0; y < RECT_H; y++) {
                s32 x;
                for (x = 0; x < RECT_W; x++) {
                    unsigned short p =
                        vram[(pRects[t]->y + y) * VRAM_W + pRects[t]->x + x];
                    h ^= p;
                    h *= 16777619u;
                    if (p != 0)
                        nonZero++;
                }
            }
            printf("[xeno-port][vram-diag] %-11s %s[%d] (%u,%u) fnv=%08x "
                   "nonzero=%d/%d\n",
                   tag, names[t], (int)r, (unsigned)pRects[t]->x,
                   (unsigned)pRects[t]->y, h, nonZero, RECT_W * RECT_H);
        }
    }
    fflush(stdout);
}

static void PcPort_MenuPartyDiag(const char* tag)
{
    s32 i;
    s32 count;

    if (!PcPort_MenuPartyDiagArmed())
        return;
    printf("[xeno-port][party-diag] %-10s buffers=%p/%p/%p skins=%d/%d/%d "
           "numActors=%d liveActors=%d player=%d\n",
           tag, g_PartyDataBuffers[0], g_PartyDataBuffers[1],
           g_PartyDataBuffers[2], (int)g_GamePartyMemberSkins[0],
           (int)g_GamePartyMemberSkins[1], (int)g_GamePartyMemberSkins[2],
           (int)g_FieldNumActors, (int)D_800ADBFC, (int)g_PlayerActorIndex);
    count = D_800ADBFC;
    if (count > 8)
        count = 8;
    for (i = 0; i < count; i++) {
        u8* actor = (u8*)g_FieldActors + i * 0x5C;
        u32 sprite = *(u32*)(actor + 0x4);
        u32 data = *(u32*)(actor + 0x4C);
        u32 skinId = 0xFFFFFFFFu;
        if (data != 0)
            skinId = *(u8*)((u8*)(uintptr_t)data + 0x126);
        printf("[xeno-port][party-diag] %-10s actor[%d] pSpriteData=%08x "
               "pActorData=%08x skinId=%02x status=%04x\n",
               tag, (int)i, (unsigned)sprite, (unsigned)data,
               (unsigned)skinId, (unsigned)*(u16*)(actor + 0x58));
    }
    fflush(stdout);
}
#endif

/* Transcribed from asm/field/nonmatchings/main/misc4/func_800799D4.s
 * (0x800799D4-0x8007A448). Early-out if request==0x80 and D_800B21D0!=0.
 * Overlay HeapAlloc uses ArchiveDecodeAlignedSize((id+5)&0x7F) when
 * D_8004F370==1, else (D_800ADB30&0xFFFFFF)+0xFFE3AFF8. Restore 0x6B9 uses
 * the same D_800ADB30 word +0xFFE23FF8 when D_8004F370==0. */
#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/field/nonmatchings/main/misc4", func_800799D4);
#else
void func_800799D4(void) {
    FieldVramCoord* pFieldRects;
    FieldVramCoord* pBackupRects;
    void* pMenuArchiveBackup = NULL;
    void* pOverlayBuf;
    void* pSaveTop;
    void* pSaveBottom;
    StreamDataQueueEntry menuLoadCommands[4];
    void* savedMenuArgSrc;
    RECT rect;
    s32 i;
    s32 menuId;
    s16* pPresentFlags;

    if (D_800ADB64 == 0x80 && D_800B21D0[0] != 0) {
        return;
    }

    /* Full-screen semi-transparent black fade tile, duplicated per context. */
    setTile(&D_800AFE54[0]);
    SetSemiTrans(&D_800AFE54[0], 1);
    D_800AFE54[0].w = 0x140;
    D_800AFE54[0].b0 = 0;
    D_800AFE54[0].g0 = 0;
    D_800AFE54[0].r0 = 0;
    D_800AFE54[0].y0 = 0;
    D_800AFE54[0].x0 = 0;
    D_800AFE54[0].h = 0xE0;
    D_800AFE64 = D_800AFE54[0];

    SetDrawMode(&D_800AFE24[0], 0, 0, GetTPage(0, 2, 0, 0), 0);
    SetDrawMode(&D_800AFE24[1], 0, 0, GetTPage(0, 2, 0, 0), 0);
#ifdef XENO_PC_PORT
    PcPort_MenuPartyDiag("pre-free");
#endif
    FieldPartyFreeSkinDataBuffers();
#ifdef XENO_PC_PORT
    PcPort_MenuPartyDiag("post-free");
#endif

    /* Preserve the streamed 0x6B9 field archive across the overlay load. */
    if (D_800B2264 != 0) {
        ArchiveSetIndex(0x4, 0);
        pMenuArchiveBackup = HeapAlloc(ArchiveDecodeAlignedSize(0x6B9), 0);
        memcpy(pMenuArchiveBackup, D_800ADB20, ArchiveDecodeAlignedSize(0x6B9));
        HeapFree(D_800ADB20);
    }

    savedMenuArgSrc = D_800ADB30;
    ArchiveSetIndex(0x10, 0);
#ifdef XENO_PC_PORT
    /* Retail's else-branch sizes this buffer as the fixed-map gap
     * [0x801C5008, D_800ADB30): the menu overlay must land at 0x801C5000 on
     * PSX, and D_800ADB30 is the next allocation above it.  On the host heap
     * D_800ADB30 is a host pointer, so that subtraction is meaningless (it
     * produced a ~5MB bogus size -> HeapAlloc failure -> GameHandleError(130)
     * in the map005 repro).  Size the buffer by what is actually loaded into
     * it: menuLoadCommands[1] streams archive file (menu id + 5) here, so use
     * that file's decoded size -- the same derivation retail's own
     * D_8004F370 branch and func_80077884's TOC branch (main.c:171) use.
     * (void)savedMenuArgSrc: keep the retail read; only the sizing differs. */
    (void)savedMenuArgSrc;
    pOverlayBuf = HeapAlloc(ArchiveDecodeAlignedSize((D_800ADB64 + 5) & 0x7F), 1);
#else
    if (D_8004F370 == 1) {
        pOverlayBuf = HeapAlloc(ArchiveDecodeAlignedSize((D_800ADB64 + 5) & 0x7F), 1);
    } else {
        pOverlayBuf = HeapAlloc(
            ((u32)savedMenuArgSrc & 0xFFFFFF) + 0xFFE3AFF8, 1);
    }
#endif

    menuLoadCommands[0].archiveIndex = 1;
    menuLoadCommands[2].archiveIndex = 0;
    menuLoadCommands[2].pData = NULL;
    menuLoadCommands[3].archiveIndex = 0;
    menuLoadCommands[3].pData = NULL;
    menuLoadCommands[0].pData = HeapAlloc(ArchiveDecodeAlignedSize(1), 1);
    D_8005945C = menuLoadCommands[0].pData;
    menuLoadCommands[1].pData = pOverlayBuf;
    menuId = D_800ADB64 & 0x7F;
    menuLoadCommands[1].archiveIndex = menuId + 5;
    if (menuId == 5 && D_8004F370 == 0) {
        menuLoadCommands[2].archiveIndex = 0xC;
        menuLoadCommands[2].pData = (void*)0x1DC000;
    }

    ArchiveCdDataSync(0);
    func_80029AFC(menuLoadCommands, 0, 0);
    ArchiveSetIndex(0x4, 0);

    /* Back up the field party-sprite VRAM rects, then two fixed screen rects. */
#ifdef XENO_PC_PORT
    PcPort_MenuVramDigest("pre-backup");
#endif
    pFieldRects = D_800ADCB0;
    pBackupRects = D_800ADCC8;
    rect.w = 0x40;
    rect.h = 0x20;
    for (i = 0; i < 6; i++) {
        rect.x = pFieldRects->x;
        rect.y = pFieldRects->y;
        MoveImage(&rect, pBackupRects->x, pBackupRects->y);
        DrawSync(0);
        pFieldRects++;
        pBackupRects++;
    }
#ifdef XENO_PC_PORT
    PcPort_MenuVramDigest("post-backup");
#endif
    func_8007995C(0x40, 0x100, 0x3C0, 0x100, 0x300, 0);
    func_8007995C(0x40, 0x100, 0x2C0, 0x100, 0x280, 0);

    D_800AFE4C.x = 0x2C0;
    D_800AFE4C.y = 0x100;
    D_800AFE4C.w = 0x140;
    D_800AFE4C.h = 0xE0;
    func_800A4748();
    MoveImage(&D_800AFE4C, 0, 0x100);
    DrawSync(0);

    for (i = 0; i < 0x20; i++) {
        func_80079784(i);
    }
    FieldRenderSync();
    D_800AFE4C.x = 0;
    D_800AFE4C.y = 0;
    FieldSwapRenderContext();
    FontFree();
    MoveImage(&D_800AFE4C, 0, 0xE0);
    FieldRenderSync();
    ArchiveCdDataSync(0);

    D_800594D0 = 0;
    g_MenuDebugEnabled = 0;
#ifndef MENU_MUTANT_NEVER_REQUEST
    D_80059460 = D_800ADB64 & 0x7F;
#endif
    pPresentFlags = D_8006BE2C;
    for (i = 0; i < 3; i++) {
        pPresentFlags[i] = ((u8*)g_pGameState)[0x22B1 + i];
    }
    func_800798BC();
    D_8005A4AC = (u8*)g_FieldRenderContexts + 0xCC;
    D_8005A4B0 = (u8*)g_FieldRenderContexts + 0x81C0;
    FieldRenderSyncAndFlush();

#ifdef XENO_PC_PORT
    printf("[xeno-port][menu] func_800799D4 request=%d D_80059460=%d -> MenuMain\n",
           (int)D_800ADB64, (int)D_80059460);
    fflush(stdout);
    /* TEST TOOLING: only the field system menu (ADB64=0x80). Title/other
     * requests must not arm PadOnControl Equip nav. */
    if (D_800ADB64 == 0x80) {
        extern int g_PcPortFieldMenuOpened;
        g_PcPortFieldMenuOpened = 1;
    }
#endif
#ifndef MENU_MUTANT_SKIP_MENUMAIN
    MenuMain();
#endif
#ifdef XENO_PC_PORT
    printf("[xeno-port][menu] MenuMain returned D_800594D0=%d\n", (int)D_800594D0);
    fflush(stdout);
#endif

    FieldRenderSyncAndFlush();
    D_80050100 = 2;

    /* Post-menu state fixups keyed on the menu return state D_800594D0. */
    if (D_800594D0 == 0 && (D_800ADB64 & 0x7F) == 2) {
        D_800B02C8 = 1;
        FieldScriptMemoryWriteU16(0x46, 0);
        FieldScriptMemoryWriteU16(4, 4);
        g_GameSceneMapNum = 4;
        *(s16*)((u8*)g_pGameState + 0x2320) = 0;
        *(s16*)((u8*)g_pGameState + 0x1932) = 0;
        *(s16*)((u8*)g_pGameState + 0x231A) = 4;
    }
    if (D_800594D0 == 2) {
        D_800B02C8 = 1;
        FieldScriptMemoryWriteU16(0x46, 2);
        FieldScriptMemoryWriteU16(4, *(u16*)((u8*)g_pGameState + 0x231A) & 0x3FFF);
        if ((*(u16*)((u8*)g_pGameState + 0x231A) & 0x3FFF) < 0x400) {
            *(s16*)((u8*)g_pGameState + 0x2320) =
                *(u16*)((u8*)g_pGameState + 0x1984);
        }
    }

    /* Restore the fade rects and back up the two menu screen halves. */
    FieldRenderSync();
    D_800AFE4C.x = 0;
    D_800AFE4C.y = 0xE0;
    D_800AFE4C.w = 0x140;
    D_800AFE4C.h = 0xE0;
    MoveImage(&D_800AFE4C, 0, 0);
    FieldRenderSync();
    D_800AFE4C.x = 0x140;
    D_800AFE4C.y = 0;
    MoveImage(&D_800AFE4C, 0, 0);
    MoveImage(&D_800AFE4C, 0, 0x100);
    FieldRenderSync();

    PutDispEnv(&g_FieldCurRenderContext->dispEnv);
    PutDrawEnv(&g_FieldCurRenderContext->drawEnvs[0]);
    D_800AFE4C.x = 0x2C0;
    D_800AFE4C.y = 0x100;
    rect.x = 0x300;
    rect.y = 0;
    rect.w = 0x40;
    rect.h = 0x100;
    pSaveTop = HeapAlloc(0x8000, 1);
    StoreImage(&rect, (u_long*)pSaveTop);
    DrawSync(0);
    rect.x = 0x280;
    rect.y = 0;
    rect.w = 0x40;
    rect.h = 0x100;
    pSaveBottom = HeapAlloc(0x8000, 1);
    StoreImage(&rect, (u_long*)pSaveBottom);
    DrawSync(0);

#ifdef XENO_PC_PORT
    PcPort_MenuVramDigest("pre-restore");
#endif
    pFieldRects = D_800ADCC8;
    pBackupRects = D_800ADCB0;
    for (i = 0; i < 6; i++) {
        rect.w = 0x40;
        rect.h = 0x20;
        rect.x = pFieldRects->x;
        rect.y = pFieldRects->y;
        MoveImage(&rect, pBackupRects->x, pBackupRects->y);
        DrawSync(0);
        pFieldRects++;
        pBackupRects++;
    }
#ifdef XENO_PC_PORT
    PcPort_MenuVramDigest("post-restore");
#endif

    ArchiveSetIndex(0x4, 0);
    HeapChangeCurrentUser(0x8, NULL);
    D_800ADB60 = 0;
    func_80070488();

    if (D_800AFE84 != 0) {
        for (i = 0x20; i < 0x3F; i++) {
            func_80079784(i);
        }
        D_800ADB50 = 1;
    } else {
        for (i = 0x1F; i >= 0; i--) {
            func_80079784(i);
        }
        func_80079784(0);
        D_800ADB50 = 0;
    }
    func_80070508();

    FieldRenderSync();
    rect.x = 0x2C0;
    /* Retail 8007A1FC restores the saved field texture strips at y=0x100. */
    rect.y = 0x100;
    rect.w = 0x40;
    rect.h = 0x100;
    LoadImage(&rect, (u_long*)pSaveBottom);
    DrawSync(0);
    rect.x = 0x3C0;
    LoadImage(&rect, (u_long*)pSaveTop);
    DrawSync(0);
    HeapFree(pSaveBottom);
    HeapFree(pSaveTop);
    HeapFree(pOverlayBuf);

    ArchiveSetIndex(0x4, 0);
    if (D_800B2264 != 0) {
#ifdef XENO_PC_PORT
        /* Same fixed-map idiom as the overlay alloc above: retail sizes the
         * restored 0x6B9 buffer as [0x801DC008, pMenuArchiveBackup), invalid
         * against a host pointer.  The buffer holds the decoded 0x6B9 archive
         * (memcpy'd back below), so use its decoded size -- identical to
         * retail's own D_8004F370 branch and main.c:171. Latent at cold boot
         * (D_800B2264 == 0 in the repro) but the same crash class once maps
         * with streamed archives open a menu. */
        D_800ADB20 = HeapAlloc(ArchiveDecodeAlignedSize(0x6B9), 1);
#else
        if (D_8004F370 == 0) {
            D_800ADB20 = HeapAlloc(
                ((u32)savedMenuArgSrc & 0xFFFFFF) + 0xFFE23FF8, 1);
        } else {
            D_800ADB20 = HeapAlloc(ArchiveDecodeAlignedSize(0x6B9), 1);
        }
#endif
        memcpy(D_800ADB20, pMenuArchiveBackup, ArchiveDecodeAlignedSize(0x6B9));
        HeapFree(pMenuArchiveBackup);
    }

    SetGeomOffset(0xA0, 0x70);
    SetGeomScreen(g_Scene.sceneScrZ);
    FieldPartyAllocateSkinDataBuffers();
#ifdef XENO_PC_PORT
    PcPort_MenuPartyDiag("post-alloc");
#endif

    if (D_800ADB64 == 1) {
        g_GamePartyMemberSkins[0] = 0xFF;
        g_GamePartyMemberSkins[1] = 0xFF;
        g_GamePartyMemberSkins[2] = 0xFF;
        D_8004F320 = 0;
        D_8004F31C = 0;
        GamePartySyncSkinData();
        GamePartySyncStreamedData();
        FieldRenderSync();
        D_800ADBEC = 0;
        D_800ADB05 = 1;
    } else {
        for (i = 0; i < 3; i++) {
            s32 skin = g_GamePartyMemberSkins[i];
            if (skin != 0xFF) {
                void* pBuf = HeapAlloc(ArchiveDecodeAlignedSize(skin + 5), 1);
                ArchiveReadFileToBuffer(skin + 5, pBuf, 0, 0x80);
                ArchiveCdDataSync(0);
                LZSSDecompress(pBuf, g_PartyDataBuffers[i]);
                HeapFree(pBuf);
            }
        }
#ifdef XENO_PC_PORT
        PcPort_MenuPartyDiag("post-lzss");
#endif
        func_800A2488();
#ifdef XENO_PC_PORT
        PcPort_MenuPartyDiag("post-2488");
#endif
        FieldRenderSync();
    }

    D_800ADB64 = 0xFF;
    func_80077544();
#ifndef MENU_MUTANT_SKIP_WAIT_CLEAR
    D_8004F350 = 0;
#endif
#ifdef XENO_PC_PORT
    printf("[xeno-port][menu] WAIT_MENU D_8004F350=%d\n", (int)D_8004F350);
    fflush(stdout);
#endif
}
#endif /* XENO_PC_PORT */


// Quad rendering
// --------------------
void FieldClampPolyFT4UVs(POLY_FT4* poly, short u0, short v0, short u1, short v1, short u2, short v2, short u3, short v3) {
    if (u0 < 0) u0 = 0;
    if (u1 < 0) u1 = 0;
    if (u2 < 0) u2 = 0;
    if (u3 < 0) u3 = 0;
    if (v0 < 0) v0 = 0;
    if (v1 < 0) v1 = 0;
    if (v2 < 0) v2 = 0;
    if (v3 < 0) v3 = 0;
    
    if (u0 >= 0x100) u0 = 0xFF;
    if (u1 >= 0x100) u1 = 0xFF;
    if (u2 >= 0x100) u2 = 0xFF;
    if (u3 >= 0x100) u3 = 0xFF;
    if (v0 >= 0x100) v0 = 0xFF;
    if (v1 >= 0x100) v1 = 0xFF;
    if (v2 >= 0x100) v2 = 0xFF;
    if (v3 >= 0x100) v3 = 0xFF;

    poly->u0 = u0;
    poly->v0 = v0;
    poly->u1 = u1;
    poly->v1 = v1;
    poly->u2 = u2;
    poly->v2 = v2;
    poly->u3 = u3;
    poly->v3 = v3;
}

extern u8 D_800B0FEC[];
extern s16 D_800ADE30[][8];
extern u8 D_800ADE70[][8];

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/field/nonmatchings/main/misc4", func_8007A5C4);
#else
void func_8007A5C4(void) {
    s32 i;

    for (i = 0; i < 4; i++) {
        Quad* pPart = (Quad*)(D_800B0FEC + (i * 0x70));
        POLY_FT4* pPoly = &pPart->polys[0];

        SetPolyFT4(pPoly);

        pPart->vertices[0].vx = D_800ADE30[i][0];
        pPart->vertices[0].vy = 0;
        pPart->vertices[0].vz = D_800ADE30[i][1];
        pPart->vertices[1].vx = D_800ADE30[i][2];
        pPart->vertices[1].vy = 0;
        pPart->vertices[1].vz = D_800ADE30[i][3];
        pPart->vertices[2].vx = D_800ADE30[i][4];
        pPart->vertices[2].vy = 0;
        pPart->vertices[2].vz = D_800ADE30[i][5];
        pPart->vertices[3].vx = D_800ADE30[i][6];
        pPart->vertices[3].vy = 0;
        pPart->vertices[3].vz = D_800ADE30[i][7];

        setRGB0(pPoly, 0x80, 0x80, 0x80);
        FieldClampPolyFT4UVs(pPoly,
                             D_800ADE70[i][0], D_800ADE70[i][1] + 0xC0,
                             D_800ADE70[i][2], D_800ADE70[i][3] + 0xC0,
                             D_800ADE70[i][4], D_800ADE70[i][5] + 0xC0,
                             D_800ADE70[i][6], D_800ADE70[i][7] + 0xC0);
        SetSemiTrans(pPoly, 1);
        pPoly->tpage = GetTPage(0, 2, 0x280, 0x1C0);
        pPoly->clut = GetClut(0x100, 0xF2);
        pPart->polys[1] = *pPoly;
    }
}
#endif /* XENO_PC_PORT */

extern SVec4 D_800ADCE0[]; // X0 -> X3 positions
extern SVec4 D_800ADD28[]; // Y0 -> Y3 positions
extern SVec4 D_800ADD70[]; // U0 -> U3s
extern SVec4 D_800ADDB8[]; // V0 -> V3s

// Texture / Clut stuff
extern short D_800ADE00[];
extern short D_800ADE02[];
extern short D_800ADE04[];
extern short D_800ADE06[];
extern short D_800ADE08[];
extern short D_800ADE0A[];

// Compass primitive initialization?
void func_8007A7F4(Quad* pPart, int x, int y, int tex) {
    int nIndexTex;
    POLY_FT4* pNextPoly;
    char _pad[0x88];

    pNextPoly = &pPart->polys[1]; 
    SetPolyFT4(&pPart->polys[0]);
    
    nIndexTex = tex * 0x6,

    // Set Position 0, 1, 2, and 3
    pPart->vertices[0].vx = D_800ADCE0[x].x;
    pPart->vertices[0].vy = 0x0;
    pPart->vertices[0].vz = D_800ADD28[y].x;

    pPart->vertices[1].vx = D_800ADCE0[x].y;
    pPart->vertices[1].vy = 0;
    pPart->vertices[1].vz = D_800ADD28[y].y;

    pPart->vertices[2].vx = D_800ADCE0[x].z;
    pPart->vertices[2].vy = 0;
    pPart->vertices[2].vz = D_800ADD28[y].z;

    pPart->vertices[3].vx = D_800ADCE0[x].w;
    pPart->vertices[3].vy = 0;
    pPart->vertices[3].vz = D_800ADD28[y].w;
    
    setRGB0(&pPart->polys[0], 0x80, 0x80, 0x80);
    
    pPart->polys[0].tpage = GetTPage(
        D_800ADE00[nIndexTex], 
        D_800ADE02[nIndexTex], 
        D_800ADE04[nIndexTex], 
        D_800ADE06[nIndexTex]
    );
    
    pPart->polys[0].clut = GetClut(D_800ADE08[nIndexTex], D_800ADE0A[nIndexTex]);
    
    FieldClampPolyFT4UVs(&pPart->polys[0], 
        D_800ADD70[x].x, D_800ADDB8[y].x, 
        D_800ADD70[x].y, D_800ADDB8[y].y, 
        D_800ADD70[x].z, D_800ADDB8[y].z, 
        D_800ADD70[x].w, D_800ADDB8[y].w
    );

    *pNextPoly = pPart->polys[0];
}

// Actor primitive initialization
void func_8007AA44(Quad* pQuad) {
    POLY_FT4* pPoly;
    POLY_FT4* pPoly2;

    pPoly = &pQuad->polys[0];
    pPoly2 = &pQuad->polys[1];
    
    SetPolyFT4(pPoly);
    pQuad->vertices[3].vx = -0x18;
    pQuad->vertices[3].vz = -0x18;
    pQuad->vertices[3].vy = 0;

    pQuad->vertices[2].vx = 0x18;
    pQuad->vertices[2].vy = 0;
    pQuad->vertices[2].vz = -0x18;

    pQuad->vertices[1].vx = -0x18;
    pQuad->vertices[1].vy = 0;
    pQuad->vertices[1].vz = 0x18;

    pQuad->vertices[0].vx = 0x18;
    pQuad->vertices[0].vy = 0;
    pQuad->vertices[0].vz = 0x18;

    setRGB0(pPoly, 0x80, 0x80, 0x80);
    pPoly->tpage = GetTPage(0, 2, 0x280, 0x1E0);
    pPoly->clut = GetClut(0x100, 0xF3);
    SetSemiTrans(pPoly, 1);

    pPoly->v0 = 0xE0;
    pPoly->v1 = 0xE0;
    pPoly->u0 = 0;
    pPoly->u1 = 0xF;
    pPoly->u2 = 0;
    pPoly->v2 = 0xEF;
    pPoly->u3 = 0xF;
    pPoly->v3 = 0xEF;
    
    *pPoly2 = *pPoly;
}

void FieldRenderQuad(u_long* ot, Quad* pQuad, MATRIX* pMatTransform, int renderContext) {
    long nInterpolation;
    long nFlag;
    POLY_FT4* pPoly;

    pPoly = &pQuad->polys[renderContext];
    PushMatrix();

    // Set our GTE transform matrix (RTM and TRV)
    SetRotMatrix(pMatTransform);
    SetTransMatrix(pMatTransform);

    // Transform our vertices to screen coordinates
    RotAverage4(
        &pQuad->vertices[0], &pQuad->vertices[1], &pQuad->vertices[2], &pQuad->vertices[3], 
        (long*)&pPoly->x0, (long*)&pPoly->x1, (long*)&pPoly->x2, (long*)&pPoly->x3, 
        &nInterpolation, &nFlag
    );

    // Queue our primitive for rendering
    addPrim(ot + 1, pPoly);

    PopMatrix();
}

void func_8007AC58(u_long* ot, Quad* pQuad, MATRIX* pMatTransform, int renderContext) {
    long nInterpolation;
    long nFlag;
    int nPosY;
    int nPosX1;
    POLY_FT4* pPoly;
    int nPosX2;

    pPoly = &pQuad->polys[renderContext];
    PushMatrix();

    // Set our GTE transform matrix (RTM and TRV)
    SetRotMatrix(pMatTransform);
    SetTransMatrix(pMatTransform);

    // Transform our vertices to screen coordinates
    RotAverage4(
        &pQuad->vertices[0], &pQuad->vertices[1], &pQuad->vertices[2], &pQuad->vertices[3], 
        (long*)&pPoly->x0, (long*)&pPoly->x1, (long*)&pPoly->x2, (long*)&pPoly->x3, 
        &nInterpolation, &nFlag
    );
    
    nPosX1 = (pPoly->x3 + pPoly->x2) / 2;
    nPosY = pPoly->y3;

    nPosX2 = nPosX1 + 8;
    nPosX1 = nPosX1 - 8;
    setXY4(pPoly,
        nPosX1, nPosY - 10,
        nPosX2, nPosY - 10,
        nPosX1, nPosY,
        nPosX2, nPosY
    );
    
    // Queue our primitive for rendering
    addPrim(ot + 1, pPoly);

    PopMatrix();
}


// Controller stuff
// --------------------
extern s32 g_pFieldControllerBuffer1;
extern s32 g_pFieldControllerBuffer2;

extern u_short g_FieldMouseSpeedX;
extern u_short g_FieldMouseSpeedY;
extern s32 D_800C3A44;
extern s32 D_800C3A4C;
extern s32 D_800C3A50;
extern s32 D_800C3A54;
extern int g_FieldMousePositionsX[2];
extern int g_FieldMousePositionsY[2];

void FieldSetControllerBuffers(void* controllerBuffer1, void* controllerBuffer2) {
    g_pFieldControllerBuffer1 = controllerBuffer1;
    g_pFieldControllerBuffer2 = controllerBuffer2;
}

void func_8007ADA4(int arg0, int arg1, int arg2, int arg3) {
    D_800C3A44 = arg0 * g_FieldMouseSpeedX;
    D_800C3A50 = arg1 * g_FieldMouseSpeedX;
    D_800C3A4C = arg2 * g_FieldMouseSpeedY;
    D_800C3A54 = arg3 * g_FieldMouseSpeedY;
}

void FieldSetMouseSpeed(u_short xSpeed, u_short ySpeed) {
    g_FieldMouseSpeedX = xSpeed;
    g_FieldMouseSpeedY = ySpeed;
}

void FieldSetMousePosition(int mouseIndex, int xMovement, int yMovement) {
    g_FieldMousePositionsX[mouseIndex] = xMovement * g_FieldMouseSpeedX;
    g_FieldMousePositionsY[mouseIndex] = yMovement * g_FieldMouseSpeedY;
}

extern void func_8007AF74(s32 mouseIndex);

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/field/nonmatchings/main/misc4", func_8007AE78);
#else
void func_8007AE78(s32 mouseIndex, void* out) {
    s32* pOut = out;
    u8* pController;

    func_8007AF74(mouseIndex);

    pOut[0] = g_FieldMousePositionsX[mouseIndex] / g_FieldMouseSpeedX;
    pOut[1] = g_FieldMousePositionsY[mouseIndex] / g_FieldMouseSpeedY;
    pOut[2] = -0x100;

    pController = (u8*)(uintptr_t)(mouseIndex ? g_pFieldControllerBuffer2 : g_pFieldControllerBuffer1);
    pOut[3] = (s8)pController[4];
    pOut[4] = (s8)pController[5];

    if ((s8)pController[0] == 0 && (s8)pController[1] == 0x12) {
        pOut[2] = (u8)(~pController[3]) & 0x0C;
    }
}
#endif /* XENO_PC_PORT */

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/field/nonmatchings/main/misc4", func_8007AF74);
#else
void func_8007AF74(s32 mouseIndex) {
    u8* pController = (u8*)(uintptr_t)(mouseIndex ? g_pFieldControllerBuffer2 : g_pFieldControllerBuffer1);
    s32 pos;

    if ((s8)pController[0] != 0 || (s8)pController[1] != 0x12) {
        return;
    }

    g_FieldMousePositionsX[mouseIndex] += (s8)pController[4];
    g_FieldMousePositionsY[mouseIndex] += (s8)pController[5];

    pos = g_FieldMousePositionsX[mouseIndex];
    if (pos > D_800C3A50) {
        g_FieldMousePositionsX[mouseIndex] = D_800C3A50;
    } else if (pos < D_800C3A44) {
        g_FieldMousePositionsX[mouseIndex] = D_800C3A44;
    }

    pos = g_FieldMousePositionsY[mouseIndex];
    if (pos > D_800C3A54) {
        g_FieldMousePositionsY[mouseIndex] = D_800C3A54;
    } else if (pos < D_800C3A4C) {
        g_FieldMousePositionsY[mouseIndex] = D_800C3A4C;
    }
}
#endif /* XENO_PC_PORT */

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/field/nonmatchings/main/misc4", func_8007B07C);
#else
void func_8007B07C(s16* arg0, s16* arg1, s16* arg2, s16* arg3, VECTOR* arg4) {
    VECTOR vec0;
    VECTOR normal0;
    VECTOR normal1;

    vec0.vx = arg1[0] - arg0[0];
    vec0.vy = arg1[1] - arg0[1];
    vec0.vz = arg1[2] - arg0[2];
    VectorNormal(&vec0, &normal0);

    vec0.vx = arg2[0] - arg0[0];
    vec0.vy = arg2[1] - arg0[1];
    vec0.vz = arg2[2] - arg0[2];
    VectorNormal(&vec0, &normal1);

    OuterProduct12(&normal0, &normal1, arg4);

    if (arg4->vy == 0) {
        arg3[1] = 0;
        return;
    }

    arg3[1] = arg0[1] +
              ((-(arg4->vx * (arg3[0] - arg0[0])) -
                (arg4->vz * (arg3[2] - arg0[2]))) /
               arg4->vy);
}
#endif /* XENO_PC_PORT */

/* ---- func_8007B1C4: walkmesh point-to-triangle locator ---------------------
 * Faithful port of the handwritten GTE asm (asm/.../misc4/func_8007B1C4.s,
 * 181 insns). For layer `idx`, walks its D_800AFB44[idx] triangles and finds
 * the one that contains the XZ point (posX, posZ) via three NCLIP
 * (cross-product) sign tests (all >= 0 => inside). On a hit it calls
 * func_8007B07C to interpolate the surface Y at the point (into pOut) and the
 * face normal (into pState), then returns the triangle index. If no triangle
 * contains the point (or the layer has no triangles), it zeroes both outputs
 * and returns 0 -- exactly the asm's fall-through behavior. Used at actor
 * spawn (func_8009E574 / func_80080A74) to seed walkmeshTriIds[], and for the
 * camera path (func_8007B478). Requires D_800AFB44 to be populated by the
 * walkmesh loader (FieldLoad); with it zero the loop is skipped. */
extern s32 D_800AFB24[];
extern s32 D_800AFB34[];
extern s32 D_800AFB44[];
extern u32 D_800AFB20[];
extern u8 D_800B21CC;
extern s16 D_800AFB54;

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/field/nonmatchings/main/misc4", func_8007B1C4);
#else
s16 func_8007B1C4(s16 posX, s16 posZ, s32 idx, s16* pOut, s32* pState) {
    u8* triBase = (u8*)(uintptr_t)(u32)D_800AFB24[idx];
    s32 count = D_800AFB44[idx];
    u8* vertBase = (u8*)(uintptr_t)(u32)D_800AFB34[idx];
    s32 packedPos = ((s32)posX << 16) + posZ;
    s16 point[3];
    s32 i;

    point[0] = posX;
    point[1] = 0;
    point[2] = posZ;

    for (i = 0; i < count; i++) {
        s16* tri = (s16*)(triBase + i * 14);
        s16* v0 = (s16*)(vertBase + tri[0] * 8);
        s16* v1 = (s16*)(vertBase + tri[1] * 8);
        s16* v2 = (s16*)(vertBase + tri[2] * 8);
        s32 p0 = ((s32)v0[0] << 16) + v0[2];
        s32 p1 = ((s32)v1[0] << 16) + v1[2];
        s32 p2 = ((s32)v2[0] << 16) + v2[2];

        /* NOTE: retail inlines NCLIP here too, but this function's overall
         * loop structure is still a faithful rewrite (see 8007B1C4 header), so
         * the macro form measures 12 bytes WORSE than the call form until the
         * rest of the body is matched. Keep the call form for now. */
        if (NormalClip(p0, p1, packedPos) < 0) {
            continue;
        }
        if (NormalClip(p1, p2, packedPos) < 0) {
            continue;
        }
        if (NormalClip(p2, p0, packedPos) < 0) {
            continue;
        }

        /* Point is inside triangle i: interpolate Y + normal, return index. */
        func_8007B07C(v0, v1, v2, point, (VECTOR*)pState);
        pOut[0] = point[0];
        pOut[1] = point[1];
        pOut[2] = point[2];
        return (s16)i;
    }

    /* No containing triangle: zero outputs, return 0. */
    pOut[0] = 0;
    pOut[1] = 0;
    pOut[2] = 0;
    pState[0] = 0;
    pState[1] = 0;
    pState[2] = 0;
    return 0;
}
#endif /* XENO_PC_PORT */

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/field/nonmatchings/main/misc4", func_8007B478);
#else
s32 func_8007B478(SVECTOR* p0, SVECTOR* p1, SVECTOR* p2, SVECTOR* pTest) {
    VECTOR edge1, edge2, cross;
    /* Edge1 = p1 - p0, Edge2 = p2 - p0 */
    edge1.vx = p1->vx - p0->vx;
    edge1.vy = p1->vy - p0->vy;
    edge1.vz = 0;
    edge2.vx = p2->vx - p0->vx;
    edge2.vy = p2->vy - p0->vy;
    edge2.vz = 0;
    OuterProduct0(&edge1, &edge2, &cross);
    if (cross.vz < 0) return -1;

    /* Edge1 = pTest - p0, Edge2 = p2 - p0 */
    edge1.vx = pTest->vx - p0->vx;
    edge1.vy = pTest->vy - p0->vy;
    edge1.vz = 0;
    edge2.vx = p2->vx - p0->vx;
    edge2.vy = p2->vy - p0->vy;
    edge2.vz = 0;
    OuterProduct0(&edge1, &edge2, &cross);
    if (cross.vz < 0) return -1;

    /* Edge1 = p1 - pTest, Edge2 = p2 - pTest */
    edge1.vx = p1->vx - pTest->vx;
    edge1.vy = p1->vy - pTest->vy;
    edge1.vz = 0;
    edge2.vx = p2->vx - pTest->vx;
    edge2.vy = p2->vy - pTest->vy;
    edge2.vz = 0;
    OuterProduct0(&edge1, &edge2, &cross);
    return cross.vz >> 31;
}
#endif /* XENO_PC_PORT */

extern s16 D_800B218C;

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/field/nonmatchings/main/misc4", func_8007B614);
#else
void func_8007B614(s32* pOut, s16 scale, s16 angle) {
    s32 magnitude = (scale << 4) * D_800B218C;

    magnitude >>= 12;
    angle &= 0xFFF;

    pOut[0] = rsin(angle) * magnitude;
    pOut[1] = 0;
    pOut[2] = -(rcos(angle) * magnitude);
}
#endif /* XENO_PC_PORT */

s32 func_8007B694(s32* arg0) {
    return -ratan2(arg0[2], arg0[0]) & 0xFFF;
}

extern long FieldGetVec2Magnitude(long x, long y);

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/field/nonmatchings/main/misc4", func_8007B6C4);
#else
s32 func_8007B6C4(s16 angle, s16* edge, s32* vec) {
    VECTOR side;
    VECTOR normal;
    s32 edgeAngle;
    s32 deltaAngle;
    s32 magnitude;

    deltaAngle = (0xC00 - angle) & 0xFFF;
    edgeAngle = -ratan2(edge[6] - edge[2], edge[4] - edge[0]) & 0xFFF;
    deltaAngle = (deltaAngle + edgeAngle) & 0xFFF;

    if ((u32)(deltaAngle - 0x80) >= 0xF01) {
        vec[0] = 0;
        vec[1] = 0;
        vec[2] = 0;
        return edgeAngle;
    }

    if (deltaAngle < 0x800) {
        side.vx = edge[0] - edge[4];
        side.vy = 0;
        side.vz = edge[2] - edge[6];
        edgeAngle = (edgeAngle + 0x800) & 0xFFF;
    } else {
        side.vx = edge[4] - edge[0];
        side.vy = 0;
        side.vz = edge[6] - edge[2];
    }

    VectorNormal(&side, &normal);
    magnitude = FieldGetVec2Magnitude(vec[0] >> 12, vec[2] >> 12);
    vec[0] = normal.vx * magnitude;
    vec[1] = 0;
    vec[2] = normal.vz * magnitude;
    return edgeAngle;
}
#endif /* XENO_PC_PORT */

extern s32 D_800ADB98;
extern s32 func_8007C694(s32* move, s32* base, void* pActorData,
                         s16* outEdge, s16* outPoint, s32 mode);

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/field/nonmatchings/main/misc4", func_8007B814);
#else
void* func_8007B814(s32* pVec, void* pActorData, s32* pOut, s16 angle) {
    u8* actorData = pActorData;
    s16 resolvedAngle = angle;
    s16 point[16];
    s32 move[3];
    s32 result;
    s32 candidateAngle;

    candidateAngle = angle & 0x0FFF;
    move[0] = pVec[0] + (rsin(candidateAngle) << 6);
    move[1] = 0;
    move[2] = pVec[2] - (rcos(candidateAngle) << 6);
    result = func_8007C694(move, (s32*)(actorData + 0x20), actorData,
                           (s16*)pOut, point, -1);

    if (result != -1) {
        resolvedAngle = result >> 16;

        candidateAngle = (resolvedAngle - 0x100) & 0x0FFF;
        move[0] = pVec[0] + (rsin(candidateAngle) << 6);
        move[1] = 0;
        move[2] = pVec[2] - (rcos(candidateAngle) << 6);
        result = func_8007C694(move, (s32*)(actorData + 0x20), actorData,
                               (s16*)pOut, point, -1);

        if (result != -1) {
            candidateAngle = (resolvedAngle + 0x100) & 0x0FFF;
            move[0] = pVec[0] + (rsin(candidateAngle) << 6);
            move[1] = 0;
            move[2] = pVec[2] - (rcos(candidateAngle) << 6);
            result = func_8007C694(move, (s32*)(actorData + 0x20), actorData,
                                   (s16*)pOut, point, -1);

            if (result != -1) {
                move[0] = pVec[0];
                move[1] = pVec[1];
                move[2] = pVec[2];
            } else {
                move[0] = pVec[0];
                move[1] = pVec[1];
                move[2] = pVec[2];
                func_8007B6C4(resolvedAngle, (s16*)pOut, move);
            }
        } else {
            move[0] = pVec[0];
            move[1] = pVec[1];
            move[2] = pVec[2];
            func_8007B6C4(resolvedAngle, (s16*)pOut, move);
        }
    } else {
        move[0] = pVec[0];
        move[1] = pVec[1];
        move[2] = pVec[2];
        func_8007B6C4(resolvedAngle, (s16*)pOut, move);
    }

    if (func_8007C694(move, (s32*)(actorData + 0x20), actorData,
                      (s16*)pOut, point, 0) == -1) {
        return (void*)-1;
    }

    if ((*(u32*)actorData & 0x00040000) != 0) {
        point[1] = *(u16*)(actorData + 0xEC);
    } else if (((s32)point[1] << 16) < *(s32*)(actorData + 0x24) &&
               D_800ADB98 == 0) {
        return (void*)-1;
    }

    move[1] = ((s32)point[1] << 16) - *(s32*)(actorData + 0x24);
    pVec[0] = move[0];
    pVec[1] = move[1];
    pVec[2] = move[2];
    *(s16*)(actorData + 0x72) = (*(s32*)(actorData + 0x24) + pVec[1]) >> 16;
    return 0;
}
#endif /* XENO_PC_PORT */

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/field/nonmatchings/main/misc4", func_8007BAC0);
#else
void* func_8007BAC0(s32* pVec, void* pActorData, s32* pOut, s16 angle) {
    u8* actorData = pActorData;
    s16 signedAngle = angle;
    s16 edge[4];
    s16 point[16];
    s16 savedPoint[16];
    s32 move[3];
    s32 savedMove[3];
    s32 flags = 0;
    s32 candidateAngle;
    s32 magnitude;
    VECTOR normalIn;
    VECTOR normalOut;

    candidateAngle = (signedAngle - 0x100) & 0x0FFF;
    move[0] = pVec[0] + (rsin(candidateAngle) << 6);
    move[1] = 0;
    move[2] = pVec[2] - (rcos(candidateAngle) << 6);
    if (func_8007BEF4(move, (s32*)(actorData + 0x20), actorData,
                      (s16*)pOut, point, -1, &flags) != -1) {
        candidateAngle = (signedAngle + 0x100) & 0x0FFF;
        move[0] = pVec[0] + (rsin(candidateAngle) << 6);
        move[1] = 0;
        move[2] = pVec[2] - (rcos(candidateAngle) << 6);
        if (func_8007BEF4(move, (s32*)(actorData + 0x20), actorData,
                          (s16*)pOut, point, -1, &flags) != -1) {
            candidateAngle = angle & 0x0FFF;
            move[0] = pVec[0] + (rsin(candidateAngle) << 6);
            move[1] = 0;
            move[2] = pVec[2] - (rcos(candidateAngle) << 6);
            if (func_8007BEF4(move, (s32*)(actorData + 0x20), actorData,
                              (s16*)pOut, point, -1, &flags) != -1) {
                move[0] = pVec[0];
                move[1] = pVec[1];
                move[2] = pVec[2];
            } else {
                move[0] = pVec[0];
                move[1] = pVec[1];
                move[2] = pVec[2];
                func_8007B6C4(signedAngle, (s16*)pOut, move);
            }
        } else {
            move[0] = pVec[0];
            move[1] = pVec[1];
            move[2] = pVec[2];
            func_8007B6C4(signedAngle, (s16*)pOut, move);
        }
    } else {
        move[0] = pVec[0];
        move[1] = pVec[1];
        move[2] = pVec[2];
        func_8007B6C4(signedAngle, (s16*)pOut, move);
    }

    if (func_8007BEF4(move, (s32*)(actorData + 0x20), actorData,
                      (s16*)pOut, point, 0, &flags) == -1) {
        return (void*)-1;
    }

    savedMove[0] = move[0];
    savedMove[1] = move[1];
    savedMove[2] = move[2];
    savedPoint[0] = point[0];
    savedPoint[1] = point[1];
    savedPoint[2] = point[2];
    savedPoint[3] = point[3];

    if (point[1] < *(s16*)(actorData + 0x26)) {
retry_along_normal:
        normalIn.vx = -move[0] >> 8;
        normalIn.vy = (((s32)point[1] << 16) - *(s32*)(actorData + 0x24)) >> 8;
        normalIn.vz = -move[2] >> 8;
        VectorNormal(&normalIn, &normalOut);

        magnitude = FieldGetVec2Magnitude(move[0] >> 8, move[2] >> 8);
        move[0] = -(magnitude * normalOut.vx) >> 4;
        move[1] = (magnitude * normalOut.vy) >> 4;
        move[2] = -(magnitude * normalOut.vz) >> 4;

        if (func_8007BEF4(move, (s32*)(actorData + 0x20), actorData,
                          (s16*)pOut, point, 0, &flags) == -1) {
            return (void*)-1;
        }

        *(u32*)actorData |= 0x04000000;
    } else {
        if ((flags & 0x00200000) != 0) {
            goto retry_along_normal;
        }

        if ((flags & 0x00420000) != 0) {
            if ((*(u32*)(actorData + 0x14) & 0x00420000) != 0) {
                goto retry_along_normal;
            }
        } else if (point[1] < *(s16*)(actorData + 0x26) + 0x40) {
            goto retry_along_normal;
        }

        move[0] = savedMove[0];
        move[1] = savedMove[1];
        move[2] = savedMove[2];
        point[0] = savedPoint[0];
        point[1] = savedPoint[1];
        point[2] = savedPoint[2];
        point[3] = savedPoint[3];
    }

    move[1] = ((s32)point[1] << 16) - *(s32*)(actorData + 0x24);
    pVec[0] = move[0];
    pVec[1] = move[1];
    pVec[2] = move[2];
    *(s16*)(actorData + 0x72) = (*(s32*)(actorData + 0x24) + pVec[1]) >> 16;
    return 0;
}
#endif /* XENO_PC_PORT */

static inline s32 FieldPackedXZ(const s16* vert) {
    return ((s32)vert[0] << 16) + vert[2];
}

static inline void FieldCopyCameraEdge(u8* out, const s16* a, const s16* b) {
    *(u16*)(out + 0x00) = (u16)a[0];
    *(u16*)(out + 0x02) = (u16)a[1];
    *(u16*)(out + 0x04) = (u16)a[2];
    *(u16*)(out + 0x08) = (u16)b[0];
    *(u16*)(out + 0x0A) = (u16)b[1];
    *(u16*)(out + 0x0C) = (u16)b[2];
}

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/field/nonmatchings/main/misc4", func_8007BEF4);
#else
s32 func_8007BEF4(s32* move, s32* base, u8* actorData, s16* outEdge,
                  s16* outPoint, s32 mode, s32* outFlags) {
    s32 state = *(s16*)(actorData + 0x10);
    s16 triIndex = *(s16*)(actorData + state * 2 + 0x08);
    u8* triBase = (u8*)(uintptr_t)(u32)D_800AFB24[state];
    u8* vertBase = (u8*)(uintptr_t)(u32)D_800AFB34[state];
    s32 packedNewXZ;
    s32 packedOldXZ;
    s32 collisionMask = 0;
    s32 forceEdgeSearch;
    s32 steps = 0;
    s32 lastTri = triIndex;
    s32 sideMask = 0;

    if (triIndex == -1) {
        return -1;
    }

    outPoint[0] = (base[0] + move[0]) >> 16;
    outPoint[1] = 0;
    outPoint[2] = (base[2] + move[2]) >> 16;
    packedNewXZ = ((s32)outPoint[0] << 16) + outPoint[2];
    packedOldXZ = ((base[0] >> 16) << 16) + (base[2] >> 16);

    if (((*(u32*)(actorData + 0x04) >> (state + 3)) & 1) == 0) {
        collisionMask = (D_800B21CC == 0) ? -1 : 0;
    }

    /* NOTE: kept hoisted here. Moving this computation inside the loop below
     * (to mimic retail's duplicated `triBase + idx*14` scaling at 8007BFF0 and
     * 8007C00C) was TRIED and MEASURED: it made func_8007BEF4 1504 -> 1484 B,
     * i.e. 20 bytes WORSE, because GCC then treats the whole sequence as
     * loop-invariant and hoists more. Do not re-attempt without new evidence. */
    {
        s16* tri = (s16*)(triBase + triIndex * 14);
        u32 triFlags = ((u32*)(uintptr_t)D_800AFB20[0])[*((u8*)tri + 0x0C)];
        forceEdgeSearch = ((triFlags & 0x00400000) != 0) || mode == 0x80;
    }

    for (;;) {
        s16* tri = (s16*)(triBase + triIndex * 14);
        s16* v0 = (s16*)(vertBase + tri[0] * 8);
        s16* v1 = (s16*)(vertBase + tri[1] * 8);
        s16* v2 = (s16*)(vertBase + tri[2] * 8);
        s32 p0 = FieldPackedXZ(v0);
        s32 p1 = FieldPackedXZ(v1);
        s32 p2 = FieldPackedXZ(v2);
        s32 nextTri = -1;
        u32 flags;

        lastTri = triIndex;
        sideMask = 0;
        if (NormalClip(p0, p1, packedNewXZ) < 0) {
            sideMask |= 1;
        }
        if (NormalClip(p1, p2, packedNewXZ) < 0) {
            sideMask |= 2;
        }
        if (NormalClip(p2, p0, packedNewXZ) < 0) {
            sideMask |= 4;
        }

        switch (sideMask) {
        case 0:
            steps = 0xFF;
            break;
        case 1:
            nextTri = tri[3];
            break;
        case 2:
            nextTri = tri[4];
            break;
        case 3:
            if (NormalClip(p1, packedNewXZ, packedOldXZ) < 0) {
                nextTri = tri[3];
#ifndef FIELD_WALKMESH_MUTANT_KEEP_COMPOUND_SIDE
                sideMask = 1;
#endif
            } else {
                nextTri = tri[4];
#ifndef FIELD_WALKMESH_MUTANT_KEEP_COMPOUND_SIDE
                sideMask = 2;
#endif
            }
            break;
        case 4:
            nextTri = tri[5];
            break;
        case 5:
            if (NormalClip(p0, packedNewXZ, packedOldXZ) < 0) {
                nextTri = tri[5];
#ifndef FIELD_WALKMESH_MUTANT_KEEP_COMPOUND_SIDE
                sideMask = 4;
#endif
            } else {
                nextTri = tri[3];
#ifndef FIELD_WALKMESH_MUTANT_KEEP_COMPOUND_SIDE
                sideMask = 1;
#endif
            }
            break;
        case 6:
            if (NormalClip(p2, packedNewXZ, packedOldXZ) >= 0) {
                nextTri = tri[5];
#ifndef FIELD_WALKMESH_MUTANT_KEEP_COMPOUND_SIDE
                sideMask = 4;
#endif
            } else {
                nextTri = tri[4];
#ifndef FIELD_WALKMESH_MUTANT_KEEP_COMPOUND_SIDE
                sideMask = 2;
#endif
            }
            break;
        case 7:
            /* Retail's switch has EIGHT cases: the dispatch range check is
             * `sltiu ..., 0x8` (x2) in
             * asm/field/matchings/main/misc4/func_8007BEF4.s and jtbl_8006FB8C has
             * eight entries. Index 7's target is 0x8007C244 =
             * `addiu $s0, $zero, -0x1` (line 241), i.e. triIndex = -1 - the same
             * body our `default:` had, which is why GCC emitted a literal-0 8th
             * table slot instead of a real address. */
            triIndex = -1;
            break;
        default:
            triIndex = -1;
            break;
        }

        if (sideMask != 0 && sideMask < 7) {
            triIndex = (s16)nextTri;
        }

        if (triIndex != -1) {
            tri = (s16*)(triBase + triIndex * 14);
            flags = ((u32*)(uintptr_t)D_800AFB20[0])[*((u8*)tri + 0x0C)] & collisionMask;
            *outFlags = flags;

            if ((((*(u32*)actorData >> 8) & 7) & (flags >> 5)) != 0) {
                triIndex = -1;
            } else if ((flags & 0x00800000) != 0 && *(s16*)(actorData + 0x10) == 0) {
                triIndex = -1;
            } else if ((flags & 0x00400000) != 0) {
                if (!forceEdgeSearch) {
                    func_8007B07C((s16*)(vertBase + tri[0] * 8),
                                  (s16*)(vertBase + tri[1] * 8),
                                  (s16*)(vertBase + tri[2] * 8),
                                  outPoint, (VECTOR*)((u8*)outPoint + 0x08));
                    if (outPoint[1] < *(s16*)((u8*)base + 0x06)) {
                        triIndex = -1;
                    }
                }
            }
        }

        if (triIndex != -1) {
            steps++;
            if (steps < 0x20) {
                continue;
            }
        }
        break;
    }

    if (triIndex != -1 && steps != 0x20) {
        if (mode == -1) {
            return 0;
        }

        {
            s16* tri = (s16*)(triBase + triIndex * 14);
            func_8007B07C((s16*)(vertBase + tri[0] * 8),
                          (s16*)(vertBase + tri[1] * 8),
                          (s16*)(vertBase + tri[2] * 8),
                          outPoint, (VECTOR*)((u8*)outPoint + 0x08));
        }
        return 0;
    }

    if (sideMask == 1 || sideMask == 2 || sideMask == 4) {
        s16* tri = (s16*)(triBase + lastTri * 14);
        /* Retail's three arms share the final b.z store at 8007C628.
         * Keep the matching-build expression unchanged here; the native
         * branches below restore that shared-tail effect for edges 1/2. */
        if (sideMask == 1) {
            const s16* a = (const s16*)(vertBase + tri[0] * 8);
            const s16* b = (const s16*)(vertBase + tri[1] * 8);

            ((u16*)outEdge)[0x0] = (u16)a[0];
            ((u16*)outEdge)[0x1] = (u16)a[1];
            ((u16*)outEdge)[0x2] = (u16)a[2];
            ((u16*)outEdge)[0x4] = (u16)b[0];
            ((u16*)outEdge)[0x5] = (u16)b[1];
#ifdef XENO_PC_PORT
            /* 8007C4C0 / 8007C570 jump to the b.z store at 8007C634. */
            ((u16*)outEdge)[0x6] = (u16)b[2];
#endif
        } else if (sideMask == 2) {
            const s16* a = (const s16*)(vertBase + tri[1] * 8);
            const s16* b = (const s16*)(vertBase + tri[2] * 8);

            ((u16*)outEdge)[0x0] = (u16)a[0];
            ((u16*)outEdge)[0x1] = (u16)a[1];
            ((u16*)outEdge)[0x2] = (u16)a[2];
            ((u16*)outEdge)[0x4] = (u16)b[0];
            ((u16*)outEdge)[0x5] = (u16)b[1];
#ifdef XENO_PC_PORT
            /* 8007C4C0 / 8007C570 jump to the b.z store at 8007C634. */
            ((u16*)outEdge)[0x6] = (u16)b[2];
#endif
        } else {
            const s16* a = (const s16*)(vertBase + tri[2] * 8);
            const s16* b = (const s16*)(vertBase + tri[0] * 8);

            ((u16*)outEdge)[0x0] = (u16)a[0];
            ((u16*)outEdge)[0x1] = (u16)a[1];
            ((u16*)outEdge)[0x2] = (u16)a[2];
            ((u16*)outEdge)[0x4] = (u16)b[0];
            ((u16*)outEdge)[0x5] = (u16)b[1];
            ((u16*)outEdge)[0x6] = (u16)b[2];
        }
    }

    return -1;
}
#endif /* XENO_PC_PORT */

void func_8007C670(s32* arg0, s32* arg1, s32 arg2) {
    s32 value = *arg0;

    if (arg2 >= 0) {
        *arg0 = value;
        value += arg2;
    } else {
        *arg0 = value;
    }

    *arg1 = value;
}

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/field/nonmatchings/main/misc4", func_8007C694);
#else
s32 func_8007C694(s32* move, s32* base, void* pActorData,
                  s16* outEdge, s16* outPoint, s32 mode) {
    u8* actorData = pActorData;
    s32 state = *(s16*)(actorData + 0x10);
    s16 triIndex = *(s16*)(actorData + state * 2 + 0x08);
    u8* triBase = (u8*)(uintptr_t)(u32)D_800AFB24[state];
    u8* vertBase = (u8*)(uintptr_t)(u32)D_800AFB34[state];
    s32 packedNewXZ;
    s32 packedOldXZ;
    s32 collisionMask = 0;
    s32 steps = 0;
    s32 lastTri = triIndex;
    s32 sideMask = 0;

    if (triIndex == -1) {
        return -1;
    }

    outPoint[0] = (base[0] + move[0]) >> 16;
    outPoint[1] = 0;
    outPoint[2] = (base[2] + move[2]) >> 16;
    packedNewXZ = ((s32)outPoint[0] << 16) + outPoint[2];
    packedOldXZ = ((base[0] >> 16) << 16) + (base[2] >> 16);

    if (((*(u32*)(actorData + 0x04) >> (state + 3)) & 1) == 0) {
        collisionMask = (D_800B21CC == 0) ? -1 : 0;
    }

    for (;;) {
        s16* tri = (s16*)(triBase + triIndex * 14);
        s16* v0 = (s16*)(vertBase + tri[0] * 8);
        s16* v1 = (s16*)(vertBase + tri[1] * 8);
        s16* v2 = (s16*)(vertBase + tri[2] * 8);
        s32 p0 = FieldPackedXZ(v0);
        s32 p1 = FieldPackedXZ(v1);
        s32 p2 = FieldPackedXZ(v2);
        s32 nextTri = -1;
        u32 flags;

        lastTri = triIndex;
        sideMask = 0;
        if (NormalClip(p0, p1, packedNewXZ) < 0) {
            sideMask |= 1;
        }
        if (NormalClip(p1, p2, packedNewXZ) < 0) {
            sideMask |= 2;
        }
        if (NormalClip(p2, p0, packedNewXZ) < 0) {
            sideMask |= 4;
        }

        switch (sideMask) {
        case 0:
            steps = 0xFF;
            break;
        case 1:
            nextTri = tri[3];
            break;
        case 2:
            nextTri = tri[4];
            break;
        case 3:
            if (NormalClip(p1, packedNewXZ, packedOldXZ) < 0) {
                nextTri = tri[3];
                sideMask = 1;
            } else {
                nextTri = tri[4];
                sideMask = 2;
            }
            break;
        case 4:
            nextTri = tri[5];
            break;
        case 5:
            if (NormalClip(p0, packedNewXZ, packedOldXZ) < 0) {
                nextTri = tri[5];
                sideMask = 4;
            } else {
                nextTri = tri[3];
                sideMask = 1;
            }
            break;
        case 6:
            if (NormalClip(p2, packedNewXZ, packedOldXZ) >= 0) {
                nextTri = tri[5];
                sideMask = 4;
            } else {
                nextTri = tri[4];
                sideMask = 2;
            }
            break;
        case 7:
            /* Same missing case as in func_8007BEF4 above: the retail dispatch in
             * asm/field/matchings/main/misc4/func_8007C694.s has two
             * `sltiu ..., 0x8` range checks (eight cases) and jtbl_8006FBAC has
             * eight entries; index 7's target 0x8007C980 is
             * `addiu $s0, $zero, -0x1` (line 214) = triIndex = -1. */
            triIndex = -1;
            break;
        default:
            triIndex = -1;
            break;
        }

        if (sideMask != 0 && triIndex != -1) {
            triIndex = (s16)nextTri;
        }

        if (triIndex == -1) {
            break;
        }

        tri = (s16*)(triBase + triIndex * 14);
        flags = ((u32*)(uintptr_t)D_800AFB20[0])[*((u8*)tri + 0x0C)] & collisionMask;

        if ((((*(u32*)actorData >> 9) & 3) & (flags >> 3)) != 0 ||
            (((*(u32*)actorData >> 8) & 7) & (flags >> 5)) != 0 ||
            ((flags & 0x00800000) != 0 && *(s16*)(actorData + 0x10) == 0)) {
            triIndex = -1;
            break;
        }

        steps++;
        if (steps < 0x20) {
            continue;
        }
        break;
    }

    if (triIndex != -1 && steps != 0x20) {
        if (mode == -1) {
            return 0;
        }

        {
            s16* tri = (s16*)(triBase + triIndex * 14);
            func_8007B07C((s16*)(vertBase + tri[0] * 8),
                          (s16*)(vertBase + tri[1] * 8),
                          (s16*)(vertBase + tri[2] * 8),
                          outPoint, (VECTOR*)((u8*)outPoint + 0x08));
        }
        return 0;
    }

    if (sideMask == 1 || sideMask == 2 || sideMask == 4) {
        s16* tri = (s16*)(triBase + lastTri * 14);
        if (sideMask == 1) {
            FieldCopyCameraEdge((u8*)outEdge, (s16*)(vertBase + tri[0] * 8),
                                (s16*)(vertBase + tri[1] * 8));
        } else if (sideMask == 2) {
            FieldCopyCameraEdge((u8*)outEdge, (s16*)(vertBase + tri[1] * 8),
                                (s16*)(vertBase + tri[2] * 8));
        } else {
            FieldCopyCameraEdge((u8*)outEdge, (s16*)(vertBase + tri[2] * 8),
                                (s16*)(vertBase + tri[0] * 8));
        }
    }

    return -1;
}
#endif /* XENO_PC_PORT */

extern s32 D_800ADC10;

void* func_8007CD3C(s32 a0) {
    s32 old = D_800ADC10;
    D_800ADC10 = old + a0;
    return (void*)((old << 2) + 0x1F800000);
}

void func_8007CD60(s32 a0) {
    D_800ADC10 -= a0;
}

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/field/nonmatchings/main/misc4", func_8007CD80);
#else
s32 func_8007CD80(void* arg0, void* arg1, void* arg2) {
    s16 posX = *(s16*)((u8*)arg0 + 0x02);
    s16 posZ = *(s16*)((u8*)arg0 + 0x0A);
    s16 clampedX;
    s16 clampedZ;
    s32 packedPos = ((s32)posX << 16) + posZ;
    s32 packedClamped;
    u8* triBase;
    u8* vertBase;
    s16 triIndex;
    s16 edgeTri = 0;
    s32 edge = 0;
    s32 steps = 0;

    if (posX < *(s16*)((u8*)&g_Scene + 0x4C)) {
        clampedX = *(s16*)((u8*)&g_Scene + 0x4C);
    } else {
        s32 maxX = *(s16*)((u8*)&g_Scene + 0x4C) +
                   *(s16*)((u8*)&g_Scene + 0x50);
        clampedX = (maxX < posX) ? (s16)maxX : posX;
    }

    if (*(s16*)((u8*)&g_Scene + 0x4E) < posZ) {
        clampedZ = *(s16*)((u8*)&g_Scene + 0x4E);
    } else {
        s32 maxZ = *(s16*)((u8*)&g_Scene + 0x4E) +
                   *(s16*)((u8*)&g_Scene + 0x52);
        clampedZ = (posZ < maxZ) ? (s16)maxZ : posZ;
    }

    packedClamped = ((s32)clampedX << 16) + clampedZ;
    triIndex = func_8007B1C4(clampedX, clampedZ, D_800AFB54 - 1,
                             (s16*)arg2, (s32*)((u8*)arg2 + 0x08));
    triBase = (u8*)(uintptr_t)D_800AFB20[D_800AFB54];
    vertBase = (u8*)(uintptr_t)D_800AFB20[D_800AFB54 + 4];

    for (;;) {
        s16* tri;
        s16* v0;
        s16* v1;
        s16* v2;
        s32 p0;
        s32 p1;
        s32 p2;
        s32 sideMask;
        s32 nextTri;

        if (triIndex < 0) {
            break;
        }

        tri = (s16*)(triBase + triIndex * 14);
        v0 = (s16*)(vertBase + tri[0] * 8);
        v1 = (s16*)(vertBase + tri[1] * 8);
        v2 = (s16*)(vertBase + tri[2] * 8);
        p0 = FieldPackedXZ(v0);
        p1 = FieldPackedXZ(v1);
        p2 = FieldPackedXZ(v2);

        sideMask = 0;
        {
            s32 nclip;

            gte_NormalClip(p0, p1, packedPos, &nclip);
            if (nclip < 0) {
                sideMask |= 1;
            }
            gte_NormalClip(p1, p2, packedPos, &nclip);
            if (nclip < 0) {
                sideMask |= 2;
            }
            gte_NormalClip(p2, p0, packedPos, &nclip);
            if (nclip < 0) {
                sideMask |= 4;
            }
        }

        nextTri = -1;
        switch (sideMask) {
        case 0:
            steps = 0xFF;
            break;
        case 1:
            nextTri = *(s16*)((u8*)triBase + triIndex * 14 + 6);
            break;
        case 2:
            nextTri = *(s16*)((u8*)triBase + triIndex * 14 + 8);
            break;
        case 3: {
            s32 nclip;

            gte_NormalClip(p1, packedPos, packedClamped, &nclip);
            nextTri = (nclip < 0)
                ? *(s16*)((u8*)triBase + triIndex * 14 + 6)
                : *(s16*)((u8*)triBase + triIndex * 14 + 8);
            break;
        }
        case 4:
            nextTri = *(s16*)((u8*)triBase + triIndex * 14 + 10);
            break;
        case 5: {
            s32 nclip;

            gte_NormalClip(p0, packedPos, packedClamped, &nclip);
            nextTri = (nclip < 0)
                ? *(s16*)((u8*)triBase + triIndex * 14 + 10)
                : *(s16*)((u8*)triBase + triIndex * 14 + 6);
            break;
        }
        case 6: {
            s32 nclip;

            gte_NormalClip(p2, packedPos, packedClamped, &nclip);
            nextTri = (nclip >= 0)
                ? *(s16*)((u8*)triBase + triIndex * 14 + 10)
                : *(s16*)((u8*)triBase + triIndex * 14 + 8);
            break;
        }
        case 7:
            /* Retail switch admits 8 values: dispatch is
             * `sltiu $v0, $a1, 0x8` (x2) in
             * asm/field/matchings/main/misc4/func_8007CD80.s lines 159/161
             * (0x8007CF94 / 0x8007CF9C), and jtbl_8006FBCC has eight entries.
             * $a1 is sideMask. Index 7's target 0x8007D0F0 is
             * `addiu $a3, $zero, -0x1` (line 250). $a3 is the live triangle
             * index written by cases 1-6 (nextTri in this C). */
#ifdef FIELD_WALKMESH_MUTANT_CASE7_WALK
            nextTri = tri[3];
#else
            nextTri = -1;
#endif
            break;
        default:
            nextTri = -1;
            break;
        }

        if (sideMask != 0) {
            edgeTri = triIndex;
            triIndex = (s16)nextTri;
            edge = sideMask;
        }

        {
            s16* curTri;
            u32 flags;

            if (triIndex < 0) {
                break;
            }

            curTri = (s16*)(triBase + triIndex * 14);
            flags = ((u32*)(uintptr_t)D_800AFB20[0])[*((u8*)curTri + 0x0C)];
            if ((flags & 0x800000) == 0) {
                triIndex = -1;
                break;
            }
        }

        steps++;
        if (triIndex == -1 || steps >= 0xF0) {
            break;
        }
        if (sideMask == 0) {
            break;
        }
    }

    if (triIndex != -1 && steps != 0xF0) {
        return 0;
    }

    if (edge == 1 || edge == 2 || edge == 4) {
        s16* tri = (s16*)(triBase + edgeTri * 14);
        s16* a;
        s16* b;

        if (edge == 1) {
            a = (s16*)(vertBase + tri[0] * 8);
            b = (s16*)(vertBase + tri[1] * 8);
        } else if (edge == 2) {
            a = (s16*)(vertBase + tri[1] * 8);
            b = (s16*)(vertBase + tri[2] * 8);
        } else {
            a = (s16*)(vertBase + tri[2] * 8);
            b = (s16*)(vertBase + tri[0] * 8);
        }
        FieldCopyCameraEdge((u8*)arg1, a, b);
    }

    *(s16*)((u8*)arg2 + 0x00) = posX;
    *(s16*)((u8*)arg2 + 0x02) = posZ;
    *(s16*)((u8*)arg2 + 0x04) = clampedX;
    *(s16*)((u8*)arg2 + 0x06) = clampedZ;
    return -1;
}
#endif /* XENO_PC_PORT */

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/field/nonmatchings/main/misc4", func_8007D3D4);
#else
s32 func_8007D3D4(u8* actorData, s32 idx, s32* outHeight0,
                  VECTOR* outNormal, s16* outTriangle, s32* outHeight1) {
    s16 triIndex;
    u8* triBase;
    u8* vertBase;
    s16 point[4];
    s32 packedNewXZ;
    s32 packedOldXZ;
    s32 collisionMask;
    s32 steps;

    triIndex = *(s16*)(actorData + idx * 2 + 0x08);
    triBase = (u8*)(uintptr_t)(u32)D_800AFB24[idx];
    vertBase = (u8*)(uintptr_t)(u32)D_800AFB34[idx];

    if (triIndex == -1) {
        return -1;
    }

    point[0] = (*(s32*)(actorData + 0x20) + *(s32*)(actorData + 0x30)) >> 16;
    point[1] = 0;
    point[2] = (*(s32*)(actorData + 0x28) + *(s32*)(actorData + 0x38)) >> 16;
    packedNewXZ = (point[0] << 16) + point[2];
    packedOldXZ = ((*(s32*)(actorData + 0x20) >> 16) << 16) +
                  (*(s32*)(actorData + 0x28) >> 16);

    collisionMask = 0;
    if (((*(u32*)(actorData + 0x04) >> (idx + 3)) & 1) == 0) {
        collisionMask = (D_800B21CC == 0) ? -1 : 0;
    }

    steps = 0;
    for (;;) {
        s16* tri = (s16*)(triBase + triIndex * 14);
        s16* v0 = (s16*)(vertBase + tri[0] * 8);
        s16* v1 = (s16*)(vertBase + tri[1] * 8);
        s16* v2 = (s16*)(vertBase + tri[2] * 8);
        s32 p0 = (v0[0] << 16) + v0[2];
        s32 p1 = (v1[0] << 16) + v1[2];
        s32 p2 = (v2[0] << 16) + v2[2];
        s32 sideMask = 0;
        s32 nextTri = -1;

        if (NormalClip(p0, p1, packedNewXZ) < 0) {
            sideMask |= 1;
        }
        if (NormalClip(p1, p2, packedNewXZ) < 0) {
            sideMask |= 2;
        }
        if (NormalClip(p2, p0, packedNewXZ) < 0) {
            sideMask |= 4;
        }

        /* Retail's case-body emission order is 0,3,5,1,6,2,4,7, recovered from
         * jtbl_8006FBEC's target addresses (0x8007D5A8, 0x8007D5B0, 0x8007D5D0,
         * 0x8007D5E8, 0x8007D604, 0x8007D61C, 0x8007D638, 0x8007D654). Index i
         * still dispatches to case value i, and every body ends in `break`, so
         * this label order is behaviour-neutral.
         *
         * GCC 2.7.2 lays the bodies out by CFG, not source order: indexing the
         * cached `tri` local ranks the emitted table (0,2,6,1,5,3,4,7) for any
         * source order. Writing the address as `triBase + triIndex * 14` in each
         * body reproduces retail's shape (0x8007D5E8/0x8007D61C/0x8007D638
         * compute it, and cases 3/5/6 branch into those shared suffixes), which
         * is what lets this label order canonicalise to the retail table. */
        switch (sideMask) {
        case 0:
            steps = 0xFF;
            break;
        case 3:
            nextTri = (NormalClip(p1, packedNewXZ, packedOldXZ) < 0)
                ? *(s16*)((u8*)triBase + triIndex * 14 + 6)
                : *(s16*)((u8*)triBase + triIndex * 14 + 8);
            break;
        case 5:
            nextTri = (NormalClip(p0, packedNewXZ, packedOldXZ) < 0)
                ? *(s16*)((u8*)triBase + triIndex * 14 + 10)
                : *(s16*)((u8*)triBase + triIndex * 14 + 6);
            break;
        case 1:
#ifdef FIELD_WALKMESH_MUTANT_D3D4_SWAP12
            nextTri = *(s16*)((u8*)triBase + triIndex * 14 + 8);
#else
            nextTri = *(s16*)((u8*)triBase + triIndex * 14 + 6);
#endif
            break;
        case 6:
            nextTri = (NormalClip(p2, packedNewXZ, packedOldXZ) >= 0)
                ? *(s16*)((u8*)triBase + triIndex * 14 + 10)
                : *(s16*)((u8*)triBase + triIndex * 14 + 8);
            break;
        case 2:
#ifdef FIELD_WALKMESH_MUTANT_D3D4_SWAP12
            nextTri = *(s16*)((u8*)triBase + triIndex * 14 + 6);
#else
            nextTri = *(s16*)((u8*)triBase + triIndex * 14 + 8);
#endif
            break;
        case 4:
            nextTri = *(s16*)((u8*)triBase + triIndex * 14 + 10);
            break;
        case 7:
            /* Retail's switch really does have a case 7: its dispatch range check is
             * `sltiu $v0, $s0, 0x8` (asm/field/matchings/main/misc4/func_8007D3D4.s
             * lines 127/129) and jtbl_8006FBEC has EIGHT entries. Our C previously
             * stopped at case 6 and let `default:` absorb index 7, which is why the
             * built table had only 7 entries where retail has 8 - the last 4 bytes of
             * the field rodata discrepancy. The index-7 target in retail is
             * 0x8007D654 = `addiu $s1, $zero, -0x1`, i.e. nextTri = -1. */
            nextTri = -1;
            break;
        default:
            nextTri = -1;
            break;
        }

        if (sideMask == 0) {
            steps++;
        } else {
            triIndex = nextTri;
            if (triIndex == -1) {
                return -1;
            }
            steps++;
            if (steps < 0x20) {
                continue;
            }
            if (steps == 0x20) {
                return -1;
            }
        }

        tri = (s16*)(triBase + triIndex * 14);
        v0 = (s16*)(vertBase + tri[0] * 8);
        v1 = (s16*)(vertBase + tri[1] * 8);
        v2 = (s16*)(vertBase + tri[2] * 8);
        func_8007B07C(v0, v1, v2, point, outNormal);

        *outTriangle = triIndex;

        {
            s32 extraHeight = ((s8)*((u8*)tri + 0x0D)) << 2;
            u32 flags = ((u32*)(uintptr_t)D_800AFB20[0])[*((u8*)tri + 0x0C)];
            s32 blocked = flags & (collisionMask & 0x800000);

            if (*(s16*)(actorData + 0x10) != idx) {
                if (blocked != 0) {
                    *outHeight0 = 0x7FFFFFFF;
                    *outHeight1 = 0x7FFFFFFF;
                    return 0;
                }
                *outHeight0 = point[1];
                func_8007C670(outHeight0, outHeight1, extraHeight);
                return 0;
            }

            if (blocked != 0) {
                *outHeight0 = 0x7FFFFFFF;
                *outHeight1 = 0x7FFFFFFF;
                return 0;
            }

            if (*(s32*)(actorData + 0x30) == 0 &&
                *(s32*)(actorData + 0x34) == 0 &&
                *(s32*)(actorData + 0x38) == 0) {
                *outHeight0 = point[1];
            } else {
                *outHeight0 = *(s16*)(actorData + 0x72);
            }
            func_8007C670(outHeight0, outHeight1, extraHeight);
            return 0;
        }
    }
}
#endif /* XENO_PC_PORT */

extern s32 func_8007D8B4(s32 a, s32 b, s32 c);

void func_8007D818(s32* pVec, VECTOR* pOut) {
    s32 result = func_8007D8B4(pVec[0], pVec[1], pVec[2]);
    pVec[0] >>= 12;
    pVec[1] >>= 12;
    pVec[2] >>= 12;
    if (result < 0) {
        pVec[0] = -pVec[0];
        pVec[1] = -pVec[1];
        pVec[2] = -pVec[2];
    }
    VectorNormal((VECTOR*)pVec, pOut);
}

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/field/nonmatchings/main/misc4", func_8007D8B4);
#else
s32 func_8007D8B4(s32 x, s32 y, s32 z) {
    s32 ax = x < 0 ? -x : x;
    s32 ay = y < 0 ? -y : y;
    s32 az = z < 0 ? -z : z;
    if (ax >= ay) {
        if (ax >= az) return x;
    } else if (ay >= az) {
        return y;
    }
    return z;
}
#endif /* XENO_PC_PORT */
