/* W34N51 production-linked certificate for func_80034FFC.
 *
 * Retail adds outline color only around present glyph pixels.  The old port
 * reversed that predicate on both interleaved texture pages, filling every
 * absent pixel and turning otherwise valid labels into solid blocks. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"

extern void func_80034FFC(s32 lead, s32 trail, void *work,
                          s32 stride_words, s32 page);
extern u32 D_8005934C;
extern u32 D_80059350;
extern u32 D_8005935C;
extern u32 D_80059364;

/* Retained by func_80034FFC's special-glyph branch even though these tests
 * deliberately select the ordinary single-byte glyph path. */
u16 D_800501D0[11];

enum {
    kStrideWords = 4,
    kRows = 13,
    kGuardWords = 8
};

static u16 s_glyph[11];
static u16 s_storage[kGuardWords + kStrideWords * kRows + kGuardWords];
static int s_pass;
static int s_total;
static int s_failed;

static void check(const char *name, int condition)
{
    s_total++;
    if (condition) {
        s_pass++;
        printf("PASS %s\n", name);
    } else {
        fprintf(stderr, "ASSERTION %s\n", name);
        s_failed = 1;
    }
}

static u16 *work_buffer(void)
{
    return &s_storage[kGuardWords];
}

static void reset_fixture(u16 fill)
{
    size_t i;

    memset(s_glyph, 0, sizeof(s_glyph));
    for (i = 0; i < sizeof(s_storage) / sizeof(s_storage[0]); i++)
        s_storage[i] = 0x5AA5u;
    for (i = 0; i < (size_t)(kStrideWords * kRows); i++)
        work_buffer()[i] = fill;

    D_8005934C = 0xFEu;
    D_80059350 = 0u;
    D_8005935C = (u32)(uintptr_t)s_glyph;
    D_80059364 = 0x41u;
}

static int guards_intact(void)
{
    size_t i;

    for (i = 0; i < kGuardWords; i++) {
        if (s_storage[i] != 0x5AA5u)
            return 0;
        if (s_storage[kGuardWords + kStrideWords * kRows + i] != 0x5AA5u)
            return 0;
    }
    return 1;
}

static int raster_equals(u16 value)
{
    size_t i;

    for (i = 0; i < (size_t)(kStrideWords * kRows); i++) {
        if (work_buffer()[i] != value)
            return 0;
    }
    return 1;
}

static int padding_unchanged(u16 value)
{
    size_t row;

    for (row = 0; row < kRows; row++) {
        if (work_buffer()[row * kStrideWords + 3] != value)
            return 0;
    }
    return 1;
}

static int active_bits_nonzero(u16 mask)
{
    size_t row;
    size_t column;

    for (row = 0; row < kRows; row++) {
        for (column = 0; column < 3; column++) {
            if ((work_buffer()[row * kStrideWords + column] & mask) != 0u)
                return 1;
        }
    }
    return 0;
}

static void test_empty_glyph_page0(void)
{
    u16 glyph_before[11];

    reset_fixture(0xCCCCu);
    memcpy(glyph_before, s_glyph, sizeof(glyph_before));
    func_80034FFC(0, 0x41, work_buffer(), kStrideWords, 0);

    check("row0.empty-glyph-does-not-fill-outline",
          raster_equals(0xCCCCu));
    check("row0.inactive-page-bits-preserved",
          !active_bits_nonzero(0x3333u));
    check("row0.glyph-read-only",
          memcmp(glyph_before, s_glyph, sizeof(glyph_before)) == 0);
    check("row0.write-bounds", guards_intact());
}

static void test_empty_glyph_page1(void)
{
    reset_fixture(0x3333u);
    func_80034FFC(0, 0x41, work_buffer(), kStrideWords, 1);

    check("row1.empty-glyph-does-not-fill-outline",
          raster_equals(0x3333u));
    check("row1.inactive-page-bits-preserved",
          !active_bits_nonzero(0xCCCCu));
    check("row1.write-bounds", guards_intact());
}

static void test_present_glyph_still_renders(void)
{
    reset_fixture(0u);
    s_glyph[5] = 0xFFFFu;
    func_80034FFC(0, 0x41, work_buffer(), kStrideWords, 0);
    check("row0.present-glyph-renders", active_bits_nonzero(0x3333u));
    check("row0.padding-is-not-written", padding_unchanged(0u));

    reset_fixture(0u);
    s_glyph[5] = 0xFFFFu;
    func_80034FFC(0, 0x41, work_buffer(), kStrideWords, 1);
    check("row1.present-glyph-renders", active_bits_nonzero(0xCCCCu));
    check("row1.padding-is-not-written", padding_unchanged(0u));
}

int main(void)
{
    test_empty_glyph_page0();
    test_empty_glyph_page1();
    test_present_glyph_still_renders();

    if (s_failed != 0 || s_pass != s_total)
        return EXIT_FAILURE;
    printf("W34N51 FONT OUTLINE CERTIFICATE PASS\n");
    return EXIT_SUCCESS;
}
