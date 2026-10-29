/* Dev menu controller / input-capture test (DEV_MENU_SPEC.md §3, §5, §8, §10)
 * with SDL virtual pads.  Links PsyCross's patched src/pad/PsyX_pad.cpp and
 * the port's pc_port/src/port_dev_menu.c + port_dev_menu_warps.c; the pad is
 * read exactly as in the game (port 1's active pad), so what passes here is
 * what the Steam virtual pad would do.  Run: run_dev_menu_test.sh. */
#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <limits.h>

#include "PsyX/PsyX_public.h"
#include "port_dev_menu.h"

int PsyX_Pad_InitSystem();
void PsyX_Pad_Event_ControllerAdded(Sint32 deviceId);
void PsyX_Pad_Event_ControllerRemoved(Sint32 deviceId);

extern "C" {
void PsyX_Pad_InitPad(int slot, unsigned char* padData);
void PsyX_Pad_InternalPadUpdates();
void PsyX_Pad_LatchKeyboardInput(int scancode);
SDL_JoystickID PsyX_Pad_HostActiveInstance();

int g_padCommEnable = 1;
int g_activeKeyboardControllers = 0x1;
PsyXControllerMapping g_cfg_controllerMapping;
PsyXKeyboardMapping g_cfg_keyboardMapping;
static int s_verbose = 0;
static void vlog(const char* fmt, va_list ap) { if (s_verbose) vprintf(fmt, ap); }
void PsyX_Log(const char* fmt, ...) { va_list ap; va_start(ap, fmt); vlog(fmt, ap); va_end(ap); }
void PsyX_Log_Info(const char* fmt, ...) { va_list ap; va_start(ap, fmt); vlog(fmt, ap); va_end(ap); }
void PsyX_Log_Warning(const char* fmt, ...) { va_list ap; va_start(ap, fmt); vlog(fmt, ap); va_end(ap); }
void PsyX_Log_Error(const char* fmt, ...) { va_list ap; va_start(ap, fmt); vlog(fmt, ap); va_end(ap); }
void PsyX_Log_Success(const char* fmt, ...) { va_list ap; va_start(ap, fmt); vlog(fmt, ap); va_end(ap); }

/* Config: only cheats.dev_menu is read by the menu. */
static int s_cfgDevMenu = 0;
int xg_plat_config_int(const char* key, int fallback)
{
    return strcmp(key, "cheats.dev_menu") == 0 ? s_cfgDevMenu : fallback;
}

/* Every game mutation goes through the cheat queue: record it. */
static int s_queued = 0;
static char s_lastQueued[64];
void PcPort_CheatQueue(const char* line)
{
    s_queued++;
    snprintf(s_lastQueued, sizeof s_lastQueued, "%s", line);
}
}

/* Host hooks: count calls (identity check) and model state. */
static int s_hostCalls = 0, s_ff = 0, s_speed = 1, s_rec = 0, s_shots = 0, s_hd2d = 1, s_god = 0;
static int h_ff_on(void) { return s_ff; }
static void h_ff_toggle(void) { s_hostCalls++; s_ff ^= 1; }
static int h_speed(void) { return s_speed; }
static void h_speed_cycle(void) { s_hostCalls++; s_speed = s_speed % 5 + 1; }
static int h_rec(void) { return s_rec; }
static void h_rec_toggle(void) { s_hostCalls++; s_rec ^= 1; }
static void h_shot(void) { s_hostCalls++; s_shots++; }
static int h_hd2d_avail(void) { return 1; }
static int h_hd2d_on(void) { return s_hd2d; }
static void h_hd2d_toggle(void) { s_hostCalls++; s_hd2d ^= 1; }
static int h_god(void) { return s_god; }
static int h_battles(void) { return 1; }
static int s_confirmCircle = 0;
static int h_confirm(void) { return s_confirmCircle; }
static void h_confirm_toggle(void) { s_hostCalls++; s_confirmCircle ^= 1; }
static const PcPortDevMenuHost s_host = {
    h_ff_on, h_ff_toggle, h_speed, h_speed_cycle, h_rec, h_rec_toggle, h_shot,
    h_hd2d_avail, h_hd2d_on, h_hd2d_toggle, h_god, h_battles, h_confirm, h_confirm_toggle,
};

enum { PS_SELECT = 0x1, PS_START = 0x8, PS_UP = 0x10, PS_DOWN = 0x40, PS_CROSS = 0x4000, PS_CIRCLE = 0x2000 };

static int s_fail = 0;
static unsigned char s_pad[2][0x22];
#define CHECK(c, ...) do { if (c) printf("ok    "); else { printf("FAIL  "); s_fail++; } \
    printf(__VA_ARGS__); printf("\n"); } while (0)

