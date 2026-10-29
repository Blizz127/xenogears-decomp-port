/* input.pad_confirm at run time (src/port_pad_confirm.c + xg_plat_config.c):
 * toggling swaps the physical Cross/Circle mapping at once, writes
 * `pad_confirm = ...` under [input] in the user's config file (creating it,
 * keeping every other line and comment), and the value survives a restart
 * (a fresh process reloads the file).  Driven by run_pad_confirm_test.sh:
 *   pad_confirm_test write     no file yet: toggle creates $XDG_CONFIG_HOME/xenogears-port/config.ini
 *   pad_confirm_test reload    new process: the saved choice is applied at start; toggle back
 *   pad_confirm_test preserve  XENO_CONFIG file with comments/keys kept; JP layout
 * The backend calls are stubbed; the mapping is a model of PsyCross's
 * gc_cross/gc_circle exchange (the real one is covered by pad_select_test). */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "../include/xg_plat/config.h"
#include "../include/xg_plat/audio.h"
#include "../include/xg_plat/input.h"
#include "../include/xg_plat/timing.h"
#include "../src/port_pad_confirm.h"

/* model of g_cfg_controllerMapping: SDL button (south = A, east = B) carrying each PS button */
static int gc_cross = 0 /* south */, gc_circle = 1 /* east */, s_swapped;
void xg_plat_input_set_pad_face_swap(int swapped)
{
    swapped = swapped ? 1 : 0;
    if (swapped != s_swapped) { int c = gc_cross; gc_cross = gc_circle; gc_circle = c; s_swapped = swapped; }
}
void xg_plat_timing_set_speed(int m) { (void)m; }
void xg_plat_timing_set_speed_from(int m, const char* s) { (void)m; (void)s; }
void xg_plat_timing_set_fast_forward_speed(int m) { (void)m; }
int xg_plat_audio_set_user_volume(int p) { (void)p; return 0; }
const char* xg_plat_input_button_name(XgPlatButton b) { (void)b; return "x"; }
int xg_plat_input_bind_key(XgPlatButton b, const char* k) { (void)b; (void)k; return 1; }

static int fails;
#define CHECK(c, what) do { if (!(c)) { fails++; printf("FAIL %s\n", what); } else printf("ok   %s\n", what); } while (0)

static char* slurp(const char* path)
{
    FILE* f = fopen(path, "rb");
    char* t; long n;
    if (!f) return NULL;
    fseek(f, 0, SEEK_END); n = ftell(f); fseek(f, 0, SEEK_SET);
    t = (char*)malloc((size_t)n + 1);
    n = (long)fread(t, 1, (size_t)n, f); t[n] = '\0'; fclose(f);
    return t;
}
static int south_is_cross(void) { return gc_cross == 0; }

int main(int argc, char** argv)
{
    const char* mode = argc > 1 ? argv[1] : "";
    char path[1100];
    const char* xdg = getenv("XDG_CONFIG_HOME");
    char* text;

    if (xdg && *xdg)
        snprintf(path, sizeof path, "%s/xenogears-port/config.ini", xdg);
    else
        snprintf(path, sizeof path, "%s/.config/xenogears-port/config.ini", getenv("HOME"));
    if (!strcmp(mode, "write")) {
        CHECK(access(path, 0) != 0, "no user config yet");
        xg_plat_config_load();
        PcPort_PadConfirmInit(0);
        CHECK(!PcPort_PadConfirmCircle() && south_is_cross(), "default: south button = Cross (US)");
        PcPort_PadConfirmToggle();
        CHECK(PcPort_PadConfirmCircle() && !south_is_cross() && gc_circle == 0,
              "toggle applies immediately: south button = Circle, east = Cross");
        text = slurp(path);
        CHECK(text && strcmp(text, "[input]\npad_confirm = circle\n") == 0, "config file created with [input] pad_confirm = circle");
        if (text) printf("---- %s\n%s----\n", path, text);
        free(text);
        CHECK(!strcmp(xg_plat_config_get("input.pad_confirm"), "circle"), "in-memory value updated");
    } else if (!strcmp(mode, "reload")) {
        int n = xg_plat_config_load();
        CHECK(n == 1 && xg_plat_config_path() && !strcmp(xg_plat_config_path(), path), "restart: config reloaded from the saved file");
        PcPort_PadConfirmInit(0);
        CHECK(PcPort_PadConfirmCircle() && !south_is_cross(), "restart: Circle confirm restored from config");
        PcPort_PadConfirmToggle();
        CHECK(!PcPort_PadConfirmCircle() && south_is_cross(), "toggle back: Cross (US)");
        text = slurp(path);
        CHECK(text && strcmp(text, "[input]\npad_confirm = cross\n") == 0, "file updated in place (one line, no duplicates)");
        free(text);
    } else if (!strcmp(mode, "preserve")) {
        const char* file = getenv("XENO_CONFIG");
        const char* seed =
            "# my settings\n[video]\nwidth = 1280   # window\n\n[input]\n# pad_confirm = cross\ncircle = Z\n"
            "\n[cheats]\ndev_menu = on\n";
        const char* want1 =
            "# my settings\n[video]\nwidth = 1280   # window\n\n[input]\n# pad_confirm = cross\ncircle = Z\n"
            "pad_confirm = cross\n\n[cheats]\ndev_menu = on\n";
        const char* seed2 = "[input]\n  pad_confirm = cross   # retail\nsquare = V\n";
        const char* want2 = "[input]\n  pad_confirm = circle  # retail\nsquare = V\n";
        FILE* f;
        CHECK(file != NULL, "XENO_CONFIG set");
        f = fopen(file, "wb"); fputs(seed, f); fclose(f);
        xg_plat_config_load();
        PcPort_PadConfirmInit(1); /* JP layout: the retail confirm is Circle */
        CHECK(PcPort_PadConfirmCircle() == 1 && s_swapped == 0, "JP layout, no key: Circle confirms, no exchange");
        PcPort_PadConfirmToggle();
        CHECK(!PcPort_PadConfirmCircle() && s_swapped == 1, "toggle: Cross confirms (exchanged against the JP layout)");
        text = slurp(file);
        CHECK(text && strcmp(text, want1) == 0,
              "pad_confirm inserted after the last [input] key; comments, blanks, other sections byte-identical");
        if (text && strcmp(text, want1)) printf("---- got\n%s----\n", text);
        free(text);
        f = fopen(file, "wb"); fputs(seed2, f); fclose(f);
        PcPort_PadConfirmToggle();
        text = slurp(file);
        CHECK(text && strcmp(text, want2) == 0, "existing 'pad_confirm = ...  # comment' line rewritten in place, indent and comment kept");
        if (text && strcmp(text, want2)) printf("---- got\n%s----\n", text);
        free(text);
    }
    printf("pad_confirm_test %s: %s\n", mode, fails ? "FAIL" : "PASS");
    return fails ? 1 : 0;
}
