/*
 * Phase 0 entry point for the Xenogears native PC port.
 *
 * Right now this only proves the integration boundary: it links the port
 * executable against PsyCross (the PSX hardware abstraction layer) and brings
 * the runtime up and down. In Phase 1 this will hand control to the game's own
 * entry point after asset/disc setup, with PsyCross standing in for the PSX
 * GPU/SPU/GTE/CD hardware.
 */

#include <stdio.h>
#include "retail_data.h"
#include "disc_check.h"
#include "../include/xg_plat/disc.h"
#include "../include/xg_plat/renderer.h"
#include "../include/xg_plat/config.h"
#include "../include/xg_plat/mods.h"
#include "cheat_console.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdatomic.h>

#include "xeno_pc.h"
#include "psx_memory.h"
#include "test_input.h"
#include "boot_menu.h"
#include "field_direct_boot_route.h"
#include "boot_sound_banks.h"
#include "boot_sound_commit.h"
#include "PsyX/PsyX_public.h"
#include "psx/libspu.h"   /* Phase-2 sound-SDK primitive probe: SpuReverbAttr/SpuCommonAttr + prims */

/* Forward-declared to avoid pulling the full PsyQ headers (libgpu needs libgte
 * first, etc.). Signatures match PsyCross. */
extern int ResetCallback(void);
extern int ResetGraph(int mode);
extern int SetVideoMode(int mode);
extern void SpuInit(void);            /* PsyCross LIBSPU: wakes the OpenAL SPU backend */

/* Phase-1 sound-pump synthetic probe (env XENO_SOUND_PUMP_PROBE=1). Registers a
 * counter callback via the real OpenEvent on the RCnt2 (counter-2) event,
 * enables it, measures the dispatch rate, disables it, and confirms dispatch
 * stops -- validating the pump architecture independent of SoundInitialize. */
extern int OpenEvent(unsigned int event, int spec, int mode, long(*func)());
extern int EnableEvent(unsigned int event);
extern int DisableEvent(unsigned int event);
extern int CloseEvent(unsigned int event);
#define PORT_RCntCNT2 0xF2000002u   /* DescRC|0x02 */
#define PORT_EvSpINT  0x0002
#define PORT_EvMdINTR 0x1000
extern int PsyX_SPUAL_IsInit(void);
/* _Atomic: incremented on the pump thread, read on main -- keeps the probe
 * counter itself out of TSan reports (the gate validation regime). */
static _Atomic long s_soundPumpProbeCount = 0;
static long PortSoundPumpProbe(void) { s_soundPumpProbeCount++; return 0; }
static void PortSleepMs(long ms) {
    struct timespec ts;
    ts.tv_sec  = ms / 1000;
    ts.tv_nsec = (ms % 1000) * 1000000L;
    nanosleep(&ts, NULL);
}
/* Phase-1 sound-pump synthetic probe: proves the counter-2 pump dispatches an
 * OpenEvent-registered callback at ~240Hz when enabled and stops when disabled,
 * independent of SoundInitialize (which registers the real func_8003C020 in a
 * later phase). Env-gated diagnostic; runs before MainLoop. */
static void PortRunSoundPumpProbe(void) {
    int handle;
    long c_enabled, c_disabled_start, c_disabled_end;
    int rate_ok, stop_ok;
    printf("[sound-probe] g_spuInit=%d (Phase 0, want 1)\n", PsyX_SPUAL_IsInit());
    handle = OpenEvent(PORT_RCntCNT2, PORT_EvSpINT, PORT_EvMdINTR, PortSoundPumpProbe);
    printf("[sound-probe] OpenEvent(RCntCNT2) handle=%d (want >0)\n", handle);
    s_soundPumpProbeCount = 0;
    EnableEvent(handle);
    PortSleepMs(500);
    c_enabled = s_soundPumpProbeCount;         /* ~120 ticks @ 240Hz over 0.5s */
    DisableEvent(handle);
    PortSleepMs(50);                            /* let any in-flight tick settle */
    c_disabled_start = s_soundPumpProbeCount;
    PortSleepMs(200);
    c_disabled_end = s_soundPumpProbeCount;
    CloseEvent(handle);
    rate_ok = (c_enabled > 90 && c_enabled < 150);            /* 240Hz, ~25% band */
    stop_ok = (c_disabled_end == c_disabled_start);
    printf("[sound-probe] enabled 500ms -> %ld ticks (~%.0f Hz, want ~240)\n",
           c_enabled, c_enabled / 0.5);
    printf("[sound-probe] disabled 200ms -> +%ld ticks (want 0)\n",
           c_disabled_end - c_disabled_start);
    printf("[sound-probe] RESULT: rate_ok=%d stop_ok=%d handle_ok=%d -> %s\n",
           rate_ok, stop_ok, (handle > 0),
           (rate_ok && stop_ok && handle > 0) ? "PASS" : "FAIL");
}

/* Phase-2 sound-SDK primitive probe (env XENO_SOUND_PRIM_PROBE=1). Behavioral,
 * NO oracle: exercises the 5 init-reached SDK primitives (SpuSetReverbModeType/
 * Depth, SpuSetCommonAttr, SpuSetIRQ, SpuSetIRQCallback) plus the reverb-param
 * round-trip against the awake backend and reads the state back. This validates
 * the primitives in isolation (as the pump probe validates the pump); it is NOT
 * the init happy path itself -- SoundInitialize drives them for real. */
extern float PsyX_SPUAL_GetMasterVolume(void);
static void PortSoundIrqCb1(void) {}
static void PortSoundIrqCb2(void) {}
static void PortRunSoundPrimProbe(void) {
    SpuReverbAttr rv;
    SpuCommonAttr cm;
    int t_mode, t_depthL, t_depthR, t_master, t_irq, t_cb;
    float mg;
    SpuIRQCallbackProc old1, old2;

    /* 1. reverb type + depth -> shared state -> read back via SpuGetReverbModeParam */
    SpuSetReverbModeType(SPU_REV_MODE_HALL);
    SpuSetReverbModeDepth((short)0x4000, (short)0x5000);
    memset(&rv, 0, sizeof(rv));
    SpuGetReverbModeParam(&rv);
    t_mode   = (rv.mode == SPU_REV_MODE_HALL);
    t_depthL = (rv.depth.left  == (short)0x4000);
    t_depthR = (rv.depth.right == (short)0x5000);

    /* 2. common-attr master volume -> backend listener gain (0x2000/16384 = 0.5) */
    memset(&cm, 0, sizeof(cm));
    cm.mask = SPU_COMMON_MVOLL | SPU_COMMON_MVOLR;
    cm.mvol.left  = (short)0x2000;
    cm.mvol.right = (short)0x2000;
    SpuSetCommonAttr(&cm);
    mg = PsyX_SPUAL_GetMasterVolume();
    t_master = (mg > 0.45f && mg < 0.55f);

    /* 3. IRQ enable state + callback register round-trip (registered, never fired) */
    t_irq = (SpuSetIRQ(SPU_ON) == SPU_ON);
    old1 = SpuSetIRQCallback(PortSoundIrqCb1);   /* returns previous (NULL) */
    old2 = SpuSetIRQCallback(PortSoundIrqCb2);   /* returns PortSoundIrqCb1 */
    t_cb = (old1 == NULL && old2 == PortSoundIrqCb1);
    SpuSetIRQCallback(NULL);
    SpuSetIRQ(SPU_OFF);

    printf("[sound-prim] reverb round-trip: mode=%d(HALL ok=%d) depthL=0x%04x(ok=%d) depthR=0x%04x(ok=%d)\n",
           rv.mode, t_mode, (unsigned short)rv.depth.left, t_depthL,
           (unsigned short)rv.depth.right, t_depthR);
    printf("[sound-prim] mixer master vol -> listener gain=%.3f (want ~0.5, ok=%d)\n", mg, t_master);
    printf("[sound-prim] IRQ set=%d cb round-trip ok=%d\n", t_irq, t_cb);
    printf("[sound-prim] RESULT: %s\n",
           (t_mode && t_depthL && t_depthR && t_master && t_irq && t_cb) ? "PASS" : "FAIL");
}

/* Init-proof milestone: the game's own SoundInitialize(0) routed into boot, plus
 * a coherence probe. Reads BACK audio-manager fields (not just non-null) to
 * defeat the hollow-init trap: after init, D_800595D8 must point at a manager
 * whose func_8003B32C / SoundInitializeAudioManager fields are set. */
extern void SoundInitialize(int mode);
extern unsigned int D_800595D8;         /* packed audio-manager address */
extern unsigned long g_unk_SoundEvent;  /* RCnt2 tick event handle */
static void PortRunSoundInitProbe(void) {
    unsigned int mgrAddr = D_800595D8;
    unsigned char* mgr = (unsigned char*)(uintptr_t)mgrAddr;
    int ok_addr = (mgrAddr != 0);
    int ec = -1, flags = -1, f32 = -1, f38 = -1, f18 = -1;
    int coherent;

    if (ok_addr) {
        ec    = mgr[0x14];                 /* elementCount u8, want 0x10 */
        flags = *(short*)(mgr + 0x10);     /* unk_Flags, want 2 (func_8003B32C) */
        f18   = mgr[0x18];                 /* unk_0x18 u8, want 0x7F (func_8003B32C) */
        f32   = *(short*)(mgr + 0x32);     /* unk_0x32, want 1 (SoundInitializeAudioManager) */
        f38   = *(short*)(mgr + 0x38);     /* unk_0x38, want 4 */
    }
    printf("[sound-init] D_800595D8=0x%08x (want !=0, ok=%d)\n", mgrAddr, ok_addr);
    printf("[sound-init] manager fields: elementCount=0x%x(want 0x10) flags=%d(want 2) "
           "unk18=0x%x(want 0x7F) unk32=%d(want 1) unk38=%d(want 4)\n",
           ec, flags, f18, f32, f38);
    printf("[sound-init] tick event g_unk_SoundEvent=0x%lx (want !=0)\n",
           (unsigned long)g_unk_SoundEvent);
    coherent = ok_addr && ec == 0x10 && flags == 2 && f18 == 0x7F && f32 == 1 && f38 == 4;
    printf("[sound-init] RESULT: %s\n", coherent ? "PASS (manager coherent)" : "FAIL");

    /* Post-init pump liveness: the real func_8003C020 is now registered on
     * counter-2/EvSpINT and enabled (SoundAddAudioManagerToList). Register a
     * shadow counter on the same event and measure 500ms -- the rate func_8003C020
     * rides. Confirms the pump still dispatches at 240Hz with the real tick live. */
    {
        int h = OpenEvent(PORT_RCntCNT2, PORT_EvSpINT, PORT_EvMdINTR, PortSoundPumpProbe);
        long n;
        s_soundPumpProbeCount = 0;
        EnableEvent(h);
        PortSleepMs(500);
        n = s_soundPumpProbeCount;
        DisableEvent(h);
        CloseEvent(h);
        printf("[sound-init] post-init pump: %ld ticks/500ms (~%.0f Hz, want ~240) -> tick %s\n",
               n, n / 0.5, (n > 90 && n < 150) ? "FIRING" : "NOT firing");
    }
}

/* Gate-stress probe (env XENO_SOUND_GATE_STRESS=1): the concurrency regime for
 * the sound tick gate (g_SoundTickMutex). Validates, with the REAL tick event
 * registered and its stub dispatching at 240Hz:
 *   1. the gated pump still dispatches at ~240Hz (TryLock does not starve);
 *   2. an open DisableEvent bracket freezes tick dispatch while the pump
 *      THREAD stays alive (vblank counter advances -- no pump stall), and
 *      EnableEvent resumes dispatch (retail toggle func_80037F44/F88 shape);
 *   3. under main-thread-vs-tick contention, multi-word invariants written
 *      inside brackets are never observed torn by the tick callback (and vice
 *      versa), the callback's func_8003AE84-style re-entrant bracket does not
 *      self-deadlock, and nested main-thread brackets recurse safely.
 * All shared fields are accessed ONLY under the bracket/mutex -- so a gate
 * regression shows up BOTH as an invariant violation here and as a TSan data
 * race in the XENO_TSAN=1 build. Completion of the probe is itself the
 * no-deadlock proof (a lost lock level or self-deadlock hangs it). */
extern int PsyX_Sys_GetVBlankCount(void);
/* Initialised to a tuple SATISFYING the invariants (y=2x+1, z=x^y; q=3p) so
 * ticks that run before the first main-thread write don't count as torn. */