static void mapping(void)
{
    PsyXControllerMapping& m = g_cfg_controllerMapping;
    PsyXKeyboardMapping& k = g_cfg_keyboardMapping;
    memset(&m, 0, sizeof m);
    m.gc_square = SDL_CONTROLLER_BUTTON_X; m.gc_circle = SDL_CONTROLLER_BUTTON_B;
    m.gc_triangle = SDL_CONTROLLER_BUTTON_Y; m.gc_cross = SDL_CONTROLLER_BUTTON_A;
    m.gc_l1 = SDL_CONTROLLER_BUTTON_LEFTSHOULDER; m.gc_r1 = SDL_CONTROLLER_BUTTON_RIGHTSHOULDER;
    m.gc_l2 = SDL_CONTROLLER_AXIS_TRIGGERLEFT | CONTROLLER_MAP_FLAG_AXIS;
    m.gc_r2 = SDL_CONTROLLER_AXIS_TRIGGERRIGHT | CONTROLLER_MAP_FLAG_AXIS;
    m.gc_l3 = SDL_CONTROLLER_BUTTON_LEFTSTICK; m.gc_r3 = SDL_CONTROLLER_BUTTON_RIGHTSTICK;
    m.gc_dpad_up = SDL_CONTROLLER_BUTTON_DPAD_UP; m.gc_dpad_down = SDL_CONTROLLER_BUTTON_DPAD_DOWN;
    m.gc_dpad_left = SDL_CONTROLLER_BUTTON_DPAD_LEFT; m.gc_dpad_right = SDL_CONTROLLER_BUTTON_DPAD_RIGHT;
    m.gc_select = SDL_CONTROLLER_BUTTON_BACK; m.gc_start = SDL_CONTROLLER_BUTTON_START;
    m.gc_axis_left_x = SDL_CONTROLLER_AXIS_LEFTX | CONTROLLER_MAP_FLAG_AXIS;
    m.gc_axis_left_y = SDL_CONTROLLER_AXIS_LEFTY | CONTROLLER_MAP_FLAG_AXIS;
    m.gc_axis_right_x = SDL_CONTROLLER_AXIS_RIGHTX | CONTROLLER_MAP_FLAG_AXIS;
    m.gc_axis_right_y = SDL_CONTROLLER_AXIS_RIGHTY | CONTROLLER_MAP_FLAG_AXIS;
    memset(&k, 0, sizeof k);
    k.kc_cross = SDL_SCANCODE_Z; k.kc_square = SDL_SCANCODE_V; k.kc_circle = SDL_SCANCODE_C;
    k.kc_triangle = SDL_SCANCODE_X; k.kc_start = SDL_SCANCODE_RETURN; k.kc_select = SDL_SCANCODE_SPACE;
    k.kc_dpad_up = SDL_SCANCODE_UP; k.kc_dpad_down = SDL_SCANCODE_DOWN;
    k.kc_dpad_left = SDL_SCANCODE_LEFT; k.kc_dpad_right = SDL_SCANCODE_RIGHT;
    k.kc_l1 = k.kc_l2 = k.kc_l3 = k.kc_r1 = k.kc_r2 = k.kc_r3 = SDL_SCANCODE_UNKNOWN;
}

static void frame(void)
{
    SDL_Event ev;
    SDL_PumpEvents();
    while (SDL_PollEvent(&ev)) {
        if (ev.type == SDL_CONTROLLERDEVICEADDED) PsyX_Pad_Event_ControllerAdded(ev.cdevice.which);
        else if (ev.type == SDL_CONTROLLERDEVICEREMOVED) PsyX_Pad_Event_ControllerRemoved(ev.cdevice.which);
    }
    PsyX_Pad_InternalPadUpdates();
}
static unsigned held(void) { return (unsigned)(~(s_pad[0][2] | (s_pad[0][3] << 8)) & 0xFFFF); }

typedef struct { SDL_Joystick* js; SDL_JoystickID id; } VPad;
static VPad attach(Uint16 v, Uint16 p, const char* name)
{
    SDL_VirtualJoystickDesc d;
    VPad r = { NULL, -1 };
    memset(&d, 0, sizeof d);
    d.version = SDL_VIRTUAL_JOYSTICK_DESC_VERSION;
    d.type = SDL_JOYSTICK_TYPE_GAMECONTROLLER;
    d.naxes = SDL_CONTROLLER_AXIS_MAX;
    d.nbuttons = SDL_CONTROLLER_BUTTON_DPAD_RIGHT + 1;
    d.vendor_id = v; d.product_id = p; d.name = name;
    int i = SDL_JoystickAttachVirtualEx(&d);
    r.js = SDL_JoystickOpen(i);
    r.id = SDL_JoystickInstanceID(r.js);
    return r;
}
static void detach(VPad* p)
{
    for (int i = 0; i < SDL_NumJoysticks(); i++)
        if (SDL_JoystickGetDeviceInstanceID(i) == p->id) { SDL_JoystickClose(p->js); SDL_JoystickDetachVirtual(i); break; }
}
static void btn(VPad* p, SDL_GameControllerButton b, int d) { SDL_JoystickSetVirtualButton(p->js, b, (Uint8)d); }
static void ax(VPad* p, SDL_GameControllerAxis a, int v) { SDL_JoystickSetVirtualAxis(p->js, a, (Sint16)v); }
static void tap(VPad* p, SDL_GameControllerButton b) { btn(p, b, 1); frame(); btn(p, b, 0); frame(); }
static const char* row(int i) { return PcPort_DevMenuRow(i); }

/* Enhancement registry (HD-2D lane) test entries. */
static int s_enhA = 0, s_enhB = 1, s_enhCalls = 0;
static int enh_a_get(void) { return s_enhA; }
static void enh_a_toggle(void) { s_enhA ^= 1; s_enhCalls++; }
static int enh_b_get(void) { return s_enhB; }
static void enh_b_toggle(void) { s_enhB ^= 1; s_enhCalls++; }
static int enh_const_on(void) { return 1; }
static void enh_noop(void) { s_enhCalls++; }
static int s_intensity = 25, s_valueCalls = 0;
static int enh_value_get(void) { return s_intensity; }
static void enh_value_set(int value) { s_intensity = value; s_valueCalls++; }
static void open_enhancements(void)
{
    PcPort_DevMenuKey(PC_DEV_OPEN, 1);
    for (int i = 0; i < 3; i++) PcPort_DevMenuKey(PC_DEV_DOWN, 1);
    PcPort_DevMenuKey(PC_DEV_ACCEPT, 1);     /* OPTIONS */
    PcPort_DevMenuKey(PC_DEV_UP, 1);         /* wraps to ENHANCEMENTS > */
    PcPort_DevMenuKey(PC_DEV_ACCEPT, 1);
}

