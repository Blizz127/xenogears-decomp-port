/*
 * World-map frame driver 0x800712D0.
 * Session prologue, recurring render/update loop, and natural session exit.
 */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "system/controller.h"
#include "world_map_frame_driver_712d0.h"
#include "world_map_image_transfer_25044.h"
#include "world_map_helper_96130.h"
#include "world_map_helper_97244.h"
#include "world_map_helper_9623c.h"
#include "world_map_helper_981c8.h"
#include "world_map_helper_983a0.h"
#include "world_map_helper_98cc0.h"
#include "world_map_helper_9932c.h"
#include "world_map_helper_737ec.h"
#include "world_map_helper_740b8.h"
#include "world_map_helper_747dc.h"
#include "world_map_helper_848f4.h"
#include "world_map_helper_85cdc.h"
#include "world_map_helper_8615c.h"
#include "world_map_helper_86798.h"
#include "world_map_helper_89748.h"
#include "world_map_helper_89c78.h"
#include "world_map_helper_93f18.h"
#include "world_map_helper_73b04.h"
#include "world_map_helper_75d4c.h"
#include "world_map_helper_75e7c.h"
#include "world_map_helper_762fc.h"
#include "world_map_menu_lifecycle.h"
#include "world_map_r4world_71a58.h"
#include "world_map_ot_adapter.h"
#include "world_map_pause.h"
#include "world_map_upload_pump_74f2c.h"
#include "world_map_upload_pump_75104.h"

/* PsyQ functions */
extern void DrawSync(void (*func)(unsigned long));
extern int Vsync(int mode);
extern void PutDrawEnv(void *env);
extern void PutDispEnv(void *env);
extern void SetGeomOffset(long ofx, long ofy);
extern void MoveImage(void *rect, long x, long y);
extern int CdSync(int mode, u_char *result);
extern void wm_80097800(void);
extern u8 D_8005954C;
extern u8 D_80059179;
extern u8 D_80059460;
extern u8 D_80059171;
extern u8 g_MenuDebugEnabled;
extern void PcPort_WorldTestInputMerge(u16 *held_buttons,
                                       u16 *pressed_edges,
                                       u16 *repeat_edges)
    __attribute__((weak));

extern void func_800250E0(int context);
extern void func_8001D468(void);
extern int ControllerPopState(void);
extern int ControllerGetType(int port);
extern void ResetGraph(int mode);
extern void GameCheckAndHandleSoftReset(void);
extern void MenuMain(void);

/* Global addresses */
#define D_8009BE3C  0x8009BE3Cu
#define D_8009D7F0  0x8009D7F0u
#define D_8009D554  0x8009D554u
#define D_8009BD1C  0x8009BD1Cu
#define D_8009BD14  0x8009BD14u
#define D_8009CD50  0x8009CD50u
#define D_8009BD18  0x8009BD18u
#define D_8009BD10  0x8009BD10u
#define D_8009CD4C  0x8009CD4Cu
#define D_8009BD34  0x8009BD34u
#define D_8009C178  0x8009C178u
#define D_8009D804  0x8009D804u
#define D_8009BD24  0x8009BD24u
#define D_8009CE68  0x8009CE68u
#define D_8009BE10  0x8009BE10u
#define D_8009BE0C  0x8009BE0Cu
#define D_8009D80C  0x8009D80Cu
#define D_8009D558  0x8009D558u
#define D_8006EE76  0x8006EE76u
#define D_8006EE70  0x8006EE70u
#define D_8006EE68  0x8006EE68u
#define D_8009D7CC  0x8009D7CCu
#define D_8009D7D8  0x8009D7D8u
#define D_8009D55C  0x8009D55Cu
#define D_8006F8E5  0x8006F8E5u
#define D_8009B6E4  0x8009B6E4u
#define D_8009BC9C  0x8009BC9Cu
#define D_8009C588  0x8009C588u
#define D_8009B6E4  0x8009B6E4u
#define D_8006EF64  0x8006EF64u

static u32 fd_lw(u32 a) { u32 v; memcpy(&v, PSX_ADDR(a), 4); return v; }
static void fd_sw(u32 a, u32 v) { memcpy(PSX_ADDR(a), &v, 4); }
static u16 fd_lhu(u32 a) { u16 v; memcpy(&v, PSX_ADDR(a), 2); return v; }
static s16 fd_lh(u32 a) { s16 v; memcpy(&v, PSX_ADDR(a), 2); return v; }
static void fd_sh(u32 a, u16 v) { memcpy(PSX_ADDR(a), &v, 2); }
static u8 fd_lbu(u32 a) { return *(u8*)PSX_ADDR(a); }
static void fd_sb(u32 a, u8 v) { *(u8*)PSX_ADDR(a) = v; }

