/* Actual HD2D constructor registrations + actual menu model. No scene or GL
 * work is permitted while configuring intensities with the master OFF.
 * Pad samples exercise the model API, not SDL/device mapping or rendering;
 * the real virtual-pad runner and rendered overlay checks remain separate. */
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <GL/gl.h>
#include "port_dev_menu.h"
#include "hd2d_integration.h"
#include "hd2d_sprite.h"

int g_GameSceneMapNum = 14;
const PcPortDevWarp g_PcPortDevWarps[] = {{0}};
const PcPortDevFinish g_PcPortDevFinishes[] = {{0}};
const PcPortDevStep g_PcPortDevSteps[] = {{0}};
const int g_PcPortDevWarpCount = 0, g_PcPortDevFinishCount = 0, g_PcPortDevStepCount = 0;
static int menu_enabled = 1;

int xg_plat_config_int(const char *key, int fallback)
{
    return !strcmp(key, "cheats.dev_menu") ? menu_enabled : fallback;
}
void PcPort_CheatQueue(const char *line) { (void)line; abort(); }
int PcPort_QuickCheckpointFieldIsActive(void) { abort(); }
void glGetIntegerv(GLenum name, GLint *data) { (void)name; (void)data; abort(); }
int XgHd2d_Present(const XgHd2dConfig *config, const XgHd2dFrame *frame)
{
    (void)config; (void)frame; abort();
}

static void key(int value) { assert(PcPort_DevMenuKey(value, 1)); }

static void expect_config(const XgHd2dConfig *expected)
{
    assert(!memcmp(expected, PcPort_Hd2dConfiguration(), sizeof *expected));
}

static void expect_intensity(const XgHd2dConfig *original, int effect, float value)
{
    XgHd2dConfig expected = *original;
    expected.effect[effect].intensity = value;
    expect_config(&expected);
}

static void select_row(int target)
{
    int rows = PcPort_DevMenuRows();
    assert(target >= 0 && target < rows);
    for (int i = 0; i < rows && PcPort_DevMenuCursor() != target; ++i)
        key(PC_DEV_DOWN);
    assert(PcPort_DevMenuCursor() == target);
}

static void test_visibility(const XgHd2dConfig *original)
{
    const int sizes[][2] = {{320, 240}, {640, 480}, {1280, 720}, {1920, 1080}, {640, 399}};
    const int rows = PcPort_DevMenuRows();
    assert(rows == 17);
    for (unsigned size = 0; size < sizeof sizes / sizeof sizes[0]; ++size) {
        int width = sizes[size][0], height = sizes[size][1];
        int saw_scroll = 0, needs_scroll = 0;
        select_row(0);
        for (int direction = 0; direction < 2; ++direction) {
            unsigned visited = 0;
            for (int i = 0; i < rows; ++i) {
                PcPortDevMenuLayout layout;
                int cursor = PcPort_DevMenuCursor();
                PcPort_DevMenuGetLayout(width, height, &layout);
                assert(PcPort_DevMenuCursor() == cursor && PcPort_DevMenuRows() == rows);
                assert(layout.text_scale > 0 && layout.row_height >= 7 * layout.text_scale);
                assert(layout.first_row >= 0 && layout.visible_rows > 0);
                assert(layout.first_row <= cursor && cursor < layout.first_row + layout.visible_rows);
                assert(layout.first_row + layout.visible_rows <= rows);
                /* Public layout must place the selected text and the three
                 * footer lines inside the drawable, including after resize. */
                int selected_y = layout.top - layout.row_height * (cursor - layout.first_row + 1);
                int footer_y = layout.top - layout.row_height * (layout.visible_rows + 3);
                assert(selected_y >= 0 && selected_y + 7 * layout.text_scale <= height);
                assert(footer_y >= 4 * layout.text_scale && layout.top + 10 * layout.text_scale <= height);
                assert(width > 16 * layout.text_scale);
                if (layout.first_row > 0) saw_scroll = 1;
                if (layout.visible_rows < rows) needs_scroll = 1;
                visited |= 1u << cursor;
                key(direction ? PC_DEV_UP : PC_DEV_DOWN);
            }
            assert(visited == (1u << rows) - 1 && PcPort_DevMenuCursor() == 0);
        }
        assert(saw_scroll == needs_scroll);
        expect_config(original);
    }
    for (int width = 0; width <= 16; ++width) {
        PcPortDevMenuLayout layout;
        PcPort_DevMenuGetLayout(width, 480, &layout);
        assert(layout.visible_rows == 0);
    }
    const int short_heights[] = {-1, 0, 1, 32, 64};
    for (unsigned i = 0; i < sizeof short_heights / sizeof short_heights[0]; ++i) {
        PcPortDevMenuLayout layout;
        PcPort_DevMenuGetLayout(640, short_heights[i], &layout);
        assert(layout.visible_rows == 0);
    }
    PcPort_DevMenuGetLayout(640, 480, NULL);
    expect_config(original);
    puts("model layout: every real row reachable and visible in both directions; resize/tiny viewport PASS");
}

