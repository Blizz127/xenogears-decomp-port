#include "field/graphics.h"
#include "field/main.h"
#include "system/memory.h"

extern u32 D_800AFC68;

void func_800AABD8(void) {
    HeapFree((void *)(uintptr_t)D_800AFC68);
    DrawSync(0);
}

static void FieldWriteSpriteColor(u32 offset, u8 value) {
    u32 address = *(volatile u32 *)&D_800AFC68 + offset;
    *(volatile u8 *)(uintptr_t)address = value;
}

void func_800AADC8(int index, int red, int green, int blue) {
    u32 sprite = (u32)__builtin_offsetof(SpriteList, sprites)
               + (u32)index * (2u * (u32)sizeof(SPRT));
    for (int bank = 0; bank < 2; ++bank) {
        FieldWriteSpriteColor(sprite + __builtin_offsetof(SPRT, r0), (u8)red);
        FieldWriteSpriteColor(sprite + __builtin_offsetof(SPRT, g0), (u8)green);
        FieldWriteSpriteColor(sprite + __builtin_offsetof(SPRT, b0), (u8)blue);
        sprite += (u32)sizeof(SPRT);
    }
}

void func_800AAE4C(int index, int x, int y, int mode) {
    u32 bank = (u32)g_FieldCurRenderContextIndex;
    u32 table = D_800AFC68;
    u_long *head = FIELD_CTX_OT3(g_FieldCurRenderContext);
    u32 element = (u32)index * 2u + bank;
    u32 spriteAddress = table + (u32)__builtin_offsetof(SpriteList, sprites)
                      + element * (u32)sizeof(SPRT);
    u32 drawModeAddress = table + element * (u32)sizeof(DR_MODE);
    SPRT *sprite = (SPRT *)(uintptr_t)spriteAddress;
    DR_MODE *drawMode = (DR_MODE *)(uintptr_t)drawModeAddress;
    u32 px = (u32)x;
    u32 py = (u32)y;
    if (mode == 0 || mode == 1) {
        px -= 4;
        py -= mode == 0 ? 12u : 4u;
    }
    ((volatile SPRT *)sprite)->x0 = (short)px;
    ((volatile SPRT *)sprite)->y0 = (short)py;
    addPrim(head, sprite);
    addPrim(head, drawMode);
}
