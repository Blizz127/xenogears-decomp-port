/*
 * Retail certificate for func_800798BC movement gate.
 *
 * Retail (func_800798BC.s 0x800798BC-0x80079958):
 *   beqz D_800B2268 -> store 1;
 *   else andi actor+0x14, 0xC0 / bnez store 1 else store 0;
 *   lh D_800B234C and sb override when != 0xFF.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "field/actor.h"

extern void func_800798BC(void);

FieldActor* volatile g_FieldActors;
s32 g_PlayerActorIndex;
s32 D_800B2268;
s16 D_800B234C;
u8 D_80059179;

static FieldActor s_actor;
static u8 s_data[0x138];
static unsigned s_checks;

static void fail(const char* name, const char* detail)
{
    fprintf(stderr, "ASSERTION %s %s\n", name, detail);
    exit(1);
}

static void expect_eq_u8(const char* field, u8 actual, u8 expected)
{
    char detail[160];

    s_checks++;
    if (actual == expected) {
        return;
    }
    snprintf(detail, sizeof(detail), "field=%s actual=0x%x expected=0x%x",
             field, (unsigned)actual, (unsigned)expected);
    fail("movegate.store", detail);
}

static void setup(u32 flags14)
{
    uintptr_t data_addr = (uintptr_t)s_data;

    memset(&s_actor, 0, sizeof(s_actor));
    memset(s_data, 0, sizeof(s_data));
    if (data_addr > UINT32_MAX) {
        fail("fixture.address.lp32", "build must use -fno-pie -no-pie");
    }
    g_FieldActors = &s_actor;
    g_PlayerActorIndex = 0;
    s_actor.pActorData = (u32)data_addr;
    *(u32*)(s_data + 0x14) = flags14;
    D_80059179 = 0xAA;
}

int main(void)
{
    /* Gate off: store 1, then no override. */
    D_800B2268 = 0;
    D_800B234C = 0xFF;
    setup(0);
    func_800798BC();
    expect_eq_u8("gateoff", D_80059179, 1);

    /* Gate on, flags & 0xC0 == 0: store 0. */
    D_800B2268 = 1;
    D_800B234C = 0xFF;
    setup(0x20);
    func_800798BC();
    expect_eq_u8("flags.clear", D_80059179, 0);

    /* Gate on, bit 0x80: store 1. */
    D_800B2268 = 1;
    D_800B234C = 0xFF;
    setup(0x80);
    func_800798BC();
    expect_eq_u8("flags.80", D_80059179, 1);

    /* Gate on, bit 0x40: store 1. */
    D_800B2268 = 1;
    D_800B234C = 0xFF;
    setup(0x40);
    func_800798BC();
    expect_eq_u8("flags.40", D_80059179, 1);

    /* Override wins even when flags clear. */
    D_800B2268 = 1;
    D_800B234C = 0x03;
    setup(0);
    func_800798BC();
    expect_eq_u8("override", D_80059179, 3);

    printf("FIELD MOVEGATE 798BC certificate PASS checks=%u\n", s_checks);
    return 0;
}
