#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"

extern u8* func_8003D7C8(u8*, void*, void*);

static u8 manager[0x90];
static u8 script[4];

static void check(int ok, const char* what, int high, int low)
{
    if (!ok) {
        fprintf(stderr, "SOUND D7C8 FAIL %s high=%d low=%d\n", what, high, low);
        exit(1);
    }
}

int main(void)
{
    static const int highs[] = {0, 1, 0x7f, 0x80, 0xff};
    static const int lows[] = {0, 1, 0x7f, 0x80, 0xff};
    unsigned i, j;

    for (i = 0; i < sizeof(highs) / sizeof(*highs); i++) {
        for (j = 0; j < sizeof(lows) / sizeof(*lows); j++) {
            s32 expected;
            u8* ret;
            unsigned k;

            memset(manager, 0x5a, sizeof(manager));
            memset(script, 0xa5, sizeof(script));
            script[0] = (u8)highs[i];
            script[1] = (u8)lows[j];
            *(u16*)(manager + 0x02) = 0x1001;
            *(u16*)(manager + 0x6e) = 0x7ff0;

            ret = func_8003D7C8(script, NULL, manager);
            expected = (s32)(int8_t)highs[i] * 0x100 + lows[j] + 0x7ff0;
            check(ret == script + 2, "return", highs[i], lows[j]);
            check(*(u16*)(manager + 0x02) == 0x1201, "status", highs[i], lows[j]);
            check(*(u16*)(manager + 0x6e) == (u16)expected, "pitch", highs[i], lows[j]);
            for (k = 0; k < sizeof(manager); k++) {
                if ((k >= 2 && k < 4) || (k >= 0x6e && k < 0x70)) continue;
                check(manager[k] == 0x5a, "untouched", highs[i], lows[j]);
            }
        }
    }
    puts("SOUND PITCH OFFSET D7C8 certificate PASS checks=75");
    return 0;
}
