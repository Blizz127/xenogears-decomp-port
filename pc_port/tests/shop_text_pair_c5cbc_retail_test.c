#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "system/menu.h"

extern void func_801C5CBC(void*, u8*, s32, s32);
extern void func_801C5EE8(void);
u8 D_801D2018[] = {3, 9, 1, 8};
SystemMenu* g_Menu;

static SystemMenu menu;
static MenuString* output;
static u8 work[0x38E];
static u16 palette[16];
static u8 bundle[0x40];
static unsigned checks;
static s32 expected_offset;
static unsigned string_calls;
static unsigned render_calls;
static unsigned shape_calls;
static unsigned upload_calls;
static s32 rendered_indices[8];
static s32 shaped_indices[8];
static s32 shaped_offsets[8];

static void check(int ok, const char* what, s32 value);

void* GetStringEntry(void* pBundle, s32 index)
{
    check(pBundle == bundle, "bundle", index);
    rendered_indices[string_calls++] = index;
    return bundle + index;
}

s32 SystemRenderStringEntry(void* string, void* pWork, s32 height, s32 flag)
{
    s32 index = (s32)((u8*)string - bundle);
    check(pWork == work && height == 0x18, "render args", index);
    check(flag == (s32)(render_calls & 1), "render flag", index);
    render_calls++;
    return 0x20 + index;
}

void func_801C5A7C(MenuString* pString, s32 index, s32 offset, u8 flags)
{
    check(pString >= output && pString < output + 4, "shape pointer", index);
    check(flags == 0, "shape flags", index);
    check(render_calls == (unsigned)((index / 2 + 1) * 2), "both planes before shape", index);
    shaped_indices[shape_calls] = index;
    shaped_offsets[shape_calls++] = offset;
}

int LoadImage(RECT* rect, u_long* data)
{
    if (data == (u_long*)palette) return 0;
    check(rect == &output[upload_calls * 2].vramDest, "upload rect", upload_calls);
    check(rect->x == 320 + 32 * (s32)upload_calls && rect->y == ((2 * (s32)upload_calls + expected_offset) / 4) * 13 && rect->w == 28 && rect->h == 13, "retail upload geometry", upload_calls);
    check(memcmp(rect, &output[upload_calls * 2 + 1].vramDest, sizeof(*rect)) == 0, "paired rectangle", upload_calls);
    check((u8*)data == work, "upload buffer", upload_calls);
    upload_calls++;
    return 0;
}

int DrawSync(int mode)
{
    check(mode == 0, "draw sync", mode);
    return 0;
}

void check(int ok, const char* what, s32 value)
{
    ++checks;
    if (!ok) {
        fprintf(stderr, "SHOP C5CBC FAIL %s value=%d\n", what, (int)value);
        exit(1);
    }
}

void* HeapAlloc(u_int size, u_int flags) {
    check(flags == 0, "allocation flags", flags);
    check(size == sizeof(work) || size == sizeof(palette), "allocation size", size);
    return size == sizeof(work) ? (void*)work : (void*)palette;
}
u_int HeapFree(void* p) { check(p == palette, "palette free", 0); return 0; }
void SystemTransferPaletteToVRAM(s32 a, s32 b) { check(a == 0 && b == 0x1d1, "palette transfer", b); }

int main(void)
{
    unsigned i;
    memset(&menu, 0, sizeof(menu));
    memset(work, 0x5a, sizeof(work));
    memset(bundle, 0, sizeof(bundle));
    g_Menu = &menu;
    g_Menu->unk2E0 = bundle;
    output = g_Menu->unk4E0;
    func_801C5EE8();

    check(string_calls == 4, "string call count", string_calls);
    check(render_calls == 4, "render call count", render_calls);
    check(shape_calls == 4, "shape call count", shape_calls);
    check(upload_calls == 2, "upload call count", upload_calls);
    check(rendered_indices[0] == 3 && rendered_indices[1] == 9 &&
          rendered_indices[2] == 1 && rendered_indices[3] == 8, "string IDs", 0);
    check(shaped_indices[0] == 0 && shaped_indices[1] == 1 &&
          shaped_indices[2] == 2 && shaped_indices[3] == 3, "shape indices", 0);
    for (i = 0; i < 4; i++) {
        check(shaped_offsets[i] == 0, "shape offset", shaped_offsets[i]);
    }
    check(output[0].width == 0x23 && output[1].width == 0x29 &&
          output[2].width == 0x21 && output[3].width == 0x28, "native pair widths", 0);
    check(g_Menu->unk4E0[0].pVramBuffer == work, "initialized work pointer", 0);
    /* Preserve the separate nonzero-offset case used by other shop windows. */
    string_calls = render_calls = shape_calls = upload_calls = 0;
    expected_offset = 7;
    func_801C5CBC(output, D_801D2018, expected_offset, 4);
    check(upload_calls == 2 && render_calls == 4 && shape_calls == 4,
          "offset pair counts", 0);
    for (i = 0; i < 4; ++i)
        check(shaped_offsets[i] == expected_offset, "nonzero shape offset", i);
    printf("SHOP TEXT PAIR C5CBC certificate PASS checks=%u\n", checks);
    return 0;
}
