#include "field/graphics.h"
#include "field/main.h"

extern s16 D_800ADB54;
extern SpriteList3 D_800B0188;

static void *FieldFixedSpritePacket(u32 offset) {
    uintptr_t base = (uintptr_t)&D_800B0188;
    if (offset <= 0x7fffffffu)
        return (void *)(base + offset);
    return (void *)(base - (0u - offset));
}

void func_800ABEC8(void) {
    if (D_800ADB54 == 0)
        return;

    u32 bank = (u32)g_FieldCurRenderContextIndex;
    u_long *head = FIELD_CTX_OT3(g_FieldCurRenderContext);
    for (u32 index = 0; index < 5; ++index) {
        u32 element = index * 2u + bank;
        u32 spriteOffset = (u32)__builtin_offsetof(SpriteList3, sprites)
                         + element * (u32)sizeof(SPRT);
        SPRT *sprite = FieldFixedSpritePacket(spriteOffset);
        DR_MODE *drawMode = FieldFixedSpritePacket(element * (u32)sizeof(DR_MODE));
        addPrim(head, sprite);
        addPrim(head, drawMode);
    }
}
