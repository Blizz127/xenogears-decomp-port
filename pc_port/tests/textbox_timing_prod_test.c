/* Production-linked certificate for field text-box typewriter timing,
 * open-timer gating, and per-glyph X advance.
 *
 * Drives the shipped units:
 *   func_8007F8DC   window setup (speed byte at box+0x80)
 *   func_8008004C   per-frame pump (open-timer gate + enqueue/type)
 *   func_80033DF0   typewriter
 *   func_80034F98   per-glyph width
 *   func_80032F54   window buffer setup
 *   func_80034888   raster/pump of the window
 * Do not stub or wrap those symbols. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "field/actor.h"
#include "field/main.h"
#include "field/text_box.h"
#include "main/game.h"

enum {
    kWindowGlyphX = 0x00,
    kWindowFlags = 0x10,
    kWindowStringPtr = 0x1C,
    kWindowSpeed = 0x68,
    kBoxWindow = 0x18,
    kBoxSpeed = 0x80,
    kBoxStringEntry = 0xA8,
    kBoxOpenTimer = 0x408,
    kBoxVisibility = 0x40E,
    kBoxOrder = 0x410
};

extern void func_80033DF0(void* arg0);
extern void* g_SystemDataEntries;
extern s32 func_80034F98(s32 arg0, s32 arg1);
extern void func_80032F54(void* arg0, s32 tpageX, s32 tpageY, s32 x, s32 y,
                          s32 width, s32 mode, s32 height);
extern void func_80034888(void* arg0, void* ot, s32 renderContextIndex);
extern s32 func_8007F8DC(s32 x, s32 y, s32 stringIndex, s32 textBoxIndex,
                         s32 width, s32 height, s32 ownerActorIndex,
                         s32 talkingActorIndex, s32 mode, s32 orientationFlags,
                         s32 dialogFlags);
extern void func_8008004C(void* ot, s32 renderContextIndex);

extern u32 D_8005934C;
extern u32 D_80059350;
extern u32 D_80059354;
extern u32 D_80059358;
extern u32 D_8005935C;
extern u32 D_80059364;

u16 D_800501D0[11];
GameState g_GameState;
u_char g_ControllerButtonMappings[8];
u8 D_8005A0E4[0x400];
s16 g_SystemPalette1;
s16 g_SystemPalette2;

s32 D_800ADE90;
s32 D_800ADE94;
s32 D_800ADE98;
/* Retail cursor/face RECT tables read by func_8007E1C0 (indexed by
 * D_800ADE94 * 8). Zeroed fixture; the port's retail-initialized copies
 * live in pc_port/src/data_field.c. */
RECT D_800ADEDC[8];
RECT D_800ADF04[8];
s32 D_800B068C[4];
s16 D_800B21D6;
u16 D_800C2694;
u16 D_800C3900;
u16 D_800ADF54 = 0x0300;
u16 D_800ADF56 = 0x0100;
u8 D_800ADF34[32];
u8 D_800B1DF4_storage[0x180];
void* D_800B1DF4 = D_800B1DF4_storage;
s32 g_FieldCurRenderContextIndex;
s32 g_FieldNumActors = 1;
void* D_800ADBF0;
u16 D_800B2174[1];
FieldTextBox g_FieldTextBoxes[4];
FieldScene g_Scene;
FieldActor* volatile g_FieldActors;

static FieldActor s_fieldActors[2];
static ActorData s_actor;
static u8 s_stringTable[0x40];
static u8 s_fontBlob[0x200];
static u8 s_windowScratch[0x200];
static u8 s_nestedEntries[0x100];
static u8 s_nestedTable6[0x40];
static u8 s_nestedTable7[0x40];
static u8 s_nestedTable8[0x40];
static u8 s_nestedScripts[3][4];
static u8 s_heapArena[0x10000];
static size_t s_heapUsed;
static int s_failures;
static int s_pass;
static int s_total;

