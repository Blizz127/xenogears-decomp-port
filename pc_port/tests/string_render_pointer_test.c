/* Real string wrapper/interpreter/rasterizer; low/high address equivalence.
 * Synthetic font data, not a visual or whole-renderer retail certificate. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include "common.h"
#include "main/game.h"

GameState g_GameState;
u8 g_ControllerButtonMappings[8], D_8005A0E4[0x400];
u16 D_800501D0[11];
s16 g_SystemPalette1, g_SystemPalette2;
extern u32 D_8005934C, D_80059350, D_80059354, D_80059358;
extern u32 D_8005935C, D_80059364;
extern void *g_SystemDataEntries;
extern s32 SystemRenderStringEntry(void *, void *, s32, s32);

enum { WORK_WORDS = 0x220, WORK_BYTES = WORK_WORDS * 2 };
static u16 *font, *low_work;
static u32 *entries;
static u8 *nested, *low_string;
static unsigned fixtures;

static void check(const u8 *input, size_t length, s32 expected_width) {
    u8 high_string[64];
    u16 high_work[0x220], expected[0x220];
    assert(length <= sizeof high_string);
    assert((uintptr_t)high_string > UINT32_MAX && (uintptr_t)high_work > UINT32_MAX);
    memcpy(low_string, input, length);
    memcpy(high_string, input, length);
    memset(low_work, 0, WORK_BYTES);
    low_work[0] = low_work[0x21f] = 0xa55a;
    s32 width = SystemRenderStringEntry(low_string, low_work + 1, 0x24, 0);
    assert(width == expected_width);
    memcpy(expected, low_work, sizeof expected);
    for (unsigned mode = 1; mode < 4; ++mode) {
        u16 *work = mode & 2 ? high_work : low_work;
        memset(work, 0, WORK_BYTES);
        work[0] = work[0x21f] = 0xa55a;
        assert(SystemRenderStringEntry(mode & 1 ? high_string : low_string,
                                       work + 1, 0x24, 0) == width);
        assert(memcmp(work, expected, sizeof expected) == 0);
        assert(work[0] == 0xa55a && work[0x21f] == 0xa55a);
        ++fixtures;
    }
}

int main(void) {
    u8 *low = mmap(NULL, 0x5000, PROT_READ | PROT_WRITE,
                   MAP_PRIVATE | MAP_ANONYMOUS | MAP_32BIT, -1, 0);
    assert(low != MAP_FAILED && (uintptr_t)low + 0x5000 <= UINT32_MAX);
    font = (u16 *)low; entries = (u32 *)(low + 0x2000);
    nested = low + 0x2100; low_string = low + 0x2200;
    low_work = (u16 *)(low + 0x2300);
#ifdef EXPECT_HIGH_DATA
    assert((uintptr_t)&g_GameState > UINT32_MAX);
#endif
    for (unsigned i = 0; i < 0x1000; ++i)
        font[i] = (u16)(0x1357u * (i + 1));
    D_8005934C = 0x80; D_80059364 = 0x20; D_80059354 = 0x60;
    D_80059358 = 0x80; D_80059350 = 0x1000;
    D_8005935C = (u32)(uintptr_t)font;
    g_SystemDataEntries = entries;
    entries[0] = (u32)(uintptr_t)nested;
    nested[4] = 8; nested[8] = 0x42; nested[9] = 0;
    for (unsigned count = 0; count <= 10; ++count) {
        for (unsigned wide = 0; wide < 2; ++wide) {
            u8 script[64]; unsigned n = 0;
            for (unsigned i = 0; i < count; ++i) {
                if (wide) script[n++] = 0x80;
                script[n++] = 0x40 + i;
            }
            script[n++] = 0;
            check(script, n, count * 8);
        }
    }
    const u8 push_pop[] = {0x41, 0xf, 3, 0, 0, 0x43, 0};
    check(push_pop, sizeof push_pop, 24);
    const u8 flat[] = {0x41, 0x42, 0x43, 0};
    u16 nested_pixels[0x220];
    memcpy(nested_pixels, low_work, WORK_BYTES);
    check(flat, sizeof flat, 24);
    assert(memcmp(nested_pixels, low_work, WORK_BYTES) == 0);
    ((u8 *)&g_GameState)[0] = 0x42;
    ((u8 *)&g_GameState)[1] = 0;
    const u8 push_name[] = {0x41, 0xf, 5, 0, 0x43, 0};
    check(push_name, sizeof push_name, 24);
    assert(memcmp(nested_pixels, low_work, WORK_BYTES) == 0);
    for (unsigned code = 1; code <= 3; ++code) {
        const u8 control[] = {0x41, (u8)code, 0x42, 0};
        check(control, sizeof control, 8);
    }
    printf("PASS %u high-string/work raster equivalence fixtures, nested return and controls\n", fixtures);
    assert(munmap(low, 0x5000) == 0);
}
