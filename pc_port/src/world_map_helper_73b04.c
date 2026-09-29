/*
 * World-map geometry submitter 0x80073B04.
 *
 * This is the bounded retail body only.  It submits the two static horizon
 * quads at 0x8009A300; it is intentionally not registered as a scheduler
 * callback and is not called by 0x80071A58 in the port frontier.
 */
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_helper_73b04.h"
#include "psyq/libgte.h"

#define WM_73B04_ANGLE             0x8009BD3Au
#define WM_73B04_INDEX             0x8009D7F0u
#define WM_73B04_VERTEX_BASE       0x8009A300u
#define WM_73B04_PACKET_BASE       0x8009C744u
#define WM_73B04_TWIN_A            0x8009D3D8u
#define WM_73B04_TWIN_B            0x8009D3E4u
#define WM_73B04_DB_PTR             0x8009BE3Cu
#define WM_73B04_CAMERA             0x8009C808u
#define WM_73B04_SHIFT              0x80050100u
#define WM_73B04_SCRATCH_ANGLES    0x1F800000u
#define WM_73B04_SCRATCH_ROT       0x1F800038u
#define WM_73B04_SCRATCH_COMP      0x1F800018u
#define WM_73B04_SCRATCH_P         0x1F800058u
#define WM_73B04_SCRATCH_FLAG      0x1F80005Cu

#define WM_73B04_PACKET_STRIDE     0x28u
#define WM_73B04_PACKET_BUFFER_STRIDE 0x50u
#define WM_73B04_VERTEX_STRIDE     0x20u
#define WM_73B04_POINTER_MASK      0x00FFFFFFu
#define WM_73B04_TAG_MASK          0xFF000000u

#if defined(WM_73B04_TRACE)
extern void wm_73b04_test_trace(u32 event, u32 address);
#define WM_73B04_TRACE_EVENT(event, address) wm_73b04_test_trace((event), (address))
#else
#define WM_73B04_TRACE_EVENT(event, address) ((void)0)
#endif

#if defined(WM_73B04_MUTANT_M7)
#define WM_73B04_MUTANT_POINTER_MASK 0x0000FFFFu
#else
#define WM_73B04_MUTANT_POINTER_MASK WM_73B04_POINTER_MASK
#endif

#if defined(WM_73B04_MUTANT_M8)
#define WM_73B04_MUTANT_TAG_MASK 0u
#else
#define WM_73B04_MUTANT_TAG_MASK WM_73B04_TAG_MASK
#endif

