/* Exact native transcription of retail [0x80084068, 0x8008440C). */
#include <stdint.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_callback_827ec.h"
#include "world_map_callback_84068.h"
#include "world_map_common_tail.h"

#define WM_M18O_POOL_PTR UINT32_C(0x8009BE24)
#define WM_M18O_OBJECT   UINT32_C(0x8009C620)
#define WM_M18O_SCRATCH  UINT32_C(0x1F800000)

static s16 m18o_lh(u32 address)
{
    s16 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static u32 m18o_lw(u32 address)
{
    u32 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static void m18o_sh(u32 address, u16 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static void m18o_sw(u32 address, u32 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static s32 m18o_as_s32(u32 bits)
{
    s32 value;
    memcpy(&value, &bits, sizeof(value));
    return value;
}

static u32 m18o_as_u32(s32 value)
{
    u32 bits;
    memcpy(&bits, &value, sizeof(bits));
    return bits;
}

static u32 m18o_sra(u32 bits, unsigned shift)
{
    u32 value = bits >> shift;

    if ((bits & UINT32_C(0x80000000)) != 0u)
        value |= ~(UINT32_MAX >> shift);
    return value;
}

static u32 m18o_slot(s32 slot_index)
{
    return m18o_lw(WM_M18O_POOL_PTR) + ((u32)slot_index << 7u);
}

static void m18o_clear_records(const u8 *records, unsigned count)
{
    unsigned index;

    for (index = 0u; index < count; index++)
        wm_800894C8((u32)records[index]);
}

static void m18o_consume_latch(u32 slot, u32 object)
{
    static const u8 latch2_records[] = {3u, 5u, 6u, 7u, 10u};
    static const u8 latch3_records[] = {7u, 8u, 9u};
    static const u8 latch4_records[] = {5u, 6u, 7u, 10u, 11u, 12u};

#if defined(W34N117_MUTANT_KEEP_OBJECT_ENABLED)
    (void)object;
#endif
#if defined(W34N117_MUTANT_LATCH2_WRONG_CLEAR)
    (void)latch2_records;
#endif
#if defined(W34N117_MUTANT_LATCH3_WRONG_CLEAR)
    (void)latch3_records;
#endif
#if defined(W34N117_MUTANT_LATCH4_WRONG_CLEAR)
    (void)latch4_records;
#endif

    switch (m18o_lh(slot + 0x04u)) {
    case 1:
        m18o_sh(slot + 0x04u, 0u);
        m18o_sh(slot + 0x20u, 1u);
#if !defined(W34N117_MUTANT_KEEP_OBJECT_ENABLED)
        m18o_sh(object + 0x2A0u, 0u);
        m18o_sh(object + 0x24Cu, 0u);
#endif
        break;
    case 2:
        m18o_sh(slot + 0x04u, 0u);
        m18o_sh(slot + 0x20u, 2u);
#if defined(W34N117_MUTANT_LATCH2_WRONG_CLEAR)
        wm_800894C8(4u);
#else
        m18o_clear_records(latch2_records,
                           (unsigned)(sizeof(latch2_records) /
                                      sizeof(latch2_records[0])));
#endif
        break;
    case 3:
        m18o_sh(slot + 0x04u, 0u);
        m18o_sh(slot + 0x20u, 3u);
#if defined(W34N117_MUTANT_LATCH3_WRONG_CLEAR)
        wm_800894C8(6u);
#else
        m18o_clear_records(latch3_records,
                           (unsigned)(sizeof(latch3_records) /
                                      sizeof(latch3_records[0])));
#endif
        break;
    case 4:
        m18o_sh(slot + 0x04u, 0u);
        m18o_sh(slot + 0x20u, 4u);
#if defined(W34N117_MUTANT_LATCH4_WRONG_CLEAR)
        wm_800894C8(13u);
#else
        m18o_clear_records(latch4_records,
                           (unsigned)(sizeof(latch4_records) /
                                      sizeof(latch4_records[0])));
#endif
        break;
    default:
        break;
    }
}

static void m18o_build_vector(u32 slot)
{
    m18o_sh(WM_M18O_SCRATCH + 0xA0u,
            (u16)m18o_sra(m18o_lw(slot + 0x28u), 12u));
    m18o_sh(WM_M18O_SCRATCH + 0xA2u,
            (u16)m18o_sra(m18o_lw(slot + 0x2Cu), 12u));
    m18o_sh(WM_M18O_SCRATCH + 0xA4u,
            (u16)m18o_sra(m18o_lw(slot + 0x30u), 12u));
}

static void m18o_emit_records(const u8 *records, unsigned count)
{
    unsigned index;

    for (index = 0u; index < count; index++)
        wm_80089160((u32)records[index], WM_M18O_SCRATCH + 0xA0u, 0u);
}

static void m18o_approach_y(u32 slot)
{
    s32 current = m18o_as_s32(m18o_lw(slot + 0x2Cu));

#if defined(W34N117_MUTANT_WRONG_Y_STEP)
    current = wm_800771D8(current, INT32_C(-0x00180000),
                          INT32_C(0x00000400));
#else
    current = wm_800771D8(current, INT32_C(-0x00180000),
                          INT32_C(-0x00000400));
#endif
    m18o_sw(slot + 0x2Cu, m18o_as_u32(current));
}

static void m18o_run_state(u32 slot)
{
    static const u8 state1_records[] = {3u, 5u, 6u, 7u, 10u};
    static const u8 state2_records[] = {7u, 8u, 9u};
    static const u8 state3_records[] = {5u, 6u, 7u, 10u, 11u, 12u};

#if defined(W34N117_MUTANT_STATE1_WRONG_LIST)
    (void)state1_records;
#endif
#if defined(W34N117_MUTANT_STATE2_WRONG_LIST)
    (void)state2_records;
#endif
#if defined(W34N117_MUTANT_STATE3_WRONG_LIST)
    (void)state3_records;
#endif

    switch (m18o_lh(slot + 0x20u)) {
    case 1:
        m18o_approach_y(slot);
        m18o_build_vector(slot);
#if defined(W34N117_MUTANT_STATE1_WRONG_LIST)
        wm_80089160(4u, WM_M18O_SCRATCH + 0xA0u, 0u);
#else
        m18o_emit_records(state1_records,
                          (unsigned)(sizeof(state1_records) /
                                     sizeof(state1_records[0])));
#endif
        break;
    case 2:
        m18o_approach_y(slot);
        m18o_build_vector(slot);
#if defined(W34N117_MUTANT_STATE2_WRONG_LIST)
        wm_80089160(6u, WM_M18O_SCRATCH + 0xA0u, 0u);
#else
        m18o_emit_records(state2_records,
                          (unsigned)(sizeof(state2_records) /
                                     sizeof(state2_records[0])));
#endif
        break;
    case 3:
        m18o_approach_y(slot);
        m18o_build_vector(slot);
#if defined(W34N117_MUTANT_STATE3_WRONG_LIST)
        wm_80089160(4u, WM_M18O_SCRATCH + 0xA0u, 0u);
#else
        m18o_emit_records(state3_records,
                          (unsigned)(sizeof(state3_records) /
                                     sizeof(state3_records[0])));
#endif
        break;
    case 4:
        m18o_approach_y(slot);
        m18o_build_vector(slot);
        wm_80089160(13u, WM_M18O_SCRATCH + 0xA0u, 0u);
#if !defined(W34N117_MUTANT_STATE4_KEEP_Y)
        m18o_sh(WM_M18O_SCRATCH + 0xA2u, 0u);
#endif
        wm_80089160(14u, WM_M18O_SCRATCH + 0xA0u, 0u);
        break;
    default:
        break;
    }
}

static void m18o_publish_object_position(u32 slot, u32 object)
{
    u32 subrecord = object + 0x24Cu;
    u32 x = m18o_sra(m18o_lw(slot + 0x28u), 12u);
    u32 y = m18o_sra(m18o_lw(slot + 0x2Cu), 12u);
    u32 z = m18o_sra(m18o_lw(slot + 0x30u), 12u);

#if defined(W34N117_MUTANT_SKIP_OBJECT_PUBLISH)
    (void)subrecord;
    (void)x;
    (void)y;
    (void)z;
#else
    m18o_sw(subrecord + 0x5Cu, x);
    m18o_sw(subrecord + 0x08u, x);
    m18o_sw(subrecord + 0x60u, y);
    m18o_sw(subrecord + 0x0Cu, y);
    m18o_sw(subrecord + 0x64u, z);
    m18o_sw(subrecord + 0x10u, z);
#endif
}

s32 wm_80084068(s32 slot_index)
{
    u32 slot = m18o_slot(slot_index);
    u32 object = m18o_lw(WM_M18O_OBJECT);

    m18o_consume_latch(slot, object);
    m18o_run_state(slot);
    m18o_publish_object_position(slot, object);
    return 1;
}
