/*
 * Retail certificate for func_80080A74 init stores.
 *
 * First 0x12C mask is addiu -0x21 (clear bit 5), not ~0x30000.
 * 0x130 also takes $a3 = 0xFFF801FF. Fail path zeros the 4th-arg pOut.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "field/actor.h"
#include "psyq/libgte.h"

extern void func_80080A74(s32 actorIndex);

FieldActor* volatile g_FieldActors;
s16 D_800AFB54;
s32 D_800AFB44[4];
s32 D_800AFB24[4];
s32 D_800AFB20;

static FieldActor s_actor;
static u8 s_data[0x138];
static u8 s_row[64];
static u32 s_lookup[4];
static s16* s_pout_ptr;
static unsigned s_checks;
static int s_clip_ret = 0;
static s16 s_pout_seed[3];
static s32 s_pstate_seed[3];

int rand(void)
{
    return 0x1234;
}

s16 func_8007B1C4(s16 posX, s16 posZ, s32 idx, s16* pOut, s16* pState)
{
    (void)posX;
    (void)posZ;
    (void)idx;
    pOut[0] = s_pout_seed[0];
    pOut[1] = s_pout_seed[1];
    pOut[2] = s_pout_seed[2];
    memcpy(pState, s_pstate_seed, sizeof(s_pstate_seed));
    s_pout_ptr = pOut;
    return (s16)s_clip_ret;
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
    fail("actor.init.store", detail);
}

static void setup_actor(u16 status)
{
    uintptr_t data_addr;
    uintptr_t row_addr;
    uintptr_t lookup_addr;

    memset(&s_actor, 0, sizeof(s_actor));
    memset(s_data, 0xAA, sizeof(s_data));
    memset(s_row, 0, sizeof(s_row));
    s_lookup[0] = 0x11111111u;
    s_row[0x0C] = 0;

    data_addr = (uintptr_t)s_data;
    row_addr = (uintptr_t)s_row;
    lookup_addr = (uintptr_t)s_lookup;
    if (data_addr > UINT32_MAX || row_addr > UINT32_MAX ||
        lookup_addr > UINT32_MAX) {
        fail("fixture.address.lp32", "build must use -fno-pie -no-pie");
    }

    g_FieldActors = &s_actor;
    s_actor.pActorData = (u32)data_addr;
    *(s32*)((u8*)&s_actor + 0x20) = 0x10;
    *(s32*)((u8*)&s_actor + 0x24) = 0x20;
    *(s32*)((u8*)&s_actor + 0x28) = 0x30;
    s_actor.status = (s16)status;

    *(s32*)(s_data + 0x12C) = -1;
    *(s32*)(s_data + 0x130) = -1;
    *(s32*)(s_data + 0x134) = -1;

    D_800AFB24[0] = (s32)(u32)row_addr;
    D_800AFB20 = (s32)(u32)lookup_addr;
}

int main(void)
{
    /* In-range B1C4: keep pOut, copy FieldActor+0x24 from stateBuf[0x21]. */
    D_800AFB54 = 2;
    D_800AFB44[0] = 1;
    s_clip_ret = 0;
    s_pout_seed[0] = 0x1111;
    s_pout_seed[1] = 0x1234;
    s_pout_seed[2] = 0x3333;
    s_pstate_seed[0] = 0x01020304;
    s_pstate_seed[1] = 0x05060708;
    s_pstate_seed[2] = 0x090A0B0C;
    setup_actor(0);
    func_80080A74(0);

    expect_eq_s32("hdr0", *(s32*)(s_data + 0x00), 0xB0);
    expect_eq_s32("hdr4", *(s32*)(s_data + 0x04), 0x800);
    expect_eq_s32("tri", *(s16*)(s_data + 0x08), 0);
    expect_eq_s32("lookup", *(s32*)(s_data + 0x14), 0x11111111);
    expect_eq_s32("rand", *(s16*)(s_data + 0x102), 0x1234);
    expect_eq_s32("f12c", *(s32*)(s_data + 0x12C), (s32)0xF000E000);
    expect_eq_s32("f130", *(s32*)(s_data + 0x130), (s32)0xF0000000);
    expect_eq_s32("state0", *(s32*)(s_data + 0x50), 0x01020304);
    expect_eq_s32("state1", *(s32*)(s_data + 0x54), 0x05060708);
    expect_eq_s32("state2", *(s32*)(s_data + 0x58), 0x090A0B0C);
    expect_eq_s32("fa24", *(s32*)((u8*)&s_actor + 0x24), 0x1234);
    expect_eq_s32("ad20", *(s32*)(s_data + 0x20), 0x10 << 16);
    expect_eq_s32("ad24", *(s32*)(s_data + 0x24), 0x1234 << 16);
    expect_eq_s32("ad72", *(s16*)(s_data + 0x72), 0x1234);
    expect_eq_s32("pout1", s_pout_ptr[1], 0x1234);

    /* Out-of-range B1C4: fail path zeros pOut (sh 0x40/42/44($s2)). */
    D_800AFB44[0] = 1;
    s_clip_ret = 5;
    s_pout_seed[0] = 0x1111;
    s_pout_seed[1] = 0x2222;
    s_pout_seed[2] = 0x3333;
    setup_actor(0);
    func_80080A74(0);
    expect_eq_s32("fail.pout0", s_pout_ptr[0], 0);
    expect_eq_s32("fail.pout1", s_pout_ptr[1], 0);
    expect_eq_s32("fail.pout2", s_pout_ptr[2], 0);
    expect_eq_s32("fail.count", D_800AFB44[0], 0);
    expect_eq_s32("fail.fa24", *(s32*)((u8*)&s_actor + 0x24), 0);

    printf("FIELD ACTOR INIT 80A74 certificate PASS checks=%u\n", s_checks);
    return 0;
}
