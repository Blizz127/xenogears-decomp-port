/* Exact native transcription of retail [0x8007BB60, 0x8007BF50). */
#include <stdint.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "psyq/libgte.h"
#include "world_map_callback_7bb60.h"
#include "world_map_common_tail.h"
#include "world_map_helper_76858.h"
#include "world_map_helper_848b4.h"
#include "world_map_helper_97070.h"

extern long VectorNormal(VECTOR *input, VECTOR *output);
extern void OuterProduct12(VECTOR *left, VECTOR *right, VECTOR *output);

#define WM_M14C_POOL_PTR       UINT32_C(0x8009BE24)
#define WM_M14C_CONTEXT_PTR    UINT32_C(0x8009C620)
#define WM_M14C_PATH_TABLE     UINT32_C(0x8009A490)

#define WM_M14C_SC_FIRST       UINT32_C(0x1F800000)
#define WM_M14C_SC_SECOND      UINT32_C(0x1F800010)
#define WM_M14C_SC_UP          UINT32_C(0x1F800020)
#define WM_M14C_SC_CROSS       UINT32_C(0x1F800030)
#define WM_M14C_SC_MARKER      UINT32_C(0x1F8000A0)
#define WM_M14C_SC_ANGLES      UINT32_C(0x1F8000A8)
#define WM_M14C_SC_MATRIX      UINT32_C(0x1F8000F0)

