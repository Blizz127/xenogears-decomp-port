/* Retail mode-9 scheduler callbacks [0x80077DC8,0x80078948). */
#include <stdint.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "psyq/libgte.h"
#include "world_map_callback_77dc8.h"
#include "world_map_common_tail.h"
#include "world_map_helper_76858.h"
#include "world_map_helper_848b4.h"
#include "world_map_helper_93354.h"
#include "world_map_helper_94154.h"
#include "world_map_helper_97070.h"
#include "world_map_helper_97244.h"
#include "world_map_helper_97770.h"

extern void* D_8006259C;
extern void func_80039E60(s32 packed_id);
extern void func_8003A2E4(s32 packed_id, s32 volume);
extern void func_8003A3B8(s32 packed_id, s32 pitch, s32 steps);

#define WM_M9_POOL_PTR        0x8009BE24u
#define WM_M9_PATH_TABLE      0x8009A3F0u
#if defined(W34N60_MUTANT_WRONG_RENDER_Z)
#define WM_M9_RENDER_Z        0x8009BBB4u
#else
#define WM_M9_RENDER_Z        0x8009BBBCu
#endif
#define WM_M9_ANGLES          0x8009BD38u
#define WM_M9_CAMERA_INPUT    0x8009BD40u
#define WM_M9_CAMERA_MATRIX   0x8009C808u
#define WM_M9_POSITION        0x8009BE28u
#define WM_M9_CONTEXT_PTR     0x8009C620u
#define WM_M9_CAMERA_SOURCE   0x8009C5ACu
#define WM_M9_GEOM_Y          0x8009BE0Cu
#define WM_M9_MODE_STATE      0x8009D144u
#define WM_M9_HEIGHT          0x8009D3F0u
#define WM_M9_CCA4            0x8009CCA4u
#define WM_M9_D3CC            0x8009D3CCu
#define WM_M9_D554            0x8009D554u
#define WM_M9_D7CC            0x8009D7CCu

#define WM_M9_SC_VECTOR       0x1F800000u
#define WM_M9_SC_VECTOR_B     0x1F800010u
#define WM_M9_SC_DISTANCE     0x1F800020u
#define WM_M9_SC_PATH_A       0x1F8000A0u
#define WM_M9_SC_PATH_B       0x1F8000A8u
#define WM_M9_SC_PATH_C       0x1F8000B0u
#define WM_M9_SC_ROT_A        0x1F8000F0u
#define WM_M9_SC_ROT_B        0x1F800110u
#define WM_M9_SC_ROT_C        0x1F800130u
#define WM_M9_SC_ROT_D        0x1F800150u

