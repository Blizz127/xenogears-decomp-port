/*
 * Port-only functional implementations of boot-path functions discovered via
 * the oracle (running xeno-port and reading the "[stub] <name>" it stops at).
 *
 * These live here, rather than in the matching tree, when the original lives in
 * a raw asm segment that hasn't been carved into a C translation unit yet.
 * They are correct-by-inspection of the disassembly, NOT byte-matched. When a
 * function later gets a proper home in src/ (matched), delete it from here.
 *
 * Compiled into the port build by pc_port/build_port.sh; defining a symbol here
 * removes it from the auto-generated stub set.
 */

/* The typed vblank callback owner now lives in controller_vblank_dispatch.c. */

/* func_80019548 (asm/slus_006.64/9D24.s): PSX boot tail that restores the
 * hardware stack/global registers before returning. Native PC state is already
 * established by the C runtime, so the port equivalent is intentionally empty. */
void func_80019548(void) {}

/* ---------------------------------------------------------------------------
 * Game-state dispatch table (data migration, Silent-Hill style).
 *
 * On PSX this table is initialized data embedding absolute RAM addresses for
 * each state's memory/heap regions. We rebuild it at runtime: function pointers
 * to the real state mains, and PSX_ADDR() for the mem/heap regions so they live
 * in the emulated PSX RAM buffer. Values extracted from the matching ELF.
 * --------------------------------------------------------------------------- */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#ifdef _WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#endif
#include "common.h"
#include "battle_mips_adapter.h"
#include "main/main.h"
#include "field/actor.h"
#include "field/camera.h"
#include "field/effects.h"
#include "field/main.h"
#include "system/controller.h"
#include "system/font.h"
#include "system/kernel.h"
#include "system/math.h"
#include "system/memory.h"
#include "system/sound.h"
#include "psyq/libcd.h"
#include "psyq/libgte.h"
#include "psyq/libgpu.h"
#include <psx/inline_c.h>
#include <psx/gtereg.h>
#include "model_prim_link.h"
#include "model_polygon_projection.h"
#include "model_polygon_lighting.h"
#include "psx_memory.h"
#include "boot_menu.h"
#include "boot_str.h"
#include "boot_assets.h"

extern void PsyX_TakeScreenshotPath(const char* path)
    __asm__("_Z23PsyX_TakeScreenshotPathPKc");

static void PcPort_BootCapture(const char* name)
{
    const char* dir = getenv("XENO_BOOT_CAPTURE");
    char path[512];
    if (!dir || !dir[0])
        return;
    snprintf(path, sizeof(path), "%s/%s.png", dir, name);
    PsyX_TakeScreenshotPath(path);
    printf("[xeno-port][boot] captured %s\n", path);
    fflush(stdout);
}

/* Dev-harness field capture: when XENO_FIELD_CAPTURE_DIR is set, writes
 * <dir>/field-frame-%06d.png every XENO_FIELD_CAPTURE_EVERY presented frames
 * (default 60). Called from Vsync right after PsyX_EndScene, the same present
 * boundary the world-map capture uses, so the shot reflects the finished
 * frame. Strictly env-gated; zero overhead otherwise. */
unsigned g_PcPortPresentedFrames;
void PcPort_FieldCaptureOnVsync(void)
{
    static int s_init;
    static const char* s_dir;
    static int s_every;
    static unsigned s_frames;
    /* Optional presented-frame window [FROM, TO]; outside it nothing is
     * written. Lets a dense (EVERY=1) capture target one transition without
     * filling the disk with the rest of the run. Both default to "no bound". */
    static unsigned s_from;
    static unsigned s_to;
    char path[512];

    if (!s_init) {
        const char* every;
        const char* from;
        const char* to;
        s_init = 1;
        s_dir = getenv("XENO_FIELD_CAPTURE_DIR");
        s_every = 60;
        every = getenv("XENO_FIELD_CAPTURE_EVERY");
        if (every && *every)
            s_every = atoi(every);
        if (s_every <= 0)
            s_every = 60;
        s_from = 0;
        s_to = 0xFFFFFFFFu;
        from = getenv("XENO_FIELD_CAPTURE_FROM");
        if (from && *from)
            s_from = (unsigned)strtoul(from, NULL, 0);
        to = getenv("XENO_FIELD_CAPTURE_TO");
        if (to && *to)
            s_to = (unsigned)strtoul(to, NULL, 0);
    }
    /* Presented-frame count, exported so other diagnostics (the battle OT
     * histogram in battle_mips_runtime.c) can line up with capture names.
     * Counted whether or not a capture directory is set. */
    s_frames++;
    g_PcPortPresentedFrames = s_frames;
    /* TEST TOOLING: XENO_FIELD_STEP_CHECKPOINTS="s1,s2,..." -- the first frame
     * presented once XENO_FIELD_STEP_PAD_INPUT's step clock equals s is saved
     * as <dir>/step-%06u.png, with its presented-frame and vblank counts. */
    {
        extern int PcPort_FieldStepPadArmedStep(unsigned *step);
        extern int D_80059488;
        static int cp_init;
        static unsigned cp_steps[32], cp_count, cp_next;
        unsigned step;

        if (!cp_init) {
            const char* e = getenv("XENO_FIELD_STEP_CHECKPOINTS");
            cp_init = 1;
            while (e != NULL && *e != '\0' && cp_count < 32) {
                char* end;
                unsigned long v = strtoul(e, &end, 10);
                if (end == e)
                    break;
                cp_steps[cp_count++] = (unsigned)v;
                e = (*end == ',') ? end + 1 : end;
                if (*end != ',')
                    break;
            }
        }
        if (cp_next < cp_count && PcPort_FieldStepPadArmedStep(&step) &&
            step >= cp_steps[cp_next]) {
            const char* dir = getenv("XENO_FIELD_CAPTURE_DIR");
            /* CPU counters at the checkpoint too, so the evidence does not
             * depend on the process exiting cleanly. */
            {
                PcPortMipsExecutionStats st = PcPortMipsExecutionStatsGet();
                extern int g_FrameDeltaTime;
                extern unsigned short D_800AFE9C;
                /* Player position read directly (null checks only): POSDIAG
                 * also gates on the field transition flag, which stays clear
                 * after a field menu, so it cannot serve as evidence there. */
                extern s32 g_PlayerActorIndex;
                int ppos[3] = {0, 0, 0}, pok = 0;
                if (g_FieldActors != NULL) {
                    ActorData* pa = (ActorData*)(uintptr_t)g_FieldActors[g_PlayerActorIndex].pActorData;
                    if (pa != NULL) {
                        ppos[0] = (int)(pa->position.vx >> 0x10);
                        ppos[1] = (int)(pa->position.vy >> 0x10);
                        ppos[2] = (int)(pa->position.vz >> 0x10);
                        pok = 1;
                    }
                }
                fprintf(stderr, "[field-step-pad] checkpoint step=%u at_step=%u presented=%u "
                        "vblank=%d entries=%llu native_bridges=%llu interpreted_instructions=%llu "
                        "disabled_refusals=%llu unresolved_bridges=%llu frame_delta=%d "
                        "c1=0x%04x held=0x%04x player=%s(%d,%d,%d)\n",
                        cp_steps[cp_next], step, s_frames, D_80059488,
                        (unsigned long long)st.entries, (unsigned long long)st.native_bridges,
                        (unsigned long long)st.interpreted_instructions,
                        (unsigned long long)st.disabled_refusals,
                        (unsigned long long)st.unresolved_bridges, g_FrameDeltaTime,
                        (unsigned)g_C1ButtonState, (unsigned)D_800AFE9C,
                        pok ? "" : "unavailable", ppos[0], ppos[1], ppos[2]);
            }
            if (dir != NULL && dir[0] != '\0') {
                snprintf(path, sizeof(path), "%s/step-%06u.png", dir, cp_steps[cp_next]);
                PsyX_TakeScreenshotPath(path);
            }
            cp_next++;
        }
    }
    if (!s_dir || !s_dir[0])
        return;
    /* TEST TOOLING: presents the Fei HD-2D footprint test asks for
     * (fei_hd2d.c, XENO_FEI_HD2D_TEST_CAPTURES), independent of EVERY. */
    {
        extern int PcPort_FeiHd2dTestCaptureWanted(unsigned present) __attribute__((weak));
        if (PcPort_FeiHd2dTestCaptureWanted && PcPort_FeiHd2dTestCaptureWanted(s_frames)) {
            snprintf(path, sizeof(path), "%s/fei-frame-%06u.png", s_dir, s_frames);
            PsyX_TakeScreenshotPath(path);
        }
    }
    if (s_frames < s_from || s_frames > s_to)
        return;
    if ((s_frames % (unsigned)s_every) != 0)
        return;
    snprintf(path, sizeof(path), "%s/field-frame-%06u.png", s_dir, s_frames);
    PsyX_TakeScreenshotPath(path);
    printf("[xeno-port][field] captured %s\n", path);
    fflush(stdout);
}

extern long DisableEvent(long event);
extern long EnableEvent(long event);

extern void KernelMenuMain(void);
extern void KernelMenuInitialize(void);
extern void FieldMain(void);
extern void func_8001B6C4(void);
extern void MenuMain(void);
extern void ChangeGameState(unsigned int state);
extern void FontPrintf(char*, ...);
extern void FontDrawLetters(void*);
extern void SetDispMask(int mask);
extern unsigned short g_C1ButtonStatePressedOnce;
extern unsigned short g_C1ButtonStateReleased;

/* ControllerInit applies the regional face-button layout to the mappings.
 * The input path needs both the mappings and the masks to report presses. */
u_char  g_ControllerButtonMappings[8];
u_short g_ControllerButtonMasks[8];

/* Each game state selects its overlay archive entry here. */
int g_GameStateOverlayArchiveOffsets[NUM_GAME_STATE_OVERLAYS];

/* Main executable .sdata sentinels near 0x8004F308. These are initialized ROM
 * data, not BSS; leaving them as zeroed auto-stubs changes cold field state. */
s32 D_8004F308;
s32 D_8004F324;
s32 D_8004F328;
s32 D_8004F330;
s32 D_8004F334;
s32 D_8004F338;
s32 D_8004F340;
s32 g_GameSceneMapNum;

/* F14 transition capture instrumentation. Opcode 0x56 itself remains a
 * retail-faithful state writer in src/field/main/misc11.c. Retail scripts the
 * departure as FE54 lock -> 0x56 arm -> FieldMain teardown. The first hook
 * records FE54's pre-lock value; the second logs the complete armed state, then
 * restores D_800ADBE4 and (only for an actor/IP-adjacent snapshot) that pre-lock
 * value. Both restores exist solely because the harness suppresses the
 * departure those retail writes assume. F15 removes this hold only after
 * func_8007954C's exit-1 arm and the field->world-map path are scoped. */
u32 g_PcPortOpcode56Func8009FEE4ConditionCount;

static struct {
    s16 controlBefore;
    u16 lockIp;
    s32 actor;
    u8 pending;
} s_Opcode56ControlLockSnapshot;

extern s32 D_800ADBE4;
extern u8 D_800B02C8;
extern void* g_pGameState;

void PcPort_FieldOpcode56RecordControlLock(
    s16 controlBefore, s32 actor, u16 lockIp) {
    /* FE54 (func_80093B10) re-runs itself (IP--) until D_800ADBDC and
     * D_800ADBE4 are both non-zero, so the gate values AT THIS MOMENT decide
     * whether the field departure can ever arm. Print them with the snapshot: a
     * pre-lock control value alone cannot tell a stalled departure from a
     * healthy one (2026-09-18 field-14 investigation). */
    extern s32 D_800ADBDC, D_800ADBE4, D_800ADB2C, D_800ADB90, D_800B00C0;
    s_Opcode56ControlLockSnapshot.controlBefore = controlBefore;
    s_Opcode56ControlLockSnapshot.lockIp = lockIp;
    s_Opcode56ControlLockSnapshot.actor = actor;
    s_Opcode56ControlLockSnapshot.pending = 1;

    fprintf(stderr,
            "[xeno-port][opcode56] FE54 snapshot actor=%d lock_ip=0x%04x "
            "pre_lock_control=%d pending=1 gates: dbdc=%d dbe4=%d db2c=%d "
            "f308=%d db90=%d b00c0=%d\n",
            actor, lockIp, controlBefore, (int)D_800ADBDC, (int)D_800ADBE4,
            (int)D_800ADB2C, (int)D_8004F308, (int)D_800ADB90,
            (int)D_800B00C0);
}

void PcPort_FieldOpcode56TransitionIntercept(
    s32 readinessBefore, u32 refreshConditionCountBefore,
    s32 opcodeActor, u16 opcodeIp) {
    const char* hold = getenv("XENO_FIELD_HOLD_TRANSITION");
    u32 refreshConditionDelta =
        g_PcPortOpcode56Func8009FEE4ConditionCount -
        refreshConditionCountBefore;
    s32 snapshotMatches =
        s_Opcode56ControlLockSnapshot.pending != 0 &&
        s_Opcode56ControlLockSnapshot.actor == opcodeActor &&
        (u16)(s_Opcode56ControlLockSnapshot.lockIp + 1) == opcodeIp;

    fprintf(stderr,
            "[xeno-port][opcode56] at-arm actor=%d opcode_ip=0x%04x "
            "selector=0x%04x heading=0x%04x "
            "unknown231e=0x%04x entrance=0x%04x "
            "D_800B02C8=%u field_control=%d D_800ADBE4=%d "
            "func_8009FEE4_condition=%s count_delta=%u "
            "lock_snapshot=%s\n",
            opcodeActor,
            opcodeIp,
            *(u16*)((u8*)g_pGameState + 0x231A),
            *(u16*)((u8*)g_pGameState + 0x231C),
            *(u16*)((u8*)g_pGameState + 0x231E),
            *(u16*)((u8*)g_pGameState + 0x2320),
            D_800B02C8,
            g_FieldControl.isRandomEncountersEnabled,
            D_800ADBE4,
            refreshConditionDelta != 0 ? "FIRED" : "not-fired",
            refreshConditionDelta,
            snapshotMatches ? "adjacent" : "missing-or-stale");

    if (hold != NULL && hold[0] == '1') {
        D_800ADBE4 = readinessBefore;
        if (snapshotMatches) {
            g_FieldControl.isRandomEncountersEnabled =
                s_Opcode56ControlLockSnapshot.controlBefore;
            fprintf(stderr,
                    "[xeno-port][opcode56] post-restore D_800ADBE4=%d "
                    "field_control=%d snapshot_actor=%d lock_ip=0x%04x; "
                    "held-field teardown and input lock vetoed\n",
                    D_800ADBE4,
                    g_FieldControl.isRandomEncountersEnabled,
                    s_Opcode56ControlLockSnapshot.actor,
                    s_Opcode56ControlLockSnapshot.lockIp);
        } else {
            fprintf(stderr,
                    "[xeno-port][opcode56] post-restore D_800ADBE4=%d; "
                    "control unchanged=%d because FE54 snapshot was "
                    "missing or stale\n",
                    D_800ADBE4,
                    g_FieldControl.isRandomEncountersEnabled);
        }
    }

    s_Opcode56ControlLockSnapshot.pending = 0;
}

/* Main executable .sdata @0x8005917C. Retail stores a pointer to D_80010000;
 * func_8001B6C4/shop setup test *D_8005917C for the boot/media sentinel. */
extern s32 D_80010000;
s32* D_8005917C = &D_80010000;

/* Main executable .sdata @0x8004F32C. This gates whether the field SEDS data is
 * already present in the streamed party-data buffer. Retail initializes it to
 * -1; a zeroed data stub makes func_80085890 memcpy from an absent stream. */
s32 D_8004F32C = -1;

/* Main executable .sdata @0x8004F33C. Current streamed audio bank id; retail
 * starts at -1 so the first field request is not mistaken for already loaded. */
s32 D_8004F33C = -1;

/* Model primitive descriptors associate drawing callbacks with packet builders.
 * The host uses callable function pointers for the supported render paths. */
typedef s32 (*ModelPrimProc)(u8* pCmd, s32 count);
/* Build-pass proc (func_8002C8CC dispatch): fn(D_80059538, D_80059528, shade);
 * must stay layout-identical to the typedef in src/slus_006.64/system/temp2.c. */
typedef s32 (*ModelPrimBuildProc)(u32 pSrc, u32 pCmd, s32 shade);

typedef struct ModelPrimDesc {
    ModelPrimProc proc[6];
    ModelPrimBuildProc buildProc;   /* host packet builder */
    u32 cmdStride;
    u32 packetStride;
    u32 outputStride;
} ModelPrimDesc;

