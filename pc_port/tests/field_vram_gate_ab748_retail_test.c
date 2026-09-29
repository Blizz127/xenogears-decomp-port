/*
 * Retail certificate for func_800AB748 (VRAM-restore gate).
 *
 * Retail (func_800AB748.s 0x800AB748-0x800AB804, jtbl_8006FDD8):
 *   sltiu slot, 5; cases 0-3 andi 0x8/0x10/0x20/0x40 then bit?0:-1;
 *   case 4 andi 0x80 inverts (bit?-1:0); slot>=5 returns -1.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"

extern s32 func_800AB748(s32 slot);

void* g_pGameState;

static u8 s_state[0x1A20];
static unsigned s_checks;

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
    fail("vram.gate", detail);
}

static void set_flags(u16 flags)
{
    memset(s_state, 0, sizeof(s_state));
    *(u16*)(s_state + 0x1A16) = flags;
    g_pGameState = s_state;
}

int main(void)
{
    uintptr_t addr = (uintptr_t)s_state;

    if (addr > UINT32_MAX) {
        fail("fixture.address.lp32", "build must use -fno-pie -no-pie");
    }

    set_flags(0);
    expect_eq_s32("s0.clear", func_800AB748(0), -1);
    expect_eq_s32("s1.clear", func_800AB748(1), -1);
    expect_eq_s32("s2.clear", func_800AB748(2), -1);
    expect_eq_s32("s3.clear", func_800AB748(3), -1);
    expect_eq_s32("s4.clear", func_800AB748(4), 0);
    expect_eq_s32("oob", func_800AB748(5), -1);
    expect_eq_s32("neg", func_800AB748(-1), -1);

    set_flags(0x8 | 0x10 | 0x20 | 0x40 | 0x80);
    expect_eq_s32("s0.set", func_800AB748(0), 0);
    expect_eq_s32("s1.set", func_800AB748(1), 0);
    expect_eq_s32("s2.set", func_800AB748(2), 0);
    expect_eq_s32("s3.set", func_800AB748(3), 0);
    expect_eq_s32("s4.set", func_800AB748(4), -1);

    set_flags(0x80);
    expect_eq_s32("s0.only80", func_800AB748(0), -1);
    expect_eq_s32("s4.only80", func_800AB748(4), -1);

    printf("FIELD VRAM GATE AB748 certificate PASS checks=%u\n", s_checks);
    return 0;
}