static void check(int condition, const char* name)
{
    s_total++;
    if (condition) {
        s_pass++;
        printf("PASS %s\n", name);
    } else {
        fprintf(stderr, "ASSERTION %s\n", name);
        s_failures++;
    }
}

void HeapSetCurrentContentType(u_short contentTag)
{
    (void)contentTag;
}

void* HeapAlloc(u_int allocSize, u_int allocFlags)
{
    size_t size = allocSize ? (size_t)allocSize : 1u;
    size_t aligned;
    void* p;

    (void)allocFlags;
    aligned = (s_heapUsed + 15u) & ~((size_t)15u);
    if (aligned + size > sizeof(s_heapArena)) {
        fprintf(stderr, "ASSERTION heap.arena.exhausted\n");
        abort();
    }
    p = &s_heapArena[aligned];
    memset(p, 0, size);
    s_heapUsed = aligned + size;
    return p;
}

void HeapFree(void* p)
{
    (void)p;
}

int FieldScriptVMGetVariableValue(s32 index)
{
    (void)index;
    return 0;
}

void PcPort_AddPrimDomainAware(void* ot, void* prim)
{
    (void)ot;
    (void)prim;
}

void func_80031798(void* ot, void* prim)
{
    (void)ot;
    (void)prim;
}

int LoadImage(RECT* rect, u_long* data)
{
    (void)rect;
    (void)data;
    return 0;
}

void SetSemiTrans(void* p, int abe)
{
    (void)p;
    (void)abe;
}

void SetShadeTex(void* p, int tge)
{
    (void)p;
    (void)tge;
}

u_short GetTPage(int tp, int abr, int x, int y)
{
    (void)tp;
    (void)abr;
    (void)x;
    (void)y;
    return 0;
}

u_short GetClut(int x, int y)
{
    (void)x;
    (void)y;
    return 0;
}

void SetDrawMode(DR_MODE* p, int dfe, int dtd, int tpage, RECT* tw)
{
    (void)p;
    (void)dfe;
    (void)dtd;
    (void)tpage;
    (void)tw;
}

void SetPolyFT4(POLY_FT4* p)
{
    (void)p;
}

void SetSprt(SPRT* p)
{
    (void)p;
}

void SetTile(TILE* p)
{
    (void)p;
}

MATRIX* CompMatrix(MATRIX* a, MATRIX* b, MATRIX* c)
{
    (void)a;
    (void)b;
    return c;
}

void SetRotMatrix(MATRIX* m)
{
    (void)m;
}

void SetTransMatrix(MATRIX* m)
{
    (void)m;
}

int RotTransPers(SVECTOR* v0, int* sxy, long* p, long* flag)
{
    (void)v0;
    *sxy = 0x004000A0;
    *p = 0;
    *flag = 0;
    return 0;
}

static u8* box_bytes(s32 index)
{
    return (u8*)&g_FieldTextBoxes[index];
}

static u8* box_window(s32 index)
{
    return box_bytes(index) + kBoxWindow;
}

static s16 window_glyph_x(u8* window)
{
    return *(s16*)(window + kWindowGlyphX);
}

static u32 window_string(u8* window)
{
    return *(u32*)(window + kWindowStringPtr);
}

static u8 window_speed(u8* window)
{
    return window[kWindowSpeed];
}

static void init_font_tables(void)
{
    memset(s_fontBlob, 0, sizeof(s_fontBlob));
    D_8005934C = 0xFEu;
    D_80059350 = 0u;
    D_80059354 = 0x10u;
    D_80059358 = 0x10u;
    D_8005935C = (u32)(uintptr_t)s_fontBlob;
    D_80059364 = 0x41u;
}

