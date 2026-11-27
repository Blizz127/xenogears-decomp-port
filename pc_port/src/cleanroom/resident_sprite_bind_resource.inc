void func_800222BC(SpriteData *sprite, SpriteAnimPackageFileHeader *resource)
{
    /* These records use the 32-bit guest layout, including the coordinate pair
     * at offset 0x0e of the transform. */
    typedef u32 PackedWord __attribute__((__may_alias__, __aligned__(1)));
    typedef s16 PackedCoordinate __attribute__((__may_alias__, __aligned__(1)));

    u8 *bytes = (u8 *)sprite;
    u32 recordAddress = *(const volatile PackedWord *)(bytes + 0x24);
    u8 *record = (u8 *)AnimWordPtr(recordAddress);
    if (!resource)
        return;

    volatile PackedWord *binding = (volatile PackedWord *)(bytes + 0x44);
    if (resource != (SpriteAnimPackageFileHeader *)AnimWordPtr(*binding)) {
        volatile PackedWord *flags = (volatile PackedWord *)(bytes + 0x3c);
        u32 selector = (*flags >> 20) & 15u;
        SVEC2 tex, clut;
        tex.x = *(const volatile PackedCoordinate *)(record + 4);
        clut.x = *(const volatile PackedCoordinate *)(record + 8);
        tex.y = *(const volatile PackedCoordinate *)(record + 6);
        clut.y = *(const volatile PackedCoordinate *)(record + 10);
        func_80022224((SpriteAnimPackage *)record, resource, tex, clut, selector);

        u32 currentFlags = *flags;
        *binding = (u32)(uintptr_t)resource;
        *flags = currentFlags | (1u << 30);
    }

    if (!*(const volatile u8 *)&D_800591AD)
        return;

    u32 frames = *(const volatile PackedWord *)record;
    if (func_8001EE68(AnimWordPtr(frames)) == 0) {
        *(volatile PackedCoordinate *)(record + 6) = 0x100;
        *(volatile PackedCoordinate *)(record + 4) = 0x300;
    } else {
        u32 transformAddress = *(const volatile PackedWord *)(bytes + 0x7c);
        const u8 *transform = (const u8 *)AnimWordPtr(transformAddress);
        *(volatile PackedWord *)(record + 4) = *(const volatile PackedWord *)(transform + 0x0e);
    }
}
