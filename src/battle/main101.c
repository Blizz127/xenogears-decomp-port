#include "common.h"


#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/battle/nonmatchings/main101", func_800B8098);
extern void SetDispMask(int mask);
extern void SoundFreeWdsEntry(s32);
extern void func_80021B04(void*, s16, s16, s16);
extern void func_800B8840(void);
extern void func_800B88C4(void);
extern void func_801E62E0(s32);
extern s32 D_800595AC[];
extern u8* D_800658C8[];
extern u8 D_800D30A0[];
extern u8 D_800D3354[];
extern u8 D_800D335C[];

void func_800B81BC(s32 arg0) {
    func_800B88C4();
    func_800B8840();
    func_801E62E0(arg0);
    SoundFreeWdsEntry(D_800595AC[0]);
    func_80021B04(D_800D30A0, *(s16*)(D_800658C8[0] + 0x482), *(s16*)(D_800658C8[0] + 0x484), *(s16*)(D_800658C8[0] + 0x486));
    func_80021B04(D_800D3354, *(s16*)(D_800658C8[0] + 0x482), *(s16*)(D_800658C8[0] + 0x484), *(s16*)(D_800658C8[0] + 0x486));
    func_80021B04(D_800D30A0 + 8, *(s16*)(D_800658C8[0] + 0x47C), *(s16*)(D_800658C8[0] + 0x47E), *(s16*)(D_800658C8[0] + 0x480));
    func_80021B04(D_800D335C, *(s16*)(D_800658C8[0] + 0x47C), *(s16*)(D_800658C8[0] + 0x47E), *(s16*)(D_800658C8[0] + 0x480));
    SetDispMask(1);
}

#include "psyq/libgpu.h"
extern void SetGeomOffset(long ofx, long ofy);
typedef struct {
    DRAWENV draw;
    DISPENV disp;
    u8 pad[0x4070 - 0x5C - 0x14];
} DB;
typedef struct {
    u8 pad[0xB70];
    DB db[2];
} Glob3EB0;
extern Glob3EB0 D_800C3EB0;

void func_800B8284(void) {
    SetGeomOffset(0xA0, 0xA4);
    SetDefDispEnv(&D_800C3EB0.db[0].disp, 0, 0xE0, 0x140, 0xE0);
    SetDefDispEnv(&D_800C3EB0.db[1].disp, 0, 0, 0x140, 0xE0);
    SetDefDrawEnv(&D_800C3EB0.db[0].draw, 0, 0, 0x140, 0xE0);
    SetDefDrawEnv(&D_800C3EB0.db[1].draw, 0, 0xE0, 0x140, 0xE0);
    D_800C3EB0.db[1].disp.screen.y = 10;
    D_800C3EB0.db[0].disp.screen.y = 10;
    D_800C3EB0.db[1].disp.screen.w = 0x100;
    D_800C3EB0.db[0].disp.screen.w = 0x100;
    D_800C3EB0.db[1].disp.screen.x = 0;
    D_800C3EB0.db[0].disp.screen.x = 0;
    D_800C3EB0.db[1].disp.screen.h = 0xD8;
    D_800C3EB0.db[0].disp.screen.h = 0xD8;
}

#endif


extern s32 ArchiveDataSync(void);
extern void func_800BE790(void);


/* func_800B8354.s */
void func_800B8354(void) {
    while (ArchiveDataSync() != 0) {
        func_800BE790();
    }
}
