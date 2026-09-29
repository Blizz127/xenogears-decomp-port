/* Exact native transcription of retail [0x80081C3C, 0x80081FB4). */
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "psyq/libgpu.h"
#include "world_map_callback_81c3c.h"

extern void *HeapAlloc(u32 size, u32 flags);

#define WM_M16P_BUFFER_A    UINT32_C(0x8009D158)
#define WM_M16P_BUFFER_B    UINT32_C(0x8009D15C)
#define WM_M16P_WIDTH_TABLE UINT32_C(0x8009D148)
#define WM_M16P_MOVE_BASE   UINT32_C(0x8009D164)
#define WM_M16P_STREAM_SIDE UINT32_C(0x8009D7F0)
#define WM_M16P_DRAW_RECORD UINT32_C(0x8009BE3C)

#define WM_M16P_COUNT       UINT32_C(192)
#define WM_M16P_STRIDE      UINT32_C(40)
#define WM_M16P_BUFFER_SIZE (WM_M16P_COUNT * WM_M16P_STRIDE)
#define WM_M16P_TABLE_SIZE  (WM_M16P_COUNT * UINT32_C(2))
#define WM_M16P_LINK_MASK   UINT32_C(0x00FFFFFF)
#define WM_M16P_LEN_MASK    UINT32_C(0xFF000000)

