#include "gear_records.h"

void func_801E4258(void *context, u8 id)
{
    u8 *gear = gear_record(id);
    const u8 *row = gear_table(context, 0x10) + gear[8] * 20;
    gear_put16(gear + 0x70, gear_u16(row + 8));
    gear_put16(gear + 0x72, gear_u16(row + 10));
}

void func_801E42AC(s32 context, u8 id)
{
    u8 *gear = gear_record(id);
    const u8 *row = gear_table((void *)(uintptr_t)(u32)context, 0x0c) + gear[3] * 16;
    u16 capacity = gear_u16(row + 6);
    if (gear_u16(gear + 0x38) > capacity)
        gear_put16(gear + 0x38, capacity);
    gear_put16(gear + 0x3a, capacity);
    gear[0x3c] = row[12];
    gear[0x3d] = row[13];
    gear[0x3e] = row[14];
    gear[0x3f] = row[14];
}
