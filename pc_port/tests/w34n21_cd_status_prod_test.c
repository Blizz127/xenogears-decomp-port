/* Production-linked certificate for the retail world-map CD status lane. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_frame_driver_712d0.h"
#include "world_map_helper_96130.h"

#define D_8009BE44 UINT32_C(0x8009BE44)
#define D_8009BCB8 UINT32_C(0x8009BCB8)
#define D_8009BD2C UINT32_C(0x8009BD2C)
#define D_8009C588 UINT32_C(0x8009C588)
#define D_8009C624 UINT32_C(0x8009C624)
#define D_8009CD44 UINT32_C(0x8009CD44)
#define D_8009D788 UINT32_C(0x8009D788)

#define TABLE_WORD(address, index) ((address) + (u32)(index) * 4u)

uint8_t g_PsxRam[PSX_RAM_SIZE];
uint8_t g_PsxScratchpad[4096];

static u32 s_status_values[8];
static size_t s_status_value_count;
static size_t s_status_calls;
static int s_zero_state_on_wm_call;
static int s_vsync_calls;
static int s_vsync_modes[8];
static int s_cd_sync_calls;
static int s_cd_sync_mode;
static u_char *s_cd_sync_result;
static int s_cd_loader_calls;
static u32 s_cd_loader_arg;
static int s_pc_loader_calls;
static u32 s_pc_loader_arg;
static int s_pc_loader_moves_tail;
static u32 s_pc_loader_new_tail;
static char s_lane_events[16];
static size_t s_lane_event_count;
static u32 s_write_addresses[8];
static u32 s_write_values[8];
static size_t s_write_count;
static int s_state4_hook_armed;
static u32 s_state4_hook_value;

static void wr32(u32 address, u32 value);

static void lane_event(char value)
{
    if (s_lane_event_count < sizeof(s_lane_events))
        s_lane_events[s_lane_event_count] = value;
    s_lane_event_count++;
}

void w34n21_test_q96_sw(u32 address, u32 value)
{
    if (s_write_count < sizeof(s_write_addresses) / sizeof(s_write_addresses[0])) {
        s_write_addresses[s_write_count] = address;
        s_write_values[s_write_count] = value;
    }
    s_write_count++;
}

void w34n21_test_state4_after_countdown(void)
{
    if (s_state4_hook_armed != 0) {
        wr32(D_8009CD44, s_state4_hook_value);
        s_state4_hook_armed = 0;
    }
}

static void wr32(u32 address, u32 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static u32 rd32(u32 address)
{
    u32 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static int expect_true(const char *assertion, int condition)
{
    if (condition == 0) {
        fprintf(stderr, "ASSERTION %s\n", assertion);
        return 0;
    }
    return 1;
}

static void set_status_pair(u32 first, u32 second)
{
    s_status_values[0] = first;
    s_status_values[1] = second;
    s_status_value_count = 2u;
}

static void reset_fixture(void)
{
    memset(g_PsxRam, 0, sizeof(g_PsxRam));
    memset(g_PsxScratchpad, 0, sizeof(g_PsxScratchpad));
    memset(s_status_values, 0, sizeof(s_status_values));
    s_status_value_count = 0u;
    s_status_calls = 0u;
    s_zero_state_on_wm_call = 0;
    s_vsync_calls = 0;
    memset(s_vsync_modes, 0xA5, sizeof(s_vsync_modes));
    s_cd_sync_calls = 0;
    s_cd_sync_mode = -1;
    s_cd_sync_result = NULL;
    s_cd_loader_calls = 0;
    s_cd_loader_arg = 0u;
    s_pc_loader_calls = 0;
    s_pc_loader_arg = 0u;
    s_pc_loader_moves_tail = 0;
    s_pc_loader_new_tail = 0u;
    memset(s_lane_events, 0, sizeof(s_lane_events));
    s_lane_event_count = 0u;
    memset(s_write_addresses, 0, sizeof(s_write_addresses));
    memset(s_write_values, 0, sizeof(s_write_values));
    s_write_count = 0u;
    s_state4_hook_armed = 0;
    s_state4_hook_value = 0u;
}

u32 func_8002C3D8(void)
{
    size_t call = s_status_calls;
    u32 value = 0u;

    if ((call & 1u) == 0u)
        lane_event('W');
    /* wm_800967E4 calls this observer twice.  Mutating on the first observer
     * call of the selected wm_800967E4 invocation bounds skip-Vsync mutants. */
    if (s_zero_state_on_wm_call > 0 &&
            call == (size_t)(s_zero_state_on_wm_call - 1) * 2u)
        wr32(D_8009CD44, 0u);
    if (call < s_status_value_count)
        value = s_status_values[call];
    s_status_calls++;
    return value;
}

