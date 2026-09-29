/* W34B27 production-linked certificate for retail
 * 0x800714D4..0x80071698, the first state-gate continuation after the
 * post-pass display setup. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_frame_driver.h"

uint8_t g_PsxRam[PSX_RAM_SIZE];
uint8_t g_PsxScratchpad[4096];
u16 g_C1ButtonState, g_C2ButtonState;
u16 g_C1ButtonStateReleased, g_C2ButtonStateReleased;
u16 g_C1ButtonStatePressedOnce, g_C2ButtonStatePressedOnce;
u32 g_ArchiveDebugTable;

#define ENVREC0 0x8009BBC8u
#define ENVREC1 0x8009BC40u
#define DB_PTR  0x8009BE3Cu
#define INDEX   0x8009D7F0u
#define D554    0x8009D554u
#define BD34    0x8009BD34u
#define C178    0x8009C178u
#define D804    0x8009D804u
#define BD24    0x8009BD24u
#define CE68    0x8009CE68u
#define D80C    0x8009D80Cu

static int s_pass, s_fail, s_total;
static int s_tail_calls;

void wm_712d0_test_trace(u32 pc, u32 kind, u32 address,
                         u32 width, u32 value)
{
    (void)pc; (void)kind; (void)address; (void)width; (void)value;
}

static void check_case(const char *name, const char *assertion, int ok)
{
    s_total++;
    if (ok) s_pass++;
    else {
        s_fail++;
        printf("FAIL [%s]: ASSERTION %s\n", name, assertion);
    }
}

static void store_u8(u32 address, u8 value)
{
    *(u8 *)PSX_ADDR(address) = value;
}

static void store_u16(u32 address, u16 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static void store_u32(u32 address, u32 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static u32 load_u32(u32 address)
{
    u32 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

int ControllerPopState(void) { return 0; }
int VSync(int mode) { (void)mode; s_tail_calls++; return 0; }
int CdSync(int mode, u8 *result) { (void)mode; (void)result; return 0; }
u32 *ClearOTagR(u32 *ot, int n) { (void)n; return ot; }
void func_800250E0(int context) { (void)context; }
void func_8001D468(void) { }
u32 wm_800967E4_dispatch_cd_work(void) { return 0; }
void wm_80097800(void) { }
int DrawSync(int mode) { (void)mode; s_tail_calls++; return 0; }
void GameCheckAndHandleSoftReset(void) { s_tail_calls++; }
void PutDispEnv(void *env) { (void)env; s_tail_calls++; }
void PutDrawEnv(void *env) { (void)env; s_tail_calls++; }

static void reset_fixture(void)
{
    memset(g_PsxRam, 0, sizeof(g_PsxRam));
    memset(g_PsxScratchpad, 0, sizeof(g_PsxScratchpad));
    s_tail_calls = 0;
    store_u32(ENVREC0 + 0x70u, 0x800F0000u);
    store_u32(ENVREC1 + 0x70u, 0x800F1000u);
    wm_fp_reset();
}

static void run_natural(void)
{
    reset_fixture();
    wm_800712D0_frame_prologue();
    check_case("natural-bd34-zero", "cut-0x8007169C",
               wm_fp_get_cut_pc() == 0x8007169Cu);
    check_case("natural-bd34-zero", "common-store-bd34-zero",
               load_u32(BD34) == 0u);
    check_case("natural-bd34-zero", "post-pass-tail-executed",
               s_tail_calls == 5);
}

static void run_all_gates_pass(void)
{
    reset_fixture();
    store_u8(0x80069179u, 0u);
    store_u32(BD34, 1u);
    store_u32(C178, 0u);
    store_u32(D804, 0u);
    store_u16(BD24, 0xFFFFu);
    store_u16(CE68, 0xFFFFu);
    store_u32(D80C, 0u);
    wm_800712D0_frame_prologue();
    check_case("all-gates-pass", "cut-before-0x80093F18",
               wm_fp_get_cut_pc() == 0x80071578u);
    check_case("all-gates-pass", "bd34-cleared-before-call-frontier",
               load_u32(BD34) == 0u);
    check_case("all-gates-pass", "tail-still-executed-on-gated-path",
               s_tail_calls == 5);
}

static void run_first_gate_fail(void)
{
    reset_fixture();
    store_u8(0x80069179u, 1u);
    store_u32(BD34, 1u);
    wm_800712D0_frame_prologue();
    check_case("first-gate-fail", "branch-target-frontier-0x8007169C",
               wm_fp_get_cut_pc() == 0x8007169Cu);
    check_case("first-gate-fail", "branch-target-clears-bd34",
               load_u32(BD34) == 0u);
}

int main(void)
{
    printf("=== W34B27 0x800714D4 state-gate continuation ===\n");
    printf("RETAIL_BOUNDARY [0x800714D4,0x8007169C) bytes=456 insns=114\n");
    run_natural();
    run_all_gates_pass();
    run_first_gate_fail();
    printf("=== Results: %d/%d PASS ===\n", s_pass, s_total);
    return s_fail == 0 ? 0 : 1;
}
