#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_callback_8b644.h"
#include "world_map_callback_8c844.h"
#include "world_map_callback_8d678.h"

#define POOL       0x800D0000u
#define B_SLOT     (POOL + 2u * 0x80u)
#define C_SLOT     (POOL + 4u * 0x80u)
#define D_SLOT     (POOL + 5u * 0x80u)
#define POOL_PTR   0x8009BE24u
#define RING_INDEX 0x8009D154u
#define RING_BASE  0x8009CEC4u

static u8 s_sprite_b[0xC0];
static u8 s_sprite_c[0xC0];
static u8 s_sprite_d[0xC0];
static int s_failures;
static u32 s_anim_calls;
static s16 s_last_animation;

static void check(const char* name, u32 got, u32 want)
{
    if (got != want) {
        fprintf(stderr, "ASSERTION %s: got=%u expected=%u\n", name, got,
                want);
        s_failures++;
    }
}

static void wr8(u32 address, u8 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static void wr16(u32 address, u16 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static void wr32(u32 address, u32 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static u32 sprite_bits(u8* sprite)
{
    uintptr_t bits = (uintptr_t)sprite;

    if (bits > UINT32_MAX) {
        fprintf(stderr, "ASSERTION native-backed-sprite-address: 0x%llx\n",
                (unsigned long long)bits);
        exit(1);
    }
    return (u32)bits;
}

static void set_animation(u8* sprite, s8 native_value, s8 guest_alias)
{
    u32 bits = sprite_bits(sprite);

    sprite[0xAF] = (u8)native_value;
    *(s8*)PSX_ADDR(bits + 0xAFu) = guest_alias;
}

static void reset_fixture(void)
{
    memset(g_PsxRam, 0, 2u * 1024u * 1024u);
    memset(s_sprite_b, 0, sizeof(s_sprite_b));
    memset(s_sprite_c, 0, sizeof(s_sprite_c));
    memset(s_sprite_d, 0, sizeof(s_sprite_d));
    wr32(POOL_PTR, POOL);
    s_anim_calls = 0;
    s_last_animation = -1;
}

void func_800245D8(void* object, s16 animation)
{
    (void)object;
    s_anim_calls++;
    s_last_animation = animation;
}

long rcos(long angle) { (void)angle; return 0; }
long rsin(long angle) { (void)angle; return 0; }
s32 ratan2(s32 y, s32 x) { (void)y; (void)x; return 0; }
void wm_80074794(s32 tag, u32 pose) { (void)tag; (void)pose; }
void wm_8007528C(void) {}
void wm_80089160(s32 a, s32 b, s32 c) { (void)a; (void)b; (void)c; }
void wm_800894C8(u32 index) { (void)index; }
s32 wm_8008BEC8(u32 object) { (void)object; return 0; }
void wm_8008C1DC(u32 context, u32 object, u32 output)
{
    (void)context; (void)object; (void)output;
}
s32 wm_8008DFF4(u32 a, u32 b) { (void)a; (void)b; return 0; }
s32 wm_80090C68(u32 slot) { (void)slot; return 0; }
s32 wm_80093978(s32 x, s32 z) { (void)x; (void)z; return 0; }
s32 wm_80093F18(u32 position) { (void)position; return 0; }
s32 wm_80094060(s32 mode, s32 area) { (void)mode; (void)area; return 0; }
s32 wm_800941C4(u32 a, u32 b, u32 output, u32 angle)
{
    (void)a; (void)b; (void)output; (void)angle; return 0;
}
s32 wm_80094238(u32 position, u32 index)
{
    (void)position; (void)index; return 0;
}
s32 wm_80095414(u32 position, u32 velocity, u32 output, s32 speed, s32 mode)
{
    (void)position; (void)velocity; (void)output; (void)speed; (void)mode;
    return 0;
}
s32 wm_80097770(u32 slot, s32 value) { (void)slot; (void)value; return 0; }

static void setup_b(int moving, s8 native_animation, s8 guest_animation)
{
    reset_fixture();
    wr32(B_SLOT + 0x4Cu, sprite_bits(s_sprite_b));
    wr16(B_SLOT + 0x20u, 0);
    wr16(RING_INDEX, 0);
    wr32(B_SLOT + 0x58u, 0);
    wr32(B_SLOT + 0x28u, moving ? 99u : 11u);
    wr32(B_SLOT + 0x2Cu, 22u);
    wr32(B_SLOT + 0x30u, 33u);
    wr32(RING_BASE + 0u, 11u);
    wr32(RING_BASE + 4u, 22u);
    wr32(RING_BASE + 8u, 33u);
    set_animation(s_sprite_b, native_animation, guest_animation);
}

static void setup_c(int path, s8 native_animation, s8 guest_animation)
{
    reset_fixture();
    wr32(C_SLOT + 0x4Cu, sprite_bits(s_sprite_c));
    wr16(C_SLOT + 0x20u, 0);
    if (path != 3) {
        wr8(0x8006F8E5u, 1);
        if (path == 1)
            wr32(C_SLOT + 0x38u, 1);
    }
    set_animation(s_sprite_c, native_animation, guest_animation);
}

static void setup_d(int path, s8 native_animation, s8 guest_animation)
{
    reset_fixture();
    wr32(D_SLOT + 0x4Cu, sprite_bits(s_sprite_d));
    wr16(D_SLOT + 0x20u, path == 30 ? 0x30u : 0u);
    if (path == 3) {
        wr8(0x8006F364u + 5u, 0);
    } else if (path != 30) {
        wr8(0x8006F364u + 5u, 1);
        wr16(RING_INDEX, 0);
        wr32(D_SLOT + 0x58u, 0);
        wr32(D_SLOT + 0x28u, path == 1 ? 99u : 11u);
        wr32(D_SLOT + 0x2Cu, 22u);
        wr32(D_SLOT + 0x30u, 33u);
        wr32(RING_BASE + 0u, 11u);
        wr32(RING_BASE + 4u, 22u);
        wr32(RING_BASE + 8u, 33u);
    }
    set_animation(s_sprite_d, native_animation, guest_animation);
}

static void check_no_restart(const char* name, s32 ret)
{
    check(name, s_anim_calls, 0);
    check("callback-return", (u32)ret, 1);
}

static void check_change(const char* name, s32 ret, s16 animation)
{
    check(name, s_anim_calls, 1);
    check("changed-animation", (u32)(u16)s_last_animation,
          (u32)(u16)animation);
    check("callback-return", (u32)ret, 1);
}

int main(void)
{
    PsxMemory_Init();

    setup_b(0, 0, 7);
    check_no_restart("8b644.idle.call-count", wm_8008B644(2));
    setup_b(1, 1, 7);
    check_no_restart("8b644.walk.call-count", wm_8008B644(2));
    setup_b(1, 0, 1);
    check_change("8b644.walk-change.call-count", wm_8008B644(2), 1);

    setup_c(0, 0, 7);
    check_no_restart("8c844.idle.call-count", wm_8008C844(4));
    setup_c(1, 1, 7);
    check_no_restart("8c844.walk.call-count", wm_8008C844(4));
    setup_c(3, 3, 7);
    check_no_restart("8c844.anim3.call-count", wm_8008C844(4));
    setup_c(3, 0, 3);
    check_change("8c844.anim3-change.call-count", wm_8008C844(4), 3);

    setup_d(3, 3, 7);
    check_no_restart("8d678.anim3.call-count", wm_8008D678(5));
    setup_d(0, 0, 7);
    check_no_restart("8d678.idle.call-count", wm_8008D678(5));
    setup_d(1, 1, 7);
    check_no_restart("8d678.walk.call-count", wm_8008D678(5));
    setup_d(30, 1, 7);
    check_no_restart("8d678.state30-walk.call-count", wm_8008D678(5));
    setup_d(30, 3, 7);
    check_no_restart("8d678.state30-anim3.call-count", wm_8008D678(5));
    setup_d(30, 0, 3);
    check_change("8d678.state30-change.call-count", wm_8008D678(5), 3);

    if (s_failures != 0) {
        fprintf(stderr, "W34C1 animation guard certificate FAIL failures=%d\n",
                s_failures);
        return 1;
    }
    puts("W34C1 animation guard certificate PASS");
    return 0;
}
