/* Retail certificate for SoundMuteAllSpuChannels (0x80037EE4-0x80037F44). */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"

extern void SoundMuteAllSpuChannels(void);

static u8 s_regs[0x18 * 0x10];
/* Both globals are defined by sound.c; the test only points the register base
 * at its own scratch buffer. */
extern u8* g_pSoundSpuRegisters;   /* defined by sound.c */
u16 g_SoundControlFlags;            /* referenced by sound.c, defined here */

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
        fail("sound.mute", detail);
    }
}

int main(void)
{
    int v;

    g_pSoundSpuRegisters = s_regs;
    memset(s_regs, 0x5A, sizeof(s_regs));
    g_SoundControlFlags = 0x0001;

    SoundMuteAllSpuChannels();

    expect_eq_s32("flags", g_SoundControlFlags, 0x0041);

    for (v = 0; v < 0x18; v++) {
        u8* p = s_regs + v * 0x10;
        if (v == 0) {
            expect_eq_s32("v0.00", *(u16*)(p + 0x00), 0);
            expect_eq_s32("v0.02", *(u16*)(p + 0x02), 0);
            expect_eq_s32("v0.04", *(u16*)(p + 0x04), 0);
            expect_eq_s32("v0.0a", *(u16*)(p + 0x0A), 0x1FDF);
            /* low byte 0x5A kept, high byte forced to 0x7F */
            expect_eq_s32("v0.08", *(u16*)(p + 0x08), 0x7F5A);
        }
        if (v == 0x17) {
            expect_eq_s32("v23.00", *(u16*)(p + 0x00), 0);
            expect_eq_s32("v23.0a", *(u16*)(p + 0x0A), 0x1FDF);
            expect_eq_s32("v23.08", *(u16*)(p + 0x08), 0x7F5A);
        }
    }
    /* untouched fields keep their fill and no voice past 23 is written */
    expect_eq_s32("v0.06", *(u16*)(s_regs + 0x06), 0x5A5A);
    expect_eq_s32("v0.0c", *(u16*)(s_regs + 0x0C), 0x5A5A);

    printf("SOUND MUTE ALL certificate PASS checks=%u\n", s_checks);
    return 0;
}
