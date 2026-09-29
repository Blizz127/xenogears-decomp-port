/*
 * W34B18-B prologue plus W34B19-B second scheduler call.
 *
 * Fresh world_map.bin listing (load base 0x8006FAF0):
 *   [0x800712D0, 0x80071488) = 110 instructions, 440 bytes,
 *   SHA-256 fccdf4bb24527fca3e26a8d91f368886ea127344a0c59c8f5d0b1ca595f0805d
 *   [0x80071488, 0x80071490) = 2 instructions, 8 bytes,
 *   SHA-256 f082c48aadb535cf17202501b97d36df5f30688c9ecdaac5f5fc41f66b197893
 *     80071488  jal 0x80097800
 *     8007148C  nop
 *   [0x80071490, 0x800714D4) = post-pass sync/display setup, 17 instructions.
 *   [0x800714D4,0x8007169C) = bounded state gates and BD34 convergence.
 *   [0x8007169C,0x800716AC) = natural C178 branch; alternate state stops
 *   before unresolved helper 0x8007634C.
 *   [0x8007185C,0x8007197C) = natural flag/update lane; alternate BE10
 *   call-bearing state stops before unresolved helper 0x800758C0.
 *   [0x8007197C,0x80071980) = mapped 0x80025044 image-list transfer;
 *   [0x80071984,0x8007502C) = native 0x80074F2C upload pump;
 *   stop before sibling helper 0x80075104.
 *
 * Both the production game build and the production-linked test link this
 * same object. Do NOT duplicate this function elsewhere.
 */
#include <stdio.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_frame_driver.h"
#include "world_map_image_transfer_25044.h"
#include "world_map_upload_pump_74f2c.h"
#include "world_map_upload_pump_75104.h"
#include "world_map_frame_tail_71984.h"
#include "world_map_ot_adapter.h"

#define WM_FP_DB_PTR        0x8009BE3Cu
#define WM_FP_ENVREC0       0x8009BBC8u
#define WM_FP_ENVREC1       0x8009BC40u
#define WM_FP_INDEX         0x8009D7F0u
#define WM_FP_D554          0x8009D554u
#define WM_FP_BD1C          0x8009BD1Cu
#define WM_FP_BD14          0x8009BD14u
#define WM_FP_CD50          0x8009CD50u
#define WM_FP_BD18          0x8009BD18u
#define WM_FP_BD10          0x8009BD10u
#define WM_FP_CD4C          0x8009CD4Cu
#define WM_FP_CDSYNC_BUF    0x8009C588u
#define WM_FP_69179         0x80069179u
#define WM_FP_BD34          0x8009BD34u
#define WM_FP_C178          0x8009C178u
#define WM_FP_D804          0x8009D804u
#define WM_FP_BD24          0x8009BD24u
#define WM_FP_CE68          0x8009CE68u
#define WM_FP_D80C          0x8009D80Cu
#define WM_FP_BE10          0x8009BE10u
#define WM_FP_EE76          0x8007EE76u
#define WM_FP_OT_OFF        0x70u
#define WM_FP_ENV_STRIDE    0x78u

#if defined(WM_712D0_MUTANT_M2)
#undef WM_FP_DB_PTR
#define WM_FP_DB_PTR        0x8009BE38u
#endif
#if defined(WM_712D0_MUTANT_M12)
#undef WM_FP_OT_OFF
#define WM_FP_OT_OFF        0x6Cu
#endif

#if defined(WM_712D0_MUTANT_M13)
#define WM_FP_OT_COUNT      0x3FF
#else
#define WM_FP_OT_COUNT      0x400
#endif

extern u16 g_C1ButtonState;
extern u16 g_C2ButtonState;
extern u16 g_C1ButtonStateReleased;
extern u16 g_C2ButtonStateReleased;
extern u16 g_C1ButtonStatePressedOnce;
extern u16 g_C2ButtonStatePressedOnce;

