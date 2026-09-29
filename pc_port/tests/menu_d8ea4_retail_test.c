#include "common.h"
#include "main/game.h"
#include "system/menu.h"
#include <stdint.h>
#include <stdio.h>
#include <sys/mman.h>

enum {
    EV_ALLOC,
    EV_WEAPON,
    EV_ACCESSORY,
    EV_A2C,
    EV_A5C,
    EV_RENDER,
    EV_LOAD,
    EV_DRAW,
    EV_STRING,
    EV_TPAGE,
    EV_VERTS,
    EV_FREE,
};

typedef struct {
    int kind;
    intptr_t a;
    intptr_t b;
    intptr_t c;
    intptr_t d;
} TestEvent;

SystemMenu* g_Menu;
GameState g_GameState;
u8 D_801EA584[12];
u16 D_801EA5D0[6];
u16 D_801E9D88[26];
u16 g_SystemPalette1 = 0x1111;
u16 g_SystemPalette2 = 0x2222;

static SystemMenu s_menu;
static MenuManager s_manager;
static u8 s_render_buffer[0x3F6];
static u8* s_mapping;
static u8* s_work;
static TestEvent s_events[256];
static int s_event_count;
static int s_render_count;

extern void func_801D8EA4(s32 slot, s32 mode, s32 buildText,
                           s32 renderSlot);

static void fill(void* pointer, u8 value, size_t size) {
    u8* bytes = pointer;
    size_t i;

    for (i = 0; i < size; i++) bytes[i] = value;
}

static void event(int kind, intptr_t a, intptr_t b, intptr_t c, intptr_t d) {
    assert(s_event_count < (int)(sizeof(s_events) / sizeof(s_events[0])));
    s_events[s_event_count++] = (TestEvent){kind, a, b, c, d};
}

static void* name_token(int kind, int index) {
    return (void*)(uintptr_t)(0x10000 + kind * 0x100 + index);
}

void* HeapAlloc(u_int size, u_int flags) {
    event(EV_ALLOC, size, flags, 0, 0);
    assert(size == 0x3F6);
    assert(flags == 0);
    fill(s_render_buffer, 0, sizeof(s_render_buffer));
    return s_render_buffer;
}

u_int HeapFree(void* pointer) {
    event(EV_FREE, (intptr_t)pointer, 0, 0, 0);
    assert(pointer == s_render_buffer);
    return 0;
}

void* GetWeaponName(s32 index) {
    event(EV_WEAPON, index, 0, 0, 0);
    return name_token(EV_WEAPON, index);
}

void* GetAccessoryName(s32 index) {
    event(EV_ACCESSORY, index, 0, 0, 0);
    return name_token(EV_ACCESSORY, index);
}

void* func_80033A2C(s32 index) {
    event(EV_A2C, index, 0, 0, 0);
    return name_token(EV_A2C, index);
}

void* func_80033A5C(s32 index) {
    event(EV_A5C, index, 0, 0, 0);
    return name_token(EV_A5C, index);
}

s32 SystemRenderStringEntry(void* string, void* work, s32 height, s32 field) {
    s32 width = 0x40 + s_render_count++;
    event(EV_RENDER, (intptr_t)string, (intptr_t)work, height, field);
    assert(work == s_render_buffer);
    assert(height == 0x24);
    return width;
}

int LoadImage(RECT16* rect, u_long* pixels) {
    event(EV_LOAD, rect->x, rect->y, rect->w, rect->h);
    assert((void*)pixels == s_render_buffer);
    return 0;
}

int DrawSync(int mode) {
    event(EV_DRAW, mode, 0, 0, 0);
    return 0;
}

u_short GetTPage(int tp, int abr, int x, int y) {
    event(EV_TPAGE, tp, abr, x, y);
    return 0x1234;
}

void func_801E7C50(MenuString* string, s32 index, s32 yOffset, s32 style) {
    uintptr_t p = (uintptr_t)string;

    /* D8EA4 must pass its native staging record, never a retail work row. */
    assert(p < (uintptr_t)s_work || p >= (uintptr_t)(s_work + 0x2AC));
    event(EV_STRING, index, yOffset, style, string->width);
    string->polys[0].r0 = (u8)(0xA0 + index);
    string->polys[1].b0 = (u8)(0xB0 + index);
    string->unk7C = (u8)(index & 1);
    string->unk7F = 0;
}

void func_801C851C(SVECTOR* vertices, s32 x, s32 y, s32 w, s32 h) {
    intptr_t offset = (u8*)vertices - s_work;

    event(EV_VERTS, offset, x, y, ((w & 0xFFFF) << 16) | (h & 0xFFFF));
    vertices[0].vx = (s16)x;
    vertices[0].vy = (s16)y;
    vertices[1].vx = (s16)(x + w);
    vertices[1].vy = (s16)y;
    vertices[2].vx = (s16)x;
    vertices[2].vy = (s16)(y + h);
    vertices[3].vx = (s16)(x + w);
    vertices[3].vy = (s16)(y + h);
}

