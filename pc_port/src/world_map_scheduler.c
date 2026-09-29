/*
 * World-map callback scheduler (W34B5H) — production implementation.
 *
 * Exact bounded transcription of retail 0x80097800..0x800978FB (63
 * instructions), decoded fresh in W34B5G from disc/world_map.bin
 * (sha256 4c15fd32…ac70) and mechanically re-verified in W34B5H.
 *
 * Retail shape:
 *   s1 = *(0x8009BE24); s0 = s1 + 0x4C; s2 = 0;
 *   loop @0x80097830:
 *     if (slot.+0x1C == 0) -> next                       (occupancy)
 *     state = (s16)slot.+0x00
 *     if ((u32)(s32)state >= 5) -> next                  (sltiu after lh)
 *     switch (jt_0x80070CE8[state]):
 *       0: cb = slot.+0x18; goto call
 *       1: cb = slot.+0x1C; goto call
 *       call @0x8009787C: jalr cb, a0 = slot index;
 *            slot.+0x00 = callback return               (delay-slot sh)
 *       2: if ((s16)(--slot.+0x02) <= 0) slot.+0x00 = 1
 *       3: (dormant)
 *       4: if (slot.+0x4C) func_800230A8(slot.+0x4C)
 *   next @0x800978C8: s2++; s0 += 0x80; s1 += 0x80 (delay); while s2 < 64
 *
 * Bounded dispatch frontier: guest callback addresses are NEVER cast to
 * native pointers. Resolution classes:
 *   IMPLEMENTED — a registered test body or an explicitly linked,
 *                 independently accepted production body;
 *   MISSING     — recognized guest callback with no body (the 30 distinct
 *                 callback pointers registered by the accepted Table A/B
 *                 initialization); the scheduler stops BEFORE writing the
 *                 slot state, BEFORE advancing, and the caller cuts before
 *                 DrawSync;
 *   INVALID     — any other address; bounded failure, no host call.
 */
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_callback_923a8.h"
#include "world_map_callback_925a0.h"
#include "world_map_callback_8a2c8.h"
#include "world_map_callback_8b2bc.h"
#include "world_map_callback_8bb40.h"
#include "world_map_callback_8c530.h"
#include "world_map_callback_8d3f0.h"
#include "world_map_callback_8dd6c.h"
#include "world_map_callback_8e76c.h"
#include "world_map_callback_8e190.h"
#include "world_map_callback_906e0.h"
#include "world_map_helper_907f4.h"
#include "world_map_callback_91b54.h"
#include "world_map_callback_914d0.h"
#include "world_map_callback_91c18.h"
#include "world_map_callback_92234.h"
#include "world_map_callback_922ac.h"
#include "world_map_callback_92be4.h"
#include "world_map_callback_92c70.h"
#include "world_map_callback_92fd8.h"
#include "world_map_callback_92df8.h"
#include "world_map_frame_driver_712d0.h"
#include "world_map_r4world_71a58.h"
#include "world_map_callback_87734.h"
#include "world_map_callback_7756c.h"
#include "world_map_callback_76a14.h"
#include "world_map_callback_78948.h"
#include "world_map_callback_77dc8.h"
#include "world_map_callback_78e2c.h"
#include "world_map_callback_794d8.h"
#include "world_map_callback_795e4.h"
#include "world_map_callback_7a144.h"
#include "world_map_callback_7de14.h"
#include "world_map_callback_7e450.h"
#include "world_map_callback_7eca4.h"
#include "world_map_callback_7f8ac.h"
#include "world_map_callback_7fc8c.h"
#include "world_map_callback_81174.h"
#include "world_map_callback_813e8.h"
#include "world_map_callback_817a0.h"
#include "world_map_callback_819c8.h"
#include "world_map_callback_81c3c.h"
#include "world_map_callback_81fb4.h"
#include "world_map_callback_827c8.h"
#include "world_map_callback_827ec.h"
#include "world_map_callback_83214.h"
#include "world_map_callback_834d0.h"
#include "world_map_callback_838e8.h"
#include "world_map_callback_83a00.h"
#include "world_map_callback_84068.h"
#include "world_map_callback_8b644.h"
#include "world_map_callback_8c844.h"
#include "world_map_callback_8d678.h"
#include "world_map_scheduler.h"

/* Focused legacy scheduler tests intentionally link the scheduler without
 * production callback bodies.  Weak references preserve their bounded-
 * missing behavior, while the canonical link resolves these six accepted
 * bodies without any guest-function-pointer cast. */
extern s16 wm_800923A8(int slot_index) __attribute__((weak));
extern s32 wm_800925A0(s32 slot_index) __attribute__((weak));
extern s32 wm_8008A2C8(s32 slot_index) __attribute__((weak));
extern s32 wm_8008A52C(s32 slot_index) __attribute__((weak));
extern s32 wm_8008B498(s32 slot_index) __attribute__((weak));
extern s32 wm_8008BD1C(s32 slot_index) __attribute__((weak));
extern s32 wm_8008C6EC(s32 slot_index) __attribute__((weak));
extern s32 wm_8008D520(s32 slot_index) __attribute__((weak));
extern s32 wm_8008DE9C(s32 slot_index) __attribute__((weak));
extern s32 wm_8008E4F4(s32 slot_index) __attribute__((weak));
extern s32 wm_800907C4(s32 slot_index) __attribute__((weak));
extern s32 wm_80087F60(s32 slot_index) __attribute__((weak));
extern s32 wm_8008868C(s32 slot_index) __attribute__((weak));
extern s32 wm_800879E0(s32 slot_index) __attribute__((weak));
extern s32 wm_80088C90(s32 slot_index) __attribute__((weak));

extern s32 wm_8008A72C(s32 slot_index) __attribute__((weak));
extern s32 wm_8008B2BC(s32 slot_index) __attribute__((weak));
extern s32 wm_8008B644(s32 slot_index) __attribute__((weak));
extern s32 wm_8008BB40(s32 slot_index) __attribute__((weak));
extern s32 wm_8008C530(s32 slot_index) __attribute__((weak));
extern s32 wm_8008C844(s32 slot_index) __attribute__((weak));
extern s32 wm_8008D3F0(s32 slot_index) __attribute__((weak));
extern s32 wm_8008D678(s32 slot_index) __attribute__((weak));
extern s32 wm_8008DD6C(s32 slot_index) __attribute__((weak));
extern s32 wm_8008E190(s32 slot_index) __attribute__((weak));
extern s32 wm_8008E76C(s32 slot_index) __attribute__((weak));
extern s32 wm_800906E0(s32 slot_index) __attribute__((weak));
extern s32 wm_800907F4(s32 slot_index) __attribute__((weak));
/* matched C (src/world_map/main.c) */
extern s32 func_80091430(s32 arg0) __attribute__((weak));
extern s32 wm_800914D0(s32 slot_index) __attribute__((weak));
extern s32 wm_80091B54(s32 slot_index) __attribute__((weak));
extern s32 wm_80091C18(s32 slot_index) __attribute__((weak));
extern s32 wm_80092234(s32 slot_index) __attribute__((weak));
extern s32 wm_800922AC(s32 slot_index) __attribute__((weak));
extern s32 wm_80092BE4(s32 slot_index) __attribute__((weak));
extern s32 wm_80092C70(s32 slot_index) __attribute__((weak));
extern s32 wm_80092DF8(s32 slot_index) __attribute__((weak));
extern s32 wm_80092FD8(s32 slot_index) __attribute__((weak));
/* World-map batch 3: brackets 3-7 story callbacks.  Four are matched C;
 * 0x80088570 is matched too; 0x80088720 is a stopgap hand body until it matches. */
extern s32 func_80087C6C(s32 arg0) __attribute__((weak));
extern s32 func_80087FD0(s32 arg0) __attribute__((weak));
extern s32 func_80088B40(s32 arg0) __attribute__((weak));
extern s32 func_80088D00(s32 arg0) __attribute__((weak));
extern s32 func_80088570(s32 arg0) __attribute__((weak));
extern s32 wm_80088720(s32 arg0) __attribute__((weak));
/* World-map batch 2: story-selector callbacks, matched C (src/world_map/main.c).
 * Weak: until those bodies are compiled for the port the slots stay
 * unresolved as before. */