static u16 m16p_lhu(u32 address)
{
    u16 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static u32 m16p_lw(u32 address)
{
    u32 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static void m16p_sb(u32 address, u8 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static void m16p_sh(u32 address, u16 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static void m16p_sw(u32 address, u32 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static u32 m16p_publish(void *host)
{
#if defined(W34N107_MUTANT_RAW_HOST_POINTER)
    return (u32)(uintptr_t)host;
#else
    return PsxMemory_GuestAddr(host);
#endif
}

static void m16p_link_primitive(u32 ot_entry, u32 primitive)
{
    u32 primitive_tag = m16p_lw(primitive);
    u32 ot_tag = m16p_lw(ot_entry);

    m16p_sw(primitive,
             (primitive_tag & WM_M16P_LEN_MASK) |
                 (ot_tag & WM_M16P_LINK_MASK));
    m16p_sw(ot_entry,
             (ot_tag & WM_M16P_LEN_MASK) |
                 (primitive & WM_M16P_LINK_MASK));
}

static u32 m16p_current_ot(void)
{
    u32 draw_record = m16p_lw(WM_M16P_DRAW_RECORD);
    return m16p_lw(draw_record + 0x70u);
}

s32 wm_80081C3C(s32 slot_index)
{
    void *buffer_a_host;
    void *buffer_b_host;
    void *table_host;
    u32 buffer_a;
    u32 buffer_b;
    u32 table;
    u32 index;

    (void)slot_index;
#if defined(W34N107_MUTANT_WRONG_ALLOC_SIZE)
    buffer_a_host = HeapAlloc(WM_M16P_BUFFER_SIZE - WM_M16P_STRIDE, 0u);
#else
    buffer_a_host = HeapAlloc(WM_M16P_BUFFER_SIZE, 0u);
#endif
    buffer_b_host = HeapAlloc(WM_M16P_BUFFER_SIZE, 0u);
    table_host = HeapAlloc(WM_M16P_TABLE_SIZE, 0u);
    buffer_a = m16p_publish(buffer_a_host);
    buffer_b = m16p_publish(buffer_b_host);
    table = m16p_publish(table_host);
    m16p_sw(WM_M16P_BUFFER_A, buffer_a);
    m16p_sw(WM_M16P_BUFFER_B, buffer_b);
    m16p_sw(WM_M16P_WIDTH_TABLE, table);

    for (index = 0u; index < WM_M16P_COUNT; index++) {
        u32 primitive = buffer_a + index * WM_M16P_STRIDE;
        u8 code;

        m16p_sb(primitive + 3u, 9u);
#if defined(W34N107_MUTANT_WRONG_PRIMITIVE_CODE)
        m16p_sb(primitive + 7u, 43u);
#else
        m16p_sb(primitive + 7u, 44u);
#endif
        m16p_sb(primitive + 4u, 128u);
        m16p_sb(primitive + 5u, 128u);
        m16p_sb(primitive + 6u, 128u);
        code = *((u8 *)PSX_ADDR(primitive + 7u));
        m16p_sb(primitive + 7u, (u8)(code | 1u));
        m16p_sh(primitive + 22u, GetTPage(2, 0, 640, 256));
    }

#if !defined(W34N107_MUTANT_SKIP_BUFFER_MIRROR)
    memcpy(PSX_ADDR(buffer_b), PSX_ADDR(buffer_a), WM_M16P_BUFFER_SIZE);
#endif
    for (index = 0u; index < WM_M16P_COUNT; index++) {
#if defined(W34N107_MUTANT_WRONG_WIDTH_SEED)
        m16p_sh(table + index * 2u, 2u);
#else
        m16p_sh(table + index * 2u, 1u);
#endif
    }
    return 1;
}

static void m16p_write_strip_primitive(u32 primitive, u32 index, s32 offset)
{
    s32 left = offset + 64;
    s32 right = offset + 256;
    u32 next = index + 1u;

    m16p_sh(primitive + 8u, (u16)left);
    m16p_sh(primitive + 10u, (u16)index);
    m16p_sb(primitive + 12u, 0u);
    m16p_sb(primitive + 13u, (u8)index);
    m16p_sh(primitive + 16u, (u16)right);
    m16p_sh(primitive + 18u, (u16)index);
    m16p_sb(primitive + 20u, 192u);
    m16p_sb(primitive + 21u, (u8)index);
    m16p_sh(primitive + 24u, (u16)left);
    m16p_sh(primitive + 26u, (u16)next);
    m16p_sb(primitive + 28u, 0u);
    m16p_sb(primitive + 29u, (u8)next);
    m16p_sh(primitive + 32u, (u16)right);
    m16p_sh(primitive + 34u, (u16)next);
    m16p_sb(primitive + 36u, 192u);
    m16p_sb(primitive + 37u, (u8)next);
}

s32 wm_80081D80(s32 slot_index)
{
    u32 side = m16p_lw(WM_M16P_STREAM_SIDE);
    u32 selected_side = side;
    u32 buffer;
    u32 table = m16p_lw(WM_M16P_WIDTH_TABLE);
    u32 index;
    u32 move;
    RECT rect;

    (void)slot_index;
#if defined(W34N107_MUTANT_WRONG_STREAM_SIDE)
    selected_side ^= 1u;
#endif
    buffer = m16p_lw(WM_M16P_BUFFER_A + selected_side * 4u);
    for (index = 0u; index < WM_M16P_COUNT; index++) {
        u32 primitive = buffer + index * WM_M16P_STRIDE;
        u32 width = (u32)m16p_lhu(table + index * 2u);
        s32 remainder = (s32)rand() % (s32)width;
        s32 offset;

#if defined(W34N107_MUTANT_SKIP_WIDTH_CENTERING)
        offset = remainder;
#else
        offset = remainder - (s32)(width >> 1u);
#endif
        m16p_write_strip_primitive(primitive, index, offset);
        m16p_link_primitive(m16p_current_ot(), primitive);
    }

    rect.x = 64;
    rect.y = (s16)(side * 216u);
    rect.w = 192;
    rect.h = 216;
    move = WM_M16P_MOVE_BASE + side * 24u;
#if defined(W34N107_MUTANT_WRONG_MOVE_DESTINATION)
    SetDrawMove((DR_MOVE *)PSX_ADDR(move), &rect, 639, 256);
#else
    SetDrawMove((DR_MOVE *)PSX_ADDR(move), &rect, 640, 256);
#endif
    m16p_link_primitive(m16p_current_ot(), move);
    return 1;
}