static u16 wm_73b04_load_u16(u32 address)
{
    u16 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static u32 wm_73b04_load_u32(u32 address)
{
    u32 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static void wm_73b04_store_u16(u32 address, u16 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static void wm_73b04_store_u32(u32 address, u32 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static s32 wm_73b04_bits_as_s32(u32 value)
{
    s32 result;
    memcpy(&result, &value, sizeof(result));
    return result;
}

static u32 wm_73b04_s32_as_bits(s32 value)
{
    u32 result;
    memcpy(&result, &value, sizeof(result));
    return result;
}

/* MIPS SRAV uses the low five bits of the variable shift amount.  Spell out
 * the arithmetic fill so the guest operation does not depend on C's signed
 * right-shift or signed-overflow rules. */
static s32 wm_73b04_srav(s32 value, u32 shift)
{
    u32 amount = shift & 31u;
    u32 bits = wm_73b04_s32_as_bits(value);
    u32 result;

    if (amount == 0u)
        return value;
    result = bits >> amount;
    if (value < 0)
        result |= UINT32_MAX << (32u - amount);
    return wm_73b04_bits_as_s32(result);
}

#if defined(WM_73B04_MUTANT_M22)
static s32 wm_73b04_srlv(s32 value, u32 shift)
{
    u32 amount = shift & 31u;
    u32 bits = wm_73b04_s32_as_bits(value);

    return wm_73b04_bits_as_s32(bits >> amount);
}
#endif

static u32 wm_73b04_link(u32 packet_address, u32 old_ot)
{
    u32 packet_tag = wm_73b04_load_u32(packet_address);
    u32 packet_value = (packet_tag & WM_73B04_MUTANT_TAG_MASK) |
                       (old_ot & WM_73B04_MUTANT_POINTER_MASK);

    WM_73B04_TRACE_EVENT(1u, packet_address);
    wm_73b04_store_u32(packet_address, packet_value);
    return packet_value;
}

static u32 wm_73b04_publish_ot(u32 ot_address, u32 packet_address)
{
    u32 ot_word = wm_73b04_load_u32(ot_address);
    u32 new_ot = (ot_word & WM_73B04_MUTANT_TAG_MASK) |
                 (packet_address & WM_73B04_MUTANT_POINTER_MASK);

    WM_73B04_TRACE_EVENT(2u, ot_address);
    wm_73b04_store_u32(ot_address, new_ot);
    return new_ot;
}

static u32 wm_73b04_push(u32 ot_address, u32 packet_address, u32 old_ot)
{
#if defined(WM_73B04_MUTANT_M17)
    u32 new_ot = wm_73b04_publish_ot(ot_address, packet_address);
    (void)wm_73b04_link(packet_address, old_ot);
    return new_ot;
#else
    (void)wm_73b04_link(packet_address, old_ot);
    return wm_73b04_publish_ot(ot_address, packet_address);
#endif
}

static void wm_73b04_write_uv(u32 packet_address, u16 scroll)
{
#if defined(WM_73B04_MUTANT_M14)
    u16 u = (u16)(scroll & 0x007Fu);
#else
    u16 u = (u16)((scroll >> 2) & 0x007Fu);
#endif

#if defined(WM_73B04_MUTANT_M13)
    u16 uv0 = (u16)(u << 8);
    u16 uv1 = (u16)((u | 0x0080u) << 8);
    u16 uv2 = (u16)((u | 0x3F00u) >> 8);
    u16 uv3 = (u16)((u | 0x0080u | 0x3F00u) >> 8);
#else
    u16 uv0 = u;
    u16 uv1 = (u16)(u | 0x0080u);
    u16 uv2 = (u16)(u | 0x3F00u);
    u16 uv3 = (u16)(u | 0x0080u | 0x3F00u);
#endif

    wm_73b04_store_u16(packet_address + 0x0Cu, uv0);
    wm_73b04_store_u16(packet_address + 0x14u, uv1);
    wm_73b04_store_u16(packet_address + 0x1Cu, uv2);
    wm_73b04_store_u16(packet_address + 0x24u, uv3);
}

static void wm_73b04_insert_four(u32 ot_address, u32 packet0, u32 packet1)
{
    u32 old_ot = wm_73b04_load_u32(ot_address);

#if defined(WM_73B04_MUTANT_M9)
    u32 order[4] = { WM_73B04_TWIN_B, packet1, packet0, WM_73B04_TWIN_A };
#elif defined(WM_73B04_MUTANT_M10)
    u32 order[4] = { WM_73B04_TWIN_A, WM_73B04_TWIN_B, packet0, packet1 };
#else
    u32 order[4] = { WM_73B04_TWIN_B, packet0, packet1, WM_73B04_TWIN_A };
#endif
    u32 i;

    for (i = 0u; i < 4u; i++) {
        u32 packet_address = order[i];
#if defined(WM_73B04_MUTANT_M23)
        packet_address += 4u;
#endif
#if defined(WM_73B04_MUTANT_M11)
        old_ot = wm_73b04_push(ot_address + i * 4u, packet_address, old_ot);
#else
        old_ot = wm_73b04_push(ot_address, packet_address, old_ot);
#endif
    }
}

/* W34C2: retail reads the OT depth shift from main-exe .sdata 0x80050100,
 * which the world overlay writes (=2) at 0x800847D8 inside the unported
 * function [0x80084580,0x80084818). The port's authority for that slot is the
 * host global D_80050100 in game_overrides.c, shared with the natively
 * translated primitive walkers. Reading the guest twin through PSX_ADDR
 * yields 0 before the static-data loader and only the static initial value
 * after it; the runtime writer is not ported, so the host global is the one
 * place both walkers and this helper agree on. */
extern s32 D_80050100;

s32 wm_73b04_ot_shift(void)
{
#if defined(WM_73B04_MUTANT_GUEST_SHIFT)   /* W34C2 M3: PSX_ADDR read */
    return wm_73b04_bits_as_s32(wm_73b04_load_u32(WM_73B04_SHIFT));
#else
    return D_80050100;
#endif
}

void wm_80073B04(void)
{
    SVECTOR* angles = (SVECTOR*)PSX_ADDR(WM_73B04_SCRATCH_ANGLES);
    MATRIX* rotation = (MATRIX*)PSX_ADDR(WM_73B04_SCRATCH_ROT);
    MATRIX* composite = (MATRIX*)PSX_ADDR(WM_73B04_SCRATCH_COMP);
    MATRIX* camera = (MATRIX*)PSX_ADDR(WM_73B04_CAMERA);
    u32 index = wm_73b04_load_u32(WM_73B04_INDEX);
    u16 theta = wm_73b04_load_u16(WM_73B04_ANGLE);
    u32 packet0;
    u32 packet1;
    s32 otz = 0;
#if defined(WM_73B04_MUTANT_M4)
    s32 first_otz = 0;
#endif
    u32 quad;
    u32 flag_bits;

    /* Retail writes the UV halfwords before constructing the matrices. */
    packet0 = WM_73B04_PACKET_BASE + index * WM_73B04_PACKET_STRIDE;
    packet1 = WM_73B04_PACKET_BASE + WM_73B04_PACKET_BUFFER_STRIDE +
              index * WM_73B04_PACKET_STRIDE;
    wm_73b04_write_uv(packet0, theta);
    wm_73b04_write_uv(packet1, theta);

    /* The retail addresses are SVECTOR/MATRIX objects in the PSX scratchpad.
     * The source vertex data is static image data and is never copied. */
    wm_73b04_store_u16(WM_73B04_SCRATCH_ANGLES + 0u, 0u);
    wm_73b04_store_u16(WM_73B04_SCRATCH_ANGLES + 4u, 0u);
    wm_73b04_store_u16(WM_73B04_SCRATCH_ANGLES + 2u, theta);
#if defined(WM_73B04_MUTANT_M2)
    angles->vx = (s16)theta;
    angles->vy = 0;
    (void)RotMatrixYXZ(angles, rotation);
#else
    (void)RotMatrixYXZ(angles, rotation);
#endif

    /* Retail clears R.t after RotMatrixYXZ and before CompMatrix. */
#if !defined(WM_73B04_MUTANT_M3)
    wm_73b04_store_u32(0x1F80004Cu, 0u);
    wm_73b04_store_u32(0x1F800050u, 0u);
    wm_73b04_store_u32(0x1F800054u, 0u);
#endif
#if defined(WM_73B04_MUTANT_M1)
    (void)CompMatrix(rotation, camera, composite);
#elif defined(WM_73B04_MUTANT_M18)
    (void)camera;
    (void)CompMatrix(rotation, rotation, composite);
#else
    (void)CompMatrix(camera, rotation, composite);
#endif
    SetRotMatrix(composite);
    SetTransMatrix(composite);

    for (quad = 0u; quad < 2u; quad++) {
        u32 packet = quad == 0u ? packet0 : packet1;
        u32 vertex = WM_73B04_VERTEX_BASE + quad * WM_73B04_VERTEX_STRIDE;
#if defined(WM_73B04_MUTANT_M16)
        vertex += 0x08u;
#endif
        SVECTOR* v0 = (SVECTOR*)PSX_ADDR(vertex);
        SVECTOR* v1 = (SVECTOR*)PSX_ADDR(vertex + 0x08u);
        SVECTOR* v2 = (SVECTOR*)PSX_ADDR(vertex + 0x10u);
        SVECTOR* v3 = (SVECTOR*)PSX_ADDR(vertex + 0x18u);
#if defined(WM_73B04_MUTANT_M15)
        SVECTOR* swapped = v2;
        v2 = v3;
        v3 = swapped;
#endif
#if defined(WM_73B04_MUTANT_M19)
        {
            SVECTOR* swapped = v0;
            v0 = v1;
            v1 = swapped;
        }
#endif

        long* output0 = (long*)PSX_ADDR(packet + 0x08u);
        long* output1 = (long*)PSX_ADDR(packet + 0x10u);
        long* output2 = (long*)PSX_ADDR(packet + 0x18u);
        long* output3 = (long*)PSX_ADDR(packet + 0x20u);
#if defined(WM_73B04_MUTANT_M20)
        {
            long* swapped = output0;
            output0 = output1;
            output1 = swapped;
        }
#endif

        otz = (s32)RotTransPers4(
            v0, v1, v2, v3,
            output0, output1, output2, output3,
            (long*)PSX_ADDR(WM_73B04_SCRATCH_P),
            (long*)PSX_ADDR(WM_73B04_SCRATCH_FLAG));
#if defined(WM_73B04_MUTANT_M4)
        if (quad == 0u)
            first_otz = otz;
#endif
    }

#if defined(WM_73B04_MUTANT_M4)
    otz = first_otz;
#endif

    flag_bits = wm_73b04_load_u32(WM_73B04_SCRATCH_FLAG);
#if defined(WM_73B04_MUTANT_M5)
    if (flag_bits != 0u)
        return;
#else
    if ((flag_bits & UINT32_C(0x80000000)) != 0u)
        return;
#endif

    {
        u32 db = wm_73b04_load_u32(WM_73B04_DB_PTR);
        u32 ot_base = wm_73b04_load_u32(db + 0x70u);
        s32 shift = wm_73b04_ot_shift();
        s32 bucket;
#if defined(WM_73B04_MUTANT_M6)
        (void)shift;
#endif
#if defined(WM_73B04_MUTANT_M6)
        bucket = wm_73b04_srav(otz, 2u);
#else
        bucket = wm_73b04_srav(otz, (u32)shift);
#endif
#if defined(WM_73B04_MUTANT_M21)
        if (bucket < 0)
            bucket = 0;
        else if (bucket > 0xFF)
            bucket = 0xFF;
#endif
#if defined(WM_73B04_MUTANT_M22)
        bucket = wm_73b04_srlv(-1, (u32)shift);
#endif
        u32 ot_address = ot_base + ((u32)bucket << 2);

#if defined(WM_73B04_MUTANT_M12)
        wm_73b04_insert_four(ot_address,
                             WM_73B04_PACKET_BASE + index * WM_73B04_PACKET_BUFFER_STRIDE,
                             WM_73B04_PACKET_BASE + WM_73B04_PACKET_STRIDE +
                                 index * WM_73B04_PACKET_BUFFER_STRIDE);
#else
        wm_73b04_insert_four(ot_address, packet0, packet1);
#endif
    }
}