int Vsync(int mode)
{
    if ((size_t)s_vsync_calls <
            sizeof(s_vsync_modes) / sizeof(s_vsync_modes[0]))
        s_vsync_modes[s_vsync_calls] = mode;
    s_vsync_calls++;
    lane_event('V');
    return 0;
}

int CdSync(int mode, u_char *result)
{
    s_cd_sync_calls++;
    s_cd_sync_mode = mode;
    s_cd_sync_result = result;
    if (result != NULL) {
        result[0] = UINT8_C(0x5A);
        result[1] = UINT8_C(0xC3);
    }
    lane_event('C');
    return 0;
}

void wm_8009699C(u32 list)
{
    s_cd_loader_calls++;
    s_cd_loader_arg = list;
}

void wm_800966CC(u32 file_table)
{
    s_pc_loader_calls++;
    s_pc_loader_arg = file_table;
    if (s_pc_loader_moves_tail != 0)
        wr32(D_8009BCB8, s_pc_loader_new_tail);
}

static int test_dispatch_exact_states(void)
{
    static const struct DispatchCase {
        u32 state;
        u32 result;
        const char *assertion;
    } cases[] = {
        { 0u, 0u, "dispatch.state0.exact_status" },
        { 1u, 1u, "dispatch.state1.exact_status" },
        { 2u, 1u, "dispatch.state2.exact_status" },
        { 3u, 1u, "dispatch.state3.exact_status" },
        { 6u, 3u, "dispatch.state6.exact_status" },
        { UINT32_MAX, 3u, "dispatch.high_busy_range.exact_status" }
    };
    size_t i;
    int ok = 1;
    u32 state5_result;

    for (i = 0u; i < sizeof(cases) / sizeof(cases[0]); i++) {
        u32 got;
        reset_fixture();
        wr32(D_8009CD44, cases[i].state);
        got = wm_800968E0();
        ok &= expect_true(cases[i].assertion,
                          got == cases[i].result &&
                          rd32(D_8009CD44) == cases[i].state);
    }

    reset_fixture();
    wr32(D_8009CD44, 4u);
    wr32(D_8009BD2C, 2u);
    ok &= expect_true("dispatch.state4.countdown",
                      wm_800968E0() == 1u &&
                      rd32(D_8009BD2C) == 1u &&
                      rd32(D_8009CD44) == 4u);

    reset_fixture();
    wr32(D_8009CD44, 4u);
    wr32(D_8009BD2C, 1u);
    ok &= expect_true("dispatch.state4.zero_advances_to_five",
                      wm_800968E0() == 1u &&
                      rd32(D_8009BD2C) == 0u &&
                      rd32(D_8009CD44) == 5u);

    reset_fixture();
    wr32(D_8009CD44, 4u);
    wr32(D_8009BD2C, 1u);
    s_state4_hook_armed = 1;
    s_state4_hook_value = 8u;
    ok &= expect_true("dispatch.state4.reloads_live_state",
                      wm_800968E0() == 1u &&
                      rd32(D_8009BD2C) == 0u &&
                      rd32(D_8009CD44) == 9u);

    reset_fixture();
    wr32(D_8009CD44, 4u);
    wr32(D_8009BD2C, 0u);
    ok &= expect_true("dispatch.state4.bd2c_zero_underflows",
                      wm_800968E0() == 1u &&
                      rd32(D_8009BD2C) == UINT32_MAX &&
                      rd32(D_8009CD44) == 4u);

    reset_fixture();
    wr32(D_8009CD44, 5u);
    wr32(D_8009BCB8, 15u);
    wr32(TABLE_WORD(D_8009D788, 0u), UINT32_C(0x11112222));
    wr32(TABLE_WORD(D_8009D788, 15u), UINT32_C(0xAABBCCDD));
    state5_result = wm_800968E0();
    ok &= expect_true("dispatch.state5.exact_tail_wrap",
                      state5_result == 2u &&
                      rd32(D_8009CD44) == 0u &&
                      rd32(D_8009BCB8) == 0u &&
                      rd32(TABLE_WORD(D_8009D788, 15u)) == 0u &&
                      rd32(TABLE_WORD(D_8009D788, 0u)) ==
                          UINT32_C(0x11112222));
    ok &= expect_true("dispatch.state5.exact_store_order",
                      s_write_count == 3u &&
                      s_write_addresses[0] == D_8009CD44 &&
                      s_write_values[0] == 0u &&
                      s_write_addresses[1] ==
                          TABLE_WORD(D_8009D788, 15u) &&
                      s_write_values[1] == 0u &&
                      s_write_addresses[2] == D_8009BCB8 &&
                      s_write_values[2] == 0u);
    return ok;
}

