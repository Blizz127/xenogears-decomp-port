/* Host game-controller selection + mapping test (no hardware).
 *
 * Builds PsyCross's patched src/pad/PsyX_pad.cpp (with
 * pc_port/src/psycross_pad_select.inl) against the system SDL2 and drives it
 * with SDL's virtual joystick API (SDL_JoystickAttachVirtualEx, SDL >= 2.24):
 *   - Steam's inherited SDL_GAMECONTROLLER_IGNORE_DEVICES is sanitised
 *     (owner's exact value from the Legion Go / Bazzite session) and applied
 *     with a priority that beats the environment;
 *   - with an Xbox-style pad (045e:028e) and a Steam virtual pad (28de:11ff)
 *     attached, the Xbox pad feeds port 1 and the Steam pad cannot double it;
 *   - SDL buttons / D-pad / left stick produce the PS1 active-low bits in the
 *     buffer the game reads (the PsyX_Pad_InitPad buffer);
 *   - input.pad_confirm's Cross/Circle exchange (xg_plat_input_set_pad_face_swap);
 *   - hotplug: removal by instance id, fallback, Steam-virtual-only, a silent
 *     physical pad next to an active Steam pad, idle hand-over between pads;
 *   - the keyboard keeps working with and without a pad.
 *   - pad fast-forward (input.pad_fast_forward, default off): a device where A
 *     also asserts RIGHTSTICK must not change the game speed; with the option
 *     on, only a lone L3 / R3 does.
 * Run: pc_port/tests/run_pad_select_test.sh (inside xenogears-dev). */
#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "PsyX/PsyX_public.h"

/* PsyX_pad.cpp (C++ linkage, as PsyX_main.cpp declares them) */
int PsyX_Pad_InitSystem();
void PsyX_Pad_Event_ControllerAdded(Sint32 deviceId);
void PsyX_Pad_Event_ControllerRemoved(Sint32 deviceId);

extern "C" {
/* PsyX_pad.cpp (PsyX_pad.h) */
void PsyX_Pad_InitPad(int slot, unsigned char* padData);
void PsyX_Pad_InternalPadUpdates();
void PsyX_Pad_LatchKeyboardInput(int scancode);
int PsyX_Pad_GetStatus(int mtap, int slot);
/* psycross_pad_select.inl */
int PsyX_Pad_SanitizeIgnoreDevices(const char* in, char* out, size_t outSize,
                                   char* removed, size_t removedSize);
int PsyX_Pad_HostActiveSource();
SDL_JoystickID PsyX_Pad_HostActiveInstance();
const char* PsyX_Pad_SourceName(int source);
/* pc_port/src/plat/xg_plat_psycross.c */
void xg_plat_input_set_pad_face_swap(int swapped);

/* Stand-ins for the host speed controls (PsyX_main.cpp hotkeys + host speed):
 * fast-forward = L3 toggle or R3 hold -> 5x, else 1x; each change is logged
 * with its source like the real "speed:" line. */
static int s_ffEnabled = 0, s_ffToggle = 0, s_ffHold = 0, s_speedChanges = 0;
static int ff_speed(void) { return (s_ffToggle || s_ffHold) ? 5 : 1; }
static void ff_update(const char* source)
{
    static int last = 1;
    if (ff_speed() != last) {
        last = ff_speed();
        s_speedChanges++;
        printf("  speed: %dx (source: %s)\n", last, source);
    }
}
int PcPort_PadFastForwardEnabled(void) { return s_ffEnabled; }
void PsyX_HostFastForwardToggleFrom(const char* source) { s_ffToggle ^= 1; ff_update(source); }
void PsyX_HostFastForwardPadHoldFrom(int held, const char* source) { s_ffHold = held != 0; ff_update(source); }

/* Symbols PsyX_pad.cpp takes from the rest of PsyCross. */
int g_padCommEnable = 1;
int g_activeKeyboardControllers = 0x1;
PsyXControllerMapping g_cfg_controllerMapping;
PsyXKeyboardMapping g_cfg_keyboardMapping;
static void log_line(const char* tag, const char* fmt, va_list ap)
{
    printf("  %s", tag);
    vprintf(fmt, ap);
}
void PsyX_Log(const char* fmt, ...) { va_list ap; va_start(ap, fmt); log_line("", fmt, ap); va_end(ap); }
void PsyX_Log_Info(const char* fmt, ...) { va_list ap; va_start(ap, fmt); log_line("", fmt, ap); va_end(ap); }
void PsyX_Log_Warning(const char* fmt, ...) { va_list ap; va_start(ap, fmt); log_line("[warn] ", fmt, ap); va_end(ap); }
void PsyX_Log_Error(const char* fmt, ...) { va_list ap; va_start(ap, fmt); log_line("[err] ", fmt, ap); va_end(ap); }
void PsyX_Log_Success(const char* fmt, ...) { va_list ap; va_start(ap, fmt); log_line("", fmt, ap); va_end(ap); }
}

