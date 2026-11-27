/* Sprite package selection and inherited model directions. */
static int animationRelations(uint8_t *actor, uint8_t opcode, uint8_t *args)
{
    (void)args;
    switch (opcode) {
    case 0x90: {
        uint32_t primary = AnimationRead32(actor + 0x48);
        int use_alternate = AnimationRead32(actor + 0x44) == primary;
        uint32_t selected = use_alternate ? AnimationRead32(actor + 0x4c) : primary;

        func_800222BC((SpriteData *)actor,
                     (SpriteAnimPackageFileHeader *)(uintptr_t)selected);
        uint32_t flags = AnimationRead32(actor + 0xb0);
        AnimationWrite32(actor + 0xb0, use_alternate ? flags | 0x400u : flags & ~0x400u);
        return 1;
    }
    case 0x93: {
        uint8_t *parent = AnimWordPtr(AnimationRead32(actor + 0x70));
        if (!parent || !(AnimationRead32(actor + 0x3c) & 3u))
            return 1;

        uint8_t *package = AnimWordPtr(AnimationRead32(actor + 0x24));
        uint8_t *format = AnimWordPtr(AnimationRead32(package));
        if (!func_8001EE68(format)) {
            uint32_t flags = AnimationRead32(actor + 0x40);
            AnimationWrite32(actor + 0x40, (flags & ~0x1e000u) | 0x1c000u);
        }

        uint8_t *model = AnimWordPtr(AnimationRead32(actor + 0x20));
        if (model) {
            uint8_t *parent_model = AnimWordPtr(AnimationRead32(parent + 0x20));
            if (AnimationRead32(parent_model + 0x34)) {
                func_8001D4E8(actor);
                for (unsigned offset = 0; offset < 64; offset += 8) {
                    model = AnimWordPtr(AnimationRead32(actor + 0x20));
                    parent_model = AnimWordPtr(AnimationRead32(parent + 0x20));
                    uint8_t *dst = AnimWordPtr(AnimationRead32(model + 0x34)) + offset;
                    const uint8_t *src = AnimWordPtr(AnimationRead32(parent_model + 0x34)) + offset;
                    /* The tables may overlap. Each direction is sampled before writing it. */
                    uint32_t first = AnimationRead32(src);
                    uint32_t second = AnimationRead32(src + 4);
                    AnimationWrite32(dst, first);
                    AnimationWrite32(dst + 4, second);
                }
                model = AnimWordPtr(AnimationRead32(actor + 0x20));
                parent_model = AnimWordPtr(AnimationRead32(parent + 0x20));
                uint8_t vertical = parent_model[0x3d];
                model[0x3c] = parent_model[0x3c];
                model[0x3d] = vertical;
            }
        }
        func_8001D2B0(actor, (int16_t)AnimationRead16(actor + 0x34));
        return 1;
    }
    default:
        return 0;
    }
}
