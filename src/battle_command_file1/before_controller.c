/* battle_command_file1 before_controller: retail functions in retail order (asm/battle_command_file1/before_controller.s).
 * Functions still listed as INCLUDE_ASM are not byte-matching C yet. The whole
 * file is retail-build only: the port runs this overlay from its own bindings. */
#include "common.h"
#include "system/memory.h"

#ifndef XENO_PC_PORT
void func_battle_command_file1_801E57F8(void *, s32, u8, s32);

s16 func_80089B50(s32, s32);

s32 func_8003852C(void *);
s32 func_800399D4(s32);
s32 func_80039C4C(s32);
s32 func_8003A094(void *);
s32 func_800716D8();
extern s32 D_800C3E54;
extern void *D_800D2DAC;
extern void *D_800D3278;
extern void *D_800D3340;
extern void *D_800D39D0;
extern void *D_battle_command_file1_801E9C38;

INCLUDE_ASM("asm/battle_command_file1/nonmatchings/before_controller", xeno_battle_command_file1_init_5160);

s32 func_battle_command_file1_801E563C(void) {
    s32 var_s0;

    HeapFree(D_800D3278);
    var_s0 = 0;
    HeapFree(D_800D2DAC);
    HeapFree(D_800D39D0);
    HeapFree(D_800D3340);
    HeapFree(D_battle_command_file1_801E9C38);
    if (*(u8 *)((u8*)D_800D3278 + 0x81F) != 0) {
        var_s0 = 1;
        func_80039C4C(D_800C3E54);
        func_800716D8();
        func_800399D4(D_800C3E54);
        func_800716D8();
    }
    if (*(u8 *)((u8*)D_800D3278 + 0x820) != 0) {
        func_8003A094(*(void **)((u8*)D_800D3278 + 0x818));
        func_8003852C(*(void **)((u8*)D_800D3278 + 0x818));
        func_800716D8();
        HeapFree(*(void **)((u8*)D_800D3278 + 0x818));
        *(u8 *)((u8*)D_800D3278 + 0x820) = 0U;
    }
    return var_s0;
}

INCLUDE_ASM("asm/battle_command_file1/nonmatchings/before_controller", func_battle_command_file1_801E5768);
INCLUDE_ASM("asm/battle_command_file1/nonmatchings/before_controller", func_battle_command_file1_801E57C4);
INCLUDE_ASM("asm/battle_command_file1/nonmatchings/before_controller", func_battle_command_file1_801E57F8);
INCLUDE_ASM("asm/battle_command_file1/nonmatchings/before_controller", func_battle_command_file1_801E58EC);
INCLUDE_ASM("asm/battle_command_file1/nonmatchings/before_controller", func_battle_command_file1_801E5A98);
INCLUDE_ASM("asm/battle_command_file1/nonmatchings/before_controller", xeno_battle_command_file1_helper_5B00);
INCLUDE_ASM("asm/battle_command_file1/nonmatchings/before_controller", func_battle_command_file1_801E5C1C);
INCLUDE_ASM("asm/battle_command_file1/nonmatchings/before_controller", func_battle_command_file1_801E5CE4);
INCLUDE_ASM("asm/battle_command_file1/nonmatchings/before_controller", func_battle_command_file1_801E5D24);
INCLUDE_ASM("asm/battle_command_file1/nonmatchings/before_controller", func_battle_command_file1_801E5DCC);
INCLUDE_ASM("asm/battle_command_file1/nonmatchings/before_controller", func_battle_command_file1_801E5EF8);
INCLUDE_ASM("asm/battle_command_file1/nonmatchings/before_controller", func_battle_command_file1_801E5F8C);

s32 func_battle_command_file1_801E6084(s32 arg0, void *arg1) {
    func_battle_command_file1_801E57F8(arg1, 2, *(u8 *)((s8*)(arg1) + 5), 0);
    *(u16 *)((s8*)(((((*(u8 *)((s8*)(arg1) + 2) << 8) | *(u8 *)((s8*)(arg1) + 1)) & 0xFFFE) + (*(s32*)&D_800D3278))) + 0x394) = (u16) *(u16 *)((s8*)((*(s32*)&D_800D3278)) + 0x382);
    return 6;
}


s32 func_battle_command_file1_801E60E8(s32 arg0, void *arg1) {
    *(s16 *)((s8*)(((((*(u8 *)((s8*)(arg1) + 2) << 8) | *(u8 *)((s8*)(arg1) + 1)) & 0xFFFE) + (*(s32*)&D_800D3278))) + 0x394) = 1;
    return 3;
}


