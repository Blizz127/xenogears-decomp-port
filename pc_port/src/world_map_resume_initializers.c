/* Retail world-map battle-return initializers. Unlike cold initialization,
 * these rebuild released objects without resetting the saved scheduler pose.
 * This translation unit belongs only to the native port build. */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "psx_memory.h"
#include "psyq/libgte.h"
#include "psyq/libgpu.h"
#include "world_map_resume_initializers.h"
#include "world_map_helper_c364.h"
#include "world_map_helper_848b4.h"
#include "world_map_terrain_sampler.h"

#define U8(a) (*(u8 *)PSX_ADDR(a))
#define U16(a) (*(u16 *)PSX_ADDR(a))
#define U32(a) (*(u32 *)PSX_ADDR(a))
extern void *func_80024524(void *, s16, s16, s16, s16, s16);
extern void func_800245D8(void *, s16);
extern void SpriteSetScale(void *, short);

static u32 slot_address(s32 index)
{
    return U32(0x8009BE24u) + ((u32)index << 7);
}

static s32 signed_word(u32 bits)
{
    s32 value;
    memcpy(&value, &bits, sizeof(value));
    return value;
}

static s32 signed_half(u16 bits)
{
    return bits < 0x8000u ? (s32)bits : (s32)bits - 0x10000;
}

static s32 resume_sprite(s32 index, u32 package_address, s16 texture_x,
                         s16 clut_x)
{
    u32 slot = slot_address(index);
    u32 package = U32(package_address);
    void *object = func_80024524(package ? PSX_ADDR(package) : NULL,
                                texture_x, 0x1E0, clut_x, 0x100, 0x40);
    /* SpriteData fields contain low native pointers in the non-PIE port. */
    if ((uintptr_t)object > UINT32_MAX)
        abort();
    U32(slot + 0x4Cu) = (u32)(uintptr_t)object;
    func_800245D8(object, 0);
    object = (void *)(uintptr_t)U32(slot + 0x4Cu);
    SpriteSetScale(object, 0x1800);
    object = (void *)(uintptr_t)U32(slot + 0x4Cu);
    *(u32 *)((u8 *)object + 0x3C) &= ~4u;
    return 1;
}

s32 wm_8008A52C(s32 index)
{
    return resume_sprite(index, 0x8009CD34u, 0x100, 0x140);
}
s32 wm_8008B498(s32 index)
{
    if (U8(0x8006F369u) == 0xFF) return 3;
    return resume_sprite(index, 0x8009CD38u, 0x110, 0x150);
}
s32 wm_8008BD1C(s32 index)
{
    if (U8(0x8006F36Au) == 0xFF) return 3;
    return resume_sprite(index, 0x8009CD3Cu, 0x120, 0x160);
}

static s32 resume_channel(s32 index, s32 channel)
{
    u32 slot = slot_address(index);
    s32 result = wm_8008C364(slot, channel);
    u32 mode = U32(0x8009BE10u);
    if (mode >= 4u && mode < 8u) U16(slot + 0x24u) = 1;
    return result;
}
s32 wm_8008C6EC(s32 index) { return resume_channel(index, 0); }
s32 wm_8008D520(s32 index) { return resume_channel(index, 1); }
s32 wm_8008DE9C(s32 index) { return resume_channel(index, 2); }