static void init_string_table(const u8* encoded, size_t encoded_len)
{
    memset(s_stringTable, 0, sizeof(s_stringTable));
    *(u16*)(s_stringTable + 4) = 0x10;
    if (encoded_len + 0x10u >= sizeof(s_stringTable)) {
        fprintf(stderr, "ASSERTION string.table.overflow\n");
        abort();
    }
    memcpy(s_stringTable + 0x10, encoded, encoded_len);
    D_800ADBF0 = s_stringTable;
}

static void reset_world(void)
{
    int i;

    s_heapUsed = 0;
    memset(s_heapArena, 0, sizeof(s_heapArena));
    memset(&s_actor, 0, sizeof(s_actor));
    memset(s_fieldActors, 0, sizeof(s_fieldActors));
    memset(g_FieldTextBoxes, 0, sizeof(g_FieldTextBoxes));
    memset(s_windowScratch, 0, sizeof(s_windowScratch));
    memset(D_800B1DF4_storage, 0, sizeof(D_800B1DF4_storage));
    memset(&g_Scene, 0, sizeof(g_Scene));

    s_actor.faceId = 0xFF;
    s_actor.dialogFlags = 0;
    s_actor.flags = 0;
    s_fieldActors[0].pActorData = (u32)(uintptr_t)&s_actor;
    g_FieldActors = s_fieldActors;

    D_800ADE90 = 0;
    D_800ADE94 = 0;
    D_800ADE98 = 0;
    D_800C2694 = 0;
    D_800C3900 = 0;
    D_800B2174[0] = 0;
    g_FieldCurRenderContextIndex = 0;
    D_800B1DF4 = D_800B1DF4_storage;
    for (i = 0; i < 4; i++) {
        D_800B068C[i] = -1;
        g_FieldTextBoxes[i].visibility = -1;
        g_FieldTextBoxes[i].status = -1;
        g_FieldTextBoxes[i].order = 0xFFFF;
        g_FieldTextBoxes[i].ownerActorID = 0xFF;
    }
    init_font_tables();
}

static s32 open_box(s32 text_speed, s32 dialog_flags)
{
    D_800B21D6 = (s16)text_speed;
    /* mode 3 is the fixed-position box path (no actor projection). Its
     * target delta is 0, so setup does not hit the signed-sll of a negative
     * animation offset that mode 2 produces for these dimensions. */
    return func_8007F8DC(0x20, 0x20, 0, 0, 0x18, 4, 0, 0, 3, 0, dialog_flags);
}

static void pump_once(void)
{
    u32 ot[8];

    memset(ot, 0, sizeof(ot));
    func_8008004C(ot, 0);
}

static void setup_standalone_window(u8* window, const u8* encoded, u8 speed)
{
    func_80032F54(window, 0, 0, 8, 8, 0x18, 0, 4);
    *(u32*)(window + kWindowStringPtr) = (u32)(uintptr_t)encoded;
    window[kWindowSpeed] = speed;
    window[0x69] = speed;
    *(s16*)(window + kWindowGlyphX) = 0;
    *(s16*)(window + 0x02) = 0;
    window[0x6C] = 0;
    *(u16*)(window + kWindowFlags) = 0;
}

static void init_nested_table(u8* table, u16 entry, u16 string_offset, u8 glyph)
{
    memset(table, 0, 0x40);
    *(u16*)(table + 4 + entry * 2) = string_offset;
    table[string_offset] = glyph;
    table[string_offset + 1] = 0;
}

