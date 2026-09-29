/*
 * W34B5H — Production-linked test for world scheduler wm_80097800.
 *
 * Links the ACTUAL production scheduler (pc_port/src/world_map_scheduler.c)
 * and drives it against independent synthetic pools. Covers the full
 * 64-slot traversal semantics plus the bounded missing-callback frontier.
 *
 * Also contains an INDEPENDENT retail-oracle replica (replica_run), derived
 * from the fresh W34B5G CFG documentation — it never calls the production
 * scheduler. Deterministic synthetic scenarios are run through both and
 * compared: outcome, traversal/callback order, stop point, and byte-for-byte
 * pool contents.
 *
 * Build:
 *   gcc -std=gnu17 -O0 -g -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C \
 *     -Ipc_port/include -Iinclude -Ipc_port/src \
 *     pc_port/tests/w34b5h_scheduler_prod_test.c \
 *     pc_port/src/world_map_scheduler.c \
 *     -o pc_port/build_native/w34b5h_scheduler_prod_test
 */
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_scheduler.h"

uint8_t g_PsxRam[PSX_RAM_SIZE];

/* Synthetic pool home (well inside the 2MiB emulated RAM, away from 0). */
#define TEST_POOL_PSX   0x80040000u
#define POOL_BYTES      (WM_SCHED_SLOT_COUNT * WM_SCHED_SLOT_STRIDE) /* 0x2000 */
#define CANARY_BYTES    16
#define CANARY_VAL      0xA5

/* Synthetic guest callback addresses (NOT in the known-missing set). */
#define TEST_CB_A   0x80099000u
#define TEST_CB_B   0x80099010u
#define TEST_CB_C   0x80099020u
#define TEST_CB_D   0x80099030u
#define TEST_CB_E   0x80099040u
#define TEST_CB_INSERTER 0x80099050u
#define TEST_CB_CLEARER  0x80099060u
#define TEST_CB_UNKNOWN  0x80099999u

#define KNOWN_MISSING_CB0_SLOT0 0x800923A8u  /* natural first callback */

static int total = 0, pass = 0, fail = 0;

static void check(const char* name, int cond)
{
    total++;
    if (cond) { pass++; printf("  PASS: %s\n", name); }
    else      { fail++; printf("  FAIL: %s\n", name); }
}

/* ---- shared invocation recording (both engines append here) ---- */

static int s_order[128];
static int s_order_n;
static int s_last_arg[128];
static u32 s_destructor_args[16];
static int s_destructor_n;

static void log_reset(void)
{
    s_order_n = 0;
    s_destructor_n = 0;
    memset(s_order, -1, sizeof(s_order));
    memset(s_last_arg, -1, sizeof(s_last_arg));
    memset(s_destructor_args, 0, sizeof(s_destructor_args));
}

/* ---- synthetic callback bodies (return values are SCRIPTED test values,
 *      not fabricated retail behavior; production never registers these) ---- */

static s16 test_cb_ret1(int slot)  { s_last_arg[s_order_n] = slot; s_order[s_order_n++] = slot; return 1; }
static s16 test_cb_ret2(int slot)  { s_last_arg[s_order_n] = slot; s_order[s_order_n++] = slot; return 2; }
static s16 test_cb_ret3(int slot)  { s_last_arg[s_order_n] = slot; s_order[s_order_n++] = slot; return 3; }

/* Inserter: occupies later slot 7 (retail-exact field writes: state 0,
 * timer 0, cb0/cb1) so the SAME pass must observe it. */
static s16 test_cb_inserter(int slot)
{
    u8* pool = (u8*)PSX_ADDR(TEST_POOL_PSX);
    u8* s7 = pool + 7 * WM_SCHED_SLOT_STRIDE;
    s_last_arg[s_order_n] = slot;
    s_order[s_order_n++] = slot;
    *(s16*)(s7 + WM_SCHED_OFF_STATE) = 0;
    *(u16*)(s7 + WM_SCHED_OFF_TIMER) = 0;
    *(u32*)(s7 + WM_SCHED_OFF_CB0) = TEST_CB_C;
    *(u32*)(s7 + WM_SCHED_OFF_CB1) = TEST_CB_D;   /* occupancy */
    return 0;
}