static struct { long x, y, z; } s_gateA = { 0, 1, 1 };   /* main writes (bracket), tick reads */
static struct { long p, q; }    s_gateB = { 0, 0 };      /* tick writes (re-entrant bracket), main reads (bracket) */
static _Atomic long s_gateTicks       = 0; /* tick callback invocations */
static _Atomic long s_gateTickTorn    = 0; /* tick saw a torn s_gateA */
static _Atomic long s_gateReentries   = 0; /* completed re-entrant brackets on the tick path */
static int s_gateHandle = 0;

static long PortGateStressTick(void) {
    /* Runs on the pump thread, dispatched under g_SoundTickMutex. */
    long x = s_gateA.x, y = s_gateA.y, z = s_gateA.z;
    if (y != 2 * x + 1 || z != (x ^ y))
        s_gateTickTorn++;
    /* func_8003AE84 shape: the tick path itself re-enters the
     * DisableEvent/EnableEvent bracket. Non-recursive would self-deadlock. */
    DisableEvent(s_gateHandle);
    s_gateB.p++;
    s_gateB.q = s_gateB.p * 3;
    EnableEvent(s_gateHandle);
    s_gateReentries++;
    s_gateTicks++;
    return 0;
}

static double PortMonotonicSeconds(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
}

static void PortRunSoundGateStress(void) {
    long rate, frozen0, frozen1, resumed, iters = 0, mainTorn = 0;
    long ticksBefore, ticksDuring;
    int vbl0, vbl1;
    int rate_ok, freeze_ok, alive_ok, resume_ok, torn_ok, reentry_ok, ticked_ok;
    double t0;

    s_gateHandle = OpenEvent(PORT_RCntCNT2, PORT_EvSpINT, PORT_EvMdINTR,
                             PortGateStressTick);
    printf("[gate-stress] OpenEvent handle=%d (want >0)\n", s_gateHandle);

    /* 1. gated pump rate (no contention) */
    s_gateTicks = 0;
    EnableEvent(s_gateHandle);          /* unpaired enable: flag flip only */
    PortSleepMs(500);
    rate = s_gateTicks;
    rate_ok = (rate > 90 && rate < 150);
    printf("[gate-stress] gated pump: %ld ticks/500ms (~%.0f Hz, want ~240, ok=%d)\n",
           rate, rate / 0.5, rate_ok);

    /* 2. held bracket freezes dispatch but not the pump thread */
    DisableEvent(s_gateHandle);         /* bracket open: mutex HELD */
    frozen0 = s_gateTicks;
    vbl0 = PsyX_Sys_GetVBlankCount();
    PortSleepMs(200);
    frozen1 = s_gateTicks;
    vbl1 = PsyX_Sys_GetVBlankCount();
    EnableEvent(s_gateHandle);          /* bracket close */
    PortSleepMs(200);
    resumed = s_gateTicks;
    freeze_ok = (frozen1 == frozen0);
    alive_ok  = (vbl1 > vbl0);
    resume_ok = (resumed > frozen1 + 20);
    printf("[gate-stress] bracket held 200ms: +%ld ticks (want 0, ok=%d); "
           "vblank advanced %d (want >0: pump thread alive, ok=%d); "
           "resumed +%ld after release (ok=%d)\n",
           frozen1 - frozen0, freeze_ok, vbl1 - vbl0, alive_ok,
           resumed - frozen1, resume_ok);

    /* 3. contention: hammer brackets + invariants from main for ~2s */
    ticksBefore = s_gateTicks;
    t0 = PortMonotonicSeconds();
    while (PortMonotonicSeconds() - t0 < 2.0) {
        long i = ++iters, p, q;
        DisableEvent(s_gateHandle);     /* bracket open */
        s_gateA.x = i;                  /* multi-word write the tick must */
        s_gateA.y = 2 * i + 1;          /* never observe half-done       */
        s_gateA.z = i ^ (2 * i + 1);
        DisableEvent(s_gateHandle);     /* nested bracket (recursion) */
        p = s_gateB.p; q = s_gateB.q;
        if (q != p * 3)
            mainTorn++;
        EnableEvent(s_gateHandle);      /* close nested */
        EnableEvent(s_gateHandle);      /* close outer */
        if ((iters & 0x3FF) == 0)
            PortSleepMs(1);             /* windows for the tick to win the lock */
    }
    ticksDuring = s_gateTicks - ticksBefore;
    torn_ok    = (s_gateTickTorn == 0 && mainTorn == 0);
    reentry_ok = (s_gateReentries > 0);
    ticked_ok  = (ticksDuring > 50);
    printf("[gate-stress] contention 2s: %ld bracket pairs, %ld ticks ran (ok=%d), "
           "%ld re-entrant brackets (ok=%d)\n",
           iters, ticksDuring, ticked_ok, (long)s_gateReentries, reentry_ok);
    printf("[gate-stress] torn reads: tick=%ld main=%ld (want 0/0, ok=%d)\n",
           (long)s_gateTickTorn, mainTorn, torn_ok);

    DisableEvent(s_gateHandle);
    CloseEvent(s_gateHandle);           /* dissolves the open bracket (probe shape) */

    /* 4. post-close: the CloseEvent bracket-dissolve must have released the
     * mutex -- the REAL tick (func_8003C020 stub, enabled by SoundInitialize)
     * must still be dispatching or every later tick is silenced. */
    s_soundPumpProbeCount = 0;
    {
        int h = OpenEvent(PORT_RCntCNT2, PORT_EvSpINT, PORT_EvMdINTR,
                          PortSoundPumpProbe);
        long n;
        EnableEvent(h);
        PortSleepMs(300);
        n = s_soundPumpProbeCount;
        DisableEvent(h);
        CloseEvent(h);
        printf("[gate-stress] post-close pump: %ld ticks/300ms (want >40: close "
               "released the held bracket, ok=%d)\n", n, n > 40);
        rate_ok = rate_ok && (n > 40);
    }

    printf("[gate-stress] RESULT: %s (rate=%d freeze=%d alive=%d resume=%d "
           "torn=%d reentry=%d ticked=%d)\n",
           (rate_ok && freeze_ok && alive_ok && resume_ok && torn_ok &&
            reentry_ok && ticked_ok) ? "PASS" : "FAIL",
           rate_ok, freeze_ok, alive_ok, resume_ok, torn_ok, reentry_ok,
           ticked_ok);
}

/* Synthetic sequence-command probe (env XENO_SOUND_SEQ_PROBE=1): tick-leg
 * step 3 validation. Real sequence data needs WDS/B5 (deferred), so this
 * feeds a hand-built command stream to the live 240Hz interpreter instead:
 * element 0 of the initialized manager is pointed at the stream and activated
 * under the DisableEvent/EnableEvent bracket, the tick's func_8003C6E8 then
 * dispatches through the host g_SoundScriptHandlers table. Five opcodes with
 * five distinct observable effects prove TABLE ROUTING (a mis-routed opcode
 * writes the wrong field and/or desynchronizes the stream so the final IP
 * check fails):
 *   0x97 func_8003CE68  time signature   -> manager unk_0x3a/3c/38
 *   0xA0 func_8003D0E8  volume immediate -> manager unk_0x58
 *   0xE9 func_8003DEB4  pan nudge        -> element +0x74
 *   0xC1 func_8003D60C  raw ADSR         -> element +0x54..56
 *   0xA9 SoundScriptSetUnk62 (pre-batch {}) -> element +0x62
 *   0x80 func_8003CD08  rest             -> fermata + REST exit
 * SYNTHETIC data, labeled as such: full live exercise with real sequence
 * banks awaits the WDS/B5 leg. Diagnostic only; does not run in normal boot. */
static const unsigned char s_seqProbeStream[] = {
    0x97, 0x03, 0x04,       /* time signature (func_8003CE68)          */
    0xA0, 0x55,             /* volume immediate (func_8003D0E8)        */
    0x98, 0x02,             /* loop start, 2 iterations (func_8003CEF0) */
    0xA1, 0x03,             /* volume nudge +3<<16 in-loop (func_8003D110) */
    0x99,                   /* loop continue (func_8003CF38); exhausts + pops
                             * after 2 iterations. NOTE: 0x9A is an early-exit-
                             * INSIDE-loop construct, not a terminator (3d) */
    0xE9, 0x10,             /* pan nudge -> 0x1000 (func_8003DEB4)     */
    0xC1, 0x11, 0x22, 0x33, /* raw ADSR (func_8003D60C)                */
    0xA9, 0x44,             /* unk62 (SoundScriptSetUnk62)             */
    0xE1, 0x05,             /* vib accumulator nudge (func_8003DB58)   */
    0xA7, 0x02, 0x60,       /* channel fade: cnt 0x40, tgt 0x6000 (func_8003D1BC) */
    0xD4, 0x04, 0x20,       /* pitch slide arm (func_8003D7FC)         */
    0xEA, 0x08, 0x30,       /* pan fade, completes at scaled 0x2000 (func_8003DEE4 + C4C4 fade path) */
    0xFD, 0x02,             /* master level (func_8003E4BC): restores the
                             * tempo product the in-loop 0xA1 zeroed      */
    0x80, 0xFF,             /* rest (func_8003CD08) -- terminates; 0xFF
                             * outlasts the probe window (guard stays unread
                             * by the interpreter)                        */
    0x00,                   /* scan-safe epilogue: in-bounds <0x80 byte for
                             * C6E8's post-pass tie-scan (3c finding)    */
};
static void PortRunSoundSeqProbe(void) {
    unsigned char* mgr = (unsigned char*)(uintptr_t)D_800595D8;
    unsigned char* el = mgr + 0x94;
    int save58, save54, save50;
    unsigned short save3a, save3c, save38, save36;
        int ok_route, ok_mgr, ok_el, ok_ip, ok_ticked;

    if (mgr == NULL) {
        printf("[seq-probe] no manager (D_800595D8==0) -> SKIP\n");
        return;
    }
    /* Arm under the bracket: script IP -> stream, element active, manager
     * sequencing enabled at one step per tick. */
    DisableEvent(g_unk_SoundEvent);
    save58 = *(int*)(mgr + 0x58); save54 = *(int*)(mgr + 0x54);
    save50 = *(int*)(mgr + 0x50);
    save3a = *(unsigned short*)(mgr + 0x3A); save3c = *(unsigned short*)(mgr + 0x3C);
    save38 = *(unsigned short*)(mgr + 0x38); save36 = *(unsigned short*)(mgr + 0x36);
    *(unsigned short*)(mgr + 0x36) = 4;
    *(int*)(mgr + 0x48) = 1;
    *(int*)(mgr + 0x50) = 0;
    *(int*)(mgr + 0x54) = 0x10000;
    *(unsigned int*)(el + 0x14) = (unsigned int)(uintptr_t)s_seqProbeStream;  /* unk14 script IP */
    *(unsigned short*)(el + 0x02) = 0;                                  /* status */
    *(int*)(el + 0x5C) = 0;                                  /* fermata pair */
    *(unsigned short*)(el + 0x00) = 0x1;                                /* active */
    *(short*)(mgr + 0x10) |= 0x8000;                           /* manager active */
    EnableEvent(g_unk_SoundEvent);

    s_soundPumpProbeCount = 0;
    PortSleepMs(100);   /* ~24 ticks; the stream completes on the first step */
    DisableEvent(g_unk_SoundEvent);

    ok_mgr = (*(unsigned short*)(mgr + 0x3A) == 0x30) && (*(unsigned short*)(mgr + 0x3C) == 4) &&
             (*(unsigned short*)(mgr + 0x38) == 3) && (*(int*)(mgr + 0x58) == 0x5B0000)   /* 0xA0 + loop 2x 0xA1 (+3<<16): CEF0/CF38/CFA4 live */;
    ok_el = (*(unsigned short*)(el + 0x74) == 0x2000) &&   /* pan fade completed (retail sets scaled delta on final step) */
            (*(unsigned char*)(el + 0x54) == 0x11) && (*(unsigned char*)(el + 0x55) == 0x22) &&
            (*(unsigned char*)(el + 0x56) == 0x33) && (*(unsigned short*)(el + 0x62) == 0x44) &&
            (*(int*)(el + 0x78) == 0x05000000) &&               /* vib acc (0xE1) */
            (*(int*)(el + 0x84) == 0x08000000) &&               /* slide step (0xD4) */
            (*(unsigned short*)(mgr + 0x7A) == 0x6000);          /* interp70 target (0xA7) */
    ok_ip = (*(unsigned int*)(el + 0x14) ==
             (unsigned int)(uintptr_t)(s_seqProbeStream + sizeof(s_seqProbeStream) - 1));
    {
        unsigned short fermata = *(unsigned short*)(el + 0x5C);
        ok_ticked = (fermata > 0 && fermata <= 0xFF);
    }
    ok_route = ok_mgr && ok_el && ok_ip;
    printf("[seq-probe] mgr: sig=%x/%x beats=%x vol58=%08x (ok=%d)\n",
           *(unsigned short*)(mgr + 0x3A), *(unsigned short*)(mgr + 0x3C), *(unsigned short*)(mgr + 0x38),
           *(int*)(mgr + 0x58), ok_mgr);
    printf("[seq-probe] el: pan=%04x adsr=%02x/%02x/%02x unk62=%04x (ok=%d)\n",
           *(unsigned short*)(el + 0x74), *(unsigned char*)(el + 0x54), *(unsigned char*)(el + 0x55),
           *(unsigned char*)(el + 0x56), *(unsigned short*)(el + 0x62), ok_el);
    printf("[seq-probe] ip advanced to end=%d fermata=0x%x counting=%d\n",
           ok_ip, *(unsigned short*)(el + 0x5C), ok_ticked);
    printf("[seq-probe] RESULT: %s (routing=%d, SYNTHETIC stream; real "
           "sequence data awaits WDS/B5)\n",
           (ok_route && ok_ticked) ? "PASS" : "FAIL", ok_route);

    /* Teardown: park the element + manager back to the pre-probe state. */
    *(unsigned short*)(el + 0x00) = 0;
    *(unsigned short*)(el + 0x02) = 0;
    *(int*)(el + 0x5C) = 0;
    *(unsigned int*)(el + 0x14) = 0;
    *(short*)(mgr + 0x10) &= 0x7FFF;
    *(int*)(mgr + 0x48) = 0;
    *(int*)(mgr + 0x58) = save58; *(int*)(mgr + 0x54) = save54;
    *(int*)(mgr + 0x50) = save50;
    *(unsigned short*)(mgr + 0x3A) = save3a; *(unsigned short*)(mgr + 0x3C) = save3c;
    *(unsigned short*)(mgr + 0x38) = save38; *(unsigned short*)(mgr + 0x36) = save36;
    EnableEvent(g_unk_SoundEvent);
}

