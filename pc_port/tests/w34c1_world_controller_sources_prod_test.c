/* Focused production-linked certificate for the retail controller drain in
 * wm_800712D0 (0x8007134C..0x800713F4). */
#include <setjmp.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "system/controller.h"
#include "world_map_frame_driver_712d0.h"

#define D_8009CD4C UINT32_C(0x8009CD4C)
#define D_8009CD50 UINT32_C(0x8009CD50)
#define D_8009BD10 UINT32_C(0x8009BD10)
#define D_8009BD14 UINT32_C(0x8009BD14)
#define D_8009BD18 UINT32_C(0x8009BD18)
#define D_8009BD1C UINT32_C(0x8009BD1C)

typedef struct ControllerSample {
    u16 c1_held;
    u16 c2_held;
    u16 c1_released;
    u16 c2_released;
    u16 c1_pressed_once;
    u16 c2_pressed_once;
} ControllerSample;

uint8_t g_PsxRam[PSX_RAM_SIZE];
uint8_t g_PsxScratchpad[4096];

u_short g_C1ButtonState;
u_short g_C2ButtonState;
u_short g_C1ButtonStateReleased;
u_short g_C2ButtonStateReleased;
u_short g_C1ButtonStatePressedOnce;
u_short g_C2ButtonStatePressedOnce;
u8 D_8005954C;
u8 D_80059179;
u8 D_80059460;
u8 D_80059171;
u8 g_MenuDebugEnabled;

static jmp_buf s_after_drain;
static const ControllerSample *s_samples;
static size_t s_sample_count;
static size_t s_sample_index;

