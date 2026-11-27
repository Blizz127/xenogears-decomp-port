#include "gear_records.h"
#include <string.h>

extern u8 D_801E9808[];
extern u8 func_801E4928(u8 id);

static void add_effect_word(u8 *field, u16 amount)
{
    gear_put16(field, gear_u16(field) + amount);
}

static void include_mask(u8 *field, u16 bits)
{
    gear_put16(field, gear_u16(field) | bits);
}

void func_801E433C(void *context, u8 id)
{
    u8 *gear = gear_record(id);
    const u8 *table = gear_table(context, 0x14);
    const unsigned pilot = D_801E9808[id];
    u8 *pilot_effects = (u8 *)&g_GameState + 0x16c4 + pilot * 32;
    u16 flags = gear_u16(pilot_effects) & ~0x0490u;

    /* Rebuild the accessory contribution; preserve the upper four status bits. */
    memset(gear + 0x40, 0, 6);
    gear_put16(gear + 0x48, 0);
    memset(gear + 0x4c, 0, 12);
    gear_put16(gear + 0x6e, 0);
    gear_put16(gear + 0x7e, 0);
    gear_put16(gear + 0x82, 0);
    gear_put16(gear + 0x86, gear_u16(gear + 0x86) & 0xf000);
    memset(gear + 0x88, 0, 16);

    for (unsigned slot = 0; slot < 3; ++slot) {
        const u8 *item = table + gear[9 + slot] * 28;
        u16 mask = gear_u16(item + 22);

        add_effect_word(gear + 0x40, item[13]);
        add_effect_word(gear + 0x42, item[14]);
        add_effect_word(gear + 0x44, gear_u16(item + 6));
        gear[0x4c] += item[24];
        gear[0x4d] += item[20];
        for (unsigned n = 0; n < 4; ++n)
            gear[0x50 + n] += item[16 + n];
        gear[0x54] += item[27];

        switch (item[21]) {
        case 1:
            include_mask(gear + 0x7e, mask);
            break;
        case 2:
            include_mask(gear + 0x82, mask);
            break;
        case 3:
            include_mask(gear + 0x86, mask);
            break;
        case 4:
            include_mask(gear + 0x6e, mask);
            for (unsigned bit = 0; bit < 16; ++bit) {
                if (mask & (1u << bit))
                    gear[0x97 - bit] += item[26];
            }
            break;
        case 5:
            gear[0x4f] += (u8)mask;
            break;
        case 6:
            if ((flags & 0x1800) == 0x1800)
                flags |= 0x0400;
            break;
        case 7:
            if ((flags & 0x0300) == 0x0300)
                flags |= 0x0080;
            break;
        case 8:
            if ((flags & 0x0060) == 0x0060)
                flags |= 0x0010;
            break;
        case 9:
            include_mask(gear + 0x48, mask);
            gear[0x56] += (u8)mask;
            break;
        case 10:
            gear[0x56] += (u8)mask;
            break;
        case 11:
            gear[0x57] += (u8)mask;
            break;
        }
    }
    gear_put16(pilot_effects, flags);
    gear[0x4a] = func_801E4928(id);
    /* The weapon calculation can change which pilot currently has this gear. */
    if (gear[0x4f])
        pilot_effects[0x17] |= 0x80;
    else if (g_GameState.characters[D_801E9808[id]].gearId == id)
        pilot_effects[0x17] &= 0x7f;
}
