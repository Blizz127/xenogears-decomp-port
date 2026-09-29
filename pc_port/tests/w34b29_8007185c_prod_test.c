/* W34B29 production-linked certificate for retail
 * 0x8007185C..0x80071978, including the natural flag/update lane. */
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
#define BD10    0x8009BD10u
#define C178    0x8009C178u
#define D554    0x8009D554u
#define D804    0x8009D804u
#define D80C    0x8009D80Cu
#define BE10    0x8009BE10u
#define EE76    0x8007EE76u

static int s_pass, s_fail, s_total;
static int s_tail_calls;
static int s_pop_calls;

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

static void store_u16(u32 address, u16 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static void store_u32(u32 address, u32 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static u16 load_u16(u32 address)
{
    u16 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static u32 load_u32(u32 address)
{
    u32 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static void reset_fixture(void)
{
    memset(g_PsxRam, 0, sizeof(g_PsxRam));
    memset(g_PsxScratchpad, 0, sizeof(g_PsxScratchpad));
    s_tail_calls = 0;
    s_pop_calls = 0;
    g_C1ButtonState = 0;
    g_C2ButtonState = 0;
    g_C1ButtonStateReleased = 0;
    g_C2ButtonStateReleased = 0;
    g_C1ButtonStatePressedOnce = 0;
    g_C2ButtonStatePressedOnce = 0;
    store_u32(ENVREC0 + 0x70u, 0x800F0000u);
    store_u32(ENVREC1 + 0x70u, 0x800F1000u);
    wm_fp_reset();
}

static void run_natural_toggle(void)
{
    reset_fixture();
    store_u16(BD10, 0x0100u);
    store_u16(EE76, 2u);
    store_u32(C178, 1u);
    g_C1ButtonStateReleased = 0x0100u;
    wm_800712D0_frame_prologue();
    check_case("natural-toggle", "frontier-0x8007197C",
               wm_fp_get_cut_pc() == 0x8007197Cu);
    check_case("natural-toggle", "d80c-cleared", load_u32(D80C) == 0u);
    check_case("natural-toggle", "d804-cleared", load_u32(D804) == 0u);
    check_case("natural-toggle", "ee76-xori-one", load_u16(EE76) == 3u);
    check_case("natural-toggle", "tail-executed", s_tail_calls == 5);
}

static void run_no_toggle_and_alternate_call(void)
{
    reset_fixture();
    store_u16(BD10, 0u);
    store_u16(EE76, 0x1234u);
    store_u32(C178, 1u);
    wm_800712D0_frame_prologue();
    check_case("natural-no-toggle", "ee76-untouched", load_u16(EE76) == 0x1234u);

    reset_fixture();
    store_u32(D804, 1u);
    store_u32(D554, 1u);
    store_u32(BE10, 2u);
    wm_800712D0_frame_prologue();
    check_case("alternate-call-lane", "first-helper-frontier-0x800718E8",
               wm_fp_get_cut_pc() == 0x800718E8u);
}

static void run_boundary_and_width_canaries(void)
{
    reset_fixture();
    store_u32(D804, 1u);
    store_u32(D554, 1u);
    store_u32(BE10, 4u);
    wm_800712D0_frame_prologue();
    check_case("be10-four", "no-helper-at-upper-bound",
               wm_fp_get_cut_pc() == 0x8007197Cu);

    reset_fixture();
    store_u32(D804, 1u);
    store_u32(D554, 1u);
    store_u32(BE10, 0xFFFFFFFFu);
    wm_800712D0_frame_prologue();
    check_case("be10-negative", "signed-be10-falls-through",
               wm_fp_get_cut_pc() == 0x8007197Cu);

    reset_fixture();
    store_u32(D804, 1u);
    store_u32(D554, 1u);
    store_u32(BE10, 2u);
    store_u16(C178 + 1u, 1u);
    wm_800712D0_frame_prologue();
    check_case("c178-lw-width", "high-byte-makes-c178-nonzero",
               wm_fp_get_cut_pc() == 0x8007197Cu);
}

int ControllerPopState(void) { return s_pop_calls++ == 0 ? 1 : 0; }
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
    printf("=== W34B29 0x8007185C flag/update lane ===\n");
    printf("RETAIL_BOUNDARY [0x8007185C,0x8007197C) bytes=288 insns=72\n");
    run_natural_toggle();
    run_no_toggle_and_alternate_call();
    run_boundary_and_width_canaries();
    printf("=== Results: %d/%d PASS ===\n", s_pass, s_total);
    return s_fail == 0 ? 0 : 1;
}
