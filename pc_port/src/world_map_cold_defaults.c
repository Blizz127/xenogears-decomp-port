/* Retail WorldMapMain cold-entry defaults [0x80070D58, 0x80070F38). */
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_cold_defaults.h"

#define WM_CD_GAMESTATE_BASE UINT32_C(0x8006D634)
#define WM_CD_D160           UINT32_C(0x8009D160)
#define WM_CD_TABLE_X        UINT32_C(0x8009AF80)
#define WM_CD_TABLE_Z        UINT32_C(0x8009AF90)

static u32 wm_cd_host_offset(u32 guest_address)
{
    return guest_address - WM_CD_GAMESTATE_BASE;
}

static void wm_cd_sb(u8* host_gamestate, u32 guest_address, u8 value)
{
#if defined(W34N118_MUTANT_SKIP_HOST_MIRROR)
    (void)host_gamestate;
#else
    host_gamestate[wm_cd_host_offset(guest_address)] = value;
#endif
    *(u8*)PSX_ADDR(guest_address) = value;
}

static void wm_cd_sh(u8* host_gamestate, u32 guest_address, u16 value)
{
#if defined(W34N118_MUTANT_SKIP_HOST_MIRROR)
    (void)host_gamestate;
#else
    memcpy(host_gamestate + wm_cd_host_offset(guest_address), &value,
           sizeof(value));
#endif
    memcpy(PSX_ADDR(guest_address), &value, sizeof(value));
}

static u16 wm_cd_host_lhu(const u8* host_gamestate, u32 guest_address)
{
    u16 value;
    memcpy(&value, host_gamestate + wm_cd_host_offset(guest_address),
           sizeof(value));
    return value;
}

static u16 wm_cd_guest_lhu(u32 guest_address)
{
    u16 value;
    memcpy(&value, PSX_ADDR(guest_address), sizeof(value));
    return value;
}

#if !defined(W34N118_MUTANT_DROP_D160)
static void wm_cd_guest_sw(u32 guest_address, u32 value)
{
    memcpy(PSX_ADDR(guest_address), &value, sizeof(value));
}
#endif

