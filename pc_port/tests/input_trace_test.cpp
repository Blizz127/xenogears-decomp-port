/* The input trace (src/psycross_input_trace.inl) never throttles button
 * events: every key, mouse, game-controller and joystick button / hat down
 * and up is logged, even after the burst budget is long gone.  Only axis
 * motion (and the wheel) is rate limited. */
#include <SDL.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

static unsigned s_buttonLines, s_axisLines, s_otherLines;
static void eprintinfo(const char* fmt, ...)
{
    char buf[512];
    va_list ap; va_start(ap, fmt); vsnprintf(buf, sizeof buf, fmt, ap); va_end(ap);
    if (strstr(buf, "button") || strstr(buf, " key ") || strstr(buf, "hat")) s_buttonLines++;
    else if (strstr(buf, "axis")) s_axisLines++;
    else s_otherLines++;
}
#include "../src/psycross_input_trace.inl"

static int s_fail;
#define CHECK(c, ...) do { if (c) printf("ok    "); else { printf("FAIL  "); s_fail++; } printf(__VA_ARGS__); printf("\n"); } while (0)

int main(void)
{
    SDL_Event e;
    const int N = 1000;
    SDL_setenv("SDL_VIDEODRIVER", "dummy", 1);
    SDL_Init(SDL_INIT_JOYSTICK | SDL_INIT_EVENTS);
    for (int i = 0; i < N; i++) {
        memset(&e, 0, sizeof e);
        e.type = SDL_CONTROLLERBUTTONDOWN; e.cbutton.which = 1; e.cbutton.button = (Uint8)(i % 15); PsyX_InputTrace(&e);
        e.type = SDL_CONTROLLERBUTTONUP; PsyX_InputTrace(&e);
        memset(&e, 0, sizeof e);
        e.type = SDL_JOYBUTTONDOWN; e.jbutton.which = 2; e.jbutton.button = (Uint8)(i % 20); PsyX_InputTrace(&e);
        e.type = SDL_JOYBUTTONUP; PsyX_InputTrace(&e);
        memset(&e, 0, sizeof e);
        e.type = SDL_KEYDOWN; e.key.keysym.scancode = SDL_SCANCODE_A; PsyX_InputTrace(&e);
        e.type = SDL_KEYUP; PsyX_InputTrace(&e);
        memset(&e, 0, sizeof e);
        e.type = SDL_JOYHATMOTION; e.jhat.which = 2; e.jhat.value = (Uint8)(i & 1); PsyX_InputTrace(&e);
    }
    CHECK(s_buttonLines == (unsigned)N * 7, "all %u button/key/hat events logged (got %u)", N * 7, s_buttonLines);
    for (int i = 0; i < N; i++) {
        memset(&e, 0, sizeof e);
        e.type = SDL_CONTROLLERAXISMOTION; e.caxis.which = 1; e.caxis.axis = SDL_CONTROLLER_AXIS_LEFTX;
        e.caxis.value = (i & 1) ? 30000 : -30000; PsyX_InputTrace(&e);
    }
    CHECK(s_axisLines < 250 && s_axisLines > 0, "axis motion stays rate limited (%u of %d logged)", s_axisLines, N);
    printf("input_trace_test: %s\n", s_fail ? "FAIL" : "PASS");
    return s_fail ? 1 : 0;
}
