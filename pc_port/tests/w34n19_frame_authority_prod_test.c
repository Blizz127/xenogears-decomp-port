/* Production-linked certificate for retail frame-driver authority lanes. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_common_tail.h"
#include "world_map_frame_driver_712d0.h"

uint8_t g_PsxRam[PSX_RAM_SIZE];
uint8_t g_PsxScratchpad[4096];

u8 D_80059179;
u8 D_80059460;
u8 D_80059171;
u8 g_MenuDebugEnabled;
u8 D_8005954C;

static int s_area_calls;
static s32 s_area_result;
static int s_reconcile_calls;
static char s_menu_order[16];
static size_t s_menu_order_len;

static void write_u32(u32 address, u32 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static void write_u16(u32 address, u16 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static u32 read_u32(u32 address)
{
    u32 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static u16 read_u16(u32 address)
{
    u16 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static u8 read_u8(u32 address)
{
    return *(const u8 *)PSX_ADDR(address);
}

static void write_u8(u32 address, u8 value)
{
    *(u8 *)PSX_ADDR(address) = value;
}

static int assertion_true(const char *name, int condition)
{
    if (condition == 0) {
        fprintf(stderr, "ASSERTION %s\n", name);
        return 0;
    }
    return 1;
}

static void append_menu_order(char value)
{
    if (s_menu_order_len + 1u < sizeof(s_menu_order)) {
        s_menu_order[s_menu_order_len++] = value;
        s_menu_order[s_menu_order_len] = '\0';
    }
}

s32 wm_80093F18(u32 address)
{
    s_area_calls++;
    return address == UINT32_C(0x8009D55C) ? s_area_result : -1;
}

void wm_80075D4C(void)
{
    s_reconcile_calls++;
}

void wm_800758C0(void) { append_menu_order('A'); }
void wm_800762FC(void) { append_menu_order('S'); }
void MenuMain(void) { append_menu_order('M'); }
void wm_80075B58(void) { append_menu_order('B'); }

static void reset_party_guard(void)
{
    write_u32(UINT32_C(0x8009BD34), 1u);
    write_u32(UINT32_C(0x8009C178), 0u);
    write_u32(UINT32_C(0x8009D804), 0u);
    write_u16(UINT32_C(0x8009BD24), UINT16_C(0xFFFF));
    write_u16(UINT32_C(0x8009CE68), UINT16_C(0xFFFF));
    write_u32(UINT32_C(0x8009D554), 1u);
    write_u32(UINT32_C(0x8009D80C), 0u);
}

static int test_common_tail_authority(void)
{
    int ok = 1;

    memset(g_PsxRam, 0, sizeof(g_PsxRam));
    write_u32(UINT32_C(0x8009BCC0), UINT32_C(0x800A0000));
    write_u32(UINT32_C(0x8009C610), 0u);
    write_u8(UINT32_C(0x80059179), UINT8_C(0xA5));
    D_80059179 = UINT8_C(0x5A);
    wm_common_tail_p0_reset();
    (void)wm_8007290C_common_tail_p0();
    ok &= assertion_true("common_tail.native_authority",
                         D_80059179 == 1u);
    ok &= assertion_true("common_tail.guest_twin_untouched",
                         read_u8(UINT32_C(0x80059179)) == UINT8_C(0xA5));

    write_u32(UINT32_C(0x8009C610), 1u);
    D_80059179 = UINT8_C(0x5A);
    (void)wm_8007290C_common_tail_p0();
    ok &= assertion_true("common_tail.alternate_clears_native",
                         D_80059179 == 0u);
    return ok;
}

static int test_party_refresh(void)
{
    int ok = 1;

    memset(g_PsxRam, 0, sizeof(g_PsxRam));
    reset_party_guard();
    D_80059179 = 1u;
    write_u8(UINT32_C(0x80069179), 0u);
    s_area_calls = 0;
    s_reconcile_calls = 0;
    wm_712d0_run_party_refresh_lane();
    ok &= assertion_true("party.guard.native_authority",
                         s_area_calls == 0);
    ok &= assertion_true("party.reconvergence.always_clears_bd34",
                         read_u32(UINT32_C(0x8009BD34)) == 0u);

    memset(g_PsxRam, 0, sizeof(g_PsxRam));
    reset_party_guard();
    D_80059179 = 0u;
    s_area_result = 0;
    s_area_calls = 0;
    s_reconcile_calls = 0;
    memset(PSX_ADDR(UINT32_C(0x8007D940)), 0xFF, 0x1000u);
    write_u8(UINT32_C(0x8006F8E5), 0u);
    write_u8(UINT32_C(0x8006F8E6), 7u);
    write_u8(UINT32_C(0x8006F8E7), 9u);
    write_u8(UINT32_C(0x8006F368), 2u);
    write_u8(UINT32_C(0x8006F369), 3u);
    write_u8(UINT32_C(0x8006F36A), 4u);
    write_u8(UINT32_C(0x8007D940) + 2u * 0xA4u, 0u);
    write_u8(UINT32_C(0x8007D940) + 4u * 0xA4u, 1u);
    wm_712d0_run_party_refresh_lane();
    ok &= assertion_true("party.snapshot.exact_three_channels",
                         read_u16(UINT32_C(0x8006EE70)) == 0u &&
                         read_u16(UINT32_C(0x8006EE72)) == 7u &&
                         read_u16(UINT32_C(0x8006EE74)) == 9u);
    ok &= assertion_true("party.primary_selectors_and_a4_stride",
                         read_u8(UINT32_C(0x8006F8E5)) == 1u &&
                         read_u8(UINT32_C(0x8006F8E6)) == 7u &&
                         read_u8(UINT32_C(0x8006F8E7)) == 1u);
    ok &= assertion_true("party.reconcile.called_once",
                         s_reconcile_calls == 1);

    reset_party_guard();
    write_u8(UINT32_C(0x8006F8E5), 1u);
    write_u8(UINT32_C(0x8006F8E6), 2u);
    write_u8(UINT32_C(0x8006F8E7), 3u);
    s_reconcile_calls = 0;
    wm_712d0_run_party_refresh_lane();
    ok &= assertion_true("party.nonzero_first_clears_presence",
                         read_u8(UINT32_C(0x8006F8E5)) == 0u &&
                         read_u8(UINT32_C(0x8006F8E6)) == 0u &&
                         read_u8(UINT32_C(0x8006F8E7)) == 0u);

    reset_party_guard();
    s_area_result = 4;
    s_reconcile_calls = 0;
    wm_712d0_run_party_refresh_lane();
    ok &= assertion_true("party.area4.skips_reconcile",
                         s_reconcile_calls == 0);
    return ok;
}

static int test_menu_mode(void)
{
    int ok = 1;

    memset(g_PsxRam, 0, sizeof(g_PsxRam));
    write_u32(UINT32_C(0x8009C178), 0u);
    write_u32(UINT32_C(0x8009D804), 1u);
    write_u32(UINT32_C(0x8009D554), 1u);
    write_u32(UINT32_C(0x8009BE10), 2u);
    D_80059460 = UINT8_C(0xA1);
    g_MenuDebugEnabled = UINT8_C(0xA2);
    D_80059171 = UINT8_C(0xA3);
    write_u8(UINT32_C(0x80069460), UINT8_C(0xB1));
    write_u8(UINT32_C(0x80069178), UINT8_C(0xB2));
    write_u8(UINT32_C(0x80069171), UINT8_C(0xB3));
    memset(s_menu_order, 0, sizeof(s_menu_order));
    s_menu_order_len = 0u;
    wm_712d0_run_menu_mode_lane();
    ok &= assertion_true("menu.native_authorities",
                         D_80059460 == 0u && g_MenuDebugEnabled == 0u &&
                         D_80059171 == 1u);
    ok &= assertion_true("menu.wrong_page_canaries",
                         read_u8(UINT32_C(0x80069460)) == UINT8_C(0xB1) &&
                         read_u8(UINT32_C(0x80069178)) == UINT8_C(0xB2) &&
                         read_u8(UINT32_C(0x80069171)) == UINT8_C(0xB3));
    ok &= assertion_true("menu.retail_call_order",
                         strcmp(s_menu_order, "ASMSB") == 0);

    memset(s_menu_order, 0, sizeof(s_menu_order));
    s_menu_order_len = 0u;
    write_u32(UINT32_C(0x8009D554), 1u);
    write_u32(UINT32_C(0x8009D7CC), 9u);
    write_u32(UINT32_C(0x8009D7D8), 0u);
    write_u32(UINT32_C(0x8009BE10), 4u);
    write_u16(UINT32_C(0x8006EE68), UINT16_C(0x0123));
    write_u16(UINT32_C(0x8007EE68), UINT16_C(0x4567));
    wm_712d0_run_menu_mode_lane();
    ok &= assertion_true("transition.guest_6ee68_authority",
                         read_u16(UINT32_C(0x8006EE68)) ==
                             UINT16_C(0x2123));
    ok &= assertion_true("transition.wrong_page_untouched",
                         read_u16(UINT32_C(0x8007EE68)) ==
                             UINT16_C(0x4567));
    ok &= assertion_true("transition.exact_exit_publication",
                         read_u32(UINT32_C(0x8009D554)) == 0u &&
                         read_u32(UINT32_C(0x8009D7CC)) == 0u &&
                         read_u32(UINT32_C(0x8009D7D8)) ==
                             UINT32_C(0x8009B6E4));
    return ok;
}

int main(void)
{
    int ok = 1;

    ok &= test_common_tail_authority();
    ok &= test_party_refresh();
    ok &= test_menu_mode();
    if (ok == 0)
        return EXIT_FAILURE;
    puts("W34N19 FRAME AUTHORITY CERTIFICATE PASS");
    return EXIT_SUCCESS;
}
