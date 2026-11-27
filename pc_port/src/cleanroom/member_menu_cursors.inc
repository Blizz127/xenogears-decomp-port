#include <stddef.h>
#include <stdint.h>
#include "system/memory.h"

static SystemMenu* MemberChangeCursorMenu(void)
{
    return *(SystemMenu* volatile*)&g_Menu;
}

static POLY_FT4* MemberChangeCursorPage(MenuPointerCursors* owner, unsigned slot)
{
    unsigned page = ((volatile MenuPointerCursors*)owner)->renderContexts[slot];
    uintptr_t packets = (uintptr_t)&owner->polysCursor[0];
    return (POLY_FT4*)(packets + sizeof(POLY_FT4) * (2u * slot + page));
}

static void MemberChangeCursorCoordinate(volatile SystemMenu* menu, unsigned slot,
                                         size_t member, const volatile s32* positions, int delta)
{
    MenuPointerCursors* owner = menu->pCursors;
    POLY_FT4* packet = MemberChangeCursorPage(owner, slot);
    int selector = ((volatile MenuUnk2*)menu->unk32C)->unk4F7C;
    s32 position = ((const volatile s32*)D_801CB404)[selector];
    u16 coordinate = (u16)positions[position];
    *(volatile u16*)((unsigned char*)packet + member) = (u16)(coordinate + delta);
}

void MemberChangeMenuDrawCursors(void)
{
    if (!MemberChangeCursorMenu()->pManager->shouldRenderPointerCursors)
        return;
    for (unsigned slot = 0; slot < 4; ++slot) {
        SystemMenu* menu = MemberChangeCursorMenu();
        MenuPointerCursors* owner = menu->pCursors;
        if (!owner->shouldRender[slot])
            continue;
        if (owner->unk144[slot]) {
            MemberChangeCursorCoordinate(menu, slot, __builtin_offsetof(POLY_FT4, x0), D_801CB47C, 8);
            MemberChangeCursorCoordinate(menu, slot, __builtin_offsetof(POLY_FT4, y0), D_801CB4FC, -6);
            MemberChangeCursorCoordinate(menu, slot, __builtin_offsetof(POLY_FT4, x1), D_801CB47C, 24);
            MemberChangeCursorCoordinate(menu, slot, __builtin_offsetof(POLY_FT4, y1), D_801CB4FC, -6);
            MemberChangeCursorCoordinate(menu, slot, __builtin_offsetof(POLY_FT4, x2), D_801CB47C, 8);
            MemberChangeCursorCoordinate(menu, slot, __builtin_offsetof(POLY_FT4, y2), D_801CB4FC, 10);
            MemberChangeCursorCoordinate(menu, slot, __builtin_offsetof(POLY_FT4, x3), D_801CB47C, 24);
            MemberChangeCursorCoordinate(menu, slot, __builtin_offsetof(POLY_FT4, y3), D_801CB4FC, 10);
        }
        SystemMenu* submit = MemberChangeCursorMenu();
        AddPrim(&submit->pGfxEnv->ot[4], MemberChangeCursorPage(submit->pCursors, slot));
    }
}

void MemberChangeMenuInitializeCursors(u_char mode)
{
    MenuPointerCursors* owner = HeapAlloc(sizeof(*owner), 0);
    MemberChangeCursorMenu()->pCursors = owner;
    bzero(owner, sizeof(*owner));

    unsigned count;
    switch (mode) {
    case 0:
        ((volatile MenuManager*)MemberChangeCursorMenu()->pManager)->shouldRenderPointerCursors = 1;
        ((volatile MenuPointerCursors*)MemberChangeCursorMenu()->pCursors)->unk144[0] = 1;
        ((volatile MenuPointerCursors*)MemberChangeCursorMenu()->pCursors)->unk144[1] = 1;
        count = 4;
        break;
    case 2:
        count = 4;
        break;
    case 3:
        count = 1;
        break;
    default:
        return;
    }
    for (unsigned slot = 0; slot < count; ++slot) {
        SystemMenu* menu = MemberChangeCursorMenu();
        int x = mode == 3 ? 0 : D_801CB180[slot];
        int y = mode == 3 ? 0 : D_801CB190[slot];
        func_8002675C(menu->unk2DC, 264, &menu->pCursors->polysCursor[2 * slot],
                      menu->renderContext, x, y, 2048);
        SystemMenu* current = MemberChangeCursorMenu();
        current->pCursors->renderContexts[slot] = (u_char)current->renderContext;
    }
    if (mode == 3)
        ((volatile MenuManager*)MemberChangeCursorMenu()->pManager)->shouldRenderPointerCursors = 1;
}