/* B5.1 pass 2 -- the register->backend translator (FIRST AUDIBLE wiring).
 * The tick's voice-register flush (func_8003E900/EB5C, objdiff {}) writes
 * key-on/off + voice params into the static SpuUnion backing page; the real
 * backend is driven by SpuSetVoiceAttr/SpuSetKey (-> alSourcePlay). This
 * translator is registered as a second counter-2 event (OpenEvent slot AFTER
 * the tick's), so the pump dispatches it at 240Hz right after func_8003C020,
 * under the same g_SoundTickMutex bracket -- gate-serialized by construction.
 * Edge semantics: real KON/KOFF registers are write-triggered; the translator
 * consumes (clears) the page words after driving the backend.
 * ADSR phase 1: the raw ADSR1/ADSR2 register words (page +0x8/+0xA, written
 * by func_8003E900's dirty-flag flush) are forwarded too -- at KON and
 * change-detected -- and the backend's psx-spx envelope machine is advanced
 * here by this tick's share of the 44100Hz clock (PsyX_SPUAL_EnvelopeTick). */
extern void PsyX_SPUAL_EnvelopeTick(int cycles);
static _Atomic long s_regFlushKeyOns = 0;
static _Atomic long s_regFlushKeyOffs = 0;
static _Atomic long s_regFlushTicks = 0;
static int s_konTrace = -1;
static long PcPort_SpuRegFlushTick(void) {
    if (s_konTrace < 0)
        s_konTrace = (getenv("XENO_SOUND_KON_TRACE") != NULL);
    s_regFlushTicks += 1;
    extern void* g_pSoundSpuRegisters;
    unsigned char* spu = (unsigned char*)g_pSoundSpuRegisters;
    unsigned int kon = *(unsigned short*)(spu + 0x188) |
                       (*(unsigned short*)(spu + 0x18A) << 16);
    unsigned int koff = *(unsigned short*)(spu + 0x18C) |
                        (*(unsigned short*)(spu + 0x18E) << 16);
    if (kon) {
        int i;
        for (i = 0; i < 24; i++) {
            if (kon & (1u << i)) {
                unsigned char* v = spu + i * 0x10;
                SpuVoiceAttr attr;
                memset(&attr, 0, sizeof(attr));
                attr.voice = 1u << i;
                attr.mask = SPU_VOICE_VOLL | SPU_VOICE_VOLR | SPU_VOICE_PITCH |
                            SPU_VOICE_WDSA | SPU_VOICE_ADSR_ADSR1 |
                            SPU_VOICE_ADSR_ADSR2;
                attr.volume.left = *(short*)(v + 0x0);
                attr.volume.right = *(short*)(v + 0x2);
                attr.pitch = *(unsigned short*)(v + 0x4);
                attr.addr = *(unsigned short*)(v + 0x6) << 3;
                attr.adsr1 = *(unsigned short*)(v + 0x8);
                attr.adsr2 = *(unsigned short*)(v + 0xA);
                if (s_regFlushKeyOns < 4 || s_konTrace)
                    printf("[reg-flush] KON t=%.1fs v%d voll=%d volr=%d "
                           "pitch=0x%x addr=0x%x\n",
                           (double)s_regFlushTicks / 240.0, i,
                           attr.volume.left, attr.volume.right,
                           (unsigned)attr.pitch, (unsigned)attr.addr);
                SpuSetVoiceAttr(&attr);
            }
        }
        SpuSetKey(SPU_ON, kon);
        s_regFlushKeyOns += 1;
        *(unsigned short*)(spu + 0x188) = 0;
        *(unsigned short*)(spu + 0x18A) = 0;
    }
    if (koff) {
        SpuSetKey(SPU_OFF, koff);
        s_regFlushKeyOffs += 1;
        *(unsigned short*)(spu + 0x18C) = 0;
        *(unsigned short*)(spu + 0x18E) = 0;
    }
    {
        /* B5.2 continuous attr flush: envelopes (and fades) modulate
         * voll/volr/pitch through the register page every tick (func_8003E900
         * rewrites dirty registers), but real hardware applies page state
         * continuously while the key-on path above is edge-triggered. Push
         * ongoing register changes for assigned voices to the backend,
         * change-detected so OpenAL only hears actual updates. */
        static unsigned short s_lastAttr[24][5];
        extern void* g_SoundChannels[24];
        int v;
        for (v = 0; v < 24; v++) {
            unsigned char* pv = spu + v * 0x10;
            unsigned short voll = *(unsigned short*)(pv + 0x0);
            unsigned short volr = *(unsigned short*)(pv + 0x2);
            unsigned short pit = *(unsigned short*)(pv + 0x4);
            unsigned short a1 = *(unsigned short*)(pv + 0x8);
            unsigned short a2 = *(unsigned short*)(pv + 0xA);
            if (g_SoundChannels[v] == NULL)
                continue;
            if (voll != s_lastAttr[v][0] || volr != s_lastAttr[v][1] ||
                pit != s_lastAttr[v][2] || a1 != s_lastAttr[v][3] ||
                a2 != s_lastAttr[v][4]) {
                SpuVoiceAttr attr;
                memset(&attr, 0, sizeof(attr));
                attr.voice = 1u << v;
                attr.mask = SPU_VOICE_VOLL | SPU_VOICE_VOLR | SPU_VOICE_PITCH |
                            SPU_VOICE_ADSR_ADSR1 | SPU_VOICE_ADSR_ADSR2;
                attr.volume.left = voll;
                attr.volume.right = volr;
                attr.pitch = pit;
                attr.adsr1 = a1;
                attr.adsr2 = a2;
                SpuSetVoiceAttr(&attr);
                s_lastAttr[v][0] = voll;
                s_lastAttr[v][1] = volr;
                s_lastAttr[v][2] = pit;
                s_lastAttr[v][3] = a1;
                s_lastAttr[v][4] = a2;
            }
        }
    }
    {
        /* ADSR phase 1: advance the backend envelope machine by this tick's
         * share of the 44100Hz envelope clock. 44100/240 = 183.75 -- the
         * fractional accumulator keeps the long-run rate exact (184,184,184,
         * 183 repeating). Runs under the same gate bracket as the tick. */
        static int s_envCycleAcc = 0;
        int cycles;
        s_envCycleAcc += 44100;
        cycles = s_envCycleAcc / 240;
        s_envCycleAcc -= cycles * 240;
        PsyX_SPUAL_EnvelopeTick(cycles);
    }
    return 0;
}

/* B5.1 play probe (env XENO_SOUND_PLAY_PROBE=1): with the real WDS bank
 * loaded (pass 1), arm element 0 with a note-on stream (bank+instrument via
 * 0xFC, note 0x30 vel 0x05, long rest). The tick interprets, keys the voice
 * on, the translator drives the backend -> alSourcePlay. Proof tiers:
 * translator counters here; the WAV-capture RMS check is the runner's job
 * (ALSOFT wave backend). SYNTHETIC note; real sequences await field music. */
static const unsigned char s_playProbeStream[] = {
    0xFC, 0x00, 0x00,   /* bank (list head fallback) + instrument 0 */
    0xE0, 0x7F,         /* element volume accumulator = 0x7F<<24 (el+0x78;
                         * its hi16 feeds the voll chain -- 0 == silence) */
    0x30, 0x05,         /* note 0x30, velocity 5 (duration table: 64 steps) */
    0x80, 0xFF,         /* rest outlasting the window */
    0x00,               /* scan-safe guard */
};
static void PortRunSoundPlayProbe(void) {
    unsigned char* mgr = (unsigned char*)(uintptr_t)D_800595D8;
    unsigned char* el = mgr + 0x94;
    long kons0 = s_regFlushKeyOns;
    int ok_keyed, ok_chan, ok_playing;
    int saved54;
    extern void* g_SoundChannels[24];

    if (mgr == NULL) { printf("[play-probe] no manager -> SKIP\n"); return; }
    DisableEvent(g_unk_SoundEvent);
    *(unsigned short*)(mgr + 0x36) = 4;
    *(int*)(mgr + 0x48) = 1;
    *(int*)(mgr + 0x50) = 0;
    saved54 = *(int*)(mgr + 0x54);
    *(int*)(mgr + 0x54) = 0x4000;                 /* gentle tempo */
    *(int*)(mgr + 0x70) = 0x7FFF0000;             /* channel level interp
                                                   * (hi16 scales every voll/
                                                   * volr; 0 == silence) */
    *(unsigned int*)(el + 0x14) = (unsigned int)(uintptr_t)s_playProbeStream;
    *(unsigned short*)(el + 0x02) = 0;
    *(int*)(el + 0x5C) = 0;
    *(unsigned char*)(el + 0x27) = 2;             /* voice_number = 2 */
    *(short*)(el + 0x76) = 0x7FFF;                /* expression factor (st[0x3A]):
                                                   * set by the unported song-
                                                   * start path; the probe stands
                                                   * in for it (0 == silence) */
    *(unsigned short*)(el + 0x00) = 0x401;        /* active + RESTING: the note
                                                   * path keys on via status|=1
                                                   * only from the rest state */
    *(short*)(mgr + 0x10) |= 0x8000;
    EnableEvent(g_unk_SoundEvent);
    {
        /* AL source-state tier: poll the backend during the window; the voice
         * must be observed AL_PLAYING at least once. */
        extern int SpuGetKeyStatus(unsigned int voice_bit);
        int t;
        ok_playing = 0;
        for (t = 0; t < 30; t++) {
            PortSleepMs(10);
            if (SpuGetKeyStatus(1u << 2))
                ok_playing = 1;
        }
    }
    DisableEvent(g_unk_SoundEvent);
    ok_keyed = (s_regFlushKeyOns > kons0);
    ok_chan = (g_SoundChannels[2] != NULL);
    printf("[play-probe] keyons=%ld (delta ok=%d) chan2=%d vd.start=0x%x pitch=0x%x\n",
           (long)s_regFlushKeyOns, ok_keyed, ok_chan,
           *(unsigned int*)(el + 0x4C), *(unsigned short*)(el + 0x44));
    printf("[play-probe] vol-chain: el76=%d el7A=%d elD2=%d mgr70hi=%d "
           "vd.voll=%d vd.volr=%d\n",
           *(short*)(el + 0x76), *(short*)(el + 0x7A), *(short*)(el + 0xD2),
           ((short*)(mgr + 0x70))[1], *(short*)(el + 0x38), *(short*)(el + 0x3A));
    printf("[play-probe] al-source-state: observed playing=%d (want 1)\n",
           ok_playing);
    printf("[play-probe] RESULT: %s (voice keyed + AL source PLAYING; WAV RMS "
           "is the output-tier proof -- see runner)\n",
           (ok_keyed && ok_chan && ok_playing) ? "PASS" : "FAIL");
    /* teardown */
    *(unsigned short*)(el + 0x00) = 0;
    *(unsigned short*)(el + 0x02) = 0;
    *(int*)(el + 0x5C) = 0;
    *(unsigned int*)(el + 0x14) = 0;
    *(short*)(mgr + 0x10) &= 0x7FFF;
    *(int*)(mgr + 0x48) = 0;
    /* restore the pre-probe tempo (zeroing it froze the sequencer for any
     * later sound work in the same run -- caught by the S1 SFX probe) */
    *(int*)(mgr + 0x54) = saved54;
    EnableEvent(g_unk_SoundEvent);
}