static int run_d788_ready_case(u32 r1, u32 r2, const char *assertion)
{
    const u32 payload = UINT32_C(0x81234560);
    int ok = 1;

    reset_fixture();
    set_status_pair(r1, r2);
    wr32(D_8009CD44, 0u);
    wr32(D_8009BCB8, 6u);
    wr32(D_8009BE44, 2u);
    wr32(TABLE_WORD(D_8009D788, 2u), UINT32_C(0x8BADF00D));
    wr32(TABLE_WORD(D_8009D788, 6u), payload);
    ok &= expect_true(assertion,
                      wm_800967E4() == 0u &&
                      s_status_calls == 2u &&
                      s_cd_loader_calls == 1 &&
                      s_cd_loader_arg == payload &&
                      s_pc_loader_calls == 0 &&
                      rd32(D_8009BCB8) == 6u &&
                      rd32(TABLE_WORD(D_8009D788, 6u)) == payload);
    return ok;
}

static int test_routes_and_callback_reload(void)
{
    const u32 original_payload = UINT32_C(0x80123450);
    const u32 reloaded_payload = UINT32_C(0x806789A0);
    int ok = 1;

    ok &= run_d788_ready_case(0u, 0u, "route.d788.r1_ready_uses_bcb8");
    ok &= run_d788_ready_case(1u, UINT32_MAX,
                              "route.d788.r2_minus_one_ready_uses_bcb8");

    reset_fixture();
    set_status_pair(1u, 0u);
    wr32(D_8009CD44, 6u);
    wr32(D_8009BCB8, 3u);
    wr32(TABLE_WORD(D_8009C624, 3u), original_payload);
    wr32(TABLE_WORD(D_8009C624, 7u), reloaded_payload);
    wr32(TABLE_WORD(D_8009D788, 3u), UINT32_C(0x8BADF00D));
    s_pc_loader_moves_tail = 1;
    s_pc_loader_new_tail = 7u;
    ok &= expect_true("route.c624.reloads_callback_tail",
                      wm_800967E4() == 0u &&
                      s_status_calls == 2u &&
                      s_pc_loader_calls == 1 &&
                      s_pc_loader_arg == original_payload &&
                      s_cd_loader_calls == 0 &&
                      rd32(TABLE_WORD(D_8009C624, 3u)) == original_payload &&
                      rd32(TABLE_WORD(D_8009C624, 7u)) == 0u &&
                      rd32(D_8009BCB8) == 8u &&
                      rd32(D_8009CD44) == 6u);
    return ok;
}

static int run_propagation_case(u32 state, u32 expected,
                                const char *assertion)
{
    int ok = 1;

    reset_fixture();
    set_status_pair(0u, 0u);
    wr32(D_8009CD44, state);
    wr32(D_8009BD2C, 2u);
    wr32(D_8009BCB8, 4u);
    ok &= expect_true(assertion,
                      wm_800967E4() == expected &&
                      s_status_calls == 2u &&
                      s_cd_loader_calls == 0 &&
                      s_pc_loader_calls == 0);
    return ok;
}

