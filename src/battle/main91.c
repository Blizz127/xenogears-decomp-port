#include "common.h"


/* func_800B69E4.s: add three signed 16-bit LE deltas (found through the record's
 * self-offset) to the three halfwords at a0. */
void func_800B69E4(u16* a0, u8* a1) {
    u8* r = a1 + (((s32)(s8)a1[1] << 8) | a1[0]);
    s32 d;

    d = ((s32)(s8)r[1] << 8) | r[0];
    a0[0] = a0[0] + d;
    d = ((s32)(s8)r[3] << 8) | r[2];
    a0[1] = a0[1] + d;
    d = ((s32)(s8)r[5] << 8) | r[4];
    a0[2] = a0[2] + d;
}/* func_800B6A50.s */
void func_800B6A50(u8* p) {
    u8* q = *(u8**)(p + 0x70);

    *(u32*)(p + 0xC) = *(u32*)(q + 0xC);
    *(u32*)(p + 0x10) = *(u32*)(q + 0x10);
    *(u32*)(p + 0x14) = *(u32*)(q + 0x14);
}