static void test_nested_table_offsets(void)
{
    static const struct {
        u8 subcode;
        u16 table_offset;
        u8* table;
        u16 entry;
        u16 string_offset;
        u8 glyph;
        const char* label;
    } cases[] = {
        {0x6, 0x5C, s_nestedTable6, 3, 0x10, 0x41, "table6"},
        {0x7, 0x60, s_nestedTable7, 5, 0x12, 0x42, "table7"},
        {0x8, 0x64, s_nestedTable8, 7, 0x14, 0x43, "table8"},
    };
    u8* window = s_windowScratch;

    reset_world();
    memset(s_nestedEntries, 0, sizeof(s_nestedEntries));
    init_nested_table(s_nestedTable6, cases[0].entry, cases[0].string_offset, cases[0].glyph);
    init_nested_table(s_nestedTable7, cases[1].entry, cases[1].string_offset, cases[1].glyph);
    init_nested_table(s_nestedTable8, cases[2].entry, cases[2].string_offset, cases[2].glyph);
    *(u32*)(s_nestedEntries + 0x5C) = (u32)(uintptr_t)s_nestedTable6;
    *(u32*)(s_nestedEntries + 0x60) = (u32)(uintptr_t)s_nestedTable7;
    *(u32*)(s_nestedEntries + 0x64) = (u32)(uintptr_t)s_nestedTable8;
    g_SystemDataEntries = s_nestedEntries;

    for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
        u8* table;
        u8* nested;

        s_nestedScripts[i][0] = 0x0F;
        s_nestedScripts[i][1] = cases[i].subcode;
        s_nestedScripts[i][2] = (u8)cases[i].entry;
        s_nestedScripts[i][3] = 0;
        setup_standalone_window(window, s_nestedScripts[i], 1);
        func_80033DF0(window);

        table = (u8*)(uintptr_t)*(u32*)(s_nestedEntries + cases[i].table_offset);
        nested = GetStringEntry(table, cases[i].entry);
        check(window_string(window) == (u32)(uintptr_t)(nested + 1),
              cases[i].label);
        check(*(u32*)(window + 0x20) == (u32)(uintptr_t)(s_nestedScripts[i] + 2),
              "nested.table.saved.return.pc");
        check(*(u8*)(uintptr_t)window_string(window) == 0,
              "nested.table.terminator.follows.glyph");
        check(*(u8*)(uintptr_t)(window_string(window) - 1) == cases[i].glyph,
              "nested.table.selected.entry.glyph");
        func_80033DF0(window);
        check(window_string(window) == (u32)(uintptr_t)(s_nestedScripts[i] + 3),
              "nested.table.return.pc.restored");
    }
}

static void test_setup_speed_byte(void)
{
    u8* window;
    s16 stored_width;

    reset_world();
    init_string_table((const u8*)"ABCD\x02", 6);
    check(open_box(8, 0) == 0, "setup.speed8.opens");
    window = box_window(0);
    check(*(u8*)(box_bytes(0) + kBoxSpeed) == 1, "setup.speed8.byte.is.1");
    check(window_speed(window) == 1, "setup.speed8.window+0x68.is.1");

    stored_width = *(s16*)(window + 0x0A);
    check((stored_width & ~1) == 0x18, "setup.window.width.not.width*2+8");
    check(stored_width != (s16)(0x18 * 2 + 8), "setup.window.width.rejects.width*2+8");

    reset_world();
    init_string_table((const u8*)"ABCD\x02", 6);
    check(open_box(6, 0) == 0, "setup.speed6.opens");
    window = box_window(0);
    check(*(u8*)(box_bytes(0) + kBoxSpeed) == 2, "setup.speed-not-8.byte.is.2");
    check(window_speed(window) == 2, "setup.speed-not-8.window+0x68.is.2");

    reset_world();
    init_string_table((const u8*)"ABCD\x02", 6);
    check(open_box(4, 0) == 0, "setup.speed4.opens");
    check(*(u8*)(box_bytes(0) + kBoxSpeed) == 2, "setup.speed4.byte.is.2");
}