extern s32 func_800877E0(s32 arg0) __attribute__((weak));
extern s32 func_80087804(s32 arg0) __attribute__((weak));
extern s32 func_800879A8(s32 arg0) __attribute__((weak));
extern s32 func_80087A8C(s32 arg0) __attribute__((weak));
extern s32 func_80088D64(s32 arg0) __attribute__((weak));
extern s32 func_80088DE4(s32 arg0) __attribute__((weak));
extern s32 func_80088E1C(s32 arg0) __attribute__((weak));
extern s32 func_80088E68(s32 arg0) __attribute__((weak));
extern s32 func_80088EA0(s32 arg0) __attribute__((weak));
extern s32 func_80088F1C(s32 arg0) __attribute__((weak));
extern s32 func_80088F54(void) __attribute__((weak));
extern s32 func_80088F5C(void) __attribute__((weak));
/* matched C (src/world_map/main.c) */
extern s32 func_80071A50(void) __attribute__((weak));
extern s32 wm_80071A58(s32 slot_index) __attribute__((weak));
/* matched C (src/world_map/main.c) */
extern s32 func_80087710(s32 arg0) __attribute__((weak));
extern s32 wm_80087734(s32 slot_index) __attribute__((weak));
extern s32 wm_8007756C(s32 slot_index) __attribute__((weak));
extern s32 wm_800776E0(s32 slot_index) __attribute__((weak));
/* matched C (src/world_map/main.c) */
extern s32 func_80076A14(void) __attribute__((weak));
extern s32 wm_80076A1C(s32 slot_index) __attribute__((weak));
/* matched C (src/world_map/main.c) */
extern s32 func_80078948(void) __attribute__((weak));
extern s32 wm_80078950(s32 slot_index) __attribute__((weak));
extern s32 wm_80077DC8(s32 slot_index) __attribute__((weak));
extern s32 wm_80077E68(s32 slot_index) __attribute__((weak));
extern s32 wm_8007828C(s32 slot_index) __attribute__((weak));
extern s32 wm_800783E8(s32 slot_index) __attribute__((weak));
/* matched C (src/world_map/main.c) */
extern s32 func_80078E2C(s32 arg0) __attribute__((weak));
extern s32 wm_80078EA4(s32 slot_index) __attribute__((weak));
/* matched C (src/world_map/main.c) */
extern s32 func_800794D8(s32 arg0) __attribute__((weak));
/* matched C (src/world_map/main.c) */
extern s32 func_80079538(s32 arg0) __attribute__((weak));
extern s32 wm_800795E4(s32 slot_index) __attribute__((weak));
extern s32 wm_80079778(s32 slot_index) __attribute__((weak));
/* matched C (src/world_map/main.c) */
extern s32 func_8007A410(s32 arg0) __attribute__((weak));
extern s32 wm_8007A430(s32 slot_index) __attribute__((weak));
/* matched C (src/world_map/main.c) */
extern s32 func_8007A568(void) __attribute__((weak));
/* matched C (src/world_map/main.c) */
extern s32 func_8007A570(s32 arg0) __attribute__((weak));
extern s32 wm_8007A144(s32 slot_index) __attribute__((weak));
extern s32 wm_8007A1B4(s32 slot_index) __attribute__((weak));
extern s32 wm_8007A9B4(s32 slot_index) __attribute__((weak));
extern s32 wm_8007A9F8(s32 slot_index) __attribute__((weak));
extern s32 wm_8007AD34(s32 slot_index) __attribute__((weak));
extern s32 wm_8007ADD4(s32 slot_index) __attribute__((weak));
/* matched C (src/world_map/main.c) */
extern s32 func_8007BA08(void) __attribute__((weak));
extern s32 wm_8007BA10(s32 slot_index) __attribute__((weak));
/* matched C (src/world_map/main.c) */
extern s32 func_8007BB60(s32 arg0) __attribute__((weak));
extern s32 wm_8007BBEC(s32 slot_index) __attribute__((weak));
extern s32 wm_8007B200(s32 slot_index) __attribute__((weak));
extern s32 wm_8007B394(s32 slot_index) __attribute__((weak));
extern s32 wm_8007B604(s32 slot_index) __attribute__((weak));
extern s32 wm_8007B798(s32 slot_index) __attribute__((weak));
/* matched C (src/world_map/main.c) */
extern s32 func_8007D600(s32 arg0) __attribute__((weak));
/* matched C (src/world_map/main.c) */
extern s32 func_8007D690(s32 arg0) __attribute__((weak));
/* matched C (src/world_map/main.c) */
extern s32 func_8007D774(s32 arg0) __attribute__((weak));
extern s32 wm_8007D7FC(s32 slot_index) __attribute__((weak));
/* matched C (src/world_map/main.c) */
extern s32 func_8007D078(s32 arg0) __attribute__((weak));
extern s32 wm_8007D110(s32 slot_index) __attribute__((weak));
/* matched C (src/world_map/main.c) */
extern s32 func_8007D228(s32 arg0) __attribute__((weak));
extern s32 wm_8007D2B8(s32 slot_index) __attribute__((weak));
/* matched C (src/world_map/main.c) */
extern s32 func_8007D414(s32 arg0) __attribute__((weak));
extern s32 wm_8007D4A4(s32 slot_index) __attribute__((weak));
/* matched C (src/world_map/main.c) */
extern s32 func_8007CE84(s32 arg0) __attribute__((weak));
extern s32 wm_8007CF18(s32 slot_index) __attribute__((weak));
/* matched C (src/world_map/main.c) */
extern s32 func_8007CC6C(s32 arg0) __attribute__((weak));
extern s32 wm_8007CD20(s32 slot_index) __attribute__((weak));
extern s32 wm_8007C36C(s32 slot_index) __attribute__((weak));
extern s32 wm_8007C3B8(s32 slot_index) __attribute__((weak));
extern s32 wm_8007C724(s32 slot_index) __attribute__((weak));
extern s32 wm_8007C7D8(s32 slot_index) __attribute__((weak));
extern s32 wm_8007DE14(s32 slot_index) __attribute__((weak));
extern s32 wm_8007DE98(s32 slot_index) __attribute__((weak));
extern s32 wm_8007E450(s32 slot_index) __attribute__((weak));
extern s32 wm_8007E4E4(s32 slot_index) __attribute__((weak));
extern s32 wm_8007ECA4(s32 slot_index) __attribute__((weak));
extern s32 wm_8007EE34(s32 slot_index) __attribute__((weak));
extern s32 wm_8007F8AC(s32 slot_index) __attribute__((weak));
extern s32 wm_8007F968(s32 slot_index) __attribute__((weak));
extern s32 wm_8007FC8C(s32 slot_index) __attribute__((weak));
extern s32 wm_8007FD30(s32 slot_index) __attribute__((weak));
/* matched C (src/world_map/main.c) */
extern s32 func_80080900(s32 arg0) __attribute__((weak));
/* matched C (src/world_map/main.c) */
extern s32 func_80080944(s32 arg0) __attribute__((weak));
extern s32 wm_8008032C(s32 slot_index) __attribute__((weak));
extern s32 wm_80080370(s32 slot_index) __attribute__((weak));
extern s32 wm_80080A28(s32 slot_index) __attribute__((weak));
extern s32 wm_80080AC4(s32 slot_index) __attribute__((weak));
extern s32 wm_80080578(s32 slot_index) __attribute__((weak));
extern s32 wm_80080600(s32 slot_index) __attribute__((weak));
extern s32 wm_80081174(s32 slot_index) __attribute__((weak));
extern s32 wm_800811C0(s32 slot_index) __attribute__((weak));
extern s32 wm_800813E8(s32 slot_index) __attribute__((weak));
extern s32 wm_80081470(s32 slot_index) __attribute__((weak));
extern s32 wm_800817A0(s32 slot_index) __attribute__((weak));
extern s32 wm_80081868(s32 slot_index) __attribute__((weak));
extern s32 wm_800819C8(s32 slot_index) __attribute__((weak));
extern s32 wm_80081B24(s32 slot_index) __attribute__((weak));
extern s32 wm_80081C3C(s32 slot_index) __attribute__((weak));
extern s32 wm_80081D80(s32 slot_index) __attribute__((weak));
/* matched C (src/world_map/main.c) */
extern s32 func_80081FB4(s32 arg0) __attribute__((weak));
extern s32 wm_80081FD8(s32 slot_index) __attribute__((weak));
extern s32 wm_800827C8(s32 slot_index) __attribute__((weak));
extern s32 wm_80076B34(s32 slot_index) __attribute__((weak));
extern s32 wm_800827EC(s32 slot_index) __attribute__((weak));
extern s32 wm_800828DC(s32 slot_index) __attribute__((weak));
extern s32 wm_80083214(s32 slot_index) __attribute__((weak));
extern s32 wm_80083264(s32 slot_index) __attribute__((weak));
/* matched C (src/world_map/main.c) */
extern s32 func_800834D0(void) __attribute__((weak));
extern s32 wm_800834D8(s32 slot_index) __attribute__((weak));
extern s32 wm_800838E8(s32 slot_index) __attribute__((weak));
extern s32 wm_8008390C(s32 slot_index) __attribute__((weak));
extern s32 wm_80083FE4(s32 slot_index) __attribute__((weak));
extern s32 wm_80083A00(s32 slot_index) __attribute__((weak));
extern s32 wm_80084068(s32 slot_index) __attribute__((weak));

#define WM_SCHED_RAM(a) ((u8*)PSX_ADDR(a))

/* Recognized callback addresses from the W34B5G natural pool capture.
 * Linked accepted bodies are resolved before this missing fallback:
 * 16 occupied slots x {+0x18, +0x1C}; duplicates 0x8008B644/0x8008D678
 * collapsed — 30 distinct addresses). */
static const u32 s_wm_sched_known_missing[] = {
    0x800923A8u, 0x800925A0u, /* slot 0  (Table A record 0)  */
    0x8008A2C8u, 0x8008A72Cu, /* slot 1  */
    0x8008B2BCu, 0x8008B644u, /* slot 2  */
    0x8008BB40u,              /* slot 3 cb0 (cb1 = 0x8008B644) */
    0x8008C530u, 0x8008C844u, /* slot 4  */
    0x8008D3F0u, 0x8008D678u, /* slot 5  */
    0x8008DD6Cu,              /* slot 6 cb0 (cb1 = 0x8008D678) */
    0x8008E190u, 0x8008E76Cu, /* slot 7  */
    0x800906E0u, 0x800907F4u, /* slot 8  */
    0x80091430u, 0x800914D0u, /* slot 9  */
    0x80091B54u, 0x80091C18u, /* slot 10 */
    0x80092234u, 0x800922ACu, /* slot 11 */
    0x80092BE4u, 0x80092C70u, /* slot 12 */
    0x80092DF8u, 0x80092FD8u, /* slot 13 */
    0x80071A50u, 0x80071A58u, /* slot 14 */
    0x80087710u, 0x80087734u, /* slot 15 (Table B, selector 0) */
    0x8007756Cu, 0x800776E0u, /* mode 8/11 camera slot */
    0x80076A14u, 0x80076A1Cu, /* shared mode-13 draw slot */
    0x80078948u, 0x80078950u, /* shared mode draw slot */
    0x80077DC8u, 0x80077E68u, /* mode 9 path/camera slot */
    0x8007828Cu, 0x800783E8u, /* mode 9 context slot */
    0x80078E2Cu, 0x80078EA4u, /* mode 10 camera/event slot */
    0x800794D8u, 0x80079538u, /* mode 10 orbit/context slot */
    0x800795E4u, 0x80079778u, /* mode 10 sequence/context slot */
    0x8007A410u, 0x8007A430u, /* mode 10 timed marker slot */
    0x8007A568u, 0x8007A570u, /* mode 10 marker initializer slot */
    0x8007A144u, 0x8007A1B4u, /* mode 10 scaled-stream slot */
    0x8007A9B4u, 0x8007A9F8u, /* mode 14 scripted-sequence slot */
    0x8007AD34u, 0x8007ADD4u, /* mode 14 camera-control slot */
    0x8007BA08u, 0x8007BA10u, /* mode 14 timed-marker slot */
    0x8007BB60u, 0x8007BBECu, /* mode 14 path-camera slot */
    0x8007B200u, 0x8007B394u, /* mode 14 first scaled-object slot */
    0x8007B604u, 0x8007B798u, /* mode 14 second scaled-object slot */
    0x8007D600u, 0x8007D690u, /* mode 12 moving-marker slot */
    0x8007D774u, 0x8007D7FCu, /* mode 12 moving-target slot */
    0x8007D078u, 0x8007D110u, /* mode 12 linked-marker slot */
    0x8007D228u, 0x8007D2B8u, /* mode 12 linked-marker-B slot */
    0x8007D414u, 0x8007D4A4u, /* mode 12 linked-marker-C slot */
    0x8007CE84u, 0x8007CF18u, /* mode 12 dual-marker slot */
    0x8007CC6Cu, 0x8007CD20u, /* mode 12 four-link slot */
    0x8007C36Cu, 0x8007C3B8u, /* mode 12 scripted-control slot */
    0x8007C724u, 0x8007C7D8u, /* mode 12 camera/control slot */
    0x8007DE14u, 0x8007DE98u, /* mode 15 scripted-control slot */
    0x8007E450u, 0x8007E4E4u, /* mode 15 camera/control slot */
    0x8007ECA4u, 0x8007EE34u, /* mode 15 moving-object slot */
    0x8007F8ACu, 0x8007F968u, /* mode 15 trail slots */
    0x8007FC8Cu, 0x8007FD30u, /* mode 15 dual scaled-stream slot */
    0x80080900u, 0x80080944u, /* mode 13 marker slot */
    0x8008032Cu, 0x80080370u, /* mode 13 scripted-control slot */
    0x80080A28u, 0x80080AC4u, /* mode 13 dual scaled-stream slot */
    0x80080578u, 0x80080600u, /* mode 13 camera/control slot */
    0x80081174u, 0x800811C0u, /* mode 16 scripted-control slot */
    0x800813E8u, 0x80081470u, /* mode 16 camera-control slot */
};
#define WM_SCHED_KNOWN_MISSING_COUNT \
    (sizeof(s_wm_sched_known_missing) / sizeof(s_wm_sched_known_missing[0]))