/* PsyCross's defaults (src/PsyX_main.cpp PsyX_Sys_InitialiseInput; the run
 * script checks that file still carries exactly these). */
static void set_psycross_default_mapping(void)
{
    PsyXControllerMapping& m = g_cfg_controllerMapping;
    PsyXKeyboardMapping& k = g_cfg_keyboardMapping;
    memset(&m, 0, sizeof m);
    m.gc_square = SDL_CONTROLLER_BUTTON_X;
    m.gc_circle = SDL_CONTROLLER_BUTTON_B;
    m.gc_triangle = SDL_CONTROLLER_BUTTON_Y;
    m.gc_cross = SDL_CONTROLLER_BUTTON_A;
    m.gc_l1 = SDL_CONTROLLER_BUTTON_LEFTSHOULDER;
    m.gc_l2 = SDL_CONTROLLER_AXIS_TRIGGERLEFT | CONTROLLER_MAP_FLAG_AXIS;
    m.gc_l3 = SDL_CONTROLLER_BUTTON_LEFTSTICK;
    m.gc_r1 = SDL_CONTROLLER_BUTTON_RIGHTSHOULDER;
    m.gc_r2 = SDL_CONTROLLER_AXIS_TRIGGERRIGHT | CONTROLLER_MAP_FLAG_AXIS;
    m.gc_r3 = SDL_CONTROLLER_BUTTON_RIGHTSTICK;
    m.gc_dpad_up = SDL_CONTROLLER_BUTTON_DPAD_UP;
    m.gc_dpad_down = SDL_CONTROLLER_BUTTON_DPAD_DOWN;
    m.gc_dpad_left = SDL_CONTROLLER_BUTTON_DPAD_LEFT;
    m.gc_dpad_right = SDL_CONTROLLER_BUTTON_DPAD_RIGHT;
    m.gc_select = SDL_CONTROLLER_BUTTON_BACK;
    m.gc_start = SDL_CONTROLLER_BUTTON_START;
    m.gc_axis_left_x = SDL_CONTROLLER_AXIS_LEFTX | CONTROLLER_MAP_FLAG_AXIS;
    m.gc_axis_left_y = SDL_CONTROLLER_AXIS_LEFTY | CONTROLLER_MAP_FLAG_AXIS;
    m.gc_axis_right_x = SDL_CONTROLLER_AXIS_RIGHTX | CONTROLLER_MAP_FLAG_AXIS;
    m.gc_axis_right_y = SDL_CONTROLLER_AXIS_RIGHTY | CONTROLLER_MAP_FLAG_AXIS;
    /* The port's USA keyboard map (port_main.c): Z confirm = Cross. */
    memset(&k, 0, sizeof k);
    k.kc_cross = SDL_SCANCODE_Z;
    k.kc_square = SDL_SCANCODE_V;
    k.kc_circle = SDL_SCANCODE_C;
    k.kc_triangle = SDL_SCANCODE_X;
    k.kc_l1 = SDL_SCANCODE_Q; k.kc_l2 = SDL_SCANCODE_1; k.kc_l3 = SDL_SCANCODE_2;
    k.kc_r1 = SDL_SCANCODE_E; k.kc_r2 = SDL_SCANCODE_3; k.kc_r3 = SDL_SCANCODE_4;
    k.kc_start = SDL_SCANCODE_RETURN; k.kc_select = SDL_SCANCODE_SPACE;
    k.kc_dpad_left = SDL_SCANCODE_LEFT; k.kc_dpad_right = SDL_SCANCODE_RIGHT;
    k.kc_dpad_up = SDL_SCANCODE_UP; k.kc_dpad_down = SDL_SCANCODE_DOWN;
}

