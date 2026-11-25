#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "fixture.h"
#include "../../src/cleanroom/font_controls.inc"
#include "../../src/cleanroom/font_resource_binding.inc"
#include "../../src/cleanroom/font_services.inc"
#include "../../src/cleanroom/font_draw_letters.inc"
#include "../../src/cleanroom/font_glyph.inc"

static Font first, second;
static SPRT_16 packets[2][6];
static unsigned table;
static int allocationsReleased, formats;
static char formatted[64];

static void prepare(void)
{
    memset(&first, 0, sizeof(first));
    memset(&second, 0, sizeof(second));
    memset(packets, 0, sizeof(packets));
    first.startX = 4;
    first.startY = 9;
    first.areaWidth = 70;
    first.areaHeight = 55;
    first.letterWidth = 7;
    first.rowHeight = 12;
    first.maxNumLetters = 4;
    first.vramLetterOffsetX = 3;
    for (unsigned i = 0; i < 4; ++i)
        first.letterClutIds[i] = (short)(17 + 2 * i);
    first.pPrimBuffers[0] = packets[0];
    first.pPrimBuffers[1] = packets[1];
    g_Font = &first;
    FontResetLetterRendering();
}

static void testPositionsAndFlags(void)
{
    prepare();
    FontSetFlag(0x8201);
    assert(FontGetFlags() == (int)(int16_t)0x8201);
    FontRemoveFlag(0x0201);
    assert(FontGetFlags() == (int)(int16_t)0x8000);
    FontSetLetterFlag(0x4205);
    FontRemoveLetterFlag(0x201);
    assert(FontGetLetterFlags() == 0x4004);
    FontSetLetterWidth(13);
    FontSetRowHeight(18);
    assert(FontGetDefaultLetterWidth() == 13 && FontGetDefaultLetterHeight() == 18);

    FontSetCurLetterX(-17);
    FontSetCurLetterY(23);
    first.nextRowX = 6;
    FontStoreCurrentPositions();
    FontSetPositionsByOffset(8, -14);
    assert(FontGetCurLetterX() == 12 && FontGetCurLetterY() == -5);
    func_8003708C(-5, 18);
    assert(first.nextRowX == 4 && FontGetCurLetterX() == 4 && FontGetCurLetterY() == 27);
    FontRestoreCurrentPositions();
    assert(FontGetCurLetterX() == -17 && FontGetCurLetterY() == 23 && first.nextRowX == 6);

    first.primitiveCode = 0x54;
    FontSetColor(145, 160, 180);
    assert((uint8_t)first.r == 145 && (uint8_t)first.g == 160 && (uint8_t)first.b == 180);
    assert(first.primitiveCode == 0x55);
    FontSetColor(126, 179, 140);
    assert(first.primitiveCode == 0x54);
    FontSetLetterFlag(FONT_LETTER_RENDER_CONTEXT);
    first.curNumLetters = 3;
    FontResetLetterRendering();
    assert(FontGetCurLetterCount() == 0 && first.pLetterRenderPacket == packets[1]);
    assert(first.storedPosX == first.startX && first.storedPosY == first.startY);
    assert(first.storedRowX == first.startX);
    g_Font = NULL;
    FontSetPositionsByOffset(2, 3);
    func_8003708C(8, 11);
}

static void testGlyphs(void)
{
    prepare();
    FontSetColor(21, 51, 83);
    first.primitiveCode = 0x38;
    packets[0][0].link = 0x1234;
    FontAddLetterPrimitive('A');
    const SPRT_16 *glyph = &packets[0][0];
    assert(glyph->link == 0x1234 && glyph->r0 == 21 && glyph->g0 == 51 && glyph->b0 == 83);
    assert(glyph->code == 0x38 && glyph->x0 == 4 && glyph->y0 == 9);
    assert(glyph->u0 == 8 && glyph->v0 == 3 && glyph->clut == first.letterClutIds[2]);
    assert(FontGetCurLetterCount() == 1 && first.curX == 11 && first.pLetterRenderPacket == packets[0] + 1);
    FontAddLetterPrimitive(' ');
    assert(first.curX == 18 && first.curNumLetters == 1);
    FontAddLetterPrimitive('\n');
    assert(first.curX == 4 && first.curY == 21 && first.curNumLetters == 1);
    FontAddLetterPrimitive('\t');
    assert(first.curX == 4 && first.curY == 21);

    FontSetLetterFlag(FONT_LETTER_ALL_UPPERCASE | FONT_LETTER_16x16 | 8);
    first.letterWidths['A' - ' '] = 5;
    first.curX = first.startX + first.areaWidth - 5;
    FontSetFlag(8);
    FontAddLetterPrimitive('a');
    assert(first.curNumLetters == 1 && first.curX == 69);
    FontRemoveFlag(8);
    FontAddLetterPrimitive('a');
    glyph = &packets[0][1];
    assert(glyph->x0 == 4 && glyph->y0 == 33 && glyph->u0 == 16 && glyph->v0 == 19);
    assert(glyph->clut == first.letterClutIds[0] && first.curX == 9 && first.curNumLetters == 2);
    FontAddLetterPrimitive(777);
    assert(first.curNumLetters == 2 && first.curX == 9);
    first.maxNumLetters = 2;
    FontAddLetterPrimitive('a');
    assert(first.curNumLetters == 3);
    FontAddLetterPrimitive('a');
    assert(first.curNumLetters == 3 && first.curX == 14);
    first.curY = first.startY + first.areaHeight;
    FontAddLetterPrimitive('\n');
    assert(first.curY == 64 && first.curX == 14);
    g_Font = NULL;
    FontAddLetterPrimitive('A');
}