/* Test-only/synthetic callback registry. Production bodies use the explicit
 * built-in resolution below. */
#define WM_SCHED_REGISTRY_MAX 32
static struct {
    u32 guest_addr;
    wm_sched_callback_fn fn;
} s_wm_sched_registry[WM_SCHED_REGISTRY_MAX];
static int s_wm_sched_registry_count;

static wm_sched_destructor_fn s_wm_sched_test_destructor;

/* Instrumentation (per world-init lifecycle; see wm_sched_reset). */
static int s_entry;
static int s_slots_inspected;
static int s_occupied_inspected;
static int s_state_seen[5];
static int s_state_invalid_seen;
static int s_dispatch_attempts;
static int s_callbacks_executed;
static int s_missing_hits;
static int s_invalid_hits;
static int s_destructor_calls;
static int s_destructor_boundary_hits;
static int s_completed_passes;
static int s_last_slot;
static u32 s_last_callback;
static int s_last_callback_state;
static int s_outcome = WM_SCHED_PASS_COMPLETE;
static u32 s_frontier_pc = WM_SCHED_CUT_BEFORE_DRAWSYNC;

typedef struct {
    u32 guest_addr;
    unsigned count;
} wm_sched_stub_hit;

static wm_sched_stub_hit s_stub_hits[64];
static unsigned s_stub_hit_count;

static void wm_sched_log_stub(u32 guest_addr, const char* kind,
                              int slot_index, int state)
{
    unsigned i;
    for (i = 0; i < s_stub_hit_count; i++) {
        if (s_stub_hits[i].guest_addr == guest_addr) {
            s_stub_hits[i].count++;
            fprintf(stderr,
                    "[worldmap-stub] guest=0x%08x kind=%s count=%u "
                    "slot=%d state=%d default_return=0\n",
                    guest_addr, kind, s_stub_hits[i].count,
                    slot_index, state);
            return;
        }
    }
    if (s_stub_hit_count < sizeof(s_stub_hits) / sizeof(s_stub_hits[0])) {
        s_stub_hits[s_stub_hit_count].guest_addr = guest_addr;
        s_stub_hits[s_stub_hit_count].count = 1;
        s_stub_hit_count++;
    }
    fprintf(stderr,
            "[worldmap-stub] guest=0x%08x kind=%s count=1 slot=%d "
            "state=%d default_return=0\n",
            guest_addr, kind, slot_index, state);
}

void wm_sched_reset(void)
{
    s_entry = 0;
    s_slots_inspected = 0;
    s_occupied_inspected = 0;
    memset(s_state_seen, 0, sizeof(s_state_seen));
    s_state_invalid_seen = 0;
    s_dispatch_attempts = 0;
    s_callbacks_executed = 0;
    s_missing_hits = 0;
    s_invalid_hits = 0;
    s_destructor_calls = 0;
    s_destructor_boundary_hits = 0;
    s_completed_passes = 0;
    s_last_slot = -1;
    s_last_callback = 0;
    s_last_callback_state = -1;
    s_outcome = WM_SCHED_PASS_COMPLETE;
    s_frontier_pc = WM_SCHED_CUT_BEFORE_DRAWSYNC;
    s_stub_hit_count = 0;
    memset(s_stub_hits, 0, sizeof(s_stub_hits));
}

void wm_sched_callback_register(u32 guest_addr, wm_sched_callback_fn fn)
{
    if (s_wm_sched_registry_count >= WM_SCHED_REGISTRY_MAX)
        return;
    s_wm_sched_registry[s_wm_sched_registry_count].guest_addr = guest_addr;
    s_wm_sched_registry[s_wm_sched_registry_count].fn = fn;
    s_wm_sched_registry_count++;
}

void wm_sched_callback_registry_clear(void)
{
    s_wm_sched_registry_count = 0;
    s_wm_sched_test_destructor = 0;
}

void wm_sched_test_set_destructor(wm_sched_destructor_fn fn)
{
    s_wm_sched_test_destructor = fn;
}

static wm_sched_callback_fn wm_sched_lookup(u32 guest_addr)
{
    int i;
    for (i = 0; i < s_wm_sched_registry_count; i++)
        if (s_wm_sched_registry[i].guest_addr == guest_addr)
            return s_wm_sched_registry[i].fn;
    return 0;
}

static s16 wm_sched_builtin_8008A52C(int slot_index)
{
    return (s16)wm_8008A52C((s32)slot_index);
}

static s16 wm_sched_builtin_8008B498(int slot_index)
{
    return (s16)wm_8008B498((s32)slot_index);
}

static s16 wm_sched_builtin_8008BD1C(int slot_index)
{
    return (s16)wm_8008BD1C((s32)slot_index);
}

static s16 wm_sched_builtin_8008C6EC(int slot_index)
{
    return (s16)wm_8008C6EC((s32)slot_index);
}

static s16 wm_sched_builtin_8008D520(int slot_index)
{
    return (s16)wm_8008D520((s32)slot_index);
}

static s16 wm_sched_builtin_8008DE9C(int slot_index)
{
    return (s16)wm_8008DE9C((s32)slot_index);
}

static s16 wm_sched_builtin_8008E4F4(int slot_index)
{
    return (s16)wm_8008E4F4((s32)slot_index);
}

static s16 wm_sched_builtin_800907C4(int slot_index)
{
    return (s16)wm_800907C4((s32)slot_index);
}

static s16 wm_sched_builtin_80087F60(int slot_index)
{
    return (s16)wm_80087F60((s32)slot_index);
}

static s16 wm_sched_builtin_8008868C(int slot_index)
{
    return (s16)wm_8008868C((s32)slot_index);
}

static s16 wm_sched_builtin_800879E0(int slot_index)
{
    return (s16)wm_800879E0((s32)slot_index);
}

static s16 wm_sched_builtin_80088C90(int slot_index)
{
    return (s16)wm_80088C90((s32)slot_index);
}

static s16 wm_sched_builtin_8008A2C8(int slot_index)
{
    return (s16)wm_8008A2C8((s32)slot_index);
}

/* Slot 0 Table-A cb1. */
static s16 wm_sched_builtin_800925A0(int slot_index)
{
    return (s16)wm_800925A0((s32)slot_index);
}

static s16 wm_sched_builtin_8008A72C(int slot_index)
{
    return (s16)wm_8008A72C((s32)slot_index);
}

static s16 wm_sched_builtin_8008B2BC(int slot_index)
{
    return (s16)wm_8008B2BC((s32)slot_index);
}

static s16 wm_sched_builtin_8008B644(int slot_index)
{
    return (s16)wm_8008B644((s32)slot_index);
}

static s16 wm_sched_builtin_8008BB40(int slot_index)
{
    return (s16)wm_8008BB40((s32)slot_index);
}

static s16 wm_sched_builtin_8008C530(int slot_index)
{
    return (s16)wm_8008C530((s32)slot_index);
}

static s16 wm_sched_builtin_8008C844(int slot_index)
{
    return (s16)wm_8008C844((s32)slot_index);
}

static s16 wm_sched_builtin_8008D3F0(int slot_index)
{
    return (s16)wm_8008D3F0((s32)slot_index);
}

static s16 wm_sched_builtin_8008D678(int slot_index)
{
    return (s16)wm_8008D678((s32)slot_index);
}

static s16 wm_sched_builtin_8008DD6C(int slot_index)
{
    return (s16)wm_8008DD6C((s32)slot_index);
}

static s16 wm_sched_builtin_8008E76C(int slot_index)
{
    return (s16)wm_8008E76C((s32)slot_index);
}

static s16 wm_sched_builtin_8008E190(int slot_index)
{
    return (s16)wm_8008E190((s32)slot_index);
}

static s16 wm_sched_builtin_800906E0(int slot_index)
{
    return (s16)wm_800906E0((s32)slot_index);
}

/* Slot 8 Table-A cb1. */
static s16 wm_sched_builtin_800907F4(int slot_index)
{
    return (s16)wm_800907F4((s32)slot_index);
}

static s16 wm_sched_builtin_80091430(int slot_index)
{
    return (s16)func_80091430((s32)slot_index);
}

static s16 wm_sched_builtin_800914D0(int slot_index)
{
    return (s16)wm_800914D0((s32)slot_index);
}

static s16 wm_sched_builtin_80091B54(int slot_index)
{
    return (s16)wm_80091B54((s32)slot_index);
}

static s16 wm_sched_builtin_80091C18(int slot_index)
{
    return (s16)wm_80091C18((s32)slot_index);
}

static s16 wm_sched_builtin_80092234(int slot_index)
{
    return (s16)wm_80092234((s32)slot_index);
}

static s16 wm_sched_builtin_800922AC(int slot_index)
{
    return (s16)wm_800922AC((s32)slot_index);
}

static s16 wm_sched_builtin_80092BE4(int slot_index)
{
    return (s16)wm_80092BE4((s32)slot_index);
}

static s16 wm_sched_builtin_80092C70(int slot_index)
{
    return (s16)wm_80092C70((s32)slot_index);
}

static s16 wm_sched_builtin_80092DF8(int slot_index)
{
    return (s16)wm_80092DF8((s32)slot_index);
}

static s16 wm_sched_builtin_80092FD8(int slot_index)
{
    return (s16)wm_80092FD8((s32)slot_index);
}

/* Slot 14 Table-A cb0. Retail 0x80071A50 is a two-instruction return-1 leaf;
 * its cb1 partner 0x80071A58 remains unresolved by design. */
static s16 wm_sched_builtin_80071A50(int slot_index)
{
    (void)slot_index;
    return (s16)func_80071A50();
}

static s16 wm_sched_builtin_80071A58(int slot_index)
{
    return (s16)wm_80071A58((s32)slot_index);
}

/* Slot 15 Table-B cb0. Twin 0x800877E0 remains a distinct unresolved
 * Table-B stream and is not aliased to this cb1. */
static s16 wm_sched_builtin_80087710(int slot_index)
{
    return (s16)func_80087710((s32)slot_index);
}

