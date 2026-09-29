/*
 * W34B18-C production-linked certificate for the world frame-driver prologue
 * 0x800712D0 .. 0x80071484 inclusive, plus the return-bearing 0x800967E4
 * helper that the prologue consumes.
 *
 * Addresses and control flow are derived from disc/world_map.bin
 * (load base 0x8006FAF0), not from production macros.
 *
 *   [0x800712D0, 0x80071488)  110 insns  440 bytes
 *   SHA-256 fccdf4bb24527fca3e26a8d91f368886ea127344a0c59c8f5d0b1ca595f0805d
 *
 * The test object intentionally leaves wm_800967E4_dispatch_cd_work undefined.
 * The focused runner links that symbol from pc_port/src/world_map_init.c and
 * links the frame prologue from pc_port/src/world_map_frame_driver.c.
 *
 *   pc_port/tests/run_w34b18c_800712d0_prod_test.sh
 */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_frame_driver.h"

uint8_t g_PsxRam[PSX_RAM_SIZE];
uint8_t g_PsxScratchpad[4096];

u16 g_C1ButtonState;
u16 g_C2ButtonState;
u16 g_C1ButtonStateReleased;
u16 g_C2ButtonStateReleased;
u16 g_C1ButtonStatePressedOnce;
u16 g_C2ButtonStatePressedOnce;
u32 g_ArchiveDebugTable;

extern u32 wm_800967E4_dispatch_cd_work(void);

/* Retail-derived addresses (lui/addiu and lui/load-store from world_map.bin). */
#define OR_ENVREC1     (0x800A0000u + (u32)(s32)(s16)0xBC40) /* -0x43C0 */
#define OR_DB_PTR      (0x800A0000u + (u32)(s32)(s16)0xBE3C) /* -0x41C4 */
#define OR_INDEX       (0x800A0000u + (u32)(s32)(s16)0xD7F0) /* -0x2810 */
#define OR_D554        (0x800A0000u + (u32)(s32)(s16)0xD554) /* -0x2AAC */
#define OR_BD1C        (0x800A0000u + (u32)(s32)(s16)0xBD1C) /* -0x42E4 */
#define OR_BD14        (0x800A0000u + (u32)(s32)(s16)0xBD14) /* -0x42EC */
#define OR_CD50        (0x800A0000u + (u32)(s32)(s16)0xCD50) /* -0x32B0 */
#define OR_BD18        (0x800A0000u + (u32)(s32)(s16)0xBD18) /* -0x42E8 */
#define OR_BD10        (0x800A0000u + (u32)(s32)(s16)0xBD10) /* -0x42F0 */
#define OR_CD4C        (0x800A0000u + (u32)(s32)(s16)0xCD4C) /* -0x32B4 */
#define OR_CDSYNC      (0x800A0000u + (u32)(s32)(s16)0xC588) /* -0x3A78 */
#define OR_ENVREC0     (0x800A0000u + (u32)(s32)(s16)0xBBC8) /* -0x4438 */
#define OR_OT_OFF      0x70u
#define OR_ENV_STRIDE  0x78u
#define OR_BCB8        0x8009BCB8u
#define OR_D788        0x8009D788u
#define OR_C624        0x8009C624u
#define OR_C1          0x80059570u
#define OR_C2          0x80059574u
#define OR_C1REL       0x8005948Cu
#define OR_C2REL       0x80059490u
#define OR_C1ONCE      0x800594A4u
#define OR_C2ONCE      0x800594A8u

STATIC_ASSERT(OR_ENVREC1 == 0x8009BC40u, envrec1);
STATIC_ASSERT(OR_DB_PTR == 0x8009BE3Cu, dbptr);
STATIC_ASSERT(OR_INDEX == 0x8009D7F0u, index);
STATIC_ASSERT(OR_ENVREC0 == 0x8009BBC8u, envrec0);

enum {
    TR_LW = 1,
    TR_SW = 2,
    TR_LHU = 3,
    TR_SH = 4,
    TR_CALL = 5,
    TR_RET = 6,
    TR_BR = 7
};

