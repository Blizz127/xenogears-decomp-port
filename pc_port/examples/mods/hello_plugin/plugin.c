/* Example xenogears-port plugin (docs/port/MODDING.md).
 *
 * Logs every field map the player enters and, once a minute of presented
 * frames, how many frames have been shown.  Code only: it ships no game data.
 *
 * Build from the repository root:
 *   cc -shared -fPIC -Ipc_port/include -o mods/hello_plugin/plugin.so \
 *      pc_port/examples/mods/hello_plugin/plugin.c
 * then copy mod.txt next to plugin.so (mods/hello_plugin/). */
#include "xg_plat/mods.h"

static const XgPlatModApi* s_api;
static unsigned long s_frames;

static void on_room_enter(const XgPlatModEventData* e, void* user)
{
    (void)user;
    s_api->log("hello_plugin: entered field map %u", (unsigned)e->arg0);
}

static void on_frame_tick(const XgPlatModEventData* e, void* user)
{
    (void)e;
    (void)user;
    if (++s_frames % 1800 == 0) /* 30 fps: once a minute */
        s_api->log("hello_plugin: %lu frames presented", s_frames);
}

int xg_mod_init(const XgPlatModApi* api)
{
    if (api->abi != XG_PLAT_MODS_ABI_VERSION)
        return -1; /* built against a different mod ABI: refuse to load */
    s_api = api;
    api->subscribe(XG_PLAT_EVENT_ROOM_ENTER, on_room_enter, 0);
    api->subscribe(XG_PLAT_EVENT_FRAME_TICK, on_frame_tick, 0);
    api->log("hello_plugin: loaded");
    return 0;
}