/* Song-start M2 probe (env XENO_SOUND_SONG_PROBE=1): buffer-read a REAL
 * song pair from archive dir 0x1C -- WDS bank (default file 0x13, 'wds ')
 * via the proven SoundLoadWdsFile path, then the 'smds' song file (default
 * 0x14) -- and drive the M1 arming chain exactly as retail field code does:
 * func_80039850 (create manager from the song file) -> func_80039A80
 * (start: header init + element arm + level 0x7F + running flag). The
 * already-ported tick then interprets the real sequence at 240Hz and the
 * register->backend translator keys voices. Observation: per-second voice/
 * key-on sampling here; the WAV-capture RMS profile is the runner's proof
 * tier. Teardown via func_80039C4C + func_800399D4 (the real destroy path).
 * Env overrides: XENO_SOUND_SONG_FILE / XENO_SOUND_BANK_FILE (hex). */
static void PortRunSoundSongProbe(void) {
    extern void* func_80039850(void* pSongFile);
    extern void func_80039A80(void* manager, int level, int steps);
    extern void func_80039C4C(void* manager);
    extern void func_800399D4(void* manager);
    extern void* SoundLoadWdsFile(void* pWdsFile, int mode);
    extern int SpuGetKeyStatus(unsigned int voice_bit);
    extern int ArchiveDecodeAlignedSize(unsigned int entryIndex);
    extern void* HeapAlloc(int size, int flags);
    extern void HeapFree(void* pMemory);
    extern int ArchiveSetIndex(int directoryIndex, int entryIndex);
    extern void ArchiveReadFileToBuffer(int fileIndex, void* pBuffer, int arg2,
                                        int arg3);
    int bankFile = 0x13, songFile = 0x14;
    unsigned char* bankEntry;
    unsigned char* mgr;
    void* bankBuf;
    unsigned char* songBuf;
    int sz, t;
    long kons0, koffs0;
    int ok_mgr, ok_run, ok_played, ok_seq;
    int maxVoices = 0, samplesWithSound = 0;

    if (getenv("XENO_SOUND_BANK_FILE")) bankFile = (int)strtol(getenv("XENO_SOUND_BANK_FILE"), NULL, 16);
    if (getenv("XENO_SOUND_SONG_FILE")) songFile = (int)strtol(getenv("XENO_SOUND_SONG_FILE"), NULL, 16);

    ArchiveSetIndex(0x1C, 0);
    sz = ArchiveDecodeAlignedSize(bankFile);
    bankBuf = HeapAlloc(sz, 1);
    ArchiveReadFileToBuffer(bankFile, bankBuf, 0, 0x80);
    bankEntry = (unsigned char*)SoundLoadWdsFile(bankBuf, 0);
    printf("[song-probe] bank file 0x%02x: size=%d entry=%d id=0x%x spuAddr=0x%x\n",
           bankFile, sz, bankEntry != NULL,
           bankEntry ? *(unsigned short*)(bankEntry + 0x20) : 0,
           bankEntry ? *(unsigned int*)(bankEntry + 0x28) : 0);
    HeapFree(bankBuf);
    if (bankEntry == NULL) {
        printf("[song-probe] RESULT: FAIL (bank load)\n");
        return;
    }

    sz = ArchiveDecodeAlignedSize(songFile);
    songBuf = HeapAlloc(sz, 1);
    ArchiveReadFileToBuffer(songFile, songBuf, 0, 0x80);
    printf("[song-probe] song file 0x%02x: size=%d magic=%.4s elemCnt=%d wdsId=0x%x\n",
           songFile, sz, (char*)songBuf, songBuf[0x14],
           *(unsigned short*)(songBuf + 0x16));

    kons0 = s_regFlushKeyOns;
    koffs0 = s_regFlushKeyOffs;
    mgr = (unsigned char*)func_80039850(songBuf);
    ok_mgr = (mgr != NULL);
    printf("[song-probe] manager=%d elemCnt=%d flags=0x%x (create)\n",
           ok_mgr, ok_mgr ? mgr[0x14] : 0,
           ok_mgr ? *(unsigned short*)(mgr + 0x10) : 0);
    if (!ok_mgr) {
        printf("[song-probe] RESULT: FAIL (manager create -- M1 guard territory)\n");
        HeapFree(songBuf);
        ArchiveSetIndex(4, 0);
        return;
    }
    if (getenv("XENO_SOUND_SONG_CONTROL")) {
        /* Silent-control tier: manager created, song NEVER started -- the
         * wave capture must be digitally silent. */
        printf("[song-probe] CONTROL: created but not started\n");
        PortSleepMs(8000);
        printf("[song-probe] CONTROL keyons=%ld (want 0)\n",
               (long)(s_regFlushKeyOns - kons0));
        func_800399D4(mgr);
        HeapFree(songBuf);
        ArchiveSetIndex(4, 0);
        return;
    }
    func_80039A80(mgr, 0x7F, 0);
    ok_run = ((*(unsigned short*)(mgr + 0x10) & 0x8000) != 0);
    printf("[song-probe] started: flags=0x%x run=%d mgr70hi=%d\n",
           *(unsigned short*)(mgr + 0x10), ok_run, ((short*)(mgr + 0x70))[1]);

    /* 8 seconds of real playback, sampled twice a second. */
    for (t = 0; t < 16; t++) {
        int v, playing = 0;
        PortSleepMs(500);
        for (v = 0; v < 24; v++) {
            playing += (SpuGetKeyStatus(1u << v) != 0);
        }
        if (playing > maxVoices) maxVoices = playing;
        if (playing > 0) samplesWithSound++;
        printf("[song-probe] t=%2d.%ds voices=%2d keyons=%ld keyoffs=%ld\n",
               (t + 1) / 2, ((t + 1) % 2) * 5, playing,
               (long)(s_regFlushKeyOns - kons0),
               (long)(s_regFlushKeyOffs - koffs0));
    }
    ok_played = (s_regFlushKeyOns - kons0) > 4 && maxVoices > 0;
    ok_seq = (s_regFlushKeyOns - kons0) > 16 && samplesWithSound >= 8 && maxVoices >= 2;
    printf("[song-probe] totals: keyons=%ld keyoffs=%ld maxVoices=%d "
           "samplesWithSound=%d/16\n",
           (long)(s_regFlushKeyOns - kons0), (long)(s_regFlushKeyOffs - koffs0),
           maxVoices, samplesWithSound);
    printf("[song-probe] RESULT: %s (create=%d run=%d played=%d sequence=%d; "
           "WAV RMS profile is the output-tier proof -- see runner)\n",
           (ok_mgr && ok_run && ok_played && ok_seq) ? "PASS" : "FAIL",
           ok_mgr, ok_run, ok_played, ok_seq);

    func_80039C4C(mgr);
    func_800399D4(mgr);
    HeapFree(songBuf);
    ArchiveSetIndex(4, 0);
}

/* S1 SFX probe (env XENO_SOUND_SFX_PROBE=1): load the field's common SED
 * (archive dir 4 file 0xA8 -- the same file the ported func_80085890 loads)
 * with the real SoundAddSedsEntry, then fire an effect through the real SFX
 * chain: func_80039F18 (the CFF0-spawn/API entry) -> func_8003A65C slot
 * allocation -> func_8003B644 pair-arm. The already-ported tick interprets
 * the SED entry scripts; the translator keys voices. SFX are short
 * one-shots, so the observation window is tight. CONTROL
 * (XENO_SOUND_SFX_CONTROL=1): SED loaded, nothing fired.
 * XENO_SOUND_SFX_ENTRY overrides the effect index (decimal). */
static void PortRunSoundSfxProbe(void) {
    extern void func_80039F18(int packedId, int volume, int pan);
    extern void SoundAddSedsEntry(void* pSoundFile);
    extern int SpuGetKeyStatus(unsigned int voice_bit);
    extern int ArchiveDecodeAlignedSize(unsigned int entryIndex);
    extern int ArchiveSetIndex(int directoryIndex, int entryIndex);
    extern void ArchiveReadFileToBuffer(int fileIndex, void* pBuffer, int arg2,
                                        int arg3);
    extern void* HeapAlloc(int size, int flags);
    void* sedBuf;
    unsigned short sedId;
    int entry = 1;
    int entryCount;
    long kons0;
    int t, maxVoices = 0, samplesWithSound = 0;
    int ok_keyed, ok_played;

    if (getenv("XENO_SOUND_SFX_ENTRY")) {
        entry = atoi(getenv("XENO_SOUND_SFX_ENTRY"));
    }
    {
        /* The effect scripts bind WDS banks (opcode 0xFC); the common bank
         * must be resident first (engine truth -- same requirement the
         * field boot satisfies via func_80085FB8/F30). */
        extern void* SoundLoadWdsFile(void* pWdsFile, int mode);
        extern void HeapFree(void* pMemory);
        void* bankBuf;
        ArchiveSetIndex(0x1C, 0);
        bankBuf = HeapAlloc(ArchiveDecodeAlignedSize(3), 1);
        ArchiveReadFileToBuffer(3, bankBuf, 0, 0x80);
        SoundLoadWdsFile(bankBuf, 0);
        HeapFree(bankBuf);
    }
    ArchiveSetIndex(4, 0);
    sedBuf = HeapAlloc(ArchiveDecodeAlignedSize(0xA8), 1);
    ArchiveReadFileToBuffer(0xA8, sedBuf, 0, 0x80);
    SoundAddSedsEntry(sedBuf);   /* buffer stays resident (linked) */
    sedId = *(unsigned short*)((unsigned char*)sedBuf + 0x14);
    entryCount = (*(unsigned short*)((unsigned char*)sedBuf + 0x18) - 0x20) / 4;
    printf("[sfx-probe] common SED: id=0x%x entries=%d probing entry=%d "
           "(offs=0x%x,0x%x)\n", sedId, entryCount, entry,
           *(unsigned short*)((unsigned char*)sedBuf + 0x20 + entry * 4),
           *(unsigned short*)((unsigned char*)sedBuf + 0x22 + entry * 4));
    if (getenv("XENO_SOUND_SFX_CONTROL")) {
        printf("[sfx-probe] CONTROL: SED loaded, nothing fired\n");
        PortSleepMs(2000);
        printf("[sfx-probe] CONTROL keyons=%ld (want 0)\n",
               (long)s_regFlushKeyOns);
        return;
    }
    kons0 = s_regFlushKeyOns;
    func_80039F18((sedId << 16) | entry, 0x7F, 0x40);
    for (t = 0; t < 20; t++) {
        int v, playing = 0;
        PortSleepMs(100);
        for (v = 0; v < 24; v++) {
            playing += (SpuGetKeyStatus(1u << v) != 0);
        }
        if (playing > maxVoices) maxVoices = playing;
        if (playing > 0) samplesWithSound++;
    }
    ok_keyed = (s_regFlushKeyOns - kons0) > 0;
    ok_played = maxVoices > 0;
    printf("[sfx-probe] keyons=%ld maxVoices=%d samplesWithSound=%d/20\n",
           (long)(s_regFlushKeyOns - kons0), maxVoices, samplesWithSound);
    printf("[sfx-probe] RESULT: %s (keyed=%d AL-playing=%d; WAV RMS is the "
           "output-tier proof -- see runner)\n",
           (ok_keyed && ok_played) ? "PASS" : "FAIL", ok_keyed, ok_played);
}

#define WINDOW_TITLE  "Xenogears (PC port)"
#define SCREEN_WIDTH  640
#define SCREEN_HEIGHT 480

/* Decompiled game entry (src/slus_006.64/main/main_loop.c). */
extern void MainLoop(int errorCode);
/* Port-side runtime build of the game-state dispatch table (game_overrides.c). */
extern void PcPort_InitGameStates(void);
/* Port-side one-time HeapInit the asm boot would have done (game_overrides.c). */
extern void PcPort_HeapBoot(void);
/* Original boot global-state initializer called by func_80019578 before MainLoop. */
extern void func_8001AADC(void);
/* Field dialog-box interior darkening color (subtractive-blend TILE color), set in
 * .bss at retail boot by func_8001BB50 (also called from func_80019578). */
extern unsigned char D_800594D4;
extern unsigned char D_800594D5;
extern unsigned char D_800594D6;
/* "Published by Square" splash, decompressed + drawn from the migrated EXE data. */
extern void GameShowSplashScreen(void);