extern s32 func_8002E688(u8* pCmd, s32 count);
extern s32 func_8002F2E0(u8* pCmd, s32 count);
extern s32 func_8002F4B4(u8* pCmd, s32 count);
extern s32 func_8002ED20(u8* pCmd, s32 count); /* model_prim_ed20.c */
extern s32 func_8002FAE8(u8* pCmd, s32 count);
/* Native packet builders used by the uniform descriptor callbacks below. */
extern s32 func_8002CDCC(u8* pSrc, u8* pCmd, s32 shade);
extern s32 func_8002CF34(s32* a0);
extern s32 func_8002CF58(u8* pSrc, u8* pCmd, s32 shade);
extern s32 func_8002D0C0(s32* a0);
extern s32 func_8002D984(u8* pSrc);
extern s32 func_8002D0E4(u8* pSrc);
extern s32 func_8002D814(u8* pColor, s16* pIndices, s32 flags);
/* Rows 0x02 / 0x06 build passes (temp2.c). Retail-matching bodies with the
 * retail signatures: they return 1, and func_8002D6AC takes flags as its
 * third argument (the dispatcher's `shade`). */
extern s32 func_8002D6AC(u8* pColor, s16* pIndices, s32 flags);
extern s32 func_8002D77C(u8* pColor, s16* pIndices);
extern s32 func_8002D530(u8* pColor, s16* pIndices, s32 flags);
extern s32 func_8002DA14(u8* pColor, s16* pIndices);
extern s32 func_8002D180(u8* pColor, s16* pIndices, s32 shade);
extern s32 func_8002D244(u8* pColor, s16* pIndices);
static s32 ModelPrimQuadGT4Variant0(u8* pCmd, s32 count);
static s32 ModelPrimQuadF4Variant0(u8* pCmd, s32 count);
static s32 ModelPrimQuadFT4Variant0(u8* pCmd, s32 count);
static s32 ModelPrimQuadG4Variant0(u8* pCmd, s32 count);
static s32 ModelPrimQuadGT4MaxSZVariant2(u8* pCmd, s32 count);
static s32 ModelPrimQuadG4MaxSZVariant2(u8* pCmd, s32 count);
static s32 ModelPrimQuadF4MaxSZVariant2(u8* pCmd, s32 count);
static s32 ModelPrimQuadF4LitVariant1(u8* pCmd, s32 count);
static s32 ModelPrimTriMediumLitVariant1(u8* pCmd, s32 count);
static s32 ModelPrimQuadGT4MinSZVariant3(u8* pCmd, s32 count);
static s32 ModelPrimQuadG4MinSZVariant3(u8* pCmd, s32 count);
static s32 ModelPrimQuadF4MinSZVariant3(u8* pCmd, s32 count);
static s32 ModelPrimQuadFT4MinSZVariant3(u8* pCmd, s32 count);
static s32 ModelPrimTriSmallAverageVariant0(u8* pCmd, s32 count);
static s32 ModelPrimTriSharedSpine(u8* pCmd, s32 count, u32 packetStep,
                                   u32 vertexStride, u32 tagLen);
static s32 ModelPrimTriSmallMaxSZVariant2(u8* pCmd, s32 count);
static s32 ModelPrimTriGT3MaxSZVariant2(u8* pCmd, s32 count);
static s32 ModelPrimTriMediumMaxSZVariant2(u8* pCmd, s32 count);
static s32 ModelPrimTriAverageVariant0(u8* pCmd, s32 count);
static s32 ModelPrimTriMediumAverageVariant0(u8* pCmd, s32 count);
static s32 ModelPrimTriGT3Variant0(u8* pCmd, s32 count);
static s32 ModelPrimTriMaxSZVariant2(u8* pCmd, s32 count);
static s32 ModelPrimTriGT3MinSZVariant3(u8* pCmd, s32 count);
static s32 ModelPrimTriMediumMinSZVariant3(u8* pCmd, s32 count);
static s32 ModelPrimTriSmallMinSZVariant3(u8* pCmd, s32 count);
static s32 ModelPrimTriMinSZVariant3(u8* pCmd, s32 count);
extern s32 func_80030750(u8* pCmd, s32 count); /* temp2.c */
extern s32 func_8002DAFC(void); /* temp2.c row 0x10 buildProc */
static s32 ModelPrimTriDepthCueVariant4(u8* pCmd, s32 count);
static s32 ModelPrimTriDepthCueMaxSZVariant5(u8* pCmd, s32 count);
static s32 ModelPrimQuadFT4DepthCueVariant4(u8* pCmd, s32 count);
static s32 ModelPrimQuadFT4DepthCueMaxSZVariant5(u8* pCmd, s32 count);

/* F13 dormant-path tripwires.  Retail did not enter either variant-5 body in
 * the current Map001/014/047 probes; retain live counters and first-N entry
 * reports so the first real encounter announces that the deferred FIFO gate
 * must resume.  These are port-only state and do not alter packet payloads. */
u32 g_ModelPrimTriDepthCueMaxSZVariant5EntryCount;
u32 g_ModelPrimQuadFT4DepthCueMaxSZVariant5EntryCount;

static void ModelPrimVariant5Tripwire(const char* name, u32* entryCount,
                                      u8* pCmd, s32 count) {
    *entryCount += 1;
    if (*entryCount <= 4) {
        fprintf(stderr, "[xeno-port][variant5] %s entry=%u cmd=%p count=%d; "
                        "resume F13 retail FIFO/culling gate\n",
                name, *entryCount, (void*)pCmd, count);
    }
}

#include "../include_shim/host_only_bss.h"

ModelPrimDesc D_8004FE50[17] HOST_ONLY_BSS;

static ModelPrimProc ModelPrimWalker(u32 guest)
{
    switch (guest) {
    case 0x8002E010u: return ModelPrimTriGT3Variant0;
    case 0x8002E024u: return ModelPrimTriMediumAverageVariant0;
    case 0x8002E038u: return ModelPrimTriSmallAverageVariant0;
    case 0x8002E04Cu: return ModelPrimTriAverageVariant0;
    case 0x8002E22Cu: return ModelPrimQuadGT4Variant0;
    case 0x8002E240u: return ModelPrimQuadG4Variant0;
    case 0x8002E254u: return ModelPrimQuadF4Variant0;
    case 0x8002E268u: return ModelPrimQuadFT4Variant0;
    case 0x8002E448u: return ModelPrimTriGT3MaxSZVariant2;
    case 0x8002E45Cu: return ModelPrimTriMediumMaxSZVariant2;
    case 0x8002E470u: return ModelPrimTriSmallMaxSZVariant2;
    case 0x8002E484u: return ModelPrimTriMaxSZVariant2;
    case 0x8002E64Cu: return ModelPrimQuadGT4MaxSZVariant2;
    case 0x8002E660u: return ModelPrimQuadG4MaxSZVariant2;
    case 0x8002E674u: return ModelPrimQuadF4MaxSZVariant2;
    case 0x8002E688u: return func_8002E688;
    case 0x8002E8B4u: return ModelPrimTriGT3MinSZVariant3;
    case 0x8002E8C8u: return ModelPrimTriMediumMinSZVariant3;
    case 0x8002E8DCu: return ModelPrimTriSmallMinSZVariant3;
    case 0x8002E8F0u: return ModelPrimTriMinSZVariant3;
    case 0x8002EAB8u: return ModelPrimQuadGT4MinSZVariant3;
    case 0x8002EACCu: return ModelPrimQuadG4MinSZVariant3;
    case 0x8002EAE0u: return ModelPrimQuadF4MinSZVariant3;
    case 0x8002EAF4u: return ModelPrimQuadFT4MinSZVariant3;
    case 0x8002ED20u: return func_8002ED20;
    case 0x8002EF0Cu: return ModelPrimTriDepthCueVariant4;
    case 0x8002F0E4u: return ModelPrimTriDepthCueMaxSZVariant5;
    case 0x8002F2E0u: return func_8002F2E0;
    case 0x8002F4B4u: return func_8002F4B4;
    case 0x8002F6B4u: return ModelPrimTriMediumLitVariant1;
    case 0x8002F8D0u: return ModelPrimQuadF4LitVariant1;
    case 0x8002FAE8u: return func_8002FAE8;
    case 0x8002FCFCu: return ModelPrimQuadFT4DepthCueVariant4;
    case 0x8002FF0Cu: return ModelPrimQuadFT4DepthCueMaxSZVariant5;
    case 0x80030750u: return func_80030750;
    default: return NULL;
    }
}

#include "cleanroom/model_packet_callbacks.inc"

static ModelPrimBuildProc ModelPrimBuilder(u32 guest)
{
    switch (guest) {
    case 0x8002CDCCu: return ModelBuild8002CDCC;
    case 0x8002CF34u: return ModelBuild8002CF34;
    case 0x8002CF58u: return ModelBuild8002CF58;
    case 0x8002D0C0u: return ModelBuild8002D0C0;
    case 0x8002D0E4u: return ModelBuild8002D0E4;
    case 0x8002D180u: return ModelBuild8002D180;
    case 0x8002D244u: return ModelBuild8002D244;
    case 0x8002D530u: return ModelBuild8002D530;
    case 0x8002D6ACu: return ModelBuild8002D6AC;
    case 0x8002D77Cu: return ModelBuild8002D77C;
    case 0x8002D814u: return ModelBuild8002D814;
    case 0x8002D984u: return ModelBuild8002D984;
    case 0x8002DA14u: return ModelBuild8002DA14;
    case 0x8002DAFCu: return ModelBuild8002DAFC;
    default: return NULL;
    }
}


/* Initialized renderer bounds/depth-shift globals adjacent to D_8004FE50. */
s32 D_800500F8;
s32 D_800500FC;
s32 D_80050100;
s32 D_80050104;

/* Main-exe BSS consumed by the decompiled encounter roll func_80079288
 * (src/field/main/misc4.c): D_80059508 = battle-transition param.
 * D_800594F8 now lives in src/slus_006.64/system/temp3.c (matching owner).
 *
 * D_80065ADC (the 16 per-formation encounter WEIGHTS) is deliberately NOT
 * defined here any more. Retail stores it as the tail of ONE contiguous object
 * beginning at D_800658DC -- 16 formation records of 0x20 bytes, then the
 * weights at +0x200, which is exactly the address 0x80065ADC. FieldLoad
 * decompresses that whole section to D_800658DC in a single call
 * (src/field/main/misc3.c:831-833), so a standalone 16-byte object here could
 * never receive the weights: the entire decode landed inside D_800658DC, the
 * sum stayed 0, and no encounter ever fired. D_80065ADC is now an alias at
 * g_SlusBss_800658DC + 0x200 -- see pc_port/src/data_field.c and
 * docs/evidence/encounter-weight-aliasing-20260906/README.md. */
u8 D_80059508;

/* 0xBC sub-command 0x26's track table (animation_scripts.c): 16 entries of
 * 0x1C bytes. Sized properly here so the port build doesn't fall back to a
 * 32-byte auto-stub (index reaches 15*0x1C+0x10). Zero until its authoring
 * subsystem is ported; the sub-command then yields a zero vector. */
u8 D_800C3EB0[0x1C0];

/* func_800AD898 (asm/field/3D334.s 3DDA8-3DE84, unsplit region -- port
 * implementation here like the other asm-only field fns): when the field
 * flag D_800B2268 is set, walk the three party slots (D_8005A444); for each
 * live member whose game-state byte at +0x22B1+slot equals 1, set flag
 * 0x200 and clear 0x500 on the member's actor data.  Called each frame by
 * func_800A24C4's party-skin branch. */
extern s32 D_800B2268;
extern void* g_pGameState;
extern s32 D_8005A444[3];
extern FieldActor* volatile g_FieldActors;
void func_800AD898(void)
{
    s32 i;

    if (D_800B2268 == 0) {
        return;
    }
    for (i = 0; i < 3; i++) {
        s32 idx = D_8005A444[i];
        u8* actorData;

        if (idx == 0xFF) {
            continue;
        }
        if (*((u8*)g_pGameState + 0x22B1 + i) != 1) {
            continue;
        }
        actorData = (u8*)(uintptr_t)
            *(u32*)((u8*)g_FieldActors + idx * 0x5C + 0x4C);
        *(u32*)(actorData + 0x00) |= 0x200;
        *(u32*)(actorData + 0x00) &= ~0x500u;
    }
}

/* ---- PC-HDD developer debug overlay (retail 0x80280000) ---------------------
 * field.bin calls ten functions at 0x8028xxxx. They are not field code: they
 * are the developer debug overlay, archive directory 4 entry 0xAD (25032 bytes
 * on disc 1), which FieldMain reads to the raw address 0x80280000 -- past the
 * retail console's 2 MB, i.e. a DTL-H 8 MB dev-kit module -- only when
 * g_FieldSystemMode == SYSTEM_MODE_PC_HDD. Its rodata is the debug HUD
 * ("CPU=%04d GPU=%04d", "---------- Player Info -----", ...).
 *   func_80281B00(label)  CPU profiler section mark ("MATRIX", "FntPrint", ...)
 *   func_80281204(n)      per-formation encounter counter (NOT the battle
 *                         handoff -- func_80079288 does that itself, earlier)
 *   func_802811EC/8028125C/802812A4  profiler/HUD state resets
 *   func_80281400/80281450  HUD draw submit; func_802815B0/80281678/80284EA4
 *                         camera/actor/event debug readouts
 * Retail never runs any of them: D_80010000 is 0xFFFFFFFF on the disc, so
 * FieldMain always selects SYSTEM_MODE_CD_ROM, and every call site (28 in
 * field.bin, 25 in src/field) sits behind `if (g_FieldSystemMode == 0)`.
 * Proof, from the shipped bytes, with negative controls:
 * pc_port/tests/run_field_debug_overlay_unreachable_test.sh.
 *
 * So there is no retail behaviour to port. Reaching one of these means the
 * port broke the CD-ROM-mode invariant (or FieldMain was bypassed while
 * g_FieldSystemMode still held its zero-initialised PC-HDD value); on hardware
 * that jal would land in unloaded memory. Fail closed and name the caller
 * instead of the generated stubs' silent no-op. */
static void FieldDebugOverlayUnreachable(const char* name) __attribute__((noreturn));
static void FieldDebugOverlayUnreachable(const char* name) {
    fprintf(stderr,
            "[xeno-port] FATAL: %s is in the PC-HDD debug overlay (archive 4/0xAD "
            "at 0x80280000), reachable only when g_FieldSystemMode == "
            "SYSTEM_MODE_PC_HDD; retail never loads it. g_FieldSystemMode=%d\n",
            name, (int)g_FieldSystemMode);
    abort();
}
void func_802811EC(void) { FieldDebugOverlayUnreachable("func_802811EC"); }
void func_80281204(s32 formationIndex) { (void)formationIndex; FieldDebugOverlayUnreachable("func_80281204"); }
void func_8028125C(void) { FieldDebugOverlayUnreachable("func_8028125C"); }
void func_802812A4(void) { FieldDebugOverlayUnreachable("func_802812A4"); }
void func_80281400(void) { FieldDebugOverlayUnreachable("func_80281400"); }
void func_80281450(void) { FieldDebugOverlayUnreachable("func_80281450"); }
void func_802815B0(void) { FieldDebugOverlayUnreachable("func_802815B0"); }
void func_80281678(void* actorData) { (void)actorData; FieldDebugOverlayUnreachable("func_80281678"); }
void func_80281B00(void* label) { (void)label; FieldDebugOverlayUnreachable("func_80281B00"); }
void func_80284EA4(void) { FieldDebugOverlayUnreachable("func_80284EA4"); }

extern u8* D_80059424;
extern u32 D_8005953C;
extern u32 D_80059498; /* lighting-record cursor (func_8002F8D0/F6B4) */
extern u32 D_8005952C; /* lit-walker normal table (func_8002F6B4) */
extern uint8_t g_PsxScratchpad[];
extern u32 D_80059568;
extern s32 D_80059578;

static u32 ModelPrimVertexIndex1(u32 word) {
    return (word >> 16) & 0xFFFF;
}

/* Retail screen cull (e.g. asm 8002E15C-8002E1A0): keep a prim only when at
 * least one vertex has packed SXY <u D_800500FC (y within [0, screenH-1], so
 * negative or below-screen y fails) AND at least one vertex has
 * x <u D_800500F8. An earlier port transcription inverted the first
 * comparison, culling exactly the on-screen prims. */