int main(void)
{
    VPad pad, padB;
    setvbuf(stdout, NULL, _IONBF, 0);
    s_verbose = getenv("DEV_MENU_TEST_VERBOSE") != NULL;
    mapping();
    unsetenv("XENO_DEV_MENU");
    unsetenv("XENO_CHEATS");
    PcPort_DevMenuSetHost(&s_host);

    CHECK(PsyX_Pad_InitSystem() == 1, "pad system up");
    PsyX_Pad_InitPad(0, s_pad[0]);
    PsyX_Pad_InitPad(1, s_pad[1]);
    pad = attach(0x28de, 0x11ff, "Steam Virtual Gamepad"); /* the Legion Go 2's only pad */
    frame();
    CHECK(PsyX_Pad_HostActiveInstance() == pad.id, "Steam virtual pad alone is port 1 at once");

    printf("-- off by default / hard-disable\n");
    PcPort_DevMenuInit();
    CHECK(!PcPort_DevMenuEnabled(), "no opt-in: menu disabled");
    btn(&pad, SDL_CONTROLLER_BUTTON_BACK, 1); btn(&pad, SDL_CONTROLLER_BUTTON_START, 1); frame();
    CHECK(!PcPort_DevMenuOpen() && held() == (PS_SELECT | PS_START),
          "no opt-in: Select+Start reaches the game unchanged (0x%04x), no menu", held());
    btn(&pad, SDL_CONTROLLER_BUTTON_BACK, 0); btn(&pad, SDL_CONTROLLER_BUTTON_START, 0); frame();
    CHECK(PcPort_DevMenuKey(PC_DEV_OPEN, 1) == 0 && !PcPort_DevMenuOpen(), "no opt-in: F8 does nothing");
    setenv("XENO_DEV_MENU", "1", 1); setenv("XENO_CHEATS", "0", 1);
    PcPort_DevMenuInit();
    CHECK(!PcPort_DevMenuEnabled(), "XENO_DEV_MENU=1 + XENO_CHEATS=0: hard-disabled");
    unsetenv("XENO_CHEATS"); unsetenv("XENO_DEV_MENU"); s_cfgDevMenu = 1;
    PcPort_DevMenuInit();
    CHECK(PcPort_DevMenuEnabled(), "config [cheats] dev_menu = on: enabled");
    s_cfgDevMenu = 0; setenv("XENO_DEV_MENU", "1", 1);
    PcPort_DevMenuInit();
    CHECK(PcPort_DevMenuEnabled(), "XENO_DEV_MENU=1: enabled");

    printf("-- all-off identity (spec 8): opt-in on, browse every page, select nothing\n");
    unsigned baseline;
    frame(); baseline = held();
    btn(&pad, SDL_CONTROLLER_BUTTON_BACK, 1); frame();
    CHECK(!PcPort_DevMenuOpen() && held() == 0, "Back alone: held back from the game (combo window), 0x%04x", held());
    btn(&pad, SDL_CONTROLLER_BUTTON_START, 1); frame();
    CHECK(PcPort_DevMenuOpen() && held() == 0, "Back+Start on the same pad opens the menu; game sees neutral");
    frame(); frame();
    CHECK(held() == 0, "combo still held: nothing reaches the game");
    btn(&pad, SDL_CONTROLLER_BUTTON_BACK, 0); btn(&pad, SDL_CONTROLLER_BUTTON_START, 0); frame();
    CHECK(!strcmp(PcPort_DevMenuTitle(), "DEV MENU") && PcPort_DevMenuRows() == 4 &&
          !strcmp(row(0), "WARP >") && !strcmp(row(1), "FINISH AREA >") &&
          !strcmp(row(2), "CHEATS >") && !strcmp(row(3), "OPTIONS >"),
          "root: WARP > FINISH AREA > CHEATS > OPTIONS");
    CHECK(PcPort_DevMenuCursor() == 0, "cursor on WARP");
    btn(&pad, SDL_CONTROLLER_BUTTON_DPAD_DOWN, 1);
    for (int i = 0; i < 40; i++) frame();
    CHECK(PcPort_DevMenuCursor() == 1 && held() == 0, "D-pad down held 40 samples: one row, no auto-repeat");
    btn(&pad, SDL_CONTROLLER_BUTTON_DPAD_DOWN, 0); frame();
    ax(&pad, SDL_CONTROLLER_AXIS_LEFTY, 32767); frame(); frame();
    CHECK(PcPort_DevMenuCursor() == 2, "left stick down: next row (%d)", PcPort_DevMenuCursor());
    ax(&pad, SDL_CONTROLLER_AXIS_LEFTY, 0); frame();
    ax(&pad, SDL_CONTROLLER_AXIS_LEFTY, -32768); frame();
    ax(&pad, SDL_CONTROLLER_AXIS_LEFTY, 0); frame();
    CHECK(PcPort_DevMenuCursor() == 1, "left stick up: previous row");
    tap(&pad, SDL_CONTROLLER_BUTTON_DPAD_UP);
    CHECK(PcPort_DevMenuCursor() == 0, "D-pad up: WARP");
    tap(&pad, SDL_CONTROLLER_BUTTON_A);
    if (g_PcPortDevWarpCount == 0) {
        CHECK(!strcmp(PcPort_DevMenuTitle(), "WARP") && PcPort_DevMenuRows() == 1 &&
              !strcmp(row(0), "(NO VERIFIED WARPS YET)") && !PcPort_DevMenuRowSelectable(0) &&
              PcPort_DevMenuCursor() == -1,
              "A opens WARP: one dimmed '(NO VERIFIED WARPS YET)', nothing selectable");
    } else {
        /* Filled table: every row is an area folder ("<area> >"), none is a
         * placeholder, and A opens the folder that lists named spots. */
        int folders = 1, r;
        CHECK(!strcmp(PcPort_DevMenuTitle(), "WARP") && PcPort_DevMenuRows() >= 1 &&
              PcPort_DevMenuRowSelectable(0) && PcPort_DevMenuCursor() == 0 &&
              row(0)[strlen(row(0)) - 1] == '>',
              "A opens WARP: area folders (%s ...), %d rows", row(0), PcPort_DevMenuRows());
        for (r = 0; r < PcPort_DevMenuRows(); r++)
            if (!PcPort_DevMenuRowSelectable(r) || row(r)[strlen(row(r)) - 1] != '>') folders = 0;
        CHECK(folders, "WARP root lists only selectable folders");
        tap(&pad, SDL_CONTROLLER_BUTTON_A);
        CHECK(PcPort_DevMenuRows() >= 1 && PcPort_DevMenuRowSelectable(0) &&
              strcmp(PcPort_DevMenuTitle(), "WARP") != 0, "A opens the first folder: '%s' with %d spots",
              PcPort_DevMenuTitle(), PcPort_DevMenuRows());
        tap(&pad, SDL_CONTROLLER_BUTTON_B);
        CHECK(!strcmp(PcPort_DevMenuTitle(), "WARP"), "B from a folder returns to WARP");
    }
    if (g_PcPortDevWarpCount == 0) {
        tap(&pad, SDL_CONTROLLER_BUTTON_DPAD_DOWN); tap(&pad, SDL_CONTROLLER_BUTTON_A);
        CHECK(!strcmp(PcPort_DevMenuTitle(), "WARP") && PcPort_DevMenuCursor() == -1,
              "empty group: down / A do nothing");
    }
    tap(&pad, SDL_CONTROLLER_BUTTON_B);
    CHECK(!strcmp(PcPort_DevMenuTitle(), "DEV MENU") && PcPort_DevMenuCursor() == 0, "B: back to root, cursor kept");
    tap(&pad, SDL_CONTROLLER_BUTTON_DPAD_DOWN); tap(&pad, SDL_CONTROLLER_BUTTON_A);
    if (g_PcPortDevStepCount == 0 && g_PcPortDevFinishCount == 0) {
        CHECK(!strcmp(PcPort_DevMenuTitle(), "FINISH AREA") && !strcmp(row(0), "(NO VERIFIED AREAS YET)") &&
              !PcPort_DevMenuRowSelectable(0), "FINISH AREA: empty group");
    } else {
        CHECK(!strcmp(PcPort_DevMenuTitle(), "FINISH AREA") && !strcmp(row(0), "FINISH CURRENT STEP") &&
              PcPort_DevMenuRowSelectable(0), "FINISH AREA: FINISH CURRENT STEP first (%s), %d rows",
              row(0), PcPort_DevMenuRows());
    }
    tap(&pad, SDL_CONTROLLER_BUTTON_B);
    tap(&pad, SDL_CONTROLLER_BUTTON_DPAD_DOWN); tap(&pad, SDL_CONTROLLER_BUTTON_A);
    CHECK(!strcmp(PcPort_DevMenuTitle(), "CHEATS") && !strcmp(row(0), "GOD MODE: OFF") &&
          !strcmp(row(1), "RANDOM BATTLES: ON") && !strcmp(row(2), "REFILL PARTY HP AND EP") &&
          !strcmp(row(3), "ADD 10000 GOLD") && PcPort_DevMenuRows() == 4,
          "CHEATS page lists the verified actions (items top-up hidden): %s / %s / %s / %s, %d rows", row(0), row(1), row(2), row(3), PcPort_DevMenuRows());
    tap(&pad, SDL_CONTROLLER_BUTTON_B);
    tap(&pad, SDL_CONTROLLER_BUTTON_DPAD_DOWN); tap(&pad, SDL_CONTROLLER_BUTTON_A);
    CHECK(!strcmp(PcPort_DevMenuTitle(), "OPTIONS") && !strcmp(row(0), "FAST-FORWARD: OFF") &&
          !strcmp(row(1), "GAME SPEED: 1X") && !strcmp(row(2), "RECORD VIDEO: OFF") &&
          !strcmp(row(3), "SCREENSHOT") && !strcmp(row(4), "QUICK SAVE") &&
          !strcmp(row(5), "QUICK LOAD") && !strcmp(row(6), "CONFIRM BUTTON: CROSS (US)") &&
          !strcmp(row(7), "ENHANCEMENTS >"), "OPTIONS page rows (incl. CONFIRM BUTTON)");
    tap(&pad, SDL_CONTROLLER_BUTTON_DPAD_UP); /* wraps to ENHANCEMENTS */
    tap(&pad, SDL_CONTROLLER_BUTTON_DPAD_UP); /* CONFIRM BUTTON */
    tap(&pad, SDL_CONTROLLER_BUTTON_A);
    CHECK(s_confirmCircle == 1 && !strcmp(row(6), "CONFIRM BUTTON: CIRCLE (JP)") && PcPort_DevMenuOpen(),
          "OPTIONS > CONFIRM BUTTON + A: host toggled, row reads CIRCLE (JP), menu stays open");
    tap(&pad, SDL_CONTROLLER_BUTTON_A);
    CHECK(s_confirmCircle == 0 && !strcmp(row(6), "CONFIRM BUTTON: CROSS (US)"), "A again: back to CROSS (US)");
    s_hostCalls -= 2; /* the identity check below counts only its own host actions */
    tap(&pad, SDL_CONTROLLER_BUTTON_DPAD_DOWN); /* ENHANCEMENTS */
    tap(&pad, SDL_CONTROLLER_BUTTON_A);
    CHECK(!strcmp(PcPort_DevMenuTitle(), "ENHANCEMENTS") && !strcmp(row(0), "HD-2D FEI: ON"),
          "ENHANCEMENTS: HD-2D Fei (atlas present)");
    tap(&pad, SDL_CONTROLLER_BUTTON_B); tap(&pad, SDL_CONTROLLER_BUTTON_B);
    CHECK(!strcmp(PcPort_DevMenuTitle(), "DEV MENU") && PcPort_DevMenuOpen(), "B B: root");
    btn(&pad, SDL_CONTROLLER_BUTTON_B, 1); frame();
    CHECK(!PcPort_DevMenuOpen() && held() == 0, "B at root closes; B still held -> game neutral (drain)");
    frame(); frame();
    CHECK(held() == 0, "drain holds while B is held");
    btn(&pad, SDL_CONTROLLER_BUTTON_B, 0); frame();
    btn(&pad, SDL_CONTROLLER_BUTTON_A, 1); frame();
    CHECK(held() == PS_CROSS, "after release, the game gets A -> Cross again");
    btn(&pad, SDL_CONTROLLER_BUTTON_A, 0); frame();
    CHECK(s_queued == 0 && s_hostCalls == 0 && held() == baseline,
          "identity: no cheat queued (%d), no host action (%d), pad back to baseline", s_queued, s_hostCalls);

    printf("-- Select / Start alone still reach the game\n");
    btn(&pad, SDL_CONTROLLER_BUTTON_START, 1); frame();
    CHECK(held() == 0, "Start pressed: held back during the combo window");
    btn(&pad, SDL_CONTROLLER_BUTTON_START, 0); frame();
    CHECK(held() == PS_START, "Start tapped: the tap is replayed to the game (0x%04x)", held());
    frame();
    CHECK(held() == PS_START, "replay lasts two samples");
    frame();
    CHECK(held() == 0, "then neutral");
    btn(&pad, SDL_CONTROLLER_BUTTON_BACK, 1); frame();
    SDL_Delay(200); frame();
    CHECK(held() == PS_SELECT && !PcPort_DevMenuOpen(), "Select held past the window: passes through");
    btn(&pad, SDL_CONTROLLER_BUTTON_BACK, 0); frame(); frame(); frame();

    printf("-- same pad only: Back on one pad + Start on another does not open\n");
    padB = attach(0x045e, 0x028e, "Xbox 360 Controller");
    frame();
    tap(&pad, SDL_CONTROLLER_BUTTON_A); /* keep the Steam pad the active one (it has input) */
    btn(&padB, SDL_CONTROLLER_BUTTON_BACK, 1); btn(&pad, SDL_CONTROLLER_BUTTON_START, 1); frame(); frame();
    CHECK(!PcPort_DevMenuOpen(), "Back (pad B) + Start (pad A): no menu");
    btn(&padB, SDL_CONTROLLER_BUTTON_BACK, 0); btn(&pad, SDL_CONTROLLER_BUTTON_START, 0);
    for (int i = 0; i < 4; i++) frame();
    detach(&padB); frame();

    printf("-- F8 and the keyboard\n");
    CHECK(PcPort_DevMenuKey(PC_DEV_OPEN, 1) == 1 && PcPort_DevMenuOpen(), "F8 opens");
    PcPort_DevMenuKey(PC_DEV_DOWN, 1); PcPort_DevMenuKey(PC_DEV_DOWN, 1); PcPort_DevMenuKey(PC_DEV_DOWN, 1);
    PcPort_DevMenuKey(PC_DEV_ACCEPT, 1);
    CHECK(!strcmp(PcPort_DevMenuTitle(), "OPTIONS"), "arrows + Enter: OPTIONS");
    PcPort_DevMenuKey(PC_DEV_BACK, 1);
    PsyX_Pad_LatchKeyboardInput(SDL_SCANCODE_Z); frame();
    CHECK(held() == 0, "keyboard Z while the menu is open: not passed to the game");
    PcPort_DevMenuKey(PC_DEV_OPEN, 1);
    PsyX_Pad_LatchKeyboardInput(SDL_SCANCODE_RETURN); frame();
    CHECK(!PcPort_DevMenuOpen() && held() == 0, "F8 closes; a key held while closing is drained");
    frame();
    PsyX_Pad_LatchKeyboardInput(SDL_SCANCODE_Z); frame();
    CHECK(held() == PS_CROSS, "then the keyboard reaches the game again (Z -> Cross)");

    printf("-- actions go through the queue / host (after the identity check)\n");
    /* Refill / gold / items are verified (run_dev_menu_cheats_state_test.sh)
     * and always listed. */
    PcPort_DevMenuKey(PC_DEV_OPEN, 1);
    PcPort_DevMenuKey(PC_DEV_DOWN, 1); PcPort_DevMenuKey(PC_DEV_DOWN, 1); PcPort_DevMenuKey(PC_DEV_ACCEPT, 1);
    frame();
    tap(&pad, SDL_CONTROLLER_BUTTON_A);
    CHECK(s_queued == 1 && !strcmp(s_lastQueued, "god"), "CHEATS > GOD MODE + A queues 'god'");
    s_god = 1;
    CHECK(PcPort_DevMenuCheatsActive() && !strcmp(row(0), "GOD MODE: ON"), "CHEATS marker on while god mode is on");
    s_god = 0;
    tap(&pad, SDL_CONTROLLER_BUTTON_DPAD_DOWN); tap(&pad, SDL_CONTROLLER_BUTTON_DPAD_DOWN);
    tap(&pad, SDL_CONTROLLER_BUTTON_A);
    CHECK(s_queued == 2 && !strcmp(s_lastQueued, "dev heal"), "REFILL PARTY HP AND EP queues 'dev heal'");
    tap(&pad, SDL_CONTROLLER_BUTTON_B);
    tap(&pad, SDL_CONTROLLER_BUTTON_DPAD_DOWN); tap(&pad, SDL_CONTROLLER_BUTTON_A);
    tap(&pad, SDL_CONTROLLER_BUTTON_A);
    CHECK(s_ff == 1 && !strcmp(row(0), "FAST-FORWARD: ON"), "OPTIONS > FAST-FORWARD toggles");
    tap(&pad, SDL_CONTROLLER_BUTTON_DPAD_DOWN); tap(&pad, SDL_CONTROLLER_BUTTON_A);
    CHECK(s_speed == 2 && !strcmp(row(1), "GAME SPEED: 2X"), "GAME SPEED cycles");
    tap(&pad, SDL_CONTROLLER_BUTTON_DPAD_DOWN); tap(&pad, SDL_CONTROLLER_BUTTON_A);
    CHECK(s_rec == 1, "RECORD VIDEO toggles");
    tap(&pad, SDL_CONTROLLER_BUTTON_DPAD_DOWN); tap(&pad, SDL_CONTROLLER_BUTTON_A);
    CHECK(s_shots == 1 && !PcPort_DevMenuOpen(), "SCREENSHOT closes the menu, then shoots");
    for (int i = 0; i < 3; i++) frame();
    PcPort_DevMenuKey(PC_DEV_OPEN, 1);
    for (int i = 0; i < 3; i++) PcPort_DevMenuKey(PC_DEV_DOWN, 1);
    PcPort_DevMenuKey(PC_DEV_ACCEPT, 1);
    for (int i = 0; i < 4; i++) PcPort_DevMenuKey(PC_DEV_DOWN, 1);
    PcPort_DevMenuKey(PC_DEV_ACCEPT, 1);
    CHECK(s_queued == 3 && !strcmp(s_lastQueued, "save") && !PcPort_DevMenuOpen(), "QUICK SAVE queues 'save'");
    frame(); frame();

    printf("-- a pad that appears with a button held must go neutral first\n");
    PcPort_DevMenuKey(PC_DEV_OPEN, 1);
    detach(&pad); frame();
    pad = attach(0x28de, 0x11ff, "Steam Virtual Gamepad");
    btn(&pad, SDL_CONTROLLER_BUTTON_A, 1);
    frame(); frame();
    CHECK(!strcmp(PcPort_DevMenuTitle(), "DEV MENU") && PsyX_Pad_HostActiveInstance() == pad.id,
          "reconnected pad with A held: no activation");
    btn(&pad, SDL_CONTROLLER_BUTTON_A, 0); frame();
    tap(&pad, SDL_CONTROLLER_BUTTON_A);
    CHECK(!strcmp(PcPort_DevMenuTitle(), "WARP"), "after release, A works (WARP)");
    btn(&pad, SDL_CONTROLLER_BUTTON_BACK, 1); btn(&pad, SDL_CONTROLLER_BUTTON_START, 1); frame();
    CHECK(!PcPort_DevMenuOpen(), "Select+Start closes the menu");
    btn(&pad, SDL_CONTROLLER_BUTTON_BACK, 0); btn(&pad, SDL_CONTROLLER_BUTTON_START, 0); frame(); frame();
    CHECK(held() == 0, "combo release drained, nothing leaked");

    printf("-- enhancement registry (PcPort_DevMenuRegisterEnhancement)\n");
    CHECK(PcPort_DevMenuEnhancementCount() == 0, "registry starts empty");
    PcPort_DevMenuRegisterEnhancement("HD-2D AREA", enh_a_get, enh_a_toggle);
    PcPort_DevMenuRegisterEnhancement("HD-2D BLOOM", enh_b_get, enh_b_toggle);
    PcPort_DevMenuRegisterEnhancement("", enh_a_get, enh_a_toggle);
    PcPort_DevMenuRegisterEnhancement(NULL, enh_a_get, enh_a_toggle);
    CHECK(PcPort_DevMenuEnhancementCount() == 2, "two entries (empty/NULL labels refused)");
    open_enhancements();
    CHECK(!strcmp(PcPort_DevMenuTitle(), "ENHANCEMENTS") && PcPort_DevMenuRows() == 3 &&
          !strcmp(row(0), "HD-2D FEI: ON") && !strcmp(row(1), "HD-2D AREA: OFF") &&
          !strcmp(row(2), "HD-2D BLOOM: ON") && PcPort_DevMenuCursor() == 0,
          "registered rows follow HD-2D FEI: %s / %s / %s", row(0), row(1), row(2));
    {
        int hostBefore = s_hostCalls;
        PcPort_DevMenuKey(PC_DEV_DOWN, 1); PcPort_DevMenuKey(PC_DEV_ACCEPT, 1);
        CHECK(s_enhA == 1 && s_enhCalls == 1 && !strcmp(row(1), "HD-2D AREA: ON") &&
              s_hostCalls == hostBefore && s_hd2d == 1, "selecting a registered row calls its toggle only");
        PcPort_DevMenuKey(PC_DEV_DOWN, 1); PcPort_DevMenuKey(PC_DEV_ACCEPT, 1);
        CHECK(s_enhB == 0 && s_enhCalls == 2 && !strcmp(row(2), "HD-2D BLOOM: OFF"), "second entry toggles");
        PcPort_DevMenuKey(PC_DEV_DOWN, 1);
        CHECK(PcPort_DevMenuCursor() == 0, "cursor wraps over the registered rows");
    }
    PcPort_DevMenuRegisterEnhancement("HD-2D AREA", enh_const_on, enh_noop);
    CHECK(PcPort_DevMenuEnhancementCount() == 2 && !strcmp(row(1), "HD-2D AREA: ON"),
          "re-registering a label replaces its entry");
    printf("-- numeric enhancement controls\n");
    PcPort_DevMenuRegisterEnhancementValue("BLOOM INTENSITY", enh_value_get, enh_value_set, 0, 100, 5, "%");
    PcPort_DevMenuRegisterEnhancementValue("WIDE RANGE", enh_value_get, enh_value_set, INT_MIN, INT_MAX, INT_MAX, NULL);
    PcPort_DevMenuRegisterEnhancementValue("INVALID", NULL, enh_value_set, 0, 100, 5, "%");
    PcPort_DevMenuRegisterEnhancementValue("INVALID", enh_value_get, NULL, 0, 100, 5, "%");
    PcPort_DevMenuRegisterEnhancementValue("INVALID", enh_value_get, enh_value_set, 100, 0, 5, "%");
    PcPort_DevMenuRegisterEnhancementValue("BLOOM INTENSITY", enh_value_get, enh_value_set, 0, 100, 0, "%");
    PcPort_DevMenuRegisterEnhancementValue(NULL, enh_value_get, enh_value_set, 0, 100, 5, "%");
    CHECK(PcPort_DevMenuEnhancementCount() == 4 && s_intensity == 25 && s_valueCalls == 0 &&
          !strcmp(row(3), "BLOOM INTENSITY: 25%") && PcPort_DevMenuRowAdjustable(3),
          "numeric registration exposes percentage without changing it; invalid registrations refused");
    {
        int togglesBefore = s_enhCalls, queuedBefore = s_queued, hostBefore = s_hostCalls;
        PcPort_DevMenuKey(PC_DEV_RIGHT, 1); /* Fei toggle row */
        CHECK(s_hd2d == 1 && s_hostCalls == hostBefore, "horizontal input leaves boolean rows unchanged");
        for (int i = 0; i < 3; i++) PcPort_DevMenuKey(PC_DEV_DOWN, 1);
        PcPort_DevMenuKey(PC_DEV_RIGHT, 0);
        CHECK(s_intensity == 25, "numeric key release has no effect");
        PcPort_DevMenuKey(PC_DEV_RIGHT, 1);
        CHECK(s_intensity == 30 && !strcmp(row(3), "BLOOM INTENSITY: 30%"), "Right increases by 5 percent");
        PcPort_DevMenuKey(PC_DEV_LEFT, 1);
        CHECK(s_intensity == 25, "Left decreases by 5 percent");
        PcPort_DevMenuKey(PC_DEV_ACCEPT, 1);
        CHECK(s_intensity == 30 && PcPort_DevMenuOpen(), "Accept increases value and keeps menu open");
        btn(&pad, SDL_CONTROLLER_BUTTON_DPAD_RIGHT, 1);
        for (int i = 0; i < 40; i++) frame();
        CHECK(s_intensity == 35 && held() == 0, "D-pad right held: one adjustment and neutral game input");
        btn(&pad, SDL_CONTROLLER_BUTTON_DPAD_RIGHT, 0); frame();
        tap(&pad, SDL_CONTROLLER_BUTTON_DPAD_LEFT);
        CHECK(s_intensity == 30, "D-pad left decreases value");
        ax(&pad, SDL_CONTROLLER_AXIS_LEFTX, 32767); frame(); frame();
        CHECK(s_intensity == 35 && held() == 0, "left stick right increases once and stays captured");
        ax(&pad, SDL_CONTROLLER_AXIS_LEFTX, 0); frame();
        ax(&pad, SDL_CONTROLLER_AXIS_LEFTX, -32768); frame();
        CHECK(s_intensity == 30, "left stick left decreases value");
        ax(&pad, SDL_CONTROLLER_AXIS_LEFTX, 0); frame();
        int diagonalCalls = s_valueCalls;
        ax(&pad, SDL_CONTROLLER_AXIS_LEFTX, 32767);
        ax(&pad, SDL_CONTROLLER_AXIS_LEFTY, 32767); frame();
        CHECK(PcPort_DevMenuCursor() == 4 && s_intensity == 30 && s_valueCalls == diagonalCalls && held() == 0,
              "down-right stick moves to next value without adjusting it");
        ax(&pad, SDL_CONTROLLER_AXIS_LEFTX, 0); frame();
        ax(&pad, SDL_CONTROLLER_AXIS_LEFTX, 32767); frame();
        CHECK(PcPort_DevMenuCursor() == 4 && s_valueCalls == diagonalCalls,
              "new horizontal press while vertical held cannot adjust a value");
        ax(&pad, SDL_CONTROLLER_AXIS_LEFTX, 0);
        ax(&pad, SDL_CONTROLLER_AXIS_LEFTY, 0); frame();
        ax(&pad, SDL_CONTROLLER_AXIS_LEFTX, -32768);
        ax(&pad, SDL_CONTROLLER_AXIS_LEFTY, -32768); frame();
        CHECK(PcPort_DevMenuCursor() == 3 && s_intensity == 30 && s_valueCalls == diagonalCalls && held() == 0,
              "up-left stick moves to previous value without adjusting it");
        ax(&pad, SDL_CONTROLLER_AXIS_LEFTX, 0);
        ax(&pad, SDL_CONTROLLER_AXIS_LEFTY, 0); frame();
        s_intensity = 98;
        PcPort_DevMenuKey(PC_DEV_RIGHT, 1);
        CHECK(s_intensity == 100, "last partial step clamps at maximum");
        int calls = s_valueCalls;
        PcPort_DevMenuKey(PC_DEV_RIGHT, 1);
        CHECK(s_intensity == 100 && s_valueCalls == calls, "maximum neither wraps nor calls setter again");
        s_intensity = 2;
        PcPort_DevMenuKey(PC_DEV_LEFT, 1);
        CHECK(s_intensity == 0, "last partial step clamps at minimum");
        calls = s_valueCalls;
        PcPort_DevMenuKey(PC_DEV_LEFT, 1);
        CHECK(s_intensity == 0 && s_valueCalls == calls, "minimum neither wraps nor calls setter again");
        PcPort_DevMenuKey(PC_DEV_DOWN, 1);
        s_intensity = INT_MAX - 2;
        PcPort_DevMenuKey(PC_DEV_RIGHT, 1);
        CHECK(s_intensity == INT_MAX, "large positive step clamps without signed overflow");
        s_intensity = INT_MIN + 2;
        PcPort_DevMenuKey(PC_DEV_LEFT, 1);
        CHECK(s_intensity == INT_MIN, "large negative step clamps without signed overflow");
        CHECK(s_enhCalls == togglesBefore && s_hostCalls == hostBefore && s_queued == queuedBefore,
              "numeric edits do not toggle effects, enable Fei, or queue game mutations");
        s_intensity = 25;
        PcPort_DevMenuRegisterEnhancement("BLOOM INTENSITY", enh_const_on, enh_noop);
        CHECK(!PcPort_DevMenuRowAdjustable(3) && !strcmp(row(3), "BLOOM INTENSITY: ON"),
              "toggle registration replaces numeric type");
        PcPort_DevMenuRegisterEnhancementValue("BLOOM INTENSITY", enh_value_get, enh_value_set, 0, 100, 5, "%");
        CHECK(PcPort_DevMenuRowAdjustable(3) && PcPort_DevMenuEnhancementCount() == 4,
              "numeric registration replaces toggle without adding another row");
    }
    for (int i = 0; i < PC_DEV_MAX_ENHANCEMENTS + 2; i++) {
        char label[16];
        snprintf(label, sizeof label, "FX %d", i);
        PcPort_DevMenuRegisterEnhancement(label, i % 2 ? enh_const_on : NULL, enh_noop);
    }
    CHECK(PcPort_DevMenuEnhancementCount() == PC_DEV_MAX_ENHANCEMENTS &&
          PcPort_DevMenuRows() == PC_DEV_MAX_ENHANCEMENTS + 1 &&
          !strcmp(row(5), "FX 0") && !strcmp(row(6), "FX 1: ON") &&
          !strcmp(row(PC_DEV_MAX_ENHANCEMENTS), "FX 19: ON"),
          "all %d registered entries reachable (%d rows, last %s)",
          PC_DEV_MAX_ENHANCEMENTS, PcPort_DevMenuRows(), row(PcPort_DevMenuRows() - 1));
    {
        const int sizes[][2] = {{320, 240}, {640, 480}, {1280, 720}, {1920, 1080}};
        int calls = s_valueCalls;
        for (const auto& size : sizes) {
            bool visible = true, fits = true;
            for (int i = 0; i < PcPort_DevMenuRows(); i++) {
                PcPortDevMenuLayout layout;
                PcPort_DevMenuGetLayout(size[0], size[1], &layout);
                int cursor = PcPort_DevMenuCursor();
                visible &= layout.visible_rows > 0 && layout.first_row <= cursor &&
                    cursor < layout.first_row + layout.visible_rows &&
                    layout.first_row + layout.visible_rows <= PcPort_DevMenuRows();
                fits &= layout.top + 10 * layout.text_scale <= size[1] &&
                    layout.top - layout.row_height * (layout.visible_rows + 3) - 4 * layout.text_scale >= 0;
                PcPort_DevMenuKey(PC_DEV_DOWN, 1);
            }
            CHECK(visible && fits, "every cursor and footer stays inside %dx%d viewport", size[0], size[1]);
        }
        PcPortDevMenuLayout layout;
        PcPort_DevMenuGetLayout(320, 240, &layout);
        CHECK(layout.text_scale == 1 && layout.visible_rows < PcPort_DevMenuRows(),
              "320x240 uses compact text and scrolls");
        PcPort_DevMenuGetLayout(640, 480, &layout);
        CHECK(layout.text_scale == 2 && layout.visible_rows < PcPort_DevMenuRows(),
              "640x480 preserves readable text and scrolls");
        PcPort_DevMenuGetLayout(0, 0, &layout);
        CHECK(layout.visible_rows == 0 && s_valueCalls == calls, "empty viewport and browsing leave values untouched");
        for (int width = 1; width <= 16; ++width) {
            PcPort_DevMenuGetLayout(width, 480, &layout);
            CHECK(layout.visible_rows == 0, "width %d skips drawing before any invalid scissor rectangle", width);
        }
    }
    PcPort_DevMenuKey(PC_DEV_OPEN, 1);
    CHECK(!PcPort_DevMenuOpen(), "F8 closes");
    frame(); frame();

    SDL_Quit();
    printf("dev_menu_test: %s (%d failure%s)\n", s_fail ? "FAIL" : "PASS", s_fail, s_fail == 1 ? "" : "s");
    return s_fail ? 1 : 0;
}
