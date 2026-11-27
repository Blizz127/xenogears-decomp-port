/* Original port implementation, MIT, 2026 Xenogears port contributors.
 * The sprite and transform records use the packed little-endian game ABI.
 * AnimWordPtr resolves a pointer word from that ABI to host memory. */
#include <stdint.h>

static uint16_t spriteDataHalf(const uint8_t *address)
{
    return (uint16_t)(address[0] | (unsigned)address[1] << 8);
}

static uint32_t spriteDataWord(const uint8_t *address)
{
    return spriteDataHalf(address) | (uint32_t)spriteDataHalf(address + 2) << 16;
}

static void spriteDataSetHalf(uint8_t *address, uint16_t value)
{
    address[0] = (uint8_t)value;
    address[1] = (uint8_t)(value >> 8);
}

static void spriteDataSetWord(uint8_t *address, uint32_t value)
{
    spriteDataSetHalf(address, (uint16_t)value);
    spriteDataSetHalf(address + 2, (uint16_t)(value >> 16));
}

static void spriteDataTransformChanged(uint8_t *actor)
{
    spriteDataSetWord(actor + 0x3c, spriteDataWord(actor + 0x3c) | (1u << 28));
}

/* Return one for every recognized instruction, including the documented
 * no-operation when the corresponding transform is unbound. */
static int animationData(uint8_t *actor, uint8_t opcode, uint8_t *args)
{
    uint8_t *basis;
    uint8_t *coordinate;
    int delta;
    switch (opcode) {
    case 0x8a:
        spriteDataSetWord(actor + 0x0c, 0);
        spriteDataSetWord(actor + 0x14, 0);
        spriteDataSetWord(actor + 0x18, 0);
        break;
    case 0xa2:
        actor[0x3d] = args[0];
        break;
    case 0xad: {
        uint32_t flags = spriteDataWord(actor + 0xa8);
        const uint32_t field = 0x3ffu << 1;
        spriteDataSetWord(actor + 0xa8, (flags & ~field) | ((uint32_t)args[0] << 1));
        break;
    }
    case 0xae:
    case 0xaf:
        delta = (int8_t)args[0] * 16;
        if (spriteDataWord(actor + 0xac) & 4u)
            delta = -delta;
        basis = AnimWordPtr(spriteDataWord(actor + 0x20));
        if (basis) {
            coordinate = basis + 4;
            if (opcode == 0xae)
                delta += spriteDataHalf(coordinate);
            spriteDataSetHalf(coordinate, (uint16_t)delta);
            spriteDataTransformChanged(actor);
        }
        break;
    case 0xb5: {
        unsigned mode = spriteDataWord(actor + 0x3c) & 3u;
        uint16_t scale = (uint16_t)args[0] << 8;
        if (mode) {
            basis = AnimWordPtr(spriteDataWord(actor + 0x20));
            if (basis) {
                spriteDataSetHalf(actor + 0x2c, scale);
                spriteDataSetHalf(basis + 10, scale);
                spriteDataSetHalf(basis + 8, scale);
                spriteDataSetHalf(basis + 6, scale);
                spriteDataTransformChanged(actor);
            }
        }
        break;
    }
    case 0xb6:
    case 0xb7:
        basis = AnimWordPtr(spriteDataWord(actor + 0x20));
        if (basis) {
            coordinate = basis + (opcode - 0xb6) * 2u;
            delta = (int8_t)args[0] * 16;
            spriteDataSetHalf(coordinate, (uint16_t)(spriteDataHalf(coordinate) + delta));
            spriteDataTransformChanged(actor);
        }
        break;
    case 0xb8:
        actor[0x8c] -= args[0];
        break;
    case 0xbb:
        /* The renderer adds this signed bias to the sprite's ordering-table depth. */
        delta = (int8_t)args[0];
        spriteDataSetHalf(actor + 0x30, (uint16_t)(spriteDataHalf(actor + 0x30) + delta));
        break;
    case 0xbf:
        spriteDataSetHalf(actor + 0x36, args[0]);
        break;
    case 0xc9:
        if (spriteDataWord(actor + 0xa8) & 1u) {
            uint16_t value = spriteDataHalf(args);
            uint8_t *aux = AnimWordPtr(spriteDataWord(actor + 0x7c));
            if (aux)
                spriteDataSetHalf(aux + 0x0c, value);
        }
        break;
    case 0xcc: {
        int offset = (int16_t)spriteDataHalf(args);
        spriteDataSetWord(actor + 0x88, spriteDataWord(actor + 0x64) + (uint32_t)offset);
        break;
    }
    case 0xe9:
    case 0xea:
    case 0xeb: {
        unsigned amount = 2u * spriteDataHalf(args);
        basis = AnimWordPtr(spriteDataWord(actor + 0x20));
        if (basis) {
            coordinate = basis + 6 + (opcode - 0xe9) * 2u;
            spriteDataSetHalf(coordinate, (uint16_t)(spriteDataHalf(coordinate) + amount));
            spriteDataTransformChanged(actor);
        }
        break;
    }
    case 0xed:
    case 0xef: {
        unsigned axis = opcode == 0xed ? 0 : 8;
        spriteDataSetWord(actor + axis, (uint32_t)spriteDataHalf(args) << 16);
        break;
    }
    case 0xee: {
        int distance = (int16_t)spriteDataHalf(args);
        int scale = (int16_t)spriteDataHalf(actor + 0x2c);
        int origin = (int16_t)spriteDataHalf(actor + 0x84);
        int coordinate_y = origin + distance * scale / 4096;
        spriteDataSetWord(actor + 4, (uint32_t)coordinate_y << 16);
        break;
    }
    default:
        return 0;
    }
    return 1;
}
