/* W34B30 production-linked certificate for the mapped 0x80025044 call
 * at retail 0x8007197C. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_image_transfer_25044.h"

typedef struct {
    int16_t x;
    int16_t y;
    int16_t w;
    int16_t h;
} RECT;
typedef unsigned long u_long;
typedef unsigned char u_char;

uint8_t g_PsxRam[PSX_RAM_SIZE];
uint8_t g_PsxScratchpad[4096];
u16 g_C1ButtonState, g_C2ButtonState;
u16 g_C1ButtonStateReleased, g_C2ButtonStateReleased;
u16 g_C1ButtonStatePressedOnce, g_C2ButtonStatePressedOnce;
u32 g_ArchiveDebugTable;
s32 g_GfxCurContext;
u32 g_GfxImageList[2];

#define ENVREC0 0x8009BBC8u
#define ENVREC1 0x8009BC40u
#define C178    0x8009C178u
#define NODE0   0x800F2000u
#define NODE1   0x800F2040u
#define DATA0   0x800F3000u

static int s_pass, s_fail, s_total;
static int s_load_calls, s_clear_calls;
static RECT *s_last_rect;
static u_long *s_last_data;

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

static void store_u32(u32 address, u32 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static void reset_fixture(void)
{
    memset(g_PsxRam, 0, sizeof(g_PsxRam));
    memset(g_PsxScratchpad, 0, sizeof(g_PsxScratchpad));
    memset(g_GfxImageList, 0, sizeof(g_GfxImageList));
    g_GfxCurContext = 0;
    s_load_calls = 0;
    s_clear_calls = 0;
    s_last_rect = NULL;
    s_last_data = NULL;
    store_u32(ENVREC0 + 0x70u, 0x800F0000u);
    store_u32(ENVREC1 + 0x70u, 0x800F1000u);
    store_u32(C178, 1u);
    wm_25044_reset();
}

static void run_empty_and_unknown(void)
{
    reset_fixture();
    wm_80025044_guest_safe();
    check_case("empty-list", "empty-list-consumed",
               wm_25044_get_unknowns() == 0 && g_GfxImageList[0] == 0u);

    reset_fixture();
    g_GfxImageList[0] = 0x12345678u;
    wm_80025044_guest_safe();
    check_case("unknown-head", "log-count-and-clear",
               wm_25044_get_unknowns() == 1 && g_GfxImageList[0] == 0u);
}

static void run_known_load_and_clear(void)
{
    reset_fixture();
    g_GfxImageList[0] = NODE0;
    store_u32(NODE0 + 0x08u, DATA0);
    store_u32(NODE0 + 0x0Cu, 0u);
    wm_80025044_guest_safe();
    check_case("known-load", "map-guest-rect-and-data",
               s_load_calls == 1 && s_clear_calls == 0 &&
               s_last_rect == (RECT *)PSX_ADDR(NODE0) &&
               s_last_data == (u_long *)PSX_ADDR(DATA0));

    reset_fixture();
    g_GfxImageList[0] = NODE0;
    store_u32(NODE0 + 0x08u, 0u);
    store_u32(NODE0 + 0x0Cu, 0u);
    wm_80025044_guest_safe();
    check_case("known-clear", "clear-null-data-image",
               s_load_calls == 0 && s_clear_calls == 1);
}

static void run_unknown_data_and_chain(void)
{
    reset_fixture();
    g_GfxImageList[0] = NODE0;
    store_u32(NODE0 + 0x08u, 0x123u);
    store_u32(NODE0 + 0x0Cu, 0u);
    wm_80025044_guest_safe();
    check_case("unknown-data", "log-count-and-clear",
               wm_25044_get_unknowns() == 1 && s_load_calls == 0 &&
               g_GfxImageList[0] == 0u);

    reset_fixture();
    g_GfxImageList[0] = NODE0;
    store_u32(NODE0 + 0x08u, DATA0);
    store_u32(NODE0 + 0x0Cu, NODE1);
    store_u32(NODE1 + 0x08u, DATA0 + 4u);
    store_u32(NODE1 + 0x0Cu, 0u);
    wm_80025044_guest_safe();
    check_case("known-chain", "walk-two-mapped-images",
               wm_25044_get_unknowns() == 0 && s_load_calls == 2);
}

static void run_bad_context(void)
{
    reset_fixture();
    g_GfxCurContext = 2;
    wm_80025044_guest_safe();
    check_case("bad-context", "log-unknown-context",
               wm_25044_get_unknowns() == 1);
}

static void run_native_in_guest_ram(void)
{
    u8 *node = (u8 *)PSX_ADDR(NODE0);
    u8 *data = (u8 *)PSX_ADDR(DATA0);

    reset_fixture();
    g_GfxImageList[0] = (u32)(uintptr_t)node;
    store_u32(NODE0 + 0x08u, (u32)(uintptr_t)data);
    store_u32(NODE0 + 0x0Cu, 0u);
    wm_80025044_guest_safe();
    check_case("native-in-guest-ram", "resolve-node-and-data",
               wm_25044_get_unknowns() == 0 && s_load_calls == 1 &&
               (void *)s_last_rect == (void *)node &&
               (void *)s_last_data == (void *)data &&
               g_GfxImageList[0] == 0u);
}

int ControllerPopState(void) { return 0; }
int VSync(int mode) { (void)mode; return 0; }
int CdSync(int mode, u8 *result) { (void)mode; (void)result; return 0; }
u32 *ClearOTagR(u32 *ot, int n) { (void)n; return ot; }
void func_800250E0(int context) { (void)context; }
void func_8001D468(void) { }
u32 wm_800967E4_dispatch_cd_work(void) { return 0; }
void wm_80097800(void) { }
int DrawSync(int mode) { (void)mode; return 0; }
void GameCheckAndHandleSoftReset(void) { }
void PutDispEnv(void *env) { (void)env; }
void PutDrawEnv(void *env) { (void)env; }
int LoadImage(RECT *rect, u_long *data)
{
    s_load_calls++;
    s_last_rect = rect;
    s_last_data = data;
    return 0;
}
int ClearImage(RECT *rect, u_char r, u_char g, u_char b)
{
    (void)rect; (void)r; (void)g; (void)b;
    s_clear_calls++;
    return 0;
}

int main(void)
{
    printf("=== W34B30 0x8007197C mapped image transfer ===\n");
    printf("RETAIL_BOUNDARY [0x8007197C,0x80071984) bytes=8 insns=2\n");
    run_empty_and_unknown();
    run_known_load_and_clear();
    run_unknown_data_and_chain();
    run_bad_context();
    run_native_in_guest_ram();
    printf("=== Results: %d/%d PASS ===\n", s_pass, s_total);
    return s_fail == 0 ? 0 : 1;
}
