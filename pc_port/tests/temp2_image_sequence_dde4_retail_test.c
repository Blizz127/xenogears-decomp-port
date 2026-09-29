/* Retail certificate for func_8002DDE4 (0x8002DDE4-0x8002DFE0).
 * Image-sequence uploader: this pins the RECT each texture mode computes, the
 * LoadImage destination, the cursor advance and the 0x1101 return for an
 * unknown opcode. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "psyq/libgpu.h"

extern s32 func_8002DDE4(u32* pImageData, s32 arg1, s32 texX, s32 texY, s32 modeB, u16 ofsX, u16 ofsY);

static u8 s_buf[0x80];
static unsigned s_checks;
static int s_loads;
static s16 s_x, s_y, s_w, s_h;
static void* s_dst;

/* matches PsyCross's prototype (RECT16/long*) */
int LoadImage(RECT16* pRect, u_long* pData)
{
    s_loads++;
    s_x = pRect->x; s_y = pRect->y; s_w = pRect->w; s_h = pRect->h;
    s_dst = pData;
    return 0;
}

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
        fail("temp2.dde4", detail);
    }
}

static void expect_eq_ptr(const char* field, const void* actual, const void* expected)
{
    char detail[160];

    s_checks++;
    if (actual != expected) {
        snprintf(detail, sizeof(detail), "field=%s actual=%p expected=%p",
                 field, actual, expected);
        fail("temp2.dde4", detail);
    }
}

/* one record: opcode, u0/v0/u1/v1, w, h, then the pixel payload */
static void build(u32 opcode, u16 u0, u16 v0, u16 u1, u16 v1, u16 w, u16 h)
{
    memset(s_buf, 0, sizeof(s_buf));
    *(u32*)s_buf = 1;
    *(u32*)(s_buf + 8) = opcode;
    *(u16*)(s_buf + 12) = u0;
    *(u16*)(s_buf + 14) = v0;
    *(u16*)(s_buf + 16) = u1;
    *(u16*)(s_buf + 18) = v1;
    *(u16*)(s_buf + 20) = w;
    *(u16*)(s_buf + 22) = h;
    s_loads = 0;
    s_x = s_y = s_w = s_h = 0;
    s_dst = NULL;
}

int main(void)
{
    s32 rc;

    /* mode 1 uses the offset pair and adds texX/texY */
    build(0x1100, 0x10, 0x20, 0x30, 0x40, 8, 4);
    rc = func_8002DDE4((u32*)s_buf, 1, 0x100, 0x200, 0, 0, 0);
    expect_eq_s32("m1.rc", rc, 0);
    expect_eq_s32("m1.x", s_x, 0x100 + 0x30);
    expect_eq_s32("m1.y", s_y, 0x200 + 0x40);
    expect_eq_s32("m1.w", s_w, 8);
    expect_eq_s32("m1.h", s_h, 4);
    expect_eq_ptr("m1.dst", s_dst, s_buf + 24);
    expect_eq_s32("m1.loads", s_loads, 1);

    /* mode 2 adds the record sums and the offsets */
    build(0x1100, 0x10, 0x20, 0x30, 0x40, 2, 2);
    func_8002DDE4((u32*)s_buf, 2, 0x100, 0x200, 0, 0, 0);
    expect_eq_s32("m2.x", s_x, 0x100 + 0x10 + 0x30);
    expect_eq_s32("m2.y", s_y, 0x200 + 0x20 + 0x40);

    /* mode 0 (neither 1 nor 2) uses the record sums alone */
    build(0x1100, 0x11, 0x22, 0x33, 0x44, 1, 1);
    func_8002DDE4((u32*)s_buf, 0, 0x100, 0x200, 0, 0, 0);
    expect_eq_s32("m0.x", s_x, 0x11 + 0x33);
    expect_eq_s32("m0.y", s_y, 0x22 + 0x44);

    /* 0x1101 uses ofsX/ofsY through modeB */
    build(0x1101, 0x10, 0x20, 0x30, 0x40, 3, 5);
    func_8002DDE4((u32*)s_buf, 0, 0, 0, 2, 0x400, 0x500);
    expect_eq_s32("mB2.x", s_x, 0x400 + 0x10 + 0x30);
    expect_eq_s32("mB2.y", s_y, 0x500 + 0x20 + 0x40);
    expect_eq_s32("mB2.w", s_w, 3);

    /* unknown opcode returns the delay-slot value 0x1101 and uploads nothing */
    build(0x1234, 1, 2, 3, 4, 1, 1);
    rc = func_8002DDE4((u32*)s_buf, 0, 0, 0, 0, 0, 0);
    expect_eq_s32("bad.rc", rc, 0x1101);
    expect_eq_s32("bad.loads", s_loads, 0);

    /* non-positive count returns 0 without touching LoadImage */
    memset(s_buf, 0, sizeof(s_buf));
    s_loads = 0;
    rc = func_8002DDE4((u32*)s_buf, 0, 0, 0, 0, 0, 0);
    expect_eq_s32("zero.rc", rc, 0);
    expect_eq_s32("zero.loads", s_loads, 0);

    printf("TEMP2 IMAGE SEQUENCE DDE4 certificate PASS checks=%u\n", s_checks);
    return 0;
}
