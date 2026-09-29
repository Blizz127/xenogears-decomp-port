/* Test plugin for run_xg_plat_mods_test.sh: subscribes one frame_tick hook. */
#include "../include/xg_plat/mods.h"
static void tick(const XgPlatModEventData* e, void* u) { (void)e; (void)u; }
int xg_mod_init(const XgPlatModApi* api)
{
    if (api->abi != XG_PLAT_MODS_ABI_VERSION)
        return 1;
    api->log("test plugin loaded");
    return api->subscribe(XG_PLAT_EVENT_FRAME_TICK, tick, 0) > 0 ? 0 : 1;
}
