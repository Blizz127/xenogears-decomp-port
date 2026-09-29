/* world_map_port_data.h -- port view of the world-map overlay's data for the
 * matched C in src/world_map/main.c (included there under XENO_PC_PORT).
 *
 * On the port the world-map overlay lives in emulated PSX RAM: world_map_init.c
 * copies the user's world_map.bin to its retail address, and the port's
 * world-map code keeps all overlay state at the retail guest addresses.  The
 * matched bodies name that state through retail data symbols (D_8009xxxx), so
 * each symbol a switched body uses is defined here as its guest location.  The
 * bodies themselves stay byte-identical to the retail-matching source.
 *
 * Guest pointers (words holding a PSX address) are turned into host addresses
 * with WM_GUEST_PTR.  The value is an s32 like the retail declaration: the
 * port is linked non-PIE with all statics (g_PsxRam included) below 2 GiB, so
 * the host address survives the 32-bit round trip.  The object pool pointer
 * D_8009BE24 is the exception: retail declares it WmObj * (world_map/wm_obj.h),
 * so here it is a real host pointer and no s32 round trip applies to it.
 *
 * Grow the list per switch batch (docs/ARCHITECTURE-PORT.md).  Everything here
 * is port-only; the retail build never sees this header. */
#ifndef WORLD_MAP_PORT_DATA_H
#define WORLD_MAP_PORT_DATA_H

#include <stdint.h>
#include "psx_memory.h"
#include "world_map/wm_obj.h"

#define WM_GUEST(type, addr) (*(type*)PSX_ADDR(addr))
#define WM_GUEST_PTR(addr) ((s32)(intptr_t)PSX_ADDR(WM_GUEST(u32, addr)))

/* batch 1: func_80071A50, func_800834D0, func_80080900, func_80087710,
 * func_80078E2C */
#define D_8009BE24 ((WmObj*)PSX_ADDR(WM_GUEST(u32, 0x8009BE24))) /* actor slot pool (0x80-byte slots) */
#define D_8009C5AC WM_GUEST(s32, 0x8009C5AC)
#define D_8009C5B0 WM_GUEST(s32, 0x8009C5B0)
#define D_8009C5B4 WM_GUEST(s32, 0x8009C5B4)
#define D_8009BE0C WM_GUEST(s32, 0x8009BE0C)
#define D_8009D3F0 WM_GUEST(s32, 0x8009D3F0)
#define D_8009D144 WM_GUEST(s32, 0x8009D144)
#define D_8009BD38 WM_GUEST(s16, 0x8009BD38)
#define D_8009BD3A WM_GUEST(u16, 0x8009BD3A)
#define D_8009BD3C WM_GUEST(s16, 0x8009BD3C)

/* batch 2: story-selector callbacks (func_800877E0, 80087804, 800879A8,
 * 80087A8C, 80088D64..80088F5C) and their callees func_800848B4 and
 * func_800879E0 */
#define D_8009C610 WM_GUEST(s32, 0x8009C610) /* story-progress bracket */
#define D_8009C620 ((u8*)PSX_ADDR(WM_GUEST(u32, 0x8009C620))) /* 0x54-byte record array */
#define D_8009B624 ((u16*)PSX_ADDR(0x8009B624))
#define D_8009B626 ((u16*)PSX_ADDR(0x8009B626))
#define D_8009B64C ((u16*)PSX_ADDR(0x8009B64C))
#define D_8009B64E ((u16*)PSX_ADDR(0x8009B64E))
#define D_8009B69C ((u16*)PSX_ADDR(0x8009B69C))
#define D_8009B6B0 ((u16*)PSX_ADDR(0x8009B6B0))

/* batch 3: story-selector callbacks func_80087C6C, 80087FD0, 80088B40,
 * 80088D00 and their matched callees func_80087B84, 80087F60, 800894C8,
 * 80093354, 80093534, 80094154 */
#define D_8009A180 WM_GUEST(WmMat, 0x8009A180)
#define D_8009AF80 ((u16*)PSX_ADDR(0x8009AF80))
#define D_8009AF90 ((u16*)PSX_ADDR(0x8009AF90))
#define D_8009AFDC ((s16*)PSX_ADDR(0x8009AFDC))
#define D_8009B674 ((u16*)PSX_ADDR(0x8009B674))
#define D_8009CD68 ((WmSVec*)PSX_ADDR(0x8009CD68))
#define D_8009CD6C ((WmSVec*)PSX_ADDR(0x8009CD6C))
#define D_8009CD70 ((s16*)PSX_ADDR(0x8009CD70))
#define D_8009BCC0 ((u8*)PSX_ADDR(WM_GUEST(u32, 0x8009BCC0))) /* guest pointer */
#define D_8009D160 WM_GUEST(s32, 0x8009D160)
#define D_8009D2B4 WM_GUEST(s32, 0x8009D2B4)
/* g_GameState is the port's host copy (authoritative; world_map_gamestate.h):
 * the bodies address it as (u8 *)&g_GameState + offset. */
extern u8 g_GameState[];

/* batch 5: func_8007A570, 800794D8, 80079538, 8007BB60, 80091430 */
#define D_8009BE10 WM_GUEST(s32, 0x8009BE10) /* world-map mode word */
#define D_8009D52C WM_GUEST(u16, 0x8009D52C) /* published heading */
#define D_8009D55C_vec WM_GUEST(WmVec, 0x8009D55C) /* published position */
#define D_8009BD3A_arr ((s16*)PSX_ADDR(0x8009BD3A)) /* array view of D_8009BD3A */

/* batch 6: world-map event callbacks for the selector cleanup/init states */
#define D_8009A450 WM_GUEST(u16, 0x8009A450)
#define D_8009A46C ((u16*)PSX_ADDR(0x8009A46C))
#define D_8009A698 WM_GUEST(u16, 0x8009A698)
#define D_8009A6AC ((u16*)PSX_ADDR(0x8009A6AC))

#endif /* WORLD_MAP_PORT_DATA_H */