static void expect_event(int* cursor, int kind, intptr_t a, intptr_t b,
                         intptr_t c, intptr_t d) {
    TestEvent* got = &s_events[(*cursor)++];
    assert(got->kind == kind);
    assert(got->a == a);
    assert(got->b == b);
    assert(got->c == c);
    assert(got->d == d);
}

static int resolver_for_row(int mode, int renderSlot, int row,
                            const u8* first, const u8* second,
                            const u8* accessories, int* index) {
    if (row == 0) {
        *index = mode == 2 ? second[0] : first[0];
        return renderSlot ? EV_A5C : EV_WEAPON;
    }
    if (row == 4 && !renderSlot) {
        return -1;
    }
    if (mode < 2) {
        if (!renderSlot) {
            *index = accessories[row - 1];
            return EV_ACCESSORY;
        }
        if (mode == 0) {
            if (row == 1) {
                *index = first[3];
                return EV_A5C;
            }
            *index = accessories[row - 2];
            return EV_A2C;
        }
        *index = accessories[row - 1];
        return EV_A2C;
    }
    *index = second[row];
    return renderSlot ? EV_A5C : EV_WEAPON;
}

static void run_case(int mode, int renderSlot, int buildText) {
    const size_t page = 4096;
    u8* character;
    u8* gear;
    u8* first;
    u8* second;
    u8* accessories;
    int rowCount = mode ? 4 : 5;
    int labelOffset = mode == 1 ? 5 : mode ? 9 : 0;
    int baseX = mode ? 0x28 : 0xD0;
    int renderOrdinal = 0;
    int cursor = 0;
    int upload = 0;
    int row;

    fill(&s_menu, 0, sizeof(s_menu));
    fill(&s_manager, 0, sizeof(s_manager));
    fill(&g_GameState, 0, sizeof(g_GameState));
    fill(s_mapping, 0xCD, page);
    s_work = s_mapping + 0x100;
    fill(s_work, 0, 0x2AC);
    for (row = 0; row < 5; row++) {
        int byte;

        for (byte = 0x70; byte < 0x7C; byte++) {
            s_work[row * 0x80 + byte] = 0x5A;
        }
        s_work[row * 0x80 + 0x7D] = 0x5A;
    }
    g_Menu = &s_menu;
    g_Menu->pManager = &s_manager;
    g_Menu->renderContext = 1;
    g_Menu->pManager->currentCharacterIDs[1] = 2;

    character = (u8*)&g_GameState.characters[2];
    character[0xA0] = 3;
    first = character + 0x6A;
    second = character + 0x6F;
    accessories = character + 0x74;
    for (row = 0; row < 5; row++) {
        first[row] = (u8)(10 + row);
        second[row] = (u8)(20 + row);
    }
    for (row = 0; row < 3; row++) accessories[row] = (u8)(30 + row);

    gear = (u8*)&g_GameState.gears[3];
    for (row = 0; row < 5; row++) {
        gear[0xC + row] = (u8)(40 + row);
        gear[4 + row] = (u8)(50 + row);
    }
    for (row = 0; row < 3; row++) gear[9 + row] = (u8)(60 + row);

    for (row = 0; row < 5; row++) {
        s_work[0x29C + row] = (u8)(70 + row);
        s_work[0x2A1 + row] = (u8)(80 + row);
    }
    for (row = 0; row < 3; row++) s_work[0x2A6 + row] = (u8)(90 + row);

    for (row = 0; row < 26; row++) D_801E9D88[row] = (u16)(100 + row);
    *(s32*)&D_801EA584[4] = 7;
    ((u8*)D_801EA5D0)[4] = 9;

    /* Adjacent four-byte slots plus a nonzero following word make any old
     * eight-byte load at +0x360 observably invalid on x64. */
    *(u32*)((u8*)g_Menu + 0x358) = 0x11111111;
    *(u32*)((u8*)g_Menu + 0x35C) = 0x22222222;
    *(u32*)((u8*)g_Menu + 0x360) = (u32)(uintptr_t)s_work;
    *(u32*)((u8*)g_Menu + 0x364) = 0x33333333;

    if (renderSlot) {
        first = gear + 0xC;
        second = gear + 4;
        accessories = gear + 9;
    }
    if (buildText) {
        first = s_work + 0x29C;
        second = s_work + 0x2A1;
        accessories = s_work + 0x2A6;
    }

    s_event_count = 0;
    s_render_count = 0;
    func_801D8EA4(1, mode, buildText, renderSlot);

    expect_event(&cursor, EV_ALLOC, 0x3F6, 0, 0, 0);
    for (row = 0; row < rowCount; row++) {
        int resolverIndex = 0;
        int resolver = resolver_for_row(mode, renderSlot, row, first, second,
                                        accessories, &resolverIndex);
        int width;

        if (resolver >= 0) {
            intptr_t token = (intptr_t)name_token(resolver, resolverIndex);
            expect_event(&cursor, resolver, resolverIndex, 0, 0, 0);
            expect_event(&cursor, EV_RENDER, token, (intptr_t)s_render_buffer,
                         0x24, row & 1);
            width = 0x40 + renderOrdinal++;
        } else {
            width = 0x60;
        }

        if (row & 1) upload = 1;
        else if (!renderSlot) upload = 0;
        else if (mode == 0 && row == 4) upload = 1;
        if (upload) {
            expect_event(&cursor, EV_LOAD,
                         0x140 + ((row << 4) & 0x20),
                         0x27 + (row / 4) * 0xD, 0x28, 0xD);
            expect_event(&cursor, EV_DRAW, 0, 0, 0, 0);
        }

        expect_event(&cursor, EV_STRING, row, 0xC, 0,
                     resolver >= 0 ? width : 0);
        if (row == 4 && !renderSlot) {
            expect_event(&cursor, EV_TPAGE, 0, 0, 0x180, 0);
        }
        /* Retail reads the row-Y table at word stride (sll 2 + lhu); the
         * halfword array below models the low half of each word. */
        expect_event(&cursor, EV_VERTS, row * 0x80 + 0x50, baseX,
                     D_801E9D88[(labelOffset + row) * 2],
                     ((width & 0xFFFF) << 16) | 0xD);

        assert(s_work[row * 0x80 + 0x7E] == width);
        assert(s_work[row * 0x80 + 0x04] == (u8)(0xA0 + row));
        assert(s_work[row * 0x80 + 0x2E] == (u8)(0xB0 + row));
        assert(s_work[row * 0x80 + 0x7C] == (u8)(row & 1));
        assert(s_work[row * 0x80 + 0x7D] == 0x5A);
        assert(s_work[row * 0x80 + 0x7F] == 0);
        assert(s_work[0x294 + row] == 1);
        {
            SVECTOR* vertices = (SVECTOR*)(s_work + row * 0x80 + 0x50);
            int y = D_801E9D88[(labelOffset + row) * 2];

            assert(vertices[0].vx == baseX && vertices[0].vy == y);
            assert(vertices[1].vx == baseX + width && vertices[1].vy == y);
            assert(vertices[2].vx == baseX && vertices[2].vy == y + 0xD);
            assert(vertices[3].vx == baseX + width &&
                   vertices[3].vy == y + 0xD);
        }
        {
            int byte;

            for (byte = 0x70; byte < 0x7C; byte++) {
                assert(s_work[row * 0x80 + byte] == 0x5A);
            }
        }
    }
    expect_event(&cursor, EV_FREE, (intptr_t)s_render_buffer, 0, 0, 0);
    assert(cursor == s_event_count);

    assert(s_work[0x299] == 1);
    assert(g_Menu->pManager->unk4A[1] == 1);
    assert(*(u32*)((u8*)g_Menu + 0x358) == 0x11111111);
    assert(*(u32*)((u8*)g_Menu + 0x35C) == 0x22222222);
    assert(*(u32*)((u8*)g_Menu + 0x360) == (u32)(uintptr_t)s_work);
    assert(*(u32*)((u8*)g_Menu + 0x364) == 0x33333333);
    if (mode == 0 && renderSlot == 0) {
        u8* poly = s_work + 0x200 + 0x28;

        assert(*(u16*)(poly + 0x16) == 0x1234);
        assert(*(u16*)(poly + 0x0E) == g_SystemPalette1);
        assert(poly[0x0C] == 0x1C && poly[0x0D] == 9);
        assert(poly[0x14] == 0x7C && poly[0x15] == 9);
        assert(poly[0x1C] == 0x1C && poly[0x1D] == 0x16);
        assert(poly[0x24] == 0x7C && poly[0x25] == 0x16);
    }
    for (row = 0; row < 0x100; row++) assert(s_mapping[row] == 0xCD);
    for (row = 0x3AC; row < (int)page; row++) assert(s_mapping[row] == 0xCD);
}

int main(void) {
    int mode;
    int renderSlot;
    int buildText;

    s_mapping = mmap(NULL, 4096, PROT_READ | PROT_WRITE,
                     MAP_PRIVATE | MAP_ANONYMOUS | MAP_32BIT, -1, 0);
    assert(s_mapping != MAP_FAILED);
    assert((uintptr_t)s_mapping <= UINT32_MAX);

    for (buildText = 0; buildText < 2; buildText++) {
        for (mode = 0; mode < 3; mode++) {
            for (renderSlot = 0; renderSlot < 2; renderSlot++) {
                run_case(mode, renderSlot, buildText);
            }
        }
    }

    munmap(s_mapping, 4096);
    puts("menu D8EA4 production-body behavior: PASS (12 cases)");
    return 0;
}