int wm_80070D58_cold_defaults(u8* host_gamestate)
{
    u16 table_index;
    u32 table_offset;

    if (host_gamestate == NULL)
        return -1;

    /* GameState tuple consumed by the shared path at 0x80070F90. */
    wm_cd_sh(host_gamestate, UINT32_C(0x8006F952), UINT16_C(0x0FFF));
    wm_cd_sh(host_gamestate, UINT32_C(0x8006F950), UINT16_C(0x0C00));
    wm_cd_sh(host_gamestate, UINT32_C(0x8006F94E), UINT16_C(0x0400));
#if !defined(W34N118_MUTANT_KEEP_ZERO_ENTRANCE)
    wm_cd_sh(host_gamestate, UINT32_C(0x8006F954), UINT16_C(1));
#endif

    wm_cd_sh(host_gamestate, UINT32_C(0x8006EE68), UINT16_C(0x4003));
    wm_cd_sh(host_gamestate, UINT32_C(0x8006EE60), UINT16_C(0x6680));
    wm_cd_sh(host_gamestate, UINT32_C(0x8006EE62), UINT16_C(0xFF00));
    wm_cd_sh(host_gamestate, UINT32_C(0x8006EE64), UINT16_C(0x2A00));
    wm_cd_sh(host_gamestate, UINT32_C(0x8006EE56), UINT16_C(0x2C00));

    wm_cd_sb(host_gamestate, UINT32_C(0x8006F369), UINT8_C(10));
    wm_cd_sb(host_gamestate, UINT32_C(0x8006D940), UINT8_C(15));
    wm_cd_sb(host_gamestate, UINT32_C(0x8006D9E4), UINT8_C(2));
    wm_cd_sb(host_gamestate, UINT32_C(0x8006DB2C), UINT8_C(4));
#if defined(W34N118_MUTANT_BAD_CHANNEL2)
    wm_cd_sb(host_gamestate, UINT32_C(0x8006F36A), UINT8_C(0));
#else
    wm_cd_sb(host_gamestate, UINT32_C(0x8006F36A), UINT8_C(5));
#endif
    wm_cd_sb(host_gamestate, UINT32_C(0x8006DBD0), UINT8_C(5));
    wm_cd_sb(host_gamestate, UINT32_C(0x8006DC74), UINT8_C(6));
    wm_cd_sb(host_gamestate, UINT32_C(0x8006DDBC), UINT8_C(7));
    wm_cd_sb(host_gamestate, UINT32_C(0x8006DE60), UINT8_C(8));
    wm_cd_sb(host_gamestate, UINT32_C(0x8006DA88), UINT8_C(3));
    wm_cd_sb(host_gamestate, UINT32_C(0x8006DF04), UINT8_C(3));

    wm_cd_sh(host_gamestate, UINT32_C(0x8006EE6A), UINT16_C(1));
    wm_cd_sh(host_gamestate, UINT32_C(0x8006EE66), UINT16_C(0));
    wm_cd_sh(host_gamestate, UINT32_C(0x8006EE54), UINT16_C(0x7580));
    wm_cd_sh(host_gamestate, UINT32_C(0x8006EE58), UINT16_C(0));
    wm_cd_sb(host_gamestate, UINT32_C(0x8006F368), UINT8_C(0));
    wm_cd_sb(host_gamestate, UINT32_C(0x8006F8E5), UINT8_C(0));
    wm_cd_sb(host_gamestate, UINT32_C(0x8006F8E6), UINT8_C(0));
    wm_cd_sb(host_gamestate, UINT32_C(0x8006F8E7), UINT8_C(0));
    wm_cd_sb(host_gamestate, UINT32_C(0x8006DD18), UINT8_C(9));
    wm_cd_sb(host_gamestate, UINT32_C(0x8006DFA8), UINT8_C(9));

    wm_cd_sh(host_gamestate, UINT32_C(0x8006EF8E), UINT16_C(0x0400));
    wm_cd_sh(host_gamestate, UINT32_C(0x8006EF90), UINT16_C(0x7500));
    wm_cd_sh(host_gamestate, UINT32_C(0x8006EF92), UINT16_C(0x2E58));
    wm_cd_sh(host_gamestate, UINT32_C(0x8006EF94), UINT16_C(0x0400));
    wm_cd_sh(host_gamestate, UINT32_C(0x8006EF96), UINT16_C(0x7580));
    wm_cd_sh(host_gamestate, UINT32_C(0x8006EF98), UINT16_C(0x2E58));
    wm_cd_sh(host_gamestate, UINT32_C(0x8006EF9A), UINT16_C(0x0400));
    wm_cd_sh(host_gamestate, UINT32_C(0x8006EF9C), UINT16_C(0x7600));
    wm_cd_sh(host_gamestate, UINT32_C(0x8006EF9E), UINT16_C(0x2E58));
    wm_cd_sh(host_gamestate, UINT32_C(0x8006EE7C), UINT16_C(1));

    table_index = wm_cd_host_lhu(host_gamestate,
                                 UINT32_C(0x8006EE7A));
#if defined(W34N118_MUTANT_WRONG_TABLE_INDEX)
    table_index = (u16)(table_index + UINT16_C(1));
#endif
    table_offset = (u32)table_index * UINT32_C(2);
    wm_cd_sh(host_gamestate, UINT32_C(0x8006EE78),
             wm_cd_guest_lhu(WM_CD_TABLE_X + table_offset));
#if !defined(W34N118_MUTANT_DROP_D160)
    wm_cd_guest_sw(WM_CD_D160, UINT32_C(0x07FFFFFF));
#endif
    wm_cd_sh(host_gamestate, UINT32_C(0x8006EE7A),
             wm_cd_guest_lhu(WM_CD_TABLE_Z + table_offset));

    return 0;
}