static int ModelPrimTriOverlapsScreen(u32 xy0, u32 xy1, u32 xy2) {
    u32 yMaxPacked = (u32)D_800500FC;
    u32 xMax = (u32)D_800500F8;

    if (!(xy0 < yMaxPacked || xy1 < yMaxPacked || xy2 < yMaxPacked)) {
        return 0;
    }
    return (((xy0 & 0xFFFF) < xMax) || ((xy1 & 0xFFFF) < xMax) ||
            ((xy2 & 0xFFFF) < xMax));
}

static int ModelPrimQuadOverlapsScreen(u32 xy0, u32 xy1, u32 xy2, u32 xy3) {
    u32 yMaxPacked = (u32)D_800500FC;
    u32 xMax = (u32)D_800500F8;

    if (!(xy0 < yMaxPacked || xy1 < yMaxPacked || xy2 < yMaxPacked || xy3 < yMaxPacked)) {
        return 0;
    }
    return (((xy0 & 0xFFFF) < xMax) || ((xy1 & 0xFFFF) < xMax) ||
            ((xy2 & 0xFFFF) < xMax) || ((xy3 & 0xFFFF) < xMax));
}

/* -------------------------------------------------------------------------
 * XENO_CULL_CAM_LOG — temporary interactive camera+cull logger (Lane A).
 * Enable with XENO_CULL_CAM_LOG=1. Writes gated lines to
 *   captures/render_diag/cullcam_<timestamp>.log
 * (override with XENO_CULL_CAM_LOG_PATH). Press SELECT to stamp a MARK line
 * at the moment of a repro. Off by default; remove after the session.
 *
 * Log field "nclip_backface" counts GTE NCLIP / NormalClip backface-winding
 * drops (OPZ < 0 on the three screen-space verts). It is NOT near-plane clip
 * (that lives in the FLAG / otz guards). Optional NCLIP_SXY / FLAG_SAMPLE
 * lines dump a few drops at the known well pose (or after MARK).
 * ------------------------------------------------------------------------- */
enum {
    CC_SEEN = 0,
    CC_EMIT,
    CC_FLAG,
    CC_OTZ,
    /* CC_NCLIP_BACKFACE: NormalClip/NCLIP OPZ<0 (screen winding), not near-Z. */
    CC_NCLIP_BACKFACE,
    CC_OVERLAP,
    CC_OVERSIZE,
    CC_GTE31,       /* FLAG bit 31 set (sign-extend path relevant) */
    CC_QF4_SEEN,    /* ModelPrimQuadF4Variant0 = retail 0x08/0x0C walker */
    CC_QF4_FLAG,
    CC_QF4_EMIT,
    CC_N
};

/* Keep old enumerator name as alias so call sites stay readable if any remain. */
#define CC_NCLIP CC_NCLIP_BACKFACE

static int s_ccOn = -1;
static u32 s_cc[CC_N];
static u32 s_ccFrame;
static FILE* s_ccFile;
static u32 s_ccNclipSamples;
static u32 s_ccFlagSamples;
static u32 s_ccMarkArmFrames; /* after SELECT, allow samples for a few summary frames */

enum { CC_NCLIP_SAMPLE_MAX = 48, CC_FLAG_SAMPLE_MAX = 48 };

static int CullCamOn(void) {
    if (s_ccOn < 0) {
        const char* e = getenv("XENO_CULL_CAM_LOG");
        s_ccOn = (e != NULL && e[0] != '\0' && e[0] != '0') ? 1 : 0;
    }
    return s_ccOn;
}

/* Known failing pose from MARK session (cullcam_20260711_184135). */
static int CullCamAtWellPose(void) {
    extern VECTOR g_CameraEye2;
    extern CameraInterpolation g_CamInterpolation;
    int ex = (int)(g_CameraEye2.vx >> 16);
    int ey = (int)(g_CameraEye2.vy >> 16);
    int ez = (int)(g_CameraEye2.vz >> 16);
    int ay = (int)g_CamInterpolation.curAngleY;
    if (ay != -1536) {
        return 0;
    }
    if (ex < -564 - 12 || ex > -564 + 12) {
        return 0;
    }
    if (ey < 1491 - 24 || ey > 1491 + 24) {
        return 0;
    }
    if (ez < 512 - 12 || ez > 512 + 12) {
        return 0;
    }
    return 1;
}

static void CullCamSampleNclipDrop(long xy0, long xy1, long xy2, long opz,
                                   const SVECTOR* v0, const SVECTOR* v1,
                                   const SVECTOR* v2, int isQf4) {
    extern VECTOR g_CameraEye2;
    extern CameraInterpolation g_CamInterpolation;
    int sx0, sy0, sx1, sy1, sx2, sy2;

    if (!CullCamOn() || s_ccFile == NULL) {
        return;
    }
    if (s_ccNclipSamples >= CC_NCLIP_SAMPLE_MAX) {
        return;
    }
    if (!CullCamAtWellPose() && s_ccMarkArmFrames == 0) {
        return;
    }

    sx0 = (int)(short)(xy0 & 0xFFFF);
    sy0 = (int)(short)((xy0 >> 16) & 0xFFFF);
    sx1 = (int)(short)(xy1 & 0xFFFF);
    sy1 = (int)(short)((xy1 >> 16) & 0xFFFF);
    sx2 = (int)(short)(xy2 & 0xFFFF);
    sy2 = (int)(short)((xy2 >> 16) & 0xFFFF);

    fprintf(s_ccFile,
            "NCLIP_SXY f=%u qf4=%d opz=%ld "
            "sxy0=%d,%d sxy1=%d,%d sxy2=%d,%d "
            "w0=%d,%d,%d w1=%d,%d,%d w2=%d,%d,%d "
            "eye2=%d,%d,%d angY=%d\n",
            s_ccFrame, isQf4, (long)opz,
            sx0, sy0, sx1, sy1, sx2, sy2,
            v0 ? (int)v0->vx : 0, v0 ? (int)v0->vy : 0, v0 ? (int)v0->vz : 0,
            v1 ? (int)v1->vx : 0, v1 ? (int)v1->vy : 0, v1 ? (int)v1->vz : 0,
            v2 ? (int)v2->vx : 0, v2 ? (int)v2->vy : 0, v2 ? (int)v2->vz : 0,
            (int)(g_CameraEye2.vx >> 16), (int)(g_CameraEye2.vy >> 16),
            (int)(g_CameraEye2.vz >> 16),
            (int)g_CamInterpolation.curAngleY);
    s_ccNclipSamples++;
    if (s_ccNclipSamples == 1 || (s_ccNclipSamples % 8) == 0) {
        fprintf(stderr, "[cull-cam] NCLIP_SXY sample %u/%u opz=%ld sxy=(%d,%d)(%d,%d)(%d,%d)\n",
                s_ccNclipSamples, (unsigned)CC_NCLIP_SAMPLE_MAX, (long)opz,
                sx0, sy0, sx1, sy1, sx2, sy2);
    }
}

/* Compact FLAG bit tags (PsyCross / nocash GTE FLAG). Bit31 = retail bltz. */
static void CullCamFormatFlagBits(u32 flag, char* out, size_t outSz) {
    static const struct { u32 bit; const char* name; } kBits[] = {
        { 31, "E" },    { 30, "MAC3" }, { 29, "MAC2" }, { 28, "MAC1" },
        { 27, "A1lo" }, { 26, "A2lo" }, { 25, "A3lo" },
        { 24, "IR1" },  { 23, "IR2" },  { 22, "IR3" },
        { 18, "SZ3" },  { 17, "DIV" },  { 16, "MAC0+" }, { 15, "MAC0-" },
        { 14, "SX" },   { 13, "SY" },   { 12, "H?" },
    };
    size_t n = 0;
    size_t i;
    out[0] = '\0';
    for (i = 0; i < sizeof(kBits) / sizeof(kBits[0]); i++) {
        if (flag & (1u << kBits[i].bit)) {
            int wrote = snprintf(out + n, outSz > n ? outSz - n : 0, "%s%s",
                                 n ? "," : "", kBits[i].name);
            if (wrote > 0) {
                n += (size_t)wrote;
            }
        }
    }
    if (n == 0) {
        snprintf(out, outSz, "-");
    }
}

static void CullCamSampleFlagDrop(long flag, long otz,
                                  long xy0, long xy1, long xy2, long xy3,
                                  const SVECTOR* v0, const SVECTOR* v1,
                                  const SVECTOR* v2, const SVECTOR* v3,
                                  const char* gteOp, int isQf4, int nVert) {
    extern VECTOR g_CameraEye2;
    extern CameraInterpolation g_CamInterpolation;
    char bits[96];
    int sx0, sy0, sx1, sy1, sx2, sy2, sx3, sy3;
    u32 f = (u32)flag;

    if (!CullCamOn() || s_ccFile == NULL) {
        return;
    }
    if (s_ccFlagSamples >= CC_FLAG_SAMPLE_MAX) {
        return;
    }
    if (!CullCamAtWellPose() && s_ccMarkArmFrames == 0) {
        return;
    }

    CullCamFormatFlagBits(f, bits, sizeof(bits));
    sx0 = (int)(short)(xy0 & 0xFFFF);
    sy0 = (int)(short)((xy0 >> 16) & 0xFFFF);
    sx1 = (int)(short)(xy1 & 0xFFFF);
    sy1 = (int)(short)((xy1 >> 16) & 0xFFFF);
    sx2 = (int)(short)(xy2 & 0xFFFF);
    sy2 = (int)(short)((xy2 >> 16) & 0xFFFF);
    sx3 = (int)(short)(xy3 & 0xFFFF);
    sy3 = (int)(short)((xy3 >> 16) & 0xFFFF);

    fprintf(s_ccFile,
            "FLAG_SAMPLE f=%u qf4=%d gte=%s flag=0x%08x bits=%s otz=%ld "
            "sxy0=%d,%d sxy1=%d,%d sxy2=%d,%d sxy3=%d,%d "
            "w0=%d,%d,%d w1=%d,%d,%d w2=%d,%d,%d w3=%d,%d,%d "
            "eye2=%d,%d,%d angY=%d\n",
            s_ccFrame, isQf4, gteOp ? gteOp : "?", f, bits, (long)otz,
            sx0, sy0, sx1, sy1, sx2, sy2,
            nVert >= 4 ? sx3 : 0, nVert >= 4 ? sy3 : 0,
            v0 ? (int)v0->vx : 0, v0 ? (int)v0->vy : 0, v0 ? (int)v0->vz : 0,
            v1 ? (int)v1->vx : 0, v1 ? (int)v1->vy : 0, v1 ? (int)v1->vz : 0,
            v2 ? (int)v2->vx : 0, v2 ? (int)v2->vy : 0, v2 ? (int)v2->vz : 0,
            (nVert >= 4 && v3) ? (int)v3->vx : 0,
            (nVert >= 4 && v3) ? (int)v3->vy : 0,
            (nVert >= 4 && v3) ? (int)v3->vz : 0,
            (int)(g_CameraEye2.vx >> 16), (int)(g_CameraEye2.vy >> 16),
            (int)(g_CameraEye2.vz >> 16),
            (int)g_CamInterpolation.curAngleY);
    s_ccFlagSamples++;
    if (s_ccFlagSamples == 1 || (s_ccFlagSamples % 8) == 0) {
        fprintf(stderr, "[cull-cam] FLAG_SAMPLE %u/%u flag=0x%08x bits=%s gte=%s otz=%ld\n",
                s_ccFlagSamples, (unsigned)CC_FLAG_SAMPLE_MAX, f, bits,
                gteOp ? gteOp : "?", (long)otz);
    }
}

static void CullCamSeen(int isQf4, long flag) {
    if (!CullCamOn()) {
        return;
    }
    s_cc[CC_SEEN]++;
    if (isQf4) {
        s_cc[CC_QF4_SEEN]++;
    }
    if ((u32)flag & 0x80000000u) {
        s_cc[CC_GTE31]++;
    }
}

static void CullCamDrop(int reason, int isQf4) {
    if (!CullCamOn()) {
        return;
    }
    s_cc[reason]++;
    if (isQf4 && reason == CC_FLAG) {
        s_cc[CC_QF4_FLAG]++;
    }
}

static void CullCamEmit(int isQf4) {
    if (!CullCamOn()) {
        return;
    }
    s_cc[CC_EMIT]++;
    if (isQf4) {
        s_cc[CC_QF4_EMIT]++;
    }
}

