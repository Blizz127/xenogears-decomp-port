/* Retail mode-17 camera/position controller and direct motion helpers. */
#ifndef PC_PORT_WORLD_MAP_CALLBACK_827EC_H
#define PC_PORT_WORLD_MAP_CALLBACK_827EC_H

#include "common.h"

s32 wm_800827EC(s32 slot_index);
s32 wm_800828DC(s32 slot_index);

void wm_80076DA4(u32 slot, u32 work);
void wm_80076F54(u32 slot, u32 work);
void wm_80076FA8(u32 slot, u32 work);
s32 wm_800771D8(s32 current, s32 target, s32 step);

#endif