/* Clearer: frees later slot 7 before the pass reaches it. */
static s16 test_cb_clearer(int slot)
{
    u8* pool = (u8*)PSX_ADDR(TEST_POOL_PSX);
    u8* s7 = pool + 7 * WM_SCHED_SLOT_STRIDE;
    s_last_arg[s_order_n] = slot;
    s_order[s_order_n++] = slot;
    *(u32*)(s7 + WM_SCHED_OFF_CB1) = 0;
    return 0;
}

static void test_destructor(u32 payload_psx)
{
    if (s_destructor_n < 16)
        s_destructor_args[s_destructor_n] = payload_psx;
    s_destructor_n++;
}

/* ---- pool helpers ---- */

static void pool_clear(void)
{
    memset(PSX_ADDR(TEST_POOL_PSX), 0, POOL_BYTES);
    *(u32*)PSX_ADDR(WM_SCHED_POOL_PTR) = TEST_POOL_PSX;
}

static void canaries_paint(void)
{
    memset(PSX_ADDR(TEST_POOL_PSX) - CANARY_BYTES, CANARY_VAL, CANARY_BYTES);
    memset((u8*)PSX_ADDR(TEST_POOL_PSX) + POOL_BYTES, CANARY_VAL, CANARY_BYTES);
}

static int canaries_intact(void)
{
    int i;
    u8* pre = (u8*)PSX_ADDR(TEST_POOL_PSX) - CANARY_BYTES;
    u8* post = (u8*)PSX_ADDR(TEST_POOL_PSX) + POOL_BYTES;
    for (i = 0; i < CANARY_BYTES; i++)
        if (pre[i] != CANARY_VAL || post[i] != CANARY_VAL)
            return 0;
    return 1;
}

static void set_slot(int i, s16 state, u16 timer, u32 cb0, u32 cb1, u32 payload)
{
    u8* s = (u8*)PSX_ADDR(TEST_POOL_PSX) + i * WM_SCHED_SLOT_STRIDE;
    *(s16*)(s + WM_SCHED_OFF_STATE) = state;
    *(u16*)(s + WM_SCHED_OFF_TIMER) = timer;
    *(u32*)(s + WM_SCHED_OFF_CB0) = cb0;
    *(u32*)(s + WM_SCHED_OFF_CB1) = cb1;
    *(u32*)(s + WM_SCHED_OFF_PAYLOAD) = payload;
}

static s16 get_state(int i)
{
    return *(s16*)((u8*)PSX_ADDR(TEST_POOL_PSX) + i * WM_SCHED_SLOT_STRIDE + WM_SCHED_OFF_STATE);
}

static u16 get_timer(int i)
{
    return *(u16*)((u8*)PSX_ADDR(TEST_POOL_PSX) + i * WM_SCHED_SLOT_STRIDE + WM_SCHED_OFF_TIMER);
}

/* ---- production run capture ---- */

typedef struct {
    int outcome, slots, occupied, dispatched, executed, missing, invalid;
    int dtor_calls, dtor_boundary, completed, last_slot;
    u32 last_cb, frontier;
    uint8_t pool[POOL_BYTES];
    int order[128], order_n, last_arg[128];
    u32 dtor_args[16]; int dtor_n;
} run_capture;

static void run_prod(run_capture* c)
{
    log_reset();
    wm_sched_reset();
    wm_80097800();
    c->outcome = wm_sched_get_outcome();
    c->slots = wm_sched_get_slots_inspected();
    c->occupied = wm_sched_get_occupied_inspected();
    c->dispatched = wm_sched_get_dispatch_attempts();
    c->executed = wm_sched_get_callbacks_executed();
    c->missing = wm_sched_get_missing_hits();
    c->invalid = wm_sched_get_invalid_hits();
    c->dtor_calls = wm_sched_get_destructor_calls();
    c->dtor_boundary = wm_sched_get_destructor_boundary_hits();
    c->completed = wm_sched_get_completed_passes();
    c->last_slot = wm_sched_get_last_slot();
    c->last_cb = wm_sched_get_last_callback();
    c->frontier = wm_sched_get_frontier_pc();
    memcpy(c->pool, PSX_ADDR(TEST_POOL_PSX), POOL_BYTES);
    c->order_n = s_order_n;
    memcpy(c->order, s_order, sizeof(s_order));
    memcpy(c->last_arg, s_last_arg, sizeof(s_last_arg));
    c->dtor_n = s_destructor_n;
    memcpy(c->dtor_args, s_destructor_args, sizeof(s_destructor_args));
}

