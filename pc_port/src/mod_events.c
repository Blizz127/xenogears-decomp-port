/* mod_events.c -- game-side sources of xg_plat mod events.
 *
 * The mods layer (src/plat/xg_plat_mods.c) knows nothing about game state;
 * this glue watches the game on the frame_tick hook and turns state changes
 * into named events:
 *   room_enter  when the field map number (g_GameSceneMapNum, set by
 *               FieldMain on entry and by the map-change opcode) changes to a
 *               valid map; arg0 = map number & 0xFFFF, arg1 = raw value.
 * Read-only: no game state is written.
 */
#include <stdint.h>

#include "../include/xg_plat/mods.h"

extern int32_t g_GameSceneMapNum;

static int32_t s_last_room = -1;

static void watch_room(const XgPlatModEventData* event, void* user)
{
    int32_t room = g_GameSceneMapNum;
    (void)event;
    (void)user;
    if (room >= 0 && room != s_last_room) {
        s_last_room = room;
        xg_plat_mods_emit(XG_PLAT_EVENT_ROOM_ENTER, (uint32_t)room & 0xFFFFu, (uint32_t)room);
    } else if (room < 0) {
        s_last_room = -1;
    }
}

void PcPort_ModEventsInit(void)
{
    static int done;
    if (!done) {
        done = 1;
        xg_plat_mods_subscribe(XG_PLAT_EVENT_FRAME_TICK, watch_room, 0);
    }
}
