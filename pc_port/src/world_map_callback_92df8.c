/*
 * World-map scheduler callback 0x80092DF8.
 *
 * Retail constructs a pair of 144x68 semi-transparent POLY_FT4 packets and
 * prepares its text-window object. It deliberately read-modify-writes the
 * predecessor object's flags at 0x8009D4A8; that asymmetric address and the
 * load/store ordering are observable retail behavior. No primitive is linked
 * into an ordering table here. The scheduler-supplied slot index is unused.
 */
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_callback_92df8.h"

#define WM_92DF8_WINDOW       0x8009BD64u
#define WM_92DF8_SELECTION    0x8009CE68u
#define WM_92DF8_WINDOW_STATE 0x8009BDCCu
#define WM_92DF8_PRIOR_FLAGS  0x8009D4A8u
#define WM_92DF8_PRIM_A       0x8009D2B8u
#define WM_92DF8_PRIM_B       0x8009D2E0u

/* Retail supplies seven meaningful constructor values. The accepted host
 * ABI retains one dead placeholder so retail's seventh value maps to height. */
void func_80032F54(void* object, s32 tpage_x, s32 tpage_y,
                   s32 x, s32 y, s32 width, s32 host_dead_mode,
                   s32 height);
void func_80034614(void* object);
u16 GetTPage(int tp, int abr, int x, int y);
u16 GetClut(int x, int y);
void SetSemiTrans(void* primitive, int enabled);

/* Opaque wrappers preserve the retail access widths and sequence at O2 while
 * avoiding host alignment and strict-aliasing assumptions. */
static u16 wm_92df8_load_u16(u32 pc, u32 address)
    __attribute__((noinline));
static u16 wm_92df8_load_u16(u32 pc, u32 address)
{
    u16 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
#if defined(WM_92DF8_TEST_TRACE)
    wm_92df8_test_trace(pc, WM_92DF8_TRACE_LHU, address, 2u, (u32)value);
#else
    (void)pc;
#endif
    return value;
}

static u32 wm_92df8_load_u32(u32 pc, u32 address)
    __attribute__((noinline));
static u32 wm_92df8_load_u32(u32 pc, u32 address)
{
    u32 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
#if defined(WM_92DF8_TEST_TRACE)
    wm_92df8_test_trace(pc, WM_92DF8_TRACE_LW, address, 4u, value);
#else
    (void)pc;
#endif
    return value;
}

static void wm_92df8_store_u8(u32 pc, u32 address, u8 value)
    __attribute__((noinline));
static void wm_92df8_store_u8(u32 pc, u32 address, u8 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
#if defined(WM_92DF8_TEST_TRACE)
    wm_92df8_test_trace(pc, WM_92DF8_TRACE_SB, address, 1u, (u32)value);
#else
    (void)pc;
#endif
}

static void wm_92df8_store_u16(u32 pc, u32 address, u16 value)
    __attribute__((noinline));
static void wm_92df8_store_u16(u32 pc, u32 address, u16 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
#if defined(WM_92DF8_TEST_TRACE)
    wm_92df8_test_trace(pc, WM_92DF8_TRACE_SH, address, 2u, (u32)value);
#else
    (void)pc;
#endif
}

static void wm_92df8_store_u32(u32 pc, u32 address, u32 value)
    __attribute__((noinline));
static void wm_92df8_store_u32(u32 pc, u32 address, u32 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
#if defined(WM_92DF8_TEST_TRACE)
    wm_92df8_test_trace(pc, WM_92DF8_TRACE_SW, address, 4u, value);
#else
    (void)pc;
#endif
}