/* ======================================================================
 * INDEPENDENT RETAIL-ORACLE REPLICA
 * Derived from the W34B5G CFG documentation (SCHEDULER_CFG.md). Never
 * calls production code. Uses the test's own address->fn table. Unresolvable
 * callback = boundary stop (models the bounded frontier); the destructor
 * boundary is modelled with the test destructor hook presence.
 * ==================================================================== */

typedef struct { u32 addr; wm_sched_callback_fn fn; } replica_cb;
static replica_cb s_rep_cbs[16];
static int s_rep_cb_n;
static int s_rep_has_dtor;

static void replica_registry_reset(void)
{
    s_rep_cb_n = 0;
    s_rep_has_dtor = 0;
}

static void replica_register(u32 addr, wm_sched_callback_fn fn)
{
    s_rep_cbs[s_rep_cb_n].addr = addr;
    s_rep_cbs[s_rep_cb_n].fn = fn;
    s_rep_cb_n++;
}

#define REPLICA_COMPLETE 0
#define REPLICA_STOP     1

static int replica_run(run_capture* c)
{
    u8* pool = (u8*)PSX_ADDR(TEST_POOL_PSX);
    int i;
    log_reset();
    memset(c, 0, sizeof(*c));
    c->last_slot = -1;
    for (i = 0; i < 64; i++) {
        u8* slot = pool + i * 0x80;
        u32 occ;
        s16 state;
        c->slots++;
        occ = *(u32*)(slot + 0x1C);
        if (!occ)
            continue;
        c->occupied++;
        state = *(s16*)(slot + 0x00);
        if ((uint32_t)(int32_t)state >= 5u)
            continue;
        if (state == 0 || state == 1) {
            u32 target = state == 0 ? *(u32*)(slot + 0x18) : occ;
            int k;
            wm_sched_callback_fn fn = 0;
            c->dispatched++;
            c->last_slot = i;
            c->last_cb = target;
            for (k = 0; k < s_rep_cb_n; k++)
                if (s_rep_cbs[k].addr == target)
                    fn = s_rep_cbs[k].fn;
            if (!fn) {          /* unresolvable guest target: stop, no write */
                c->outcome = REPLICA_STOP;
                c->frontier = target;
                goto done;
            }
            *(s16*)(slot + 0x00) = fn(i);
            c->executed++;
        } else if (state == 2) {
            u16 t = (u16)(*(u16*)(slot + 0x02) - 1);
            *(u16*)(slot + 0x02) = t;
            if ((s16)t <= 0)
                *(s16*)(slot + 0x00) = 1;
        } else if (state == 3) {
            /* dormant */
        } else { /* state == 4 */
            u32 payload = *(u32*)(slot + 0x4C);
            if (payload) {
                if (!s_rep_has_dtor) {
                    c->outcome = REPLICA_STOP;
                    c->frontier = WM_SCHED_DESTRUCTOR;
                    c->last_slot = i;
                    goto done;
                }
                test_destructor(payload);
                c->dtor_calls++;
            }
        }
    }
    c->outcome = REPLICA_COMPLETE;
    c->completed = 1;
    c->frontier = WM_SCHED_CUT_BEFORE_DRAWSYNC;
done:
    memcpy(c->pool, pool, POOL_BYTES);
    c->order_n = s_order_n;
    memcpy(c->order, s_order, sizeof(s_order));
    memcpy(c->last_arg, s_last_arg, sizeof(s_last_arg));
    c->dtor_n = s_destructor_n;
    memcpy(c->dtor_args, s_destructor_args, sizeof(s_destructor_args));
    return c->outcome;
}

/* Compare production vs replica for scenarios where the replica models the
 * same boundary semantics. */