enum {
    PS_SELECT = 0x1, PS_L3 = 0x2, PS_R3 = 0x4, PS_START = 0x8,
    PS_UP = 0x10, PS_RIGHT = 0x20, PS_DOWN = 0x40, PS_LEFT = 0x80,
    PS_L2 = 0x100, PS_R2 = 0x200, PS_L1 = 0x400, PS_R1 = 0x800,
    PS_TRIANGLE = 0x1000, PS_CIRCLE = 0x2000, PS_CROSS = 0x4000, PS_SQUARE = 0x8000,
};

static int s_failures = 0;
static unsigned char s_padBuffer[2][0x22];

#define CHECK(cond, ...) do { \
    if (cond) { printf("ok    "); } else { printf("FAIL  "); s_failures++; } \
    printf(__VA_ARGS__); printf("\n"); } while (0)

/* One game frame, as PsyX_UpdateInput does: SDL events, then the pad update. */
static void frame(void)
{
    SDL_Event ev;
    SDL_PumpEvents();
    while (SDL_PollEvent(&ev)) {
        if (ev.type == SDL_CONTROLLERDEVICEADDED)
            PsyX_Pad_Event_ControllerAdded(ev.cdevice.which);
        else if (ev.type == SDL_CONTROLLERDEVICEREMOVED)
            PsyX_Pad_Event_ControllerRemoved(ev.cdevice.which);
    }
    PsyX_Pad_InternalPadUpdates();
}

/* Pressed PS1 bits (active-high) of port 1, as the game's buffer holds them. */
static unsigned held(void)
{
    return (unsigned)(~(s_padBuffer[0][2] | (s_padBuffer[0][3] << 8)) & 0xFFFF);
}

typedef struct { SDL_Joystick* js; SDL_JoystickID id; const char* label; } VPad;

static VPad attach(Uint16 vendor, Uint16 product, const char* name, const char* label)
{
    SDL_VirtualJoystickDesc d;
    VPad p = { NULL, -1, label };
    int index;
    memset(&d, 0, sizeof d);
    d.version = SDL_VIRTUAL_JOYSTICK_DESC_VERSION;
    d.type = SDL_JOYSTICK_TYPE_GAMECONTROLLER;
    d.naxes = SDL_CONTROLLER_AXIS_MAX;
    d.nbuttons = SDL_CONTROLLER_BUTTON_DPAD_RIGHT + 1;
    d.vendor_id = vendor;
    d.product_id = product;
    d.name = name;
    index = SDL_JoystickAttachVirtualEx(&d);
    if (index < 0) {
        printf("FAIL  attach %s: %s\n", label, SDL_GetError());
        s_failures++;
        return p;
    }
    p.js = SDL_JoystickOpen(index);
    p.id = SDL_JoystickInstanceID(p.js);
    return p;
}

static void detach(VPad* p)
{
    for (int i = 0; i < SDL_NumJoysticks(); i++) {
        if (SDL_JoystickGetDeviceInstanceID(i) == p->id) {
            SDL_JoystickClose(p->js);
            SDL_JoystickDetachVirtual(i);
            break;
        }
    }
    p->js = NULL;
}

static void button(VPad* p, SDL_GameControllerButton b, int down)
{
    SDL_JoystickSetVirtualButton(p->js, b, (Uint8)down);
}

static void axis(VPad* p, SDL_GameControllerAxis a, int v)
{
    SDL_JoystickSetVirtualAxis(p->js, a, (Sint16)v);
}

static void release_all(VPad* p)
{
    for (int b = 0; b <= SDL_CONTROLLER_BUTTON_DPAD_RIGHT; b++)
        button(p, (SDL_GameControllerButton)b, 0);
    for (int a = 0; a < SDL_CONTROLLER_AXIS_MAX; a++)
        axis(p, (SDL_GameControllerAxis)a, 0);
}

/* Press, sample one frame, release, sample again (so the next check starts
 * from an idle pad). Returns the bits held during the press frame. */
static unsigned tap(VPad* p, SDL_GameControllerButton b)
{
    unsigned bits;
    button(p, b, 1);
    frame();
    bits = held();
    button(p, b, 0);
    frame();
    return bits;
}

static unsigned tilt(VPad* p, int x, int y)
{
    axis(p, SDL_CONTROLLER_AXIS_LEFTX, x);
    axis(p, SDL_CONTROLLER_AXIS_LEFTY, y);
    frame();
    return held();
}

