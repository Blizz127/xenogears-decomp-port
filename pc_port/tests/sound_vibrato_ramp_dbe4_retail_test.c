#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"

extern u8* func_8003DBE4(u8*, void*, void*);

static u8 element[0x150];
static u8 script[4];

static void check(int ok, const char* what, unsigned from, unsigned n, unsigned to)
{
    if (!ok) {
        fprintf(stderr, "SOUND DBE4 FAIL %s from=%u n=%u to=%u\n", what, from, n, to);
        exit(1);
    }
}

int main(void)
{
    static const unsigned values[] = {0, 1, 0x7f, 0x80, 0xff};
    unsigned i, j, k;

    for (i = 0; i < sizeof(values) / sizeof(*values); i++) {
        for (j = 0; j < sizeof(values) / sizeof(*values); j++) {
            for (k = 0; k < sizeof(values) / sizeof(*values); k++) {
                s32 from = (s32)((u32)values[i] << 24);
                s32 to = (s32)((u32)values[k] << 24);
                s32 diff = (s32)((u32)to - (u32)from);
                u8* ret;
                unsigned n;

                memset(element, 0x5a, sizeof(element));
                memset(script, 0xa5, sizeof(script));
                script[0] = (u8)values[i];
                script[1] = (u8)values[j];
                script[2] = (u8)values[k];
                *(u16*)(element + 0x04) = 0x0108;
                *(s32*)(element + 0x7c) = 0x13579bdf;
                *(u16*)(element + 0x80) = 0x2468;
                *(u16*)(element + 0x82) = 0xbeef;

                ret = func_8003DBE4(script, NULL, element);
                check(ret == script + 3, "return", values[i], values[j], values[k]);
                if (diff != 0 && values[j] != 0) {
                    check(*(u16*)(element + 0x82) == (u16)(from >> 16), "from", values[i], values[j], values[k]);
                    check(*(u16*)(element + 0x80) == (u16)values[j], "count", values[i], values[j], values[k]);
                    check(*(u16*)(element + 0x04) == 0x0100, "flags arm", values[i], values[j], values[k]);
                    check(*(s32*)(element + 0x7c) == diff / (s32)values[j], "step", values[i], values[j], values[k]);
                } else {
                    check(*(u16*)(element + 0x04) == 0x0008, "flags clear", values[i], values[j], values[k]);
                    check(*(s32*)(element + 0x7c) == 0x13579bdf, "guard step", values[i], values[j], values[k]);
                    check(*(u16*)(element + 0x80) == 0x2468, "guard count", values[i], values[j], values[k]);
                    check(*(u16*)(element + 0x82) == 0xbeef, "guard from", values[i], values[j], values[k]);
                }
                for (n = 0; n < sizeof(element); n++) {
                    if (n >= 0x04 && n < 0x06) continue;
                    if (n >= 0x7c && n < 0x84) continue;
                    check(element[n] == 0x5a, "untouched", values[i], values[j], values[k]);
                }
            }
        }
    }
    puts("SOUND VIBRATO RAMP DBE4 certificate PASS checks=125");
    return 0;
}
