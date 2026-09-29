/* Retail certificate for func_80026338 (0x80026338-0x800263E4).
 * Record getter: `index` picks a u16 offset out of `base`; the record supplies a
 * signed first field, a u16 at +4 shifted right by 2 (flags non-zero) or 4, three
 * more signed fields, and two masked sums. All six land in the caller's outs. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"

extern void func_80026338(u8* base, s32 index, s32* pOut0, s32* pOut1, s32* pOut2,
                          s32* pOut3, s32* pOut4, s32* pOut5);

static u8 s_base[0x80];

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
    if (actual != expected) {
        snprintf(detail, sizeof(detail), "field=%s actual=%d expected=%d",
                 field, (int)actual, (int)expected);
        fail("temp1.record", detail);
    }
}

/* base layout: u16 offset table at +0, record at +0x20 */
static void build_record(s16 flags, u16 packed, s16 f0, s16 f1, s16 f2, s16 maskedA, s16 maskedB, s16 extra)
{
    memset(s_base, 0, sizeof(s_base));
    /* entry for index 1: base + 1*2, and the record offset is its u16 [2] => base + 6 */
    *(u16*)(s_base + 6) = 0x20;
    *(s16*)(s_base + 0x20 + 0) = f0;
    *(u16*)(s_base + 0x20 + 4) = packed;
    *(s16*)(s_base + 0x20 + 6) = extra;
    *(s16*)(s_base + 0x20 + 0x14) = flags;
    *(s16*)(s_base + 0x20 + 0x16) = f1;
    *(s16*)(s_base + 0x20 + 0x18) = f2;
    *(u16*)(s_base + 0x20 + 0x1A) = (u16)maskedA;
    *(u16*)(s_base + 0x20 + 0x1C) = (u16)maskedB;
}

int main(void)
{
    s32 o0, o1, o2, o3, o4, o5;

    /* flags != 0 -> (s16)packed >> 2 */
    build_record(1, 0x0040, -5, 7, 9, 0x1234, 0x5678, 3);
    func_80026338(s_base, 1, &o0, &o1, &o2, &o3, &o4, &o5);
    expect_eq_s32("flags.0", o0, -5);
    expect_eq_s32("flags.1", o1, 1);
    expect_eq_s32("flags.2", o2, 7);
    expect_eq_s32("flags.3", o3, 9);
    /* 0x40 >> 2 = 0x10; (0x1234 & 0xFFC0) = 0x1200 -> 0x1200 + 0x10 */
    expect_eq_s32("flags.4", o4, 0x1210);
    /* (0x5678 & 0xFF00) = 0x5600 -> 0x5600 + 3 */
    expect_eq_s32("flags.5", o5, 0x5603);

    /* flags == 0 -> (s16)packed >> 4, and a negative packed value sign-extends */
    build_record(0, (u16)-0x40, 11, -12, -13, 0x00C0, 0xFF00, -7);
    func_80026338(s_base, 1, &o0, &o1, &o2, &o3, &o4, &o5);
    expect_eq_s32("zero.0", o0, 11);
    expect_eq_s32("zero.1", o1, 0);
    expect_eq_s32("zero.2", o2, -12);
    expect_eq_s32("zero.3", o3, -13);
    /* (s16)-0x40 = -64; -64 >> 4 = -4; (0x00C0 & 0xFFC0) = 0xC0 -> 0xBC */
    expect_eq_s32("zero.4", o4, 0xBC);
    /* (0xFF00 & 0xFF00) = 0xFF00 as s16 = -256; -256 + (-7) = -263 */
    expect_eq_s32("zero.5", o5, -263);

    printf("TEMP1 RECORD GETTER 26338 certificate PASS checks=%u\n", s_checks);
    return 0;
}