/* Input wiring. The game reads its BIOS controller buffer g_C1Buffer directly
 * (system/controller.c: ControllerGetButtonState reads [status,type,btn,btn] at
 * stride 0x22). On PSX the BIOS auto-fills it each vblank after InitPAD/StartPAD;
 * those are asm (bypassed boot) and PsyCross's InitPAD/PadRead are unimplemented.
 * PsyCross's PADRAW layout (status,id,buttons[2],analog[4]) matches the game's
 * buffer byte-for-byte, so we register g_C1Buffer's two pad slots with PsyX_Pad
 * and enable pad comms here. The per-frame refresh (PsyX_UpdateInput +
 * ControllerPoll) is driven from the Vsync shim in psyq_compat.c. */
extern unsigned char g_C1Buffer[];
extern void PsyX_Pad_InitPad(int slot, unsigned char* padData);
extern int g_padCommEnable;
#define PORT_CONTROLLER_BUFFER_SIZE 0x22

/* XENO_BUTTON_LAYOUT: "usa" (default; retail SLUS-00664 ControllerInit table)
 * or "jp" (identity remap table: Circle/Triangle are confirm/menu). Anything
 * else is a configuration error, reported instead of silently guessed. */
extern void ControllerInit(void);
static int PortButtonLayoutJp(void) {
    static int s_layout = -1;
    if (s_layout < 0) {
        const char* v = getenv("XENO_BUTTON_LAYOUT");
        if (v == NULL || *v == '\0' || strcmp(v, "usa") == 0) {
            s_layout = 0;
        } else if (strcmp(v, "jp") == 0) {
            s_layout = 1;
        } else {
            fprintf(stderr, "[xeno-port] XENO_BUTTON_LAYOUT=%s: expected usa or jp\n", v);
            exit(EXIT_FAILURE);
        }
        printf("[xeno-port] button layout: %s\n", s_layout ? "jp (identity)" : "usa (retail)");
    }
    return s_layout;
}

/* Disc / archive bring-up. The asm boot (func_80019578) calls
 * ArchiveInit(&D_80010004 [table buf], &D_80018004 [header buf], 0 [CD path])
 * after HeapInit; it CdInit()s and reads the archive index off the disc (sectors
 * 0x18/0x28). The port replaces the async CD path with synchronous PsyCross-libcd
 * reads (archive_port.c), so we just point PsyCross at the disc image and make the
 * same call. ArchiveReadFileToBuffer (used by the menu/field overlay loads) then
 * pulls real file data straight from disc1.bin. Gated on the image being present
 * so a disc-less run still boots to the menu as before. */
extern void PsyX_CDFS_Init(const char* imageFileName, int track, int sectorSize);
extern void ArchiveInit(unsigned int pArchiveTable, unsigned int pHeaderTable,
                        unsigned int pDebugTable);
extern int ArchiveSetIndex(int directoryIndex, int entryIndex);
extern int ArchiveDecodeSize(int fileIndex);
extern void ArchiveReadFileToBuffer(int fileIndex, void* pBuffer, int arg2, int arg3);
extern int ArchiveCdDataSync(int mode);
extern void* HeapAlloc(int size, int flags);
extern void HeapFree(void* pMemory);
extern void HeapSetCurrentContentType(int contentType);
extern void* LZSSHeapDecompress(void* pCompressed, int flags);
extern void SystemInitializeFont(void* pSystemFont);
extern void SystemInitializeData(void* pSystemData);
extern void func_8001ACA4(void);
extern unsigned short D_8006F954;    /* field entrance/spawn index (sister of D_8006F94E) */
extern unsigned short D_8006F950;    /* field transition approach angle (-> var 8 camera octant) */
extern unsigned char D_80010000[];  /* build/mode flag: -1 in retail ROM        */
extern unsigned char D_80010004[];  /* archive table buffer  (g_ArchiveTable)  */
extern unsigned char D_80018004[];  /* archive header buffer (g_ArchiveHeader) */
extern unsigned short D_8006F94E;    /* field map selected by FieldMain         */

/* ADSR phase 1 unit probe (env XENO_SOUND_ADSR_UNIT=1): drive the backend's
 * PRODUCTION envelope generator (PsyX_SPUAL_AdsrDebugCycle -- the exact code
 * the runtime uses, one 44100Hz cycle per call) over a rate/mode matrix and
 * print sampled (cycle, phase, level) trajectories. The golden oracle is an
 * independent Python transcription of the psx-spx pseudocode
 * (scratchpad/adsr_model.py); the two outputs must diff EMPTY (cycle-exact).
 * KON at cycle 0; KOFF at half the window. Pure computation -- runs before
 * any AL/game init and exits. */
extern void PsyX_SPUAL_AdsrDebugCycle(unsigned short adsr1, unsigned short adsr2,
                                      int* phase, int* level, int* counter);
static void PortRunSoundAdsrUnitDump(void) {
    /* Matrix: linear attack; exp attack (>0x6000 slowdown, shift<10 and
     * shift 10/11 legs); zero-ADSR instant paths; never-step attack (7Fh);
     * slow-shift counter paths; exp sustain-decrease; linear+exp release. */
    static const struct {
        unsigned short a1, a2;
        int kon, koff;
    } M[] = {
        { 0x2068, 0x5FCD, 60000, 60000 },   /* lin attack s8, decay s6 SL8, sustain hold, lin release s13 */
        { 0x9943, 0xCAAA, 60000, 60000 },   /* exp attack s6 st1, decay s4 SL3, exp sus-dec s10 st2, exp release s10 */
        { 0x0000, 0x0000, 2000, 2000 },     /* all-zero: instant attack/decay, sus-increase, instant release */
        { 0xFF43, 0x4FCA, 30000, 30000 },   /* exp attack s31 st3 never-step; lin sus-dec s15 st3; lin release s10 */
        { 0xA843, 0xDFEC, 400000, 200000 }, /* exp attack s10 (half/half leg), sus hold 1Fh/3, exp release s12 */
        { 0x5C64, 0x467F, 400000, 1 },      /* lin attack s23 (small-inc counter leg), release 1Fh never-step */
        { 0xB055, 0x5FC8, 20000, 20000 },   /* exp attack s12 (inc/4 leg above 6000h), decay s5 SL5, lin release s8 */
        { 0x7000, 0x5FC0, 100000, 100 },    /* lin attack s28 (inc==0 -> clamp-to-1 leg), lin release s0 */
    };
    int m;
    printf("[adsr-unit] begin (stride 97)\n");
    for (m = 0; m < (int)(sizeof(M) / sizeof(M[0])); m++) {
        int phase = 1, level = 0, counter = 0;  /* KON: attack, level 0 */
        long c;
        long total = M[m].kon + M[m].koff;
        printf("[adsr-unit] set a1=%04x a2=%04x kon=%d koff=%d\n",
               M[m].a1, M[m].a2, M[m].kon, M[m].koff);
        for (c = 1; c <= total; c++) {
            if (c == M[m].kon + 1 && phase != 0) {  /* KOFF edge */
                phase = 4;
                counter = 0;
            }
            PsyX_SPUAL_AdsrDebugCycle(M[m].a1, M[m].a2, &phase, &level, &counter);
            if (c % 97 == 0)
                printf("%ld %d %d\n", c, phase, level);
        }
    }
    printf("[adsr-unit] RESULT: DUMPED (diff vs adsr_model.py decides PASS)\n");
}

/* ADSR phase 1 capture probe (env XENO_SOUND_ADSR_PROBE=1|2): one note on a
 * constant-|amplitude| synthetic square (hand-built SPU-ADPCM: filter 0
 * shift 3, nibble +/-4 -> +/-2048, 4+4 block half-period = 197Hz, seamless
 * loop flags), with KNOWN ADSR params -- so the wave capture's RMS envelope
 * IS the ADSR curve times a constant. Mode 1: linear release s12 (186ms
 * ramp); mode 2: exp release s12 (τ=186ms). Attack lin s10 (53ms -- inside
 * the 5-50ms stepping-measurement band), decay s7 to SL7 (plateau
 * 0x4000/0x7FFF = -6.02dB). The envelope advances on the live 240Hz pump
 * (the real clock); envx sampled every 50ms for the state tier. */
static void PortRunSoundAdsrProbe(int mode) {
    extern unsigned int SpuSetTransferStartAddr(unsigned int addr);
    extern unsigned int SpuWrite(unsigned char* addr, unsigned int size);
    extern void PsyX_SPUAL_ShutdownSound(void);
    extern void PsyX_Sys_SoundGateEnterCritical(void);
    extern void PsyX_Sys_SoundGateExitCritical(void);
    static unsigned char sq[16 * 64];
    unsigned short a1 = 0x2877;                       /* lin attack s10, decay s7, SL7 */
    unsigned short a2 = (mode == 2) ? 0x5FEC : 0x5FCC; /* sus hold 1Fh/3; release s12 exp|lin */
    int b, i, t;
    long keyStat = -1;
    short envx = -1;
    /* 64 blocks: 8-block (224-sample) period, +2048 half then -2048 half. */
    for (b = 0; b < 64; b++) {
        unsigned char nib = ((b % 8) < 4) ? 0x4 : 0xC;
        sq[b * 16 + 0] = 0x03;                        /* shift 3, filter 0 */
        sq[b * 16 + 1] = (b == 0) ? 0x04 : (b == 63) ? 0x03 : 0x00; /* LoopStart / LoopEnd|Repeat */
        for (i = 2; i < 16; i++)
            sq[b * 16 + i] = (unsigned char)(nib | (nib << 4));
    }
    /* Gate-bracketed main-thread upload (see the ADPCM probe's note). */
    PsyX_Sys_SoundGateEnterCritical();
    SpuSetTransferStartAddr(0x2000);
    SpuWrite(sq, sizeof(sq));
    PsyX_Sys_SoundGateExitCritical();
    {
        SpuVoiceAttr attr;
        memset(&attr, 0, sizeof(attr));
        attr.voice = 1u << 0;
        attr.mask = SPU_VOICE_VOLL | SPU_VOICE_VOLR | SPU_VOICE_PITCH |
                    SPU_VOICE_WDSA | SPU_VOICE_LSAX | SPU_VOICE_ADSR_ADSR1 |
                    SPU_VOICE_ADSR_ADSR2;
        attr.volume.left = 0x3000;
        attr.volume.right = 0x3000;
        attr.pitch = 0x1000;
        attr.addr = 0x2000;
        attr.loop_addr = 0x2000;
        attr.adsr1 = a1;
        attr.adsr2 = a2;
        SpuSetVoiceAttr(&attr);
    }
    printf("[adsr-probe] mode=%d a1=%04x a2=%04x KON\n", mode, a1, a2);
    SpuSetKey(SPU_ON, 1u << 0);
    for (t = 0; t < 24; t++) {                        /* 1200ms keyed */
        PortSleepMs(50);
        SpuGetVoiceEnvelopeAttr(0, &keyStat, &envx);
        printf("[adsr-probe] t=%dms keyStat=%ld envx=%d\n", (t + 1) * 50, keyStat, envx);
    }
    printf("[adsr-probe] KOFF\n");
    SpuSetKey(SPU_OFF, 1u << 0);
    for (t = 0; t < 20 && envx != 0; t++) {           /* release tail, 1000ms cap */
        PortSleepMs(50);
        SpuGetVoiceEnvelopeAttr(0, &keyStat, &envx);
        printf("[adsr-probe] t=+%dms keyStat=%ld envx=%d srcPlaying=%d\n",
               (t + 1) * 50, keyStat, envx, SpuGetKeyStatus(1u << 0));
    }
    PortSleepMs(200);
    printf("[adsr-probe] RESULT: %s (envx rose, plateaued, decayed to 0; curve-match = python)\n",
           (envx == 0 && SpuGetKeyStatus(1u << 0) == 0) ? "PASS" : "FAIL");
    PsyX_SPUAL_ShutdownSound();                       /* finalize the wave capture */
}

/* ADPCM/Gaussian unit probe (env XENO_SOUND_ADPCM_UNIT=1): drive the
 * PRODUCTION integer ADPCM decoder and Gaussian interpolator over crafted
 * vectors (all 5 filters, shifts incl. the 13-15 -> 9 clamp, extreme
 * nibbles for clamp16) and print PCM/interp lines; the golden oracle is the
 * independent Python model (scratchpad/adpcm_gauss_model.py) -- diff EMPTY.
 * Pure computation; exits before game boot. */
extern void PsyX_SPUAL_AdpcmDecodeDebug(const unsigned char* data, int nBlocks,
                                        short* outPcm, short* p1io, short* p2io);