extern int ControllerPopState(void);
extern int VSync(int mode);
extern int CdSync(int mode, u8* result);
extern u32* ClearOTagR(u32* ot, int n);
extern void func_800250E0(int context);
extern void func_8001D468(void);
extern u32 wm_800967E4_dispatch_cd_work(void);
extern void wm_80097800(void);
extern int DrawSync(int mode);
extern void GameCheckAndHandleSoftReset(void);
extern void PutDispEnv(void *env);
extern void PutDrawEnv(void *env);
enum {
    WM_FP_TRACE_LW = 1,
    WM_FP_TRACE_SW = 2,
    WM_FP_TRACE_LHU = 3,
    WM_FP_TRACE_SH = 4,
    WM_FP_TRACE_CALL = 5,
    WM_FP_TRACE_RET = 6,
    WM_FP_TRACE_BR = 7
};

enum {
    WM_FP_CALL_POP = 1,
    WM_FP_CALL_967E4 = 2,
    WM_FP_CALL_VSYNC = 3,
    WM_FP_CALL_CDSYNC = 4,
    WM_FP_CALL_CLEAROTAG = 5,
    WM_FP_CALL_250E0 = 6,
    WM_FP_CALL_1D468 = 7,
    WM_FP_CALL_97800 = 8,
    WM_FP_CALL_DRAWSYNC_2 = 9,
    WM_FP_CALL_VSYNC_2 = 10,
    WM_FP_CALL_SOFT_RESET = 11,
    WM_FP_CALL_PUT_DISP = 12,
    WM_FP_CALL_PUT_DRAW = 13,
    WM_FP_CALL_25044 = 14,
    WM_FP_CALL_74F2C = 15,
    WM_FP_CALL_75104 = 16
};

#if defined(WM_712D0_TEST_TRACE)
extern void wm_712d0_test_trace(u32 pc, u32 kind, u32 address,
                                u32 width, u32 value);
#define WM_FP_TRACE(pc, kind, address, width, value) \
    wm_712d0_test_trace((pc), (kind), (address), (width), (value))
#else
#define WM_FP_TRACE(pc, kind, address, width, value) ((void)0)
#endif

static int s_fp_entry;
static int s_fp_cd_work_calls;
static int s_fp_vsync_retries;
static int s_fp_pad_iters;
static int s_fp_scheduler_calls;
static u32 s_fp_cut_pc;

int wm_fp_get_entry(void) { return s_fp_entry; }
int wm_fp_get_cd_work_calls(void) { return s_fp_cd_work_calls; }
int wm_fp_get_vsync_retries(void) { return s_fp_vsync_retries; }
int wm_fp_get_pad_iters(void) { return s_fp_pad_iters; }
int wm_fp_get_scheduler_calls(void) { return s_fp_scheduler_calls; }
int wm_fp_get_image_unknowns(void) { return wm_25044_get_unknowns(); }
u32 wm_fp_get_cut_pc(void) { return s_fp_cut_pc; }

int wm_800719C8_should_reenter_once(int gate_enabled, u32 d554)
{
#if defined(WM_W34B42_MUTANT_GATE)
    (void)gate_enabled;
    return d554 != 0u;
#elif defined(WM_W34B42_MUTANT_D554)
    (void)d554;
    return gate_enabled != 0;
#elif defined(WM_W34B42_MUTANT_ALWAYS)
    (void)gate_enabled;
    (void)d554;
    return 1;
#else
    /* Retail 0x800719C0 loads D554; 0x800719C8 branches when nonzero. */
    return gate_enabled != 0 && d554 != 0u;
#endif
}

void wm_fp_reset(void)
{
    s_fp_entry = 0;
    s_fp_cd_work_calls = 0;
    s_fp_vsync_retries = 0;
    s_fp_pad_iters = 0;
    s_fp_scheduler_calls = 0;
    wm_25044_reset();
    s_fp_cut_pc = 0;
    wm_74f2c_reset();
}

