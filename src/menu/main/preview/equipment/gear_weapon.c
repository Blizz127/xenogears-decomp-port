#include "gear_records.h"
#include <string.h>

u8 func_801E4928(u8 id)
{
    const u8 *gear = gear_record(id);
    int remaining = gear_u16(gear + 0x44) / 120 - gear[0x75];
    return remaining > 0 ? (u8)(remaining / 2) : 0;
}

static void copy_weapon_stats(u8 *dest, const u8 *row)
{
    gear_put16(dest, gear_u16(row + 18));
    dest[2] = row[14];
    dest[3] = row[16];
    dest[4] = row[17];
}

void func_801E4754(void *context, u8 id)
{
    u8 *gear = gear_record(id);
    const u8 *table = gear_table(context, 0x18);
    const u8 *weapon = table + 20 * gear[0x0c];
    memcpy(gear + 0x5c, weapon, 4);
    if (id == 5 || id == 13) {
        static const u8 slots[] = {4, 5, 7};
        for (unsigned i = 0; i < 3; ++i) {
            const u8 *row = table + 20 * gear[slots[i]];
            gear[0x5d + i] = row[1 + i];
            copy_weapon_stats(gear + 0x10 + 8 * i, row);
        }
    } else {
        copy_weapon_stats(gear + 0x10, weapon);
    }
    if (weapon[17] == 100) {
        u16 flags = gear_u16(gear + 0x86) & 0x0fff;
        gear_put16(gear + 0x86, flags | gear_u16(weapon + 18));
    }
}