extern int PsyX_SPUAL_GaussDebug(const short* hist4, int idx);
extern int PsyX_SPUAL_IsStreaming(void);
static void PortRunSoundAdpcmUnitDump(void) {
    static unsigned char blocks[16 * 16];
    static short pcm[16 * 28];
    short p1 = 0, p2 = 0;
    int b, i;
    /* 16 blocks: filters 0-4 (+5-7 clamp), shifts 0..15 (incl. 13-15),
     * nibble patterns exercising sign extremes and history feedback. */
    for (b = 0; b < 16; b++) {
        unsigned char* blk = &blocks[b * 16];
        blk[0] = (unsigned char)(((b % 8) << 4) | (b & 0x0F));
        blk[1] = 0;
        for (i = 2; i < 16; i++)
            blk[i] = (unsigned char)(0x87 + i * 7 + b * 13);
    }
    printf("[adpcm-unit] begin blocks=16\n");
    PsyX_SPUAL_AdpcmDecodeDebug(blocks, 16, pcm, &p1, &p2);
    for (b = 0; b < 16; b++) {
        printf("B%02d:", b);
        for (i = 0; i < 28; i++)
            printf(" %d", pcm[b * 28 + i]);
        printf("\n");
    }
    printf("HIST %d %d\n", p1, p2);
    {
        static const short hists[3][4] = {
            { -32768, 32767, -32768, 32767 },
            { 1000, -2000, 3000, -4000 },
            { 2048, 2048, 2048, 2048 },
        };
        int h, idx;
        for (h = 0; h < 3; h++) {
            printf("G%d:", h);
            for (idx = 0; idx < 256; idx += 5)
                printf(" %d", PsyX_SPUAL_GaussDebug(hists[h], idx));
            printf("\n");
        }
    }
    printf("[adpcm-unit] RESULT: DUMPED (diff vs adpcm_gauss_model.py decides PASS)\n");
}

/* ADPCM/Gaussian capture probe (env XENO_SOUND_ADPCM_PROBE=1): one long
 * note on the constant-|amplitude| synthetic square (same fixture as the
 * ADSR probe) at a NON-1:1 pitch (XENO_SOUND_ADPCM_PITCH, default 0x1200)
 * so the resampler interpolates every sample; near-instant full-level ADSR
 * (a1=0x000F: shift0 attack, SL15) for a clean sustain window.  Run under
 * ALSOFT wave capture; the FFT's alias-line structure is the proof
 * (Gaussian vs the legacy cubic A/B, vs the Python-rendered reference). */
static void PortRunSoundAdpcmProbe(void) {
    extern unsigned int SpuSetTransferStartAddr(unsigned int addr);
    extern unsigned int SpuWrite(unsigned char* addr, unsigned int size);
    extern void PsyX_SPUAL_ShutdownSound(void);
    extern void PsyX_Sys_SoundGateEnterCritical(void);
    extern void PsyX_Sys_SoundGateExitCritical(void);
    static unsigned char sq[16 * 64];
    unsigned short a1 = 0x000F;
    unsigned short a2 = 0x5FC0;   /* sustain hold; linear release shift 0 */
    unsigned int pitch = 0x1200;
    const char* pEnv = getenv("XENO_SOUND_ADPCM_PITCH");
    int b, i, t;
    long keyStat = -1;
    short envx = -1;
    if (pEnv && pEnv[0])
        pitch = (unsigned int)strtoul(pEnv, NULL, 0);
    for (b = 0; b < 64; b++) {
        unsigned char nib = ((b % 8) < 4) ? 0x4 : 0xC;
        sq[b * 16 + 0] = 0x03;
        sq[b * 16 + 1] = (b == 0) ? 0x04 : (b == 63) ? 0x03 : 0x00;
        for (i = 2; i < 16; i++)
            sq[b * 16 + i] = (unsigned char)(nib | (nib << 4));
    }
    /* Main-thread upload: bracket with the sound gate so the transfer
     * callback's flag writes serialize against the 240Hz tick (the game's
     * own transfers drain ON the tick thread; only probes write from
     * main -- TSan-clean by the same gate the queue path uses). */
    PsyX_Sys_SoundGateEnterCritical();
    SpuSetTransferStartAddr(0x2000);
    SpuWrite(sq, sizeof(sq));
    PsyX_Sys_SoundGateExitCritical();
    {
        SpuVoiceAttr attr;
        memset(&attr, 0, sizeof(attr));
        attr.voice = 1u << 0;
        attr.mask = SPU_VOICE_VOLL | SPU_VOICE_VOLR | SPU_VOICE_PITCH |
                    SPU_VOICE_WDSA | SPU_VOICE_LSAX | SPU_VOICE_ADSR_ADSR1 |
                    SPU_VOICE_ADSR_ADSR2;
        attr.volume.left = 0x3000;
        attr.volume.right = 0x3000;
        attr.pitch = (unsigned short)pitch;
        attr.addr = 0x2000;
        attr.loop_addr = 0x2000;
        attr.adsr1 = a1;
        attr.adsr2 = a2;
        SpuSetVoiceAttr(&attr);
    }
    printf("[adpcm-probe] streaming=%d pitch=0x%x KON\n",
           PsyX_SPUAL_IsStreaming(), pitch);
    SpuSetKey(SPU_ON, 1u << 0);
    for (t = 0; t < 28; t++) {                        /* 1400ms sustain */
        PortSleepMs(50);
        if ((t % 7) == 6) {
            SpuGetVoiceEnvelopeAttr(0, &keyStat, &envx);
            printf("[adpcm-probe] t=%dms keyStat=%ld envx=%d\n",
                   (t + 1) * 50, keyStat, envx);
        }
    }
    printf("[adpcm-probe] KOFF\n");
    SpuSetKey(SPU_OFF, 1u << 0);
    PortSleepMs(250);
    SpuGetVoiceEnvelopeAttr(0, &keyStat, &envx);
    printf("[adpcm-probe] RESULT: %s (streaming=%d envx=%d)\n",
           (PsyX_SPUAL_IsStreaming() && envx == 0) ? "PASS" : "FAIL",
           PsyX_SPUAL_IsStreaming(), envx);
    PsyX_SPUAL_ShutdownSound();
}

/* MODE2/2352 image; PsyCross extracts the 2048-byte data payload per sector. */
#define PORT_CD_SECTOR_SIZE 2352

/* Disc image path from the disc/storage layer (xg_plat/disc.h): XENO_DISC,
 * else disc/disc1.bin relative to the working directory, ../ or ../../. */
static const char* PcPort_FindDiscImage(void) {
    return xg_plat_disc_image_path();
}

static void PcPort_LoadSystemTextData(void) {
    void* pCompressed;
    void* pDecoded;

    ArchiveSetIndex(0, 1);

    pCompressed = HeapAlloc(ArchiveDecodeSize(6), 0);
    ArchiveReadFileToBuffer(6, pCompressed, 0, 0);
    ArchiveCdDataSync(0);
    HeapSetCurrentContentType(0x30);
    pDecoded = LZSSHeapDecompress(pCompressed, 1);
    SystemInitializeFont(pDecoded);
    HeapFree(pCompressed);

    pCompressed = HeapAlloc(ArchiveDecodeSize(7), 0);
    ArchiveReadFileToBuffer(7, pCompressed, 0, 0);
    ArchiveCdDataSync(0);
    HeapSetCurrentContentType(0x31);
    pDecoded = LZSSHeapDecompress(pCompressed, 1);
    SystemInitializeData(pDecoded);
    HeapFree(pCompressed);
}

