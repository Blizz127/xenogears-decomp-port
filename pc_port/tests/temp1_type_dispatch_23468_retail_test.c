/* Retail certificate for func_80023468 (0x80023468-0x800234AC).
 * Dispatch on `type`: 0/5/6/10-14 -> 1, 1/4/8/9 -> 0, 2/7/15 -> 2, and case 3
 * together with every type >= 0x10 returns the caller's a1 slot (the C models
 * that as its second parameter). */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "common.h"

extern s32 func_80023468(s32 type, s32 arg1);

static unsigned s_checks;

static void fail(const char* name, const char* detail)
{
    fprintf(stderr, "ASSERTION %s %s\n", name, detail);
    exit(1);
}


static void check(s32 type, s32 expected, s32 ret)
{
    char detail[64];

    s_checks++;
    if (ret != expected) {
        snprintf(detail, sizeof(detail), "type=%d", (int)type);
        fail("temp1.dispatch", detail);
    }
}

int main(void)
{
    s32 t;

    /* group 1 */
    for (t = 0; t < 16; t++) {
        if (t == 0 || t == 5 || t == 6 || (t >= 10 && t <= 14)) {
            check(t, 1, func_80023468(t, 0x77));
        }
    }
    /* group 0 */
    check(1, 0, func_80023468(1, 0x77));
    check(4, 0, func_80023468(4, 0x77));
    check(8, 0, func_80023468(8, 0x77));
    check(9, 0, func_80023468(9, 0x77));
    /* group 2 */
    check(2, 2, func_80023468(2, 0x77));
    check(7, 2, func_80023468(7, 0x77));
    check(15, 2, func_80023468(15, 0x77));

    /* case 3 and out-of-range return the a1 slot */
    check(3, 0x77, func_80023468(3, 0x77));
    check(3, -5, func_80023468(3, -5));
    check(16, 0x1234, func_80023468(16, 0x1234));
    check(0x100, -1, func_80023468(0x100, -1));

    printf("TEMP1 TYPE DISPATCH 23468 certificate PASS checks=%u\n", s_checks);
    return 0;
}
