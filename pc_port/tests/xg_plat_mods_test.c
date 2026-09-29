/* xg_plat mods layer (src/plat/xg_plat_mods.c): manifests, load order,
 * event hooks, a C plugin, and hash-keyed asset replacement.  Synthetic
 * data only (no game files).  argv[1] = scratch mods root prepared by
 * run_xg_plat_mods_test.sh. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/xg_plat/mods.h"
#include "../include/xg_plat/audio.h"
#include "../include/xg_plat/input.h"
#include "../include/xg_plat/timing.h"

/* backend stubs needed by xg_plat_config.c */
void xg_plat_timing_set_speed(int m) { (void)m; }
void xg_plat_timing_set_fast_forward_speed(int m) { (void)m; }
int xg_plat_audio_set_user_volume(int p) { (void)p; return 0; }
const char* xg_plat_input_button_name(XgPlatButton b) { (void)b; return "x"; }
int xg_plat_input_bind_key(XgPlatButton b, const char* k) { (void)b; (void)k; return 1; }

static int fails;
#define CHECK(c, what) do { if (!(c)) { fails++; printf("FAIL %s\n", what); } else printf("ok   %s\n", what); } while (0)

static int s_ticks;
int32_t g_GameSceneMapNum = -1;       /* for mod_events.c */
void PcPort_ModEventsInit(void);
static int s_rooms[4], s_nrooms;
static void on_room(const XgPlatModEventData* e, void* user)
{
    (void)user;
    if (s_nrooms < 4)
        s_rooms[s_nrooms++] = (int)e->arg0;
}
static void on_tick(const XgPlatModEventData* e, void* user) { (void)e; (*(int*)user)++; }

int main(int argc, char** argv)
{
    XgPlatModManifest m;
    unsigned char buf[64];
    size_t n;
    int h;
    (void)argc;
    CHECK(xg_plat_mods_parse_manifest("name = A\nversion = 2\nload_order = 5\n", &m) &&
          m.load_order == 5 && !strcmp(m.version, "2"), "manifest parse");
    CHECK(xg_plat_mods_parse_manifest("name = S\nscript = main.lua\n", &m) &&
              !strcmp(m.script, "main.lua"), "reserved script key parsed (not run)");
    CHECK(!xg_plat_mods_parse_manifest("version = 1\n", &m), "manifest without name rejected");
    CHECK(!xg_plat_mods_parse_manifest("name = B\nabi = 99\n", &m), "incompatible abi rejected");

    setenv("XENO_MODS_DIR", argv[1], 1);
    setenv("XDG_DATA_HOME", "/nonexistent-xg-test", 1);
    CHECK(xg_plat_mods_init() == 3, "three valid mods loaded (bad manifest skipped)");
    CHECK(!strcmp(xg_plat_mods_get(0)->name, "First") && !strcmp(xg_plat_mods_get(2)->name, "Last"),
          "load order sorted");
    /* plugin in "Last" subscribed one frame_tick hook */
    h = xg_plat_mods_subscribe(XG_PLAT_EVENT_FRAME_TICK, on_tick, &s_ticks);
    CHECK(h > 0 && xg_plat_mods_emit(XG_PLAT_EVENT_FRAME_TICK, 0, 0) == 2 && s_ticks == 1,
          "plugin hook + local hook both run");
    CHECK(xg_plat_mods_unsubscribe(h) && xg_plat_mods_emit(XG_PLAT_EVENT_FRAME_TICK, 0, 0) == 1,
          "unsubscribe");

    memset(buf, 'A', 32);       /* "original" asset: 32 x 'A' */
    n = xg_plat_mods_filter_asset(buf, 32, sizeof buf);
    CHECK(n == 5 && !memcmp(buf, "HELLO", 5) && buf[5] == 0, "asset replaced by the later mod");
    memset(buf, 'B', 32);       /* replacement for 32 x 'B' is larger than 40 bytes */
    n = xg_plat_mods_filter_asset(buf, 32, 40);
    CHECK(n == 32 && buf[0] == 'B', "oversized replacement refused, original kept");
    memset(buf, 'C', 32);
    n = xg_plat_mods_filter_asset(buf, 32, sizeof buf);
    CHECK(n == 32 && buf[0] == 'C', "unknown asset untouched");
    /* room_enter from the frame_tick watcher (mod_events.c) */
    PcPort_ModEventsInit();
    xg_plat_mods_subscribe(XG_PLAT_EVENT_ROOM_ENTER, on_room, NULL);
    xg_plat_mods_emit(XG_PLAT_EVENT_FRAME_TICK, 0, 0);            /* -1: nothing */
    g_GameSceneMapNum = 490; xg_plat_mods_emit(XG_PLAT_EVENT_FRAME_TICK, 0, 0);
    xg_plat_mods_emit(XG_PLAT_EVENT_FRAME_TICK, 0, 0);            /* same map: nothing */
    g_GameSceneMapNum = 4;   xg_plat_mods_emit(XG_PLAT_EVENT_FRAME_TICK, 0, 0);
    CHECK(s_nrooms == 2 && s_rooms[0] == 490 && s_rooms[1] == 4, "room_enter on map changes only");
    CHECK(!strcmp(xg_plat_mods_event_name(XG_PLAT_EVENT_ROOM_ENTER), "room_enter"), "event name");
    printf("xg_plat mods test: %s\n", fails ? "FAIL" : "PASS");
    return fails != 0;
}