static void test_typewriter_one_glyph_per_tick(void)
{
    static const u8 kText[] = { 0x41, 0x42, 0x43, 0x44, 0x02, 0x45 };
    u8* window = s_windowScratch;
    u32 start_ptr;
    u32 after_ptr;
    s16 after_x;

    reset_world();
    setup_standalone_window(window, kText, 1);
    start_ptr = window_string(window);
    check(start_ptr == (u32)(uintptr_t)kText, "typewriter.string.starts.at.first.glyph");

    func_80033DF0(window);

    after_ptr = window_string(window);
    after_x = window_glyph_x(window);
    check(after_ptr == start_ptr + 1, "typewriter.speed1.advances.one.glyph");
    check(after_ptr != start_ptr + 4, "typewriter.speed1.does.not.reach.wait");
    check(*(u8*)(uintptr_t)after_ptr != 0x02, "typewriter.speed1.cursor.not.on.wait");
    check(*(u8*)(uintptr_t)after_ptr == 0x42, "typewriter.speed1.cursor.on.second.glyph");
    check(after_x == (s16)func_80034F98(0, 0x41), "typewriter.speed1.glyph-x.one.glyph");
    check(after_x != 0, "typewriter.speed1.glyph-x.moved");
    check((*(u16*)(window + kWindowFlags) & 0x8) == 0,
          "typewriter.speed1.did.not.hit.page.wait");

    memset(s_windowScratch, 0, sizeof(s_windowScratch));
    setup_standalone_window(window, kText, 2);
    start_ptr = window_string(window);
    func_80033DF0(window);
    after_ptr = window_string(window);
    after_x = window_glyph_x(window);
    check(after_ptr == start_ptr + 2, "typewriter.speed2.advances.two.glyphs");
    check(*(u8*)(uintptr_t)after_ptr == 0x43, "typewriter.speed2.cursor.on.third.glyph");
    check(after_x == (s16)(func_80034F98(0, 0x41) + func_80034F98(0, 0x42)),
          "typewriter.speed2.glyph-x.two.glyphs");
    check(*(u8*)(uintptr_t)after_ptr != 0x02, "typewriter.speed2.does.not.reach.wait");
}

static void test_glyph_widths(void)
{
    static const u8 kTwo[] = { 0x41, 0x02 };
    static const u8 kThree[] = { 0x51, 0x02 };
    u8* window = s_windowScratch;
    s32 width2;
    s32 width3;

    reset_world();
    width2 = func_80034F98(0, 0x41);
    width3 = func_80034F98(0, 0x51);
    check(width2 == 2, "width.single-byte.ordinary.is.2");
    check(width3 == 3, "width.single-byte.wide.is.3");

    setup_standalone_window(window, kTwo, 1);
    func_80033DF0(window);
    check(window_glyph_x(window) == 2, "glyph-x.2-unit.glyph.adds.2");

    memset(s_windowScratch, 0, sizeof(s_windowScratch));
    setup_standalone_window(window, kThree, 1);
    func_80033DF0(window);
    check(window_glyph_x(window) == 3, "glyph-x.3-unit.glyph.adds.3");
}

static void test_control_bytes_halt_tick(void)
{
    static const u8 kNewline[] = { 0x41, 0x01, 0x42, 0x02 };
    static const u8 kWait[] = { 0x41, 0x02, 0x42 };
    u8* window = s_windowScratch;
    u32 start;

    reset_world();
    setup_standalone_window(window, kNewline, 8);
    start = window_string(window);
    func_80033DF0(window);
    check(window_string(window) == start + 2, "newline.consumes.0x01.only");
    check(*(u8*)(uintptr_t)window_string(window) == 0x42,
          "newline.does.not.consume.following.glyph");
    check(window_glyph_x(window) == 0x64, "newline.sets.glyph-x.sentinel");

    memset(s_windowScratch, 0, sizeof(s_windowScratch));
    setup_standalone_window(window, kWait, 8);
    start = window_string(window);
    func_80033DF0(window);
    check(window_string(window) == start + 2, "pagewait.consumes.0x02.only");
    check(*(u8*)(uintptr_t)window_string(window) == 0x42,
          "pagewait.does.not.consume.following.glyph");
    check((*(u16*)(window + kWindowFlags) & 0x8) != 0, "pagewait.sets.wait.flag");
}