static s16 wm_sched_builtin_80087734(int slot_index)
{
    return (s16)wm_80087734((s32)slot_index);
}

static s16 wm_sched_builtin_8007756C(int slot_index)
{
    return (s16)wm_8007756C((s32)slot_index);
}

static s16 wm_sched_builtin_800776E0(int slot_index)
{
    return (s16)wm_800776E0((s32)slot_index);
}

static s16 wm_sched_builtin_80078948(int slot_index)
{
    (void)slot_index;
    return (s16)func_80078948();
}

static s16 wm_sched_builtin_80078950(int slot_index)
{
    return (s16)wm_80078950((s32)slot_index);
}

static s16 wm_sched_builtin_80077DC8(int slot_index)
{
    return (s16)wm_80077DC8((s32)slot_index);
}

static s16 wm_sched_builtin_80077E68(int slot_index)
{
    return (s16)wm_80077E68((s32)slot_index);
}

static s16 wm_sched_builtin_8007828C(int slot_index)
{
    return (s16)wm_8007828C((s32)slot_index);
}

static s16 wm_sched_builtin_800783E8(int slot_index)
{
    return (s16)wm_800783E8((s32)slot_index);
}

static s16 wm_sched_builtin_80078E2C(int slot_index)
{
    return (s16)func_80078E2C((s32)slot_index);
}

static s16 wm_sched_builtin_80078EA4(int slot_index)
{
    return (s16)wm_80078EA4((s32)slot_index);
}

static s16 wm_sched_builtin_800794D8(int slot_index)
{
    return (s16)func_800794D8((s32)slot_index);
}

static s16 wm_sched_builtin_80079538(int slot_index)
{
    return (s16)func_80079538((s32)slot_index);
}

static s16 wm_sched_builtin_800795E4(int slot_index)
{
    return (s16)wm_800795E4((s32)slot_index);
}

static s16 wm_sched_builtin_80079778(int slot_index)
{
    return (s16)wm_80079778((s32)slot_index);
}

static s16 wm_sched_builtin_8007A410(int slot_index)
{
    return (s16)func_8007A410((s32)slot_index);
}

static s16 wm_sched_builtin_8007A430(int slot_index)
{
    return (s16)wm_8007A430((s32)slot_index);
}

static s16 wm_sched_builtin_8007A568(int slot_index)
{
    (void)slot_index;
    return (s16)func_8007A568();
}

static s16 wm_sched_builtin_8007A570(int slot_index)
{
    return (s16)func_8007A570((s32)slot_index);
}

static s16 wm_sched_builtin_8007A144(int slot_index)
{
    return (s16)wm_8007A144((s32)slot_index);
}

static s16 wm_sched_builtin_8007A1B4(int slot_index)
{
    return (s16)wm_8007A1B4((s32)slot_index);
}

static s16 wm_sched_builtin_8007A9B4(int slot_index)
{
    return (s16)wm_8007A9B4((s32)slot_index);
}

static s16 wm_sched_builtin_8007A9F8(int slot_index)
{
    return (s16)wm_8007A9F8((s32)slot_index);
}

static s16 wm_sched_builtin_8007AD34(int slot_index)
{
    return (s16)wm_8007AD34((s32)slot_index);
}

static s16 wm_sched_builtin_8007ADD4(int slot_index)
{
    return (s16)wm_8007ADD4((s32)slot_index);
}

static s16 wm_sched_builtin_8007BA08(int slot_index)
{
    (void)slot_index;
    return (s16)func_8007BA08();
}

static s16 wm_sched_builtin_8007BA10(int slot_index)
{
    return (s16)wm_8007BA10((s32)slot_index);
}

static s16 wm_sched_builtin_8007BB60(int slot_index)
{
    return (s16)func_8007BB60((s32)slot_index);
}

static s16 wm_sched_builtin_8007BBEC(int slot_index)
{
    return (s16)wm_8007BBEC((s32)slot_index);
}

static s16 wm_sched_builtin_8007B200(int slot_index)
{
    return (s16)wm_8007B200((s32)slot_index);
}

static s16 wm_sched_builtin_8007B394(int slot_index)
{
    return (s16)wm_8007B394((s32)slot_index);
}

static s16 wm_sched_builtin_8007B604(int slot_index)
{
    return (s16)wm_8007B604((s32)slot_index);
}

static s16 wm_sched_builtin_8007B798(int slot_index)
{
    return (s16)wm_8007B798((s32)slot_index);
}

static s16 wm_sched_builtin_8007D600(int slot_index)
{
    return (s16)func_8007D600((s32)slot_index);
}

static s16 wm_sched_builtin_8007D690(int slot_index)
{
    return (s16)func_8007D690((s32)slot_index);
}

static s16 wm_sched_builtin_8007D774(int slot_index)
{
    return (s16)func_8007D774((s32)slot_index);
}

static s16 wm_sched_builtin_8007D7FC(int slot_index)
{
    return (s16)wm_8007D7FC((s32)slot_index);
}

static s16 wm_sched_builtin_8007D078(int slot_index)
{
    return (s16)func_8007D078((s32)slot_index);
}

static s16 wm_sched_builtin_8007D110(int slot_index)
{
    return (s16)wm_8007D110((s32)slot_index);
}

static s16 wm_sched_builtin_80076A14(int slot_index)
{
    (void)slot_index;
    return (s16)func_80076A14();
}

static s16 wm_sched_builtin_80076A1C(int slot_index)
{
    return (s16)wm_80076A1C((s32)slot_index);
}

static s16 wm_sched_builtin_8007D228(int slot_index)
{
    return (s16)func_8007D228((s32)slot_index);
}

static s16 wm_sched_builtin_8007D2B8(int slot_index)
{
    return (s16)wm_8007D2B8((s32)slot_index);
}

static s16 wm_sched_builtin_8007D414(int slot_index)
{
    return (s16)func_8007D414((s32)slot_index);
}

static s16 wm_sched_builtin_8007D4A4(int slot_index)
{
    return (s16)wm_8007D4A4((s32)slot_index);
}

static s16 wm_sched_builtin_8007CE84(int slot_index)
{
    return (s16)func_8007CE84((s32)slot_index);
}

static s16 wm_sched_builtin_8007CF18(int slot_index)
{
    return (s16)wm_8007CF18((s32)slot_index);
}

static s16 wm_sched_builtin_8007CC6C(int slot_index)
{
    return (s16)func_8007CC6C((s32)slot_index);
}

static s16 wm_sched_builtin_8007CD20(int slot_index)
{
    return (s16)wm_8007CD20((s32)slot_index);
}

static s16 wm_sched_builtin_8007C36C(int slot_index)
{
    return (s16)wm_8007C36C((s32)slot_index);
}

static s16 wm_sched_builtin_8007C3B8(int slot_index)
{
    return (s16)wm_8007C3B8((s32)slot_index);
}

static s16 wm_sched_builtin_8007C724(int slot_index)
{
    return (s16)wm_8007C724((s32)slot_index);
}

static s16 wm_sched_builtin_8007C7D8(int slot_index)
{
    return (s16)wm_8007C7D8((s32)slot_index);
}

static s16 wm_sched_builtin_8007DE14(int slot_index)
{
    return (s16)wm_8007DE14((s32)slot_index);
}

static s16 wm_sched_builtin_8007DE98(int slot_index)
{
    return (s16)wm_8007DE98((s32)slot_index);
}

static s16 wm_sched_builtin_8007E450(int slot_index)
{
    return (s16)wm_8007E450((s32)slot_index);
}

static s16 wm_sched_builtin_8007E4E4(int slot_index)
{
    return (s16)wm_8007E4E4((s32)slot_index);
}

static s16 wm_sched_builtin_8007ECA4(int slot_index)
{
    return (s16)wm_8007ECA4((s32)slot_index);
}

static s16 wm_sched_builtin_8007EE34(int slot_index)
{
    return (s16)wm_8007EE34((s32)slot_index);
}

static s16 wm_sched_builtin_8007F8AC(int slot_index)
{
    return (s16)wm_8007F8AC((s32)slot_index);
}

static s16 wm_sched_builtin_8007F968(int slot_index)
{
    return (s16)wm_8007F968((s32)slot_index);
}

static s16 wm_sched_builtin_8007FC8C(int slot_index)
{
    return (s16)wm_8007FC8C((s32)slot_index);
}

static s16 wm_sched_builtin_8007FD30(int slot_index)
{
    return (s16)wm_8007FD30((s32)slot_index);
}

static s16 wm_sched_builtin_80080900(int slot_index)
{
    return (s16)func_80080900((s32)slot_index);
}

static s16 wm_sched_builtin_80080944(int slot_index)
{
    return (s16)func_80080944((s32)slot_index);
}

static s16 wm_sched_builtin_8008032C(int slot_index)
{
    return (s16)wm_8008032C((s32)slot_index);
}

static s16 wm_sched_builtin_80080370(int slot_index)
{
    return (s16)wm_80080370((s32)slot_index);
}

static s16 wm_sched_builtin_80080A28(int slot_index)
{
    return (s16)wm_80080A28((s32)slot_index);
}

static s16 wm_sched_builtin_80080AC4(int slot_index)
{
    return (s16)wm_80080AC4((s32)slot_index);
}

static s16 wm_sched_builtin_80080578(int slot_index)
{
    return (s16)wm_80080578((s32)slot_index);
}

static s16 wm_sched_builtin_80080600(int slot_index)
{
    return (s16)wm_80080600((s32)slot_index);
}

static s16 wm_sched_builtin_80081174(int slot_index)
{
    return (s16)wm_80081174((s32)slot_index);
}

static s16 wm_sched_builtin_800811C0(int slot_index)
{
    return (s16)wm_800811C0((s32)slot_index);
}

static s16 wm_sched_builtin_800813E8(int slot_index)
{
    return (s16)wm_800813E8((s32)slot_index);
}

static s16 wm_sched_builtin_80081470(int slot_index)
{
    return (s16)wm_80081470((s32)slot_index);
}

static s16 wm_sched_builtin_800817A0(int slot_index)
{
    return (s16)wm_800817A0((s32)slot_index);
}

static s16 wm_sched_builtin_80081868(int slot_index)
{
    return (s16)wm_80081868((s32)slot_index);
}

static s16 wm_sched_builtin_800819C8(int slot_index)
{
    return (s16)wm_800819C8((s32)slot_index);
}

static s16 wm_sched_builtin_80081B24(int slot_index)
{
    return (s16)wm_80081B24((s32)slot_index);
}

static s16 wm_sched_builtin_80081C3C(int slot_index)
{
    return (s16)wm_80081C3C((s32)slot_index);
}

static s16 wm_sched_builtin_80081D80(int slot_index)
{
    return (s16)wm_80081D80((s32)slot_index);
}