void PcPort_CullCamLogOnVsync(void) {
    extern VECTOR g_CameraEye2;
    extern VECTOR g_CameraAt2;
    extern VECTOR g_CameraEye;
    extern VECTOR g_CameraAt;
    extern CameraInterpolation g_CamInterpolation;
    extern FieldActor* volatile g_FieldActors;
    extern s32 g_PlayerActorIndex;
    extern u_short g_C1ButtonStateReleased;
    extern FieldScene g_Scene;
    static u16 s_prevSelect;
    u16 selectEdge;
    s32 feiX = 0, feiY = 0, feiZ = 0;
    u32 feiStatus = 0, feiFlags4 = 0;
    int mark;

    if (!CullCamOn()) {
        return;
    }

    if (s_ccFile == NULL) {
        const char* path = getenv("XENO_CULL_CAM_LOG_PATH");
        char autoPath[256];
        if (path == NULL || path[0] == '\0') {
            time_t now = time(NULL);
            struct tm* t = localtime(&now);
            snprintf(autoPath, sizeof(autoPath),
                     "captures/render_diag/"
                     "cullcam_%04d%02d%02d_%02d%02d%02d.log",
                     t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
                     t->tm_hour, t->tm_min, t->tm_sec);
            path = autoPath;
            /* Relative to the working directory: no build-machine path in the binary. */
#ifdef _WIN32
            _mkdir("captures");
            _mkdir("captures/render_diag");
#else
            mkdir("captures", 0755);
            mkdir("captures/render_diag", 0755);
#endif
        }
        s_ccFile = fopen(path, "w");
        if (s_ccFile == NULL) {
            fprintf(stderr, "[cull-cam] FAILED to open log path '%s'\n", path);
            s_ccOn = 0;
            return;
        }
        setvbuf(s_ccFile, NULL, _IOLBF, 0);
        fprintf(stderr, "[cull-cam] logging to %s (SELECT=MARK)\n", path);
        fprintf(s_ccFile,
                "# cull-cam log: f eye2XYZ(>>16) eyeXYZ(>>16) at2XYZ(>>16) angY camRotXYZ "
                "feiXYZ(>>16) feiStatus feiFlags4 otEmit | "
                "seen emit flag otz nclip_backface(=NCLIP/OPZ winding, NOT near-Z) "
                "overlap oversize gte31 qf4seen qf4flag qf4emit\n"
                "# NCLIP_SXY lines: sample of backface drops at well pose "
                "(eye2~-564,1491,512 angY=-1536) or after MARK — SXY + OPZ + model-space verts\n"
                "# FLAG_SAMPLE lines: sample of flag<0 drops at same pose — raw FLAG bits + "
                "preceding RotTransPers* (RTPT3/RTPT4) + SXY + model verts\n"
                "# CAMZ lines: camera-zone / clamp gate — unk48, bit4000 (skip eye-Y "
                "mesh clamp), sceneDIP/SCRZ/scale, camMode, meshY at eye2 XZ, enc\n");
    }

    selectEdge = (u16)(g_C1ButtonStateReleased & CTRL_BTN_SELECT);
    mark = (selectEdge != 0 && s_prevSelect == 0);
    s_prevSelect = selectEdge;

    /* Field busy-waits on Vsync(-1) and also hits Vsync(1) pre-draw; logging every
     * call produced multi-GB logs of all-zero cull rows. Only emit when the user
     * MARKs or the instrumented walkers actually saw prims this frame. */
    if (!mark && s_cc[CC_SEEN] == 0 && s_cc[CC_QF4_SEEN] == 0) {
        if (s_ccMarkArmFrames > 0) {
            s_ccMarkArmFrames--;
        }
        return;
    }

    if (g_FieldActors != NULL && g_PlayerActorIndex >= 0) {
        FieldActor* fa = &g_FieldActors[g_PlayerActorIndex];
        feiStatus = (u32)(u16)fa->status;
        if (fa->pActorData != 0) {
            u8* ad = (u8*)(uintptr_t)fa->pActorData;
            feiFlags4 = *(u32*)(ad + 0x04);
            feiX = *(s32*)(ad + 0x20) >> 16;
            feiY = *(s32*)(ad + 0x24) >> 16;
            feiZ = *(s32*)(ad + 0x28) >> 16;
        }
    }

    if (mark) {
        fprintf(s_ccFile, "MARK f=%u <<< user SELECT — repro moment\n", s_ccFrame);
        fprintf(stderr, "[cull-cam] MARK f=%u\n", s_ccFrame);
        s_ccMarkArmFrames = 120; /* allow NCLIP_SXY for a short window after MARK */
    }
    if (s_ccMarkArmFrames > 0) {
        s_ccMarkArmFrames--;
    }

    /* CAMZ: once per well-pose (or MARK) — clamp-gate + sceneDIP/SCRZ + mesh probe.
     * Does NOT modify camera state; re-probes the same path as func_80073230. */
    {
        static int s_camzLogged;
        int atPose = CullCamAtWellPose();
        /* Also sample the first few activity frames so spawn/approach state is
         * visible even before the well pose (unk48 / DIP / meshY). */
        if ((atPose || mark || s_camzLogged < 4) && s_camzLogged < 12) {
            extern s16 g_FieldCameraMode;
            extern s16 D_800AFB54;
            extern FieldControl g_FieldControl;
            extern s16 func_8007B1C4(s16 a0, s16 a1, s32 a2, void* a3, void* a4);
            s16 meshPoint[4];
            s32 meshNormal[4];
            s32 unk48 = g_Scene.unk48;
            int bit4000 = (unk48 & 0x4000) != 0;
            s16 dip = *(s16*)((u8*)&g_Scene + 0x6C);
            s32 scrz = *(s32*)((u8*)&g_Scene + 0x68);
            s16 scale = *(s16*)((u8*)&g_Scene + 0x6E);
            s16 sceneAng56 = *(s16*)((u8*)&g_Scene + 0x56);
            int e2x = (int)(g_CameraEye2.vx >> 16);
            int e2y = (int)(g_CameraEye2.vy >> 16);
            int e2z = (int)(g_CameraEye2.vz >> 16);
            int meshY = 0x7FFF;
            int meshRet = -1;
            int wouldSnap = 0;
            meshPoint[0] = meshPoint[1] = meshPoint[2] = meshPoint[3] = 0;
            meshNormal[0] = meshNormal[1] = meshNormal[2] = meshNormal[3] = 0;
            meshRet = (int)func_8007B1C4((s16)e2x, (s16)e2z, D_800AFB54 - 1,
                                         meshPoint, meshNormal);
            meshY = (int)meshPoint[1];
            wouldSnap = (!bit4000 && meshY < e2y) ? 1 : 0;
            fprintf(s_ccFile,
                    "CAMZ f=%u unk48=0x%08x bit4000=%d camMode=%d AFB54=%d "
                    "DIP=%d SCRZ=%d scale=%d scene56=%d "
                    "eye2=%d,%d,%d meshY=%d meshRet=%d wouldSnap=%d "
                    "eye=%d,%d,%d at2=%d,%d,%d angY=%d enc=%d\n",
                    s_ccFrame, (unsigned)unk48, bit4000, (int)g_FieldCameraMode,
                    (int)D_800AFB54, (int)dip, (int)scrz, (int)scale,
                    (int)sceneAng56, e2x, e2y, e2z, meshY, meshRet, wouldSnap,
                    (int)(g_CameraEye.vx >> 16), (int)(g_CameraEye.vy >> 16),
                    (int)(g_CameraEye.vz >> 16),
                    (int)(g_CameraAt2.vx >> 16), (int)(g_CameraAt2.vy >> 16),
                    (int)(g_CameraAt2.vz >> 16),
                    (int)g_CamInterpolation.curAngleY,
                    (int)g_FieldControl.isRandomEncountersEnabled);
            fprintf(stderr,
                    "[cull-cam] CAMZ bit4000=%d DIP=%d SCRZ=%d meshY=%d eye2y=%d "
                    "wouldSnap=%d\n",
                    bit4000, (int)dip, (int)scrz, meshY, e2y, wouldSnap);
            s_camzLogged++;
        }
    }

    {
    extern s16 g_FieldCameraMode;
    extern u16 g_CamMovementFlags;
    extern VECTOR g_CamEyeMovementCurrent;
    fprintf(s_ccFile,
            "f=%u camMode=%d camFlags=%04x eyeCur=%d,%d,%d "
            "eye2=%d,%d,%d eye=%d,%d,%d at2=%d,%d,%d angY=%d camRot=%d,%d,%d "
            "fei=%d,%d,%d st=%04x f4=%08x otEmit=%d | "
            "seen=%u emit=%u flag=%u otz=%u nclip_backface=%u overlap=%u oversize=%u "
            "gte31=%u qf4seen=%u qf4flag=%u qf4emit=%u\n",
            s_ccFrame,
            /* TEMP-DIAG: g_FieldCameraMode selects the eye source in
             * func_80073230 (misc2.c) -- 0/2 follow-cam, 1 scripted eye from
             * g_CamEyeMovementCurrent.  Logged to tell "camera never armed"
             * apart from "geometry missing" on maps that render black. */
            (int)g_FieldCameraMode,
            (unsigned)g_CamMovementFlags,
            (int)(g_CamEyeMovementCurrent.vx >> 16),
            (int)(g_CamEyeMovementCurrent.vy >> 16),
            (int)(g_CamEyeMovementCurrent.vz >> 16),
            (int)(g_CameraEye2.vx >> 16), (int)(g_CameraEye2.vy >> 16),
            (int)(g_CameraEye2.vz >> 16),
            (int)(g_CameraEye.vx >> 16), (int)(g_CameraEye.vy >> 16),
            (int)(g_CameraEye.vz >> 16),
            (int)(g_CameraAt2.vx >> 16), (int)(g_CameraAt2.vy >> 16),
            (int)(g_CameraAt2.vz >> 16),
            (int)g_CamInterpolation.curAngleY,
            (int)g_Scene.camRotation.vx, (int)g_Scene.camRotation.vy,
            (int)g_Scene.camRotation.vz,
            feiX, feiY, feiZ, (unsigned)feiStatus, feiFlags4,
            (int)D_80059578,
            s_cc[CC_SEEN], s_cc[CC_EMIT], s_cc[CC_FLAG], s_cc[CC_OTZ],
            s_cc[CC_NCLIP_BACKFACE], s_cc[CC_OVERLAP], s_cc[CC_OVERSIZE],
            s_cc[CC_GTE31], s_cc[CC_QF4_SEEN], s_cc[CC_QF4_FLAG],
            s_cc[CC_QF4_EMIT]);
    }

    {
        int i;
        for (i = 0; i < CC_N; i++) {
            s_cc[i] = 0;
        }
    }
    s_ccFrame++;
}

static s32 ModelPrimTriSmallAverageVariant0(u8* pCmd, s32 count) {
    return ModelPrimTriSharedSpine(pCmd, count, 0x14, 0x4, 0x04000000);
}

static s32 ModelPrimTriMaxSZSharedSpine(u8* pCmd, s32 count, u32 packetStep,
                                        u32 vertexStride, u32 tagLen,
                                        int selectMin) {
    (void)tagLen;
    return PcPort_DrawModelPolygons(pCmd, count, 3, packetStep, vertexStride,
                                 selectMin ? PCPORT_MODEL_DEPTH_MINIMUM : PCPORT_MODEL_DEPTH_MAXIMUM);
}

static s32 ModelPrimTriGT3MaxSZVariant2(u8* pCmd, s32 count) {
    return ModelPrimTriMaxSZSharedSpine(pCmd, count, 0x28, 0xC, 0x09000000, 0);
}

static s32 ModelPrimTriMediumMaxSZVariant2(u8* pCmd, s32 count) {
    return ModelPrimTriMaxSZSharedSpine(pCmd, count, 0x1C, 0x8, 0x06000000, 0);
}

static s32 ModelPrimTriSmallMaxSZVariant2(u8* pCmd, s32 count) {
    return ModelPrimTriMaxSZSharedSpine(pCmd, count, 0x14, 0x4, 0x04000000, 0);
}

static s32 ModelPrimTriMaxSZVariant2(u8* pCmd, s32 count) {
    return ModelPrimTriMaxSZSharedSpine(pCmd, count, 0x20, 0x8, 0x07000000, 0);
}

static s32 ModelPrimTriGT3MinSZVariant3(u8* pCmd, s32 count) {
    return ModelPrimTriMaxSZSharedSpine(pCmd, count, 0x28, 0xC, 0x09000000, 1);
}

static s32 ModelPrimTriMediumMinSZVariant3(u8* pCmd, s32 count) {
    return ModelPrimTriMaxSZSharedSpine(pCmd, count, 0x1C, 0x8, 0x06000000, 1);
}

static s32 ModelPrimTriSmallMinSZVariant3(u8* pCmd, s32 count) {
    return ModelPrimTriMaxSZSharedSpine(pCmd, count, 0x14, 0x4, 0x04000000, 1);
}

static s32 ModelPrimTriMinSZVariant3(u8* pCmd, s32 count) {
    return ModelPrimTriMaxSZSharedSpine(pCmd, count, 0x20, 0x8, 0x07000000, 1);
}

static s32 ModelPrimQuadSharedSpine(u8* pCmd, s32 count, u32 packetStep,
                                    u32 vertexStride, u32 tagLen) {
    (void)tagLen;
    return PcPort_DrawModelPolygons(pCmd, count, 4, packetStep, vertexStride,
                                 PCPORT_MODEL_DEPTH_AVERAGE);
}

static s32 ModelPrimQuadGT4Variant0(u8* pCmd, s32 count) {
    return ModelPrimQuadSharedSpine(pCmd, count, 0x34, 0xC, 0x0C000000);
}

static s32 ModelPrimQuadG4Variant0(u8* pCmd, s32 count) {
    return ModelPrimQuadSharedSpine(pCmd, count, 0x24, 0x8, 0x08000000);
}

static s32 ModelPrimQuadF4Variant0(u8* pCmd, s32 count) {
    return ModelPrimQuadSharedSpine(pCmd, count, 0x18, 0x4, 0x05000000);
}

static s32 ModelPrimQuadFT4Variant0(u8* pCmd, s32 count) {
    return ModelPrimQuadSharedSpine(pCmd, count, 0x28, 0x8, 0x09000000);
}

static s32 ModelPrimTriMediumLitVariant1(u8* pCmd, s32 count) {
    return PcPort_DrawLitModelPolygons(pCmd, count, PCPORT_MODEL_LIT_G3);
}

static s32 ModelPrimQuadF4LitVariant1(u8* pCmd, s32 count) {
    return PcPort_DrawLitModelPolygons(pCmd, count, PCPORT_MODEL_LIT_F4);
}

static s32 ModelPrimQuadSZSelectSpine(u8* pCmd, s32 count, u32 packetStep,
                                      u32 vertexStride, u32 tagLen,
                                      int selectMin) {
    (void)tagLen;
    return PcPort_DrawModelPolygons(pCmd, count, 4, packetStep, vertexStride,
                                 selectMin ? PCPORT_MODEL_DEPTH_MINIMUM : PCPORT_MODEL_DEPTH_MAXIMUM);
}

static s32 ModelPrimQuadGT4MaxSZVariant2(u8* pCmd, s32 count) {
    return ModelPrimQuadSZSelectSpine(pCmd, count, 0x34, 0xC, 0x0C000000, 0);
}

static s32 ModelPrimQuadG4MaxSZVariant2(u8* pCmd, s32 count) {
    return ModelPrimQuadSZSelectSpine(pCmd, count, 0x24, 0x8, 0x08000000, 0);
}

static s32 ModelPrimQuadF4MaxSZVariant2(u8* pCmd, s32 count) {
    return ModelPrimQuadSZSelectSpine(pCmd, count, 0x18, 0x4, 0x05000000, 0);
}

static s32 ModelPrimQuadGT4MinSZVariant3(u8* pCmd, s32 count) {
    return ModelPrimQuadSZSelectSpine(pCmd, count, 0x34, 0xC, 0x0C000000, 1);
}

static s32 ModelPrimQuadG4MinSZVariant3(u8* pCmd, s32 count) {
    return ModelPrimQuadSZSelectSpine(pCmd, count, 0x24, 0x8, 0x08000000, 1);
}

static s32 ModelPrimQuadF4MinSZVariant3(u8* pCmd, s32 count) {
    return ModelPrimQuadSZSelectSpine(pCmd, count, 0x18, 0x4, 0x05000000, 1);
}

static s32 ModelPrimQuadFT4MinSZVariant3(u8* pCmd, s32 count) {
    return ModelPrimQuadSZSelectSpine(pCmd, count, 0x28, 0x8, 0x09000000, 1);
}

static s32 ModelPrimTriSharedSpine(u8* pCmd, s32 count, u32 packetStep,
                                   u32 vertexStride, u32 tagLen) {
    (void)tagLen;
    return PcPort_DrawModelPolygons(pCmd, count, 3, packetStep, vertexStride,
                                 PCPORT_MODEL_DEPTH_AVERAGE);
}

static s32 ModelPrimTriAverageVariant0(u8* pCmd, s32 count) {
    return ModelPrimTriSharedSpine(pCmd, count, 0x20, 0x8, 0x07000000);
}

static s32 ModelPrimTriMediumAverageVariant0(u8* pCmd, s32 count) {
    return ModelPrimTriSharedSpine(pCmd, count, 0x1C, 0x8, 0x06000000);
}

static s32 ModelPrimTriGT3Variant0(u8* pCmd, s32 count) {
    return ModelPrimTriSharedSpine(pCmd, count, 0x28, 0xC, 0x09000000);
}

/* Model base color set by func_8002C6E0; retail's variant-4 walker mtc2-loads
 * the 32-bit word at 0x80059598 into GTE RGBC.  The port defines the three
 * bytes as separate BSS symbols, so the word is composed explicitly. */
extern u8 D_80059598;
extern u8 D_80059599;
extern u8 D_8005959A;

static s32 ModelPrimTriDepthCueVariant4(u8* pCmd, s32 count) {
    return PcPort_DrawModelPolygons(pCmd, count, 3, 32, 8, PCPORT_MODEL_DEPTH_AVERAGE);
}

static s32 ModelPrimTriDepthCueMaxSZVariant5(u8* pCmd, s32 count) {
    ModelPrimVariant5Tripwire("tri", &g_ModelPrimTriDepthCueMaxSZVariant5EntryCount, pCmd, count);
    return PcPort_DrawModelPolygons(pCmd, count, 3, 32, 8, PCPORT_MODEL_DEPTH_MAXIMUM);
}

static s32 ModelPrimQuadFT4DepthCueVariant4(u8* pCmd, s32 count) {
    return PcPort_DrawModelPolygons(pCmd, count, 4, 40, 8, PCPORT_MODEL_DEPTH_AVERAGE);
}

static s32 ModelPrimQuadFT4DepthCueMaxSZVariant5(u8* pCmd, s32 count) {
    ModelPrimVariant5Tripwire("quad", &g_ModelPrimQuadFT4DepthCueMaxSZVariant5EntryCount, pCmd, count);
    return PcPort_DrawModelPolygons(pCmd, count, 4, 40, 8, PCPORT_MODEL_DEPTH_MAXIMUM);
}

extern s32 D_8004F2F4;
extern s32 D_8004F2F8;
extern s32 D_8004F2FC;
extern s32 D_8004F300;
extern s32 D_8004F304;
extern s32 D_8004F310;
extern s32 D_8004F314;
extern s32 D_8004F318;
extern s32 D_8004F31C;
extern s32 D_8004F320;
extern s32 D_8004F344;
extern s32 D_8004F348;
extern s32 D_8004F350;
extern s32 D_8004F354;
extern s32 D_8004F358;
extern s32 D_8004F35C;
extern s32 g_GameHasLoadedWDS;
extern s32 D_8004F364;
extern s32 D_8004F368;
extern s32 D_8004F36C;
extern s32 D_8004F370;
extern s32 D_8004F378;
extern s32 D_8004F37C;
extern s32 D_8004F380;
extern s16 D_8004F384;
extern u8 D_8005942C;
extern u8 D_800594D0;
extern s32 D_8005A444[3];
extern s32 D_8006F990[3];
extern s32 D_80062518;
extern s32 D_8006251C;
extern s32 D_80062524;
extern s32 g_GamePartySkinsInitialized;
extern s32 g_GamePartyMembers[3];
extern s32 g_GamePartyMemberSkins[3];
extern s32 g_PartyIsWaitingForStreamData;

/* func_8001AADC (asm/slus_006.64/system/temp3.s): original boot global-state
 * initializer called by func_80019578 before MainLoop. The native port enters
 * MainLoop directly, so keep this small reset here until the matching body is
 * port-safe for field actor gates. */