static u16 wm_fp_load_u16(u32 address)
{
    u16 value;
    value = *(volatile u16*)PSX_ADDR(address);
    return value;
}

static void wm_fp_store_u16(u32 address, u16 value)
{
    *(volatile u16*)PSX_ADDR(address) = value;
}

static u32 wm_fp_load_u32(u32 address)
{
    return *(volatile u32*)PSX_ADDR(address);
}

static u8 wm_fp_load_u8(u32 address) __attribute__((unused));
static u8 wm_fp_load_u8(u32 address)
{
    return *(volatile u8*)PSX_ADDR(address);
}

static void wm_fp_store_u32(u32 address, u32 value)
{
    *(volatile u32*)PSX_ADDR(address) = value;
}

static u16 wm_fp_merge_u16(u16 acc, u16 src)
{
#if defined(WM_712D0_MUTANT_M4)
    /* Signedness mutant: treat bit 15 as a signed negative and drop the merge. */
    if ((src & 0x8000u) != 0u)
        return acc;
    return (u16)(acc | src);
#else
    return (u16)(acc | src);
#endif
}

void wm_800712D0_frame_prologue(void)
{
    int pop;
    u32 cd_work;
    u32 db_ptr;
    u32 env;
    u32 ot;
    u32 index;
    u32 flipped;
#if defined(WM_712D0_MUTANT_M6)
    u32 stale_cd = 0;
#endif
#if defined(WM_712D0_TEST_TRACE)
    int pad_guard = 0;
    int cd_guard = 0;
#endif

    s_fp_entry++;
    fprintf(stderr, "[worldmap-frame-prologue] entry call=%d\n", s_fp_entry);

    wm_fp_store_u32(WM_FP_DB_PTR, WM_FP_ENVREC1);
    WM_FP_TRACE(0x800712ECu, WM_FP_TRACE_SW, WM_FP_DB_PTR, 4u, WM_FP_ENVREC1);

    wm_fp_store_u32(WM_FP_INDEX, 1u);
    WM_FP_TRACE(0x80071300u, WM_FP_TRACE_SW, WM_FP_INDEX, 4u, 1u);

    wm_fp_store_u32(WM_FP_D554, 1u);
    WM_FP_TRACE(0x80071308u, WM_FP_TRACE_SW, WM_FP_D554, 4u, 1u);

#if defined(WM_712D0_MUTANT_M3)
    wm_fp_store_u32(WM_FP_BD1C, 0u);
    wm_fp_store_u32(WM_FP_BD14, 0u);
    wm_fp_store_u32(WM_FP_CD50, 0u);
    wm_fp_store_u32(WM_FP_BD18, 0u);
    wm_fp_store_u32(WM_FP_BD10, 0u);
    wm_fp_store_u32(WM_FP_CD4C, 0u);
#else
    wm_fp_store_u16(WM_FP_BD1C, 0);
    WM_FP_TRACE(0x80071310u, WM_FP_TRACE_SH, WM_FP_BD1C, 2u, 0u);
    wm_fp_store_u16(WM_FP_BD14, 0);
    WM_FP_TRACE(0x80071318u, WM_FP_TRACE_SH, WM_FP_BD14, 2u, 0u);
    wm_fp_store_u16(WM_FP_CD50, 0);
    WM_FP_TRACE(0x80071320u, WM_FP_TRACE_SH, WM_FP_CD50, 2u, 0u);
    wm_fp_store_u16(WM_FP_BD18, 0);
    WM_FP_TRACE(0x80071328u, WM_FP_TRACE_SH, WM_FP_BD18, 2u, 0u);
    wm_fp_store_u16(WM_FP_BD10, 0);
    WM_FP_TRACE(0x80071330u, WM_FP_TRACE_SH, WM_FP_BD10, 2u, 0u);
    wm_fp_store_u16(WM_FP_CD4C, 0);
    WM_FP_TRACE(0x80071338u, WM_FP_TRACE_SH, WM_FP_CD4C, 2u, 0u);
#endif

    for (;;) {
        pop = ControllerPopState();
        WM_FP_TRACE(0x8007133Cu, WM_FP_TRACE_CALL, WM_FP_CALL_POP, 0u,
                    (u32)pop);
        s_fp_pad_iters++;
#if defined(WM_712D0_TEST_TRACE)
        pad_guard++;
        if (pad_guard > 32)
            break;
#endif
#if defined(WM_712D0_MUTANT_M1)
        if (pop != 0)
#else
        if (pop == 0)
#endif
        {
            WM_FP_TRACE(0x80071344u, WM_FP_TRACE_BR, 0x800713FCu, 0u, 0u);
            break;
        }
        /* Exact retail access order for one pad-merge iteration. */
        {
            u16 acc_cd4c = wm_fp_load_u16(WM_FP_CD4C);
            u16 c1 = g_C1ButtonState;
            u16 acc_cd50 = wm_fp_load_u16(WM_FP_CD50);
            u16 c2 = g_C2ButtonState;
            WM_FP_TRACE(0x80071350u, WM_FP_TRACE_LHU, WM_FP_CD4C, 2u, acc_cd4c);
            WM_FP_TRACE(0x80071358u, WM_FP_TRACE_LHU, 0x80059570u, 2u, c1);
            WM_FP_TRACE(0x80071360u, WM_FP_TRACE_LHU, WM_FP_CD50, 2u, acc_cd50);
            WM_FP_TRACE(0x80071368u, WM_FP_TRACE_LHU, 0x80059574u, 2u, c2);
            acc_cd4c = wm_fp_merge_u16(acc_cd4c, c1);
            wm_fp_store_u16(WM_FP_CD4C, acc_cd4c);
            WM_FP_TRACE(0x80071374u, WM_FP_TRACE_SH, WM_FP_CD4C, 2u, acc_cd4c);

            {
                u16 acc_bd10 = wm_fp_load_u16(WM_FP_BD10);
                u16 c1rel = g_C1ButtonStateReleased;
                WM_FP_TRACE(0x8007137Cu, WM_FP_TRACE_LHU, WM_FP_BD10, 2u,
                            acc_bd10);
                WM_FP_TRACE(0x80071384u, WM_FP_TRACE_LHU, 0x8005948Cu, 2u,
                            c1rel);
                acc_cd50 = wm_fp_merge_u16(acc_cd50, c2);
                wm_fp_store_u16(WM_FP_CD50, acc_cd50);
                WM_FP_TRACE(0x80071390u, WM_FP_TRACE_SH, WM_FP_CD50, 2u,
                            acc_cd50);

                {
                    u16 acc_bd14 = wm_fp_load_u16(WM_FP_BD14);
                    u16 c2rel = g_C2ButtonStateReleased;
                    WM_FP_TRACE(0x80071398u, WM_FP_TRACE_LHU, WM_FP_BD14, 2u,
                                acc_bd14);
                    WM_FP_TRACE(0x800713A0u, WM_FP_TRACE_LHU, 0x80059490u, 2u,
                                c2rel);
                    acc_bd10 = wm_fp_merge_u16(acc_bd10, c1rel);
                    wm_fp_store_u16(WM_FP_BD10, acc_bd10);
                    WM_FP_TRACE(0x800713ACu, WM_FP_TRACE_SH, WM_FP_BD10, 2u,
                                acc_bd10);

                    {
                        u16 acc_bd18 = wm_fp_load_u16(WM_FP_BD18);
                        u16 c1once = g_C1ButtonStatePressedOnce;
                        WM_FP_TRACE(0x800713B4u, WM_FP_TRACE_LHU, WM_FP_BD18,
                                    2u, acc_bd18);
                        WM_FP_TRACE(0x800713BCu, WM_FP_TRACE_LHU, 0x800594A4u,
                                    2u, c1once);
                        acc_bd14 = wm_fp_merge_u16(acc_bd14, c2rel);
                        wm_fp_store_u16(WM_FP_BD14, acc_bd14);
                        WM_FP_TRACE(0x800713C8u, WM_FP_TRACE_SH, WM_FP_BD14,
                                    2u, acc_bd14);

                        {
                            u16 acc_bd1c = wm_fp_load_u16(WM_FP_BD1C);
                            u16 c2once = g_C2ButtonStatePressedOnce;
                            WM_FP_TRACE(0x800713D0u, WM_FP_TRACE_LHU,
                                        WM_FP_BD1C, 2u, acc_bd1c);
                            WM_FP_TRACE(0x800713D8u, WM_FP_TRACE_LHU,
                                        0x800594A8u, 2u, c2once);
                            acc_bd18 = wm_fp_merge_u16(acc_bd18, c1once);
                            acc_bd1c = wm_fp_merge_u16(acc_bd1c, c2once);
                            wm_fp_store_u16(WM_FP_BD18, acc_bd18);
                            WM_FP_TRACE(0x800713E8u, WM_FP_TRACE_SH,
                                        WM_FP_BD18, 2u, acc_bd18);
                            wm_fp_store_u16(WM_FP_BD1C, acc_bd1c);
                            WM_FP_TRACE(0x800713F0u, WM_FP_TRACE_SH,
                                        WM_FP_BD1C, 2u, acc_bd1c);
                        }
                    }
                }
            }
        }
        WM_FP_TRACE(0x800713F4u, WM_FP_TRACE_BR, 0x8007133Cu, 0u, 1u);
    }

    for (;;) {
        cd_work = wm_800967E4_dispatch_cd_work();
        WM_FP_TRACE(0x800713FCu, WM_FP_TRACE_CALL, WM_FP_CALL_967E4, 0u,
                    cd_work);
        WM_FP_TRACE(0x800713FCu, WM_FP_TRACE_RET, WM_FP_CALL_967E4, 0u,
                    cd_work);
        s_fp_cd_work_calls++;
#if defined(WM_712D0_MUTANT_M6)
        {
            u32 used = stale_cd;
            stale_cd = cd_work;
            cd_work = used;
        }
#endif
#if defined(WM_712D0_TEST_TRACE)
        cd_guard++;
        if (cd_guard > 32)
            break;
#endif
#if defined(WM_712D0_MUTANT_M5)
        (void)cd_work;
        WM_FP_TRACE(0x80071404u, WM_FP_TRACE_BR, 0x8007141Cu, 0u, 1u);
        break;
#elif defined(WM_712D0_MUTANT_M8)
        if (cd_work != 0u)
#else
        if (cd_work != 3u)
#endif
        {
            WM_FP_TRACE(0x80071404u, WM_FP_TRACE_BR, 0x8007141Cu, 0u, 1u);
            break;
        }
        (void)VSync(0);
        WM_FP_TRACE(0x8007140Cu, WM_FP_TRACE_CALL, WM_FP_CALL_VSYNC, 0u, 0u);
        s_fp_vsync_retries++;
        WM_FP_TRACE(0x80071414u, WM_FP_TRACE_BR, 0x800713FCu, 0u, 1u);
    }

    (void)CdSync(1, (u8*)PSX_ADDR(WM_FP_CDSYNC_BUF));
    WM_FP_TRACE(0x80071424u, WM_FP_TRACE_CALL, WM_FP_CALL_CDSYNC, 4u,
                WM_FP_CDSYNC_BUF);

    db_ptr = wm_fp_load_u32(WM_FP_DB_PTR);
    WM_FP_TRACE(0x80071430u, WM_FP_TRACE_LW, WM_FP_DB_PTR, 4u, db_ptr);
    env = WM_FP_ENVREC0;
#if defined(WM_712D0_MUTANT_M11)
    if (db_ptr != env)
#else
    if (db_ptr == env)
#endif
        env += WM_FP_ENV_STRIDE;

    ot = wm_fp_load_u32(env + WM_FP_OT_OFF);
    WM_FP_TRACE(0x80071448u, WM_FP_TRACE_LW, env + WM_FP_OT_OFF, 4u, ot);
    index = wm_fp_load_u32(WM_FP_INDEX);
    WM_FP_TRACE(0x80071450u, WM_FP_TRACE_LW, WM_FP_INDEX, 4u, index);

#if defined(WM_712D0_MUTANT_M14)
    /* Invert sltiu result: index 1 must become 0, mutant stores 1. */
    flipped = (index < 1u) ? 0u : 1u;
#else
    flipped = (index < 1u) ? 1u : 0u;
#endif

#if defined(WM_712D0_MUTANT_M7)
    wm_ot_clear_r_guest(ot, (u32)WM_FP_OT_COUNT); /* W34B38 guest-native clear */
    WM_FP_TRACE(0x80071468u, WM_FP_TRACE_CALL, WM_FP_CALL_CLEAROTAG, 4u, ot);
    wm_fp_store_u32(WM_FP_DB_PTR, env);
    WM_FP_TRACE(0x80071458u, WM_FP_TRACE_SW, WM_FP_DB_PTR, 4u, env);
    wm_fp_store_u32(WM_FP_INDEX, flipped);
    WM_FP_TRACE(0x80071464u, WM_FP_TRACE_SW, WM_FP_INDEX, 4u, flipped);
#else
    wm_fp_store_u32(WM_FP_DB_PTR, env);
    WM_FP_TRACE(0x80071458u, WM_FP_TRACE_SW, WM_FP_DB_PTR, 4u, env);
    wm_fp_store_u32(WM_FP_INDEX, flipped);
    WM_FP_TRACE(0x80071464u, WM_FP_TRACE_SW, WM_FP_INDEX, 4u, flipped);
    wm_ot_clear_r_guest(ot, (u32)WM_FP_OT_COUNT); /* W34B38 guest-native clear */
    WM_FP_TRACE(0x80071468u, WM_FP_TRACE_CALL, WM_FP_CALL_CLEAROTAG, 4u, ot);
#endif

    index = wm_fp_load_u32(WM_FP_INDEX);
    WM_FP_TRACE(0x80071474u, WM_FP_TRACE_LW, WM_FP_INDEX, 4u, index);
    func_800250E0((int)index);
    WM_FP_TRACE(0x80071478u, WM_FP_TRACE_CALL, WM_FP_CALL_250E0, 0u, index);

#if defined(WM_71488_MUTANT_M3)
    wm_80097800();
    s_fp_scheduler_calls++;
    WM_FP_TRACE(0x80071488u, WM_FP_TRACE_CALL, WM_FP_CALL_97800, 0u, 1u);
#endif

    func_8001D468();
    WM_FP_TRACE(0x80071480u, WM_FP_TRACE_CALL, WM_FP_CALL_1D468, 0u, 0u);

#if defined(WM_712D0_MUTANT_M9) || defined(WM_71488_MUTANT_M1)
    /* Omit the second scheduler jal at 0x80071488. */
#elif defined(WM_712D0_MUTANT_M10) || defined(WM_71488_MUTANT_M2)
    wm_80097800();
    wm_80097800();
    s_fp_scheduler_calls += 2;
    WM_FP_TRACE(0x80071488u, WM_FP_TRACE_CALL, WM_FP_CALL_97800, 0u, 2u);
#elif defined(WM_71488_MUTANT_M3)
    /* Already invoked before func_8001D468. */
#else
    /* Retail 0x80071488: jal 0x80097800 / 0x8007148C: nop. */
    wm_80097800();
    s_fp_scheduler_calls++;
    WM_FP_TRACE(0x80071488u, WM_FP_TRACE_CALL, WM_FP_CALL_97800, 0u, 1u);
#endif

#if defined(WM_71488_MUTANT_M10)
    /* Copied-source cheat: invent a 925A0 body the retail jal never calls. */
    {
        extern void wm_71488_mutant_fake_925a0(void);
        wm_71488_mutant_fake_925a0();
    }
#endif

#if defined(WM_71490_CONTINUATION_DISABLED)
    /* The W34B18-C certificate intentionally stops at the old frontier. */
    s_fp_cut_pc = 0x80071490u;
#elif defined(WM_71488_MUTANT_M5)
    s_fp_cut_pc = 0x80071498u;
    (void)DrawSync(0);
#elif defined(WM_71488_MUTANT_M4)
    s_fp_cut_pc = 0x80071488u;
#else
    /* Retail 0x80071490..0x800714C4. Environment words in BE3C are guest
     * addresses; host PsyCross receives mapped pointers. */
    (void)DrawSync(0);
    WM_FP_TRACE(0x80071490u, WM_FP_TRACE_CALL, WM_FP_CALL_DRAWSYNC_2, 0u, 0u);
    (void)VSync(2);
    WM_FP_TRACE(0x80071498u, WM_FP_TRACE_CALL, WM_FP_CALL_VSYNC_2, 0u, 2u);
    GameCheckAndHandleSoftReset();
    WM_FP_TRACE(0x800714A0u, WM_FP_TRACE_CALL, WM_FP_CALL_SOFT_RESET, 0u, 0u);
    env = wm_fp_load_u32(WM_FP_DB_PTR);
    WM_FP_TRACE(0x800714ACu, WM_FP_TRACE_LW, WM_FP_DB_PTR, 4u, env);
    PutDispEnv(PSX_ADDR(env + 0x5Cu));
    WM_FP_TRACE(0x800714B0u, WM_FP_TRACE_CALL, WM_FP_CALL_PUT_DISP, 4u,
                env + 0x5Cu);
    PutDrawEnv(PSX_ADDR(env));
    WM_FP_TRACE(0x800714C0u, WM_FP_TRACE_CALL, WM_FP_CALL_PUT_DRAW, 4u, env);
#if defined(WM_7169C_CONTINUATION_DISABLED)
    s_fp_cut_pc = 0x800714D4u;
#else
    s_fp_cut_pc = WM_FRAME_PROLOGUE_CUT;
#endif

    /* Retail 0x800714D4..0x80071698: fresh state gates converge on the
     * 0x80071694 BD34=0 store.  Keep the next 0x80093F18 call outside this
     * bounded slice; when every gate passes, report its exact call PC. */
 #if !defined(WM_7169C_CONTINUATION_DISABLED)
    {
        int reaches_93f18 = 0;
        if (wm_fp_load_u8(WM_FP_69179) == 0u &&
            wm_fp_load_u32(WM_FP_BD34) != 0u &&
            wm_fp_load_u32(WM_FP_C178) == 0u &&
            wm_fp_load_u32(WM_FP_D804) == 0u &&
            (s16)wm_fp_load_u16(WM_FP_BD24) == (s16)-1 &&
            (s16)wm_fp_load_u16(WM_FP_CE68) ==
                (s16)wm_fp_load_u16(WM_FP_BD24) &&
            wm_fp_load_u32(WM_FP_D554) != 0u &&
            wm_fp_load_u32(WM_FP_D80C) == 0u) {
            reaches_93f18 = 1;
        }
        wm_fp_store_u32(WM_FP_BD34, 0u);
        WM_FP_TRACE(0x80071698u, WM_FP_TRACE_SW, WM_FP_BD34, 4u, 0u);
        if (reaches_93f18)
            s_fp_cut_pc = 0x80071578u;
        else
            s_fp_cut_pc = 0x8007169Cu;
    }
 #endif
 #if !defined(WM_7169C_CONTINUATION_DISABLED) && \
     !defined(WM_7185C_CONTINUATION_DISABLED)
    /* Retail 0x8007169C..0x800716A8: the natural C178!=0 branch reaches
     * the next bounded region. The alternate lane is held at its first
     * unresolved helper call rather than speculating across it. */
    {
        u32 c178 = wm_fp_load_u32(WM_FP_C178);
        WM_FP_TRACE(0x800716A8u, WM_FP_TRACE_BR, WM_FP_C178, 4u,
                    c178 != 0u ? 1u : 0u);
        if (c178 != 0u)
            s_fp_cut_pc = 0x8007185Cu;
        else
            s_fp_cut_pc = 0x80071704u;
    }
 #endif
 #if !defined(WM_7169C_CONTINUATION_DISABLED) && \
     !defined(WM_7197C_CONTINUATION_DISABLED)
    /* Retail 0x8007185C..0x80071978: clear D80C, toggle EE76 for the
     * BD10 bit, then take the natural C178!=0 lane through D804=0. The
     * alternate call-bearing BE10 lane stops before 0x800758C0. */
    {
        u16 bd10 = wm_fp_load_u16(WM_FP_BD10);
        wm_fp_store_u32(WM_FP_D80C, 0u);
        WM_FP_TRACE(0x80071868u, WM_FP_TRACE_SW, WM_FP_D80C, 4u, 0u);
        if ((bd10 & 0x0100u) != 0u) {
            u16 ee76 = wm_fp_load_u16(WM_FP_EE76);
            ee76 ^= 1u;
            wm_fp_store_u16(WM_FP_EE76, ee76);
            WM_FP_TRACE(0x8007188Cu, WM_FP_TRACE_SH, WM_FP_EE76, 2u,
                        ee76);
        }
        u32 c178 = wm_fp_load_u32(WM_FP_C178);
        int reaches_758c0 = 0;
        if (c178 == 0u) {
            u32 d804 = wm_fp_load_u32(WM_FP_D804);
            u32 d554 = wm_fp_load_u32(WM_FP_D554);
            s32 be10 = (s32)wm_fp_load_u32(WM_FP_BE10);
            reaches_758c0 = d804 != 0u && d554 != 0u &&
                            be10 > 0 && be10 < 4;
        }
        if (!reaches_758c0) {
            wm_fp_store_u32(WM_FP_D804, 0u);
            WM_FP_TRACE(0x80071978u, WM_FP_TRACE_SW, WM_FP_D804, 4u, 0u);
            s_fp_cut_pc = 0x8007197Cu;
        } else {
            s_fp_cut_pc = 0x800718E8u;
        }
    }
 #endif
 #if !defined(WM_7169C_CONTINUATION_DISABLED) && \
     !defined(WM_7197C_CONTINUATION_DISABLED) && \
     !defined(WM_71984_CONTINUATION_DISABLED)
    /* Retail 0x8007197C calls the image-list transfer, then retail
     * 0x80071984 enters the native 0x80074F2C pump. */
    wm_80025044_guest_safe();
    WM_FP_TRACE(0x8007197Cu, WM_FP_TRACE_CALL, WM_FP_CALL_25044, 0u, 0u);
    (void)wm_80074F2C();
    WM_FP_TRACE(0x80071984u, WM_FP_TRACE_CALL, WM_FP_CALL_74F2C, 0u, 0u);
    (void)wm_80075104();
    WM_FP_TRACE(0x8007198Cu, WM_FP_TRACE_CALL, WM_FP_CALL_75104, 0u, 0u);
    if (wm_80071984_tail() != 0)
        s_fp_cut_pc = 0x800719C8u;
    else
        s_fp_cut_pc = 0x800719D0u;
 #endif
#endif
    fprintf(stderr,
            "[worldmap-frame-prologue] HARD CUT before 0x%08x "
            "cd_work_calls=%d vsync_retries=%d pad_iters=%d "
            "scheduler_calls=%d\n",
            s_fp_cut_pc, s_fp_cd_work_calls, s_fp_vsync_retries,
            s_fp_pad_iters, s_fp_scheduler_calls);
}
