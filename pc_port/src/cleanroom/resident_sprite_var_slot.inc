/* The selector addresses two different storage representations in the sprite:
 * a byte-relative native slot, or an offset added to a 32-bit address word. */
typedef u32 SpriteVariableAddress __attribute__((__may_alias__, __aligned__(1)));

void *func_8001FBA4(SpriteData *sprite, u8 *selector)
{
    unsigned int slot = *(const volatile u8 *)selector;
    const volatile u8 *record = (const volatile u8 *)sprite;
    if (!(slot & 0x80u)) {
        int bias = *(const volatile s8 *)(record + 0x8c);
        return (u8 *)sprite + (0x8e + bias) + slot;
    }
    u32 address = *(const volatile SpriteVariableAddress *)(record + 0x88);
    return (void *)(uintptr_t)(address + (slot & 0x7fu));
}
