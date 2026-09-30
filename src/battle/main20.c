#include "common.h"


#ifndef XENO_PC_PORT
extern u8 D_800D3420[];
static __inline__ u16* BattleD3420Row(u8 index) {
    register u16* base asm("$6");
    u32 offset = (u32)index << 6;
    base = (u16*)D_800D3420;
    return (u16*)((u8*)base + offset);
}
#else
static __inline__ u16* BattleD3420Row(u8 index) {
    return (u16*)(D_800D3420 + ((u32)index << 6));
}
#endif


/* func_8007B2C0.s: row[p[3]] = row[p[1]] & row[p[2]] on the D_800D3420 halfword row */
void func_8007B2C0(u8** ppBoard, u8 index) {
    u8* p = *ppBoard;
    u16* row = BattleD3420Row(index);

    row[p[3]] = row[p[1]] & row[p[2]];
}
/* func_8007B310.s: row[p[3]] = row[p[1]] | row[p[2]] on the D_800D3420 halfword row */
void func_8007B310(u8** ppBoard, u8 index) {
    u8* p = *ppBoard;
    u16* row = BattleD3420Row(index);

    row[p[3]] = row[p[1]] | row[p[2]];
}
/* func_8007B360.s: row[p[3]] = row[p[1]] ^ row[p[2]] on the D_800D3420 halfword row */
void func_8007B360(u8** ppBoard, u8 index) {
    u8* p = *ppBoard;
    u16* row = BattleD3420Row(index);

    row[p[3]] = row[p[1]] ^ row[p[2]];
}


extern void func_80079ED8(u8 a0, u8 a1, u8 a2, s32 a3);
extern void func_8007A280(u8 a0, u8 a1, u32 a2, u32 a3);


/* func_8007B3B0.s */
void func_8007B3B0(u8** ppBoard, u8 x) {
    u8* p = *ppBoard;

    func_80079ED8((x + 3) & 0xFF, p[1], p[2], 0);
}
/* func_8007B3E4.s */
void func_8007B3E4(u8** ppBoard, u8 v) {
    u8* p = *ppBoard;

    func_8007A280((v + 3) & 0xFF, p[1], p[2] | (p[3] << 8), 0);
}
