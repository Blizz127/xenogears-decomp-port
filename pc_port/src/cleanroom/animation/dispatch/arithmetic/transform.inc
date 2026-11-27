/* A sprite rotation command selects one of three axes. Part zero rotates
 * the entire model. Other parts keep their rotation in a separate array. */
static uint16_t animationTransformHalf(const uint8_t *field)
{
    return (uint16_t)field[0] | ((uint16_t)field[1] << 8);
}

static uint8_t *animationTransformPointer(const uint8_t *field)
{
    uint32_t address = animationTransformHalf(field) |
                       ((uint32_t)animationTransformHalf(field + 2) << 16);
    return AnimWordPtr(address);
}

static int animationTransform(uint8_t *actor, uint8_t opcode, uint8_t *args)
{
    if (opcode < 0xcd || opcode > 0xcf)
        return 0;

    uint8_t *model = animationTransformPointer(actor + 0x20);
    if (!model)
        return 1;

    unsigned command = animationTransformHalf(args);
    unsigned part = (command >> 9) & 7;
    unsigned axis = opcode - 0xcd;
    uint8_t *rotation = model;
    if (part) {
        uint8_t *parts = animationTransformPointer(model + 0x34);
        if (!parts)
            return 1;
        rotation = parts + part * 8 + 2;
    }

    unsigned amount = (command & 0x1ff) * 8;
    if (axis && (actor[0xac] & 4))
        amount = 0u - amount;
    uint8_t *angle = rotation + 2 * axis;
    if (!(command & 0x1000))
        amount += animationTransformHalf(angle);
    angle[0] = (uint8_t)amount;
    angle[1] = (uint8_t)(amount >> 8);
    if (!part)
        actor[0x3f] |= 0x10;

    return 1;
}