int main(int argc, char** argv) {
    /* --config FILE / --set key=value / --help (xg_plat/config.h). */
    switch (xg_plat_config_cli(argc, argv)) {
    case 1: return EXIT_SUCCESS;
    case -1: return 64; /* EX_USAGE */
    default: break;
    }

    printf("[xeno-port] booting (Silent-Hill-style: PSX RAM emu + runtime dispatch table)\n");
    if (xg_plat_config_load() < 0)
        return 78; /* EX_CONFIG: explicit --config / XENO_CONFIG unreadable */
    /* The data_*.c constructors already filled the native retail tables from
     * the user's disc files (retail_data.h); report what they loaded. */
    printf("[xeno-port] retail data: %u objects, %u bytes loaded from the user's "
           "files (SLUS_006.64: %s)\n", g_XenoRetailRangesLoaded, g_XenoRetailBytesLoaded,
           g_XenoRetailFiles[XENO_RD_SLUS].state > 0 ? g_XenoRetailFiles[XENO_RD_SLUS].path
                                                    : "not loaded");

    /* 1. PSX main-RAM emulation must come first (PSX_ADDR targets live here). */
    PsxMemory_Init();
    /* W34C2: main-exe rodata/sdata into guest RAM (PSX_ADDR consumers). */
    (void)PsxMemory_LoadStaticData();

    /* Parse the optional deterministic input schedule before game startup. */
    if (PcPort_TestInputInit() != 0)
        return EXIT_FAILURE;
    if (PcPort_WorldTestInputInit() != 0)
        return EXIT_FAILURE;

    /* 2. Data migration: build the game-state dispatch table at runtime. */
    PcPort_InitGameStates();

    /* SLUS_006.64 initializes g_VideoMode (80058990) to NTSC (0).
     * Publish it before PsyX_Initialise starts the interrupt thread; the
     * backend's unset mode otherwise selects the 50-Hz PAL clock. */
    SetVideoMode(0);
    {
        /* Output through the renderer interface (xg_plat/renderer.h). */
        XgPlatVideoConfig video;
        xg_plat_renderer_default_config(&video);
        video.window_width = SCREEN_WIDTH;
        video.window_height = SCREEN_HEIGHT;
        xg_plat_config_video(&video); /* config.ini / XENO_VIDEO_* / --set */
        (void)xg_plat_renderer_init(WINDOW_TITLE, &video);
    }

    /* XENO_PC_PORT keyboard map. Players expect Z to confirm/talk and V to open
     * the menu. Which PAD button that is depends on the layout ControllerInit
     * installs (see PortButtonLayoutJp): the USA layout remaps physical
     * Cross -> the game's internal confirm bit and Square -> internal menu
     * bit, the Japanese identity layout uses Circle and Triangle. Bind Z/V to
     * whichever physical buttons carry confirm/menu, and put the other two
     * face buttons on the keys they vacate (C, X). Raw SDL scancodes (Z=29,
     * V=25, C=6, X=27) avoid pulling an SDL header in here; kc_* are plain
     * ints. Arrows (d-pad), Enter (Start), Space (Select) are unchanged. */
    if (PortButtonLayoutJp()) {
        g_cfg_keyboardMapping.kc_circle   = 29; /* Z: confirm (JP layout) */
        g_cfg_keyboardMapping.kc_triangle = 25; /* V: menu */
        g_cfg_keyboardMapping.kc_cross    = 6;  /* C */
        g_cfg_keyboardMapping.kc_square   = 27; /* X */
    } else {
        g_cfg_keyboardMapping.kc_cross    = 29; /* Z: confirm (USA layout) */
        g_cfg_keyboardMapping.kc_square   = 25; /* V: menu */
        g_cfg_keyboardMapping.kc_circle   = 6;  /* C */
        g_cfg_keyboardMapping.kc_triangle = 27; /* X */
    }
    /* Config file / XENO_* overrides: game.speed, input.* bindings, ... */
    xg_plat_config_apply_runtime();
    /* Cheat console first: its frame_tick subscribers (field warp, battle
     * warp, queued commands) must run before any mod hook, in the order the
     * VSync shim used to call them. */
    PcPort_CheatConsoleInit();
    /* Mods: scan mods/ (manifests, plugins, asset replacements). */
    (void)xg_plat_mods_init();
    {   /* game-side event sources (room_enter, ...) */
        extern void PcPort_ModEventsInit(void);
        PcPort_ModEventsInit();
    }

    /* 4. PsyQ subsystem init normally done by the asm `start` before MainLoop. */
    ResetCallback();
    ResetGraph(0);

    /* 4a-sound (Phase 0, sound SDK): wake the OpenAL SPU backend. Retail's sound
     * cold-init reaches SpuInit() -> PsyX_SPUAL_InitSound(), which sets
     * g_spuInit=1; until then every PsyX_SPUAL_* call early-returns and the SPU
     * is dormant. Must follow PsyX_Initialise (the OpenAL context). Idempotent
     * (PsyX_SPUAL_InitSound guards on g_spuInit/g_SpuMutex). This is ONLY the
     * backend wake; the game-side SoundInitialize(0) routing is a later phase
     * and is intentionally NOT done here. */
    SpuInit();

    /* ADSR phase 1 unit probe: dump production-generator trajectories for the
     * golden diff vs the independent Python spec model, then exit (pure
     * computation; no game boot). Diagnostic only. */
    if (getenv("XENO_SOUND_ADSR_UNIT")) {
        PortRunSoundAdsrUnitDump();
        exit(0);
    }

    /* ADPCM/Gaussian unit probe: production decode + interpolator vs the
     * independent Python model, then exit. Diagnostic only. */
    if (getenv("XENO_SOUND_ADPCM_UNIT")) {
        PortRunSoundAdpcmUnitDump();
        exit(0);
    }

    /* 4a-probe: Phase-1 sound-pump synthetic validation (env XENO_SOUND_PUMP_PROBE).
     * The PsyX interrupt thread is already running (started by PsyX_Initialise),
     * so the pump is live here. Diagnostic only; does not run in normal boot. */
    if (getenv("XENO_SOUND_PUMP_PROBE")) {
        PortRunSoundPumpProbe();
    }

    /* 4a-probe2: Phase-2 sound-SDK primitive validation (env XENO_SOUND_PRIM_PROBE).
     * Exercises the wired init-reached primitives + reverb round-trip against the
     * awake backend. Diagnostic only; does not run in normal boot. */
    if (getenv("XENO_SOUND_PRIM_PROBE")) {
        PortRunSoundPrimProbe();
    }

    /* 4c: route the game's own SoundInitialize(0) into boot -- one duty of
     * retail func_80019578 that port_main must reproduce. This allocates the
     * audio manager, initializes its sound heap, registers the translated
     * retail 240Hz func_8003C020 tick on RCnt2, and configures reverb/mixer.
     * The disc-backed boot WDS banks are loaded below, after ArchiveInit, at
     * the corresponding point in retail func_80019578. */
    SoundInitialize(0);

    /* B5.1: register the register->backend translator on the pump (slot after
     * the tick's; dispatched at 240Hz under the same gate bracket). Always-on
     * port wiring -- this is what makes key-ons audible. */
    {
        int hFlush = OpenEvent(PORT_RCntCNT2, PORT_EvSpINT, PORT_EvMdINTR,
                               PcPort_SpuRegFlushTick);
        EnableEvent(hFlush);
    }

    /* ADSR phase 1 capture probe: one known-ADSR note on a constant-amplitude
     * synthetic loop, envelope advanced by the live 240Hz pump; run under
     * ALSOFT wave capture for the curve-match proof, then exit. Diagnostic
     * only. */
    if (getenv("XENO_SOUND_ADSR_PROBE")) {
        PortRunSoundAdsrProbe(atoi(getenv("XENO_SOUND_ADSR_PROBE")));
        exit(0);
    }

    /* ADPCM/Gaussian capture probe: one pitched note through the streaming
     * path under wave capture (FFT alias-line proof), then exit. */
    if (getenv("XENO_SOUND_ADPCM_PROBE")) {
        PortRunSoundAdpcmProbe();
        exit(0);
    }

    if (getenv("XENO_SOUND_INIT_PROBE")) {
        PortRunSoundInitProbe();
    }

    /* 4d-probe (tick-leg step 1, gate-first): concurrency validation of the
     * sound tick gate (g_SoundTickMutex; psycross_sound_gate.patch). Runs with
     * translated retail func_8003C020 registered and enabled under the gate.
     * Diagnostic only; does not run in normal boot. */
    if (getenv("XENO_SOUND_GATE_STRESS")) {
        PortRunSoundGateStress();
    }

    /* 4e-probe (tick-leg step 3): synthetic sequence-command stream through
     * the live dispatch table. Diagnostic only; does not run in normal boot. */
    if (getenv("XENO_SOUND_SEQ_PROBE")) {
        PortRunSoundSeqProbe();
    }

    /* 4b. Retail boot 80019600: ControllerInit() -- wires the game's BIOS pad
     * buffer (see note above) and installs the USA button remap table
     * (src/slus_006.64/system/controller.c). XENO_BUTTON_LAYOUT=jp restores
     * the .sdata identity table afterwards (the pre-2026-09-23 port layout). */
    ControllerInit();
    if (PortButtonLayoutJp()) {
        extern u_char g_ControllerButtonMappings[8];
        int i;
        for (i = 0; i < 8; i++) g_ControllerButtonMappings[i] = (u_char)i;
    }
    /* Retail boot 80019618-80019620 registers the game vblank body after
     * controller setup. Native dispatch stays on the game thread. */
    {
        extern void func_8003634C(void);
        extern void PcPort_ResetVblankService(void);
        extern void func_8004B7D0(void (*callback)(void));
        PcPort_ResetVblankService();
        func_8004B7D0(func_8003634C);
    }

    /* 5. One-time HeapInit the asm boot (func_80019578) runs before MainLoop;
     * MainLoop only HeapRelocate()s and would crash on an uninitialised heap. */
    PcPort_HeapBoot();

    /* 5a. Original boot state reset normally performed by func_80019578 before
     * entering MainLoop. The native oracle bypasses that raw asm entry point. */
    func_8001AADC();

    /* 5a-bis. func_80019578 also calls func_8001BB50, which writes the field
     * dialog-box interior darkening color into .bss. That color is applied to a
     * semi-transparent flat TILE drawn under the text with a *subtractive*
     * blend (background.drawModes use GetTPage abr=2), so the box interior =
     * framebuffer - (D4,D5,D6). The native boot oracle bypasses func_80019578,
     * so its now-ported func_8001BB50 call still does not run on this path.
     * Restore the exact retail values (func_8001BB50 asm 8001BB74-8001BB90:
     * D4=0x88, D5=0x76, D6=0x54). Consumed once by FieldTextBoxInitializePrimitives
     * (setRGB0 on background.tiles) at field enter, so set before MainLoop. */
    D_800594D4 = 0x88;
    D_800594D5 = 0x76;
    D_800594D6 = 0x54;

    /* 5b. Disc / archive init (see notes above). Only when the image is found,
     * so a disc-less run still reaches the menu instead of hanging in
     * ArchiveInit's `while (CdInit() == 0)`. */
    {
        const char* disc = PcPort_FindDiscImage();
        /* D_80010000 is static read-only ROM data (asm/.../data/800.rodata.s) with
         * value 0xFFFFFFFF. The exact retail bytes are now loaded from the user's
         * disc/SLUS_006.64 into pc_port/src/data_slus_rodata.c, so the old ad-hoc
         * `*(int*)D_80010000 = -1;` write is gone -- the correction is in the data,
         * not in a boot patch. A zero stub made FieldMain compute
         * g_FieldSystemMode = SYSTEM_MODE_PC_HDD (0) and hit a `break 1` trap meant
         * only for the PC-HDD dev path; the real constant picks SYSTEM_MODE_CD_ROM
         * (1). ArchiveInit treats the value as pDebugTable: -1, like 0, selects the
         * CD path (g_ArchiveDebugTable = NULL), so disc loading is unchanged. */
        if (disc && xg_plat_disc_verify() != 0)
            exit(78); /* EX_CONFIG: wrong disc image, explained on stderr */
        if (disc) {
            printf("[xeno-port] CD image: %s\n", disc);
            PsyX_CDFS_Init(disc, 0, PORT_CD_SECTOR_SIZE);
            /* pDebugTable MUST be 0 here, not D_80010000's -1. Retail passes -1, but
             * ArchiveInit only reads the archive table/header from CD when
             * pDebugTable == 0 (`if (!pDebugTable)`); with -1 it relies on the table
             * statically baked into the EXE at D_80010004/D_80018004. Those retail
             * bytes are now baked (pc_port/src/data_slus_rodata.c), but the port still
             * takes the disc path deliberately, so the live CD image -- not the
             * main-exe snapshot -- drives the archive index. g_ArchiveDebugTable still
             * ends up NULL. */
            ArchiveInit((unsigned int)D_80010004, (unsigned int)D_80018004, 0);
            printf("[xeno-port] ArchiveInit done (archive index loaded from disc).\n");

            /* Retail func_80019578 loads archive 0/1 WDS files 2..5 here,
             * waits for their SPU transfers, stores the second/fourth handles,
             * then frees only the source buffers. */
            PcPort_LoadRetailBootSoundBanks();
            printf("[xeno-port][sound] retail boot WDS banks 2..5 loaded\n");

            /* B5.1 WDS-load probe (env XENO_SOUND_WDS_PROBE=1): replicate the
             * retail loader pair (func_80085FB8 + func_80085F30 core): read the
             * REAL WDS sample bank (archive dir 0x1C, file 3) through the
             * working archive path, SoundLoadWdsFile it, and PROVE the samples
             * landed in the backend SPU-RAM image by SpuRead-back comparison.
             * Samples LOAD only -- key-on translation (audible) is B5.1 pass 2. */
            if (getenv("XENO_SOUND_WDS_PROBE")) {
                extern int ArchiveDecodeAlignedSize(unsigned int entryIndex);
                extern unsigned int SpuSetTransferStartAddr(unsigned int addr);
                extern unsigned int SpuRead(unsigned char* addr, unsigned int size);
                extern int SpuIsTransferCompleted(int flag);
                extern unsigned short g_SoundTransferQueueReadIndex;
                extern unsigned short g_SoundTransferQueueWriteIndex;
                extern void* SoundLoadWdsFile(void* pWdsFile, int mode);
                void* buf;
                unsigned char* entry;
                int size;
                ArchiveSetIndex(0x1C, 0x0);
                size = ArchiveDecodeAlignedSize(3);
                buf = HeapAlloc(size, 1);
                ArchiveReadFileToBuffer(3, buf, 0, 0x80);
                {
                    unsigned int dataOff = *(unsigned int*)((unsigned char*)buf + 0x18);
                    unsigned int dataSize = *(unsigned int*)((unsigned char*)buf + 0x14);
                    unsigned char first[16];
                    unsigned char rb[16];
                    unsigned int probeOff = 0;
                    int ok_entry, ok_spuaddr, ok_list, ok_queue, ok_bytes;
                    /* ADPCM banks open with silent blocks; verify at the first
                     * nonzero 16-byte window instead of offset 0. */
                    while (probeOff + 16 < dataSize &&
                           *(unsigned int*)((unsigned char*)buf + dataOff + probeOff) == 0) {
                        probeOff += 16;
                    }
                    memcpy(first, (unsigned char*)buf + dataOff + probeOff, 16);
                    entry = (unsigned char*)SoundLoadWdsFile(buf, 0);
                    ok_entry = (entry != NULL);
                    ok_spuaddr = ok_entry && (*(int*)(entry + 0x28) != 0);
                    {
                        extern void* g_SoundWdsLinkedList;
                        ok_list = (g_SoundWdsLinkedList == (void*)entry);
                    }
                    {
                        /* The transfer queue drains on the 240Hz tick thread;
                         * poll (TSan slows the pump ~15x -- a fixed-order
                         * readback races the drain and trips the backend's
                         * bounds assert on a not-yet-written address). */
                        int w = 0;
                        while (g_SoundTransferQueueReadIndex !=
                                   g_SoundTransferQueueWriteIndex && w < 500) {
                            PortSleepMs(10);
                            w++;
                        }
                    }
                    ok_queue = (g_SoundTransferQueueReadIndex == g_SoundTransferQueueWriteIndex);
                    memset(rb, 0, 16);
                    if (ok_spuaddr && ok_queue &&
                        (unsigned int)*(int*)(entry + 0x28) + probeOff + 16 < 0x80000) {
                        SpuSetTransferStartAddr(*(int*)(entry + 0x28) + probeOff);
                        SpuRead(rb, 16);
                    }
                    ok_bytes = (memcmp(rb, first, 16) == 0) &&
                               !(first[0]==0 && first[1]==0 && first[2]==0 && first[3]==0 &&
                                 first[4]==0 && first[5]==0 && first[6]==0 && first[7]==0);
                    printf("[wds-probe] file: size=%d dataOff=0x%x dataSize=0x%x\n",
                           size, dataOff, dataSize);
                    printf("[wds-probe] entry=%d spuAddr=0x%x list=%d queueDrained=%d\n",
                           ok_entry, ok_entry ? *(int*)(entry + 0x28) : 0, ok_list, ok_queue);
                    printf("[wds-probe] SPU-RAM readback vs source @+0x%x: match=%d (src %02x%02x%02x%02x rb %02x%02x%02x%02x)\n",
                           probeOff, ok_bytes, first[0], first[1], first[2], first[3],
                           rb[0], rb[1], rb[2], rb[3]);
                    printf("[wds-probe] RESULT: %s (samples LOADED to SPU-RAM; audibility = play probe)\n",
                           (ok_entry && ok_spuaddr && ok_list && ok_queue && ok_bytes) ? "PASS" : "FAIL");
                }
                HeapFree(buf);
                ArchiveSetIndex(4, 0);
            }

            /* B5.1 pass 2: first-audible probe (needs the WDS bank loaded --
             * run with XENO_SOUND_WDS_PROBE=1 too, and skip its HeapFree side
             * effects by design: the loader copied the header to the sound
             * heap and the samples to SPU-RAM; the file buffer is free). */
            if (getenv("XENO_SOUND_PLAY_PROBE")) {
                PortRunSoundPlayProbe();
            }

            /* Song-start M2 scan (env XENO_SOUND_SONG_SCAN=1): enumerate dir
             * 0x1C candidates -- header bytes decide which files are songs
             * (elementCount at +0x14, wdsId at +0x16) vs WDS banks -- and
             * print the loaded bank's id so the song/bank pairing is chosen
             * from measured data, not the scoping map alone. Read-only. */
            if (getenv("XENO_SOUND_SONG_SCAN")) {
                extern int ArchiveDecodeAlignedSize(unsigned int entryIndex);
                int fi;
                ArchiveSetIndex(0x1C, 0);
                for (fi = 0x0; fi <= 0x24; fi++) {
                    int sz = ArchiveDecodeAlignedSize(fi);
                    unsigned char hdr[0x40];
                    void* b;
                    if (sz <= 0 || sz > 0x300000) {
                        printf("[song-scan] file %02x: size=%d (skip)\n", fi, sz);
                        continue;
                    }
                    b = HeapAlloc(sz, 1);
                    if (b == NULL) { printf("[song-scan] file %02x: alloc fail (%d)\n", fi, sz); continue; }
                    ArchiveReadFileToBuffer(fi, b, 0, 0x80);
                    memcpy(hdr, b, 0x40);
                    printf("[song-scan] file %02x: size=%-7d magic=%02x%02x%02x%02x "
                           "id10=%04x elemCnt=%02x extra=%02x wdsId=%04x off18=%04x "
                           "tempo=%02x%02x%02x%02x\n",
                           fi, sz, hdr[0], hdr[1], hdr[2], hdr[3],
                           *(unsigned short*)(hdr + 0x10), hdr[0x14], hdr[0x15],
                           *(unsigned short*)(hdr + 0x16), *(unsigned short*)(hdr + 0x18),
                           hdr[0x1A], hdr[0x1B], hdr[0x1C], hdr[0x1D]);
                    HeapFree(b);
                }
                ArchiveSetIndex(4, 0);
            }

            /* Song-start M2: a real sequence through the real arming chain. */
            if (getenv("XENO_SOUND_SONG_PROBE")) {
                PortRunSoundSongProbe();
            }

            /* S1: a real sound effect through the real SFX chain. */
            if (getenv("XENO_SOUND_SFX_PROBE")) {
                PortRunSoundSfxProbe();
            }

            /* The KernelMenu "Field" option jumps straight into the field without
             * the new-game / worldmap setup that normally (a) fills
             * g_GameState.partyMembers and (b) selects the party-skin archive
             * directory (#4, as the skin loader func_8001ACA4 does) before field
             * entry. Without (b), GamePartyCharactersInitializeSkins resolves
             * ArchiveDecodeAlignedSize against the wrong directory -> bogus ~6MB ->
             * HeapAlloc fail. Setting the real directory here lets the field's
             * party-skin init proceed. This is a stand-in for the not-yet-ported
             * new-game init, not a permanent solution. The LoadGameStateOverlay
             * save/restore preserves this index through the field overlay load.
             *
             * These initializations are needed for BOTH the XENO_FIELD_TEST
             * harness AND the normal KernelMenu path, so they run unconditionally
             * once the archive is available. */
            PcPort_LoadSystemTextData();
            /* Retail boot (func_80019578 0x80019870): func_8001BB50 loads the
             * new-game template (archive 0x10 file 3, 0x2358 bytes) over
             * g_GameState.  The template extends past the 0x2300 GameState
             * into the field transition tuple D_8006F94E/50/52/54 (aliased into
             * the same blob by data_game_state.c), so this is what selects
             * the first field map after the opening movie: the disc-1
             * template's +0x231A is 0x01EA, map 490 (the title field).  The
             * harness lane keeps its own map/entrance selection below. */
            if (!(getenv("XENO_FIELD_TEST") && getenv("XENO_FIELD_TEST")[0] == '1')) {
                extern void func_8001BB50(void);
                PcPort_CommitPendingBootSoundCommonAttr();
                func_8001BB50();
                printf("[xeno-port][boot] func_8001BB50: new-game template loaded "
                       "(map=%u ent=%u cam=%u)\n",
                       (unsigned int)D_8006F94E, (unsigned int)D_8006F954,
                       (unsigned int)D_8006F950);
            }
            /* Harness lane only (XENO_FIELD_TEST=1).  The retail boot never
             * runs these: func_8001ACA4 has no caller in the retail EXE, and
             * its five pinned skin buffers at the top of the heap would push
             * the free region below 0x801D3008, where MovieMain places the
             * movie player module. */
            if (getenv("XENO_FIELD_TEST") && getenv("XENO_FIELD_TEST")[0] == '1') {
                extern unsigned char g_GameState[];
                /* Retail roster stand-in; remove when new-game setup is ported. */
                g_GameState[0x1D34] = 0x00; /* Fei */
                g_GameState[0x1D35] = 0x02; /* character ID 2 */
                g_GameState[0x1D36] = 0xFF; /* empty */
                func_8001ACA4();
            }
            {
                const char* fieldMap = getenv("XENO_FIELD_MAP");
                if (fieldMap != NULL && fieldMap[0] != '\0') {
                    D_8006F94E = (unsigned short)strtoul(fieldMap, NULL, 0);
                    printf("[xeno-port][field] XENO_FIELD_MAP=%u\n",
                           (unsigned int)D_8006F94E);
                }
                /* Field entrance/spawn index. D_8006F954 is the real field
                 * entrance/spawn transition input, the sister global of the map
                 * selector D_8006F94E (set just above the same way). The value
                 * propagates: FieldMain copies D_8006F954 -> g_GameState+0x1932
                 * (main.c:408); FieldLoad copies g_GameState+0x1930 ->
                 * g_FieldScriptMemory (misc3.c:390-396); the field-load script
                 * (func_800A08B8 -> func_8009FA54) reads field-script variable 2
                 * at g_FieldScriptMemory+2 to pick a spawn-table entry. Direct
                 * XENO_FIELD_MAP entry skips the transition, leaving var 2 = 0 ->
                 * entrance 0, which on some maps is an edge spawn outside the
                 * walkmesh (camera can't frame the player). Writing D_8006F954
                 * here -- the top of that copy chain, not an intermediate buffer
                 * that gets overwritten -- stands in for the missing transition.
                 * Coordinates still come from the game's own spawn table; only
                 * the index is selected. */
                const char* fieldEntrance = getenv("XENO_FIELD_ENTRANCE");
                if (fieldEntrance != NULL && fieldEntrance[0] != '\0') {
                    char* end = NULL;
                    long entrance = strtol(fieldEntrance, &end, 0);
                    if (end != fieldEntrance && entrance >= 0 &&
                        entrance <= 0xFFFF) {
                        D_8006F954 = (unsigned short)entrance;
                        printf("[xeno-port][field] XENO_FIELD_ENTRANCE=%ld "
                               "(D_8006F954 -> field-script var 2)\n", entrance);
                    } else {
                        printf("[xeno-port][field] ignoring invalid "
                               "XENO_FIELD_ENTRANCE=%s\n", fieldEntrance);
                    }
                    /* Camera approach direction. D_8006F950 is the retail
                     * transition's approach-angle input: FieldMain stores it
                     * as an octant (>>9) to g_GameState+0x1938 (main.c:409),
                     * which FieldLoad copies to field-script var 8; spawn
                     * entries whose camera rotByte is 0xFF (= "inherit from
                     * the transition") read var 8 in func_8009FA54. A cold
                     * harness boot leaves it 0, which degenerates to an
                     * axis-aligned camera yaw (0x800) that the d-pad->walk
                     * LUT is not authored for (retail field cameras are
                     * diagonal; Up walks away from the camera only there).
                     * Default octant 7 = scene yaw 0x600, empirically
                     * validated: Up walks exactly away from the camera, and
                     * the Map1 ent6 spawn is unoccluded from that side (the
                     * other consistent diagonal, octant 3, puts the camera
                     * behind the spawn-adjacent house). Override with
                     * XENO_FIELD_CAMDIR=0..7; 0 reproduces the legacy
                     * axis-aligned probes. */
                    {
                        const char* camDir = getenv("XENO_FIELD_CAMDIR");
                        long octant = 7;
                        if (camDir != NULL && camDir[0] != '\0') {
                            octant = strtol(camDir, NULL, 0) & 7;
                        }
                        D_8006F950 = (unsigned short)(octant << 9);
                        printf("[xeno-port][field] camera approach octant=%ld "
                               "(D_8006F950 -> field-script var 8)\n", octant);
                    }
                }
            }
            printf("[xeno-port][field] font + party-skin init done\n");
        } else {
            printf("[xeno-port] WARNING: no disc image found "
                   "(set XENO_DISC or place disc/disc1.bin); archive reads disabled.\n"
                   "[xeno-port] The port contains no game data: provide your own "
                   "Xenogears (USA) Disc 1 BIN image (Redump sha1 "
                   "12db8ccb93516c391630f046a143762337cc21f4). Without it only the "
                   "developer KernelMenu is reachable.\n");
        }
    }

    /* 6. Boot splash: the asm boot shows the "Published by Square" logo before
     * handing off to the game. It is fully self-contained (LZSS-decompress the
     * migrated EXE blob, LoadImage CLUT+texture, DrawPrim a sprite with a
     * fade-in/hold/fade-out via Vsync), and bypasses the game ordering table. */
    (void)xg_plat_mods_emit(XG_PLAT_EVENT_BOOT, 0, 0);
    GameShowSplashScreen();

    /* Retail boot tail (func_80019578, 0x80019888..0x80019938): the opening
     * movie state.  D_8004FE44 = movie type 1, D_8004FE46 = state 1 (Field)
     * to enter afterwards, D_8004FE47 = 0 (skipping allowed), D_8004FE45 =
     * movie selector 16 on disc 1, 7 otherwise; g_CurGameStateOverlayID = -1;
     * then func_8001B6BC (empty),
     * ChangeGameState(6) and MainLoop(0).  MovieMain plays archive 0x18/1
     * the selected movie and hands over to FieldMain with D_8006F94E from the new-game
     * template (map 490, the title field).  XENO_FIELD_TEST=1 keeps the developer path of
     * entering state 0 (KernelMenu) directly. */
    {
        const char* fieldTest = getenv("XENO_FIELD_TEST");
        const char* kernelSelection = getenv("XENO_KERNEL_SEL");
        unsigned int bootState;
        extern unsigned char D_8004FE44;
        extern unsigned char D_8004FE45;
        extern unsigned char D_8004FE46;
        extern unsigned char D_8004FE47;
        extern unsigned int g_CurGameStateOverlayID;
        extern void* g_CurGameStateOverlayBuffer;
        extern int ArchiveGetDiscNumber(void);
        extern void func_8001B6BC(void);
        extern void ChangeGameState(unsigned int state);

        g_CurGameStateOverlayID = (unsigned int)-1;
        g_CurGameStateOverlayBuffer = NULL;
        D_8004FE44 = 1;
        D_8004FE46 = 1;
        D_8004FE47 = 0;
        D_8004FE45 = PcPort_SelectBootMovie(ArchiveGetDiscNumber());
        func_8001B6BC();
        bootState = PcPort_SelectBootState(fieldTest, kernelSelection);
        if (bootState == 6u) {
            printf("[xeno-port][boot] retail boot: movie state 6 (type %u, movie %u) -> state %u\n",
                   (unsigned int)D_8004FE44, (unsigned int)D_8004FE45,
                   (unsigned int)D_8004FE46);
            ChangeGameState(6);
        } else if (bootState == 1u) {
            printf("[xeno-port][boot] direct field-test route: state 1 "
                   "(KernelMenu not rendered)\n");
            ChangeGameState(1);
        }
    }

    /* Oracle bootstrap: the real entry `start` (0x80019524) is still raw MIPS
     * asm, so we call the decompiled MainLoop() directly. It will run real game
     * code until it reaches the first not-yet-decompiled function on the live
     * path, which the stub layer logs as "[stub] <name>". That name is the next
     * thing to decompile. Expect crashes/loops until the boot chain is filled in. */
    printf("[xeno-port] entering decompiled MainLoop() (oracle)...\n");
    MainLoop(0);

    PsyX_Shutdown();
    printf("[xeno-port] Clean shutdown.\n");
    return 0;
}