static const char* OWNER_IGNORE =
    "0x28de/0x1002,0x28de/0x1003,0x28de/0x1071,0x28de/0x1052,0x28de/0x1042,"
    "0x28de/0x1203,0x28de/0x1204,0x28de/0x1205,0x28de/0x1206,0x28de/0x1302,"
    "0x28de/0x1303,0x28de/0x1304,0x28de/0x1305,0x28de/0x12ff";
static const char* OWNER_IGNORE_SANITISED =
    "0x28de/0x1002,0x28de/0x1003,0x28de/0x1071,0x28de/0x1052,0x28de/0x1042,"
    "0x28de/0x1203,0x28de/0x1204,0x28de/0x1302,"
    "0x28de/0x1303,0x28de/0x1304,0x28de/0x1305";

static void test_sanitiser(void)
{
    char out[1024], removed[256];
    int n;

    printf("-- 6. SDL_GAMECONTROLLER_IGNORE_DEVICES sanitiser\n");
    n = PsyX_Pad_SanitizeIgnoreDevices(OWNER_IGNORE, out, sizeof out, removed, sizeof removed);
    CHECK(n == 3, "owner's value: %d entries removed (want 3)", n);
    CHECK(strcmp(removed, "0x28de/0x1205,0x28de/0x1206,0x28de/0x12ff") == 0, "removed = %s", removed);
    CHECK(strcmp(out, OWNER_IGNORE_SANITISED) == 0, "kept    = %s", out);

    /* Legion entries of any product, other vendors kept, whitespace, odd
     * tokens preserved verbatim. */
    n = PsyX_Pad_SanitizeIgnoreDevices(
        "0x045e/0x028e, 0x17ef/0x6182 ,0x17EF/0x61EB,0x054c/0x0ce6,0x28DE/0x1205,bogus,0x28de/0x11ff",
        out, sizeof out, removed, sizeof removed);
    CHECK(n == 3, "mixed list: %d entries removed (want 3: two 17ef, one 28de/1205)", n);
    CHECK(strcmp(out, "0x045e/0x028e,0x054c/0x0ce6,bogus,0x28de/0x11ff") == 0, "mixed kept = %s", out);
    /* Legion Go 2 under InputPlumber: 28de:12fb, and Steam lists the whole
     * InputPlumber range 28de:12f0-12ff; 28de:12ef/1300 stay ignored. */
    n = PsyX_Pad_SanitizeIgnoreDevices(
        "0x28de/0x12ef,0x28de/0x12f0,0x28de/0x12fb,0x28de/0x12ff,0x28de/0x1300,0x28de/0x11ff",
        out, sizeof out, removed, sizeof removed);
    CHECK(n == 3 && strcmp(removed, "0x28de/0x12f0,0x28de/0x12fb,0x28de/0x12ff") == 0,
          "InputPlumber range 28de:12f0-12ff removed (Legion Go 2 = 12fb): %s", removed);
    CHECK(strcmp(out, "0x28de/0x12ef,0x28de/0x1300,0x28de/0x11ff") == 0, "range edges kept: %s", out);
    n = PsyX_Pad_SanitizeIgnoreDevices("", out, sizeof out, removed, sizeof removed);
    CHECK(n == 0 && out[0] == '\0', "empty list stays empty");
    n = PsyX_Pad_SanitizeIgnoreDevices(OWNER_IGNORE, out, 16, NULL, 0);
    CHECK(n == -1, "too-small output buffer is refused (-1), got %d", n);
}