s32 wm_80092DF8(s32 slot_index)
{
    void* const window = PSX_ADDR(WM_92DF8_WINDOW);
    u16 flags;
    u16 tpage;
    u16 clut;
    u32 source;
    u32 destination;
    u32 offset;
    u32 value0;
    u32 value1;
    u32 value2;
    u32 value3;

    (void)slot_index;

    wm_92df8_store_u16(0x80092E20u, WM_92DF8_SELECTION, 0xFFFFu);
    func_80032F54(window, 960, 397, 160, 120, 32, 0, 4);

    /* Preserve the retail clone artifact and its exact LHU -> SB -> SH order. */
    flags = wm_92df8_load_u16(0x80092E50u, WM_92DF8_PRIOR_FLAGS);
    wm_92df8_store_u8(0x80092E5Cu, WM_92DF8_WINDOW_STATE, 8u);
    wm_92df8_store_u16(0x80092E68u, WM_92DF8_PRIOR_FLAGS,
                       (u16)(flags | 2u));
    func_80034614(window);

    wm_92df8_store_u8(0x80092E80u, WM_92DF8_PRIM_A + 0x03u, 9u);
    wm_92df8_store_u8(0x80092E90u, WM_92DF8_PRIM_A + 0x07u, 0x2Cu);
    wm_92df8_store_u16(0x80092EA0u, WM_92DF8_PRIM_A + 0x0Au, 112u);
    wm_92df8_store_u16(0x80092EA8u, WM_92DF8_PRIM_A + 0x12u, 112u);
    wm_92df8_store_u16(0x80092EB4u, WM_92DF8_PRIM_A + 0x10u, 296u);
    wm_92df8_store_u16(0x80092EBCu, WM_92DF8_PRIM_A + 0x20u, 296u);
    wm_92df8_store_u16(0x80092EC8u, WM_92DF8_PRIM_A + 0x08u, 152u);
    wm_92df8_store_u16(0x80092ED0u, WM_92DF8_PRIM_A + 0x18u, 152u);
    wm_92df8_store_u16(0x80092EDCu, WM_92DF8_PRIM_A + 0x1Au, 180u);
    wm_92df8_store_u16(0x80092EE4u, WM_92DF8_PRIM_A + 0x22u, 180u);
    wm_92df8_store_u8(0x80092EF0u, WM_92DF8_PRIM_A + 0x0Cu, 0x80u);
    wm_92df8_store_u8(0x80092EF8u, WM_92DF8_PRIM_A + 0x0Du, 0u);
    wm_92df8_store_u8(0x80092F00u, WM_92DF8_PRIM_A + 0x14u, 0xFFu);
    wm_92df8_store_u8(0x80092F08u, WM_92DF8_PRIM_A + 0x15u, 0u);
    wm_92df8_store_u8(0x80092F10u, WM_92DF8_PRIM_A + 0x1Cu, 0x80u);
    wm_92df8_store_u8(0x80092F18u, WM_92DF8_PRIM_A + 0x1Du, 0x3Fu);
    wm_92df8_store_u8(0x80092F20u, WM_92DF8_PRIM_A + 0x24u, 0xFFu);
    wm_92df8_store_u8(0x80092F28u, WM_92DF8_PRIM_A + 0x25u, 0x3Fu);
    wm_92df8_store_u8(0x80092F30u, WM_92DF8_PRIM_A + 0x04u, 0x80u);
    wm_92df8_store_u8(0x80092F38u, WM_92DF8_PRIM_A + 0x05u, 0x80u);
    wm_92df8_store_u8(0x80092F40u, WM_92DF8_PRIM_A + 0x06u, 0x80u);

    tpage = GetTPage(0, 0, 896, 256);
    wm_92df8_store_u16(0x80092F54u, WM_92DF8_PRIM_A + 0x16u, tpage);
    clut = GetClut(304, 484);
    wm_92df8_store_u16(0x80092F70u, WM_92DF8_PRIM_A + 0x0Eu, clut);
    SetSemiTrans(PSX_ADDR(WM_92DF8_PRIM_A), 1);

    source = WM_92DF8_PRIM_A;
    destination = WM_92DF8_PRIM_B;
    for (offset = 0u; offset != 0x20u; offset += 0x10u) {
        value0 = wm_92df8_load_u32(0x80092F84u, source + offset);
        value1 = wm_92df8_load_u32(0x80092F88u, source + offset + 4u);
        value2 = wm_92df8_load_u32(0x80092F8Cu, source + offset + 8u);
        value3 = wm_92df8_load_u32(0x80092F90u, source + offset + 12u);
        wm_92df8_store_u32(0x80092F94u, destination + offset, value0);
        wm_92df8_store_u32(0x80092F98u, destination + offset + 4u, value1);
        wm_92df8_store_u32(0x80092F9Cu, destination + offset + 8u, value2);
        wm_92df8_store_u32(0x80092FA0u, destination + offset + 12u, value3);
    }
    value0 = wm_92df8_load_u32(0x80092FB0u, source + 0x20u);
    value1 = wm_92df8_load_u32(0x80092FB4u, source + 0x24u);
    wm_92df8_store_u32(0x80092FB8u, destination + 0x20u, value0);
    wm_92df8_store_u32(0x80092FBCu, destination + 0x24u, value1);

    return 1;
}