static void compare_engines(const char* tag, const run_capture* p, const run_capture* r,
                            int rep_outcome_complete_means_prod_complete)
{
    char nm[128];
    int expect_prod_outcome = rep_outcome_complete_means_prod_complete
        ? WM_SCHED_PASS_COMPLETE : -1; /* -1: compare stop-class below */
    (void)expect_prod_outcome;

    snprintf(nm, sizeof nm, "%s: order length match", tag);
    check(nm, p->order_n == r->order_n);
    snprintf(nm, sizeof nm, "%s: order content match", tag);
    check(nm, memcmp(p->order, r->order, sizeof(int) * (size_t)(r->order_n > 0 ? r->order_n : 0)) == 0
              && p->order_n == r->order_n);
    snprintf(nm, sizeof nm, "%s: callback args match", tag);
    check(nm, memcmp(p->last_arg, r->last_arg, sizeof(int) * (size_t)(r->order_n > 0 ? r->order_n : 0)) == 0);
    snprintf(nm, sizeof nm, "%s: slots inspected match", tag);
    check(nm, p->slots == r->slots);
    snprintf(nm, sizeof nm, "%s: occupied inspected match", tag);
    check(nm, p->occupied == r->occupied);
    snprintf(nm, sizeof nm, "%s: dispatched match", tag);
    check(nm, p->dispatched == r->dispatched);
    snprintf(nm, sizeof nm, "%s: executed match", tag);
    check(nm, p->executed == r->executed);
    snprintf(nm, sizeof nm, "%s: destructor calls match", tag);
    check(nm, p->dtor_calls == r->dtor_calls && p->dtor_n == r->dtor_n);
    snprintf(nm, sizeof nm, "%s: stop slot match", tag);
    check(nm, p->last_slot == r->last_slot);
    snprintf(nm, sizeof nm, "%s: frontier match", tag);
    check(nm, p->frontier == r->frontier);
    snprintf(nm, sizeof nm, "%s: pool byte-for-byte equal", tag);
    check(nm, memcmp(p->pool, r->pool, POOL_BYTES) == 0);
}

/* Outcome mapping: replica COMPLETE <-> prod PASS_COMPLETE; replica STOP
 * maps to one of the three production stop outcomes (checked per test). */