static s16 wm_sched_builtin_80081FB4(int slot_index)
{
    return (s16)func_80081FB4((s32)slot_index);
}

static s16 wm_sched_builtin_80081FD8(int slot_index)
{
    return (s16)wm_80081FD8((s32)slot_index);
}

static s16 wm_sched_builtin_800827C8(int slot_index)
{
    return (s16)wm_800827C8((s32)slot_index);
}

static s16 wm_sched_builtin_80076B34(int slot_index)
{
    return (s16)wm_80076B34((s32)slot_index);
}

static s16 wm_sched_builtin_800827EC(int slot_index)
{
    return (s16)wm_800827EC((s32)slot_index);
}

static s16 wm_sched_builtin_800828DC(int slot_index)
{
    return (s16)wm_800828DC((s32)slot_index);
}

static s16 wm_sched_builtin_80083214(int slot_index)
{
    return (s16)wm_80083214((s32)slot_index);
}

static s16 wm_sched_builtin_80083264(int slot_index)
{
    return (s16)wm_80083264((s32)slot_index);
}

static s16 wm_sched_builtin_800877E0(int slot_index)
{
    return (s16)func_800877E0((s32)slot_index);
}

static s16 wm_sched_builtin_80087804(int slot_index)
{
    return (s16)func_80087804((s32)slot_index);
}

static s16 wm_sched_builtin_800879A8(int slot_index)
{
    return (s16)func_800879A8((s32)slot_index);
}

static s16 wm_sched_builtin_80087A8C(int slot_index)
{
    return (s16)func_80087A8C((s32)slot_index);
}

static s16 wm_sched_builtin_80088D64(int slot_index)
{
    return (s16)func_80088D64((s32)slot_index);
}

static s16 wm_sched_builtin_80088DE4(int slot_index)
{
    return (s16)func_80088DE4((s32)slot_index);
}

static s16 wm_sched_builtin_80088E1C(int slot_index)
{
    return (s16)func_80088E1C((s32)slot_index);
}

static s16 wm_sched_builtin_80088E68(int slot_index)
{
    return (s16)func_80088E68((s32)slot_index);
}

static s16 wm_sched_builtin_80088EA0(int slot_index)
{
    return (s16)func_80088EA0((s32)slot_index);
}

static s16 wm_sched_builtin_80088F1C(int slot_index)
{
    return (s16)func_80088F1C((s32)slot_index);
}

static s16 wm_sched_builtin_80088F54(int slot_index)
{
    (void)slot_index;
    return (s16)func_80088F54();
}

static s16 wm_sched_builtin_80088F5C(int slot_index)
{
    (void)slot_index;
    return (s16)func_80088F5C();
}

static s16 wm_sched_builtin_80087C6C(int slot_index)
{
    return (s16)func_80087C6C((s32)slot_index);
}

static s16 wm_sched_builtin_80087FD0(int slot_index)
{
    return (s16)func_80087FD0((s32)slot_index);
}

static s16 wm_sched_builtin_80088B40(int slot_index)
{
    return (s16)func_80088B40((s32)slot_index);
}

static s16 wm_sched_builtin_80088D00(int slot_index)
{
    return (s16)func_80088D00((s32)slot_index);
}

static s16 wm_sched_builtin_80088570(int slot_index)
{
    return (s16)func_80088570((s32)slot_index);
}

static s16 wm_sched_builtin_80088720(int slot_index)
{
    return (s16)wm_80088720((s32)slot_index);
}

static s16 wm_sched_builtin_800834D0(int slot_index)
{
    (void)slot_index;
    return (s16)func_800834D0();
}

static s16 wm_sched_builtin_800834D8(int slot_index)
{
    return (s16)wm_800834D8((s32)slot_index);
}

static s16 wm_sched_builtin_800838E8(int slot_index)
{
    return (s16)wm_800838E8((s32)slot_index);
}

static s16 wm_sched_builtin_8008390C(int slot_index)
{
    return (s16)wm_8008390C((s32)slot_index);
}

static s16 wm_sched_builtin_80083FE4(int slot_index)
{
    return (s16)wm_80083FE4((s32)slot_index);
}

static s16 wm_sched_builtin_80083A00(int slot_index)
{
    return (s16)wm_80083A00((s32)slot_index);
}

static s16 wm_sched_builtin_80084068(int slot_index)
{
    return (s16)wm_80084068((s32)slot_index);
}

static int wm_sched_is_known_missing(u32 guest_addr)
{
    unsigned i;
    for (i = 0; i < WM_SCHED_KNOWN_MISSING_COUNT; i++)
        if (s_wm_sched_known_missing[i] == guest_addr)
            return 1;
    return 0;
}

/* Bounded guest-callback resolver. No guest address is ever called through
 * a native pointer. */
