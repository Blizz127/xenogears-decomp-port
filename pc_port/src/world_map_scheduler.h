/*
 * World-map callback scheduler module (W34B5H).
 *
 * Exact bounded transcription of retail 0x80097800 (WorldMapMain scheduler,
 * called once per frame at 0x80071064, immediately after slot-1 returns and
 * before DrawSync at 0x8007106C).
 *
 * Retail authority (W34B5G fresh decode, scratchpad/w34b5g_scheduler_80097800):
 *   start 0x80097800, jr ra 0x800978F4, end exclusive 0x800978FC,
 *   252 bytes / 63 instructions.
 *   pool base pointer: *(u32*)0x8009BE24; 64 slots; stride 0x80.
 *   slot+0x00 s16 state; slot+0x02 u16 wait timer; slot+0x04 s16 flag;
 *   slot+0x18 u32 state-0 callback; slot+0x1C u32 state-1 callback/occupancy;
 *   slot+0x4C u32 payload.
 *   state jump table 0x80070CE8: 0=call +0x18, 1=call +0x1C, 2=timer,
 *   3=dormant, 4=destroy payload via func_800230A8; >=5 (or negative) skip.
 *   callback invoked with a0 = slot index; return value becomes slot state.
 *
 * Bounded callback-dispatch frontier (W34B5H): no world callback body is
 * implemented yet, so dispatch resolves through a registry instead of
 * casting guest addresses to native pointers. A recognized-but-unimplemented
 * guest callback stops the pass BEFORE any return-dependent mutation.
 *
 * Extracted as its own module so production and the production-linked test
 * link the same authoritative object.
 */
#ifndef WORLD_MAP_SCHEDULER_H
#define WORLD_MAP_SCHEDULER_H

#include "common.h"

/* Retail addresses. */
#define WM_SCHED_RETAIL_ENTRY        0x80097800u
#define WM_SCHED_RETAIL_END_EXCL     0x800978FCu
#define WM_SCHED_CALL_SITE           0x80071064u  /* jal 0x80097800 in WorldMapMain */
#define WM_SCHED_CUT_BEFORE_DRAWSYNC 0x8007106Cu  /* first excluded caller PC */

/* Pool authority. */
#define WM_SCHED_POOL_PTR            0x8009BE24u
#define WM_SCHED_SLOT_COUNT          64
#define WM_SCHED_SLOT_STRIDE         0x80u
#define WM_SCHED_OFF_STATE           0x00u
#define WM_SCHED_OFF_TIMER           0x02u
#define WM_SCHED_OFF_FLAG            0x04u
#define WM_SCHED_OFF_CB0             0x18u
#define WM_SCHED_OFF_CB1             0x1Cu
#define WM_SCHED_OFF_PAYLOAD         0x4Cu

/* Retail state-4 payload destructor (main exe, HeapFree chain). */
#define WM_SCHED_DESTRUCTOR          0x800230A8u

/* Natural first missing callback (slot 0, state 0, Table A record 0). */
#define WM_SCHED_NATURAL_FIRST_CALLBACK 0x800923A8u

/* Callback resolution classes. */
typedef enum wm_sched_cb_resolve {
    WM_SCHED_CB_IMPLEMENTED = 0, /* registered body; return value valid */
    WM_SCHED_CB_MISSING,         /* recognized guest callback, no body: frontier */
    WM_SCHED_CB_INVALID          /* invalid/unknown guest callback address */
} wm_sched_cb_resolve_t;

/* Scheduler pass outcomes. */
typedef enum wm_sched_outcome {
    WM_SCHED_PASS_COMPLETE = 0,       /* all 64 slots visited, no boundary hit */
    WM_SCHED_STOP_MISSING_CALLBACK,   /* bounded stop before missing body */
    WM_SCHED_STOP_INVALID_CALLBACK,   /* bounded stop, unknown callback address */
    WM_SCHED_STOP_DESTRUCTOR_BOUNDARY,/* bounded stop, state-4 destructor missing */
    WM_SCHED_STOP_NO_POOL             /* pool base pointer null (unnatural) */
} wm_sched_outcome_t;

/* Production scheduler. Exact retail traversal/state machine with the
 * bounded dispatch frontier above. */
void wm_80097800(void);

/* Guest-callback registry. Production registers nothing yet; when a real
 * callback body lands it registers here. Synthetic production-linked tests
 * register test bodies the same way. a0 semantics: slot index; return
 * value becomes the slot state, exactly like retail. */
typedef s16 (*wm_sched_callback_fn)(int slot_index);
void wm_sched_callback_register(u32 guest_addr, wm_sched_callback_fn fn);
void wm_sched_callback_registry_clear(void);

/* State-4 destructor boundary hook. Production leaves this unset (retail
 * func_800230A8 body is not ported); synthetic tests set a test destructor
 * to verify scheduler control flow. When unset and a non-null payload is
 * reached, the scheduler stops at WM_SCHED_STOP_DESTRUCTOR_BOUNDARY without
 * fabricating destructor behavior. */
typedef void (*wm_sched_destructor_fn)(u32 payload_psx);
void wm_sched_test_set_destructor(wm_sched_destructor_fn fn);

/* Per-world-init reset. */
void wm_sched_reset(void);

/* Counter / frontier accessors. */
int  wm_sched_get_entry(void);
int  wm_sched_get_slots_inspected(void);
int  wm_sched_get_occupied_inspected(void);
int  wm_sched_get_state_seen(int state);      /* 0..4 */
int  wm_sched_get_state_invalid_seen(void);   /* >=5 or negative */
int  wm_sched_get_dispatch_attempts(void);
int  wm_sched_get_callbacks_executed(void);   /* host bodies actually run */
int  wm_sched_get_missing_hits(void);
int  wm_sched_get_invalid_hits(void);
int  wm_sched_get_destructor_calls(void);
int  wm_sched_get_destructor_boundary_hits(void);
int  wm_sched_get_completed_passes(void);
int  wm_sched_get_last_slot(void);
u32  wm_sched_get_last_callback(void);
int  wm_sched_get_last_callback_state(void);
int  wm_sched_get_outcome(void);              /* wm_sched_outcome_t */
u32  wm_sched_get_frontier_pc(void);          /* stopped callback addr, or
                                                 WM_SCHED_CUT_BEFORE_DRAWSYNC
                                                 when the pass completed */

#endif /* WORLD_MAP_SCHEDULER_H */
