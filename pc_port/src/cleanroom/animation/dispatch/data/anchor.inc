/* Original MIT implementation for the sprite anchor commands.
 * Requires opcodes.inc, guest pointer resolution, the public geometry API
 * and its current view matrix. */
static int animationAnchor(uint8_t *actor, uint8_t opcode, uint8_t *args)
{
    if (opcode != 0xbc || !(args[0] & 0x80u))
        return 0;

    unsigned selector = args[0] & 0x3fu;
    int short_destination = (args[0] & 0x40u) != 0;
    int camera_space = actor[0x3f] & 1u;
    const uint8_t *source;
    switch (selector) {
    case 0:
    case 0x12: case 0x13: case 0x14: case 0x15:
        source = AnimWordPtr(spriteDataWord(actor + 0x74));
        break;
    case 1:
    case 0x20: case 0x21: case 0x22: case 0x23:
        source = AnimationPlayerSprite();
        break;
    case 4:
        source = actor;
        break;
    case 0x16:
        source = AnimWordPtr(spriteDataWord(actor + 0x70));
        camera_space = 0;
        break;
    case 0x24:
    case 0x25: {
        uint8_t *object = AnimWordPtr(spriteDataWord(actor + 0x6c));
        if (!object)
            return 0;
        uint32_t flags = spriteDataWord(object + 0x14);
        uint32_t sticky = 1u << 30;
        spriteDataSetWord(object + 0x14, selector == 0x24 ? flags | sticky : flags & ~sticky);
        source = actor;
        camera_space = 0;
        break;
    }
    case 0x18:
        source = actor;
        camera_space = 0;
        break;
    default:
        return 0;
    }
    if (!source)
        return 0;
    SVECTOR position = {0};
    position.vx = (int16_t)spriteDataHalf(source + 2);
    position.vy = (int16_t)spriteDataHalf(source + 6);
    position.vz = (int16_t)spriteDataHalf(source + 10);

    if ((selector >= 0x12 && selector <= 0x15) ||
        (selector >= 0x20 && selector <= 0x23)) {
        unsigned anchor = selector < 0x20 ? selector - 0x12 : selector - 0x20;
        unsigned height;
        switch (anchor) {
        case 0: height = spriteDataHalf(source + 0x36); break;
        case 1: {
            int first = spriteDataHalf(source + 0x36);
            int second = spriteDataHalf(source + 0x38);
            height = (unsigned)(first + (second - first) / 2);
            break;
        }
        case 2: height = spriteDataHalf(source + 0x38); break;
        default: height = (spriteDataHalf(source + 0x38) + 1u) / 2u; break;
        }
        position.vy = (int16_t)((unsigned)position.vy - height);
    }
    if (camera_space) {
        SVECTOR result = {0};
        ApplyMatrixSV((MATRIX *)&D_8004FBB8, &position, &result);
        const uint8_t *matrix = (const uint8_t *)&D_8004FBB8;
        position.vx = (int16_t)(result.vx + spriteDataHalf(matrix + 0x14));
        position.vy = (int16_t)(result.vy + spriteDataHalf(matrix + 0x18));
        position.vz = (int16_t)(result.vz + spriteDataHalf(matrix + 0x1c));
    }
    if (short_destination) {
        spriteDataSetHalf(actor + 0xa0, (uint16_t)position.vx);
        spriteDataSetHalf(actor + 0xa2, (uint16_t)position.vy);
        spriteDataSetHalf(actor + 0xa4, (uint16_t)position.vz);
    } else {
        spriteDataSetWord(actor, (uint32_t)position.vx << 16);
        spriteDataSetWord(actor + 4, (uint32_t)position.vy << 16);
        spriteDataSetWord(actor + 8, (uint32_t)position.vz << 16);
    }
    return 1;
}
