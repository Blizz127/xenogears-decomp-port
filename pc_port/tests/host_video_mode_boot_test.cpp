/*
 * Startup regression fixture for the native video-mode bootstrap.
 *
 * The runner extracts the real port_main prefix through PsyX_Initialise and
 * the real PsyX_Sys_SetVMode/SetVideoMode bodies into production.inc.  The
 * leaves below are deliberately controlled so this test observes the mode at
 * the PsyX core-start boundary without opening SDL or starting a thread.
 */
#include <cstdio>
#include <cstdlib>
#include <cstring>

static int event_log[16];
static int event_count;
static int test_input_rc;
static int world_input_rc;
static int core_start_count;
static int core_start_mode;
static int core_start_order;
static int game_init_order;
static int next_order;

enum {
    EVENT_MEMORY_INIT = 1,
    EVENT_MEMORY_LOAD,
    EVENT_TEST_INPUT,
    EVENT_WORLD_INPUT,
    EVENT_GAME_STATES,
    EVENT_CORE_START
};

int g_vmode = -1;

static void record(int event)
{
    if (event_count >= static_cast<int>(sizeof(event_log) / sizeof(event_log[0]))) {
        std::fprintf(stderr, "ASSERTION event-log-overflow\n");
        std::exit(EXIT_FAILURE);
    }
    event_log[event_count++] = event;
}

extern "C" void PsxMemory_Init(void)
{
    record(EVENT_MEMORY_INIT);
    ++next_order;
}

extern "C" int PsxMemory_LoadStaticData(void)
{
    record(EVENT_MEMORY_LOAD);
    ++next_order;
    return 0;
}

extern "C" int PcPort_TestInputInit(void)
{
    record(EVENT_TEST_INPUT);
    ++next_order;
    return test_input_rc;
}

extern "C" int PcPort_WorldTestInputInit(void)
{
    record(EVENT_WORLD_INPUT);
    ++next_order;
    return world_input_rc;
}

extern "C" void PcPort_InitGameStates(void)
{
    record(EVENT_GAME_STATES);
    game_init_order = ++next_order;
}

extern "C" void PsyX_Initialise(char*, int, int, int)
{
    record(EVENT_CORE_START);
    core_start_order = ++next_order;
    core_start_mode = g_vmode;
    ++core_start_count;
}

static char window_title[] = "host-video-mode-boot-test";
#define WINDOW_TITLE window_title
#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240

#include "production.inc"

#define main xeno_port_main_prefix
#include "main_prefix.inc"
#undef main

static void require(bool condition, const char* name)
{
    if (!condition) {
        std::fprintf(stderr, "ASSERTION %s\n", name);
        std::exit(EXIT_FAILURE);
    }
}

static void reset_fixture(void)
{
    std::memset(event_log, 0, sizeof(event_log));
    event_count = 0;
    test_input_rc = 0;
    world_input_rc = 0;
    core_start_count = 0;
    core_start_mode = -2;
    core_start_order = 0;
    game_init_order = 0;
    next_order = 0;
    g_vmode = -1;
}

static void test_actual_set_video_mode_body(void)
{
    reset_fixture();
    require(SetVideoMode(0) == -1, "set-video-mode-returns-old");
    require(g_vmode == 0, "set-video-mode-ntsc");
    require(SetVideoMode(1) == 0, "set-video-mode-returns-ntsc");
    require(g_vmode == 1, "set-video-mode-pal");
}

static void test_startup_observes_ntsc_before_core(void)
{
    reset_fixture();
    require(xeno_port_main_prefix(0, NULL) == 0, "startup-success");
    require(core_start_count == 1, "core-start-once");
    require(core_start_mode == 0, "core-start-observes-ntsc");
    require(event_count == 6, "startup-event-count");
    require(event_log[0] == EVENT_MEMORY_INIT, "startup-memory-init-first");
    require(event_log[1] == EVENT_MEMORY_LOAD, "startup-static-data-second");
    require(event_log[2] == EVENT_TEST_INPUT, "startup-test-input-third");
    require(event_log[3] == EVENT_WORLD_INPUT, "startup-world-input-fourth");
    require(event_log[4] == EVENT_GAME_STATES, "startup-game-states-fifth");
    require(event_log[5] == EVENT_CORE_START, "startup-core-sixth");
    require(game_init_order < core_start_order, "game-state-before-core");
}

static void test_input_failure_stops_before_core(void)
{
    reset_fixture();
    test_input_rc = 1;
    require(xeno_port_main_prefix(0, NULL) == EXIT_FAILURE, "test-input-failure");
    require(core_start_count == 0, "test-input-failure-no-core");

    reset_fixture();
    world_input_rc = 1;
    require(xeno_port_main_prefix(0, NULL) == EXIT_FAILURE, "world-input-failure");
    require(core_start_count == 0, "world-input-failure-no-core");
}

int main(void)
{
    test_actual_set_video_mode_body();
    test_startup_observes_ntsc_before_core();
    test_input_failure_stops_before_core();
    std::puts("HOST VIDEO MODE BOOT PASS: real main prefix and SetVideoMode before core start");
    return 0;
}
