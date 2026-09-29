/*
 * Retail certificate for func_8003D0E8 (sound.c seq cmd 0x90): set the manager
 * voice volume immediately.
 *
 * Retail (asm/slus_006.64/nonmatchings/system/sound/func_8003D0E8.s, file
 * offset 0x2D8E8, 40 bytes) is:
 *   vol = *(volatile u8*)pScript;          // lbu + redundant andi 0xFF
 *   AM->unk_0x54 = vol * (s16)(AM+0x66);   // mult/mflo, low word
 *   AM->unk_0x58 = vol << 16;              // volume in the high half
 *   return pScript + 1;
 *
 * The shipped function in the TU is the matching C body (byte-exact with the
 * pinned gcc-2.6.0 + maspsx; the volatile lvalue is what keeps the redundant
 * `andi $v0,$v0,0xFF` separate from the lbu). This test drives it directly, so
 * a wrong scale, shift or return pointer is rejected.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"

extern u8* func_8003D0E8(u8* pScript, void* pAudioManager, void* pAudioElements);

static u8 s_am[0x100];
static u8 s_script[0x10];
static unsigned int s_checks;

static void expect(const char* what, unsigned vol, int iv,
                   long actual, long expected)
{
    s_checks++;
    if (actual != expected) {
        fprintf(stderr,
                "ASSERTION d0e8.result %s vol=%u iv=%d actual=0x%lx expected=0x%lx\n",
                what, vol, iv, (unsigned long)actual,
                (unsigned long)expected);
        exit(1);
    }
}

static void run_case(unsigned vol, int iv)
{
    u8* ret;
    unsigned i;

    memset(s_am, 0x33, sizeof(s_am));
    memset(s_script, 0x44, sizeof(s_script));
    s_script[0] = (u8)vol;
    *(s16*)(s_am + 0x66) = (s16)iv;

    ret = func_8003D0E8(s_script, s_am, NULL);

    expect("return", vol, iv, ret == s_script + 1, 1);
    expect("unk_0x54", vol, iv, (long)*(s32*)(s_am + 0x54),
           (long)((s32)vol * (s32)(s16)iv));
    expect("unk_0x58", vol, iv, (long)*(s32*)(s_am + 0x58),
           (long)((s32)vol << 16));

    /* Nothing else in the manager may change. */
    for (i = 0; i < sizeof(s_am); i++) {
        if ((i >= 0x54 && i < 0x5C) || i == 0x66 || i == 0x67) {
            continue;
        }
        expect("am-untouched", vol, iv, s_am[i], 0x33);
    }
    expect("script-tail", vol, iv, s_script[1], 0x44);
}

int main(void)
{
    static const unsigned volts[] = { 0, 1, 0x7F, 0x80, 0xFE, 0xFF };
    static const int ivs[] = { 0, 1, -1, 0x1234, -0x1234, 0x7FFF, -0x8000, 0x00FF };
    unsigned v;
    unsigned j;

    for (v = 0; v < sizeof(volts) / sizeof(*volts); v++) {
        for (j = 0; j < sizeof(ivs) / sizeof(*ivs); j++) {
            run_case(volts[v], ivs[j]);
        }
    }

    printf("SOUND VOICE VOLUME D0E8 certificate PASS checks=%u\n", s_checks);
    return 0;
}