s32 func_battle_command_file1_801E6118(s32 arg0, void *arg1) {
    *(s16 *)((s8*)(((((*(u8 *)((s8*)(arg1) + 2) << 8) | *(u8 *)((s8*)(arg1) + 1)) & 0xFFFE) + (*(s32*)&D_800D3278))) + 0x394) = 0;
    return 3;
}


s32 func_battle_command_file1_801E6144(s32 arg0, void *arg1) {
    void *temp_v0;

    func_battle_command_file1_801E57F8(arg1, 2, *(u8 *)((s8*)(arg1) + 5), 0);
    temp_v0 = (((*(u8 *)((s8*)(arg1) + 2) << 8) | *(u8 *)((s8*)(arg1) + 1)) & 0xFFFE) + (*(s32*)&D_800D3278);
    *(u16 *)((s8*)(temp_v0) + 0x394) = (u16) (*(u16 *)((s8*)(temp_v0) + 0x394) + *(u16 *)((s8*)((*(s32*)&D_800D3278)) + 0x382));
    return 6;
}


s32 func_battle_command_file1_801E61B4(s32 arg0, void *arg1) {
    void *temp_v0;

    func_battle_command_file1_801E57F8(arg1, 2, *(u8 *)((s8*)(arg1) + 5), 0);
    temp_v0 = (((*(u8 *)((s8*)(arg1) + 2) << 8) | *(u8 *)((s8*)(arg1) + 1)) & 0xFFFE) + (*(s32*)&D_800D3278);
    *(u16 *)((s8*)(temp_v0) + 0x394) = (u16) (*(u16 *)((s8*)(temp_v0) + 0x394) - *(u16 *)((s8*)((*(s32*)&D_800D3278)) + 0x382));
    return 6;
}


s32 func_battle_command_file1_801E6224(s32 arg0, void *arg1) {
    void *temp_v0;

    func_battle_command_file1_801E57F8(arg1, 2, *(u8 *)((s8*)(arg1) + 5), 0);
    temp_v0 = (((*(u8 *)((s8*)(arg1) + 2) << 8) | *(u8 *)((s8*)(arg1) + 1)) & 0xFFFE) + (*(s32*)&D_800D3278);
    *(u16 *)((s8*)(temp_v0) + 0x394) = (u16) (*(u16 *)((s8*)(temp_v0) + 0x394) | *(u16 *)((s8*)((*(s32*)&D_800D3278)) + 0x382));
    return 6;
}


s32 func_battle_command_file1_801E6294(s32 arg0, void *arg1) {
    void *temp_v0;

    func_battle_command_file1_801E57F8(arg1, 2, *(u8 *)((s8*)(arg1) + 5), 0);
    temp_v0 = (((*(u8 *)((s8*)(arg1) + 2) << 8) | *(u8 *)((s8*)(arg1) + 1)) & 0xFFFE) + (*(s32*)&D_800D3278);
    *(u16 *)((s8*)(temp_v0) + 0x394) = (u16) (*(u16 *)((s8*)(temp_v0) + 0x394) & ~*(u16 *)((s8*)((*(s32*)&D_800D3278)) + 0x382));
    return 6;
}


s32 func_battle_command_file1_801E6304(s32 arg0, void *arg1) {
    void *temp_v1;

    temp_v1 = (((*(u8 *)((s8*)(arg1) + 2) << 8) | *(u8 *)((s8*)(arg1) + 1)) & 0xFFFE) + (*(s32*)&D_800D3278);
    *(u16 *)((s8*)(temp_v1) + 0x394) = (u16) (*(u16 *)((s8*)(temp_v1) + 0x394) + 1);
    return 3;
}


s32 func_battle_command_file1_801E633C(s32 arg0, void *arg1) {
    void *temp_v1;

    temp_v1 = (((*(u8 *)((s8*)(arg1) + 2) << 8) | *(u8 *)((s8*)(arg1) + 1)) & 0xFFFE) + (*(s32*)&D_800D3278);
    *(u16 *)((s8*)(temp_v1) + 0x394) = (u16) (*(u16 *)((s8*)(temp_v1) + 0x394) - 1);
    return 3;
}


s32 func_battle_command_file1_801E6374(s32 arg0, void *arg1) {
    void *temp_v0;

    func_battle_command_file1_801E57F8(arg1, 2, *(u8 *)((s8*)(arg1) + 5), 0);
    temp_v0 = (((*(u8 *)((s8*)(arg1) + 2) << 8) | *(u8 *)((s8*)(arg1) + 1)) & 0xFFFE) + (*(s32*)&D_800D3278);
    *(u16 *)((s8*)(temp_v0) + 0x394) = (u16) (*(u16 *)((s8*)(temp_v0) + 0x394) & *(u16 *)((s8*)((*(s32*)&D_800D3278)) + 0x382));
    return 6;
}