static int wide_value, wide_calls;
static int wide_get(void) { return wide_value; }
static void wide_set(int value) { wide_value = value; ++wide_calls; }

static void test_integer_bounds(const XgHd2dConfig *original)
{
    int rows = PcPort_DevMenuRows();
    PcPort_DevMenuRegisterEnhancementValue("TEST INTEGER BOUNDS", wide_get, wide_set,
        INT_MIN, INT_MAX, INT_MAX, NULL);
    assert(PcPort_DevMenuRows() == rows + 1 && wide_calls == 0);
    select_row(rows);
    wide_value = INT_MAX - 2;
    key(PC_DEV_RIGHT);
    assert(wide_value == INT_MAX && wide_calls == 1);
    key(PC_DEV_RIGHT);
    assert(wide_value == INT_MAX && wide_calls == 1);
    wide_value = INT_MIN + 2;
    key(PC_DEV_LEFT);
    assert(wide_value == INT_MIN && wide_calls == 2);
    key(PC_DEV_LEFT);
    assert(wide_value == INT_MIN && wide_calls == 2);
    key(PC_DEV_RIGHT);
    assert(wide_value == -1 && wide_calls == 3);
    expect_config(original);
    select_row(0);
    puts("model bounds: full integer range clamps without overflow, wrap or repeated endpoint setters PASS");
}

static unsigned pad_time;
static unsigned pad_sample(unsigned bits, int instance)
{
    pad_time += 16;
    return PcPort_DevMenuPad(bits, instance, bits != 0, pad_time);
}
static void menu_pad(unsigned bits, int instance)
{
    assert(PcPort_DevMenuOpen());
    assert(pad_sample(bits, instance) & PC_DEV_BLOCK_ALL);
}

static void test_pad_model(const XgHd2dConfig *original)
{
    const unsigned down_right = PC_DEV_PAD_DOWN | PC_DEV_PAD_RIGHT;
    const unsigned up_left = PC_DEV_PAD_UP | PC_DEV_PAD_LEFT;
    select_row(9); /* First actual HD2D intensity. */
    menu_pad(0, 41);
    menu_pad(down_right, 41);
    assert(PcPort_DevMenuCursor() == 10);
    expect_config(original);
    for (int i = 0; i < 40; ++i) menu_pad(down_right, 41);
    assert(PcPort_DevMenuCursor() == 10);
    expect_config(original);
    menu_pad(PC_DEV_PAD_DOWN, 41);
    menu_pad(down_right, 41); /* A fresh horizontal edge, vertical still held. */
    assert(PcPort_DevMenuCursor() == 10);
    expect_config(original);
    menu_pad(0, 41);
    menu_pad(up_left, 41);
    assert(PcPort_DevMenuCursor() == 9);
    expect_config(original);
    menu_pad(0, 41);
    menu_pad(PC_DEV_PAD_LEFT | PC_DEV_PAD_RIGHT, 41);
    expect_config(original);
    menu_pad(0, 41);
    menu_pad(PC_DEV_PAD_RIGHT, 41);
    expect_intensity(original, XG_HD2D_DOF, 0.30f);
    for (int i = 0; i < 40; ++i) menu_pad(PC_DEV_PAD_RIGHT, 41);
    expect_intensity(original, XG_HD2D_DOF, 0.30f);
    menu_pad(0, 41);
    menu_pad(PC_DEV_PAD_LEFT, 41);
    expect_config(original);
    menu_pad(0, 41);
    menu_pad(PC_DEV_PAD_RIGHT, 42); /* New pad appears with right held. */
    menu_pad(PC_DEV_PAD_RIGHT, 42);
    expect_config(original);
    menu_pad(0, 42);
    menu_pad(PC_DEV_PAD_RIGHT, 42);
    expect_intensity(original, XG_HD2D_DOF, 0.30f);
    menu_pad(0, 42);
    menu_pad(PC_DEV_PAD_LEFT, 42);
    expect_config(original);
    key(PC_DEV_OPEN); /* Close with Left held: no input may escape the drain. */
    assert(!PcPort_DevMenuOpen());
    assert(pad_sample(PC_DEV_PAD_LEFT, 42) & PC_DEV_BLOCK_ALL);
    assert(pad_sample(0, 42) & PC_DEV_BLOCK_ALL);
    assert(pad_sample(0, 42) == PC_DEV_PASS);
    assert(!PcPort_DevMenuKey(PC_DEV_RIGHT, 1));
    expect_config(original);
    menu_enabled = 0;
    PcPort_DevMenuInit();
    assert(!PcPort_DevMenuEnabled() && !PcPort_DevMenuKey(PC_DEV_OPEN, 1));
    assert(pad_sample(PC_DEV_PAD_RIGHT, 42) == PC_DEV_PASS);
    expect_config(original);
    puts("model pad: diagonal priority, held edges, reconnect neutral, capture/drain and disabled identity PASS");
}