static wm_sched_cb_resolve_t wm_sched_resolve(u32 guest_addr,
                                              wm_sched_callback_fn* out_fn)
{
    wm_sched_callback_fn fn = wm_sched_lookup(guest_addr);
    if (fn != 0) {
        *out_fn = fn;
        return WM_SCHED_CB_IMPLEMENTED;
    }
    if (guest_addr == 0x800923A8u && wm_800923A8 != 0) {
        *out_fn = wm_800923A8;
        return WM_SCHED_CB_IMPLEMENTED;
    }
    if (guest_addr == 0x800925A0u && wm_800925A0 != 0) {
        *out_fn = wm_sched_builtin_800925A0;
        return WM_SCHED_CB_IMPLEMENTED;
    }
    if (guest_addr == 0x8008A52Cu && wm_8008A52C != 0) {
        *out_fn = wm_sched_builtin_8008A52C;
        return WM_SCHED_CB_IMPLEMENTED;
    }
    if (guest_addr == 0x8008B498u && wm_8008B498 != 0) {
        *out_fn = wm_sched_builtin_8008B498;
        return WM_SCHED_CB_IMPLEMENTED;
    }
    if (guest_addr == 0x8008BD1Cu && wm_8008BD1C != 0) {
        *out_fn = wm_sched_builtin_8008BD1C;
        return WM_SCHED_CB_IMPLEMENTED;
    }
    if (guest_addr == 0x8008C6ECu && wm_8008C6EC != 0) {
        *out_fn = wm_sched_builtin_8008C6EC;
        return WM_SCHED_CB_IMPLEMENTED;
    }
    if (guest_addr == 0x8008D520u && wm_8008D520 != 0) {
        *out_fn = wm_sched_builtin_8008D520;
        return WM_SCHED_CB_IMPLEMENTED;
    }
    if (guest_addr == 0x8008DE9Cu && wm_8008DE9C != 0) {
        *out_fn = wm_sched_builtin_8008DE9C;
        return WM_SCHED_CB_IMPLEMENTED;
    }
    if (guest_addr == 0x8008E4F4u && wm_8008E4F4 != 0) {
        *out_fn = wm_sched_builtin_8008E4F4;
        return WM_SCHED_CB_IMPLEMENTED;
    }
    if (guest_addr == 0x800907C4u && wm_800907C4 != 0) {
        *out_fn = wm_sched_builtin_800907C4;
        return WM_SCHED_CB_IMPLEMENTED;
    }
    if (guest_addr == 0x80087F60u && wm_80087F60 != 0) {
        *out_fn = wm_sched_builtin_80087F60;
        return WM_SCHED_CB_IMPLEMENTED;
    }
    if (guest_addr == 0x8008868Cu && wm_8008868C != 0) {
        *out_fn = wm_sched_builtin_8008868C;
        return WM_SCHED_CB_IMPLEMENTED;
    }
    if (guest_addr == 0x800879E0u && wm_800879E0 != 0) {
        *out_fn = wm_sched_builtin_800879E0;
        return WM_SCHED_CB_IMPLEMENTED;
    }
    if (guest_addr == 0x80088C90u && wm_80088C90 != 0) {
        *out_fn = wm_sched_builtin_80088C90;
        return WM_SCHED_CB_IMPLEMENTED;
    }
    if (guest_addr == 0x8008A2C8u && wm_8008A2C8 != 0) {
        *out_fn = wm_sched_builtin_8008A2C8;
        return WM_SCHED_CB_IMPLEMENTED;
    }
    if (guest_addr == 0x8008A72Cu && wm_8008A72C != 0) {
        *out_fn = wm_sched_builtin_8008A72C;
        return WM_SCHED_CB_IMPLEMENTED;
    }
    if (guest_addr == 0x8008B2BCu && wm_8008B2BC != 0) {
        *out_fn = wm_sched_builtin_8008B2BC;
        return WM_SCHED_CB_IMPLEMENTED;
    }
    if (guest_addr == 0x8008B644u && wm_8008B644 != 0) {
        *out_fn = wm_sched_builtin_8008B644;
        return WM_SCHED_CB_IMPLEMENTED;
    }
    if (guest_addr == 0x8008BB40u && wm_8008BB40 != 0) {
        *out_fn = wm_sched_builtin_8008BB40;
        return WM_SCHED_CB_IMPLEMENTED;
    }
    if (guest_addr == 0x8008C530u && wm_8008C530 != 0) {
        *out_fn = wm_sched_builtin_8008C530;
        return WM_SCHED_CB_IMPLEMENTED;
    }
    if (guest_addr == 0x8008C844u && wm_8008C844 != 0) {
        *out_fn = wm_sched_builtin_8008C844;
        return WM_SCHED_CB_IMPLEMENTED;
    }
    if (guest_addr == 0x8008D3F0u && wm_8008D3F0 != 0) {
        *out_fn = wm_sched_builtin_8008D3F0;
        return WM_SCHED_CB_IMPLEMENTED;
    }
    if (guest_addr == 0x8008D678u && wm_8008D678 != 0) {
        *out_fn = wm_sched_builtin_8008D678;
        return WM_SCHED_CB_IMPLEMENTED;
    }
    if (guest_addr == 0x8008DD6Cu && wm_8008DD6C != 0) {
        *out_fn = wm_sched_builtin_8008DD6C;
        return WM_SCHED_CB_IMPLEMENTED;
    }
    if (guest_addr == 0x8008E190u && wm_8008E190 != 0) {
        *out_fn = wm_sched_builtin_8008E190;
        return WM_SCHED_CB_IMPLEMENTED;
    }
    if (guest_addr == 0x8008E76Cu && wm_8008E76C != 0) {
        *out_fn = wm_sched_builtin_8008E76C;
        return WM_SCHED_CB_IMPLEMENTED;
    }
    if (guest_addr == 0x800906E0u && wm_800906E0 != 0) {
        *out_fn = wm_sched_builtin_800906E0;
        return WM_SCHED_CB_IMPLEMENTED;
    }
    if (guest_addr == 0x800907F4u && wm_800907F4 != 0) {
        *out_fn = wm_sched_builtin_800907F4;
        return WM_SCHED_CB_IMPLEMENTED;
    }
    if (guest_addr == 0x80091430u && func_80091430 != 0) {
        *out_fn = wm_sched_builtin_80091430;
        return WM_SCHED_CB_IMPLEMENTED;
    }
    if (guest_addr == 0x800914D0u && wm_800914D0 != 0) {
        *out_fn = wm_sched_builtin_800914D0;
        return WM_SCHED_CB_IMPLEMENTED;
    }
    if (guest_addr == 0x80091B54u && wm_80091B54 != 0) {
        *out_fn = wm_sched_builtin_80091B54;
        return WM_SCHED_CB_IMPLEMENTED;
    }
    if (guest_addr == 0x80091C18u && wm_80091C18 != 0) {
        *out_fn = wm_sched_builtin_80091C18;
        return WM_SCHED_CB_IMPLEMENTED;
    }
    if (guest_addr == 0x80092234u && wm_80092234 != 0) {
        *out_fn = wm_sched_builtin_80092234;
        return WM_SCHED_CB_IMPLEMENTED;
    }
    if (guest_addr == 0x800922ACu && wm_800922AC != 0) {
        *out_fn = wm_sched_builtin_800922AC;
        return WM_SCHED_CB_IMPLEMENTED;
    }
    if (guest_addr == 0x80092BE4u && wm_80092BE4 != 0) {
        *out_fn = wm_sched_builtin_80092BE4;
        return WM_SCHED_CB_IMPLEMENTED;
    }
    if (guest_addr == 0x80092C70u && wm_80092C70 != 0) {
        *out_fn = wm_sched_builtin_80092C70;
        return WM_SCHED_CB_IMPLEMENTED;
    }
    if (guest_addr == 0x80092DF8u && wm_80092DF8 != 0) {
        *out_fn = wm_sched_builtin_80092DF8;
        return WM_SCHED_CB_IMPLEMENTED;
    }
    if (guest_addr == 0x80092FD8u && wm_80092FD8 != 0) {
        *out_fn = wm_sched_builtin_80092FD8;
        return WM_SCHED_CB_IMPLEMENTED;
    }
    if (guest_addr == 0x80071A50u && func_80071A50 != 0) {
        *out_fn = wm_sched_builtin_80071A50;
        return WM_SCHED_CB_IMPLEMENTED;
    }
    if (guest_addr == 0x80071A58u && wm_80071A58 != 0) {
        *out_fn = wm_sched_builtin_80071A58;
        return WM_SCHED_CB_IMPLEMENTED;
    }
    if (guest_addr == 0x80087710u && func_80087710 != 0) {
        *out_fn = wm_sched_builtin_80087710;
        return WM_SCHED_CB_IMPLEMENTED;
    }
    if (guest_addr == 0x80087734u && wm_80087734 != 0) {
        *out_fn = wm_sched_builtin_80087734;
        return WM_SCHED_CB_IMPLEMENTED;
    }
    if (guest_addr == 0x8007756Cu && wm_8007756C != 0) {
        *out_fn = wm_sched_builtin_8007756C;
        return WM_SCHED_CB_IMPLEMENTED;
    }
    if (guest_addr == 0x800776E0u && wm_800776E0 != 0) {
        *out_fn = wm_sched_builtin_800776E0;
        return WM_SCHED_CB_IMPLEMENTED;
    }
    if (guest_addr == 0x80078948u && func_80078948 != 0) {
        *out_fn = wm_sched_builtin_80078948;
        return WM_SCHED_CB_IMPLEMENTED;
    }
    if (guest_addr == 0x80078950u && wm_80078950 != 0) {
        *out_fn = wm_sched_builtin_80078950;
        return WM_SCHED_CB_IMPLEMENTED;
    }
    if (guest_addr == 0x80077DC8u && wm_80077DC8 != 0) {
        *out_fn = wm_sched_builtin_80077DC8;
        return WM_SCHED_CB_IMPLEMENTED;
    }
    if (guest_addr == 0x80077E68u && wm_80077E68 != 0) {
        *out_fn = wm_sched_builtin_80077E68;
        return WM_SCHED_CB_IMPLEMENTED;
    }
    if (guest_addr == 0x8007828Cu && wm_8007828C != 0) {
        *out_fn = wm_sched_builtin_8007828C;
        return WM_SCHED_CB_IMPLEMENTED;
    }
    if (guest_addr == 0x800783E8u && wm_800783E8 != 0) {
        *out_fn = wm_sched_builtin_800783E8;
        return WM_SCHED_CB_IMPLEMENTED;
    }
    if (guest_addr == 0x80078E2Cu && func_80078E2C != 0) {
        *out_fn = wm_sched_builtin_80078E2C;
        return WM_SCHED_CB_IMPLEMENTED;
    }
    if (guest_addr == 0x80078EA4u && wm_80078EA4 != 0) {
        *out_fn = wm_sched_builtin_80078EA4;
        return WM_SCHED_CB_IMPLEMENTED;
    }
    if (guest_addr == 0x800794D8u && func_800794D8 != 0) {
        *out_fn = wm_sched_builtin_800794D8;
        return WM_SCHED_CB_IMPLEMENTED;
    }
    if (guest_addr == 0x80079538u && func_80079538 != 0) {
        *out_fn = wm_sched_builtin_80079538;
        return WM_SCHED_CB_IMPLEMENTED;
    }
    if (guest_addr == 0x800795E4u && wm_800795E4 != 0) {
        *out_fn = wm_sched_builtin_800795E4;
        return WM_SCHED_CB_IMPLEMENTED;
    }
    if (guest_addr == 0x80079778u && wm_80079778 != 0) {
        *out_fn = wm_sched_builtin_80079778;
        return WM_SCHED_CB_IMPLEMENTED;
    }
    if (guest_addr == 0x8007A410u && func_8007A410 != 0) {
        *out_fn = wm_sched_builtin_8007A410;
        return WM_SCHED_CB_IMPLEMENTED;
    }
    if (guest_addr == 0x8007A430u && wm_8007A430 != 0) {
        *out_fn = wm_sched_builtin_8007A430;
        return WM_SCHED_CB_IMPLEMENTED;
    }
    if (guest_addr == 0x8007A568u && func_8007A568 != 0) {
        *out_fn = wm_sched_builtin_8007A568;
        return WM_SCHED_CB_IMPLEMENTED;
    }
    if (guest_addr == 0x8007A570u && func_8007A570 != 0) {
        *out_fn = wm_sched_builtin_8007A570;
        return WM_SCHED_CB_IMPLEMENTED;
    }
    if (guest_addr == 0x8007A144u && wm_8007A144 != 0) {
        *out_fn = wm_sched_builtin_8007A144;
        return WM_SCHED_CB_IMPLEMENTED;
    }
    if (guest_addr == 0x8007A1B4u && wm_8007A1B4 != 0) {
        *out_fn = wm_sched_builtin_8007A1B4;
        return WM_SCHED_CB_IMPLEMENTED;
    }
    if (guest_addr == 0x8007A9B4u && wm_8007A9B4 != 0) {
        *out_fn = wm_sched_builtin_8007A9B4;
        return WM_SCHED_CB_IMPLEMENTED;
    }
    if (guest_addr == 0x8007A9F8u && wm_8007A9F8 != 0) {
        *out_fn = wm_sched_builtin_8007A9F8;
        return WM_SCHED_CB_IMPLEMENTED;
    }
    if (guest_addr == 0x8007AD34u && wm_8007AD34 != 0) {
        *out_fn = wm_sched_builtin_8007AD34;
        return WM_SCHED_CB_IMPLEMENTED;
    }
    if (guest_addr == 0x8007ADD4u && wm_8007ADD4 != 0) {
        *out_fn = wm_sched_builtin_8007ADD4;
        return WM_SCHED_CB_IMPLEMENTED;
    }
    if (guest_addr == 0x8007BA08u && func_8007BA08 != 0) {
        *out_fn = wm_sched_builtin_8007BA08;
        return WM_SCHED_CB_IMPLEMENTED;
    }
    if (guest_addr == 0x8007BA10u && wm_8007BA10 != 0) {
        *out_fn = wm_sched_builtin_8007BA10;
        return WM_SCHED_CB_IMPLEMENTED;
    }
    if (guest_addr == 0x8007BB60u && func_8007BB60 != 0) {
        *out_fn = wm_sched_builtin_8007BB60;
        return WM_SCHED_CB_IMPLEMENTED;
    }
    if (guest_addr == 0x8007BBECu && wm_8007BBEC != 0) {
        *out_fn = wm_sched_builtin_8007BBEC;
        return WM_SCHED_CB_IMPLEMENTED;
    }
    if (guest_addr == 0x8007B200u && wm_8007B200 != 0) {
        *out_fn = wm_sched_builtin_8007B200;
        return WM_SCHED_CB_IMPLEMENTED;
    }
    if (guest_addr == 0x8007B394u && wm_8007B394 != 0) {
        *out_fn = wm_sched_builtin_8007B394;
        return WM_SCHED_CB_IMPLEMENTED;
    }
    if (guest_addr == 0x8007B604u && wm_8007B604 != 0) {
        *out_fn = wm_sched_builtin_8007B604;
        return WM_SCHED_CB_IMPLEMENTED;
    }
    if (guest_addr == 0x8007B798u && wm_8007B798 != 0) {
        *out_fn = wm_sched_builtin_8007B798;
        return WM_SCHED_CB_IMPLEMENTED;
    }
    if (guest_addr == 0x8007D600u && func_8007D600 != 0) {
        *out_fn = wm_sched_builtin_8007D600;
        return WM_SCHED_CB_IMPLEMENTED;
    }
    if (guest_addr == 0x8007D690u && func_8007D690 != 0) {
        *out_fn = wm_sched_builtin_8007D690;
        return WM_SCHED_CB_IMPLEMENTED;
    }
    if (guest_addr == 0x8007D774u && func_8007D774 != 0) {
        *out_fn = wm_sched_builtin_8007D774;
        return WM_SCHED_CB_IMPLEMENTED;
    }
    if (guest_addr == 0x8007D7FCu && wm_8007D7FC != 0) {
        *out_fn = wm_sched_builtin_8007D7FC;
        return WM_SCHED_CB_IMPLEMENTED;
    }
    if (guest_addr == 0x8007D078u && func_8007D078 != 0) {
        *out_fn = wm_sched_builtin_8007D078;
        return WM_SCHED_CB_IMPLEMENTED;
    }
    if (guest_addr == 0x8007D110u && wm_8007D110 != 0) {
        *out_fn = wm_sched_builtin_8007D110;
        return WM_SCHED_CB_IMPLEMENTED;
    }
    if (guest_addr == 0x8007D228u && func_8007D228 != 0) {
        *out_fn = wm_sched_builtin_8007D228;
        return WM_SCHED_CB_IMPLEMENTED;
    }
    if (guest_addr == 0x8007D2B8u && wm_8007D2B8 != 0) {
        *out_fn = wm_sched_builtin_8007D2B8;
        return WM_SCHED_CB_IMPLEMENTED;
    }
    if (guest_addr == 0x8007D414u && func_8007D414 != 0) {
        *out_fn = wm_sched_builtin_8007D414;
        return WM_SCHED_CB_IMPLEMENTED;
    }
    if (guest_addr == 0x8007D4A4u && wm_8007D4A4 != 0) {
        *out_fn = wm_sched_builtin_8007D4A4;
        return WM_SCHED_CB_IMPLEMENTED;
    }
    if (guest_addr == 0x8007CE84u && func_8007CE84 != 0) {
        *out_fn = wm_sched_builtin_8007CE84;
        return WM_SCHED_CB_IMPLEMENTED;
    }
    if (guest_addr == 0x8007CF18u && wm_8007CF18 != 0) {
        *out_fn = wm_sched_builtin_8007CF18;
        return WM_SCHED_CB_IMPLEMENTED;
    }
    if (guest_addr == 0x8007CC6Cu && func_8007CC6C != 0) {
        *out_fn = wm_sched_builtin_8007CC6C;
        return WM_SCHED_CB_IMPLEMENTED;
    }
    if (guest_addr == 0x8007CD20u && wm_8007CD20 != 0) {
        *out_fn = wm_sched_builtin_8007CD20;
        return WM_SCHED_CB_IMPLEMENTED;
    }
    if (guest_addr == 0x8007C36Cu && wm_8007C36C != 0) {
        *out_fn = wm_sched_builtin_8007C36C;
        return WM_SCHED_CB_IMPLEMENTED;
    }
    if (guest_addr == 0x8007C3B8u && wm_8007C3B8 != 0) {
        *out_fn = wm_sched_builtin_8007C3B8;
        return WM_SCHED_CB_IMPLEMENTED;
    }
    if (guest_addr == 0x8007C724u && wm_8007C724 != 0) {
        *out_fn = wm_sched_builtin_8007C724;
        return WM_SCHED_CB_IMPLEMENTED;
    }
    if (guest_addr == 0x8007C7D8u && wm_8007C7D8 != 0) {
        *out_fn = wm_sched_builtin_8007C7D8;
        return WM_SCHED_CB_IMPLEMENTED;
    }
    if (guest_addr == 0x8007DE14u && wm_8007DE14 != 0) {
        *out_fn = wm_sched_builtin_8007DE14;
        return WM_SCHED_CB_IMPLEMENTED;
    }
    if (guest_addr == 0x8007DE98u && wm_8007DE98 != 0) {
        *out_fn = wm_sched_builtin_8007DE98;
        return WM_SCHED_CB_IMPLEMENTED;
    }
    if (guest_addr == 0x8007E450u && wm_8007E450 != 0) {
        *out_fn = wm_sched_builtin_8007E450;
        return WM_SCHED_CB_IMPLEMENTED;
    }
    if (guest_addr == 0x8007E4E4u && wm_8007E4E4 != 0) {
        *out_fn = wm_sched_builtin_8007E4E4;
        return WM_SCHED_CB_IMPLEMENTED;
    }
    if (guest_addr == 0x8007ECA4u && wm_8007ECA4 != 0) {
        *out_fn = wm_sched_builtin_8007ECA4;
        return WM_SCHED_CB_IMPLEMENTED;
    }
    if (guest_addr == 0x8007EE34u && wm_8007EE34 != 0) {
        *out_fn = wm_sched_builtin_8007EE34;
        return WM_SCHED_CB_IMPLEMENTED;
    }
    if (guest_addr == 0x8007F8ACu && wm_8007F8AC != 0) {
        *out_fn = wm_sched_builtin_8007F8AC;
        return WM_SCHED_CB_IMPLEMENTED;
    }
    if (guest_addr == 0x8007F968u && wm_8007F968 != 0) {
        *out_fn = wm_sched_builtin_8007F968;
        return WM_SCHED_CB_IMPLEMENTED;
    }
    if (guest_addr == 0x8007FC8Cu && wm_8007FC8C != 0) {
        *out_fn = wm_sched_builtin_8007FC8C;
        return WM_SCHED_CB_IMPLEMENTED;
    }
    if (guest_addr == 0x8007FD30u && wm_8007FD30 != 0) {
        *out_fn = wm_sched_builtin_8007FD30;
        return WM_SCHED_CB_IMPLEMENTED;
    }
    if (guest_addr == 0x80076A14u && func_80076A14 != 0) {
        *out_fn = wm_sched_builtin_80076A14;
        return WM_SCHED_CB_IMPLEMENTED;
    }
    if (guest_addr == 0x80076A1Cu && wm_80076A1C != 0) {
        *out_fn = wm_sched_builtin_80076A1C;
        return WM_SCHED_CB_IMPLEMENTED;
    }
    if (guest_addr == 0x80080900u && func_80080900 != 0) {
        *out_fn = wm_sched_builtin_80080900;
        return WM_SCHED_CB_IMPLEMENTED;
    }
    if (guest_addr == 0x80080944u && func_80080944 != 0) {
        *out_fn = wm_sched_builtin_80080944;
        return WM_SCHED_CB_IMPLEMENTED;
    }
    if (guest_addr == 0x8008032Cu && wm_8008032C != 0) {
        *out_fn = wm_sched_builtin_8008032C;
        return WM_SCHED_CB_IMPLEMENTED;
    }
    if (guest_addr == 0x80080370u && wm_80080370 != 0) {
        *out_fn = wm_sched_builtin_80080370;
        return WM_SCHED_CB_IMPLEMENTED;
    }
    if (guest_addr == 0x80080A28u && wm_80080A28 != 0) {
        *out_fn = wm_sched_builtin_80080A28;
        return WM_SCHED_CB_IMPLEMENTED;
    }
    if (guest_addr == 0x80080AC4u && wm_80080AC4 != 0) {
        *out_fn = wm_sched_builtin_80080AC4;
        return WM_SCHED_CB_IMPLEMENTED;
    }
    if (guest_addr == 0x80080578u && wm_80080578 != 0) {
        *out_fn = wm_sched_builtin_80080578;
        return WM_SCHED_CB_IMPLEMENTED;
    }
    if (guest_addr == 0x80080600u && wm_80080600 != 0) {
        *out_fn = wm_sched_builtin_80080600;
        return WM_SCHED_CB_IMPLEMENTED;
    }
    if (guest_addr == 0x80081174u && wm_80081174 != 0) {
        *out_fn = wm_sched_builtin_80081174;
        return WM_SCHED_CB_IMPLEMENTED;
    }
    if (guest_addr == 0x800811C0u && wm_800811C0 != 0) {
        *out_fn = wm_sched_builtin_800811C0;
        return WM_SCHED_CB_IMPLEMENTED;
    }
    if (guest_addr == 0x800813E8u && wm_800813E8 != 0) {
        *out_fn = wm_sched_builtin_800813E8;
        return WM_SCHED_CB_IMPLEMENTED;
    }
    if (guest_addr == 0x80081470u && wm_80081470 != 0) {
        *out_fn = wm_sched_builtin_80081470;
        return WM_SCHED_CB_IMPLEMENTED;
    }
    if (guest_addr == 0x800817A0u && wm_800817A0 != 0) {
        *out_fn = wm_sched_builtin_800817A0;
        return WM_SCHED_CB_IMPLEMENTED;
    }
    if (guest_addr == 0x80081868u && wm_80081868 != 0) {
        *out_fn = wm_sched_builtin_80081868;
        return WM_SCHED_CB_IMPLEMENTED;
    }
    if (guest_addr == 0x800819C8u && wm_800819C8 != 0) {
        *out_fn = wm_sched_builtin_800819C8;
        return WM_SCHED_CB_IMPLEMENTED;
    }
    if (guest_addr == 0x80081B24u && wm_80081B24 != 0) {
        *out_fn = wm_sched_builtin_80081B24;
        return WM_SCHED_CB_IMPLEMENTED;
    }
    if (guest_addr == 0x80081C3Cu && wm_80081C3C != 0) {
        *out_fn = wm_sched_builtin_80081C3C;
        return WM_SCHED_CB_IMPLEMENTED;
    }
    if (guest_addr == 0x80081D80u && wm_80081D80 != 0) {
        *out_fn = wm_sched_builtin_80081D80;
        return WM_SCHED_CB_IMPLEMENTED;
    }
    if (guest_addr == 0x80081FB4u && func_80081FB4 != 0) {
        *out_fn = wm_sched_builtin_80081FB4;
        return WM_SCHED_CB_IMPLEMENTED;
    }
    if (guest_addr == 0x80081FD8u && wm_80081FD8 != 0) {
        *out_fn = wm_sched_builtin_80081FD8;
        return WM_SCHED_CB_IMPLEMENTED;
    }
    if (guest_addr == 0x800827C8u && wm_800827C8 != 0) {
        *out_fn = wm_sched_builtin_800827C8;
        return WM_SCHED_CB_IMPLEMENTED;
    }
    if (guest_addr == 0x80076B34u && wm_80076B34 != 0) {
        *out_fn = wm_sched_builtin_80076B34;
        return WM_SCHED_CB_IMPLEMENTED;
    }
    if (guest_addr == 0x800827ECu && wm_800827EC != 0) {
        *out_fn = wm_sched_builtin_800827EC;
        return WM_SCHED_CB_IMPLEMENTED;
    }
    if (guest_addr == 0x800828DCu && wm_800828DC != 0) {
        *out_fn = wm_sched_builtin_800828DC;
        return WM_SCHED_CB_IMPLEMENTED;
    }
    if (guest_addr == 0x80083214u && wm_80083214 != 0) {
        *out_fn = wm_sched_builtin_80083214;
        return WM_SCHED_CB_IMPLEMENTED;
    }
    if (guest_addr == 0x80083264u && wm_80083264 != 0) {
        *out_fn = wm_sched_builtin_80083264;
        return WM_SCHED_CB_IMPLEMENTED;
    }
    if (guest_addr == 0x800834D0u && func_800834D0 != 0) {
        *out_fn = wm_sched_builtin_800834D0;
        return WM_SCHED_CB_IMPLEMENTED;
    }
    if (guest_addr == 0x800834D8u && wm_800834D8 != 0) {
        *out_fn = wm_sched_builtin_800834D8;
        return WM_SCHED_CB_IMPLEMENTED;
    }
    if (guest_addr == 0x800838E8u && wm_800838E8 != 0) {
        *out_fn = wm_sched_builtin_800838E8;
        return WM_SCHED_CB_IMPLEMENTED;
    }
    if (guest_addr == 0x8008390Cu && wm_8008390C != 0) {
        *out_fn = wm_sched_builtin_8008390C;
        return WM_SCHED_CB_IMPLEMENTED;
    }
    if (guest_addr == 0x80083FE4u && wm_80083FE4 != 0) {
        *out_fn = wm_sched_builtin_80083FE4;
        return WM_SCHED_CB_IMPLEMENTED;
    }
    if (guest_addr == 0x80083A00u && wm_80083A00 != 0) {
        *out_fn = wm_sched_builtin_80083A00;
        return WM_SCHED_CB_IMPLEMENTED;
    }
    if (guest_addr == 0x80084068u && wm_80084068 != 0) {
        *out_fn = wm_sched_builtin_80084068;
        return WM_SCHED_CB_IMPLEMENTED;
    }
    if (guest_addr == 0x800877E0u && func_800877E0 != 0) {
        *out_fn = wm_sched_builtin_800877E0;
        return WM_SCHED_CB_IMPLEMENTED;
    }
    if (guest_addr == 0x80087804u && func_80087804 != 0) {
        *out_fn = wm_sched_builtin_80087804;
        return WM_SCHED_CB_IMPLEMENTED;
    }
    if (guest_addr == 0x800879A8u && func_800879A8 != 0) {
        *out_fn = wm_sched_builtin_800879A8;
        return WM_SCHED_CB_IMPLEMENTED;
    }
    if (guest_addr == 0x80087A8Cu && func_80087A8C != 0) {
        *out_fn = wm_sched_builtin_80087A8C;
        return WM_SCHED_CB_IMPLEMENTED;
    }
    if (guest_addr == 0x80088D64u && func_80088D64 != 0) {
        *out_fn = wm_sched_builtin_80088D64;
        return WM_SCHED_CB_IMPLEMENTED;
    }
    if (guest_addr == 0x80088DE4u && func_80088DE4 != 0) {
        *out_fn = wm_sched_builtin_80088DE4;
        return WM_SCHED_CB_IMPLEMENTED;
    }
    if (guest_addr == 0x80088E1Cu && func_80088E1C != 0) {
        *out_fn = wm_sched_builtin_80088E1C;
        return WM_SCHED_CB_IMPLEMENTED;
    }
    if (guest_addr == 0x80088E68u && func_80088E68 != 0) {
        *out_fn = wm_sched_builtin_80088E68;
        return WM_SCHED_CB_IMPLEMENTED;
    }
    if (guest_addr == 0x80088EA0u && func_80088EA0 != 0) {
        *out_fn = wm_sched_builtin_80088EA0;
        return WM_SCHED_CB_IMPLEMENTED;
    }
    if (guest_addr == 0x80088F1Cu && func_80088F1C != 0) {
        *out_fn = wm_sched_builtin_80088F1C;
        return WM_SCHED_CB_IMPLEMENTED;
    }
    if (guest_addr == 0x80088F54u && func_80088F54 != 0) {
        *out_fn = wm_sched_builtin_80088F54;
        return WM_SCHED_CB_IMPLEMENTED;
    }
    if (guest_addr == 0x80088F5Cu && func_80088F5C != 0) {
        *out_fn = wm_sched_builtin_80088F5C;
        return WM_SCHED_CB_IMPLEMENTED;
    }
    if (guest_addr == 0x80087C6Cu && func_80087C6C != 0) {
        *out_fn = wm_sched_builtin_80087C6C;
        return WM_SCHED_CB_IMPLEMENTED;
    }
    if (guest_addr == 0x80087FD0u && func_80087FD0 != 0) {
        *out_fn = wm_sched_builtin_80087FD0;
        return WM_SCHED_CB_IMPLEMENTED;
    }
    if (guest_addr == 0x80088B40u && func_80088B40 != 0) {
        *out_fn = wm_sched_builtin_80088B40;
        return WM_SCHED_CB_IMPLEMENTED;
    }
    if (guest_addr == 0x80088D00u && func_80088D00 != 0) {
        *out_fn = wm_sched_builtin_80088D00;
        return WM_SCHED_CB_IMPLEMENTED;
    }
    if (guest_addr == 0x80088570u && func_80088570 != 0) {
        *out_fn = wm_sched_builtin_80088570;
        return WM_SCHED_CB_IMPLEMENTED;
    }
    if (guest_addr == 0x80088720u && wm_80088720 != 0) {
        *out_fn = wm_sched_builtin_80088720;
        return WM_SCHED_CB_IMPLEMENTED;
    }
    if (wm_sched_is_known_missing(guest_addr))
        return WM_SCHED_CB_MISSING;
    return WM_SCHED_CB_INVALID;
}