static s16 m14c_lh(u32 address)
{
    s16 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static u16 m14c_lhu(u32 address)
{
    u16 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static u32 m14c_lw(u32 address)
{
    u32 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static void m14c_sh(u32 address, u16 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static void m14c_sw(u32 address, u32 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static s32 m14c_as_s32(u32 bits)
{
    s32 value;
    memcpy(&value, &bits, sizeof(value));
    return value;
}

static u32 m14c_sra(u32 bits, unsigned shift)
{
    u32 shifted = bits >> shift;

    if ((bits & UINT32_C(0x80000000)) != 0u)
        shifted |= ~(UINT32_MAX >> shift);
    return shifted;
}

static u32 m14c_slot(s32 slot_index)
{
    return m14c_lw(WM_M14C_POOL_PTR) + ((u32)slot_index << 7u);
}

/* Retail TransposeMatrix writes only the nine rotation halfwords.  PsyCross
 * does not export that PsyQ entry point, so keep the exact bounded operation
 * local to this callback instead of adding a new global compatibility seam. */
static void m14c_transpose_rotation(u32 input, u32 output)
{
    u16 m00 = m14c_lhu(input + 0x00u);
    u16 m01 = m14c_lhu(input + 0x02u);
    u16 m02 = m14c_lhu(input + 0x04u);
    u16 m10 = m14c_lhu(input + 0x06u);
    u16 m11 = m14c_lhu(input + 0x08u);
    u16 m12 = m14c_lhu(input + 0x0Au);
    u16 m20 = m14c_lhu(input + 0x0Cu);
    u16 m21 = m14c_lhu(input + 0x0Eu);
    u16 m22 = m14c_lhu(input + 0x10u);

    m14c_sh(output + 0x00u, m00);
#if defined(W34N72_MUTANT_SKIP_MATRIX_TRANSPOSE)
    m14c_sh(output + 0x02u, m01);
    m14c_sh(output + 0x04u, m02);
    m14c_sh(output + 0x06u, m10);
    m14c_sh(output + 0x0Au, m12);
    m14c_sh(output + 0x0Cu, m20);
    m14c_sh(output + 0x0Eu, m21);
#else
    m14c_sh(output + 0x02u, m10);
    m14c_sh(output + 0x04u, m20);
    m14c_sh(output + 0x06u, m01);
    m14c_sh(output + 0x0Au, m21);
    m14c_sh(output + 0x0Cu, m02);
    m14c_sh(output + 0x0Eu, m12);
#endif
    m14c_sh(output + 0x08u, m11);
    m14c_sh(output + 0x10u, m22);
}

static s32 m14c_advance_motion(u32 slot)
{
    u32 phase = m14c_lw(slot + 0x50u);
    s32 segment = m14c_as_s32(m14c_sra(phase, 12u));

    if ((u32)segment >= 9u)
        return 1;

    switch (segment) {
    case 0:
    case 1:
        m14c_sw(slot + 0x50u, phase + m14c_lw(slot + 0x58u));
        m14c_sw(slot + 0x54u, m14c_lw(slot + 0x54u) + 4u);
        break;
    case 2:
    case 3:
    case 4: {
        u32 speed = m14c_lw(slot + 0x58u) - 8u;

        m14c_sw(slot + 0x50u, phase + m14c_lw(slot + 0x58u));
        m14c_sw(slot + 0x58u, speed);
        if (m14c_as_s32(speed) < 128) {
#if defined(W34N72_MUTANT_WRONG_LOW_SPEED_CLAMP)
            m14c_sw(slot + 0x58u, 127u);
#else
            m14c_sw(slot + 0x58u, 128u);
#endif
        }
        m14c_sw(slot + 0x54u, m14c_lw(slot + 0x54u) + 4u);
        break;
    }
    case 5:
    case 6:
    case 7: {
        u32 speed = m14c_lw(slot + 0x58u) + 8u;

        m14c_sw(slot + 0x50u, phase + m14c_lw(slot + 0x58u));
        m14c_sw(slot + 0x58u, speed);
        if (m14c_as_s32(speed) >= 257) {
#if defined(W34N72_MUTANT_WRONG_HIGH_SPEED_CLAMP)
            m14c_sw(slot + 0x58u, 257u);
#else
            m14c_sw(slot + 0x58u, 256u);
#endif
        }
        m14c_sw(slot + 0x54u, m14c_lw(slot + 0x54u) - 16u);
        break;
    }
    case 8:
        return 3;
    default:
        break;
    }
    return 1;
}

static u32 m14c_path_record(u32 phase)
{
    s32 record_index = m14c_as_s32(m14c_sra(phase, 12u));
    return WM_M14C_PATH_TABLE + ((u32)record_index << 3u);
}

s32 wm_8007BBEC(s32 slot_index)
{
    u32 slot = m14c_slot(slot_index);
    s32 result = m14c_advance_motion(slot);
    u32 phase = m14c_lw(slot + 0x50u);
    u32 record = m14c_path_record(phase);
    u32 context;
    u32 difference;
    int yaw;

#if defined(W34N72_MUTANT_IGNORE_PATH_SENTINEL)
    if (1)
#else
    if (m14c_lh(record + 0x16u) != -1)
#endif
        wm_80076858((s32)(phase & 0x0FFFu), record, record + 8u,
                     record + 16u, WM_M14C_SC_FIRST);

    phase += 128u;
    record = m14c_path_record(phase);
    wm_80076858((s32)(phase & 0x0FFFu), record, record + 8u,
                 record + 16u, WM_M14C_SC_SECOND);

    context = m14c_lw(WM_M14C_CONTEXT_PTR);
    m14c_sw(context + 0x08u, (u32)(s32)m14c_lh(WM_M14C_SC_FIRST + 2u));
    m14c_sw(context + 0x0Cu,
             m14c_sra(m14c_lw(WM_M14C_SC_FIRST + 4u), 16u));
    m14c_sw(context + 0x10u,
             m14c_sra(m14c_lw(WM_M14C_SC_FIRST + 8u), 16u));

    difference = m14c_lw(WM_M14C_SC_SECOND + 0u) -
                 m14c_lw(WM_M14C_SC_FIRST + 0u);
    m14c_sw(WM_M14C_SC_SECOND + 0u, m14c_sra(difference, 12u));
    difference = m14c_lw(WM_M14C_SC_SECOND + 4u) -
                 m14c_lw(WM_M14C_SC_FIRST + 4u);
    m14c_sw(WM_M14C_SC_SECOND + 4u, m14c_sra(difference, 12u));
    difference = m14c_lw(WM_M14C_SC_SECOND + 8u) -
                 m14c_lw(WM_M14C_SC_FIRST + 8u);
    m14c_sw(WM_M14C_SC_SECOND + 8u,
             0u - m14c_sra(difference, 12u));

    (void)VectorNormal((VECTOR *)PSX_ADDR(WM_M14C_SC_SECOND),
                       (VECTOR *)PSX_ADDR(WM_M14C_SC_FIRST));
    yaw = ratan2((int)m14c_as_s32(m14c_lw(WM_M14C_SC_FIRST + 0u)),
                 (int)m14c_as_s32(m14c_lw(WM_M14C_SC_FIRST + 8u)));
    m14c_sh(WM_M14C_SC_MARKER + 0u, 0u);
    m14c_sh(WM_M14C_SC_MARKER + 2u, (u16)((u32)yaw & 0x0FFFu));
    m14c_sh(WM_M14C_SC_MARKER + 4u, (u16)m14c_lw(slot + 0x54u));
    (void)RotMatrixYXZ((SVECTOR *)PSX_ADDR(WM_M14C_SC_MARKER),
                       (MATRIX *)PSX_ADDR(WM_M14C_SC_MATRIX));

    m14c_sh(WM_M14C_SC_MARKER + 0u, 0u);
    m14c_sh(WM_M14C_SC_MARKER + 2u, (u16)-4096);
    m14c_sh(WM_M14C_SC_MARKER + 4u, 0u);
    (void)ApplyMatrix((MATRIX *)PSX_ADDR(WM_M14C_SC_MATRIX),
                      (SVECTOR *)PSX_ADDR(WM_M14C_SC_MARKER),
                      (VECTOR *)PSX_ADDR(WM_M14C_SC_SECOND));

    OuterProduct12((VECTOR *)PSX_ADDR(WM_M14C_SC_FIRST),
                   (VECTOR *)PSX_ADDR(WM_M14C_SC_SECOND),
                   (VECTOR *)PSX_ADDR(WM_M14C_SC_CROSS));
    (void)VectorNormal((VECTOR *)PSX_ADDR(WM_M14C_SC_CROSS),
                       (VECTOR *)PSX_ADDR(WM_M14C_SC_UP));
    OuterProduct12((VECTOR *)PSX_ADDR(WM_M14C_SC_FIRST),
                   (VECTOR *)PSX_ADDR(WM_M14C_SC_UP),
                   (VECTOR *)PSX_ADDR(WM_M14C_SC_CROSS));
    (void)VectorNormal((VECTOR *)PSX_ADDR(WM_M14C_SC_CROSS),
                       (VECTOR *)PSX_ADDR(WM_M14C_SC_SECOND));

    m14c_sh(WM_M14C_SC_MATRIX + 0x00u,
             m14c_lhu(WM_M14C_SC_UP + 0u));
    m14c_sh(WM_M14C_SC_MATRIX + 0x02u,
             m14c_lhu(WM_M14C_SC_UP + 4u));
    m14c_sh(WM_M14C_SC_MATRIX + 0x04u,
             m14c_lhu(WM_M14C_SC_UP + 8u));
    m14c_sh(WM_M14C_SC_MATRIX + 0x06u,
             m14c_lhu(WM_M14C_SC_SECOND + 0u));
    m14c_sh(WM_M14C_SC_MATRIX + 0x08u,
             m14c_lhu(WM_M14C_SC_SECOND + 4u));
    m14c_sh(WM_M14C_SC_MATRIX + 0x0Au,
             m14c_lhu(WM_M14C_SC_SECOND + 8u));
    m14c_sh(WM_M14C_SC_MATRIX + 0x0Cu,
             m14c_lhu(WM_M14C_SC_FIRST + 0u));
    m14c_sh(WM_M14C_SC_MATRIX + 0x0Eu,
             m14c_lhu(WM_M14C_SC_FIRST + 4u));
    m14c_sh(WM_M14C_SC_MATRIX + 0x10u,
             m14c_lhu(WM_M14C_SC_FIRST + 8u));

    m14c_transpose_rotation(WM_M14C_SC_MATRIX, context + 0x20u);
    wm_80097070(WM_M14C_SC_MATRIX, WM_M14C_SC_ANGLES);

#if defined(W34N72_MUTANT_WRONG_MARKER_THRESHOLD)
    if (m14c_as_s32(m14c_lw(context + 0x0Cu)) <= -127)
#else
    if (m14c_as_s32(m14c_lw(context + 0x0Cu)) < -127)
#endif
        return result;

    m14c_sh(WM_M14C_SC_MARKER + 0u, (u16)m14c_lw(context + 0x08u));
    m14c_sh(WM_M14C_SC_MARKER + 2u, (u16)m14c_lw(context + 0x0Cu));
    m14c_sh(WM_M14C_SC_MARKER + 4u, (u16)m14c_lw(context + 0x10u));
    m14c_sh(WM_M14C_SC_ANGLES + 4u,
             (u16)(0u - (u32)m14c_lhu(WM_M14C_SC_ANGLES + 4u)));
    wm_80089160(18u, WM_M14C_SC_MARKER, WM_M14C_SC_ANGLES);
    return result;
}
