/* Cheat console (pc_port/src/cheat_console.c) on the xg_plat hook layer,
 * with the cheat backends stubbed: command parsing, queue -> frame_tick
 * execution, and that the per-frame cheat services keep their order. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/cheat_console.h"
#include "../include/xg_plat/mods.h"
#include "../include/xg_plat/audio.h"
#include "../include/xg_plat/input.h"
#include "../include/xg_plat/timing.h"

static char s_trace[256];
static void trace(const char* s) { strncat(s_trace, s, sizeof s_trace - strlen(s_trace) - 1); }
static int s_god, s_enc = 1, s_speed;
int PcPort_GodModeEnabled(void) { return s_god; }
void PcPort_GodModeToggle(void) { s_god = !s_god; trace("G"); }
int PcPort_RandomBattlesEnabled(void) { return s_enc; }
void PcPort_RandomBattlesToggle(void) { s_enc = !s_enc; trace("E"); }
void PcPort_QuickCheckpointRequestSave(void) { trace("S"); }
void PcPort_QuickCheckpointRequestLoad(void) { trace("L"); }
void PcPort_FieldWarpDiag(void) { trace("w"); }
void PcPort_FieldWarpArm(int m, int x, int z) { char b[32]; snprintf(b, sizeof b, "W%d,%d,%d", m, x, z); trace(b); }
void PcPort_DebugBattleWarp(void) { trace("b"); }
void PcPort_DebugBattleWarpQueue(int id) { char b[16]; snprintf(b, sizeof b, "B%d", id); trace(b); }
void xg_plat_timing_set_speed(int m) { s_speed = m; }
void xg_plat_timing_set_fast_forward_speed(int m) { (void)m; }
int xg_plat_audio_set_user_volume(int p) { (void)p; return 0; }
static const char* const s_btn[XG_PLAT_BTN_COUNT] = {
    "square", "circle", "triangle", "cross", "l1", "l2", "r1", "r2",
    "start", "select", "left", "right", "up", "down",
};
static char s_bound[XG_PLAT_BTN_COUNT][16];
const char* xg_plat_input_button_name(XgPlatButton b) { return s_btn[b]; }
int xg_plat_input_bind_key(XgPlatButton b, const char* k)
{
    if (!strcmp(k, "NoSuchKey"))
        return 0;
    snprintf(s_bound[b], sizeof s_bound[b], "%s", k);
    return 1;
}
const char* xg_plat_input_key_name(XgPlatButton b) { return s_bound[b]; }

static int fails;
#define CHECK(c, what) do { if (!(c)) { fails++; printf("FAIL %s (trace '%s')\n", what, s_trace); } else printf("ok   %s\n", what); } while (0)

int main(void)
{
    PcPort_CheatConsoleInit();
    s_trace[0] = 0;
    CHECK(PcPort_CheatExec("god on") == 1 && s_god == 1, "god on");
    CHECK(PcPort_CheatExec("god on") == 1 && !strcmp(s_trace, "G"), "god on again is a no-op");
    CHECK(PcPort_CheatExec("god") == 1 && s_god == 0, "god toggles");
    CHECK(PcPort_CheatExec("encounters off") == 1 && s_enc == 0, "encounters off");
    s_trace[0] = 0;
    CHECK(PcPort_CheatExec("save") == 1 && PcPort_CheatExec("load") == 1 && !strcmp(s_trace, "SL"),
          "save / load requests");
    s_trace[0] = 0;
    CHECK(PcPort_CheatExec("warp 2 100 -50") == 1 && !strcmp(s_trace, "W2,100,-50"), "warp arms");
    CHECK(PcPort_CheatExec("warp 2") == -1, "malformed warp rejected");
    s_trace[0] = 0;
    CHECK(PcPort_CheatExec("battle 17") == 1 && !strcmp(s_trace, "B17"), "battle queues");
    CHECK(PcPort_CheatExec("speed 3") == 1 && s_speed == 3, "speed");
    CHECK(PcPort_CheatExec("bind cross Space") == 1 && !strcmp(s_bound[XG_PLAT_BTN_CROSS], "Space"),
          "bind rebinds a button");
    CHECK(PcPort_CheatExec("bind cross NoSuchKey") == -1 && PcPort_CheatExec("bind nobutton X") == -1,
          "bind rejects unknown keys and buttons");
    CHECK(PcPort_CheatExec("bind") == 1, "bind lists");
    CHECK(PcPort_CheatExec("nonsense") == -1 && PcPort_CheatExec("   ") == 0, "unknown / empty");
    /* queued commands run on frame_tick, after the per-frame services */
    s_trace[0] = 0;
    PcPort_CheatQueue("save");
    CHECK(!strcmp(s_trace, ""), "queued command does not run immediately");
    xg_plat_mods_emit(XG_PLAT_EVENT_FRAME_TICK, 0, 0);
    CHECK(!strcmp(s_trace, "wbS"), "frame_tick order: field warp, battle warp, queued commands");
    s_trace[0] = 0;
    xg_plat_mods_emit(XG_PLAT_EVENT_FRAME_TICK, 0, 0);
    CHECK(!strcmp(s_trace, "wb"), "queue drained");
    printf("cheat console test: %s\n", fails ? "FAIL" : "PASS");
    return fails != 0;
}
