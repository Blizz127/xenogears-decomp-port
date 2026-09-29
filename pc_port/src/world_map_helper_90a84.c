/*
 * World-map update helper 0x80090A84.
 *
 * Retail ABI: a0 is the world-map slot pointer.  The helper returns 0, 1,
 * or 3 and updates the slot's heading/trigonometric fields plus the small
 * global transition-state channel.  It is not a scheduler callback.
 */
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_helper_90a84.h"

extern int rcos(int angle);
extern int rsin(int angle);

#define WM_90A84_HEADING          0x8009CD4Cu
#define WM_90A84_ANGLE            0x8009BD3Au
#define WM_90A84_FLAGS            0x8009BD10u
#define WM_90A84_BYTE_STATE       0x8009D738u
#define WM_90A84_SELECTION        0x8009BD24u
#define WM_90A84_OBJECT           0x8009D7D8u
#define WM_90A84_SELECTION_ALT    0x8009CE68u
#define WM_90A84_STATE_PUBLISH    0x8009D804u
#define WM_90A84_EDGE_STATE       0x8009CEC0u
#define WM_90A84_PREVIOUS_STATE   0x8009C7E8u
#define WM_90A84_CHANGED_STATE    0x8009BD34u

#define WM_90A84_SLOT_ANGLE       0x48u
#define WM_90A84_SLOT_COS         0x38u
#define WM_90A84_SLOT_SIN         0x40u

#if defined(WM_90A84_TEST_TRACE)
#define WM_90A84_TRACE(kind, address, value) \
    wm_90a84_test_trace((kind), (address), (value))
#else
#define WM_90A84_TRACE(kind, address, value) ((void)0)
#endif