static void test_open_timer_gate(void)
{
    static const u8 kText[] = { 0x41, 0x42, 0x43, 0x44, 0x02, 0x00 };
    u8* window;
    u32 ptr_before;
    u32 ptr_after;
    s16 x_before;
    s16 x_after;
    u32 entry_before;
    u16 queue_before;

    reset_world();
    init_string_table(kText, sizeof(kText));
    check(open_box(8, 0) == 0, "timer.setup.opens");
    window = box_window(0);
    g_FieldTextBoxes[0].order = 0;
    g_FieldTextBoxes[0].windowOpenTimer = 4;
    check(*(s16*)(box_bytes(0) + kBoxOpenTimer) == 4, "timer.nonzero.stored");

    ptr_before = window_string(window);
    x_before = window_glyph_x(window);
    entry_before = *(u32*)(box_bytes(0) + kBoxStringEntry);
    queue_before = *(u16*)(window + 0x82);

    pump_once();

    ptr_after = window_string(window);
    x_after = window_glyph_x(window);
    check(ptr_after == ptr_before, "timer.nonzero.string.pointer.unchanged");
    check(x_after == x_before, "timer.nonzero.glyph-x.unchanged");
    check(*(u16*)(window + 0x82) == queue_before,
          "timer.nonzero.does.not.enqueue");
    check(*(u32*)(box_bytes(0) + kBoxStringEntry) == entry_before,
          "timer.nonzero.entry.pointer.unchanged");
    check(*(u8*)(uintptr_t)entry_before == 0x41,
          "timer.nonzero.string.still.at.first.glyph");

    g_FieldTextBoxes[0].windowOpenTimer = 0;
    g_FieldTextBoxes[0].order = 0;
    pump_once();

    ptr_after = window_string(window);
    x_after = window_glyph_x(window);
    check(ptr_after != ptr_before, "timer.zero.string.pointer.advances");
    check(x_after != x_before, "timer.zero.glyph-x.advances");
    check(ptr_after == (u32)(uintptr_t)(s_stringTable + 0x10 + 1),
          "timer.zero.advances.one.glyph.at.speed1");
    check(x_after == 2, "timer.zero.glyph-x.is.one.2-unit.glyph");
    check(*(u8*)(uintptr_t)ptr_after == 0x42, "timer.zero.cursor.on.second.glyph");

    reset_world();
    init_string_table(kText, sizeof(kText));
    check(open_box(6, 0) == 0, "timer.speed2.setup.opens");
    window = box_window(0);
    check(window_speed(window) == 2, "timer.speed2.byte.is.2");
    g_FieldTextBoxes[0].order = 0;
    g_FieldTextBoxes[0].windowOpenTimer = 3;
    ptr_before = window_string(window);
    x_before = window_glyph_x(window);
    pump_once();
    check(window_string(window) == ptr_before, "timer.speed2.nonzero.string.unchanged");
    check(window_glyph_x(window) == x_before, "timer.speed2.nonzero.glyph-x.unchanged");
    g_FieldTextBoxes[0].windowOpenTimer = 0;
    g_FieldTextBoxes[0].order = 0;
    pump_once();
    check(window_string(window) == (u32)(uintptr_t)(s_stringTable + 0x10 + 2),
          "timer.speed2.zero.advances.two.glyphs");
    check(window_glyph_x(window) == 4, "timer.speed2.zero.glyph-x.two.2-unit.glyphs");
}

int main(void)
{
    test_setup_speed_byte();
    test_typewriter_one_glyph_per_tick();
    test_glyph_widths();
    test_control_bytes_halt_tick();
    test_open_timer_gate();
    test_nested_table_offsets();

    if (s_failures != 0 || s_pass != s_total) {
        return EXIT_FAILURE;
    }
    printf("TEXTBOX TIMING CERTIFICATE PASS\n");
    return EXIT_SUCCESS;
}
