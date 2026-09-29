/*
 * Retail certificate for func_800A06E8 (VM opcode 0x121 sprite bind).
 *
 * Retail (func_800A06E8.s 0x800A06E8-0x800A08B4): status =
 * (status & 0xF07F) | 0x200. Party path: func_80076AC0 mode 2, D_800AFD20
 * = -0xC0, status &= 0xFFDF twice, scriptFlags ( | 0x100) & ~0x80.
 * Non-party: mode 1, scriptFlags |= 1, flags |= 0x100000, AFFEC/B00C0 = 1.
 * Both paths IP += 3.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "field/actor.h"
#include "field/main.h"

extern void func_800A06E8(void);

s32 D_800AFD1C;
s16 D_800AFD20;
s32 D_800AFFEC;
int D_800B00C0;
void* g_PartyDataBuffers[3];
FieldActor g_actors[2];
FieldActor* volatile g_FieldActors = g_actors;
ActorData g_actor_data;
ActorData* g_FieldScriptVMCurActor = &g_actor_data;

static unsigned s_checks;
static int s_arg;
static int s_arg_slot;
static int s_cf3c;
static s32 s_cf3c_in;
static s32 s_party_id;
static int s_76ac0;
static s32 s_76_actor;
static s32 s_76_skin;
static void* s_76_pkg;
static s32 s_76_mode;
static s32 s_76_tex;
static s32 s_76_sel;
static s32 s_76_skip;
static int s_a0c94;

s32 FieldScriptVMGetArgument(s32 slot)
{
    s_arg++;
    s_arg_slot = slot;
    return 0x11;
}

s32 func_8008CF3C(s32 characterId)
{
    s_cf3c++;
    s_cf3c_in = characterId;
    return 7;
}

s32 FieldCharacterIdToPartyId(s32 characterId)
{
    (void)characterId;
    return s_party_id;
}

void func_80076AC0(s32 actorIndex, s32 skinIndex, void* pAnimPackage,
                   s32 spriteMode, s32 texPageOffset, s32 skinSelector,
                   s32 skipInitialTick)
{
    s_76ac0++;
    s_76_actor = actorIndex;
    s_76_skin = skinIndex;
    s_76_pkg = pAnimPackage;
    s_76_mode = spriteMode;
    s_76_tex = texPageOffset;
    s_76_sel = skinSelector;
    s_76_skip = skipInitialTick;
}

void func_800A0C94(void)
{
    s_a0c94++;
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
    fail("sprite.bind", detail);
}

static void expect_eq_ptr(const char* field, const void* actual,
                          const void* expected)
{
    char detail[160];

    s_checks++;
    if (actual == expected) {
        return;
    }
    snprintf(detail, sizeof(detail), "field=%s actual=%p expected=%p",
             field, actual, expected);
    fail("sprite.bind", detail);
}

static void reset_state(void)
{
    memset(g_actors, 0, sizeof(g_actors));
    memset(&g_actor_data, 0, sizeof(g_actor_data));
    g_actors[0].status = 0x20;
    g_actor_data.scriptFlags.flags = 0x80;
    g_actor_data.flags = 0;
    g_actor_data.scriptInstructionPointer = 10;
    D_800AFD1C = 0;
    D_800AFD20 = 0;
    D_800AFFEC = 0;
    D_800B00C0 = 0;
    g_PartyDataBuffers[0] = (void*)(uintptr_t)0x1000;
    g_PartyDataBuffers[1] = (void*)(uintptr_t)0x2000;
    g_PartyDataBuffers[2] = (void*)(uintptr_t)0x3000;
    s_arg = 0;
    s_cf3c = 0;
    s_76ac0 = 0;
    s_a0c94 = 0;
}

int main(void)
{
    /* Party slot 1. */
    reset_state();
    s_party_id = 1;
    func_800A06E8();
    expect_eq_s32("p.arg", s_arg, 1);
    expect_eq_s32("p.arg.slot", s_arg_slot, 1);
    expect_eq_s32("p.cf3c", s_cf3c, 1);
    expect_eq_s32("p.cf3c.in", s_cf3c_in, 0x11);
    expect_eq_s32("p.76ac0", s_76ac0, 1);
    expect_eq_s32("p.actor", s_76_actor, 0);
    expect_eq_s32("p.skin", s_76_skin, 1);
    expect_eq_ptr("p.pkg", s_76_pkg, g_PartyDataBuffers[1]);
    expect_eq_s32("p.mode", s_76_mode, 2);
    expect_eq_s32("p.tex", s_76_tex, 0);
    expect_eq_s32("p.sel", s_76_sel, 1);
    expect_eq_s32("p.skip", s_76_skip, 1);
    expect_eq_s32("p.afd20", D_800AFD20, -0xC0);
    expect_eq_s32("p.a0c94", s_a0c94, 1);
    expect_eq_s32("p.status", g_actors[0].status, 0x200);
    expect_eq_s32("p.sflags", (s32)g_actor_data.scriptFlags.flags, 0x100);
    expect_eq_s32("p.flags", (s32)g_actor_data.flags, 0);
    expect_eq_s32("p.affec", D_800AFFEC, 0);
    expect_eq_s32("p.b00c0", D_800B00C0, 0);
    expect_eq_s32("p.ip", g_actor_data.scriptInstructionPointer, 13);

    /* Non-party. */
    reset_state();
    s_party_id = -1;
    g_actors[0].status = 0x20;
    func_800A06E8();
    expect_eq_s32("n.76ac0", s_76ac0, 1);
    expect_eq_s32("n.skin", s_76_skin, 0);
    expect_eq_ptr("n.pkg", s_76_pkg, g_PartyDataBuffers[0]);
    expect_eq_s32("n.mode", s_76_mode, 1);
    expect_eq_s32("n.sel", s_76_sel, 0);
    expect_eq_s32("n.a0c94", s_a0c94, 0);
    expect_eq_s32("n.afd20", D_800AFD20, 0);
    expect_eq_s32("n.status", g_actors[0].status, 0x220);
    expect_eq_s32("n.sflags", (s32)g_actor_data.scriptFlags.flags, 0x81);
    expect_eq_s32("n.flags", (s32)g_actor_data.flags, 0x100000);
    expect_eq_s32("n.affec", D_800AFFEC, 1);
    expect_eq_s32("n.b00c0", D_800B00C0, 1);
    expect_eq_s32("n.ip", g_actor_data.scriptInstructionPointer, 13);

    printf("FIELD SPRITE BIND A06E8 certificate PASS checks=%u\n", s_checks);
    return 0;
}
