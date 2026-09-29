#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"

extern u8* func_8003D13C(u8*, void*, void*);

static u8 manager[0x90];
static u8 script[4];

static void check(int ok, const char* what, unsigned target, unsigned steps, s32 current)
{
    if (!ok) {
        fprintf(stderr, "SOUND D13C FAIL %s target=%u steps=%u current=%d\n",
                what, target, steps, current);
        exit(1);
    }
}

int main(void)
{
    static const unsigned targets[] = {0, 1, 0x7f, 0x80, 0xff};
    static const unsigned steps[] = {0, 1, 2, 0x80, 0xff};
    static const s32 currents[] = {0, 1, -1, 0x400000, -0x400000};
    unsigned i, j, k;

    for (i = 0; i < sizeof(targets) / sizeof(*targets); i++) {
        for (j = 0; j < sizeof(steps) / sizeof(*steps); j++) {
            for (k = 0; k < sizeof(currents) / sizeof(*currents); k++) {
                s32 current = currents[k];
                s32 diff = ((s32)targets[i] << 16) - current;
                u8* ret;
                unsigned n;

                memset(manager, 0x5a, sizeof(manager));
                memset(script, 0xa5, sizeof(script));
                script[0] = (u8)steps[j];
                script[1] = (u8)targets[i];
                *(s32*)(manager + 0x58) = current;
                *(s32*)(manager + 0x5c) = 0x13579bdf;
                *(u16*)(manager + 0x60) = 0x2468;
                *(u16*)(manager + 0x62) = 0xbeef;

                ret = func_8003D13C(script, manager, NULL);
                check(ret == script + 2, "return", targets[i], steps[j], current);
                check(*(u16*)(manager + 0x62) == (u16)targets[i], "target", targets[i], steps[j], current);
                if (steps[j] != 0 && diff != 0) {
                    check(*(s32*)(manager + 0x5c) == diff / (s32)steps[j], "increment", targets[i], steps[j], current);
                    check(*(u16*)(manager + 0x60) == (u16)steps[j], "steps", targets[i], steps[j], current);
                } else {
                    check(*(s32*)(manager + 0x5c) == 0x13579bdf, "zero guard increment", targets[i], steps[j], current);
                    check(*(u16*)(manager + 0x60) == 0x2468, "zero guard steps", targets[i], steps[j], current);
                }
                for (n = 0; n < sizeof(manager); n++) {
                    if ((n >= 0x5c && n < 0x60) || (n >= 0x60 && n < 0x64)) continue;
                    if (n >= 0x58 && n < 0x5c) continue;
                    check(manager[n] == 0x5a || (n >= 0x58 && n < 0x5c), "untouched", targets[i], steps[j], current);
                }
            }
        }
    }
    puts("SOUND MASTER VOLUME FADE D13C certificate PASS checks=125");
    return 0;
}