static s16 m9_lh(u32 address)
{
    s16 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static u16 m9_lhu(u32 address)
{
    u16 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static u32 m9_lw(u32 address)
{
    u32 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static void m9_sh(u32 address, u16 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static void m9_sw(u32 address, u32 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static s32 m9_as_s32(u32 bits)
{
    s32 value;
    memcpy(&value, &bits, sizeof(value));
    return value;
}

static u32 m9_sra(u32 bits, unsigned shift)
{
    u32 shifted = bits >> shift;
    if ((bits & 0x80000000u) != 0u)
        shifted |= ~(UINT32_MAX >> shift);
    return shifted;
}

static u32 m9_slot(s32 slot_index)
{
    return m9_lw(WM_M9_POOL_PTR) + ((u32)slot_index << 7);
}

static s32 m9_sound_id(void)
{
    u16 bank;
    memcpy(&bank, (uint8_t*)D_8006259C + 0x14u, sizeof(bank));
    return m9_as_s32(((u32)bank << 16) | 0x00A4u);
}

static void m9_copy_words(u32 destination, u32 source, u32 count)
{
    u32 index;
    for (index = 0u; index < count; index++)
        m9_sw(destination + index * 4u, m9_lw(source + index * 4u));
}

s32 wm_80077DC8(s32 slot_index)
{
    u32 slot = m9_slot(slot_index);

    m9_sw(WM_M9_GEOM_Y, 120u);
    m9_sw(WM_M9_HEIGHT, 0x00200000u);
    m9_sh(slot + 0x20u, 0u);
    m9_sw(slot + 0x50u, 0u);
    m9_sw(slot + 0x54u, 0u);
    m9_sw(slot + 0x58u, 0u);
    m9_sh(WM_M9_ANGLES + 0u, 0u);
    m9_sh(WM_M9_ANGLES + 2u, 0u);
    m9_sh(WM_M9_ANGLES + 4u, 0u);
    m9_sw(WM_M9_MODE_STATE, 1u);
    func_80039E60(m9_sound_id());
    m9_sh(slot + 0x22u, 24u);
    return 1;
}

static void m9_path_sample(u32 slot)
{
    u32 phase = m9_lw(slot + 0x50u);
    s32 record_index = m9_as_s32(m9_sra(phase, 12u));
    u32 record = WM_M9_PATH_TABLE + ((u32)record_index << 3);

    m9_sw(slot + 0x54u, m9_sra(phase, 12u));
    if (m9_lh(record + 0x16u) == -1)
        return;

    m9_sh(WM_M9_SC_PATH_A + 0u, m9_lhu(record + 0x00u));
    m9_sh(WM_M9_SC_PATH_B + 0u, m9_lhu(record + 0x08u));
    m9_sh(WM_M9_SC_PATH_C + 0u, m9_lhu(record + 0x10u));
    m9_sh(WM_M9_SC_PATH_A + 2u, m9_lhu(record + 0x02u));
    m9_sh(WM_M9_SC_PATH_B + 2u, m9_lhu(record + 0x0Au));
    m9_sh(WM_M9_SC_PATH_C + 2u, m9_lhu(record + 0x12u));
    m9_sh(WM_M9_SC_PATH_A + 4u, m9_lhu(record + 0x04u));
    m9_sh(WM_M9_SC_PATH_B + 4u, m9_lhu(record + 0x0Cu));
    m9_sh(WM_M9_SC_PATH_C + 4u, m9_lhu(record + 0x14u));

    wm_80076858((s32)(phase & 0x0FFFu), WM_M9_SC_PATH_A,
                 WM_M9_SC_PATH_B, WM_M9_SC_PATH_C, WM_M9_SC_VECTOR);

    m9_sh(WM_M9_CAMERA_INPUT + 0x0Cu, 0u);
    m9_sh(WM_M9_CAMERA_INPUT + 0x08u, 0u);
    m9_sw(WM_M9_CAMERA_INPUT + 0x18u, 0u);
    m9_sw(WM_M9_CAMERA_INPUT + 0x10u, 0u);
    m9_sw(WM_M9_CAMERA_INPUT + 0x14u, 0xFFFFF000u);
    m9_sh(WM_M9_CAMERA_INPUT + 0x00u, m9_lhu(WM_M9_SC_VECTOR + 2u));
    m9_sh(WM_M9_CAMERA_INPUT + 0x04u,
          (u16)(0u - (u32)m9_lhu(WM_M9_SC_VECTOR + 0x0Au)));
    m9_sh(WM_M9_CAMERA_INPUT + 0x02u, m9_lhu(WM_M9_SC_VECTOR + 6u));
    m9_sh(WM_M9_CAMERA_INPUT + 0x0Au,
          (u16)m9_sra(m9_lw(WM_M9_POSITION + 4u), 12u));
}

static void m9_update_path_state(u32 slot)
{
    s16 state = m9_lh(slot + 0x20u);

    switch (state) {
    case 0:
        m9_sw(slot + 0x58u, m9_lw(slot + 0x58u) + 5u);
        m9_sh(slot + 0x22u, (u16)(m9_lhu(slot + 0x22u) - 1u));
        if (m9_lh(slot + 0x22u) < 0) {
            m9_sw(slot + 0x58u, 128u);
            m9_sh(slot + 0x20u, 1u);
            m9_sh(WM_M9_SC_PATH_A + 0u,
                  (u16)(m9_sra(m9_lw(WM_M9_POSITION), 12u) + 50u));
            m9_sh(WM_M9_SC_PATH_A + 2u, (u16)-525);
            m9_sh(WM_M9_SC_PATH_A + 4u,
                  (u16)(m9_sra(m9_lw(WM_M9_POSITION + 8u), 12u) - 112u));
            wm_80089160(8u, WM_M9_SC_PATH_A, 0u);
        }
        break;
    case 1:
        if (m9_as_s32(m9_lw(slot + 0x50u)) >= 24576)
            m9_sh(slot + 0x20u, 2u);
        break;
    case 2:
        m9_sw(slot + 0x58u, m9_lw(slot + 0x58u) - 2u);
        if (m9_as_s32(m9_lw(slot + 0x58u)) < 25) {
            m9_sw(slot + 0x58u, 24u);
            m9_sh(slot + 0x20u, 3u);
        }
        break;
    case 3:
        if (m9_as_s32(m9_lw(slot + 0x50u)) > 32767)
            m9_sh(slot + 0x20u, 4u);
        break;
    case 4:
        m9_sw(slot + 0x58u, m9_lw(slot + 0x58u) - 1u);
        if (m9_as_s32(m9_lw(slot + 0x58u)) < 5) {
            m9_sw(slot + 0x58u, 4u);
            m9_sh(slot + 0x20u, 5u);
        }
        break;
    case 5:
        if (m9_as_s32(m9_lw(slot + 0x50u)) > 0x83FF) {
            func_8003A3B8(m9_sound_id(), 0, 256);
            (void)wm_80097770(0u, 13);
            m9_sw(WM_M9_CCA4, 2u);
            m9_sw(WM_M9_D3CC, 4u);
            m9_sh(slot + 0x22u, 80u);
            m9_sw(slot + 0x58u, 0u);
            m9_sh(slot + 0x20u, 6u);
        }
        break;
    case 6:
        m9_sh(slot + 0x22u, (u16)(m9_lhu(slot + 0x22u) - 1u));
        if (m9_lh(slot + 0x22u) <= 0) {
#if !defined(W34N60_MUTANT_SKIP_TERMINAL_EXIT)
            m9_sw(WM_M9_D554, 0u);
            m9_sw(WM_M9_D7CC, 0u);
#endif
        }
        break;
    default:
        break;
    }
}

static void m9_update_sound(u32 slot)
{
    s32 distance;

    if (m9_lh(slot + 0x20u) >= 6)
        return;

#if defined(W34N60_MUTANT_WORD_SOUND_VECTOR)
    m9_sw(WM_M9_SC_VECTOR + 0u,
          m9_lw(WM_M9_CAMERA_INPUT + 0x08u) << 12);
#else
    m9_sw(WM_M9_SC_VECTOR + 0u,
          (u32)(s32)m9_lh(WM_M9_CAMERA_INPUT + 0x08u) << 12);
#endif
    m9_sw(WM_M9_SC_VECTOR + 4u,
          (u32)(s32)m9_lh(WM_M9_CAMERA_INPUT + 0x0Au) << 12);
    m9_sw(WM_M9_SC_VECTOR + 8u,
          (u32)(s32)m9_lh(WM_M9_CAMERA_INPUT + 0x0Cu) << 12);
    m9_sw(WM_M9_SC_VECTOR_B + 0u,
          (u32)(s32)m9_lh(WM_M9_CAMERA_INPUT + 0x00u) << 12);
    m9_sw(WM_M9_SC_VECTOR_B + 4u,
          (u32)(s32)m9_lh(WM_M9_CAMERA_INPUT + 0x02u) << 12);
    m9_sw(WM_M9_SC_VECTOR_B + 8u,
          (u32)(s32)m9_lh(WM_M9_CAMERA_INPUT + 0x04u) << 12);
    distance = wm_80094154(WM_M9_SC_VECTOR, WM_M9_SC_VECTOR_B);
    distance = m9_as_s32(m9_sra((u32)distance, 3u));
    m9_sw(WM_M9_SC_DISTANCE, (u32)distance);
    func_8003A2E4(m9_sound_id(), 135 - distance);
}

s32 wm_80077E68(s32 slot_index)
{
    u32 slot = m9_slot(slot_index);

    m9_path_sample(slot);
    wm_80097244(WM_M9_CAMERA_INPUT);
    wm_80097070(WM_M9_CAMERA_MATRIX, WM_M9_ANGLES);
    m9_update_path_state(slot);
    m9_sw(slot + 0x50u, m9_lw(slot + 0x50u) + m9_lw(slot + 0x58u));
    m9_update_sound(slot);
    return 1;
}

s32 wm_8007828C(s32 slot_index)
{
    u32 slot;
    s32 destination;

#if defined(W34N60_MUTANT_SKIP_LAST_LINK)
    for (destination = 1; destination < 13; destination++)
#else
    for (destination = 1; destination <= 13; destination++)
#endif
        wm_800848B4(0, destination);

    slot = m9_slot(slot_index);
    m9_sh(slot + 0x20u, 0u);
    m9_copy_words(slot + 0x28u, WM_M9_CAMERA_SOURCE, 4u);
    m9_sw(slot + 0x50u, 0u);
    m9_sw(slot + 0x54u, 64u);
    m9_sw(slot + 0x58u, 512u);
    m9_sw(slot + 0x5Cu, 96u);
    m9_sw(slot + 0x60u, 768u);
    m9_sw(slot + 0x64u, 80u);
    m9_copy_words(WM_M9_POSITION, slot + 0x28u, 4u);
    return 1;
}

s32 wm_800783E8(s32 slot_index)
{
    u32 slot = m9_slot(slot_index);
    u32 context;

    m9_sw(slot + 0x30u, m9_lw(slot + 0x30u) - 0x4000u);
    m9_sw(WM_M9_RENDER_Z, m9_lw(WM_M9_RENDER_Z) - 0x4000u);
    wm_80093354(slot + 0x28u);

    m9_sw(WM_M9_SC_VECTOR + 0u, m9_sra(m9_lw(slot + 0x28u), 12u));
    m9_sw(WM_M9_SC_VECTOR + 4u, m9_sra(m9_lw(slot + 0x2Cu), 12u));
    m9_sw(WM_M9_SC_VECTOR + 8u, m9_sra(m9_lw(slot + 0x30u), 12u));
    context = m9_lw(WM_M9_CONTEXT_PTR);
    m9_copy_words(context + 0x08u, WM_M9_SC_VECTOR, 4u);
    m9_copy_words(WM_M9_POSITION, slot + 0x28u, 4u);

    m9_sw(slot + 0x50u, (m9_lw(slot + 0x50u) +
                         m9_lw(slot + 0x54u)) & 0x0FFFu);
    m9_sw(slot + 0x58u, (m9_lw(slot + 0x58u) +
                         m9_lw(slot + 0x5Cu)) & 0x0FFFu);
    m9_sw(slot + 0x60u, (m9_lw(slot + 0x60u) +
                         m9_lw(slot + 0x64u)) & 0x0FFFu);

    m9_sh(WM_M9_SC_PATH_A + 0u, 0u);
    m9_sh(WM_M9_SC_PATH_A + 4u, 0u);
    m9_sh(WM_M9_SC_PATH_B + 0u, 0u);
    m9_sh(WM_M9_SC_PATH_B + 4u, 0u);
    m9_sh(WM_M9_SC_PATH_C + 0u, 0u);
    m9_sh(WM_M9_SC_PATH_C + 4u, 0u);
    m9_sh(WM_M9_SC_PATH_C + 8u, 0u);
    m9_sh(WM_M9_SC_PATH_C + 0x0Au, 0u);
    m9_sh(WM_M9_SC_PATH_A + 2u, (u16)m9_lw(slot + 0x50u));
    m9_sh(WM_M9_SC_PATH_B + 2u, (u16)m9_lw(slot + 0x58u));
    m9_sh(WM_M9_SC_PATH_C + 2u, (u16)m9_lw(slot + 0x60u));
    m9_sh(WM_M9_SC_PATH_C + 0x0Cu, (u16)m9_lw(slot + 0x60u));

    (void)RotMatrixYXZ((SVECTOR*)PSX_ADDR(WM_M9_SC_PATH_A),
                       (MATRIX*)PSX_ADDR(WM_M9_SC_ROT_A));
    (void)RotMatrixYXZ((SVECTOR*)PSX_ADDR(WM_M9_SC_PATH_B),
                       (MATRIX*)PSX_ADDR(WM_M9_SC_ROT_B));
    (void)RotMatrixYXZ((SVECTOR*)PSX_ADDR(WM_M9_SC_PATH_C),
                       (MATRIX*)PSX_ADDR(WM_M9_SC_ROT_C));
    (void)RotMatrixYXZ((SVECTOR*)PSX_ADDR(WM_M9_SC_PATH_C + 8u),
                       (MATRIX*)PSX_ADDR(WM_M9_SC_ROT_D));

#if defined(W34N60_MUTANT_WRONG_MATRIX_SOURCE)
    m9_copy_words(context + 0x11Cu, WM_M9_SC_ROT_C, 8u);
#else
    m9_copy_words(context + 0x11Cu, WM_M9_SC_ROT_D, 8u);
#endif
    m9_copy_words(context + 0x0C8u, context + 0x11Cu, 8u);
    m9_copy_words(context + 0x074u, context + 0x0C8u, 8u);
    m9_copy_words(context + 0x1C4u, WM_M9_SC_ROT_C, 8u);
    m9_copy_words(context + 0x170u, context + 0x1C4u, 8u);
    m9_copy_words(context + 0x410u, WM_M9_SC_ROT_A, 8u);
    m9_copy_words(context + 0x2C0u, context + 0x410u, 8u);
    m9_copy_words(context + 0x368u, context + 0x2C0u, 8u);
    m9_copy_words(context + 0x218u, context + 0x368u, 8u);
    m9_copy_words(context + 0x464u, WM_M9_SC_ROT_B, 8u);
    m9_copy_words(context + 0x314u, context + 0x464u, 8u);
    m9_copy_words(context + 0x3BCu, context + 0x314u, 8u);
    m9_copy_words(context + 0x26Cu, context + 0x3BCu, 8u);
    return 1;
}