void wm_80097800(void)
{
    u32 pool_psx = *(u32*)WM_SCHED_RAM(WM_SCHED_POOL_PTR);
    u8* base;
    int i;

    s_entry++;
    fprintf(stderr, "[worldmap-scheduler] entry pool=0x%08x\n", pool_psx);

    if (pool_psx == 0) {
        /* Unnatural: retail would dereference near-null. Bounded stop. */
        s_outcome = WM_SCHED_STOP_NO_POOL;
        s_frontier_pc = WM_SCHED_CUT_BEFORE_DRAWSYNC;
        fprintf(stderr, "[worldmap-scheduler] ERROR: pool base null\n");
        return;
    }
    base = WM_SCHED_RAM(pool_psx);

    for (i = 0; i < WM_SCHED_SLOT_COUNT; i++) {
        u8* slot = base + (u32)i * WM_SCHED_SLOT_STRIDE;
        u32 occupancy = *(u32*)(slot + WM_SCHED_OFF_CB1);
        s16 state;

        s_slots_inspected++;
        if (occupancy == 0)
            continue;                               /* 0x80097838 -> next */
        s_occupied_inspected++;

        state = *(s16*)(slot + WM_SCHED_OFF_STATE); /* lh sign-extends */
        if ((u32)(s32)state >= 5) {                 /* sltiu v0,v1,5 */
            s_state_invalid_seen++;
            continue;                               /* 0x8009784C -> next */
        }
        s_state_seen[state]++;

        switch (state) {                            /* jr via 0x80070CE8 */
        case 0:
        case 1: {
            u32 target = (state == 0)
                ? *(u32*)(slot + WM_SCHED_OFF_CB0)  /* 0x80097868 */
                : occupancy;                        /* 0x80097874 */
            wm_sched_callback_fn fn = 0;
            wm_sched_cb_resolve_t r;

            s_dispatch_attempts++;
            s_last_slot = i;
            s_last_callback = target;
            s_last_callback_state = state;
            r = wm_sched_resolve(target, &fn);
            if (r == WM_SCHED_CB_IMPLEMENTED) {
                /* jalr target, a0 = slot index (0x8009787C/0x80097880);
                 * return value becomes the slot state (0x80097888). */
                s16 ret = fn(i);
                *(s16*)(slot + WM_SCHED_OFF_STATE) = ret;
                s_callbacks_executed++;
                fprintf(stderr, "[worldmap-scheduler] slot=%d state=%d "
                        "cb=0x%08x executed ret=%d\n", i, state, target, ret);
            } else if (r == WM_SCHED_CB_MISSING) {
                s_missing_hits++;
                wm_sched_log_stub(target, "missing_callback", i, state);
                s_frontier_pc = target;
                /* Bounded stop before the missing body: the guest address
                 * was never called, so fabricate no slot state and advance
                 * no further. Matches WM_SCHED_STOP_MISSING_CALLBACK. */
                s_outcome = WM_SCHED_STOP_MISSING_CALLBACK;
                return;
            } else {
                s_invalid_hits++;
                wm_sched_log_stub(target, "invalid_callback", i, state);
                s_frontier_pc = target;
                /* Bounded stop, unknown callback address: never fabricate
                 * slot state, never advance past the frontier. */
                s_outcome = WM_SCHED_STOP_INVALID_CALLBACK;
                return;
            }
            break;
        }
        case 2: {
            /* 0x8009788C: decrement timer; expiry (<=0) -> state 1. */
            u16 t = *(u16*)(slot + WM_SCHED_OFF_TIMER);
            t = (u16)(t - 1);
            *(u16*)(slot + WM_SCHED_OFF_TIMER) = t;
            if ((s16)t <= 0)
                *(s16*)(slot + WM_SCHED_OFF_STATE) = 1;
            break;
        }
        case 3:
            break;                                  /* dormant: 0x800978C8 */
        case 4: {
            /* 0x800978B0: payload non-null -> func_800230A8(payload). */
            u32 payload = *(u32*)(slot + WM_SCHED_OFF_PAYLOAD);
            if (payload != 0) {
                if (s_wm_sched_test_destructor != 0) {
                    s_wm_sched_test_destructor(payload);
                    s_destructor_calls++;
                } else {
                    s_destructor_boundary_hits++;
                    wm_sched_log_stub(WM_SCHED_DESTRUCTOR, "destructor", i,
                                      state);
                    s_frontier_pc = WM_SCHED_DESTRUCTOR;
                    s_last_slot = i;
                    s_last_callback = WM_SCHED_DESTRUCTOR;
                    s_last_callback_state = state;
                    /* Unknown destructor is skipped; traversal continues. */
                }
            }
            break;
        }
        }
    }

    s_completed_passes++;
    s_outcome = WM_SCHED_PASS_COMPLETE;
    s_frontier_pc = WM_SCHED_CUT_BEFORE_DRAWSYNC;
    fprintf(stderr, "[worldmap-scheduler] pass complete slots=%d occupied=%d "
            "dispatched=%d executed=%d\n",
            s_slots_inspected, s_occupied_inspected,
            s_dispatch_attempts, s_callbacks_executed);
}