int main(void)
{
    char envIgnore[1024];
    VPad steam, xbox, xbox2, steam2, padA, padB;

    setvbuf(stdout, NULL, _IONBF, 0);
    set_psycross_default_mapping();
    test_sanitiser();

    /* The environment a Steam shortcut hands the game (owner's session). */
    snprintf(envIgnore, sizeof envIgnore, "%s,0x28de/0x12fb,0x28de/0x12f0,0x17ef/0x6182,0x17ef/0x61eb", OWNER_IGNORE);
    setenv("SDL_GAMECONTROLLER_IGNORE_DEVICES", envIgnore, 1);
    setenv("SDL_GAMECONTROLLER_ALLOW_STEAM_VIRTUAL_GAMEPAD", "1", 1);
    unsetenv("SDL_JOYSTICK_HIDAPI_STEAMDECK");

    printf("-- init (PsyX_Pad_InitSystem: hints, then the GameController subsystem)\n");
    {
        SDL_version v;
        SDL_GetVersion(&v);
        CHECK(SDL_VERSIONNUM(v.major, v.minor, v.patch) >= SDL_VERSIONNUM(2, 24, 0),
              "linked SDL %d.%d.%d has the virtual joystick API", v.major, v.minor, v.patch);
    }
    CHECK(PsyX_Pad_InitSystem() == 1, "PsyX_Pad_InitSystem");
    {
        const char* hint = SDL_GetHint(SDL_HINT_GAMECONTROLLER_IGNORE_DEVICES);
        CHECK(hint && strcmp(hint, OWNER_IGNORE_SANITISED) == 0,
              "SDL reads the sanitised list despite the environment: %s", hint ? hint : "(null)");
        CHECK(strcmp(getenv("SDL_GAMECONTROLLER_IGNORE_DEVICES"), envIgnore) == 0,
              "environment itself untouched (child processes see Steam's value)");
        hint = SDL_GetHint("SDL_JOYSTICK_HIDAPI_STEAMDECK");
        CHECK(hint && strcmp(hint, "1") == 0, "SDL_JOYSTICK_HIDAPI_STEAMDECK = %s", hint ? hint : "(null)");
    }
    PsyX_Pad_InitPad(0, s_padBuffer[0]);
    PsyX_Pad_InitPad(1, s_padBuffer[1]);
    frame();
    CHECK(held() == 0 && s_padBuffer[0][1] == 0x41, "no controller: port 1 digital (0x41), idle");
    PsyX_Pad_LatchKeyboardInput(SDL_SCANCODE_Z);
    frame();
    CHECK(held() == PS_CROSS, "keyboard only: Z -> Cross (0x%04x)", held());
    frame();

    printf("-- 1/2. Steam virtual pad + Xbox-style pad: the physical-looking one is chosen\n");
    steam = attach(0x28de, 0x11ff, "Microsoft X-Box 360 pad 0", "steam-virtual");
    frame();
    CHECK(PsyX_Pad_HostActiveInstance() == steam.id, "only the Steam pad attached: it is port 1");
    xbox = attach(0x045e, 0x028e, "Xbox 360 Controller", "xbox");
    frame();
    CHECK(PsyX_Pad_HostActiveInstance() == xbox.id,
          "Xbox pad (attached second) takes port 1 over the silent Steam pad (active=%d xbox=%d steam=%d)",
          (int)PsyX_Pad_HostActiveInstance(), (int)xbox.id, (int)steam.id);
    CHECK(PsyX_Pad_HostActiveSource() >= 0 &&
          strcmp(PsyX_Pad_SourceName(PsyX_Pad_HostActiveSource()), "sdl-virtual") == 0,
          "source = %s", PsyX_Pad_SourceName(PsyX_Pad_HostActiveSource()));
    CHECK(PsyX_Pad_GetStatus(0, 0) == 1 && PsyX_Pad_GetStatus(0, 1) == 0,
          "port 1 connected, port 2 has no pad (no mirrored second controller)");

    printf("-- 3. SDL buttons -> PS1 bits in the game's pad buffer (PsyCross gc_* defaults)\n");
    CHECK(tap(&xbox, SDL_CONTROLLER_BUTTON_A) == PS_CROSS, "A -> Cross (confirm, retail USA)");
    CHECK(tap(&xbox, SDL_CONTROLLER_BUTTON_B) == PS_CIRCLE, "B -> Circle");
    CHECK(tap(&xbox, SDL_CONTROLLER_BUTTON_X) == PS_SQUARE, "X -> Square");
    CHECK(tap(&xbox, SDL_CONTROLLER_BUTTON_Y) == PS_TRIANGLE, "Y -> Triangle");
    CHECK(tap(&xbox, SDL_CONTROLLER_BUTTON_LEFTSHOULDER) == PS_L1, "LB -> L1");
    CHECK(tap(&xbox, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER) == PS_R1, "RB -> R1");
    CHECK(tap(&xbox, SDL_CONTROLLER_BUTTON_LEFTSTICK) == PS_L3, "L-stick click -> L3");
    CHECK(tap(&xbox, SDL_CONTROLLER_BUTTON_RIGHTSTICK) == PS_R3, "R-stick click -> R3");
    CHECK(tap(&xbox, SDL_CONTROLLER_BUTTON_BACK) == PS_SELECT, "Back -> Select");
    CHECK(tap(&xbox, SDL_CONTROLLER_BUTTON_START) == PS_START, "Start -> Start");
    CHECK(tap(&xbox, SDL_CONTROLLER_BUTTON_DPAD_UP) == PS_UP, "D-pad up -> Up");
    CHECK(tap(&xbox, SDL_CONTROLLER_BUTTON_DPAD_DOWN) == PS_DOWN, "D-pad down -> Down");
    CHECK(tap(&xbox, SDL_CONTROLLER_BUTTON_DPAD_LEFT) == PS_LEFT, "D-pad left -> Left");
    CHECK(tap(&xbox, SDL_CONTROLLER_BUTTON_DPAD_RIGHT) == PS_RIGHT, "D-pad right -> Right");
    axis(&xbox, SDL_CONTROLLER_AXIS_TRIGGERLEFT, 32767); frame();
    CHECK(held() == PS_L2, "LT -> L2 (0x%04x)", held());
    axis(&xbox, SDL_CONTROLLER_AXIS_TRIGGERLEFT, 0);
    axis(&xbox, SDL_CONTROLLER_AXIS_TRIGGERRIGHT, 32767); frame();
    CHECK(held() == PS_R2, "RT -> R2 (0x%04x)", held());
    axis(&xbox, SDL_CONTROLLER_AXIS_TRIGGERRIGHT, 0); frame();
    button(&xbox, SDL_CONTROLLER_BUTTON_A, 1);
    button(&xbox, SDL_CONTROLLER_BUTTON_DPAD_UP, 1); frame();
    CHECK(held() == (PS_CROSS | PS_UP), "A + D-pad up together -> Cross|Up (0x%04x)", held());
    release_all(&xbox); frame();
    CHECK(held() == 0, "released -> idle");

    printf("-- 3b. left stick = D-pad on the digital pad (deadzone, 8-way, hysteresis)\n");
    CHECK(tilt(&xbox, 0, -32768) == PS_UP, "stick up -> Up");
    CHECK(tilt(&xbox, 0, 32767) == PS_DOWN, "stick down -> Down");
    CHECK(tilt(&xbox, -32768, 0) == PS_LEFT, "stick left -> Left");
    CHECK(tilt(&xbox, 32767, 0) == PS_RIGHT, "stick right -> Right");
    CHECK(tilt(&xbox, 23170, -23170) == (PS_UP | PS_RIGHT), "stick up-right (45 deg) -> Up|Right");
    CHECK(tilt(&xbox, 30000, -8000) == PS_RIGHT, "stick 15 deg above right -> Right only");
    CHECK(tilt(&xbox, 0, 0) == 0, "centred -> nothing");
    CHECK(tilt(&xbox, 12000, 0) == 0, "12000 (inside the 13000 deadzone) -> nothing");
    CHECK(tilt(&xbox, 14000, 0) == PS_RIGHT, "14000 -> Right");
    CHECK(tilt(&xbox, 11000, 0) == PS_RIGHT, "back to 11000 -> still Right (hysteresis)");
    CHECK(tilt(&xbox, 9000, 0) == 0, "9000 -> released");
    tilt(&xbox, 0, 0);
    s_padBuffer[0][1] = 0x73; /* analog mode (Select+Start): the stick is analog data */
    CHECK(tilt(&xbox, 0, -32768) == 0 && s_padBuffer[0][7] < 16,
          "analog pad (0x73): stick stays analog (no D-pad), analog[3]=%u", s_padBuffer[0][7]);
    tilt(&xbox, 0, 0);
    s_padBuffer[0][1] = 0x41;

    printf("-- 3c. input.pad_confirm = circle exchanges physical Cross/Circle\n");
    xg_plat_input_set_pad_face_swap(1);
    xg_plat_input_set_pad_face_swap(1); /* idempotent */
    CHECK(tap(&xbox, SDL_CONTROLLER_BUTTON_B) == PS_CROSS, "circle scheme: B (east) -> Cross = the game's confirm");
    CHECK(tap(&xbox, SDL_CONTROLLER_BUTTON_A) == PS_CIRCLE, "circle scheme: A (south) -> Circle");
    CHECK(tap(&xbox, SDL_CONTROLLER_BUTTON_X) == PS_SQUARE, "circle scheme: X unchanged -> Square");
    xg_plat_input_set_pad_face_swap(0);
    CHECK(tap(&xbox, SDL_CONTROLLER_BUTTON_A) == PS_CROSS, "cross scheme restored: A -> Cross");


    printf("-- 3d. pad fast-forward is opt-in; a device where A also asserts RIGHTSTICK must not change speed\n");
    {
        s_speedChanges = 0;
        /* default config: off.  A with RIGHTSTICK (the owner's SteamOS Handheld Controller). */
        button(&xbox, SDL_CONTROLLER_BUTTON_A, 1);
        button(&xbox, SDL_CONTROLLER_BUTTON_RIGHTSTICK, 1);
        for (int i = 0; i < 6; i++) frame();
        CHECK(ff_speed() == 1 && s_speedChanges == 0, "default (off): A + R3 -> speed unchanged");
        release_all(&xbox); frame();
        button(&xbox, SDL_CONTROLLER_BUTTON_RIGHTSTICK, 1);
        for (int i = 0; i < 6; i++) frame();
        CHECK(ff_speed() == 1 && s_speedChanges == 0, "default (off): a lone R3 -> speed unchanged");
        release_all(&xbox); frame();
        button(&xbox, SDL_CONTROLLER_BUTTON_LEFTSTICK, 1);
        for (int i = 0; i < 6; i++) frame();
        release_all(&xbox); frame();
        CHECK(ff_speed() == 1 && s_speedChanges == 0, "default (off): a lone L3 -> no toggle");

        s_ffEnabled = 1;
        button(&xbox, SDL_CONTROLLER_BUTTON_A, 1);
        button(&xbox, SDL_CONTROLLER_BUTTON_RIGHTSTICK, 1);
        for (int i = 0; i < 6; i++) frame();
        CHECK(ff_speed() == 1 && s_speedChanges == 0, "on: A + R3 together -> speed unchanged");
        release_all(&xbox); frame();
        button(&xbox, SDL_CONTROLLER_BUTTON_RIGHTSTICK, 1); frame();
        button(&xbox, SDL_CONTROLLER_BUTTON_A, 1);
        for (int i = 0; i < 6; i++) frame();
        CHECK(ff_speed() == 1 && s_speedChanges == 0, "on: R3 one frame ahead of A (skewed report) -> unchanged");
        release_all(&xbox); frame();
        button(&xbox, SDL_CONTROLLER_BUTTON_DPAD_UP, 1);
        button(&xbox, SDL_CONTROLLER_BUTTON_LEFTSTICK, 1);
        for (int i = 0; i < 6; i++) frame();
        release_all(&xbox); frame();
        CHECK(ff_speed() == 1 && s_speedChanges == 0, "on: D-pad + L3 -> no toggle");
        button(&xbox, SDL_CONTROLLER_BUTTON_LEFTSTICK, 1);
        button(&xbox, SDL_CONTROLLER_BUTTON_RIGHTSTICK, 1);
        for (int i = 0; i < 6; i++) frame();
        release_all(&xbox); frame();
        CHECK(ff_speed() == 1 && s_speedChanges == 0, "on: L3 + R3 together -> unchanged");

        button(&xbox, SDL_CONTROLLER_BUTTON_RIGHTSTICK, 1);
        for (int i = 0; i < 6; i++) frame();
        CHECK(ff_speed() == 5 && s_speedChanges == 1, "on: a lone R3 held -> fast-forward (5x)");
        release_all(&xbox); frame();
        CHECK(ff_speed() == 1 && s_speedChanges == 2, "on: R3 released -> 1x");
        button(&xbox, SDL_CONTROLLER_BUTTON_LEFTSTICK, 1);
        for (int i = 0; i < 6; i++) frame();
        release_all(&xbox); frame();
        CHECK(ff_speed() == 5 && s_speedChanges == 3, "on: a lone L3 click -> toggle on (exactly once)");
        button(&xbox, SDL_CONTROLLER_BUTTON_LEFTSTICK, 1);
        for (int i = 0; i < 6; i++) frame();
        release_all(&xbox); frame();
        CHECK(ff_speed() == 1 && s_speedChanges == 4, "on: second lone L3 click -> toggle off");
        s_ffEnabled = 0;
    }

    printf("-- no double input from the Steam virtual pad\n");
    CHECK(tap(&steam, SDL_CONTROLLER_BUTTON_B) == 0 && PsyX_Pad_HostActiveInstance() == xbox.id,
          "B on the Steam pad alone is not fed to port 1 (Xbox pad stays active)");
    button(&xbox, SDL_CONTROLLER_BUTTON_A, 1);
    button(&steam, SDL_CONTROLLER_BUTTON_A, 1); frame();
    CHECK(held() == PS_CROSS && PsyX_Pad_HostActiveInstance() == xbox.id,
          "mirrored A on both (Steam Input on) -> one Cross, Xbox pad stays active");
    release_all(&xbox); release_all(&steam); frame();

    printf("-- keyboard merges with the pad\n");
    button(&xbox, SDL_CONTROLLER_BUTTON_DPAD_LEFT, 1);
    PsyX_Pad_LatchKeyboardInput(SDL_SCANCODE_V); frame();
    CHECK(held() == (PS_LEFT | PS_SQUARE), "pad Left + keyboard V -> Left|Square (0x%04x)", held());
    release_all(&xbox); frame();

    printf("-- 4. detach the active pad -> fallback to the Steam virtual pad (instance-id removal)\n");
    detach(&xbox);
    frame();
    CHECK(PsyX_Pad_HostActiveInstance() == steam.id, "Steam virtual pad is port 1 after the Xbox pad left");
    CHECK(tap(&steam, SDL_CONTROLLER_BUTTON_A) == PS_CROSS, "Steam pad A -> Cross");

    printf("-- 5. Steam-virtual-only, then a new physical pad\n");
    xbox2 = attach(0x045e, 0x028e, "Xbox 360 Controller", "xbox2");
    frame();
    CHECK(PsyX_Pad_HostActiveInstance() == steam.id,
          "a new, silent physical pad does not steal port 1 from a Steam pad in use");
    CHECK(tap(&xbox2, SDL_CONTROLLER_BUTTON_Y) == PS_TRIANGLE && PsyX_Pad_HostActiveInstance() == xbox2.id,
          "its first press moves port 1 to it (and is delivered: Y -> Triangle)");
    detach(&xbox2);
    detach(&steam);
    frame();
    CHECK(PsyX_Pad_HostActiveInstance() == -1 && held() == 0, "all pads gone -> keyboard only");
    PsyX_Pad_LatchKeyboardInput(SDL_SCANCODE_RETURN); frame();
    CHECK(held() == PS_START, "keyboard still drives port 1 (Return -> Start)");
    frame();

    printf("-- 5b. silent physical pad + Steam virtual pad producing input (Steam Input on, pad not readable)\n");
    xbox = attach(0x045e, 0x028e, "Xbox 360 Controller", "xbox3");
    steam2 = attach(0x28de, 0x11ff, "Microsoft X-Box 360 pad 0", "steam-virtual2");
    frame();
    CHECK(PsyX_Pad_HostActiveInstance() == xbox.id, "both silent: physical pad preferred");
    CHECK(tap(&steam2, SDL_CONTROLLER_BUTTON_A) == PS_CROSS && PsyX_Pad_HostActiveInstance() == steam2.id,
          "only the Steam pad produces input -> it becomes port 1 (A -> Cross)");
    detach(&xbox);
    detach(&steam2);
    frame();

    printf("-- idle hand-over between two physical pads\n");
    padA = attach(0x045e, 0x028e, "Xbox 360 Controller", "padA");
    padB = attach(0x054c, 0x09cc, "PS4 Controller", "padB");
    frame();
    CHECK(PsyX_Pad_HostActiveInstance() == padA.id, "first-connected pad is port 1");
    tap(&padA, SDL_CONTROLLER_BUTTON_A);
    CHECK(tap(&padB, SDL_CONTROLLER_BUTTON_A) == 0 && PsyX_Pad_HostActiveInstance() == padA.id,
          "pad B pressed right after pad A: ignored (pad A not idle)");
    SDL_Delay(600);
    CHECK(tap(&padB, SDL_CONTROLLER_BUTTON_B) == PS_CIRCLE && PsyX_Pad_HostActiveInstance() == padB.id,
          "pad B pressed after pad A idled 600 ms: pad B takes port 1 (B -> Circle)");
    detach(&padA);
    detach(&padB);
    frame();

    SDL_Quit();
    printf("pad_select_test: %s (%d failure%s)\n", s_failures ? "FAIL" : "PASS", s_failures,
           s_failures == 1 ? "" : "s");
    return s_failures ? 1 : 0;
}
