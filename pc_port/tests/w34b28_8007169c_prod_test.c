/* W34B28 production-linked certificate for the natural
 * 0x8007169C..0x800716A8 C178 branch. */
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
#define C178    0x8009C178u

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

static void store_u32(u32 address, u32 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static void reset_fixture(void)
{
    memset(g_PsxRam, 0, sizeof(g_PsxRam));
    memset(g_PsxScratchpad, 0, sizeof(g_PsxScratchpad));
    s_tail_calls = 0;
    store_u32(ENVREC0 + 0x70u, 0x800F0000u);
    store_u32(ENVREC1 + 0x70u, 0x800F1000u);
    wm_fp_reset();
}

static void run_c178_one(void)
{
    reset_fixture();
    store_u32(C178, 1u);
    wm_800712D0_frame_prologue();
    check_case("c178-one", "branch-target-frontier-0x8007185C",
               wm_fp_get_cut_pc() == 0x8007185Cu);
    check_case("c178-one", "post-pass-tail-executed", s_tail_calls == 5);
}

static void run_c178_zero(void)
{
    reset_fixture();
    wm_800712D0_frame_prologue();
    check_case("c178-zero", "first-helper-frontier-0x80071704",
               wm_fp_get_cut_pc() == 0x80071704u);
}

static void run_width_and_offset_canaries(void)
{
    reset_fixture();
    store_u8(C178 - 1u, 1u);
    wm_800712D0_frame_prologue();
    check_case("c178-offset-canary", "fresh-base-load-is-used",
               wm_fp_get_cut_pc() == 0x80071704u);

    reset_fixture();
    store_u8(C178 + 1u, 1u);
    wm_800712D0_frame_prologue();
    check_case("c178-width-canary", "retail-lw-sees-high-byte",
               wm_fp_get_cut_pc() == 0x8007185Cu);

    reset_fixture();
    store_u32(C178, 0x80000000u);
    wm_800712D0_frame_prologue();
    check_case("c178-sign-canary", "u32-nonzero-branch",
               wm_fp_get_cut_pc() == 0x8007185Cu);
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

int main(void)
{
    printf("=== W34B28 0x8007169C C178 branch ===\n");
    printf("RETAIL_BOUNDARY [0x8007169C,0x800716AC) bytes=16 insns=4\n");
    run_c178_one();
    run_c178_zero();
    run_width_and_offset_canaries();
    printf("=== Results: %d/%d PASS ===\n", s_pass, s_total);
    return s_fail == 0 ? 0 : 1;
}