static void write_u16(u32 address, u16 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static u16 read_u16(u32 address)
{
    u16 value;

    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static int expect_u16(const char *assertion, u32 address, u16 expected)
{
    u16 actual = read_u16(address);

    if (actual != expected) {
        fprintf(stderr,
                "ASSERTION %s address=0x%08x expected=0x%04x actual=0x%04x\n",
                assertion, address, (unsigned int)expected,
                (unsigned int)actual);
        return 0;
    }
    return 1;
}

static void set_controller_sample(const ControllerSample *sample)
{
    g_C1ButtonState = sample->c1_held;
    g_C2ButtonState = sample->c2_held;
    g_C1ButtonStateReleased = sample->c1_released;
    g_C2ButtonStateReleased = sample->c2_released;
    g_C1ButtonStatePressedOnce = sample->c1_pressed_once;
    g_C2ButtonStatePressedOnce = sample->c2_pressed_once;
}

static void run_drain(const ControllerSample *samples, size_t sample_count)
{
    s_samples = samples;
    s_sample_count = sample_count;
    s_sample_index = 0u;

    if (setjmp(s_after_drain) == 0) {
        wm_800712D0();
    }
}

static int test_zero_sources_clear_accumulators(void)
{
    static const ControllerSample zero_sample = {0u, 0u, 0u, 0u, 0u, 0u};

    memset(g_PsxRam, 0, sizeof(g_PsxRam));
    write_u16(D_8009CD4C, UINT16_C(0xA55A));
    write_u16(D_8009CD50, UINT16_C(0x5AA5));
    write_u16(D_8009BD10, UINT16_C(0x1357));
    write_u16(D_8009BD14, UINT16_C(0x2468));
    write_u16(D_8009BD18, UINT16_C(0xAAAA));
    write_u16(D_8009BD1C, UINT16_C(0x5555));

    run_drain(&zero_sample, 1u);

    if (!expect_u16("controller.accumulator.cd4c.cleared_before_drain",
                    D_8009CD4C, 0u)) {
        return 0;
    }
    if (!expect_u16("controller.zero_source.c2_held.contributes_nothing",
                    D_8009CD50, 0u) ||
        !expect_u16("controller.zero_source.c1_released.contributes_nothing",
                    D_8009BD10, 0u) ||
        !expect_u16("controller.zero_source.c2_released.contributes_nothing",
                    D_8009BD14, 0u) ||
        !expect_u16("controller.zero_source.c1_pressed_once.contributes_nothing",
                    D_8009BD18, 0u) ||
        !expect_u16("controller.zero_source.c2_pressed_once.contributes_nothing",
                    D_8009BD1C, 0u)) {
        return 0;
    }
    return 1;
}

static int test_asymmetric_sources_or_into_retail_destinations(void)
{
    static const ControllerSample samples[2] = {
        {UINT16_C(0x0001), UINT16_C(0x0002), UINT16_C(0x0004),
         UINT16_C(0x0008), UINT16_C(0x0010), UINT16_C(0x0020)},
        {UINT16_C(0x0100), UINT16_C(0x0200), UINT16_C(0x0400),
         UINT16_C(0x0800), UINT16_C(0x1000), UINT16_C(0x2000)},
    };

    memset(g_PsxRam, 0, sizeof(g_PsxRam));

    /* Poison the six fabricated +0x10000 guest sources. */
    write_u16(UINT32_C(0x80069570), UINT16_C(0x4000));
    write_u16(UINT32_C(0x80069574), UINT16_C(0x4001));
    write_u16(UINT32_C(0x8006948C), UINT16_C(0x4002));
    write_u16(UINT32_C(0x80069490), UINT16_C(0x4003));
    write_u16(UINT32_C(0x800694A4), UINT16_C(0x4004));
    write_u16(UINT32_C(0x800694A8), UINT16_C(0x4005));

    run_drain(samples, 2u);

    if (s_sample_index != 2u) {
        fprintf(stderr,
                "ASSERTION controller.poll.consumes_all_samples expected=2 actual=%zu\n",
                s_sample_index);
        return 0;
    }
    if (!expect_u16("controller.source.c1_held.native_not_guest_plus_10000",
                    D_8009CD4C, UINT16_C(0x0101)) ||
        !expect_u16("controller.source.c2_held.accumulates_cd50", D_8009CD50,
                    UINT16_C(0x0202)) ||
        !expect_u16("controller.source.c1_released.accumulates_bd10", D_8009BD10,
                    UINT16_C(0x0404)) ||
        !expect_u16("controller.source.c2_released.accumulates_bd14", D_8009BD14,
                    UINT16_C(0x0808)) ||
        !expect_u16("controller.source.c1_pressed_once.accumulates_bd18",
                    D_8009BD18, UINT16_C(0x1010)) ||
        !expect_u16("controller.source.c2_pressed_once.accumulates_bd1c",
                    D_8009BD1C, UINT16_C(0x2020))) {
        return 0;
    }
    return 1;
}

int ControllerPopState(void)
{
    if (s_sample_index >= s_sample_count) {
        return 0;
    }
    set_controller_sample(&s_samples[s_sample_index]);
    s_sample_index++;
    return 1;
}

u32 wm_800967E4(void)
{
    longjmp(s_after_drain, 1);
    return 0u;
}

/* Link-only stubs for paths beyond the bounded drain seam. */
int CdSync(int mode, u_char *result)
{
    (void)mode;
    (void)result;
    return 0;
}

void DrawSync(void (*func)(unsigned long)) { (void)func; }
void func_800250E0(int context) { (void)context; }
void func_8001D468(void) {}
void GameCheckAndHandleSoftReset(void) {}
void MenuMain(void) {}
void MoveImage(void *rect, long x, long y)
{
    (void)rect;
    (void)x;
    (void)y;
}
void PutDispEnv(void *env) { (void)env; }
void PutDrawEnv(void *env) { (void)env; }
void ResetGraph(int mode) { (void)mode; }
void SetGeomOffset(long ofx, long ofy)
{
    (void)ofx;
    (void)ofy;
}
int Vsync(int mode) { (void)mode; return 0; }
s32 wm_80093F18(u32 vec_addr)
{
    (void)vec_addr;
    return 0;
}
void wm_80097800(void) {}
void wm_80096694(void) {}
void wm_80025044_guest_safe(void) {}
int wm_80074F2C(void) { return 0; }
int wm_80075104(void) { return 0; }
void wm_80075D4C(void) {}
s32 wm_80075E7C(u32 vec_addr, s32 threshold)
{
    (void)vec_addr;
    (void)threshold;
    return 0;
}
void wm_8007634C(void) {}
int ControllerGetType(int port) { (void)port; return 1; }
void wm_80076594(void) {}
void wm_800758C0(void) {}
void wm_800762FC(void) {}
void wm_80075B58(void) {}
void wm_ot_clear_r_guest(u32 ot_guest, u32 count)
{
    (void)ot_guest;
    (void)count;
}
int wm_ot_draw_otag_guest(u32 entry_guest)
{
    (void)entry_guest;
    return 1;
}

int main(void)
{
    if (!test_zero_sources_clear_accumulators() ||
        !test_asymmetric_sources_or_into_retail_destinations()) {
        return 1;
    }
    puts("W34C1 RUNG3A controller source certificate PASS");
    return 0;
}