enum {
    CALL_POP = 1,
    CALL_967E4 = 2,
    CALL_VSYNC = 3,
    CALL_CDSYNC = 4,
    CALL_CLEAROTAG = 5,
    CALL_250E0 = 6,
    CALL_1D468 = 7,
    CALL_97800 = 8,
    CALL_968E0 = 9,
    CALL_D788 = 10,
    CALL_C624 = 11
};

typedef struct TraceEvent {
    u32 pc;
    u32 kind;
    u32 address;
    u32 width;
    u32 value;
} TraceEvent;

#define TRACE_CAP 256
static TraceEvent s_got[TRACE_CAP];
static size_t s_got_n;
static TraceEvent s_exp[TRACE_CAP];
static size_t s_exp_n;

static int s_pass;
static int s_fail;
static int s_total;

static int s_pop_script[8];
static u16 s_pop_c1[8], s_pop_c2[8], s_pop_c1r[8], s_pop_c2r[8];
static u16 s_pop_c1o[8], s_pop_c2o[8];
static int s_pop_n;
static int s_pop_i;

static u32 s_cd_script[8];
static int s_cd_n;

static u32 s_disp_script[8];
static int s_disp_n;
static int s_disp_i;

static int s_d788_calls;
static u32 s_d788_last;
static int s_c624_calls;
static u32 s_c624_last;
static int s_vsync_calls;
static int s_cdsync_calls;
static u32 s_cdsync_mode;
static u8* s_cdsync_buf;
static int s_clear_calls;
static u32 s_clear_ot;
static int s_clear_n;
static int s_250e0_calls;
static int s_250e0_ctx;
static int s_1d468_calls;
static int s_97800_calls;

static u32 oracle_967e4_body(void);

void wm_712d0_test_trace(u32 pc, u32 kind, u32 address, u32 width, u32 value)
{
    if (s_got_n < TRACE_CAP) {
        s_got[s_got_n].pc = pc;
        s_got[s_got_n].kind = kind;
        s_got[s_got_n].address = address;
        s_got[s_got_n].width = width;
        s_got[s_got_n].value = value;
    }
    s_got_n++;
}

static void exp_event(u32 pc, u32 kind, u32 address, u32 width, u32 value)
{
    if (s_exp_n < TRACE_CAP) {
        s_exp[s_exp_n].pc = pc;
        s_exp[s_exp_n].kind = kind;
        s_exp[s_exp_n].address = address;
        s_exp[s_exp_n].width = width;
        s_exp[s_exp_n].value = value;
    }
    s_exp_n++;
}