void func_8001AADC(void)
{
    s32 i;

    D_8004F364 = 1;
    D_8004F328 = 0xFF;
    D_8004F324 = 0xFF;
    D_8004F2FC = 0;
    D_8004F36C = 0;
    D_8004F2F8 = 0;
    D_8004F31C = 0;
    D_8004F320 = 0;
    D_8004F314 = 0;
    D_8004F310 = 0;
    g_GamePartySkinsInitialized = 0;
    D_8004F370 = 0;
    D_8004F35C = 0;
    g_GameHasLoadedWDS = 0;
    g_PartyIsWaitingForStreamData = 0;
    D_8004F358 = 0;
    D_8004F354 = 0;
    D_8004F350 = 0;
    D_8004F2F4 = 0;
    D_8004F344 = 0;
    D_8004F348 = 0;
    D_8004F304 = 0;
    D_8004F368 = 0;
    D_8004F300 = 0;
    D_8004F380 = 0;
    D_8004F37C = 0;
    D_8004F378 = 0;
    D_8005942C = 0;
    D_800594D0 = 0;
    D_8004F384 = 0;
    D_8004F318 = 0;
    D_8004F334 = -1;
    g_GameSceneMapNum = -1;
    D_8004F33C = -1;
    D_8004F338 = -1;
    D_8004F330 = -1;
    D_8004F32C = -1;
    D_8004F340 = -1;
    D_8004F308 = -1;

    for (i = 0; i < 3; i++) {
        g_GamePartyMemberSkins[i] = 0;
        D_8006F990[i] = 0;
        D_8005A444[i] = 0;
        g_GamePartyMembers[i] = 0;
    }

    D_80062524 = 0;
    D_8006251C = 0;
    D_80062518 = 0;
}

/* Fixed destination the per-state overlay decompresses to (ROM: .word D_8006FAF0
 * @0x80018084 -> PSX 0x8006FAF0, the low scratch region below each state's
 * relocated heap). Zeroed stub -> NULL -> LZSSDecompress(overlay, NULL) segfaults.
 * Set to the real emulated-RAM address in PcPort_HeapBoot (needs g_PsxRam base). */
void* g_MainGameStateOverlayBuffer;

/* ClearMemory(pStart, pEnd): zero a word range. Real one is asm/BIOS (bypassed);
 * main_loop.c calls it to wipe a game state's memory region before entering it,
 * so the stub (no-op) left uninitialised state -> crash on state change. */
void ClearMemory(u32* pStart, u32* pEnd)
{
    while (pStart < pEnd)
        *pStart++ = 0;
}

MainGameState g_MainGameStates[7];
static u32 s_GameStateSource[NUM_GAME_STATE_OVERLAYS][4] HOST_ONLY_BSS;

/*
 * Port-side stand-in for the retail shipping boot path.
 *
 * Retail (Noah/Ghidra): shipping builds enter movie mode (state 6) with the
 * intro STR queued, then land on the title/new-game menu overlay. This harness
 * plays disc archive file 15 (Opening STR) from the first picture, then holds
 * the last high-detail picture as the title graphic, with New Game / Continue
 * listed over it, then Field (state 1).
 *
 * Field-test / smoke runs keep KernelMenuMain as boot state 0 (see
 * PcPort_InitGameStates) so XENO_KERNEL_SEL continues to drive the debug menu.
 *
 * Phase logic lives in boot_menu.c (PcPort_BootUiTick / PcPort_BootEnterField)
 * so the boot certificate can drive the same functions as this loop.
 */
void PcPort_BootMain(void)
{
    static char sMenu[] =
        "\n\n\n\n"
        "          NEW GAME\n"
        "          CONTINUE\n";
    void* pOtag;
    PcPortBootUi ui;
    int running = 1;
    int tick;
    int i;
    int retailGraphic;

    KernelMenuInitialize();
    /* Opening menu is the last intro STR frame, not the KernelMenu navy card. */
    for (i = 0; i < 2; i++) {
        g_KernelMenuRenderEnvironments[i].drawEnv.r0 = 0;
        g_KernelMenuRenderEnvironments[i].drawEnv.g0 = 0;
        g_KernelMenuRenderEnvironments[i].drawEnv.b0 = 0;
    }
    SetDispMask(1);
    g_KernelMenuIsRunning = 0;
    D_800592C8 = 0;
    PcPort_BootUiInit(&ui);
    printf("[xeno-port][boot] title screen\n");

    while (running) {
        D_800592C4++;
        D_800592C8 = D_800592C4 & 1;
        g_KernelMenuCurRenderEnvironment =
            &g_KernelMenuRenderEnvironments[D_800592C8];
        pOtag = &g_KernelMenuCurRenderEnvironment->ot;
        TermPrim(pOtag);
        retailGraphic = PcPort_BootTitleUsedRetailGraphic();
        if (ui.phase != PC_PORT_BOOT_MOVIE)
            FontDrawLetters(pOtag);

        switch (ui.phase) {
        case PC_PORT_BOOT_TITLE:
            /* Black hold before the Opening STR; not the navy ASCII skip-card. */
            break;
        case PC_PORT_BOOT_MOVIE:
            /* STR is LoadImage'd to a 16-bit tpage in the tick; DrawPrim after
             * PutDrawEnv so the FT4s land on the isbg-cleared backbuffer. */
            break;
        case PC_PORT_BOOT_MENU:
            /* Title picture is the Opening STR freeze; still list New Game /
             * Continue (not the sole chrome when the retail graphic is up). */
            FontPrintf(sMenu);
            setXY0Fast(&g_KernelMenuCurRenderEnvironment->cursor,
                       0x38, ui.menuChoice * 8 + 0x38);
            setXY1Fast(&g_KernelMenuCurRenderEnvironment->cursor,
                       0x3F, ui.menuChoice * 8 + 0x3C);
            setXY2Fast(&g_KernelMenuCurRenderEnvironment->cursor,
                       0x38, ui.menuChoice * 8 + 0x40);
            AddPrim(pOtag, &g_KernelMenuCurRenderEnvironment->cursor);
            break;
        }

        {
            int strPicture = (ui.phase == PC_PORT_BOOT_MOVIE) ||
                             (ui.phase == PC_PORT_BOOT_MENU && retailGraphic);
            tick = PcPort_BootUiTick(&ui, g_C1ButtonStatePressedOnce,
                                     g_C1ButtonStateReleased);
            if (tick == PC_PORT_BOOT_TICK_CONTINUE) {
                PcPort_BootEnterContinue();
                running = 0;
            } else if (tick == PC_PORT_BOOT_TICK_NEWGAME) {
                PcPort_BootEnterField();
                running = 0;
            }

            DrawSync(0);
            Vsync(0);
            /* Always clear: STR is drawn as POLY_FT4s, not framebuffer LoadImage. */
            g_KernelMenuCurRenderEnvironment->drawEnv.isbg = 1;
            PutDrawEnv(&g_KernelMenuCurRenderEnvironment->drawEnv);
            PutDispEnv(&g_KernelMenuCurRenderEnvironment->dispEnv);
            /* DrawPrim the STR after the isbg clear and before DrawOTag so the
             * movie sits under New Game / Continue instead of covering them. */
            if (strPicture)
                PcPort_BootStrDrawPrims(NULL);
            DrawOTag(pOtag);
            if (strPicture && ui.phase == PC_PORT_BOOT_MOVIE &&
                ui.strFrame == 90) {
                const char* dir = getenv("XENO_BOOT_CAPTURE");
                if (dir && dir[0]) {
                    char bmp[512];
                    snprintf(bmp, sizeof(bmp), "%s/intro_pixels.bmp", dir);
                    PcPort_BootStrWriteBmp(bmp);
                }
                PcPort_BootCapture("intro");
            }
            if (strPicture && ui.phase == PC_PORT_BOOT_MENU &&
                ui.phaseFrames == 2) {
                const char* dir = getenv("XENO_BOOT_CAPTURE");
                if (dir && dir[0]) {
                    char bmp[512];
                    snprintf(bmp, sizeof(bmp), "%s/boot_menu_pixels.bmp", dir);
                    PcPort_BootStrWriteBmp(bmp);
                }
                PcPort_BootCapture("boot_menu");
            }
        }
    }

    PcPort_BootStrUnload();
    PcPort_BootAssetsUnload();
    DrawSync(0);
    MainLoop(0);
}

/* F16 hollow arrival for retail game state 3. This deliberately consumes the
 * field-authored tuple without pretending to implement the world-map overlay.
 * Vsync keeps SDL event processing responsive while the explicit placeholder
 * remains visible. */
void PcPort_WorldMapPlaceholderMain(void)
{
    static char sMessage[] =
        "\n\n\n\n"
        "       WORLD MAP NOT YET PORTED\n\n"
        "       FIELD ARRIVAL CAPTURED\n";
    u16 selector = *(u16*)((u8*)g_pGameState + 0x231A);
    u16 heading = *(u16*)((u8*)g_pGameState + 0x231C);
    u16 transitionArg2 = *(u16*)((u8*)g_pGameState + 0x231E);
    u16 rawEntrance = *(u16*)((u8*)g_pGameState + 0x2320);
    s32 worldIndex = (selector & 0x3FFF) - 0x0400;
    u16 entrance = rawEntrance & 0x7FFF;
    void* pOtag;

    fprintf(stderr,
            "[xeno-port][worldmap-placeholder] enter "
            "selector=0x%04x world_index=%d entrance=0x%04x "
            "transition_arg2=0x%04x heading=0x%04x\n",
            selector, worldIndex, entrance, transitionArg2, heading);

    KernelMenuInitialize();
    SetDispMask(1);
    g_KernelMenuIsRunning = 0;
    D_800592C8 = 0;

    for (;;) {
        D_800592C4++;
        D_800592C8 = D_800592C4 & 1;
        g_KernelMenuCurRenderEnvironment =
            &g_KernelMenuRenderEnvironments[D_800592C8];
        pOtag = &g_KernelMenuCurRenderEnvironment->ot;
        TermPrim(pOtag);
        FontDrawLetters(pOtag);
        FontPrintf(sMessage);
        DrawSync(0);
        Vsync(0);
        PutDrawEnv(&g_KernelMenuCurRenderEnvironment->drawEnv);
        PutDispEnv(&g_KernelMenuCurRenderEnvironment->dispEnv);
        DrawOTag(pOtag);
    }
}

void PcPort_InitGameStates(void)
{
    extern void MovieMain(void);

    /* [idx] = { pFnMain, pMemStart, pHeapStart, hasOverlay } — retail table
     * at SLUS 0x8001808C.  State 0 is the developer KernelMenu (0x8001A4B4);
     * the shipping boot never enters it (port_main.c calls
     * ChangeGameState(6) after the splash, as func_80019578 does), while the
     * XENO_FIELD_TEST=1 harness starts there so XENO_KERNEL_SEL can still
     * drive Field/etc. */
    g_MainGameStates[0].pFnMain    = KernelMenuMain;




    g_MainGameStates[1].pFnMain    = FieldMain;




    g_MainGameStates[2].pFnMain    = func_8001B6C4;




    /* Select the world-map entry installed by this build. */
    {
        g_MainGameStates[3].pFnMain = PcPort_WorldMapPlaceholderMain;



        printf("[xeno-port][boot] world state 3: arrival placeholder (hasOverlay=1)\n");
    }

    /* state 4 (battling, main 0x80088E90, archive 0x0D) is not yet ported;
     * left NULL until the oracle reaches it. */

    g_MainGameStates[5].pFnMain    = MenuMain;




    /* The movie loader calls the native player. */
    g_MainGameStates[6].pFnMain    = MovieMain;




    for (unsigned i = 0; i < NUM_GAME_STATE_OVERLAYS; ++i) {
        if (i == 4)
            continue;
        const u32 *record = s_GameStateSource[i];
        g_MainGameStates[i].pMemStart = PSX_ADDR(record[1]);
        g_MainGameStates[i].pHeapStart = PSX_ADDR(record[2]);
        g_MainGameStates[i].hasOverlay = (int)record[3];
    }
}

/* ---------------------------------------------------------------------------
 * Heap bootstrap.
 *
 * On hardware the boot routine func_80019578 (asm/.../main/main) calls
 * HeapInit(func_8002DFE0(), 0x801FC000) once before falling into MainLoop, where
 * func_8002DFE0 returns &D_8006FAF0. MainLoop never re-inits the heap; it only
 * HeapRelocate()s within it, which walks g_Heap and crashes if it was never set
 * up. The oracle enters MainLoop() directly (the asm `start`/boot is not yet C),
 * so the port performs that one-time HeapInit here, translated into emulated RAM.
 * --------------------------------------------------------------------------- */
void PcPort_HeapBoot(void)
{
    HeapInit(PSX_ADDR(0x8006FAF0), PSX_ADDR(0x801FC000));
    /* Overlay decompress target (see the extern def above): 0x8006FAF0 in
     * emulated RAM, resolvable only now that g_PsxRam exists. */
    g_MainGameStateOverlayBuffer = PSX_ADDR(0x8006FAF0);
}

int func_8003BDFC(int flags)
{
    if (flags & 0x10) {
        while (g_SoundControlFlags & 0x10) {
        }
    }

    if (g_SoundControlFlags & 0x10) {
        return g_SoundTransferQueue[g_SoundTransferQueueReadIndex].commandType;
    }
    return 0;
}

extern void* g_FieldScriptMemory;

extern void PcPort_RouteLogScriptWrite(int address, int value);
extern int PcPort_RouteLogOn(void);
void FieldScriptMemoryWriteU16(int index, int value)
{
    if (PcPort_RouteLogOn()) PcPort_RouteLogScriptWrite(index, value); /* XENO_ROUTE_LOG only */
    ((u16*)&g_FieldScriptMemory)[index >> 1] = value;
}

extern s32 g_GamePartySkinsInitialized;
extern s32 D_800ADBFC;
extern FieldActor* volatile g_FieldActors;
extern s32 g_PlayerActorIndex;
extern ActorData* g_FieldScriptVMCurActor;
extern s32 ArchiveSetIndex(s32 directoryIndex, s32 entryIndex);
extern s32 ArchiveDecodeAlignedSize(u32 entryIndex);
extern s32 ArchiveReadFileToBuffer(s32 index, void* pBuffer, u32 arg2, u32 flags);
extern s32 ArchiveCdDataSync(s32 mode);
extern void SpriteSetSpecialAnimFile(SpriteData* pSpriteData, void* pAnimFile);
extern void func_800A3C8C(void);
extern void FieldDistortionInitialize(s32 arg0);
extern void FieldScriptWritePartyMemberIDs(void);
extern void func_80072254(s32 actorIndex);

/* ---------------------------------------------------------------------------
 * LZSS decompressor.
 *   LZSSHeapDecompress @ 0x80032E88, LZSSDecompress @ 0x80032EB4
 *   (asm/slus_006.64/util/lzss.s -- pure asm, no C translation unit yet).
 *
 * Functional re-implementation, correct-by-inspection of the disassembly (not
 * byte-matched). Stream format: the first 4 bytes of the source are the
 * decompressed size. The remainder is a token stream of one flag byte (8 bits
 * consumed LSB-first) followed by that many tokens:
 *   bit 0 -> literal: copy the next source byte verbatim.
 *   bit 1 -> back-reference from two bytes b0,b1:
 *              offset = b0 | ((b1 & 0xF) << 8)   (12-bit window)
 *              length = (b1 >> 4) + 3
 *            copy `length` bytes from (dst - offset).
 * The original checks the output-end only at each 8-token group boundary, so a
 * group is always processed in full; this mirrors that exactly.
 * --------------------------------------------------------------------------- */
void* LZSSDecompress(void* pSrc, void* pDst)
{
    u8* src = (u8*)pSrc;
    u8* dst = (u8*)pDst;
    u8* dstStart = dst;
    u32 nDecompressedSize = *(u32*)src;
    u8* dstEnd;

    /* XENO_PC_PORT stopgap. A per-state overlay whose archive entry the port's
     * disc/overlay path can't yet resolve (e.g. the field overlay: the archive
     * directory the overlay index lives in isn't set up, so ArchiveDecodeSize
     * returns 0) makes LoadGameStateOverlay return an UNWRITTEN buffer -- so this
     * size header is heap garbage (megabytes) and decompressing it walks straight
     * off emulated RAM. The overlay is redundant in the port anyway (field/menu
     * code is statically linked), so until the overlay archive read is implemented,
     * treat an implausible size (larger than all of emulated RAM) as an empty /
     * no-op overlay rather than crashing. Real streams (splash ~4KB, overlays
     * <=~345KB) are far under this bound. */
    if (nDecompressedSize > (u32)PSX_RAM_SIZE) {
        fprintf(stderr, "[xeno-port] LZSSDecompress: implausible size 0x%x "
                        "(overlay not resolved?) -> skipping\n", nDecompressedSize);
        return pDst;
    }

    dstEnd = dst + nDecompressedSize;
    src += 4;

    while (dst != dstEnd) {
        u8 flags = *src++;
        int n;
        for (n = 0; n < 8; n++, flags >>= 1) {
            if (flags & 1) {
                u32 b0 = *src++;
                u32 b1 = *src++;
                u32 offset = b0 | ((b1 & 0xF) << 8);
                u32 length = (b1 >> 4) + 3;
                u8* ref = dst - offset;
                u32 i;
                for (i = 0; i < length; i++) {
                    *dst++ = *ref++;
                }
            } else {
                *dst++ = *src++;
            }
        }
    }
    return dstStart;
}