s32 func_battle_command_file1_801E63E4(s32 arg0, void *arg1) {
    void *temp_v0;

    func_battle_command_file1_801E57F8(arg1, 2, *(u8 *)((s8*)(arg1) + 5), 0);
    temp_v0 = (((*(u8 *)((s8*)(arg1) + 2) << 8) | *(u8 *)((s8*)(arg1) + 1)) & 0xFFFE) + (*(s32*)&D_800D3278);
    *(u16 *)((s8*)(temp_v0) + 0x394) = (u16) (*(u16 *)((s8*)(temp_v0) + 0x394) | *(u16 *)((s8*)((*(s32*)&D_800D3278)) + 0x382));
    return 6;
}


s32 func_battle_command_file1_801E6454(s32 arg0, void *arg1) {
    void *temp_v0;

    func_battle_command_file1_801E57F8(arg1, 2, *(u8 *)((s8*)(arg1) + 5), 0);
    temp_v0 = (((*(u8 *)((s8*)(arg1) + 2) << 8) | *(u8 *)((s8*)(arg1) + 1)) & 0xFFFE) + (*(s32*)&D_800D3278);
    *(u16 *)((s8*)(temp_v0) + 0x394) = (u16) (*(u16 *)((s8*)(temp_v0) + 0x394) ^ *(u16 *)((s8*)((*(s32*)&D_800D3278)) + 0x382));
    return 6;
}


s32 func_battle_command_file1_801E64C4(s32 arg0, void *arg1) {
    s32 temp_s0;
    void *temp_s0_2;

    temp_s0 = (s32) ((*(u8 *)((s8*)(arg1) + 2) << 8) | *(u8 *)((s8*)(arg1) + 1)) >> 1;
    func_battle_command_file1_801E57F8(arg1, 2, 0, 0);
    temp_s0_2 = (temp_s0 * 2) + (*(s32*)&D_800D3278);
    *(u16 *)((s8*)(temp_s0_2) + 0x394) = (u16) (*(u16 *)((s8*)(temp_s0_2) + 0x394) << *(u16 *)((s8*)((*(s32*)&D_800D3278)) + 0x382));
    return 5;
}


s32 func_battle_command_file1_801E6534(s32 arg0, void *arg1) {
    s32 temp_s0;
    void *temp_s0_2;

    temp_s0 = (s32) ((*(u8 *)((s8*)(arg1) + 2) << 8) | *(u8 *)((s8*)(arg1) + 1)) >> 1;
    func_battle_command_file1_801E57F8(arg1, 2, 0, 0);
    temp_s0_2 = (temp_s0 * 2) + (*(s32*)&D_800D3278);
    *(u16 *)((s8*)(temp_s0_2) + 0x394) = (u16) ((s32) *(u16 *)((s8*)(temp_s0_2) + 0x394) >> *(u16 *)((s8*)((*(s32*)&D_800D3278)) + 0x382));
    return 5;
}


s32 func_battle_command_file1_801E65A4(s32 arg0, void *arg1) {
    *(s16 *)((s8*)(((((*(u8 *)((s8*)(arg1) + 2) << 8) | *(u8 *)((s8*)(arg1) + 1)) & 0xFFFE) + (*(s32*)&D_800D3278))) + 0x394) = func_80089B50(0, 0x7FFF);
    return 3;
}


s32 func_battle_command_file1_801E65FC(s32 arg0, void *arg1) {
    *(s16 *)((s8*)(((((*(u8 *)((s8*)(arg1) + 4) << 8) | *(u8 *)((s8*)(arg1) + 3)) & 0xFFFE) + (*(s32*)&D_800D3278))) + 0x394) = func_80089B50(0, *(u8 *)((s8*)(arg1) + 1) | (*(u8 *)((s8*)(arg1) + 2) << 8));
    return 5;
}

INCLUDE_ASM("asm/battle_command_file1/nonmatchings/before_controller", func_battle_command_file1_801E6660);

s32 func_battle_command_file1_801E66D8(s32 arg0, void *arg1) {
    func_battle_command_file1_801E57F8(arg1, 2, *(u8 *)((s8*)(arg1) + 5), 0);
    *(s16 *)((s8*)(((((*(u8 *)((s8*)(arg1) + 2) << 8) | *(u8 *)((s8*)(arg1) + 1)) & 0xFFFE) + (*(s32*)&D_800D3278))) + 0x394) = (s16) ((s32) *(u16 *)((s8*)((*(s32*)&D_800D3278)) + 0x380) / (s32) *(u16 *)((s8*)((*(s32*)&D_800D3278)) + 0x382));
    return 6;
}

INCLUDE_ASM("asm/battle_command_file1/nonmatchings/before_controller", xeno_battle_command_file1_helper_6750);
#endif
