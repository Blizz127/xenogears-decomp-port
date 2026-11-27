/* Sprite bytecode arithmetic. The resident resolver accepts the address of
 * a selector, including selectors that alias the destination register. */
static int animationArithmetic(uint8_t *actor, uint8_t opcode, uint8_t *args)
{
    if (opcode < 0xd0 || opcode > 0xdf)
        return 0;
    if (opcode == 0xd4)
        return 1;

    uint8_t *dst = func_8001FBA4((void *)actor, args);
    unsigned left = dst[0];
    unsigned right = args[1];
    unsigned count = right & 31;
    switch (opcode) {
        case 0xd0:
        case 0xd3:
        case 0xdd:
        case 0xde:
            dst[0] = left + *(uint8_t *)func_8001FBA4((void *)actor, args + 1);
            break;
        case 0xd1:
            dst[0] = left * *(uint8_t *)func_8001FBA4((void *)actor, args + 1);
            break;
        case 0xd2:
        case 0xd5:
            right = *(uint8_t *)func_8001FBA4((void *)actor, args + 1);
            dst[0] = right ? left / right : 255;
            break;
        case 0xd6:
            dst[0] = left + right;
            break;
        case 0xd7:
            dst[0] = left * right;
            break;
        case 0xd8: {
            int divisor = right < 128 ? (int)right : (int)right - 256;
            dst[0] = divisor ? (int)left / divisor : 255;
            break;
        }
        case 0xd9:
            dst[0] = (uint32_t)left << count;
            break;
        case 0xda: {
            uint32_t value = left;
            if (left & 0x80)
                value |= UINT32_C(0xffffff00);
            dst[0] = count >= 8 && left >= 128 ? 255 : value >> count;
            break;
        }
        case 0xdb:
        case 0xdc: {
            uint32_t word = left | ((uint32_t)dst[1] << 8);
            word = opcode == 0xdb ? word << count : word >> count;
            dst[0] = (uint8_t)word;
            dst[1] = (uint8_t)(word >> 8);
            break;
        }
        case 0xdf:
            dst[0] = right;
            break;
    }
    return 1;
}