void* LZSSHeapDecompress(void* pCompressed, int flags)
{
    u32 size = *(u32*)pCompressed;
    void* pDst = HeapAlloc(size, (u_int)flags);
    if (pDst == NULL) {
        return NULL;
    }
    return LZSSDecompress(pCompressed, pDst);
}

/* ---------------------------------------------------------------------------
 * func_80036718 (asm/slus_006.64/26644.s:655) -- the font's printf-style text
 * formatter that FontPrintf delegates to. The original is a 422-line custom
 * printf that, per output character, calls func_800366F0 -> (*D_80050594) =
 * FontAddLetterPrimitive (the glyph queuer, already decompiled in font.c).
 *
 * Functional re-implementation (not byte-matched): format with vsprintf, then
 * queue each character's glyph directly via FontAddLetterPrimitive. FontPrintf
 * does va_start then passes the va_list as the 3rd argument, so it arrives here
 * as `args` (x86-64 passes a va_list by reference, matching the variadic decl).
 * --------------------------------------------------------------------------- */
#include <stdarg.h>
#include <stdio.h>
extern void FontAddLetterPrimitive(int letter);

void func_80036718(int mode, char* format, va_list args)
{
    char buf[512];
    char* p;
    (void)mode;
    vsprintf(buf, format, args);
    for (p = buf; *p != '\0'; p++) {
        FontAddLetterPrimitive((unsigned char)*p);
    }
}

/* ---------------------------------------------------------------------------
 * func_800317E0 / func_80031804 (asm/slus_006.64/.../temp2 -- byte-identical):
 * a fast addPrim that threads a primitive onto the head of an ordering table
 * with tag length 3:  old = *ot;  *ot = addr(prim) & 0xFFFFFF;  *prim = old | (3<<24);
 * FontDrawLetters' per-glyph loop calls this to link each queued letter's SPRT
 * into the OT that DrawOTag walks. PSX OT links are 24-bit addresses; the
 * emulated RAM is linked below 16 MiB (-no-pie) so the mask is lossless.
 * --------------------------------------------------------------------------- */
void func_800317E0(void* ot, void* prim)
{
    u32 old = *(u32*)ot;
    *(u32*)ot = (u32)(uintptr_t)prim & 0xFFFFFF;
    *(u32*)prim = old | 0x03000000;
}

void func_80031804(void* ot, void* prim)
{
    u32 old = *(u32*)ot;
    *(u32*)ot = (u32)(uintptr_t)prim & 0xFFFFFF;
    *(u32*)prim = old | 0x03000000;
}

/* ---------------------------------------------------------------------------
 * Anim-script opcode 0xFC image-upload chain: func_8001FBE4 (0xFC handler in
 * animation_scripts.c) -> func_8001FB30 -> func_8002DDE4 -> LoadImage.
 *
 * D_800592E4/E8/EA (sbss 0x800592E4-EA; "800592E4 -> EA is bss local" in
 * rendering.c) carry the image-blob pointer and VRAM base x/y between the
 * handler and func_8001FB30, which the matching build keeps as INCLUDE_ASM.
 * --------------------------------------------------------------------------- */
u32 D_800592E4;
s16 D_800592E8;
s16 D_800592EA;

/* func_8002DDE4 (asm/slus_006.64/nonmatchings/system/temp2/func_8002DDE4.s):
 * multi-block VRAM uploader. pImageData = { s32 count; u32 skipped[count];
 * blocks... }, each block = { u32 magic; u16 baseX, baseY, offsX, offsY;
 * u16 w, h; u16 pixels[w*h] }. magic 0x1100 = pixel data (positioned by
 * texMode), 0x1101 = CLUT (positioned by clutMode); any other magic returns 1.
 * mode 1: caller xy + block offs; mode 2: block base + caller xy + block offs;
 * other: block base + block offs. Each block is LoadImage'd into VRAM.
 * clutX/clutY are read as u16 (retail lhu), the rest stay 32-bit signed. */
s32 func_8002DDE4(void* pImageData, s32 texMode, s32 texX, s32 texY,
                  s32 clutMode, s32 clutX, s32 clutY)
{
    s32 count = *(s32*)pImageData;
    u8* pBlock = (u8*)pImageData + count * 4 + 4;
    s16 texMode16 = (s16)texMode;
    s16 clutMode16 = (s16)clutMode;
    u16 cx = (u16)clutX;
    u16 cy = (u16)clutY;
    RECT rect;
    s32 i;
    static int s_dde4Logs;
    int diag = 0;
    {
        const char* env = getenv("XENO_FIELD_DIAG");
        diag = (env != NULL && env[0] != '\0' && env[0] != '0');
    }

    if (diag && s_dde4Logs < 8) {
        u32* w0 = (u32*)pImageData;
        fprintf(stderr,
                "[DDE4] p=%p count=%d mode=%d/%d xy=(%d,%d) clut=(%d,%d) "
                "w0=%08x %08x %08x %08x\n",
                pImageData, (int)count, (int)texMode, (int)clutMode,
                (int)texX, (int)texY, (int)clutX, (int)clutY,
                w0[0], w0[1], w0[2], w0[3]);
    }

    for (i = 0; i < count; i++) {
        u32 magic = *(u32*)pBlock;
        u16 baseX;
        u16 baseY;
        u16 offsX;
        u16 offsY;
        pBlock += 4;

        baseX = *(u16*)(pBlock + 0);
        baseY = *(u16*)(pBlock + 2);
        offsX = *(u16*)(pBlock + 4);
        offsY = *(u16*)(pBlock + 6);

        if (magic == 0x1100) {
            if (texMode16 == 1) {
                rect.x = texX + offsX;
                rect.y = texY + offsY;
            } else if (texMode16 == 2) {
                rect.x = baseX + texX + offsX;
                rect.y = baseY + texY + offsY;
            } else {
                rect.x = baseX + offsX;
                rect.y = baseY + offsY;
            }
        } else if (magic == 0x1101) {
            if (clutMode16 == 1) {
                rect.x = cx + offsX;
                rect.y = cy + offsY;
            } else if (clutMode16 == 2) {
                rect.x = baseX + cx + offsX;
                rect.y = baseY + cy + offsY;
            } else {
                rect.x = baseX + offsX;
                rect.y = baseY + offsY;
            }
        } else {
            if (diag && s_dde4Logs < 8) {
                fprintf(stderr, "[DDE4] bad magic[%d]=%08x\n", (int)i, magic);
                s_dde4Logs++;
            }
            return 1;
        }

        pBlock += 8;
        rect.w = *(u16*)pBlock;
        pBlock += 2;
        rect.h = *(u16*)pBlock;
        pBlock += 2;
        if (diag && s_dde4Logs < 8) {
            fprintf(stderr,
                    "[DDE4] blk[%d] magic=%04x base=(%u,%u) offs=(%u,%u) "
                    "dest=(%d,%d) %dx%d\n",
                    (int)i, (unsigned)magic, (unsigned)baseX, (unsigned)baseY,
                    (unsigned)offsX, (unsigned)offsY,
                    (int)rect.x, (int)rect.y, (int)rect.w, (int)rect.h);
        }
        LoadImage(&rect, (u_long*)pBlock);
        if (diag && s_dde4Logs < 8 && magic == 0x1100) {
            u16* pix = (u16*)pBlock;
            fprintf(stderr,
                    "[DDE4] tex16 %04x %04x %04x %04x %04x %04x %04x %04x\n",
                    (unsigned)pix[0], (unsigned)pix[1], (unsigned)pix[2],
                    (unsigned)pix[3], (unsigned)pix[4], (unsigned)pix[5],
                    (unsigned)pix[6], (unsigned)pix[7]);
        }
        if (diag && s_dde4Logs < 8 && magic == 0x1101 &&
            rect.w > 0 && rect.w <= 256 && rect.h > 0 && rect.h <= 4) {
            /* The guard admits up to 256 x 4 halfwords; the buffer must
             * hold the full rectangle (StoreImage writes w*h halfwords). */
            u16 clutBuf[256 * 4];
            RECT rd = rect;
            int n;
            int nPix = (int)rect.w * (int)rect.h;
            DrawSync(0);
            StoreImage(&rd, (u_long*)clutBuf);
            DrawSync(0);
            fprintf(stderr, "[DDE4] clut16");
            for (n = 0; n < nPix && n < 16; n++) {
                fprintf(stderr, " %04x", (unsigned)clutBuf[n]);
            }
            fprintf(stderr, "\n");
            if (nPix > 80) {
                fprintf(stderr, "[DDE4] clut80");
                for (n = 80; n < nPix && n < 96; n++) {
                    fprintf(stderr, " %04x", (unsigned)clutBuf[n]);
                }
                fprintf(stderr, "\n");
            }
        }
        pBlock += (s16)rect.w * (s16)rect.h * 2;
    }
    if (diag && s_dde4Logs < 8) {
        s_dde4Logs++;
    }
    return 0;
}

/* func_8001FB30 (asm/slus_006.64/nonmatchings/system/rendering/func_8001FB30.s):
 * trampoline that retail runs on a heap-allocated 0x2000-byte stack (sp is
 * repointed to scratch+0x1EFC around the call). The native stack needs no
 * switch; the alloc/free pair is kept so heap state stays retail-exact.
 * Forwards the D_800592E4/E8/EA handoff into func_8002DDE4 with the CLUT
 * args zeroed (CLUT blocks then use their raw embedded coordinates). */
void func_8001FB30(void)
{
    void* scratch = HeapAlloc(0x2000, 1);
    func_8002DDE4((void*)(uintptr_t)D_800592E4, 1, D_800592E8, D_800592EA,
                  0, 0, 0);
    HeapFree(scratch);
}

/* ---------------------------------------------------------------------------
 * Anim-script opcode 0xE0 child-sprite spawn chain.
 *
 * func_8001FBE4's 0xE0 case calls func_80023B84(parentSprite, script, pkg),
 * which allocates a heap AnimTask { WorkListEntry task1(timer); task2(render);
 * SpriteData @+0x38 } via func_800233A4, copies a large slice of the parent's
 * state into the child, binds the child's animation script (func_80023538,
 * already real in temp1.c), and registers per-type post-init + render
 * callbacks (func_80024730 / func_80025224). All of these are INCLUDE_ASM in
 * temp1.c for the matching build; ported here from asm, correct-by-inspection.
 * Work-list add/remove/setter machinery lives in work_list_port.c.
 * --------------------------------------------------------------------------- */

extern void AnimScriptTick(void* pSpriteData);
extern void func_80023804(void* pSpriteData);
extern void func_80023538(void* pSpriteData, void* pAnimation);
extern void func_8002393C(void* arg0);
extern void func_80023950(void* arg0);
extern int func_8001EE74(void* arg0);
extern void func_8001CE74(void* pTargetEntry);
extern void func_8001D034(void* pTargetEntry);
extern void func_8001D3F4(void* pTargetSprite);
extern void TimerWorkListAddTask(void* pOwner, void* pEntry);
extern void WorkListAddTask(void* pOwner, void* pEntry);
extern void TimerWorkListSetTaskCallback(void* pTask, void (*callback)(void*));
extern void WorkListSetTaskCallback(void* pTask, void (*callback)(void*));
extern void WorkListTaskSetOnFreeCallback(void* pTask, void (*callback)(void*));
extern void TimerWorkListRemoveTask(void* pTargetEntry);
extern void WorkListRemoveTask(void* pTargetEntry);
extern u8 D_800591AC;
extern u8 D_800591AF;
extern void func_80022038(void* pSpriteData);
extern s32 func_8002C700(u8* a0, u8* a1, u32* a2, s32 a3);
extern MATRIX D_8004FBB8;
extern u_long* g_GfxCurOT;
extern void func_80025710(void);
extern u8 D_8006BE10[];
extern u8 D_8005A474[];
extern u32 g_GfxCurWorkBuffer;
extern s32 g_GfxCurContext;
extern u32 D_80059300[];
extern u32 D_80059304;
extern u32 D_801E8670[];

/* Model-resource overlay archive 0x6B9, retail 0x801E72CC-0x801E7374.
 *
 * D_801E8670 is a packed table of 32-bit PSX pointers. selector chooses an
 * entry, whose +4 word points at the model resource. index zero copies the
 * resource's base matrix at +0x0C. Nonzero indices compose that base matrix
 * with the matrix at resource + index*0x7C + 0x2C. Retail never reads its
 * second argument, which makes the func_800748E8 src==dst call safe.
 */
void func_801E72CC(MATRIX* dst, MATRIX* unused, s32 selector, s32 index)
{
    u8* entry;
    u8* resource;
    u32* srcWords;
    u32* dstWords;
    u32 word0;
    u32 word1;
    u32 word2;
    u32 word3;

    (void)unused;
    entry = (u8*)(uintptr_t)D_801E8670[selector];
    if (entry == NULL) {
        return;
    }

    resource = (u8*)(uintptr_t)*(u32*)(entry + 4);
    if (index != 0) {
        CompMatrix((MATRIX*)(resource + 0x0C),
                   (MATRIX*)(resource + index * 0x7C + 0x2C), dst);
        return;
    }

    srcWords = (u32*)(resource + 0x0C);
    dstWords = (u32*)dst;
    word0 = srcWords[0];
    word1 = srcWords[1];
    word2 = srcWords[2];
    word3 = srcWords[3];
    dstWords[0] = word0;
    dstWords[1] = word1;
    dstWords[2] = word2;
    dstWords[3] = word3;
    word0 = srcWords[4];
    word1 = srcWords[5];
    word2 = srcWords[6];
    word3 = srcWords[7];
    dstWords[4] = word0;
    dstWords[5] = word1;
    dstWords[6] = word2;
    dstWords[7] = word3;
}

/* .sbss @0x800592EC: counts func_80022E8C ticks (type-7 timer callback). */
s32 D_800592EC;
/* .bss @0x8006F99C / 0x8006F9AC (asm/slus_006.64/data/49AC0.bss.s, 0x10 each,
 * zero-initialized like retail): default position vectors copied into type
 * 10-13 child sprites by func_80024730. */
u32 D_8006F99C[4];
u32 D_8006F9AC[4];

/* Main-executable sprite code re-enters the active retail battle overlay for
 * floor queries and scripted child ownership. Keep its actual state and
 * nested callback stack, as with the battle animation interpreter. */
extern int PcPort_BattleMipsDispatchCallback(u32 callback, void* argument);

/* func_800BA8F4 is an adopted native battle leaf (src/battle/mainl110.c on
 * the battle guest ABI, pc_port/src/battle_overlay_host_leaves.inc); the
 * port links its battle copy as battle_func_800BA8F4 (build_port.sh renames
 * the colliding name).  When it is linked, call it directly: no interpreter
 * re-entry for the floor query.  The dispatch below remains only for links
 * without the battle host bodies (unit tests). */
extern void battle_func_800BA8F4(u8* s) __attribute__((weak));
extern int PcPort_BattleMipsActive(void) __attribute__((weak));
/* XENO_BATTLE_NATIVE_LEAVES test switch (battle_mips_runtime.c). */
extern int PcPort_BattleNativeLeafAllowed(const char* name) __attribute__((weak));

void func_800BA8F4(void* pSpriteData)
{
    static int native = -1; /* the switch is read once, so is this verdict */
    if (native < 0)
        native = battle_func_800BA8F4 != NULL && PcPort_BattleMipsActive != NULL &&
                 (PcPort_BattleNativeLeafAllowed == NULL ||
                  PcPort_BattleNativeLeafAllowed("func_800BA8F4"));
    if (native) {
        if (!PcPort_BattleMipsActive()) {
            fputs("[xeno-port][battle-mips] sprite floor query called without an active retail battle overlay\n", stderr);
            abort();
        }
        battle_func_800BA8F4((u8*)pSpriteData);
        return;
    }
    if (!PcPort_BattleMipsDispatchCallback(0x800ba8f4u, pSpriteData)) {
        fputs("[xeno-port][battle-mips] sprite floor query called without an active retail battle overlay\n", stderr);
        abort();
    }
}

void func_800BC158(void* pWrapper)
{
    if (!PcPort_BattleMipsDispatchCallback(0x800bc158u, pWrapper)) {
        fputs("[xeno-port][battle-mips] child sprite ownership called without an active retail battle overlay\n", stderr);
        abort();
    }
}

