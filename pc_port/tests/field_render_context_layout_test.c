#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "field/main.h"

typedef struct {
    DRAWENV draw;
    DISPENV disp;
} DrawDispPair;

_Static_assert(sizeof(DRAWENV) == 0x5C,
               "DRAWENV must match the retail PsyQ ABI");
_Static_assert(__builtin_offsetof(RenderContext, drawEnvs[1]) == 0x5C,
               "field draw environment stride must remain retail-sized");
_Static_assert(__builtin_offsetof(RenderContext, dispEnv) == 0xB8,
               "field display environment must remain at retail offset 0xB8");
_Static_assert(__builtin_offsetof(RenderContext, ot1) == 0xD0,
               "host OT adapter alignment must remain unchanged by the ABI fix");
_Static_assert(sizeof(OT_TAG) == 8,
               "current LP64 OT adapter requires eight-byte host OT slots");
_Static_assert(__builtin_offsetof(DrawDispPair, disp) == 0x5C,
               "a DISPENV following DRAWENV must remain at retail offset 0x5C");

static void fail(const char *message)
{
    fprintf(stderr, "field render-context layout: FAIL: %s\n", message);
    exit(1);
}

static void check(int condition, const char *message)
{
    if (!condition) {
        fail(message);
    }
}

int main(void)
{
    RenderContext field;
    DrawDispPair pair;
    DISPENV *fieldRetailDisp;
    DISPENV *retailDisp;

    memset(&field, 0, sizeof(field));
    memset(&pair, 0, sizeof(pair));

    field.dispEnv.disp.x = 0;
    field.dispEnv.disp.y = 0x100;
    field.dispEnv.disp.w = 320;
    field.dispEnv.disp.h = 224;
    field.dispEnv.isrgb24 = 0;
    fieldRetailDisp = (DISPENV *)((u8 *)&field + 0xB8);

    check(fieldRetailDisp == &field.dispEnv,
          "typed field DISPENV does not alias retail +0xB8 access");
    check(fieldRetailDisp->disp.y == 0x100 &&
          fieldRetailDisp->disp.w == 320 &&
          fieldRetailDisp->disp.h == 224 &&
          fieldRetailDisp->isrgb24 == 0,
          "retail field DISPENV access does not preserve 320x224 movie mode");

    pair.disp.disp.w = 320;
    pair.disp.disp.h = 224;
    retailDisp = (DISPENV *)((u8 *)&pair + 0x5C);
    check(retailDisp == &pair.disp,
          "a typed DISPENV does not alias retail DRAWENV +0x5C access");
    check(retailDisp->disp.w == 320 && retailDisp->disp.h == 224,
          "retail DRAWENV/DISPENV pair does not preserve dimensions");

    puts("field render-context retail DRAWENV/DISPENV layout: PASS");
    return 0;
}
