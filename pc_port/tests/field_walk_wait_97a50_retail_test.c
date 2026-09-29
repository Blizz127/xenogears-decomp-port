/*
 * Retail certificate for func_80097A50 (3D walk-wait tick).
 *
 * Retail (func_80097A50.s 0x80097A50-0x80098034): mode = bits 23-24 of
 * actor+0x90+curScriptIndex*8; countdown is that word's low half. Sprite
 * quantum is 0x4000000/speed unless ref flags&0x2000. move.vy is stored
 * then forced 0. Arrival masks 0xFE7FFFFF and writes countdown 0xFFFF.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "field/actor.h"

extern s32 func_80097A50(s32 targetValue);

#undef rsin
#undef rcos
int rsin(int a)
{
    (void)a;
    return 0x1000;
}

int rcos(int a)
{
    (void)a;
    return 0x1000;
}

s32 D_800AFD1C;
FieldActor* volatile g_FieldActors;
ActorData* g_FieldScriptVMCurActor;
void* g_FieldScriptVMCurScriptData;
int D_800B00C0;

static FieldActor s_fa[2];
static ActorData s_actor;
static ActorData s_other;
static ActorData s_ref;
static u8 s_sprite[0x40];
static u8 s_script[0x20];
static unsigned s_checks;
static int s_arg1;
static int s_arg2;
static int s_arg3;
static int s_get_actor;
static int s_get_actor_calls;
static int s_vec1;
static int s_vec3;
static int s_b694;

int FieldScriptArgument1(int index, int mask)
{
    (void)index;
    (void)mask;
    return s_arg1;
}

int FieldScriptArgument2(int index, int mask)
{
    (void)index;
    (void)mask;
    return s_arg2;
}

int FieldScriptArgument3(int index, int mask)
{
    (void)index;
    (void)mask;
    return s_arg3;
}

u32 FieldScriptVMGetActorIndex(int bytecodeOffset)
{
    (void)bytecodeOffset;
    s_get_actor_calls++;
    return (u32)s_get_actor;
}

int FieldScriptVMGetArgument(int index)
{
    (void)index;
    return 0x100;
}

long FieldGetVec1Magnitude(long x)
{
    (void)x;
    return s_vec1;
}

long FieldGetVec3Magnitude(long x, long y, long z)
{
    (void)x;
    (void)y;
    (void)z;
    return s_vec3;
}

long VectorNormal(VECTOR* v0, VECTOR* v1)
{
    (void)v0;
    v1->vx = 0x1000;
    v1->vy = 0x0800;
    v1->vz = 0;
    return 0;
}

s32 func_8007B694(s32* arg0)
{
    (void)arg0;
    s_b694++;
    return 0x123;
}

static void fail(const char* name, const char* detail)
{
    fprintf(stderr, "ASSERTION %s %s\n", name, detail);
    exit(1);
}

static void expect_eq_s32(const char* field, s32 actual, s32 expected)
{
    char detail[160];

    s_checks++;
    if (actual == expected) {
        return;
    }
    snprintf(detail, sizeof(detail), "field=%s actual=%d expected=%d",
             field, (int)actual, (int)expected);
    fail("walk.wait", detail);
}

static void reset_state(void)
{
    memset(&s_actor, 0, sizeof(s_actor));
    memset(&s_other, 0, sizeof(s_other));
    memset(&s_ref, 0, sizeof(s_ref));
    memset(s_fa, 0, sizeof(s_fa));
    memset(s_sprite, 0, sizeof(s_sprite));
    memset(s_script, 0, sizeof(s_script));
    s_fa[0].pActorData = (u32)(uintptr_t)&s_ref;
    s_fa[0].pSpriteData = (u32)(uintptr_t)s_sprite;
    s_fa[1].pActorData = (u32)(uintptr_t)&s_other;
    g_FieldActors = s_fa;
    D_800AFD1C = 0;
    g_FieldScriptVMCurActor = &s_actor;
    g_FieldScriptVMCurScriptData = s_script;
    D_800B00C0 = 0;
    s_actor.moveSpeed = 0x100;
    s_actor.curScriptIndex = 0;
    s_arg1 = 0x20;
    s_arg2 = 0x30;
    s_arg3 = 0x40;
    s_get_actor = 1;
    s_get_actor_calls = 0;
    s_vec1 = 2;
    s_vec3 = 0x100;
    s_b694 = 0;
}

static u32* flags_word(void)
{
    return (u32*)((u8*)&s_actor + 0x90);
}

int main(void)
{
    s32 rc;

    /* Arrival: countdown 0, mode 0. */
    reset_state();
    *flags_word() = (0u << 23);
    rc = func_80097A50(-1);
    expect_eq_s32("arrive.rc", rc, 0);
    expect_eq_s32("arrive.count", (s32)(*(u16*)flags_word()), 0xFFFF);
    expect_eq_s32("arrive.mask", (s32)((*flags_word() >> 24) & 1), 0);
    expect_eq_s32("arrive.move.vy", s_actor.move.vy, 0);
    expect_eq_s32("arrive.flag40", (s32)(s_actor.scriptFlags.flags & 0x400000),
                  0x400000);
    expect_eq_s32("arrive.quantum", *(s32*)(s_sprite + 0x18), 0x04000000 / 0x100);

    /* Continue: countdown 5, far distance. */
    reset_state();
    *flags_word() = (0u << 23) | 5;
    s_vec3 = 0x100;
    s_vec1 = 2;
    rc = func_80097A50(-1);
    expect_eq_s32("walk.rc", rc, -1);
    expect_eq_s32("walk.count", (s32)(*(u16*)flags_word()), 4);
    expect_eq_s32("walk.busy", D_800B00C0, 1);
    expect_eq_s32("walk.move.vy", s_actor.move.vy, 0);
    expect_eq_s32("walk.flag4", (s32)(s_actor.scriptFlags.flags & 0x40000),
                  0x40000);
    expect_eq_s32("walk.rot", (s32)(u16)s_actor.rotation.vy, 0x8123);

    /* flags&0x2000 uses 0x8000000 quantum. */
    reset_state();
    s_ref.flags = 0x2000;
    *flags_word() = 0;
    func_80097A50(0);
    expect_eq_s32("gear.quantum", *(s32*)(s_sprite + 0x18), 0x08000000 / 0x100);

    /* Mode 1 adds unkD0. */
    reset_state();
    *flags_word() = (1u << 23);
    s_actor.unkD0.vx = 10;
    s_actor.unkD0.vy = 20;
    s_actor.unkD0.vz = 30;
    s_vec3 = 0;
    func_80097A50(0);
    expect_eq_s32("mode1.done", (s32)(*(u16*)flags_word()), 0xFFFF);

    /* Mode 2: 0xFF actor index returns 0 without countdown write. */
    reset_state();
    *flags_word() = (2u << 23) | 7;
    s_get_actor = 0xFF;
    rc = func_80097A50(-1);
    expect_eq_s32("mode2.ff.rc", rc, 0);
    expect_eq_s32("mode2.ff.count", (s32)(*(u16*)flags_word()), 7);
    expect_eq_s32("mode2.ff.calls", s_get_actor_calls, 1);

    /* Mode 3: rsin/rcos path still zeros move.vy. */
    reset_state();
    *flags_word() = (3u << 23);
    s_vec3 = 0;
    func_80097A50(0);
    expect_eq_s32("mode3.move.vy", s_actor.move.vy, 0);
    expect_eq_s32("mode3.done", (s32)(*(u16*)flags_word()), 0xFFFF);

    printf("FIELD WALK WAIT 97A50 certificate PASS checks=%u\n", s_checks);
    return 0;
}