static int test_dispatch_status_propagation(void)
{
    int ok = 1;

    ok &= run_propagation_case(1u, 1u, "status.propagates.one");
    ok &= run_propagation_case(5u, 2u, "status.propagates.two");
    ok &= run_propagation_case(6u, 3u, "status.propagates.three");
    return ok;
}

static int run_driver_nonretry_case(u32 state, const char *assertion)
{
    int ok = 1;

    reset_fixture();
    set_status_pair(0u, 0u);
    wr32(D_8009CD44, state);
    wr32(D_8009BD2C, 2u);
    wr32(D_8009BCB8, 4u);
    *(u8 *)PSX_ADDR(D_8009C588) = UINT8_C(0x11);
    *((u8 *)PSX_ADDR(D_8009C588) + 1) = UINT8_C(0x22);
    wm_712d0_run_cd_sync_lane();
    ok &= expect_true(assertion,
                      s_status_calls == 2u && s_vsync_calls == 0);
    ok &= expect_true("driver.nonretry.event_order",
                      s_lane_event_count == 2u &&
                      memcmp(s_lane_events, "WC", 2u) == 0);
    ok &= expect_true("driver.cdsync.exact_mode_and_result_pointer",
                      s_cd_sync_calls == 1 && s_cd_sync_mode == 1 &&
                      s_cd_sync_result ==
                          (u_char *)PSX_ADDR(D_8009C588));
    ok &= expect_true("driver.cdsync.status_bytes_propagate",
                      *(u8 *)PSX_ADDR(D_8009C588) == UINT8_C(0x5A) &&
                      *((u8 *)PSX_ADDR(D_8009C588) + 1) ==
                          UINT8_C(0xC3));
    return ok;
}

static int test_driver_retry_contract(void)
{
    int ok = 1;

    ok &= run_driver_nonretry_case(0u, "driver.status0.no_retry_vsync");
    ok &= run_driver_nonretry_case(1u, "driver.status1.no_retry_vsync");
    ok &= run_driver_nonretry_case(5u, "driver.status2.no_retry_vsync");

    reset_fixture();
    set_status_pair(0u, 0u);
    wr32(D_8009CD44, 6u);
    s_zero_state_on_wm_call = 3;
    *(u8 *)PSX_ADDR(D_8009C588) = UINT8_C(0x11);
    *((u8 *)PSX_ADDR(D_8009C588) + 1) = UINT8_C(0x22);
    wm_712d0_run_cd_sync_lane();
    ok &= expect_true("driver.status3.retry_vsync_twice",
                      s_status_calls == 6u && s_vsync_calls == 2 &&
                      s_vsync_modes[0] == 0 && s_vsync_modes[1] == 0 &&
                      rd32(D_8009CD44) == 0u);
    ok &= expect_true("driver.retry.event_order",
                      s_lane_event_count == 6u &&
                      memcmp(s_lane_events, "WVWVWC", 6u) == 0);
    ok &= expect_true("driver.status3.then_cdsync_once",
                      s_cd_sync_calls == 1);
    ok &= expect_true("driver.cdsync.exact_mode_and_result_pointer",
                      s_cd_sync_mode == 1 &&
                      s_cd_sync_result ==
                          (u_char *)PSX_ADDR(D_8009C588));
    ok &= expect_true("driver.cdsync.status_bytes_propagate",
                      *(u8 *)PSX_ADDR(D_8009C588) == UINT8_C(0x5A) &&
                      *((u8 *)PSX_ADDR(D_8009C588) + 1) ==
                          UINT8_C(0xC3));
    return ok;
}

int main(void)
{
    int ok = 1;

    ok &= test_dispatch_exact_states();
    ok &= test_routes_and_callback_reload();
    ok &= test_dispatch_status_propagation();
    ok &= test_driver_retry_contract();
    if (ok == 0)
        return EXIT_FAILURE;
    puts("W34N21 CD STATUS CERTIFICATE PASS");
    return EXIT_SUCCESS;
}