/* Native main-executable sprite code re-enters the currently loaded battle
 * overlay here (retail SLUS call at 800248FC). Keep its actual interpreter
 * and state; never substitute the field animation interpreter or a no-op.
 * Moved from battle_mips_runtime.c: func_800C11CC is an adopted lifted leaf
 * (src/battle/main135.c), and the overlay differential includes the runtime
 * source, so the trampoline must not share a TU with it.  The port links the
 * lifted battle copy as battle_func_800C11CC (build_port.sh renames the
 * colliding name); the dispatch reaches it through the bridge, which gives a
 * lifted body the call context its jal-service calls need. */
void func_800C11CC(void* sprite)
{
    if (!PcPort_BattleMipsDispatchCallback(0x800c11ccu, sprite)) {
        fputs("[xeno-port][battle-mips] sprite animation called without an active retail battle overlay\n", stderr);
        abort();
    }
}

/* Type-2/7 child render callback (asm 80025718-800257EC): refresh the sprite
 * transform if dirty, bail when the model header is absent, apply the sprite's
 * position halfwords through TransMatrix, optionally compose with the camera
 * matrix at D_8004FBB8 unless +0x3F bit 0 is set, load the composite into the
 * GTE, then dispatch the model packet through func_8002C700. */
void func_80025718(void* pTask)
{
    u8* task = pTask;
    u8* sprite = (u8*)(uintptr_t)*(u32*)(task + 0x4);
    u8* pBase;
    u32 modelHdr;
    u32 modelBuf;
    VECTOR trans;
    MATRIX composite;
    MATRIX* pDraw;

    func_80022038(sprite);
    pBase = (u8*)(uintptr_t)*(u32*)(sprite + 0x20);
    if (pBase == NULL) {
        return;
    }
    modelHdr = *(u32*)(pBase + 0x34);
    if (modelHdr == 0) {
        return;
    }

    trans.vx = *(s16*)(sprite + 0x2);
    trans.vy = *(s16*)(sprite + 0x6);
    trans.vz = *(s16*)(sprite + 0xA);
    TransMatrix((MATRIX*)(pBase + 0xC), &trans);

    pDraw = (MATRIX*)(pBase + 0xC);
    if ((*(u8*)(sprite + 0x3F) & 1) == 0) {
        CompMatrix(&D_8004FBB8, (MATRIX*)(pBase + 0xC), &composite);
        pDraw = &composite;
    }
    SetRotMatrix(pDraw);
    SetTransMatrix(pDraw);

    modelBuf = *(u32*)(pBase + 0x2C + (u32)g_GfxCurContext * 4);

    /* TEMPORARY DIAGNOSTIC (child visibility audit, 2026-07-09): with
     * XENO_CHILD_DRAW_DIAG=1, log the OT-emit counter (D_80059578) delta
     * across func_8002C700 for the first child dispatches plus a sparse
     * tail, and the leading prim group ids. Off by default; remove once
     * child pixels are confirmed. */
    {
        static int s_diag = -1;
        static int s_calls;
        s32 emitBefore = 0;
        int doLog = 0;

        if (s_diag < 0) {
            const char* env = getenv("XENO_CHILD_DRAW_DIAG");
            s_diag = (env != NULL && env[0] != '\0' && env[0] != '0');
        }
        if (s_diag) {
            doLog = (s_calls < 12) || (s_calls % 100) == 0;
            emitBefore = D_80059578;
        }

        {
            s32 ret = func_8002C700((u8*)(uintptr_t)modelHdr,
                                    (u8*)(uintptr_t)modelBuf, (u32*)g_GfxCurOT,
                                    *(u16*)(sprite + 0x42) & 0x4);
            if (doLog) {
                u8* hdr = (u8*)(uintptr_t)modelHdr;
                u8* grp = (u8*)(uintptr_t)*(u32*)(hdr + 0x10);
                fprintf(stderr,
                        "[child-draw] #%d sprite=%p hdr=%p buf=%08x ctx=%d "
                        "var=%d groups=%u prim0=%u cnt0=%d trans=(%d,%d,%d) "
                        "drawT=(%d,%d,%d) ret=%d emits=%d->%d\n",
                        s_calls, (void*)sprite, (void*)hdr, modelBuf,
                        (int)g_GfxCurContext, (int)(*(u16*)(sprite + 0x42) & 0x4),
                        *(u16*)(hdr + 0x6), grp[0], (int)*(s16*)(grp + 0x2),
                        (int)trans.vx, (int)trans.vy, (int)trans.vz,
                        (int)pDraw->t[0], (int)pDraw->t[1], (int)pDraw->t[2],
                        ret, emitBefore, D_80059578);
            }
            if (s_diag) {
                s_calls++;
            }
        }
    }
}

extern void func_80025258(u8* pEntry);
extern void func_8002541C(u8* pEntry);
extern void func_80025544(u8* pEntry);
extern void func_800257F0(u8* pEntry);

/* Child render callback dispatch. */
#include "../include_shim/host_only_bss.h"
#include "retail_data.h"

typedef void (*ChildRenderCallback)(void *);
static uint8_t s_ChildRenderCallbackIds[16][4] HOST_ONLY_BSS;

XENO_RETAIL_DATA_BEGIN(child_render_callbacks)
    XENO_RD(s_ChildRenderCallbackIds, XENO_RD_SLUS, XENO_RD_SLUS_OFF(0x8004FD40u),
            sizeof(s_ChildRenderCallbackIds)),
XENO_RETAIL_DATA_END(child_render_callbacks)

static ChildRenderCallback ChildRenderResolve(uint32_t guest)
{
    switch (guest) {
    case 0x80025258u: return (ChildRenderCallback)func_80025258;
    case 0x8002541Cu: return (ChildRenderCallback)func_8002541C;
    case 0x80025544u: return (ChildRenderCallback)func_80025544;
    case 0x80025710u: return (ChildRenderCallback)func_80025710;
    case 0x80025718u: return (ChildRenderCallback)func_80025718;
    case 0x800257F0u: return (ChildRenderCallback)func_800257F0;
    default: return NULL;
    }
}

void func_80025224(void* pTask, int handlerIndex)
{
    static uint32_t loggedMask;
    unsigned slot = (unsigned)handlerIndex;
    ChildRenderCallback callback = NULL;

    if (slot < sizeof(s_ChildRenderCallbackIds) / sizeof(s_ChildRenderCallbackIds[0])) {
        const uint8_t *id = s_ChildRenderCallbackIds[slot];
        uint32_t guest = (uint32_t)id[0] | ((uint32_t)id[1] << 8) |
                         ((uint32_t)id[2] << 16) | ((uint32_t)id[3] << 24);
        callback = ChildRenderResolve(guest);
        if (guest && !callback && !(loggedMask & (1u << slot))) {
            loggedMask |= 1u << slot;
            fprintf(stderr, "[port] anim render callback D_8004FD40[%u] not "
                            "ported; child sprite will not render\n", slot);
        }
    }
    WorkListSetTaskCallback(pTask, callback);
}
/* End child render callback dispatch. */

/* asm 80023440: sprite type from the animation-script header halfword:
 * bits 8-10, plus 8 if bit 14 is set. */
s32 func_80023440(void* pScript)
{
    u16 header = *(u16*)pScript;
    s32 type = (header >> 8) & 0x7;

    if ((header >> 14) & 1) {
        type += 8;
    }
    return type;
}

/* asm 80023468 via jtbl_80018664: sprite type -> transform-block mode.
 * mode 0 = no transform block, 1 = transform block + per-frame buffer,
 * 2 = transform block only. Type 3 (and >= 0x10) has no table entry in
 * retail and returns an uninitialized register — a dead path (the only
 * caller re-derives type 3 from sprite flags first); -1 here makes
 * func_80023A48's assert catch it loudly if it ever becomes live. */
s32 func_80023468(s32 type)
{
    static const s8 modes[16] = { 1, 0, 2, -1, 0, 1, 1, 2,
                                  0, 0, 1, 1, 1, 1, 1, 2 };

    if ((u32)type >= 0x10) {
        return -1;
    }
    return modes[type];
}

/* asm 800233A4 (matched-C comment in temp1.c): allocate the AnimTask
 * (0xEC header = two WorkListEntry + SpriteData, plus dataSize of per-mode
 * trailing buffers), register task1 on the timer list under pOwner and task2
 * on the render list under task1, init the SpriteData, and bind the tick
 * (func_80022DF4) and free (func_80022EB8) callbacks. */
void func_80022DF4(void* pTask);
void func_80022E8C(void* pTask);
void func_80022EB8(void* pTask);

void* func_800233A4(void* pOwner, int dataSize)
{
    u8* pEntry = HeapAlloc(dataSize + 0xEC, D_800591AF);
    u8* pTask2 = pEntry + 0x1C;
    u8* pSprite = pEntry + 0x38;

    TimerWorkListAddTask(pOwner, pEntry);
    WorkListAddTask(pEntry, pTask2);
    func_80023804(pSprite);
    *(u32*)(pEntry + 0x4) = (u32)(uintptr_t)pSprite;
    *(u32*)(pTask2 + 0x4) = (u32)(uintptr_t)pSprite;
    TimerWorkListSetTaskCallback(pEntry, func_80022DF4);
    WorkListTaskSetOnFreeCallback(pEntry, func_80022EB8);
    return pEntry;
}

/* asm 80023958 (mode 2): transform block at sprite+0xB4; no per-frame or
 * direction buffers. */
void func_80023958(void* pSpriteData)
{
    u8* p = pSpriteData;
    u8* pBase = p + 0xB4;

    *(u32*)(p + 0x20) = (u32)(uintptr_t)pBase;
    func_8002393C(pBase);
    *(u32*)(pBase + 0x34) = 0;
    *(u32*)(pBase + 0x40) = 0;
}

/* asm 800239F4 (mode 1): transform block at sprite+0xB4 with the per-frame
 * work buffer at sprite+0xF4 (sized by func_80023A48's frame-count math). */
void func_800239F4(void* pSpriteData)
{
    u8* p = pSpriteData;
    u8* pBase = p + 0xB4;

    *(u32*)(p + 0x20) = (u32)(uintptr_t)pBase;
    func_8002393C(pBase);
    *(u32*)(pBase + 0x30) = (u32)(uintptr_t)(p + 0xF4);
    *(u32*)(pBase + 0x34) = 0;
    *(u32*)(pBase + 0x38) = 0;
}

/* asm 80023A48: allocate + shape the child AnimTask by mode. Mode 1 sizes a
 * trailing per-frame buffer from the package's frame count ((n-1)*24+0x58)
 * and types 5/6 swap in the global default packages. Sprite+0x86 records the
 * sprite-local allocation size (mode extra + 0xEC); +0x6C points back at the
 * wrapper AnimTask; +0x24 is the (possibly overridden) anim package. */
void* func_80023A48(s32 type, s32 mode, void* pAnimPackage, s32 dataSize,
                    void* pOwner)
{
    u8* pkg = pAnimPackage;
    s32 extra;
    u8* pWrapper;
    u8* pSprite;

    if (mode == 1) {
        if (type == 5) {
            pkg = D_8006BE10;
        }
        if (type == 6) {
            pkg = D_8005A474;
        }
        extra = (func_8001EE74((void*)(uintptr_t)*(u32*)pkg) - 1) * 24 + 0x58;
        pWrapper = func_800233A4(pOwner, extra + dataSize);
        func_800239F4(pWrapper + 0x38);
    } else if (mode == 0) {
        extra = 0;
        pWrapper = func_800233A4(pOwner, dataSize);
        func_80023950(pWrapper + 0x38);
    } else if (mode == 2) {
        extra = 0x54;
        pWrapper = func_800233A4(pOwner, dataSize + 0x54);
        func_80023958(pWrapper + 0x38);
    } else {
        /* Retail reaches here only via the type-3/invalid dead path in
         * func_80023468 and would run on uninitialized registers. */
        assert(0 && "func_80023A48: invalid sprite transform mode");
        return NULL;
    }

    pSprite = pWrapper + 0x38;
    *(u32*)(pSprite + 0x6C) = (u32)(uintptr_t)pWrapper;
    *(u16*)(pSprite + 0x86) = (u16)(extra + 0xEC);
    *(u32*)(pSprite + 0x24) = (u32)(uintptr_t)pkg;
    return pWrapper;
}

/* asm 80024730 via jtbl_800186A4: per-type post-init after the spawn copy.
 * Types 0-6/14 (and >= 0xF) just bind the render callback; 7 swaps the tick
 * callback for the counting variant; 8/9 preset frame fields; 10/11 zero the
 * frame and take a default position vector; 12/13 demote themselves to type
 * 10/11 (type-2) before doing the same. */
void func_80024730(void* pWrapper)
{
    u8* w = pWrapper;
    u8* sp = w + 0x38;
    s32 idx = (*(u32*)(sp + 0x40) >> 13) & 0xF;
    u32* src = NULL;

    if (idx < 0xF) {
        switch (idx) {
        case 7:
            TimerWorkListSetTaskCallback(w, func_80022E8C);
            break;
        case 8:
            *(u8*)(sp + 0x2B) = 0x68;
            *(u16*)(sp + 0x34) = 1;
            break;
        case 9:
            *(u16*)(sp + 0x36) = 3;
            *(u8*)(sp + 0x2B) = 0x60;
            *(u16*)(sp + 0x34) = 1;
            break;
        case 10:
            *(u16*)(sp + 0x34) = 0; /* delay slot: precedes the callee body */
            func_800BC158(w);
            src = D_8006F99C;
            break;
        case 11:
            *(u16*)(sp + 0x34) = 0; /* delay slot: precedes the callee body */
            func_800BC158(w);
            src = D_8006F9AC;
            break;
        case 12:
        case 13: {
            u32 v = *(u32*)(sp + 0x40);

            *(u16*)(sp + 0x34) = 1;
            idx = (idx - 2) & 0xF;
            *(u32*)(sp + 0x40) = (v & 0xFFFE1FFF) | (idx << 13);
            func_800BC158(w);
            src = D_8006F99C;
            break;
        }
        default:
            break;
        }
    }
    if (src) {
        *(u32*)(sp + 0x0) = src[0];
        *(u32*)(sp + 0x4) = src[1];
        *(u32*)(sp + 0x8) = src[2];
    }
    func_80025224(w + 0x1C, idx);
}

/* Motion ownership lives in the verified decompiled temp1.c functions. */
extern void func_80022CDC(u8* pSpriteData);

/* asm 80022DF4: AnimTask timer tick. Runs the sprite's script + motion; when
 * the script terminates (+0x64 == 0) — immediately, or after the double-tick
 * granted by AC bit 6 — invokes the task's free callback (func_80022EB8). */
void func_80022DF4(void* pTask)
{
    u8* t = pTask;
    u8* sp = (u8*)(uintptr_t)*(u32*)(t + 0x4);

    AnimScriptTick(sp);
    func_80022CDC(sp);
    if (*(u32*)(sp + 0x64) != 0) {
        if (!((*(u32*)(sp + 0xAC) >> 6) & 1)) {
            return;
        }
        AnimScriptTick(sp);
        func_80022CDC(sp);
        if (*(u32*)(sp + 0x64) != 0) {
            return;
        }
    }
    ((void (*)(void*))(uintptr_t)*(u32*)(t + 0xC))(pTask);
}

/* asm 80022E8C: type-7 tick variant — counts invocations in D_800592EC. */
void func_80022E8C(void* pTask)
{
    D_800592EC++;
    func_80022DF4(pTask);
}

/* asm 80025180: push an 8-byte node onto the current context's deferred
 * image list, allocated from the gfx work buffer. The host stores the two
 * list heads in separate symbols. Retail
 * advances the buffer head by 8 even when it is NULL; kept as-is. */
void func_80025180(void* pData)
{
    u8* node = (u8*)(uintptr_t)g_GfxCurWorkBuffer;

    g_GfxCurWorkBuffer = (u32)(uintptr_t)(node + 8);
    if (node != NULL) {
        u32* head = g_GfxCurContext ? &D_80059304 : &D_80059300[0];
        *(u32*)(node + 0x0) = (u32)(uintptr_t)pData;
        *(u32*)(node + 0x4) = *head;
        *head = (u32)(uintptr_t)node;
    }
}

/* asm 80022EB8: AnimTask free callback. Queues the transform block's +0x2C
 * buffer for deferred free, releases the direction table for mode-1 sprites,
 * unlinks owned tasks (AC bit 5) and parent links (+0xB0 bit 11), drops the
 * sprite from the pending-frame chain, removes both tasks and frees the
 * AnimTask allocation. */
