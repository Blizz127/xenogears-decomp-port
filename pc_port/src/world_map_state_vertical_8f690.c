/*
 * Retail vertical-transition states of wm_8008E76C (main_state at slot +0x20,
 * dispatched through jtbl_80070A50).
 *
 * Owned retail regions:
 *   state 8   [0x8008F690, 0x8008F6EC)  climb to 0xFFE20000, then state 2
 *                                      and the sound-bank switch
 *   state 12  [0x8008F6EC, 0x8008F72C)  settle down to the ground height
 *                                      (+0x68), then state 3
 *   state 16  [0x8008F72C, 0x8008F8D0)  settle down to the ground height,
 *                                      then state 1, the other sound bank
 *                                      and the party record links
 *   state 20  [0x8008F8D0, 0x8008F9D8)  climb to the ground height, then
 *                                      state 1 and the party record links
 *   copy arm  [0x800905E8, 0x80090620)  position -> D_8009D55C, heading ->
 *                                      D_8009D52C (states 8 and 12)
 * The shared tail [0x80090620, 0x800906B4) is world_map_vehicle_tail_90620.c.
 * Height is slot +0x2C in 20.12 fixed point; PSX y grows downward.
 */
#include <stdint.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_common_tail.h"
#include "world_map_gamestate.h"
#include "world_map_helper_93f18.h"
#include "world_map_helper_97770.h"
#include "world_map_state_vertical_8f690.h"
#include "world_map_vehicle_tail_90620.h"

#define WM_SV_SCRATCH      UINT32_C(0x1F800000)
#define WM_SV_CONTEXT_PTR  UINT32_C(0x8009C620)
#define WM_SV_POSE         UINT32_C(0x8009D55C) /* 4 words: x, y, z, pad */
#define WM_SV_POSE_Y       UINT32_C(0x8009D560)
#define WM_SV_HEADING      UINT32_C(0x8009D52C)
#define WM_SV_BANK_A_DATA  UINT32_C(0x8009C888)
#define WM_SV_BANK_A_ENTRY UINT32_C(0x8009D800)
#define WM_SV_BANK_B_DATA  UINT32_C(0x8009C884)
#define WM_SV_BANK_B_ENTRY UINT32_C(0x8009D3D0)
#define WM_SV_GS           UINT32_C(0x8006D634) /* g_GameState */

