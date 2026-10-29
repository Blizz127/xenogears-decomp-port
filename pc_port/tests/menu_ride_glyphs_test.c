/* func_801D7CFC (Gear screen ride-option glyphs), port body vs expectations.
 *
 * NOT a retail-differential test: no retail code runs here.  The expected
 * values were derived by hand from the retail listing
 * asm/menu/nonmatchings/main/misc/func_801D7CFC.s (the run script checks
 * that listing against the user's disc/menu.bin first):
 *   - a zero count (arg1 & 0xFF) only clears 0x358 block byte 0x2AED;
 *   - otherwise the loop runs exactly twice (slti s0,2).  Glyph
 *     D_801E977C[i] goes through func_8002675C(g_Menu->unk2DC, glyph,
 *     0x358 + 0x1E00 + i*0x50, renderContext, x = 0x78 + i*0x20, y = 0x5A,
 *     0x1000).  Its quad goes through func_801C851C(0x358 + 0x2AA0 + i*0x20,
 *     x0, y0, x1 - x0, y3 - y0), read from prim entry
 *     (renderContext + 2i)*0x28 + 0x1E00;
 *   - the highlight always follows.  func_801E91C4 is called on entry
 *     (renderContext + 2*((arg2 & 0xFF) == 0))*0x28 + 0x1E00, then its +0x16
 *     |= 0x20 and +4..+6 = 0x20; 0x2AEE = renderContext low byte; 0x2AED = 2;
 *   - g_Menu's own storage is never written.
 * The run script extracts the port body from src/menu/main/misc.c, maps
 * MenuRawPointer to this file's 0x358 block, and links it against the
 * recording doubles below.  It covers 16 cases: SystemMenu zero-filled and
 * filled with 0xA5, renderContext 0/1, arg1 0/1, arg2 0/1. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "system/menu.h"

SystemMenu* g_Menu;
s32 D_801E977C[2] = {0x31, 0x32};
static u8 block358[0x3000] __attribute__((aligned(16)));
static unsigned cases, c_fill, c_rc, c_count, c_ride;

#define EXPECT(cond, ...)                                                              \
    do {                                                                               \
        if (!(cond)) {                                                                 \
            printf("FAIL ride glyphs fill=0x%02X rc=%u count=%u ride=%u: ", c_fill, c_rc, \
                   c_count, c_ride);                                                   \
            printf(__VA_ARGS__);                                                       \
            printf("\n");                                                              \
            exit(1);                                                                   \
        }                                                                              \
    } while (0)

u8* xeno_raw_358(u32 offset)
{
    EXPECT(offset == 0x358, "MenuRawPointer(0x%x)", offset);
    return block358;
}

static struct { u8* table; s32 glyph; u8* dst; s32 rc, x, y, scale; } glyphCalls[8];
static struct { u8* verts; s32 x, y, w, h; } quadCalls[8];
static u8* highlightCalls[8];
static int nGlyph, nQuad, nHighlight;

s32 func_8002675C(u8* table, s32 glyph, void* dst, s32 rc, s32 x, s32 y, s32 scale)
{
    if (nGlyph < 8) {
        glyphCalls[nGlyph].table = table; glyphCalls[nGlyph].glyph = glyph;
        glyphCalls[nGlyph].dst = dst; glyphCalls[nGlyph].rc = rc;
        glyphCalls[nGlyph].x = x; glyphCalls[nGlyph].y = y; glyphCalls[nGlyph].scale = scale;
    }
    nGlyph++;
    return 1;
}

void func_801C851C(SVECTOR* verts, s32 x, s32 y, s32 w, s32 h)
{
    if (nQuad < 8) {
        quadCalls[nQuad].verts = (u8*)verts; quadCalls[nQuad].x = x; quadCalls[nQuad].y = y;
        quadCalls[nQuad].w = w; quadCalls[nQuad].h = h;
    }
    nQuad++;
}

void func_801E91C4(POLY_FT4* p)
{
    if (nHighlight < 8) highlightCalls[nHighlight] = (u8*)p;
    nHighlight++;
}

void func_801D7CFC(s32 arg0, s32 arg1, s32 arg2);

static void run_case(unsigned fill, unsigned rc, unsigned count, unsigned ride)
{
    static SystemMenu menu, snapshot;
    static u8 expected[sizeof block358];
    int i, k, inverted = (u8)ride == 0;

    c_fill = fill; c_rc = rc; c_count = count; c_ride = ride;
    memset(&menu, (int)fill, sizeof menu);
    menu.renderContext = (s32)rc;
    menu.unk2DC = (void*)(uintptr_t)0x12345670u;
    g_Menu = &menu;
    snapshot = menu;
    for (k = 0; k < (int)sizeof block358; k++) block358[k] = (u8)(k * 7 + rc);
    for (k = 0; k < 4; k++) {
        u8* e = block358 + 0x1E00 + k * 0x28;
        *(u16*)(e + 0x08) = (u16)(10 + k);
        *(u16*)(e + 0x0A) = (u16)(20 + k);
        *(u16*)(e + 0x10) = (u16)(50 + 3 * k);
        *(u16*)(e + 0x22) = (u16)(70 + 5 * k);
        *(u16*)(e + 0x16) &= (u16)~0x20;
        e[4] = e[5] = e[6] = 0;
    }
    memcpy(expected, block358, sizeof expected);
    nGlyph = nQuad = nHighlight = 0;

    func_801D7CFC(0, (s32)count, (s32)ride);

    EXPECT(memcmp(&menu, &snapshot, sizeof menu) == 0, "g_Menu storage was written");
    if ((u8)count == 0) {
        expected[0x2AED] = 0;
        EXPECT(nGlyph == 0 && nQuad == 0 && nHighlight == 0, "calls on a zero count");
        EXPECT(memcmp(block358, expected, sizeof expected) == 0, "0x358 block changed beyond 0x2AED");
        cases++;
        return;
    }
    EXPECT(nGlyph == 2, "func_8002675C called %d times, expected 2", nGlyph);
    EXPECT(nQuad == 2, "func_801C851C called %d times, expected 2", nQuad);
    for (i = 0; i < 2; i++) {
        int e = (int)rc + i * 2;
        EXPECT(glyphCalls[i].table == (u8*)menu.unk2DC, "glyph %d texture table", i);
        EXPECT(glyphCalls[i].glyph == D_801E977C[i], "glyph %d id 0x%x", i, glyphCalls[i].glyph);
        EXPECT(glyphCalls[i].dst == block358 + 0x1E00 + i * 0x50, "glyph %d buffer", i);
        EXPECT(glyphCalls[i].rc == (s32)rc, "glyph %d renderContext %d", i, glyphCalls[i].rc);
        EXPECT(glyphCalls[i].x == 0x78 + i * 0x20 && glyphCalls[i].y == 0x5A &&
               glyphCalls[i].scale == 0x1000, "glyph %d position", i);
        EXPECT(quadCalls[i].verts == block358 + 0x2AA0 + i * 0x20, "quad %d vertex block", i);
        EXPECT(quadCalls[i].x == 10 + e && quadCalls[i].y == 20 + e &&
               quadCalls[i].w == (50 + 3 * e) - (10 + e) && quadCalls[i].h == (70 + 5 * e) - (20 + e),
               "quad %d rectangle", i);
    }
    {
        u8* poly = block358 + ((int)rc + inverted * 2) * 0x28 + 0x1E00;
        EXPECT(nHighlight == 1 && highlightCalls[0] == poly, "highlight call (%d calls)", nHighlight);
        EXPECT((*(u16*)(poly + 0x16) & 0x20) && poly[4] == 0x20 && poly[5] == 0x20 && poly[6] == 0x20,
               "highlight shade");
    }
    EXPECT(block358[0x2AEE] == (u8)rc && block358[0x2AED] == 2, "0x2AEE=%u 0x2AED=%u",
           block358[0x2AEE], block358[0x2AED]);
    cases++;
}

int main(void)
{
    static const unsigned fills[2] = {0x00, 0xA5};
    unsigned f, rc, count, ride;
    for (f = 0; f < 2; f++)
        for (rc = 0; rc < 2; rc++)
            for (count = 0; count < 2; count++)
                for (ride = 0; ride < 2; ride++) run_case(fills[f], rc, count, ride);
    printf("MENU RIDE GLYPHS PASS cases=%u\n", cases);
    return 0;
}