/* Retail 0x80071488: the post-clear scheduler pass which republishes the
 * current frame's packets before DrawOTag. Kept as a narrow test seam so the
 * actual linked driver cannot silently regress to a local no-op again. */
void wm_712d0_run_second_scheduler(void)
{
#if defined(WM_712D0_MUTANT_NO_SECOND_SCHEDULER)
    return;
#elif defined(WM_712D0_MUTANT_DOUBLE_SECOND_SCHEDULER)
    wm_80097800();
    wm_80097800();
#else
    wm_80097800();
#endif
}

/* Retail 0x800713FC..0x8007142C: retry only dispatcher status 3, yielding
 * once per retry, then report CdSync status through the guest result word. */
void wm_712d0_run_cd_sync_lane(void)
{
    s32 queue_result;

    do {
        queue_result = (s32)wm_800967E4();
        if (queue_result == 3) {
#if !defined(W34N21_MUTANT_DRIVER_SKIP_RETRY_VSYNC)
            Vsync(0);
#endif
        }
    } while (queue_result == 3);

#if defined(W34N21_MUTANT_DRIVER_NULL_CDSYNC_RESULT)
    CdSync(1, NULL);
#else
    CdSync(1, (u_char *)PSX_ADDR(D_8009C588));
#endif
}

void wm_712d0_run_transition_lane(void)
{
    s32 result = 0;

#if defined(W34N18_LANE_MUTANT_WRONG_D80C_GUARD)
    if (fd_lw(D_8009C178) == 0u && fd_lw(D_8009D804) == 0u &&
            fd_lh(D_8009BD24) == -1 && fd_lh(D_8009CE68) == -1 &&
            fd_lw(D_8009D554) != 0u && fd_lw(D_8009D80C) == 0u) {
#else
    if (fd_lw(D_8009C178) == 0u && fd_lw(D_8009D804) == 0u &&
            fd_lh(D_8009BD24) == -1 && fd_lh(D_8009CE68) == -1 &&
            fd_lw(D_8009D554) != 0u && fd_lw(D_8009D80C) != 0u) {
#endif
#if defined(W34N18_LANE_MUTANT_SKIP_SELECTOR_CALL)
        result = 0;
#else
        result = wm_80075E7C(D_8009D55C, (s32)fd_lhu(D_8006EF64));
#endif
        if (result == 1) {
#if !defined(W34N18_LANE_MUTANT_SKIP_SESSION_EXIT)
            fd_sw(D_8009D554, 0u);
            fd_sw(D_8009D7CC, (u32)result);
#endif
            D_8005954C = 0u;
#if defined(W34N18_LANE_MUTANT_WRONG_PARTY_PUBLISH)
            fd_sh(D_8006EE70, (u16)fd_lbu(D_8006F8E5 + 1u));
#else
            fd_sh(D_8006EE70, (u16)fd_lbu(D_8006F8E5));
#endif
            fd_sh(D_8006EE70 + 2u, (u16)fd_lbu(D_8006F8E5 + 1u));
            fd_sh(D_8006EE70 + 4u, (u16)fd_lbu(D_8006F8E5 + 2u));
        }
    }

#if !defined(W34N18_LANE_MUTANT_SKIP_D80C_CLEAR)
    fd_sw(D_8009D80C, 0u);
#endif
#if defined(W34N18_LANE_MUTANT_WRONG_TOGGLE_MASK)
    if ((fd_lhu(D_8009BD10) & 0x0800u) != 0u) {
#else
    if ((fd_lhu(D_8009BD10) & 0x0100u) != 0u) {
#endif
        u16 toggle = fd_lhu(D_8006EE76);
        fd_sh(D_8006EE76, (u16)(toggle ^ 1u));
    }
}

/* Retail 0x800714C8..0x8007169C: refresh the three party-presence
 * channels after a world-area change. The byte at 0x80059179 belongs to
 * the native main-executable global; the remaining world-overlay state is
 * guest memory. */
void wm_712d0_run_party_refresh_lane(void)
{
    int guard;

#if defined(W34N19_MUTANT_PARTY_GUARD_GUEST_TWIN)
    guard = fd_lbu(0x80069179u) == 0u;
#else
    guard = D_80059179 == 0u;
#endif
    guard = guard && fd_lw(D_8009BD34) != 0u &&
            fd_lw(D_8009C178) == 0u && fd_lw(D_8009D804) == 0u &&
            fd_lh(D_8009BD24) == -1 && fd_lh(D_8009CE68) == -1 &&
            fd_lw(D_8009D554) != 0u && fd_lw(D_8009D80C) == 0u;

#if !defined(W34N19_MUTANT_PARTY_CLEAR_ONLY_ON_GUARD)
    /* Both the taken body and retail's 0x80071694 reconvergence clear BD34. */
    fd_sw(D_8009BD34, 0u);
#endif
    if (guard) {
        s32 area_result;
        u32 channel;

#if defined(W34N19_MUTANT_PARTY_CLEAR_ONLY_ON_GUARD)
        fd_sw(D_8009BD34, 0u);
#endif
        area_result = wm_80093F18(D_8009D55C);
        if ((s16)area_result == 4)
            return;

        for (channel = 0u; channel < 3u; channel++)
            fd_sh(D_8006EE70 + channel * 2u,
                  (u16)fd_lbu(D_8006F8E5 + channel));

#if defined(W34N19_MUTANT_PARTY_INVERT_PRESENCE_BRANCH)
        if (fd_lbu(D_8006F8E5) == 0u) {
#else
        if (fd_lbu(D_8006F8E5) != 0u) {
#endif
            fd_sb(D_8006F8E5 + 2u, 0u);
            fd_sb(D_8006F8E5 + 1u, 0u);
            fd_sb(D_8006F8E5, 0u);
        } else {
            for (channel = 0u; channel < 3u; channel++) {
#if defined(W34N19_MUTANT_PARTY_WRONG_SELECTOR)
                u8 selector = fd_lbu(D_8006F8E5 + channel);
#else
                u8 selector = fd_lbu(0x8006F368u + channel);
#endif
#if defined(W34N19_MUTANT_PARTY_WRONG_STRIDE)
                u32 entry = 0x8007D940u + (u32)selector * 0x154u;
#else
                u32 entry = 0x8007D940u + (u32)selector * 0xA4u;
#endif
                if (fd_lbu(entry) != 0xFFu)
                    fd_sb(D_8006F8E5 + channel, 1u);
            }
        }

#if !defined(W34N19_MUTANT_PARTY_SKIP_RECONCILE)
        wm_80075D4C();
#endif
    }
}

/* Retail 0x8007169C..0x80071774: modal START pause and controller-loss
 * recovery.  Re-read every guard for the second arm because the first modal
 * may change controller and world state before it returns. */
void wm_712d0_run_pause_lanes(void)
{
#if defined(W34N20_MUTANT_DRIVER_WRONG_D804_GUARD)
    if (fd_lw(D_8009C178) == 0u && fd_lw(D_8009D804) != 0u &&
#else
    if (fd_lw(D_8009C178) == 0u && fd_lw(D_8009D804) == 0u &&
#endif
            fd_lw(D_8009D554) != 0u && fd_lw(D_8009D80C) == 0u &&
            (fd_lhu(D_8009BD10) & 0x0800u) != 0u) {
        wm_8007634C();
    }

#if !defined(W34N20_MUTANT_DRIVER_SKIP_DISCONNECT_WAIT)
    if (fd_lw(D_8009C178) == 0u && fd_lw(D_8009D804) == 0u &&
            fd_lw(D_8009D554) != 0u && fd_lw(D_8009D80C) == 0u &&
            ControllerGetType(0) == 0) {
        wm_80076594();
    }
#endif
}

/* Retail 0x80071890..0x80071974: enter/return from modes 1..3, or request
 * the field transition for modes 4..7. */
void wm_712d0_run_menu_mode_lane(void)
{
    if (fd_lw(D_8009C178) == 0u && fd_lw(D_8009D804) != 0u &&
            fd_lw(D_8009D554) != 0u) {
        s32 mode = (s32)fd_lw(D_8009BE10);

        if (mode > 0 && mode < 4) {
            wm_800758C0();
#if defined(W34N19_MUTANT_MENU_GUEST_TWINS)
            fd_sb(0x80069460u, 0u);
            fd_sb(0x80069178u, 0u);
            fd_sb(0x80069171u, 1u);
#else
            D_80059460 = 0u;
            g_MenuDebugEnabled = 0u;
            D_80059171 = 1u;
#endif
            wm_800762FC();
            MenuMain();
            wm_800762FC();
            wm_80075B58();
        } else if (mode >= 4 && mode < 8) {
#if defined(W34N19_MUTANT_TRANSITION_WRONG_PAGE)
            u16 flags = fd_lhu(0x8007EE68u);
#else
            u16 flags = fd_lhu(D_8006EE68);
#endif
            fd_sw(D_8009D554, 0u);
            fd_sw(D_8009D7CC, 0u);
            fd_sw(D_8009D7D8, D_8009B6E4);
            flags = (u16)(flags | 0x2000u);
#if defined(W34N19_MUTANT_TRANSITION_WRONG_PAGE)
            fd_sh(0x8007EE68u, flags);
#else
            fd_sh(D_8006EE68, flags);
#endif
        }
    }
}

static void* wm_712d0_map_guest(u32 value, const char* call, u32 pc)
{
    if (value == 0u || value < 0x80000000u || value >= 0x80300000u) {
        fprintf(stderr,
                "[worldmap-safety] %s unknown guest=0x%08x at 0x%08x "
                "skip\n", call, value, pc);
        return NULL;
    }
    return PSX_ADDR(value);
}

Wm712D0RunResult wm_800712D0_run_bounded(Wm712D0BoundedRun* run)
{
    u32 draw_env_ptr;
    u32 ot_ptr;
    s32 controller_result;
    int frame;

    if (run == NULL || run->frame_limit < 0 ||
            run->displayed_frames < 0 ||
            (run->frame_limit > 0 &&
             run->displayed_frames >= run->frame_limit)) {
        return WM_712D0_RUN_ERROR;
    }

    /* Retail 0x800712D0..0x80071308: once per world session. */
    draw_env_ptr = 0x8009BC40;
    fd_sw(D_8009BE3C, draw_env_ptr);
    fd_sw(D_8009D7F0, 1);
    fd_sw(D_8009D554, 1);

    for (;;) {
        frame = run->displayed_frames + 1;
        if (run->before_frame != NULL &&
                run->before_frame(frame, run->user) != 0) {
            return WM_712D0_RUN_ERROR;
        }

        /* Retail 0x8007130C: recurring inner-frame head. */
        fd_sh(D_8009BD1C, 0);
        fd_sh(D_8009BD14, 0);
        fd_sh(D_8009CD50, 0);
        fd_sh(D_8009BD18, 0);
        fd_sh(D_8009BD10, 0);
        fd_sh(D_8009CD4C, 0);

        /* Controller polling loop */
        do {
            controller_result = ControllerPopState();
            if (controller_result != 0) {
                /* Merge controller input into global state */
                u16 buttons = fd_lhu(D_8009CD4C);
                u16 raw_btn = (u16)g_C1ButtonState;
                u16 sticks = fd_lhu(D_8009CD50);
                u16 raw_stick = (u16)g_C2ButtonState;

                fd_sh(D_8009CD4C, buttons | raw_btn);
                fd_sh(D_8009CD50, sticks | raw_stick);

                buttons = fd_lhu(D_8009BD10);
                raw_btn = (u16)g_C1ButtonStateReleased;
                sticks = fd_lhu(D_8009BD14);
                raw_stick = (u16)g_C2ButtonStateReleased;

                fd_sh(D_8009BD10, buttons | raw_btn);
                fd_sh(D_8009BD14, sticks | raw_stick);

                buttons = fd_lhu(D_8009BD18);
                raw_btn = (u16)g_C1ButtonStatePressedOnce;
                sticks = fd_lhu(D_8009BD1C);
                raw_stick = (u16)g_C2ButtonStatePressedOnce;

                fd_sh(D_8009BD18, buttons | raw_btn);
                fd_sh(D_8009BD1C, sticks | raw_stick);
            }
        } while (controller_result != 0);

        /* Test-only, env-gated world-frame schedule.  The ordinary controller
         * drain above remains authoritative; this bridge only ORs a detached
         * acceptance schedule into the same retail accumulators. */
        if (PcPort_WorldTestInputMerge != NULL) {
            u16 held = fd_lhu(D_8009CD4C);
            u16 pressed = fd_lhu(D_8009BD10);
            u16 repeated = fd_lhu(D_8009BD18);

            PcPort_WorldTestInputMerge(&held, &pressed, &repeated);
            fd_sh(D_8009CD4C, held);
            fd_sh(D_8009BD10, pressed);
            fd_sh(D_8009BD18, repeated);
        }

        wm_712d0_run_cd_sync_lane();

        /* OT pointer management */
        {
            u32 env = fd_lw(D_8009BE3C);
            u32 alt_env = 0x8009BBC8;
            if (env == alt_env) {
                alt_env += 0x78;
            }
            ot_ptr = fd_lw(alt_env + 0x70);
            fd_sw(D_8009BE3C, alt_env);

            /* Toggle double-buffer flag */
            fd_sw(D_8009D7F0, fd_lw(D_8009D7F0) < 1 ? 1 : 0);
        }

        /* Clear OT. PsyCross's ClearOTagR writes host-pointer links at its own
         * OT_TAG stride; the OT lives in guest RAM and is walked by the
         * guest-link adapter, so clear it in retail format. */
        {
            void* host_ot = wm_712d0_map_guest(ot_ptr, "ClearOTagR", 0x80071468u);
            if (host_ot != NULL)
                wm_ot_clear_r_guest(ot_ptr, 0x400u);
        }

        /* Process input */
        fd_lw(D_8009D7F0);
        func_800250E0((int)fd_lw(D_8009D7F0));

        /* Game state processing */
        func_8001D468();

        /* Scheduler */
        wm_712d0_run_second_scheduler();

        /* DrawSync */
        DrawSync(NULL);

        /* Vsync */
        Vsync(2);

        /* Soft reset check */
        GameCheckAndHandleSoftReset();

        /* Display environment */
        {
            u32 env = fd_lw(D_8009BE3C);
            void* host_env = wm_712d0_map_guest(env, "PutDrawEnv", 0x800714ACu);
            if (host_env != NULL) {
                PutDispEnv((u8*)host_env + 0x5Cu);
                PutDrawEnv(host_env);
            }
        }

        wm_712d0_run_party_refresh_lane();

        wm_712d0_run_pause_lanes();

        /* Retail 0x80071774..0x8007188C: consume a pending transition,
         * then clear its one-frame trigger and apply the bit-0x100 toggle. */
        wm_712d0_run_transition_lane();

        wm_712d0_run_menu_mode_lane();

        /* Reset boundary flag */
        fd_sw(D_8009D804, 0);

        /* Render pipeline */
        wm_80025044_guest_safe();
        (void)wm_80074F2C();
        (void)wm_80075104();

        /* Geometry offset */
        SetGeomOffset(0xA0, fd_lw(D_8009BE0C));

        /* DrawOTag */
        {
            u32 env = fd_lw(D_8009BE3C);
            u32 guest_ot = fd_lw(env + 0x70);
            void* host_ot = wm_712d0_map_guest(guest_ot + 0xFFCu,
                    "DrawOTag", 0x800719B4u);
            if (host_ot != NULL)
                (void)wm_ot_draw_otag_guest(guest_ot + 0xFFCu);
        }

        /* Retail 0x800719C8: the only recurring-frame back-edge. */
        run->displayed_frames = frame;
        if (run->after_frame != NULL &&
                run->after_frame(frame, run->user) != 0) {
            return WM_712D0_RUN_ERROR;
        }
        if (fd_lw(D_8009D554) == 0u)
            break;
        if (run->frame_limit > 0 &&
                run->displayed_frames >= run->frame_limit)
            return WM_712D0_RUN_BOUNDED_EXIT;
    }

    /* Retail 0x800719D0..0x80071A4C: natural session exit only. */
    ResetGraph(1);

    /* Final display environment */
    if (fd_lw(D_8009D7F0) == 0) {
        /* Move image for double buffering — RECT at stack */
        s16 rect[4] = {0, 0xD8, 0x140, 0xD8};
        MoveImage((void*)rect, 0, 0);
    }

    /* Final sync */
    wm_80096694();
    DrawSync(NULL);
    Vsync(0);

    /* Final display environment */
    {
        void* host_env = wm_712d0_map_guest(0x8009BC9Cu,
                "PutDispEnv", 0x80071A00u);
        if (host_env != NULL)
            PutDispEnv(host_env);
    }

    return WM_712D0_RUN_NATURAL_EXIT;
}

void wm_800712D0(void)
{
    Wm712D0BoundedRun run;

    run.frame_limit = 600;
    run.displayed_frames = 0;
    run.before_frame = NULL;
    run.after_frame = NULL;
    run.user = NULL;
    (void)wm_800712D0_run_bounded(&run);
}
