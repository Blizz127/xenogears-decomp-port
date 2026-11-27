extern void *func_8001FBA4(SpriteData *, u8 *);

static void *SpriteTickVarSlot(u8 *sprite, const u8 *selector)
{
    return func_8001FBA4((SpriteData *)sprite, (u8 *)selector);
}