static u16 load_u16(u32 address)
{
    u16 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static void store_u16(u32 address, u16 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static u32 load_u32(u32 address)
{
    u32 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static void store_u32(u32 address, u32 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

int ControllerPopState(void)
{
    if (s_pop_i >= s_pop_n)
        return 0;
    g_C1ButtonState = s_pop_c1[s_pop_i];
    g_C2ButtonState = s_pop_c2[s_pop_i];
    g_C1ButtonStateReleased = s_pop_c1r[s_pop_i];
    g_C2ButtonStateReleased = s_pop_c2r[s_pop_i];
    g_C1ButtonStatePressedOnce = s_pop_c1o[s_pop_i];
    g_C2ButtonStatePressedOnce = s_pop_c2o[s_pop_i];
    return s_pop_script[s_pop_i++];
}

int VSync(int mode)
{
    (void)mode;
    s_vsync_calls++;
    return 0;
}

int CdSync(int mode, u8* result)
{
    s_cdsync_calls++;
    s_cdsync_mode = (u32)mode;
    s_cdsync_buf = result;
    return 0;
}

u32* ClearOTagR(u32* ot, int n)
{
    s_clear_calls++;
    s_clear_ot = (u32)(uintptr_t)ot;
    s_clear_n = n;
    return ot;
}

void func_800250E0(int context)
{
    s_250e0_calls++;
    s_250e0_ctx = context;
}

void func_8001D468(void)
{
    s_1d468_calls++;
}

void wm_80097800(void)
{
    s_97800_calls++;
}

/* Link-only ownership seams added to the legacy frame-prologue TU after the
 * original W34B18C certificate was banked. */
void wm_25044_reset(void) {}
void wm_74f2c_reset(void) {}
void wm_ot_clear_r_guest(u32 ot_guest, u32 count)
{
    s_clear_calls++;
    s_clear_ot = ot_guest;
    s_clear_n = (int)count;
}

u32 wm_800968E0_dispatch_partial(void)
{
    if (s_disp_i < s_disp_n)
        return s_disp_script[s_disp_i++];
    return 0;
}

void wm_8009699C_d788_processor(u32 record)
{
    s_d788_calls++;
    s_d788_last = record;
}

void wm_800966CC_c624_processor(u32 record)
{
    s_c624_calls++;
    s_c624_last = record;
}

static void check_case(const char* fixture, const char* assertion, int ok)
{
    s_total++;
    if (ok) {
        s_pass++;
    } else {
        s_fail++;
        printf("FAIL [%s]: ASSERTION %s\n", fixture, assertion);
    }
}

static void seed_ram(u32 seed)
{
    u32 state = seed | 1u;
    size_t i;
    for (i = 0; i < (size_t)PSX_RAM_SIZE; i++) {
        state ^= state << 13;
        state ^= state >> 17;
        state ^= state << 5;
        g_PsxRam[i] = (u8)(state >> 24);
    }
}

static void reset_helpers(void)
{
    s_pop_i = s_disp_i = 0;
    s_d788_calls = s_c624_calls = 0;
    s_d788_last = s_c624_last = 0;
    s_vsync_calls = s_cdsync_calls = 0;
    s_cdsync_mode = 0;
    s_cdsync_buf = NULL;
    s_clear_calls = 0;
    s_clear_ot = 0;
    s_clear_n = 0;
    s_250e0_calls = 0;
    s_250e0_ctx = -1;
    s_1d468_calls = 0;
    s_97800_calls = 0;
    s_got_n = s_exp_n = 0;
    g_ArchiveDebugTable = 0;
    g_C1ButtonState = g_C2ButtonState = 0;
    g_C1ButtonStateReleased = g_C2ButtonStateReleased = 0;
    g_C1ButtonStatePressedOnce = g_C2ButtonStatePressedOnce = 0;
    wm_fp_reset();
}

static void oracle_pad_iter(u16 c1, u16 c2, u16 c1r, u16 c2r, u16 c1o, u16 c2o)
{
    u16 acc_cd4c = load_u16(OR_CD4C);
    u16 acc_cd50 = load_u16(OR_CD50);
    u16 acc_bd10;
    u16 acc_bd14;
    u16 acc_bd18;
    u16 acc_bd1c;

    exp_event(0x80071350u, TR_LHU, OR_CD4C, 2u, acc_cd4c);
    exp_event(0x80071358u, TR_LHU, OR_C1, 2u, c1);
    exp_event(0x80071360u, TR_LHU, OR_CD50, 2u, acc_cd50);
    exp_event(0x80071368u, TR_LHU, OR_C2, 2u, c2);
    acc_cd4c = (u16)(acc_cd4c | c1);
    store_u16(OR_CD4C, acc_cd4c);
    exp_event(0x80071374u, TR_SH, OR_CD4C, 2u, acc_cd4c);

    acc_bd10 = load_u16(OR_BD10);
    exp_event(0x8007137Cu, TR_LHU, OR_BD10, 2u, acc_bd10);
    exp_event(0x80071384u, TR_LHU, OR_C1REL, 2u, c1r);
    acc_cd50 = (u16)(acc_cd50 | c2);
    store_u16(OR_CD50, acc_cd50);
    exp_event(0x80071390u, TR_SH, OR_CD50, 2u, acc_cd50);

    acc_bd14 = load_u16(OR_BD14);
    exp_event(0x80071398u, TR_LHU, OR_BD14, 2u, acc_bd14);
    exp_event(0x800713A0u, TR_LHU, OR_C2REL, 2u, c2r);
    acc_bd10 = (u16)(acc_bd10 | c1r);
    store_u16(OR_BD10, acc_bd10);
    exp_event(0x800713ACu, TR_SH, OR_BD10, 2u, acc_bd10);

    acc_bd18 = load_u16(OR_BD18);
    exp_event(0x800713B4u, TR_LHU, OR_BD18, 2u, acc_bd18);
    exp_event(0x800713BCu, TR_LHU, OR_C1ONCE, 2u, c1o);
    acc_bd14 = (u16)(acc_bd14 | c2r);
    store_u16(OR_BD14, acc_bd14);
    exp_event(0x800713C8u, TR_SH, OR_BD14, 2u, acc_bd14);

    acc_bd1c = load_u16(OR_BD1C);
    exp_event(0x800713D0u, TR_LHU, OR_BD1C, 2u, acc_bd1c);
    exp_event(0x800713D8u, TR_LHU, OR_C2ONCE, 2u, c2o);
    acc_bd18 = (u16)(acc_bd18 | c1o);
    acc_bd1c = (u16)(acc_bd1c | c2o);
    store_u16(OR_BD18, acc_bd18);
    exp_event(0x800713E8u, TR_SH, OR_BD18, 2u, acc_bd18);
    store_u16(OR_BD1C, acc_bd1c);
    exp_event(0x800713F0u, TR_SH, OR_BD1C, 2u, acc_bd1c);
    exp_event(0x800713F4u, TR_BR, 0x8007133Cu, 0u, 1u);
}

static void oracle_frame(void)
{
    u32 db_ptr;
    u32 env;
    u32 ot;
    u32 index;
    u32 flipped;
    int i;

    store_u32(OR_DB_PTR, OR_ENVREC1);
    exp_event(0x800712ECu, TR_SW, OR_DB_PTR, 4u, OR_ENVREC1);
    store_u32(OR_INDEX, 1u);
    exp_event(0x80071300u, TR_SW, OR_INDEX, 4u, 1u);
    store_u32(OR_D554, 1u);
    exp_event(0x80071308u, TR_SW, OR_D554, 4u, 1u);
    store_u16(OR_BD1C, 0);
    exp_event(0x80071310u, TR_SH, OR_BD1C, 2u, 0u);
    store_u16(OR_BD14, 0);
    exp_event(0x80071318u, TR_SH, OR_BD14, 2u, 0u);
    store_u16(OR_CD50, 0);
    exp_event(0x80071320u, TR_SH, OR_CD50, 2u, 0u);
    store_u16(OR_BD18, 0);
    exp_event(0x80071328u, TR_SH, OR_BD18, 2u, 0u);
    store_u16(OR_BD10, 0);
    exp_event(0x80071330u, TR_SH, OR_BD10, 2u, 0u);
    store_u16(OR_CD4C, 0);
    exp_event(0x80071338u, TR_SH, OR_CD4C, 2u, 0u);

    for (i = 0; i < s_pop_n; i++) {
        exp_event(0x8007133Cu, TR_CALL, CALL_POP, 0u, (u32)s_pop_script[i]);
        if (s_pop_script[i] == 0) {
            exp_event(0x80071344u, TR_BR, 0x800713FCu, 0u, 0u);
            break;
        }
        oracle_pad_iter(s_pop_c1[i], s_pop_c2[i], s_pop_c1r[i], s_pop_c2r[i],
                        s_pop_c1o[i], s_pop_c2o[i]);
    }
    if (s_pop_n == 0 || s_pop_script[s_pop_n - 1] != 0) {
        exp_event(0x8007133Cu, TR_CALL, CALL_POP, 0u, 0u);
        exp_event(0x80071344u, TR_BR, 0x800713FCu, 0u, 0u);
    }

    for (i = 0; i < s_cd_n; i++) {
        exp_event(0x800713FCu, TR_CALL, CALL_967E4, 0u, s_cd_script[i]);
        exp_event(0x800713FCu, TR_RET, CALL_967E4, 0u, s_cd_script[i]);
        if (s_cd_script[i] != 3u) {
            exp_event(0x80071404u, TR_BR, 0x8007141Cu, 0u, 1u);
            break;
        }
        exp_event(0x8007140Cu, TR_CALL, CALL_VSYNC, 0u, 0u);
        exp_event(0x80071414u, TR_BR, 0x800713FCu, 0u, 1u);
    }

    exp_event(0x80071424u, TR_CALL, CALL_CDSYNC, 4u, OR_CDSYNC);

    db_ptr = load_u32(OR_DB_PTR);
    exp_event(0x80071430u, TR_LW, OR_DB_PTR, 4u, db_ptr);
    env = OR_ENVREC0;
    if (db_ptr == env)
        env += OR_ENV_STRIDE;
    ot = load_u32(env + OR_OT_OFF);
    exp_event(0x80071448u, TR_LW, env + OR_OT_OFF, 4u, ot);
    index = load_u32(OR_INDEX);
    exp_event(0x80071450u, TR_LW, OR_INDEX, 4u, index);
    flipped = (index < 1u) ? 1u : 0u;
    store_u32(OR_DB_PTR, env);
    exp_event(0x80071458u, TR_SW, OR_DB_PTR, 4u, env);
    store_u32(OR_INDEX, flipped);
    exp_event(0x80071464u, TR_SW, OR_INDEX, 4u, flipped);
    exp_event(0x80071468u, TR_CALL, CALL_CLEAROTAG, 4u, ot);
    index = load_u32(OR_INDEX);
    exp_event(0x80071474u, TR_LW, OR_INDEX, 4u, index);
    exp_event(0x80071478u, TR_CALL, CALL_250E0, 0u, index);
    exp_event(0x80071480u, TR_CALL, CALL_1D468, 0u, 0u);
    exp_event(0x80071488u, TR_CALL, CALL_97800, 0u, 1u);
}

static int traces_equal(void)
{
    size_t i;
    if (s_got_n != s_exp_n)
        return 0;
    for (i = 0; i < s_exp_n; i++) {
        if (s_got[i].pc != s_exp[i].pc ||
            s_got[i].kind != s_exp[i].kind ||
            s_got[i].address != s_exp[i].address ||
            s_got[i].width != s_exp[i].width ||
            s_got[i].value != s_exp[i].value)
            return 0;
    }
    return 1;
}

static void run_frame_fixture(const char* name, u32 seed,
                              int pop_n, const int* pops,
                              const u16* c1, const u16* c2,
                              const u16* c1r, const u16* c2r,
                              const u16* c1o, const u16* c2o,
                              int cd_n, const u32* cds,
                              u32 ot0, u32 ot1)
{
    static u8 expected[PSX_RAM_SIZE];
    int i;
    int vsync_exp = 0;

    seed_ram(seed);
    store_u32(OR_ENVREC0 + OR_OT_OFF, ot0);
    store_u32(OR_ENVREC0 + OR_ENV_STRIDE + OR_OT_OFF, ot1);
    store_u32(OR_DB_PTR, 0x11111111u);
    store_u32(OR_INDEX, 0x22222222u);
    store_u32(OR_D554, 0x33333333u);
    store_u16(OR_CD4C, 0xAAAAu);
    store_u16(OR_CD50, 0xBBBBu);
    store_u16(OR_BD10, 0xCCCCu);
    store_u16(OR_BD14, 0xDDDDu);
    store_u16(OR_BD18, 0xEEEEu);
    store_u16(OR_BD1C, 0xFFFFu);
    store_u32(OR_BCB8, 0u);
    store_u32(OR_D788, 0u);
    store_u32(OR_C624, 0u);

    reset_helpers();
    s_pop_n = pop_n;
    for (i = 0; i < pop_n; i++) {
        s_pop_script[i] = pops[i];
        s_pop_c1[i] = c1[i];
        s_pop_c2[i] = c2[i];
        s_pop_c1r[i] = c1r[i];
        s_pop_c2r[i] = c2r[i];
        s_pop_c1o[i] = c1o[i];
        s_pop_c2o[i] = c2o[i];
    }
    s_cd_n = cd_n;
    s_disp_n = cd_n;
    for (i = 0; i < cd_n; i++) {
        s_cd_script[i] = cds[i];
        s_disp_script[i] = cds[i];
        if (cds[i] == 3u)
            vsync_exp++;
        else
            break;
    }

    oracle_frame();
    memcpy(expected, g_PsxRam, sizeof(expected));

    seed_ram(seed);
    store_u32(OR_ENVREC0 + OR_OT_OFF, ot0);
    store_u32(OR_ENVREC0 + OR_ENV_STRIDE + OR_OT_OFF, ot1);
    store_u32(OR_DB_PTR, 0x11111111u);
    store_u32(OR_INDEX, 0x22222222u);
    store_u32(OR_D554, 0x33333333u);
    store_u16(OR_CD4C, 0xAAAAu);
    store_u16(OR_CD50, 0xBBBBu);
    store_u16(OR_BD10, 0xCCCCu);
    store_u16(OR_BD14, 0xDDDDu);
    store_u16(OR_BD18, 0xEEEEu);
    store_u16(OR_BD1C, 0xFFFFu);
    store_u32(OR_BCB8, 0u);
    store_u32(OR_D788, 0u);
    store_u32(OR_C624, 0u);
    s_pop_i = s_disp_i = 0;
    s_got_n = 0;
    s_vsync_calls = s_cdsync_calls = s_clear_calls = 0;
    s_250e0_calls = s_1d468_calls = s_97800_calls = 0;
    s_250e0_ctx = -1;
    wm_fp_reset();

    wm_800712D0_frame_prologue();

    check_case(name, "event-order-matches-retail-oracle", traces_equal());
    check_case(name, "independent-oracle-final-ram-image",
               memcmp(expected, g_PsxRam, sizeof(expected)) == 0);
    check_case(name, "hard-cut-pc-0x80071490",
               wm_fp_get_cut_pc() == 0x80071490u);
    check_case(name, "second-scheduler-once-after-1D468",
               s_97800_calls == 1 && wm_fp_get_scheduler_calls() == 1);
    check_case(name, "cdsync-mode-1-buf-C588",
               s_cdsync_calls == 1 && s_cdsync_mode == 1u &&
               s_cdsync_buf == (u8*)PSX_ADDR(OR_CDSYNC));
    check_case(name, "clearotag-n-0x400",
               s_clear_calls == 1 && s_clear_n == 0x400);
    check_case(name, "func_8001D468-once", s_1d468_calls == 1);
    check_case(name, "M967-C-dispatch-return-controls-frame-retry",
               s_vsync_calls == vsync_exp);
    check_case(name, "index-toggled-from-1-to-0",
               load_u32(OR_INDEX) == 0u);
    check_case(name, "db-ptr-switched-to-envrec0",
               load_u32(OR_DB_PTR) == OR_ENVREC0);
    check_case(name, "func_800250E0-context-is-new-index",
               s_250e0_calls == 1 && s_250e0_ctx == 0);
}

static u32 oracle_967e4_body(void)
{
    u32 dbg0 = g_ArchiveDebugTable;
    u32 dbg1 = g_ArchiveDebugTable;
    u32 dispatch_result;
    u32 tail;
    u32 d788_record;
    u32 c624_record;

    /* Retail: C624 iff dbg0 != 0 AND dbg1 != 0xFFFFFFFF. */
    if (dbg0 != 0u && dbg1 != 0xFFFFFFFFu) {
        tail = load_u32(OR_BCB8);
        c624_record = load_u32(OR_C624 + tail * 4u);
        if (c624_record == 0u)
            return 0u;
        wm_800966CC_c624_processor(c624_record);
        store_u32(OR_C624 + tail * 4u, 0u);
        store_u32(OR_BCB8, (tail + 1u) & 0x0Fu);
        return 0u;
    }
    dispatch_result = wm_800968E0_dispatch_partial();
    if (dispatch_result != 0u)
        return dispatch_result;
    tail = load_u32(OR_BCB8);
    d788_record = load_u32(OR_D788 + tail * 4u);
    if (d788_record == 0u)
        return 0u;
    wm_8009699C_d788_processor(d788_record);
    return 0u;
}

static void run_967e4_case(const char* name, u32 dbg, u32 tail, u32 d788,
                           u32 c624, u32 disp, u32 expect_ret,
                           int expect_d788, int expect_c624)
{
    static u8 expected[PSX_RAM_SIZE];
    u32 got;

    seed_ram(0x967E4001u);
    reset_helpers();
    g_ArchiveDebugTable = dbg;
    store_u32(OR_BCB8, tail);
    store_u32(OR_D788 + tail * 4u, d788);
    store_u32(OR_C624 + tail * 4u, c624);
    s_disp_n = 1;
    s_disp_script[0] = disp;
    s_disp_i = 0;
    s_d788_calls = s_c624_calls = 0;

    {
        u32 oret;
        s_disp_i = 0;
        oret = oracle_967e4_body();
        memcpy(expected, g_PsxRam, sizeof(expected));
        check_case(name, "oracle-return-matches-fixture", oret == expect_ret);
    }

    seed_ram(0x967E4001u);
    g_ArchiveDebugTable = dbg;
    store_u32(OR_BCB8, tail);
    store_u32(OR_D788 + tail * 4u, d788);
    store_u32(OR_C624 + tail * 4u, c624);
    s_disp_n = 1;
    s_disp_script[0] = disp;
    s_disp_i = 0;
    s_d788_calls = s_c624_calls = 0;
    got = wm_800967E4_dispatch_cd_work();

    check_case(name, "production-return-matches-retail", got == expect_ret);
    check_case(name, "d788-processor-calls", s_d788_calls == expect_d788);
    check_case(name, "c624-processor-calls", s_c624_calls == expect_c624);
    check_case(name, "967e4-final-ram-matches-oracle",
               memcmp(expected, g_PsxRam, sizeof(expected)) == 0);
}

int main(void)
{
    static const int pop_empty[] = {0};
    static const u16 z16[] = {0};
    static const u32 cd_idle[] = {0};
    static const u32 cd_retry[] = {3u, 3u, 1u};
    static const u32 cd_busy[] = {1u};
    static const int pop_one[] = {2, 0};
    static const u16 c1_one[] = {0x8001u, 0};
    static const u16 c2_one[] = {0x0002u, 0};
    static const u16 c1r_one[] = {0x0004u, 0};
    static const u16 c2r_one[] = {0x0008u, 0};
    static const u16 c1o_one[] = {0x0010u, 0};
    static const u16 c2o_one[] = {0x0020u, 0};
    static const int pop_two[] = {1, 1, 0};
    static const u16 c1_two[] = {0x1000u, 0x0001u, 0};
    static const u16 c2_two[] = {0x2000u, 0x0002u, 0};
    static const u16 c1r_two[] = {0x0004u, 0x0040u, 0};
    static const u16 c2r_two[] = {0x0008u, 0x0080u, 0};
    static const u16 c1o_two[] = {0x0010u, 0x0100u, 0};
    static const u16 c2o_two[] = {0x0020u, 0x0200u, 0};

    printf("=== W34B18-C real production 0x800967E4 certificate ===\n");
    printf("SUBJECT_967E4 pc_port/src/world_map_init.c::wm_800967E4_dispatch_cd_work\n");
    printf("SUBJECT_712D0 pc_port/src/world_map_frame_driver.c::wm_800712D0_frame_prologue\n");

    run_frame_fixture("natural-empty-pad-idle-cd", 0x712D0001u,
                      1, pop_empty, z16, z16, z16, z16, z16, z16,
                      1, cd_idle, 0x800F0000u, 0x800F1000u);
    run_frame_fixture("one-pad-event-or-accumulate", 0x712D0002u,
                      2, pop_one, c1_one, c2_one, c1r_one, c2r_one,
                      c1o_one, c2o_one, 1, cd_idle,
                      0x80100000u, 0x80101000u);
    run_frame_fixture("two-pad-events-asymmetric-or", 0x712D0003u,
                      3, pop_two, c1_two, c2_two, c1r_two, c2r_two,
                      c1o_two, c2o_two, 1, cd_idle,
                      0x00ABCDEF, 0x12345678u);
    run_frame_fixture("cd-work-retry-on-return-3", 0x712D0004u,
                      1, pop_empty, z16, z16, z16, z16, z16, z16,
                      3, cd_retry, 0x800E0000u, 0x800E1000u);
    run_frame_fixture("cd-work-busy-return-1-no-retry", 0x712D0005u,
                      1, pop_empty, z16, z16, z16, z16, z16, z16,
                      1, cd_busy, 0x800D0000u, 0x800D1000u);

    /* 967E4 return oracle: A-E. dbg0 is the same table as dbg1. */
    run_967e4_case("A-dbg-table-0-d788-null",
                   0u, 3u, 0u, 0u, 0u, 0u, 0, 0);
    run_967e4_case("A-dbg-table-0-d788-nonzero",
                   0u, 1u, 0x80090000u, 0x80091000u, 0u, 0u, 1, 0);
    run_967e4_case("C-dbg-other-nonzero-C624",
                   2u, 4u, 0x80090000u, 0x80092000u, 0u, 0u, 0, 1);
    run_967e4_case("B-dbg-FFFFFFFF-D788-path",
                   0xFFFFFFFFu, 2u, 0x80093000u, 0x80094000u, 0u, 0u, 1, 0);
    check_case("B-dbg-FFFFFFFF-D788-path",
               "M967-A-FFFFFFFF-sentinel-selects-D788",
               s_d788_calls == 1 && s_c624_calls == 0);
    run_967e4_case("D-D788-tail-zero-returns",
                   0u, 5u, 0u, 0x80095000u, 0u, 0u, 0, 0);
    check_case("D-D788-tail-zero-returns",
               "M967-B-D788-null-returns-without-C624",
               s_d788_calls == 0 && s_c624_calls == 0 &&
                   load_u32(OR_BCB8) == 5u &&
                   load_u32(OR_C624 + 5u * 4u) == 0x80095000u);
    run_967e4_case("E-D788-tail-nonzero-processes",
                   0u, 6u, 0x80096000u, 0x80097000u, 0u, 0u, 1, 0);
    run_967e4_case("disp-return-3-propagates",
                   0u, 0u, 0u, 0u, 3u, 3u, 0, 0);
    run_967e4_case("disp-return-1-propagates",
                   0u, 0u, 0u, 0u, 1u, 1u, 0, 0);

    /* Case A with dbg1==0 and dbg0!=0 cannot be formed from a single table
     * read twice. Retail calls func_8002C3D8 twice; the port's equivalent is
     * two reads of the same global, so dbg0==dbg1 always. The 0xFFFFFFFF vs
     * nonzero distinction is still observable (B vs C). */

    printf("RETAIL_BOUNDARY [0x800712D0,0x80071488) bytes=440 insns=110\n");
    printf("RETAIL_SLICE_SHA256 fccdf4bb24527fca3e26a8d91f368886ea127344a0c59c8f5d0b1ca595f0805d\n");
    printf("RETAIL_967E4_BOUNDARY [0x800967E4,0x800968E0) bytes=252 insns=63\n");
    printf("RETAIL_967E4_SHA256 0d16c4f020e76808390b2ad94cdff89aa6c28b60bcd4beb2bd0890af938b1530\n");
    printf("HARD_CUT 0x80071490 jal 0x80097800 executed once\n");
    printf("=== Results: %d/%d PASS ===\n", s_pass, s_total);
    return s_fail == 0 ? 0 : 1;
}
