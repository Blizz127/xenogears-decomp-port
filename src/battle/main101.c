#include "common.h"


#ifndef XENO_PC_PORT
extern u8 D_800D36B8[];
extern void func_800B8284(void);
extern void func_8001BBAC(void);
extern void ArchiveCdDataSync(s32 mode);
extern void func_801E8588(void);
extern void func_801E91E8(void);
extern void func_801E9594(void);
extern void func_801E893C(void);
extern void func_800B7870(void);
extern void func_800A8B0C(void);
extern u8 D_8005949C[];
extern s32 D_80059520[];
extern s32 D_80059470[];
extern u8 D_800CCB94[];
extern u8 D_800C4A39[];
extern u8 D_800C3EB0__b[] asm("D_800C3EB0");
extern u8 func_801E7210(u8* a0, s32 a1, s32 a2, u8* a3, u8* a4, u8* a5);
extern void func_800A5E9C(u8* a0, u8* a1);

/* func_800B8098.s: battle start-up for `mode` (kept in D_800D36B8): set up
 * the display (func_800B8284) and run func_8001BBAC; modes 1-4 sync the CD
 * and call that mode's entry (func_801E8588, func_801E91E8, func_801E9594,
 * func_801E893C), any other mode runs func_800B7870.  Then sync the CD again,
 * reset the actor slots (func_800A8B0C), store func_801E7210's result at
 * D_800C3EB0 + 0xB88 and call func_800A5E9C.  The switch table
 * (jtbl_80070A10) comes from this TU's .rodata, which config/battle.yaml
 * places at its retail address. */
void func_800B8098(u32 mode) {
    D_800D36B8[0] = mode;
    func_800B8284();
    func_8001BBAC();
    switch (mode) {
    case 1:
        ArchiveCdDataSync(0);
        func_801E8588();
        break;
    case 2:
        ArchiveCdDataSync(0);
        func_801E91E8();
        break;
    case 3:
        ArchiveCdDataSync(0);
        func_801E9594();
        break;
    case 4:
        ArchiveCdDataSync(0);
        func_801E893C();
        break;
    case 0:
    case 5:
    default:
        func_800B7870();
        break;
    }
    ArchiveCdDataSync(0);
    func_800A8B0C();
    D_800C3EB0__b[0xB88] = func_801E7210(D_8005949C, D_80059520[0], D_80059470[0], D_800CCB94, D_800CCB94 + 0x20, D_800C4A39);
    func_800A5E9C(D_800C4A39, D_800C3EB0__b + 0x4BF9);
}
extern void SetDispMask(int);
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
#ifndef XENO_PC_PORT /* port: lifted body in the h-frontier block */
void func_800B8354(void) {
    while (ArchiveDataSync() != 0) {
        func_800BE790();
    }
}
#endif