static void func_80036718(int mode, char *format, va_list args)
{
    assert(mode == 0);
    assert(vsnprintf(formatted, sizeof(formatted), format, args) >= 0);
    ++formats;
}

static void HeapFree(void *font)
{
    assert(font == &first && g_HeapDebugPrintfFn == FontPrintf);
    ++allocationsReleased;
    g_Font = &second;
    D_800593A0 = &table;
}

static void testServices(void)
{
    prepare();
    FontPrintf("%s [%d]", "glyph", -3);
    assert(formats == 1 && strcmp(formatted, "glyph [-3]") == 0);
    func_8003747C(&table);
    assert(D_800593A0 == &table);
    FontFree();
    assert(g_Font == NULL && D_800593A0 == NULL && allocationsReleased == 0);
    assert(g_HeapDebugPrintfFn == FontPrintf);
    FontPrintf("%s", "discarded");
    assert(formats == 1);
    func_8003747C(&second);
    FontFree();
    assert(D_800593A0 == NULL && allocationsReleased == 0);
    g_Font = &first;
    FontFree();
    assert(allocationsReleased == 1 && g_Font == NULL && D_800593A0 == NULL);
}

enum Service { SYNC, TERMINATE, GLYPH, PAGE, BACKGROUND, SUBMIT };
struct Event { enum Service name; void *table, *item; };
static struct Event events[9];
static unsigned recorded, checked, selectedBank;
static int changeDuringDraw;

static void record(enum Service name, void *ot, void *item)
{
    assert(recorded < sizeof(events) / sizeof(events[0]));
    events[recorded++] = (struct Event){name, ot, item};
}

static void expect(enum Service name, void *ot, void *item)
{
    assert(checked < recorded);
    const struct Event *event = &events[checked++];
    assert(event->name == name && event->table == ot && event->item == item);
}

static int DrawSync(int mode) { assert(mode == 0); record(SYNC, NULL, NULL); return 0; }
static void TermPrim(void *ot) { record(TERMINATE, ot, NULL); }
static void func_800317E0(void *ot, void *item)
{
    record(GLYPH, ot, item);
    if (changeDuringDraw) {
        first.curNumLetters = 0;
        first.pPrimBuffers[selectedBank] = NULL;
    }
}
static void AddPrim(void *ot, void *item)
{
    record(PAGE, ot, item);
    if (changeDuringDraw) {
        first.flags ^= 16;
        g_Font = &second;
    }
}
static void func_80031804(void *ot, void *item) { record(BACKGROUND, ot, item); }
static void DrawOTag(void *ot) { record(SUBMIT, ot, NULL); }

static void testDraw(void)
{
    void *requests[] = {&table, NULL, (void *)UINTPTR_MAX};
    for (unsigned request = 0; request < 3; ++request)
        for (unsigned flags = 0; flags < 4; ++flags)
            for (unsigned slot = 0; slot < 2; ++slot)
                for (int count = 0; count <= 3; ++count)
                    for (changeDuringDraw = 0; changeDuringDraw <= 1; ++changeDuringDraw) {
                        prepare();
                        recorded = checked = 0;
                        first.flags = (short)((flags & 1) | ((flags & 2) << 3));
                        first.letterFlags = (short)(0x44 | slot);
                        first.curNumLetters = (short)count;
                        second.curNumLetters = 7;
                        selectedBank = flags & 1 ? 0 : slot;
                        void *ot = request == 0 ? &table : (void *)&first.pPrimBuffersStart[selectedBank];
                        FontDrawLetters(requests[request]);
                        assert((unsigned short)first.letterFlags == (0x44u | ((flags & 1) ? 0u : (1u - slot))));
                        if (request != 0) {
                            expect(SYNC, NULL, NULL);
                            expect(TERMINATE, ot, NULL);
                        }
                        for (int i = 0; i < count; ++i)
                            expect(GLYPH, ot, &packets[selectedBank][i]);
                        expect(PAGE, ot, &first.texpageSettings[selectedBank]);
                        if (((flags & 2) != 0) != (changeDuringDraw != 0))
                            expect(BACKGROUND, ot, &first.bgTiles[selectedBank]);
                        if (request != 0) expect(SUBMIT, ot, NULL);
                        assert(checked == recorded && first.curNumLetters == 0);
                        assert(second.curNumLetters == (changeDuringDraw ? 0 : 7));
                    }
    g_Font = NULL;
    recorded = 0;
    FontDrawLetters(NULL);
    assert(recorded == 0);
}

int main(void)
{
    testPositionsAndFlags();
    testGlyphs();
    testServices();
    testDraw();
    puts("26 font providers passed synthetic state, glyph, formatting, lifetime and draw checks");
    return 0;
}