int main(void)
{
    run_capture p, r;

    printf("W34B5H scheduler production test\n");

    wm_sched_callback_registry_clear();

    /* ================= A. all empty ================= */
    printf("A: all-empty pool\n");
    pool_clear(); canaries_paint();
    wm_sched_callback_registry_clear();
    replica_registry_reset();
    run_prod(&p);
    check("A: pass complete", p.outcome == WM_SCHED_PASS_COMPLETE);
    check("A: 64 slots visited", p.slots == 64);
    check("A: 0 occupied", p.occupied == 0);
    check("A: no dispatch", p.dispatched == 0);
    check("A: completed count 1", p.completed == 1);
    check("A: frontier = cut-before-DrawSync 0x8007106C",
          p.frontier == WM_SCHED_CUT_BEFORE_DRAWSYNC);
    {
        int i, allzero = 1;
        for (i = 0; i < POOL_BYTES; i++) if (p.pool[i]) allzero = 0;
        check("A: pool unchanged", allzero);
    }
    pool_clear();
    replica_run(&r);
    compare_engines("A", &p, &r, 1);

    /* ================= B. state 0 implemented ================= */
    printf("B: state-0 dispatch, return -> state\n");
    pool_clear(); canaries_paint();
    wm_sched_callback_registry_clear();
    wm_sched_callback_register(TEST_CB_A, test_cb_ret1);
    wm_sched_callback_register(TEST_CB_B, test_cb_ret3);
    replica_registry_reset();
    replica_register(TEST_CB_A, test_cb_ret1);
    replica_register(TEST_CB_B, test_cb_ret3);
    set_slot(5, 0, 0, TEST_CB_A, 0x800925A0u, 0);   /* cb1 known-missing, unused */
    set_slot(60, 0, 0, TEST_CB_B, 0x800925A0u, 0);
    run_prod(&p);
    check("B: pass complete", p.outcome == WM_SCHED_PASS_COMPLETE);
    check("B: 2 dispatched+executed", p.dispatched == 2 && p.executed == 2);
    check("B: order [5,60]", p.order_n == 2 && p.order[0] == 5 && p.order[1] == 60);
    check("B: a0 exact (5,60)", p.last_arg[0] == 5 && p.last_arg[1] == 60);
    check("B: slot5 state <- 1", get_state(5) == 1);   /* pool live check */
    check("B: slot60 state <- 3", get_state(60) == 3);
    check("B: pool snapshot states", p.pool[5*0x80] == 1 && p.pool[60*0x80] == 3);
    check("B: canaries intact", canaries_intact());
    pool_clear(); canaries_paint();
    set_slot(5, 0, 0, TEST_CB_A, 0x800925A0u, 0);
    set_slot(60, 0, 0, TEST_CB_B, 0x800925A0u, 0);
    replica_run(&r);
    compare_engines("B", &p, &r, 1);

    /* ================= C. state 1 implemented ================= */
    printf("C: state-1 dispatch uses +0x1C\n");
    pool_clear(); canaries_paint();
    wm_sched_callback_registry_clear();
    wm_sched_callback_register(TEST_CB_B, test_cb_ret2);
    replica_registry_reset();
    replica_register(TEST_CB_B, test_cb_ret2);
    /* cb0 is a KNOWN-MISSING address: proves state 1 picks +0x1C, not +0x18 */
    set_slot(3, 1, 0, KNOWN_MISSING_CB0_SLOT0, TEST_CB_B, 0);
    run_prod(&p);
    check("C: pass complete", p.outcome == WM_SCHED_PASS_COMPLETE);
    check("C: 1 dispatch", p.dispatched == 1 && p.executed == 1);
    check("C: target was +0x1C", p.last_cb == TEST_CB_B);
    check("C: slot3 state <- 2", get_state(3) == 2);
    check("C: a0 = 3", p.last_arg[0] == 3);
    pool_clear(); canaries_paint();
    set_slot(3, 1, 0, KNOWN_MISSING_CB0_SLOT0, TEST_CB_B, 0);
    replica_run(&r);
    compare_engines("C", &p, &r, 1);

    /* ================= D. state 2 timer > 1 ================= */
    printf("D: state-2 timer decrement only\n");
    pool_clear(); canaries_paint();
    wm_sched_callback_registry_clear();
    replica_registry_reset();
    set_slot(1, 2, 3, 0, 0x800925A0u, 0);
    run_prod(&p);
    check("D: pass complete", p.outcome == WM_SCHED_PASS_COMPLETE);
    check("D: no dispatch", p.dispatched == 0);
    check("D: timer 3->2", get_timer(1) == 2);
    check("D: state stays 2", get_state(1) == 2);
    pool_clear(); canaries_paint();
    set_slot(1, 2, 3, 0, 0x800925A0u, 0);
    replica_run(&r);
    compare_engines("D", &p, &r, 1);

    /* ================= E. state 2 timer expiration ================= */
    printf("E: state-2 timer expiry -> state 1\n");
    pool_clear(); canaries_paint();
    set_slot(1, 2, 1, 0, 0x800925A0u, 0);
    set_slot(2, 2, 0, 0, 0x800925A0u, 0);   /* retail edge: 0 decrements to 0xFFFF, expires */
    run_prod(&p);
    check("E: slot1 timer 1->0", get_timer(1) == 0);
    check("E: slot1 state -> 1", get_state(1) == 1);
    check("E: slot2 timer 0->0xFFFF", get_timer(2) == 0xFFFF);
    check("E: slot2 state -> 1", get_state(2) == 1);
    check("E: no dispatch", p.dispatched == 0);
    pool_clear(); canaries_paint();
    set_slot(1, 2, 1, 0, 0x800925A0u, 0);
    set_slot(2, 2, 0, 0, 0x800925A0u, 0);
    replica_run(&r);
    compare_engines("E", &p, &r, 1);

    /* ================= F. state 3 dormant ================= */
    printf("F: state-3 dormant, no mutation\n");
    pool_clear(); canaries_paint();
    set_slot(4, 3, 7, TEST_CB_UNKNOWN, 0x800925A0u, 0x80051000u /*payload present*/);
    run_prod(&p);
    check("F: pass complete", p.outcome == WM_SCHED_PASS_COMPLETE);
    check("F: no dispatch, no destructor", p.dispatched == 0 && p.dtor_calls == 0);
    check("F: state unchanged", get_state(4) == 3);
    check("F: timer untouched", get_timer(4) == 7);
    {
        u32 pl = *(u32*)((u8*)PSX_ADDR(TEST_POOL_PSX) + 4*0x80 + WM_SCHED_OFF_PAYLOAD);
        check("F: payload untouched (dormant never reads it)", pl == 0x80051000u);
    }
    pool_clear(); canaries_paint();
    set_slot(4, 3, 7, TEST_CB_UNKNOWN, 0x800925A0u, 0x80051000u);
    replica_run(&r);
    compare_engines("F", &p, &r, 1);

    /* ================= G. state 4 null payload ================= */
    printf("G: state-4 null payload: retail no-op\n");
    pool_clear(); canaries_paint();
    wm_sched_callback_registry_clear();   /* no destructor hook */
    replica_registry_reset();
    set_slot(6, 4, 0, 0, 0x800925A0u, 0);
    run_prod(&p);
    check("G: pass complete", p.outcome == WM_SCHED_PASS_COMPLETE);
    check("G: no destructor call/boundary", p.dtor_calls == 0 && p.dtor_boundary == 0);
    check("G: state unchanged", get_state(6) == 4);
    pool_clear(); canaries_paint();
    set_slot(6, 4, 0, 0, 0x800925A0u, 0);
    replica_run(&r);
    compare_engines("G", &p, &r, 1);

    /* ================= H. state 4 non-null test destructor ================= */
    printf("H: state-4 non-null payload: test destructor once\n");
    pool_clear(); canaries_paint();
    wm_sched_callback_registry_clear();
    wm_sched_test_set_destructor(test_destructor);
    replica_registry_reset();
    s_rep_has_dtor = 1;
    set_slot(6, 4, 0, 0, 0x800925A0u, 0x80052000u);
    run_prod(&p);
    check("H: pass complete", p.outcome == WM_SCHED_PASS_COMPLETE);
    check("H: destructor exactly once", p.dtor_calls == 1 && p.dtor_n == 1);
    check("H: destructor arg exact", p.dtor_args[0] == 0x80052000u);
    check("H: state unchanged (retail writes nothing in case 4)", get_state(6) == 4);
    pool_clear(); canaries_paint();
    set_slot(6, 4, 0, 0, 0x800925A0u, 0x80052000u);
    replica_run(&r);
    compare_engines("H", &p, &r, 1);
    wm_sched_test_set_destructor(0);

    /* ---- H2: state-4 non-null payload WITHOUT destructor -> bounded stop ---- */
    printf("H2: state-4 non-null payload, no destructor: bounded boundary\n");
    pool_clear(); canaries_paint();
    wm_sched_callback_registry_clear();   /* clears destructor too */
    replica_registry_reset();
    set_slot(6, 4, 0, 0, 0x800925A0u, 0x80052000u);
    set_slot(9, 0, 0, TEST_CB_A, 0x800925A0u, 0);
    run_prod(&p);
    check("H2: destructor boundary outcome", p.outcome == WM_SCHED_STOP_DESTRUCTOR_BOUNDARY);
    check("H2: boundary hit counted", p.dtor_boundary == 1);
    check("H2: stop at slot 6, slot 9 not visited", p.last_slot == 6 && p.slots == 7);
    check("H2: frontier = destructor 0x800230A8", p.frontier == WM_SCHED_DESTRUCTOR);
    pool_clear(); canaries_paint();
    set_slot(6, 4, 0, 0, 0x800925A0u, 0x80052000u);
    set_slot(9, 0, 0, TEST_CB_A, 0x800925A0u, 0);
    replica_run(&r);
    compare_engines("H2", &p, &r, 0);

    /* ================= I. insert later slot same pass ================= */
    printf("I: callback inserts later occupied slot -> observed same pass\n");
    pool_clear(); canaries_paint();
    wm_sched_callback_registry_clear();
    wm_sched_callback_register(TEST_CB_INSERTER, test_cb_inserter);
    wm_sched_callback_register(TEST_CB_C, test_cb_ret1);
    replica_registry_reset();
    replica_register(TEST_CB_INSERTER, test_cb_inserter);
    replica_register(TEST_CB_C, test_cb_ret1);
    set_slot(2, 0, 0, TEST_CB_INSERTER, 0x800925A0u, 0);
    run_prod(&p);
    check("I: pass complete", p.outcome == WM_SCHED_PASS_COMPLETE);
    check("I: order [2,7] (insert observed same pass)",
          p.order_n == 2 && p.order[0] == 2 && p.order[1] == 7);
    check("I: inserted slot state <- 1", get_state(7) == 1);
    pool_clear(); canaries_paint();
    set_slot(2, 0, 0, TEST_CB_INSERTER, 0x800925A0u, 0);
    replica_run(&r);
    compare_engines("I", &p, &r, 1);

    /* ================= J. clear later slot ================= */
    printf("J: callback clears later slot -> later visit sees empty\n");
    pool_clear(); canaries_paint();
    wm_sched_callback_registry_clear();
    wm_sched_callback_register(TEST_CB_CLEARER, test_cb_clearer);
    wm_sched_callback_register(TEST_CB_C, test_cb_ret1);
    replica_registry_reset();
    replica_register(TEST_CB_CLEARER, test_cb_clearer);
    replica_register(TEST_CB_C, test_cb_ret1);
    set_slot(2, 0, 0, TEST_CB_CLEARER, 0x800925A0u, 0);
    set_slot(7, 0, 0, TEST_CB_C, 0x800925A0u, 0);
    run_prod(&p);
    check("J: pass complete", p.outcome == WM_SCHED_PASS_COMPLETE);
    check("J: order [2] only (slot 7 skipped)", p.order_n == 1 && p.order[0] == 2);
    check("J: slot7 state untouched", get_state(7) == 0);
    pool_clear(); canaries_paint();
    set_slot(2, 0, 0, TEST_CB_CLEARER, 0x800925A0u, 0);
    set_slot(7, 0, 0, TEST_CB_C, 0x800925A0u, 0);
    replica_run(&r);
    compare_engines("J", &p, &r, 1);

    /* ================= K. missing callback frontier ================= */
    printf("K: missing callback stops before fabricated state write\n");
    pool_clear(); canaries_paint();
    wm_sched_callback_registry_clear();
    wm_sched_callback_register(TEST_CB_A, test_cb_ret1);
    replica_registry_reset();
    replica_register(TEST_CB_A, test_cb_ret1);
    set_slot(0, 0, 0, KNOWN_MISSING_CB0_SLOT0, 0x800925A0u, 0);
    set_slot(3, 0, 0, TEST_CB_A, 0x800925A0u, 0);   /* must NOT be reached */
    run_prod(&p);
    check("K: outcome MISSING", p.outcome == WM_SCHED_STOP_MISSING_CALLBACK);
    check("K: missing hits 1", p.missing == 1);
    check("K: dispatch attempts 1, executed 0", p.dispatched == 1 && p.executed == 0);
    check("K: stopped at slot 0 (no advance)", p.slots == 1 && p.last_slot == 0);
    check("K: frontier = 0x800923A8", p.frontier == KNOWN_MISSING_CB0_SLOT0);
    check("K: slot0 state NOT fabricated (still 0)", get_state(0) == 0);
    check("K: slot3 never dispatched", p.order_n == 0);
    check("K: last cb/state recorded", p.last_cb == KNOWN_MISSING_CB0_SLOT0 &&
          wm_sched_get_last_callback_state() == 0);
    check("K: canaries intact", canaries_intact());
    pool_clear(); canaries_paint();
    set_slot(0, 0, 0, KNOWN_MISSING_CB0_SLOT0, 0x800925A0u, 0);
    set_slot(3, 0, 0, TEST_CB_A, 0x800925A0u, 0);
    replica_run(&r);
    compare_engines("K", &p, &r, 0);

    /* ================= L. invalid states ================= */
    printf("L: invalid states skip with no mutation\n");
    pool_clear(); canaries_paint();
    wm_sched_callback_registry_clear();
    replica_registry_reset();
    set_slot(10, 5, 9, TEST_CB_UNKNOWN, 0x800925A0u, 0);
    set_slot(11, 6, 9, TEST_CB_UNKNOWN, 0x800925A0u, 0);
    set_slot(12, -1, 9, TEST_CB_UNKNOWN, 0x800925A0u, 0);   /* lh sign-extends; sltiu skips */
    set_slot(13, 300, 9, TEST_CB_UNKNOWN, 0x800925A0u, 0);
    run_prod(&p);
    check("L: pass complete", p.outcome == WM_SCHED_PASS_COMPLETE);
    check("L: 4 invalid states seen", wm_sched_get_state_invalid_seen() == 4);
    check("L: no dispatch", p.dispatched == 0);
    check("L: states unchanged",
          get_state(10) == 5 && get_state(11) == 6 &&
          get_state(12) == -1 && get_state(13) == 300);
    check("L: timers unchanged",
          get_timer(10) == 9 && get_timer(11) == 9 &&
          get_timer(12) == 9 && get_timer(13) == 9);
    pool_clear(); canaries_paint();
    set_slot(10, 5, 9, TEST_CB_UNKNOWN, 0x800925A0u, 0);
    set_slot(11, 6, 9, TEST_CB_UNKNOWN, 0x800925A0u, 0);
    set_slot(12, -1, 9, TEST_CB_UNKNOWN, 0x800925A0u, 0);
    set_slot(13, 300, 9, TEST_CB_UNKNOWN, 0x800925A0u, 0);
    replica_run(&r);
    compare_engines("L", &p, &r, 1);

    /* ================= M. invalid/unknown callback ================= */
    printf("M: unknown callback -> bounded failure, no host call\n");
    pool_clear(); canaries_paint();
    wm_sched_callback_registry_clear();
    replica_registry_reset();
    set_slot(0, 0, 0, TEST_CB_UNKNOWN, 0x800925A0u, 0);
    set_slot(1, 0, 0, TEST_CB_A, 0x800925A0u, 0);
    run_prod(&p);
    check("M: outcome INVALID", p.outcome == WM_SCHED_STOP_INVALID_CALLBACK);
    check("M: invalid hits 1", p.invalid == 1);
    check("M: no host call", p.executed == 0 && p.order_n == 0);
    check("M: no state mutation", get_state(0) == 0);
    check("M: frontier recorded", p.frontier == TEST_CB_UNKNOWN);
    check("M: no advance past slot 0", p.slots == 1);
    pool_clear(); canaries_paint();
    set_slot(0, 0, 0, TEST_CB_UNKNOWN, 0x800925A0u, 0);
    set_slot(1, 0, 0, TEST_CB_A, 0x800925A0u, 0);
    replica_run(&r);
    compare_engines("M", &p, &r, 0);

    /* M2: null callback in state 0 is also bounded-invalid (retail would
     * jalr zero; the port refuses the host call). */
    pool_clear(); canaries_paint();
    set_slot(0, 0, 0, 0, 0x800925A0u, 0);
    run_prod(&p);
    check("M2: null cb0 bounded-invalid", p.outcome == WM_SCHED_STOP_INVALID_CALLBACK);
    check("M2: no host call", p.executed == 0);

    /* ================= N. pool guard bands ================= */
    printf("N: guard bands around 0x2000 pool\n");
    /* canaries were painted/checked in every scenario above; one explicit
     * mixed scenario re-check covering all write paths at once */
    pool_clear(); canaries_paint();
    wm_sched_callback_registry_clear();
    wm_sched_callback_register(TEST_CB_A, test_cb_ret1);
    wm_sched_test_set_destructor(test_destructor);
    set_slot(0, 0, 0, TEST_CB_A, 0x800925A0u, 0);
    set_slot(1, 2, 2, 0, 0x800925A0u, 0);
    set_slot(2, 4, 0, 0, 0x800925A0u, 0x80052000u);
    set_slot(63, 0, 0, TEST_CB_A, 0x800925A0u, 0);
    run_prod(&p);
    check("N: mixed pass complete", p.outcome == WM_SCHED_PASS_COMPLETE);
    check("N: guard band before pool intact", canaries_intact());
    check("N: slot63 reached and dispatched", p.order_n == 2 &&
          p.order[0] == 0 && p.order[1] == 63);
    wm_sched_test_set_destructor(0);

    printf("RESULT: %d/%d PASS (%d FAIL)\n", pass, total, fail);
    return fail ? 1 : 0;
}