static u32 sv_lw(u32 address)
{
    u32 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static void sv_sw(u32 address, u32 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static s16 sv_lh(u32 address)
{
    s16 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static void sv_sh(u32 address, u16 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static s32 sv_s32(u32 bits)
{
    s32 value;
    memcpy(&value, &bits, sizeof(value));
    return value;
}

static u16 sv_sra12(u32 bits)
{
    return (u16)(sv_s32(bits) >> 12);
}

/* 0x800905E8: publish the slot position and heading, then the shared tail. */
static s32 sv_copy_and_tail(u32 slot)
{
    u32 i;

    for (i = 0u; i < 4u; i++)
        sv_sw(WM_SV_POSE + i * 4u, sv_lw(slot + 0x28u + i * 4u));
    sv_sh(WM_SV_HEADING, (u16)sv_lh(slot + 0x48u));
    wm_8008E76C_shared_tail(slot);
    return 1;
}

/* 0x8008F9C4: publish the height, then the shared tail. */
static s32 sv_height_and_tail(u32 slot)
{
#if !defined(W34N125_MUTANT_M3_SKIP_HEIGHT_PUBLISH)
    sv_sw(WM_SV_POSE_Y, sv_lw(slot + 0x2Cu));
#endif
    wm_8008E76C_shared_tail(slot);
    return 1;
}

/* 0x8008F7A4..0x8008F80C and 0x8008F918..0x8008F980: raise the party
 * member links; members 2 and 3 only when their g_GameState slot is used.
 * The two retail copies claim the pool slots in a different order. */
static void sv_party_links(int landing)
{
    wm_gs_sb(WM_SV_GS + 0x22B1u, 1u);
    if (wm_gs_lbu(WM_SV_GS + 0x1D35u) != 0xFFu) {
        (void)wm_80097770(landing ? 5u : 2u, 3);
        (void)wm_80097770(landing ? 2u : 5u, 3);
        wm_gs_sb(WM_SV_GS + 0x22B2u, 1u);
    }
    if (wm_gs_lbu(WM_SV_GS + 0x1D36u) != 0xFFu) {
        (void)wm_80097770(landing ? 6u : 3u, 3);
        (void)wm_80097770(landing ? 3u : 6u, 3);
        wm_gs_sb(WM_SV_GS + 0x22B3u, 1u);
    }
}

static void sv_stop(u32 slot)
{
    sv_sw(slot + 0x40u, 0u);
    sv_sw(slot + 0x3Cu, 0u);
    sv_sw(slot + 0x38u, 0u);
    sv_sw(slot + 0x74u, 0u);
    wm_gs_sh(WM_SV_GS + 0x1834u,
             (u16)(wm_gs_lhu(WM_SV_GS + 0x1834u) & 0x3FFFu));
}

s32 wm_8008E76C_state8(u32 slot)
{
    u32 y = sv_lw(slot + 0x2Cu) + UINT32_C(0xFFFF8000);

    sv_sw(slot + 0x2Cu, y);
    if (sv_s32(UINT32_C(0xFFE20000)) < sv_s32(y))
        return sv_copy_and_tail(slot);
    sv_sw(slot + 0x2Cu, UINT32_C(0xFFE20000));
    if (sv_lh(slot + 4u) != 0xB)
        return sv_copy_and_tail(slot);
    {
#if defined(W34N125_MUTANT_M1_WRONG_BANK)
        u32 data = sv_lw(WM_SV_BANK_B_DATA);
        u32 entry = sv_lw(WM_SV_BANK_B_ENTRY);
#else
        u32 data = sv_lw(WM_SV_BANK_A_DATA);
        u32 entry = sv_lw(WM_SV_BANK_A_ENTRY);
#endif

        sv_sw(slot + 0x64u, 0u);
        sv_sh(slot + 0x20u, 2u);
        sv_sh(slot + 4u, 0u);
        wm_800767D4(data, entry);
    }
    return sv_copy_and_tail(slot);
}

s32 wm_8008E76C_state12(u32 slot)
{
    u32 y = sv_lw(slot + 0x2Cu) + 0x800u;
    u32 ground = sv_lw(slot + 0x68u);
    u32 context;

    sv_sw(slot + 0x2Cu, y);
    if (sv_s32(y) < sv_s32(ground))
        return sv_copy_and_tail(slot);
    sv_sw(slot + 0x2Cu, ground);
    context = sv_lw(WM_SV_CONTEXT_PTR);
    sv_sh(slot + 0x20u, 3u);
    sv_sh(context + 0xFCu, 1u);
    sv_sh(context + 0xA8u, 1u);
    sv_sh(context + 0x54u, 1u);
    return sv_copy_and_tail(slot);
}

s32 wm_8008E76C_state16(u32 slot)
{
    u32 y = sv_lw(slot + 0x2Cu) + 0x8000u;
    u32 ground = sv_lw(slot + 0x68u);

    sv_sw(slot + 0x2Cu, y);
    if (sv_s32(y) < sv_s32(ground)) {
        /* 0x8008F850: still falling; once below 0xFFE00000 publish the
         * landing marker a single time (slot +0x7C). */
        if (sv_s32(UINT32_C(0xFFE00000)) < sv_s32(y) && sv_lw(slot + 0x7Cu) != 0u) {
            s32 region;

            sv_sw(slot + 0x7Cu, 0u);
            sv_sh(WM_SV_SCRATCH + 0xA0u, sv_sra12(sv_lw(slot + 0x28u)));
            sv_sh(WM_SV_SCRATCH + 0xA2u, sv_sra12(sv_lw(slot + 0x68u)));
            sv_sh(WM_SV_SCRATCH + 0xA4u, sv_sra12(sv_lw(slot + 0x30u)));
            region = (s32)(s16)(u16)wm_80093F18(slot + 0x28u);
            wm_80089160(region == 3 ? 0x3Eu : 0x3Du, WM_SV_SCRATCH + 0xA0u, 0u);
        }
        return sv_height_and_tail(slot);
    }
    {
        u32 data = sv_lw(WM_SV_BANK_B_DATA);
        u32 entry = sv_lw(WM_SV_BANK_B_ENTRY);

#if !defined(W34N125_MUTANT_M2_NO_GROUND_CLAMP)
        sv_sw(slot + 0x2Cu, ground);
#endif
        sv_sh(slot + 0x20u, 1u);
        wm_800767D4(data, entry);
    }
    (void)wm_80097770(8u, 10);
    (void)wm_80097770(9u, 10);
    (void)wm_80097770(10u, 10);
    (void)wm_80097770(4u, 3);
    (void)wm_80097770(1u, 3);
#if defined(W34N125_MUTANT_M4_PARTY_ORDER)
    sv_party_links(0);
#else
    sv_party_links(1);
#endif
    sv_stop(slot);
    wm_800894C8(0u);
    wm_800894C8(1u);
    wm_80075228();
    return sv_height_and_tail(slot);
}

s32 wm_8008E76C_state20(u32 slot)
{
    u32 y = sv_lw(slot + 0x2Cu) - 0x800u;
    u32 ground = sv_lw(slot + 0x68u);
    u32 context;

    sv_sw(slot + 0x2Cu, y);
    if (sv_s32(ground) < sv_s32(y))
        return sv_height_and_tail(slot);
    sv_sw(slot + 0x2Cu, ground);
    sv_sh(slot + 0x20u, 1u);
    (void)wm_80097770(8u, 10);
    (void)wm_80097770(1u, 3);
    (void)wm_80097770(4u, 3);
    sv_party_links(0);
    sv_stop(slot);
    context = sv_lw(WM_SV_CONTEXT_PTR);
    sv_sh(context + 0xFCu, 0u);
    sv_sh(context + 0xA8u, 0u);
    sv_sh(context + 0x54u, 0u);
    wm_80075228();
    return sv_height_and_tail(slot);
}

/* Retail 0x800767D4 (byte-exact C in src/world_map/main.c, which the port
 * cannot compile as is: its arguments are guest addresses).  Stop the
 * current sound set, copy the new bank to D_80062648 and start it. */
extern void func_80039CC4(void);
extern void func_800399D4(void *manager);
extern int ArchiveDecodeAlignedSize(unsigned int entry_index);
extern void *func_80039850(void *song_file);
extern void func_80039A80(void *manager, int level, int steps);
extern unsigned char D_80062648[];
extern void *D_80062528;

void wm_800767D4(u32 song_data, u32 archive_entry)
{
    void *manager;

    func_80039CC4();
    func_800399D4(D_80062528);
    memcpy(D_80062648, PSX_ADDR(song_data),
           (size_t)ArchiveDecodeAlignedSize(archive_entry));
    manager = func_80039850(D_80062648);
    D_80062528 = manager;
    func_80039A80(manager, 0x7F, 0);
}
