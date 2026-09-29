/*
 * Retail certificate for func_80098430 (opcode 0x4B).
 *
 * Retail (func_80098430.s 0x80098430-0x800984E8):
 *   lw 0x90(actor+idx*8); and 0xFE7FFFFF; sw;
 *   lhu 0x90 == 0xFFFF -> FieldScriptVMGetArgument(6), sh back after
 *     reloading g_FieldScriptVMCurActor;
 *   always GetArgument(6) then func_80099AC0; on 0, lhu 0xCC += 8.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "field/actor.h"

extern void func_80098430(void);

ActorData* g_FieldScriptVMCurActor;

static u8 s_actor[0x138];
static unsigned s_checks;
static int s_arg_calls;
static int s_arg_last;
static int s_arg_ret = 0x1234;
static int s_wait_ret;
static int s_wait_arg = -1;

int FieldScriptVMGetArgument(int index)
{
    s_arg_calls++;
    s_arg_last = index;
    return s_arg_ret;
}

s32 func_80099AC0(s32 useStoredAngle)
{
    s_wait_arg = useStoredAngle;
    return s_wait_ret;
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
    snprintf(detail, sizeof(detail), "field=%s actual=0x%x expected=0x%x",
             field, (unsigned)actual, (unsigned)expected);
    fail("opcode.98430", detail);
}

static void setup(u8 idx, u32 slot_word, u16 ip)
{
    memset(s_actor, 0, sizeof(s_actor));
    s_actor[0xCE] = idx;
    *(u32*)(s_actor + 0x90 + idx * 8) = slot_word;
    *(u16*)(s_actor + 0xCC) = ip;
    g_FieldScriptVMCurActor = (ActorData*)s_actor;
    s_arg_calls = 0;
    s_arg_last = -1;
    s_wait_arg = -1;
}

int main(void)
{
    uintptr_t actor_addr = (uintptr_t)s_actor;

    if (actor_addr > UINT32_MAX) {
        fail("fixture.address.lp32", "build must use -fno-pie -no-pie");
    }

    /* FFFF low half: mask, seed from arg 6, wait 0 -> IP += 8. */
    s_arg_ret = 0x1234;
    s_wait_ret = 0;
    setup(0, 0xFFFFFFFFu, 0x10);
    func_80098430();
    expect_eq_s32("mask.word", *(s32*)(s_actor + 0x90), (s32)0xFE7F1234);
    expect_eq_s32("arg.calls", s_arg_calls, 2);
    expect_eq_s32("arg.idx", s_arg_last, 6);
    expect_eq_s32("wait.arg", s_wait_arg, 0x1234);
    expect_eq_s32("ip.inc", *(u16*)(s_actor + 0xCC), 0x18);

    /* Non-FFFF: mask only, one GetArgument, wait 0 still bumps IP. */
    s_arg_ret = 0x20;
    s_wait_ret = 0;
    setup(0, 0xFE7F0001u, 0x30);
    func_80098430();
    expect_eq_s32("keep.word", *(s32*)(s_actor + 0x90), (s32)0xFE7F0001);
    expect_eq_s32("keep.calls", s_arg_calls, 1);
    expect_eq_s32("keep.ip", *(u16*)(s_actor + 0xCC), 0x38);

    /* Wait busy: IP unchanged. */
    s_arg_ret = 0x20;
    s_wait_ret = 1;
    setup(0, 0xFE7F0001u, 0x40);
    func_80098430();
    expect_eq_s32("busy.ip", *(u16*)(s_actor + 0xCC), 0x40);

    /* Slot 1 at +0x98. */
    s_arg_ret = 0xAABB;
    s_wait_ret = 0;
    setup(1, 0xFFFFFFFFu, 0);
    func_80098430();
    expect_eq_s32("slot1.word", *(s32*)(s_actor + 0x98), (s32)0xFE7FAABB);
    expect_eq_s32("slot1.slot0", *(s32*)(s_actor + 0x90), 0);

    printf("FIELD OPCODE 98430 certificate PASS checks=%u\n", s_checks);
    return 0;
}