static u8 wm_90a84_load_u8(u32 address)
{
    u8 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static u16 wm_90a84_load_u16(u32 address)
{
    u16 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static u32 wm_90a84_load_u32(u32 address)
{
    u32 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static void wm_90a84_store_u16(u32 address, u16 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static void wm_90a84_store_u32(u32 address, u32 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static s16 wm_90a84_s16(u16 value)
{
    s16 result;
    memcpy(&result, &value, sizeof(result));
    return result;
}

static u32 wm_90a84_negate_bits(s32 value)
{
    u32 bits;

    memcpy(&bits, &value, sizeof(bits));
#if defined(WM_90A84_MUTANT_M5)
    return bits;
#else
    return 0u - bits;
#endif
}

/* Exact body of the direct retail helper called at 0x80090C48 and 0x80090DF4. */
void wm_80090A18(void)
{
    u16 heading = wm_90a84_load_u16(WM_90A84_HEADING);
    u32 current = ((heading & 1u) != 0u && (heading & 2u) != 0u)
                    ? 1u : 0u;
    u32 previous;

#if defined(WM_90A84_MUTANT_M13)
    previous = 0u;
#else
    previous = wm_90a84_load_u32(WM_90A84_PREVIOUS_STATE);
#endif
    u32 changed = (current ^ previous) & current;

    WM_90A84_TRACE(WM_90A84_TRACE_LOAD_HEADING, WM_90A84_HEADING, heading);
    WM_90A84_TRACE(WM_90A84_TRACE_LOAD_PREVIOUS_STATE,
                   WM_90A84_PREVIOUS_STATE, previous);
#if defined(WM_90A84_MUTANT_M15)
    WM_90A84_TRACE(WM_90A84_TRACE_STORE_EDGE_STATE,
                   WM_90A84_CHANGED_STATE, current);
    wm_90a84_store_u32(WM_90A84_CHANGED_STATE, current);
#else
    WM_90A84_TRACE(WM_90A84_TRACE_STORE_EDGE_STATE,
                   WM_90A84_EDGE_STATE, current);
    wm_90a84_store_u32(WM_90A84_EDGE_STATE, current);
#endif

#if defined(WM_90A84_MUTANT_M14)
    WM_90A84_TRACE(WM_90A84_TRACE_STORE_PREVIOUS_STATE,
                   WM_90A84_PREVIOUS_STATE, current);
    wm_90a84_store_u32(WM_90A84_PREVIOUS_STATE, current);
#endif
#if defined(WM_90A84_MUTANT_M16)
    WM_90A84_TRACE(WM_90A84_TRACE_STORE_CHANGED_STATE,
                   WM_90A84_EDGE_STATE, changed);
    wm_90a84_store_u32(WM_90A84_EDGE_STATE, changed);
#else
    WM_90A84_TRACE(WM_90A84_TRACE_STORE_CHANGED_STATE,
                   WM_90A84_CHANGED_STATE, changed);
    wm_90a84_store_u32(WM_90A84_CHANGED_STATE, changed);
#endif
#if !defined(WM_90A84_MUTANT_M14)
    WM_90A84_TRACE(WM_90A84_TRACE_STORE_PREVIOUS_STATE,
                   WM_90A84_PREVIOUS_STATE, current);
    wm_90a84_store_u32(WM_90A84_PREVIOUS_STATE, current);
#endif
}

static u16 wm_90a84_adjust_angle(u16 angle, s32 delta)
{
    u32 adjusted = (u32)angle + (u32)delta;

#if defined(WM_90A84_MUTANT_M7)
    return (u16)adjusted;
#else
    return (u16)(adjusted & 0x0FFFu);
#endif
}

static u16 wm_90a84_phase9_angle(u16 angle)
{
#if defined(WM_90A84_MUTANT_M6)
    return wm_90a84_adjust_angle(angle, -0x300);
#else
    return wm_90a84_adjust_angle(angle, -0x200);
#endif
}

static void wm_90a84_select_heading(u32 slot_addr, u32 phase)
{
    u16 angle;

    if (phase < 1u || phase > 12u)
        return;

    angle = wm_90a84_load_u16(WM_90A84_ANGLE);
    WM_90A84_TRACE(WM_90A84_TRACE_LOAD_ANGLE, WM_90A84_ANGLE, angle);
    switch (phase) {
    case 1u:
        WM_90A84_TRACE(WM_90A84_TRACE_STORE_ANGLE,
                       slot_addr + WM_90A84_SLOT_ANGLE, angle);
        wm_90a84_store_u16(slot_addr + WM_90A84_SLOT_ANGLE,
                            angle);
        break;
    case 2u:
        WM_90A84_TRACE(WM_90A84_TRACE_STORE_ANGLE,
                       slot_addr + WM_90A84_SLOT_ANGLE,
                       wm_90a84_adjust_angle(angle, 0x400));
        wm_90a84_store_u16(slot_addr + WM_90A84_SLOT_ANGLE,
                            wm_90a84_adjust_angle(angle, 0x400));
        break;
    case 3u:
        WM_90A84_TRACE(WM_90A84_TRACE_STORE_ANGLE,
                       slot_addr + WM_90A84_SLOT_ANGLE,
                       wm_90a84_adjust_angle(angle, 0x200));
        wm_90a84_store_u16(slot_addr + WM_90A84_SLOT_ANGLE,
                            wm_90a84_adjust_angle(angle, 0x200));
        break;
    case 4u:
        WM_90A84_TRACE(WM_90A84_TRACE_STORE_ANGLE,
                       slot_addr + WM_90A84_SLOT_ANGLE,
                       wm_90a84_adjust_angle(angle, 0x800));
        wm_90a84_store_u16(slot_addr + WM_90A84_SLOT_ANGLE,
                            wm_90a84_adjust_angle(angle, 0x800));
        break;
    case 6u:
        WM_90A84_TRACE(WM_90A84_TRACE_STORE_ANGLE,
                       slot_addr + WM_90A84_SLOT_ANGLE,
                       wm_90a84_adjust_angle(angle,
#if defined(WM_90A84_MUTANT_M8)
                                             0x400
#else
                                             0x600
#endif
                                             ));
        wm_90a84_store_u16(slot_addr + WM_90A84_SLOT_ANGLE,
                            wm_90a84_adjust_angle(angle,
#if defined(WM_90A84_MUTANT_M8)
                                                  0x400
#else
                                                  0x600
#endif
                                                  ));
        break;
    case 8u:
        WM_90A84_TRACE(WM_90A84_TRACE_STORE_ANGLE,
                       slot_addr + WM_90A84_SLOT_ANGLE,
                       wm_90a84_adjust_angle(angle, -0x400));
        wm_90a84_store_u16(slot_addr + WM_90A84_SLOT_ANGLE,
                            wm_90a84_adjust_angle(angle, -0x400));
        break;
    case 9u:
        WM_90A84_TRACE(WM_90A84_TRACE_STORE_ANGLE,
                       slot_addr + WM_90A84_SLOT_ANGLE,
                       wm_90a84_phase9_angle(angle));
        wm_90a84_store_u16(slot_addr + WM_90A84_SLOT_ANGLE,
                            wm_90a84_phase9_angle(angle));
        break;
    case 12u:
        WM_90A84_TRACE(WM_90A84_TRACE_STORE_ANGLE,
                       slot_addr + WM_90A84_SLOT_ANGLE,
                       wm_90a84_adjust_angle(angle, -0x600));
        wm_90a84_store_u16(slot_addr + WM_90A84_SLOT_ANGLE,
                            wm_90a84_adjust_angle(angle, -0x600));
        break;
    default:
        /* Retail table entries 5, 7, 10, and 11 target the common tail. */
        break;
    }
}

static s32 wm_90a84_update_result(void)
{
    u16 flags = wm_90a84_load_u16(WM_90A84_FLAGS);
    s16 selection;
    u32 object_bits;
    u32 object_address;

    WM_90A84_TRACE(WM_90A84_TRACE_LOAD_FLAGS, WM_90A84_FLAGS, flags);
    if ((flags & 0x20u) != 0u) {
#if defined(WM_90A84_MUTANT_M9)
        if (wm_90a84_load_u8(WM_90A84_BYTE_STATE) != 0u)
            return 1;
#else
        WM_90A84_TRACE(WM_90A84_TRACE_LOAD_BYTE_STATE, WM_90A84_BYTE_STATE,
                       wm_90a84_load_u8(WM_90A84_BYTE_STATE));
        if (wm_90a84_load_u8(WM_90A84_BYTE_STATE) != 0u)
            return 3;
#endif

#if defined(WM_90A84_MUTANT_M11)
        selection = wm_90a84_s16(wm_90a84_load_u16(WM_90A84_SELECTION_ALT));
#else
        selection = wm_90a84_s16(wm_90a84_load_u16(WM_90A84_SELECTION));
#endif
        WM_90A84_TRACE(WM_90A84_TRACE_LOAD_SELECTION, WM_90A84_SELECTION,
                       (u32)(u16)selection);
        if (selection != (s16)-1)
            return 1;
    } else {
        selection = wm_90a84_s16(wm_90a84_load_u16(WM_90A84_SELECTION));
        WM_90A84_TRACE(WM_90A84_TRACE_LOAD_SELECTION, WM_90A84_SELECTION,
                       (u32)(u16)selection);
        if (selection != (s16)-1) {
            object_bits = wm_90a84_load_u32(WM_90A84_OBJECT);
            WM_90A84_TRACE(WM_90A84_TRACE_LOAD_OBJECT,
                           WM_90A84_OBJECT, object_bits);
            object_address = object_bits + 0x0Eu;
#if !defined(WM_90A84_MUTANT_M10)
            WM_90A84_TRACE(WM_90A84_TRACE_LOAD_OBJECT_STATE, object_address,
                           wm_90a84_load_u16(object_address));
            if (wm_90a84_s16(wm_90a84_load_u16(object_address)) == (s16)1)
                return 1;
#else
            (void)object_address;
#endif
        }
    }

    if ((flags & 0x10u) != 0u) {
        selection = wm_90a84_s16(wm_90a84_load_u16(WM_90A84_SELECTION_ALT));
        WM_90A84_TRACE(WM_90A84_TRACE_LOAD_ALT_SELECTION,
                       WM_90A84_SELECTION_ALT, (u32)(u16)selection);
        if (selection == (s16)-1) {
            selection = wm_90a84_s16(wm_90a84_load_u16(WM_90A84_SELECTION));
            WM_90A84_TRACE(WM_90A84_TRACE_LOAD_SELECTION,
                           WM_90A84_SELECTION, (u32)(u16)selection);
            if (selection == (s16)-1) {
                WM_90A84_TRACE(WM_90A84_TRACE_STORE_STATE_PUBLISH,
                               WM_90A84_STATE_PUBLISH, 1u);
                wm_90a84_store_u32(WM_90A84_STATE_PUBLISH, 1u);
            }
        }
    }

    WM_90A84_TRACE(WM_90A84_TRACE_CALL_90A18, 0x80090C48u, 0u);
#if !defined(WM_90A84_MUTANT_M12)
    wm_80090A18();
#endif
    return 0;
}

s32 wm_80090A84(u32 slot_addr)
{
    u16 heading = wm_90a84_load_u16(WM_90A84_HEADING);
    u32 phase = (u32)heading >> 12;
    s32 angle;
    s32 cosine;
    s32 sine;

    WM_90A84_TRACE(WM_90A84_TRACE_ENTRY, slot_addr, phase);
    WM_90A84_TRACE(WM_90A84_TRACE_LOAD_HEADING, WM_90A84_HEADING, heading);
    wm_90a84_select_heading(slot_addr, phase);

    if ((heading & 0xF000u) != 0u) {
        angle = (s32)wm_90a84_s16(
            wm_90a84_load_u16(slot_addr + WM_90A84_SLOT_ANGLE));
#if defined(WM_90A84_MUTANT_M4)
        WM_90A84_TRACE(WM_90A84_TRACE_CALL_RSIN, 0x80090B78u,
                       (u32)(s32)angle);
        sine = rsin(angle);
#endif
        WM_90A84_TRACE(WM_90A84_TRACE_CALL_RCOS, 0x80090B6Cu,
                       (u32)(s32)angle);
#if defined(WM_90A84_MUTANT_M2)
        cosine = 0;
#else
        cosine = rcos(angle);
#endif
        /* 80090B7C is the jal rsin delay slot: v0 still holds rcos. */
        WM_90A84_TRACE(WM_90A84_TRACE_STORE_COS,
                       slot_addr + WM_90A84_SLOT_COS, (u32)cosine);
        wm_90a84_store_u32(slot_addr + WM_90A84_SLOT_COS,
                            (u32)cosine);
#if defined(WM_90A84_MUTANT_M3)
        sine = 0;
#elif !defined(WM_90A84_MUTANT_M4)
        WM_90A84_TRACE(WM_90A84_TRACE_CALL_RSIN, 0x80090B78u,
                       (u32)(s32)angle);
        sine = rsin(angle);
#endif
#if defined(WM_90A84_MUTANT_M1)
        /* Reproduce the former post-call store bug. */
        wm_90a84_store_u32(slot_addr + WM_90A84_SLOT_COS, (u32)sine);
#endif
        WM_90A84_TRACE(WM_90A84_TRACE_STORE_SIN,
                       slot_addr + WM_90A84_SLOT_SIN,
                       wm_90a84_negate_bits(sine));
        wm_90a84_store_u32(slot_addr + WM_90A84_SLOT_SIN,
                           wm_90a84_negate_bits(sine));
    }

    {
        s32 result = wm_90a84_update_result();
        WM_90A84_TRACE(WM_90A84_TRACE_RETURN, 0x80090C50u,
                       (u32)result);
        return result;
    }
}