s32 wm_8008E4F4(s32 index)
{
    u32 slot = slot_address(index);
    u32 model = U32(0x8009C620u);
    u32 pose[4];
    memcpy(pose, PSX_ADDR(slot + 0x28u), sizeof(pose));
    memcpy(PSX_ADDR(model + 8u), pose, sizeof(pose));
    model = U32(0x8009C620u);
    U16(model) = U16(slot + 0x24u);
    s32 result = (U16(0x8006EE68u) & 0x1FFFu) ? 1 : 3;
    u32 mode = U32(0x8009BE10u);
    if (mode >= 1u && mode <= 3u) {
        s32 x = signed_word(U32(slot + 0x28u));
        s32 z = signed_word(U32(slot + 0x30u));
        if (mode != 1u) U16(slot + 0x20u) = 1;
        U32(slot + 0x2Cu) = (u32)wm_80093978(x, z);
    } else if (mode >= 4u && mode <= 7u) {
        model = U32(0x8009C620u);
        u16 value = mode < 6u;
        U16(model + 0xFCu) = value;
        U16(model + 0xA8u) = value;
        U16(model + 0x54u) = value;
    }
    u32 rotation = U32(slot + 0x70u);
    u32 shifted = (rotation >> 12) | ((rotation & 0x80000000u) ? 0xFFF00000u : 0u);
    model = U32(0x8009C620u);
    u16 value = (u16)(0u - shifted);
    memcpy(g_PsxScratchpad + 0xA0, &value, 2);
    u16 heading = U16(slot + 0x48u);
    value = U16(0x8009BD3Cu);
    memcpy(g_PsxScratchpad + 0xA4, &value, 2);
    memcpy(g_PsxScratchpad + 0xA2, &heading, 2);
    SVECTOR *vector = (SVECTOR *)(void *)(g_PsxScratchpad + 0xA0);
    (void)RotMatrixYXZ(vector, (MATRIX *)PSX_ADDR(model + 0x20u));
    model = U32(0x8009C620u);
    (void)RotMatrixYXZ(vector, (MATRIX *)PSX_ADDR(model + 0x74u));
    return result;
}

s32 wm_800907C4(s32 index)
{
    (void)index;
    wm_800848B4(0, 2);
    wm_800848B4(0, 3);
    return 1;
}
s32 wm_80087F60(s32 index)
{
    (void)index;
    s32 base = U16(0x8009B674u + (U32(0x8009C610u) << 1));
    for (s32 offset = -4; offset < 0; ++offset)
        wm_800848B4(base, base + offset);
    return 1;
}
s32 wm_8008868C(s32 index)
{
    (void)index;
    s32 base = U16(0x8009B688u + (U32(0x8009C610u) << 1));
    for (u32 row = 0x8009AFA0u; U16(row) != 0xFFFFu; row += 4u)
        wm_800848B4(base + signed_half(U16(row)),
                     base + signed_half(U16(row + 2u)));
    return 1;
}

/* Retail 0x80087904: mark textured packets translucent, select their page,
 * and copy the first packet buffer to the second. All record pointers here
 * are guest addresses, unlike the native SpriteData pointers above. */
static void resume_model_packets(u32 record, u32 packets, u32 count)
{
    for (u32 i = 0; i < count; ++i) {
        u16 page = GetTPage(0, 3, 0x1A0, 0xA0);
        u8 command = U8(packets + 7u);
        U16(packets + 0x16u) = page;
        U8(packets + 7u) = command | 2u;
        packets += 0x28u;
    }
    memcpy(PSX_ADDR(U32(record + 0x4Cu)),
           PSX_ADDR(U32(record + 0x48u)), count * 0x28u);
}
/* Entry for the matched C (func_800879E0 in src/world_map/main.c; 0x80087904
 * itself is not matched yet): the record arrives as a host pointer into
 * g_PsxRam (the port view of D_8009C620), the packet buffer as the guest
 * address read from the record. */
void func_80087904(void* record, s32 packets, u16 count, s32 unused)
{
    (void)unused;
    resume_model_packets(PsxMemory_GuestAddr(record), (u32)packets, count);
}

/* Entry for the matched C (func_80088570 in src/world_map/main.c;
 * 0x8008868C itself is not matched yet): the slot index is a scalar. */
s32 func_8008868C(s32 index)
{
    return wm_8008868C(index);
}

s32 wm_800879E0(s32 index)
{
    (void)index;
    u32 selector = U32(0x8009C610u);
    u32 model = U32(0x8009C620u);
    u32 first = model + U16(0x8009B64Cu + (selector << 2)) * 0x54u;
    u32 second = model + U16(0x8009B64Eu + (selector << 2)) * 0x54u;
    resume_model_packets(first, U32(first + 0x48u), U16(U32(first + 0x40u) + 4u));
    resume_model_packets(second, U32(second + 0x48u), U16(U32(second + 0x40u) + 4u));
    return 1;
}
s32 wm_80088C90(s32 index)
{
    (void)index;
    u32 model = U32(0x8009C620u);
    U16(model + 0x16A4u) = 1;
    for (u32 row = 0x8009AFDCu; U16(row) != 0xFFFFu; row += 2u)
        U16(model + (u32)signed_half(U16(row)) * 0x54u) = 1;
    return 3;
}