int  wm_sched_get_entry(void)                   { return s_entry; }
int  wm_sched_get_slots_inspected(void)         { return s_slots_inspected; }
int  wm_sched_get_occupied_inspected(void)      { return s_occupied_inspected; }
int  wm_sched_get_state_seen(int state)
{
    if (state < 0 || state > 4) return 0;
    return s_state_seen[state];
}
int  wm_sched_get_state_invalid_seen(void)      { return s_state_invalid_seen; }
int  wm_sched_get_dispatch_attempts(void)       { return s_dispatch_attempts; }
int  wm_sched_get_callbacks_executed(void)      { return s_callbacks_executed; }
int  wm_sched_get_missing_hits(void)            { return s_missing_hits; }
int  wm_sched_get_invalid_hits(void)            { return s_invalid_hits; }
int  wm_sched_get_destructor_calls(void)        { return s_destructor_calls; }
int  wm_sched_get_destructor_boundary_hits(void){ return s_destructor_boundary_hits; }
int  wm_sched_get_completed_passes(void)        { return s_completed_passes; }
int  wm_sched_get_last_slot(void)               { return s_last_slot; }
u32  wm_sched_get_last_callback(void)           { return s_last_callback; }
int  wm_sched_get_last_callback_state(void)     { return s_last_callback_state; }
int  wm_sched_get_outcome(void)                 { return s_outcome; }
u32  wm_sched_get_frontier_pc(void)             { return s_frontier_pc; }