int main(void)
{
    XgHd2dConfig original = *PcPort_Hd2dConfiguration();
    assert(!original.enabled);
    assert(PcPort_DevMenuEnhancementCount() == 17);
    PcPort_DevMenuInit();
    key(PC_DEV_OPEN);
    for (int i = 0; i < 3; ++i) key(PC_DEV_DOWN);
    key(PC_DEV_ACCEPT);
    key(PC_DEV_UP);
    key(PC_DEV_ACCEPT);
    assert(!strcmp(PcPort_DevMenuTitle(), "ENHANCEMENTS"));
    assert(PcPort_DevMenuRows() == 17);
    assert(!strcmp(PcPort_DevMenuRow(0), "HD-2D (Octopath style): OFF"));
    for (int i = 0; i < 9; ++i) key(PC_DEV_DOWN);
    for (int effect = 0; effect < XG_HD2D_EFFECT_COUNT; ++effect) {
        assert(PcPort_DevMenuCursor() == 9 + effect);
        assert(PcPort_DevMenuRowAdjustable(9 + effect));
        assert(strstr(PcPort_DevMenuRow(9 + effect), ": 25%"));
        key(PC_DEV_RIGHT);
        const XgHd2dConfig *changed = PcPort_Hd2dConfiguration();
        assert(!changed->enabled && changed->effect[effect].intensity == 0.30f);
        for (int other = 0; other < XG_HD2D_EFFECT_COUNT; ++other) {
            assert(changed->effect[other].enabled == original.effect[other].enabled);
            if (other != effect)
                assert(changed->effect[other].intensity == original.effect[other].intensity);
        }
        assert(strstr(PcPort_DevMenuRow(9 + effect), ": 30%"));
        key(PC_DEV_LEFT);
        expect_config(&original);
        assert(PcPort_Hd2dSetIntensity(effect, 0.98f));
        key(PC_DEV_RIGHT);
        expect_intensity(&original, effect, 1.0f);
        assert(strstr(PcPort_DevMenuRow(9 + effect), ": 100%"));
        key(PC_DEV_RIGHT);
        expect_intensity(&original, effect, 1.0f);
        key(PC_DEV_LEFT);
        expect_intensity(&original, effect, 0.95f);
        assert(PcPort_Hd2dSetIntensity(effect, 0.02f));
        key(PC_DEV_LEFT);
        expect_intensity(&original, effect, 0.0f);
        assert(strstr(PcPort_DevMenuRow(9 + effect), ": 0%"));
        key(PC_DEV_LEFT);
        expect_intensity(&original, effect, 0.0f);
        key(PC_DEV_ACCEPT);
        expect_intensity(&original, effect, 0.05f);
        assert(PcPort_DevMenuOpen());
        assert(PcPort_DevMenuKey(PC_DEV_LEFT, 0));
        expect_intensity(&original, effect, 0.05f);
        assert(PcPort_Hd2dSetIntensity(effect, original.effect[effect].intensity));
        expect_config(&original);
        key(PC_DEV_DOWN);
    }
    assert(PcPort_DevMenuCursor() == 0);
    test_visibility(&original);
    test_integer_bounds(&original);
    test_pad_model(&original);
    PcPort_Hd2dPostProcess(0, 640, 480); /* OFF must still bypass every host hook. */
    puts("dev_menu_enhancement_test: 17 real rows, eight isolated bounded controls, layout/pad model and master OFF identity PASS");
    return 0;
}
