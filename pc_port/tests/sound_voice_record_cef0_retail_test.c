/*
 * Retail certificate for func_8003CEF0 (sound.c seq cmd 0x98): push a looping
 * voice record.
 *
 * Retail (asm/slus_006.64/nonmatchings/system/sound/func_8003CEF0.s, file
 * offset 0x2D6F0, 72 bytes) is:
 *   sel = *(u16*)(elem + 0x72); *(u16*)(elem + 0x72) = sel + 1;   // then RELOAD
 *   rec = (u8*)elem + (*(u16*)(elem + 0x72) * 12 + 0x9C);
 *   rec[0] = *pScript++ + 0xFF;
 *   *(u32*)(rec + 4) = pScript;                 // SOUND_PTR_TO_PSX
 *   rec[2] = (u8)elem->octave;                  // lbu of the low byte at 0x66
 *   return pScript;
 *
 * The shipped function in the TU is the matching C body (byte-exact with the
 * pinned gcc-2.6.0 + maspsx); this test drives it directly, so a wrong record
 * base, byte addend, selector step or octave source is rejected.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"

extern u8* func_8003CEF0(u8* pScript, void* pAudioManager, void* pAudioElements);

static u8 s_elem[0x200];
static u8 s_script[0x40];
static unsigned int s_checks;

static u32 guest_addr(const void* p)
{
    return (u32)(unsigned long)p;
}

static void fail(const char* what, unsigned sel, unsigned idx,
                 unsigned actual, unsigned expected)
{
    fprintf(stderr,
            "ASSERTION cef0.result %s sel=%u case=%u actual=0x%x expected=0x%x\n",
            what, sel, idx, actual, expected);
    exit(1);
}

static void expect(const char* what, unsigned sel, unsigned idx,
                   unsigned actual, unsigned expected)
{
    s_checks++;
    if (actual != expected) {
        fail(what, sel, idx, actual, expected);
    }
}

/*
 * One dispatch: selector value `sel` (0..3), script byte, octave value.
 * The record base is elem + ((sel + 1) * 12 + 0x9C); the previous slot
 * (elem + (sel * 12 + 0x9C)) and the next slot (elem + ((sel + 2) * 12 +
 * 0x9C)) must stay untouched.
 */
static void run_case(unsigned sel, unsigned byte, unsigned octave)
{
    u8* ret;
    u8* rec;
    unsigned i;

    memset(s_elem, 0x5A, sizeof(s_elem));
    memset(s_script, 0x11, sizeof(s_script));
    s_script[0] = (u8)byte;
    *(u16*)(s_elem + 0x66) = (u16)octave;
    *(u16*)(s_elem + 0x72) = (u16)sel;

    ret = func_8003CEF0(s_script, NULL, s_elem);

    expect("return", sel, 0, ret == s_script + 1, 1);
    expect("selector", sel, 0, *(u16*)(s_elem + 0x72), sel + 1);

    rec = s_elem + ((sel + 1) * 12 + 0x9C);
    expect("rec[0]", sel, 0, rec[0], (u8)(byte + 0xFF));
    expect("rec+4", sel, 0, *(u32*)(rec + 4),
           guest_addr(s_script + 1));
    expect("rec[2]", sel, 0, rec[2], (u8)octave);

    /* The stale selector slot before rec and the slot after must be untouched. */
    expect("pre-slot", sel, 0, *(u32*)(s_elem + (sel * 12 + 0x9C)), 0x5A5A5A5A);
    expect("post-slot", sel, 0,
           *(u32*)(s_elem + ((sel + 2) * 12 + 0x9C) + 4), 0x5A5A5A5A);

    /* Only rec[0], rec[2] and rec[4..7] may change in the destination slot. */
    for (i = 1; i < 12; i++) {
        if (i == 2 || (i >= 4 && i < 8)) {
            continue;
        }
        expect("rec-untouched", sel, i, rec[i], 0x5A);
    }
}

int main(void)
{
    run_case(0, 0x00, 0x0000);
    run_case(0, 0x7F, 0x1234);
    run_case(1, 0xFF, 0x00FF);
    run_case(2, 0x01, 0x00AB);
    run_case(3, 0x80, 0xABCD);

    printf("SOUND VOICE RECORD CEF0 certificate PASS checks=%u\n", s_checks);
    return 0;
}
