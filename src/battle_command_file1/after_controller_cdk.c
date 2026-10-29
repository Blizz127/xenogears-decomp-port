/* Retail TU part 2 of after_controller.c: the tail from func_801E93E8 to the
 * end of the segment was built with the CDK compiler (li as `addiu $r,$zero,K`,
 * compiler-split %hi/%lo loads with the lui hoisted away from its %lo use),
 * while everything before it is gcc 2.6.3/aspsx code, so this run is its own
 * matching TU on the BattleCdk preset (gears.toml).  Host/port builds compile
 * after_controller.c whole; this file is empty for them. */
#include "common.h"
#ifndef XENO_PC_PORT
#include "system/memory.h"

extern void * D_800D3278__a_void[] asm("D_800D3278");
s32 func_800245D8(s32);
void func_80021BF8(void *, s32);
void TimerWorkListRemoveTask(s32);
void WorkListRemoveTask(s32);
void func_8001CE74(s32);
void func_800BC2F0(s32);
void func_800BC3F8(s32);
extern s8 D_800C37C8[];

void func_battle_command_file1_801E93E8(void *arg0) {
    u32 lo = *(u32 *)((s8*)(arg0) + 0xA8) >> 0x1E;
    u32 hi = (*(u32 *)((s8*)(arg0) + 0xAC) & 3) << 2;

    *(s8 *)((s8*)((D_800D3278__a_void[0] + (hi | lo))) + 0x804) = 0;
    func_80021BF8(arg0, 0);
}

INCLUDE_ASM("asm/battle_command_file1/nonmatchings/after_controller_cdk", func_battle_command_file1_801E9430);
INCLUDE_ASM("asm/battle_command_file1/nonmatchings/after_controller_cdk", func_battle_command_file1_801E950C);
INCLUDE_ASM("asm/battle_command_file1/nonmatchings/after_controller_cdk", func_battle_command_file1_801E9550);
INCLUDE_ASM("asm/battle_command_file1/nonmatchings/after_controller_cdk", func_battle_command_file1_801E958C);
INCLUDE_ASM("asm/battle_command_file1/nonmatchings/after_controller_cdk", func_battle_command_file1_801E95B0);
INCLUDE_ASM("asm/battle_command_file1/nonmatchings/after_controller_cdk", func_battle_command_file1_801E95E4);
INCLUDE_ASM("asm/battle_command_file1/nonmatchings/after_controller_cdk", func_battle_command_file1_801E9694);
INCLUDE_ASM("asm/battle_command_file1/nonmatchings/after_controller_cdk", func_battle_command_file1_801E9700);
INCLUDE_ASM("asm/battle_command_file1/nonmatchings/after_controller_cdk", func_battle_command_file1_801E9760);
INCLUDE_ASM("asm/battle_command_file1/nonmatchings/after_controller_cdk", func_battle_command_file1_801E9894);

void func_battle_command_file1_801E9958(s32 arg0, u16 arg1) {
    func_800245D8(arg0 + 0x38);
}

INCLUDE_ASM("asm/battle_command_file1/nonmatchings/after_controller_cdk", func_battle_command_file1_801E9978);

s32 func_battle_command_file1_801E9AD4(s32 arg0) {
    func_8001CE74(arg0);
    WorkListRemoveTask(arg0 + 0x1C);
    TimerWorkListRemoveTask(arg0);
    HeapFree(arg0);
    func_800BC3F8(1);
    func_800BC2F0(1);
    D_800C37C8[0] = 0;
}

INCLUDE_ASM("asm/battle_command_file1/nonmatchings/after_controller_cdk", func_battle_command_file1_801E9B2C);
#endif