void func_80022EB8(void* pTask)
{
    u8* t = pTask;
    u8* sp = (u8*)(uintptr_t)*(u32*)(t + 0x4);
    u8* pBase = (u8*)(uintptr_t)*(u32*)(sp + 0x20);
    u32 v;

    if (pBase != NULL) {
        v = *(u32*)(pBase + 0x2C);
        if (v != 0) {
            func_80025180((void*)(uintptr_t)v);
        }
    }
    if ((*(u32*)(sp + 0x3C) & 0x3) == 1) {
        pBase = (u8*)(uintptr_t)*(u32*)(sp + 0x20);
        v = *(u32*)(pBase + 0x34);
        if (v != 0) {
            HeapFree((void*)(uintptr_t)v);
        }
    }
    if ((*(u32*)(sp + 0xAC) >> 5) & 1) {
        func_8001CE74(pTask);
    }
    if ((*(u32*)(sp + 0xB0) >> 11) & 1) {
        func_8001D034(pTask);
    }
    if ((*(u32*)(sp + 0x3C) & 0x3) == 1) {
        func_8001D3F4(sp);
    }
    TimerWorkListRemoveTask(pTask);
    WorkListRemoveTask(t + 0x1C);
    HeapFree(pTask);
}

/* asm 80023B84: THE opcode-0xE0 worker — spawn a child sprite driven by the
 * script at pScript, cloning a large slice of the parent's state. Returns
 * the child SpriteData. D_800591AC is suppressed across the spawn when the
 * parent's +0xB0 bit 8 is set (the child then skips the unk14_3 timer flag
 * in TimerWorkListAddTask). */
void* func_80023B84(void* pParentSprite, void* pScript, void* pAnimPackage)
{
    u8* pSrc = pParentSprite;
    u8 savedTimerFlag = D_800591AC;
    s32 type, mode;
    u8* pWrapper;
    u8* pDst;
    u32 v, combo;

    v = *(u32*)(pSrc + 0xB0) | 0x800;
    *(u32*)(pSrc + 0xB0) = v;
    if ((v >> 8) & 1) {
        D_800591AC = 0;
    }

    type = func_80023440(pScript);
    if (type == 3) {
        type = (*(u32*)(pSrc + 0x40) >> 13) & 0xF;
    }
    mode = func_80023468(type);

    pWrapper = func_80023A48(type, mode, pAnimPackage, 0,
                             (void*)(uintptr_t)*(u32*)(pSrc + 0x6C));
    pDst = pWrapper + 0x38;

    *(u32*)(pWrapper + 0x14) |= 0x20000000; /* task1.unk14_1: child marker */

    /* Type into +0x40 bits 13-16, mode into +0x3C bits 0-1. */
    *(u32*)(pDst + 0x40) =
        (*(u32*)(pDst + 0x40) & 0xFFFE1FFF) | ((type & 0xF) << 13);
    *(u32*)(pDst + 0x3C) = (*(u32*)(pDst + 0x3C) & ~0x3u) | (mode & 0x3);

    /* Inherited flag bits. */
    *(u32*)(pDst + 0x40) =
        (*(u32*)(pDst + 0x40) & ~0x1F00u) | (*(u32*)(pSrc + 0x40) & 0x1F00);
    *(u32*)(pDst + 0x3C) =
        (*(u32*)(pDst + 0x3C) & ~0x8u) | (*(u32*)(pSrc + 0x3C) & 0x8);
    *(u32*)(pDst + 0x3C) =
        (*(u32*)(pDst + 0x3C) & ~0x10u) | (*(u32*)(pSrc + 0x3C) & 0x10);
    *(u8*)(pDst + 0x3D) = *(u8*)(pSrc + 0x3D);
    *(u32*)(pDst + 0x40) =
        (*(u32*)(pDst + 0x40) & 0xFFFBFFFF) | (*(u32*)(pSrc + 0x40) & 0x40000);
    *(u32*)(pDst + 0x3C) = (*(u32*)(pDst + 0x3C) | 0x4000000) & ~0x4u;

    *(u32*)(pDst + 0x18) = *(u32*)(pSrc + 0x18);
    *(u16*)(pDst + 0x32) = *(u16*)(pSrc + 0x32);
    *(u16*)(pDst + 0x2C) = *(u16*)(pSrc + 0x2C);
    *(u16*)(pDst + 0x34) = *(u16*)(pSrc + 0x34);

    /* +0xB0 bit 9 (scaled mode): copy; when set, also inherit the slow-motion
     * timer and force +0x40 bits 8-12 to 3. */
    v = (*(u32*)(pSrc + 0xB0) >> 9) & 1;
    *(u32*)(pDst + 0xB0) = (*(u32*)(pDst + 0xB0) & ~0x200u) | (v << 9);
    if (v) {
        *(u16*)(pDst + 0x3A) = *(u16*)(pSrc + 0x3A);
        *(u32*)(pDst + 0x40) = (*(u32*)(pDst + 0x40) & 0xFFFFE0FF) | 0x300;
    }

    /* Parent A8 top 2 bits and AC bits 0-1. */
    combo = ((*(u32*)(pSrc + 0xAC) & 0x3) << 2) | (*(u32*)(pSrc + 0xA8) >> 30);
    *(u32*)(pDst + 0xA8) = (*(u32*)(pDst + 0xA8) & 0x3FFFFFFF) | (combo << 30);
    *(u32*)(pDst + 0xAC) = (*(u32*)(pDst + 0xAC) & ~0x3u) | (combo >> 2);

    /* +0xB0 bit 8; +0xAC bit 6, bits 7-18, bit 2; clear child A8 bit 0. */
    *(u32*)(pDst + 0xB0) =
        (*(u32*)(pDst + 0xB0) & ~0x100u) | (*(u32*)(pSrc + 0xB0) & 0x100);
    *(u32*)(pDst + 0xAC) =
        (*(u32*)(pDst + 0xAC) & ~0x40u) | (*(u32*)(pSrc + 0xAC) & 0x40);
    v = (*(u32*)(pDst + 0xAC) & 0xFFF8007F) | (*(u32*)(pSrc + 0xAC) & 0x7FF80);
    *(u32*)(pDst + 0xAC) = v;
    *(u32*)(pDst + 0xA8) &= ~0x1u;
    *(u32*)(pDst + 0xAC) = (v & ~0x4u) | (*(u32*)(pSrc + 0xAC) & 0x4);

    /* Share the parent's +0x7C state block unless the parent owns it
     * exclusively (A8 bit 0). */
    if (*(u32*)(pSrc + 0xA8) & 0x1) {
        *(u32*)(pDst + 0x7C) = 0;
    } else {
        *(u32*)(pDst + 0x7C) = *(u32*)(pSrc + 0x7C);
    }

    *(u32*)(pDst + 0x70) = (u32)(uintptr_t)pSrc; /* parent back-link */
    *(u32*)(pDst + 0x44) = *(u32*)(pSrc + 0x44);
    *(u32*)(pDst + 0x48) = *(u32*)(pSrc + 0x48);
    *(u32*)(pDst + 0x74) = *(u32*)(pSrc + 0x74);
    *(u16*)(pDst + 0x82) = *(u16*)(pSrc + 0x82);
    *(u32*)(pDst + 0x50) = *(u32*)(pSrc + 0x50);
    *(u8*)(pDst + 0x8D) = *(u8*)(pSrc + 0xAF);
    *(u32*)(pDst + 0x78) = *(u32*)(pSrc + 0x78);
    *(u32*)(pDst + 0x00) = *(u32*)(pSrc + 0x00);
    *(u32*)(pDst + 0x04) = *(u32*)(pSrc + 0x04);
    *(u32*)(pDst + 0x08) = *(u32*)(pSrc + 0x08);
    *(u32*)(pDst + 0x0C) = *(u32*)(pSrc + 0x0C);
    *(u32*)(pDst + 0x10) = *(u32*)(pSrc + 0x10);
    *(u32*)(pDst + 0x14) = *(u32*)(pSrc + 0x14);

    if (mode != 0) {
        u8* srcBase = (u8*)(uintptr_t)*(u32*)(pSrc + 0x20);
        u8* dstBase = (u8*)(uintptr_t)*(u32*)(pDst + 0x20);

        *(u16*)(dstBase + 0x0) = *(u16*)(srcBase + 0x0);
        *(u16*)(dstBase + 0x2) = *(u16*)(srcBase + 0x2);
        *(u16*)(dstBase + 0x4) = *(u16*)(srcBase + 0x4);
        *(u16*)(dstBase + 0x6) = *(u16*)(srcBase + 0x6);
        *(u16*)(dstBase + 0x8) = *(u16*)(srcBase + 0x8);
        *(u16*)(dstBase + 0xA) = *(u16*)(srcBase + 0xA);
    }

    func_80023538(pDst, pScript);
    func_80024730(pWrapper);
    D_800591AC = savedTimerFlag;
    return pDst;
}

/* ---------------------------------------------------------------------------
 * func_8002CC10: native port override for the matching implementation in
 * src/slus_006.64/system/temp2.c. The matching body is decompiled, but the
 * port must keep ownership here because GetTPage and the tpage latch use
 * host PsyCross state. This is anim-script opcode 0x8D: latch the sprite
 * texture-page override from VRAM x/y and select the merge mode
 * (D_80050108 = 1).
 * --------------------------------------------------------------------------- */
extern s32 D_80059310;
extern s32 D_80050108;

void func_8002CC10(s32 x, s32 y)
{
    D_80059310 = GetTPage(0, 0, x & 0xFFFF, y & 0xFFFF) & 0x1F;
    D_80050108 = 1;
}

/* ---------------------------------------------------------------------------
 * func_8002C59C (asm/slus_006.64/nonmatchings/system/temp2/func_8002C59C.s,
 * INCLUDE_ASM at temp2.c:240): in-place pointer relocation of a sprite model
 * blob, sibling of the already-real func_8002C3E8 (temp2.c:193) with a
 * different header layout. One-shot (header flag bit 0x20): relocates the
 * header offset words +0x8/+0x10/+0xC/+0x14 by the blob base, then, when the
 * +0x1C sub-list offset is non-zero, relocates it and walks its 12-byte-
 * stride entries (count word first, then entries; count == -1 means none)
 * from index count down to 0, relocating each entry's +0x4/+0x8. Stores are
 * truncated u32 (host RAM below 4 GiB), matching the retail `sw` width.
 * Used by anim-script opcode 0xF5's model-data load.
 * --------------------------------------------------------------------------- */
void func_8002C59C(u8* pModel)
{
    u16 flags = *(u16*)(pModel + 0x0);
    u32 base = (u32)(uintptr_t)pModel;
    u32 off;
    u8* pEntry;
    s32 count;

    if (flags & 0x20) {
        return;
    }
    *(u16*)(pModel + 0x0) = flags | 0x20;
    *(u32*)(pModel + 0x8) += base;
    *(u32*)(pModel + 0x10) += base;
    *(u32*)(pModel + 0xC) += base;
    *(u32*)(pModel + 0x14) += base;

    off = *(u32*)(pModel + 0x1C);
    if (off == 0) {
        return;
    }
    *(u32*)(pModel + 0x1C) = base + off;

    count = *(s32*)(pModel + off);
    if (count != -1) {
        /* Decrement-first walk, entries count down to 0 (count+1 total);
         * same do-while shape as the matched sibling func_8002C3E8. */
        pEntry = pModel + off + 0x4 + count * 12;
        do {
            count--;
            *(u32*)(pEntry + 0x4) += base;
            *(u32*)(pEntry + 0x8) += base;
            pEntry -= 12;
        } while (count != -1);
    }
}

/* Shared native decompilation of retail battle primitive RGB recoloring. */
#include "../../src/battle/primitive_colors.inc"

/* Native animation-render callback 15 (func_800257F0) and its battle-overlay
 * renderer dependency (func_800B1F6C / func_800B1F0C). */
#include "../../src/battle/anim_render_index15.inc"

/* Retail data, loaded from the user's disc before main() (retail_data.h):
 * symbol, retail file, file offset (from the guest address), size. */
#include "retail_data.h"

_Static_assert(sizeof(g_ControllerButtonMappings) == 0x8, "g_ControllerButtonMappings size");
_Static_assert(sizeof(g_ControllerButtonMasks) == 0x10, "g_ControllerButtonMasks size");
_Static_assert(sizeof(g_GameStateOverlayArchiveOffsets) == 0x1C, "g_GameStateOverlayArchiveOffsets size");
_Static_assert(sizeof(D_8004F308) == 0x4, "D_8004F308 size");
_Static_assert(sizeof(D_8004F324) == 0x4, "D_8004F324 size");
_Static_assert(sizeof(D_8004F328) == 0x4, "D_8004F328 size");
_Static_assert(sizeof(D_8004F330) == 0x4, "D_8004F330 size");
_Static_assert(sizeof(D_8004F334) == 0x4, "D_8004F334 size");
_Static_assert(sizeof(D_8004F338) == 0x4, "D_8004F338 size");
_Static_assert(sizeof(D_8004F340) == 0x4, "D_8004F340 size");
_Static_assert(sizeof(g_GameSceneMapNum) == 0x4, "g_GameSceneMapNum size");

XENO_RETAIL_DATA_BEGIN(game_overrides)
    XENO_RD(s_GameStateSource, XENO_RD_SLUS, XENO_RD_SLUS_OFF(0x8001808Cu), sizeof(s_GameStateSource)),
    XENO_RD(D_800500F8, XENO_RD_SLUS, XENO_RD_SLUS_OFF(0x800500F8u), sizeof(D_800500F8)),
    XENO_RD(D_800500FC, XENO_RD_SLUS, XENO_RD_SLUS_OFF(0x800500FCu), sizeof(D_800500FC)),
    XENO_RD(D_80050100, XENO_RD_SLUS, XENO_RD_SLUS_OFF(0x80050100u), sizeof(D_80050100)),
    XENO_RD(D_80050104, XENO_RD_SLUS, XENO_RD_SLUS_OFF(0x80050104u), sizeof(D_80050104)),
    XENO_RD(g_ControllerButtonMappings, XENO_RD_SLUS, XENO_RD_SLUS_OFF(0x80050238u), 0x8),
    XENO_RD(g_ControllerButtonMasks, XENO_RD_SLUS, XENO_RD_SLUS_OFF(0x800501E8u), 0x10),
    XENO_RD(g_GameStateOverlayArchiveOffsets, XENO_RD_SLUS, XENO_RD_SLUS_OFF(0x8004EAA0u), 0x1C),
    XENO_RD(D_8004F308, XENO_RD_SLUS, XENO_RD_SLUS_OFF(0x8004F308u), 0x4),
    XENO_RD(D_8004F324, XENO_RD_SLUS, XENO_RD_SLUS_OFF(0x8004F324u), 0x4),
    XENO_RD(D_8004F328, XENO_RD_SLUS, XENO_RD_SLUS_OFF(0x8004F328u), 0x4),
    XENO_RD(D_8004F330, XENO_RD_SLUS, XENO_RD_SLUS_OFF(0x8004F330u), 0x4),
    XENO_RD(D_8004F334, XENO_RD_SLUS, XENO_RD_SLUS_OFF(0x8004F334u), 0x4),
    XENO_RD(D_8004F338, XENO_RD_SLUS, XENO_RD_SLUS_OFF(0x8004F338u), 0x4),
    XENO_RD(D_8004F340, XENO_RD_SLUS, XENO_RD_SLUS_OFF(0x8004F340u), 0x4),
    XENO_RD(g_GameSceneMapNum, XENO_RD_SLUS, XENO_RD_SLUS_OFF(0x8004F34Cu), 0x4),
XENO_RETAIL_DATA_END(game_overrides)

/* Read the guest's descriptor rows and bind each documented entry point to its
 * callable host implementation. The rows contain three independent strides. */
__attribute__((constructor)) static void LoadModelPrimDescriptors(void)
{
    u32 rows[17][10] = {{0}};
    const XenoRetailRange range =
        XENO_RD(rows, XENO_RD_SLUS, XENO_RD_SLUS_OFF(0x8004FE50u), sizeof(rows));
    XenoRetailData_LoadTable("model-primitives", &range, 1);
    for (unsigned i = 0; i < 17; ++i) {
        ModelPrimDesc *desc = &D_8004FE50[i];
        for (unsigned j = 0; j < 6; ++j)
            desc->proc[j] = ModelPrimWalker(rows[i][j]);
        desc->buildProc = ModelPrimBuilder(rows[i][6]);
        desc->cmdStride = rows[i][7];
        desc->packetStride = rows[i][8];
        desc->outputStride = rows[i][9];
    }
}
