#include "common.h"
#ifndef XENO_PC_PORT
void AddPrim(s32, void *);
extern void *D_800C3E24;
extern s32 D_800CCB04;
extern void * D_800D2D28;
void func_80034888(s32, s32, s32);
extern s32 D_800CCB34;
extern s32 D_800D2DAC;
extern void *D_800D3278;
void func_80074D4C(void);
void func_80074F70(void);
#endif

#ifndef XENO_PC_PORT
#include "system/memory.h"
#endif


#ifndef XENO_PC_PORT
void func_800728B8(void *prims, s32 count, s32 start);
extern void *D_800D2D28;
extern void *D_800D2DB4;
void func_80074AB8(void);
void func_80074EEC(void);
#endif

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/battle/nonmatchings/main2", func_80071AE0);
INCLUDE_ASM("asm/battle/nonmatchings/main2", func_80071B94);
INCLUDE_ASM("asm/battle/nonmatchings/main2", func_80072270);
INCLUDE_ASM("asm/battle/nonmatchings/main2", func_80072324);
INCLUDE_ASM("asm/battle/nonmatchings/main2", func_800723E0);
INCLUDE_ASM("asm/battle/nonmatchings/main2", func_8007252C);

#ifndef XENO_PC_PORT
/* Link `count` primitives of 0x28 bytes, every other one from `start`. */
void func_800728B8(void* prims, s32 count, s32 start) {
    s32 i;

    for (i = 0; i < count; i++) {
        AddPrim(D_800CCB04 + 4, (u8*)prims + (start + i * 2) * 0x28);
    }
}
#endif

INCLUDE_ASM("asm/battle/nonmatchings/main2", func_80072938);
INCLUDE_ASM("asm/battle/nonmatchings/main2", func_80072A9C);
INCLUDE_ASM("asm/battle/nonmatchings/main2", func_80072DA8);
INCLUDE_ASM("asm/battle/nonmatchings/main2", func_80072F38);
INCLUDE_ASM("asm/battle/nonmatchings/main2", func_80073380);
INCLUDE_ASM("asm/battle/nonmatchings/main2", func_80073538);
INCLUDE_ASM("asm/battle/nonmatchings/main2", func_80073A58);
INCLUDE_ASM("asm/battle/nonmatchings/main2", func_80073B64);
INCLUDE_ASM("asm/battle/nonmatchings/main2", func_80073E88);
INCLUDE_ASM("asm/battle/nonmatchings/main2", func_80073F08);
INCLUDE_ASM("asm/battle/nonmatchings/main2", func_80073FB8);
INCLUDE_ASM("asm/battle/nonmatchings/main2", func_800742A0);
INCLUDE_ASM("asm/battle/nonmatchings/main2", func_800743A4);
INCLUDE_ASM("asm/battle/nonmatchings/main2", func_800744BC);
INCLUDE_ASM("asm/battle/nonmatchings/main2", func_80074554);
INCLUDE_ASM("asm/battle/nonmatchings/main2", func_800745EC);

#ifndef XENO_PC_PORT
void func_80074AB8(void) {
    u16 temp_v0;

    if (*(u8 *)((s8*)(D_800D2D28) + 0xC7) != 0) {
        func_800728B8(D_800D2DB4 + 0x3E80, *(u8 *)((s8*)(D_800D2DB4) + 0x5D77), *(u8 *)((s8*)(D_800D2DB4) + 0x5D86));
        func_800728B8(D_800D2DB4 + 0x5280, *(u8 *)((s8*)(D_800D2DB4) + 0x5D7C), *(u8 *)((s8*)(D_800D2DB4) + 0x5D8B));
        func_800728B8(D_800D2DB4 + 0x53C0, *(u8 *)((s8*)(D_800D2DB4) + 0x5D7D), *(u8 *)((s8*)(D_800D2DB4) + 0x5D8C));
        func_800728B8(D_800D2DB4 + 0x43D0, *(u8 *)((s8*)(D_800D2DB4) + 0x5D7F), *(u8 *)((s8*)(D_800D2DB4) + 0x5D8E));
    }
    if (*(u8 *)((s8*)(D_800D2D28) + 0xAD) != 0) {
        func_800728B8(D_800D2DB4 + 0x46A0, 0x14U, *(u8 *)((s8*)(D_800D2DB4) + 0x5D98));
        func_800728B8(D_800D2DB4, *(u8 *)((s8*)(D_800D2DB4) + 0x5D70), *(u8 *)((s8*)(D_800D2DB4) + 0x5D92));
        func_800728B8(D_800D2DB4 + 0x1720, *(u8 *)((s8*)(D_800D2DB4) + 0x5D74), *(u8 *)((s8*)(D_800D2DB4) + 0x5D83));
        temp_v0 = *(u16 *)((s8*)(D_800D2DB4) + 0x5DA2) + 1;
        *(u16 *)((s8*)(D_800D2DB4) + 0x5DA2) = temp_v0;
        if ((s16) temp_v0 < 0xF) {
            func_800728B8(D_800D2DB4 + 0x4CE0, *(u8 *)((s8*)(D_800D2DB4) + 0x5DA1), *(u8 *)((s8*)(D_800D2DB4) + 0x5DA0));
        } else if ((s16) temp_v0 >= 0x15) {
            *(u16 *)((s8*)(D_800D2DB4) + 0x5DA2) = 0U;
        }
        func_800728B8(D_800D2DB4 + 0x3AC0, *(u8 *)((s8*)(D_800D2DB4) + 0x5D75), *(u8 *)((s8*)(D_800D2DB4) + 0x5D84));
        func_800728B8(D_800D2DB4 + 0x3E30, *(u8 *)((s8*)(D_800D2DB4) + 0x5D76), *(u8 *)((s8*)(D_800D2DB4) + 0x5D85));
        func_800728B8(D_800D2DB4 + 0x4E70, *(u8 *)((s8*)(D_800D2DB4) + 0x5D78), *(u8 *)((s8*)(D_800D2DB4) + 0x5D87));
        func_800728B8(D_800D2DB4 + 0x4FB0, *(u8 *)((s8*)(D_800D2DB4) + 0x5D79), *(u8 *)((s8*)(D_800D2DB4) + 0x5D88));
        func_800728B8(D_800D2DB4 + 0x50A0, *(u8 *)((s8*)(D_800D2DB4) + 0x5D7A), *(u8 *)((s8*)(D_800D2DB4) + 0x5D89));
        func_800728B8(D_800D2DB4 + 0x51E0, *(u8 *)((s8*)(D_800D2DB4) + 0x5D7B), *(u8 *)((s8*)(D_800D2DB4) + 0x5D8A));
        func_800728B8(D_800D2DB4 + 0x2530, *(u8 *)((s8*)(D_800D2DB4) + 0x5D7E), *(u8 *)((s8*)(D_800D2DB4) + 0x5D8D));
        func_800728B8(D_800D2DB4 + 0x32A0, *(u8 *)((s8*)(D_800D2DB4) + 0x5D96), *(u8 *)((s8*)(D_800D2DB4) + 0x5D97));
        func_800728B8(D_800D2DB4 + 0x3C0, *(u8 *)((s8*)(D_800D2DB4) + 0x5D71), *(u8 *)((s8*)(D_800D2DB4) + 0x5D93));
        func_800728B8(D_800D2DB4 + 0xD20, *(u8 *)((s8*)(D_800D2DB4) + 0x5D72), *(u8 *)((s8*)(D_800D2DB4) + 0x5D94));
        func_800728B8(D_800D2DB4 + 0xFA0, *(u8 *)((s8*)(D_800D2DB4) + 0x5D73), *(u8 *)((s8*)(D_800D2DB4) + 0x5D95));
    }
}
#endif


#ifndef XENO_PC_PORT
void func_80074D4C(void) {
    s32 temp_a0;
    s32 temp_v0;
    s32 temp_v0_2;
    s32 var_s0;

    if (*(u8 *)((s8*)(D_800D2D28) + 0xC6) != 0) {
        if (*(u8 *)((s8*)(D_800C3E24) + 0xE5) != 0) {
            temp_v0 = *(s32 *)((s8*)(D_800C3E24) + 0xE0) - 4;
            *(s32 *)((s8*)(D_800C3E24) + 0xE0) = temp_v0;
            if (temp_v0 < 0x40) {
                *(u8 *)((s8*)(D_800C3E24) + 0xE5) = 0U;
                *(s32 *)((s8*)(D_800C3E24) + 0xE0) = 0x40;
            }
            goto block_6;
        }
        temp_v0_2 = *(s32 *)((s8*)(D_800C3E24) + 0xE0) + 4;
        *(s32 *)((s8*)(D_800C3E24) + 0xE0) = temp_v0_2;
        var_s0 = 0;
        if (temp_v0_2 >= 0x100) {
            *(u8 *)((s8*)(D_800C3E24) + 0xE5) = 1U;
            *(s32 *)((s8*)(D_800C3E24) + 0xE0) = 0xFC;
block_6:
            var_s0 = 0;
        }
        do {
            if (*(u8 *)((s8*)((D_800C3E24 + var_s0)) + 0xE6) != 0) {
                temp_a0 = var_s0 * 2;
                *(u8 *)((s8*)((((temp_a0 + *(u8 *)((s8*)(D_800C3E24) + 0xE4)) * 0x1C) + D_800C3E24)) + 4) = (u8) *(s32 *)((s8*)(D_800C3E24) + 0xE0);
                *(s8 *)((s8*)((((temp_a0 + *(u8 *)((s8*)(D_800C3E24) + 0xE4)) * 0x1C) + D_800C3E24)) + 5) = 0;
                *(s8 *)((s8*)((((temp_a0 + *(u8 *)((s8*)(D_800C3E24) + 0xE4)) * 0x1C) + D_800C3E24)) + 6) = 0;
                AddPrim(D_800CCB04 + 4, ((temp_a0 + *(u8 *)((s8*)(D_800C3E24) + 0xE4)) * 0x1C) + D_800C3E24);
            }
            var_s0 += 1;
        } while (var_s0 < 4);
    }
}
#endif


#ifndef XENO_PC_PORT
void func_80074EEC(void) {
    if (*(u8 *)((s8*)(D_800D2D28) + 0xA8) != 0) {
        func_800728B8(D_800D2DB4 + 0x5550, *(u8 *)((s8*)(D_800D2DB4) + 0x5D80), *(u8 *)((s8*)(D_800D2DB4) + 0x5D8F));
        func_800728B8(D_800D2DB4 + 0x5640, *(u8 *)((s8*)(D_800D2DB4) + 0x5D81), *(u8 *)((s8*)(D_800D2DB4) + 0x5D90));
        func_800728B8(D_800D2DB4 + 0x5C80, *(u8 *)((s8*)(D_800D2DB4) + 0x5D82), *(u8 *)((s8*)(D_800D2DB4) + 0x5D91));
    }
}
#endif


#ifndef XENO_PC_PORT
void func_80074F70(void) {
    if (*(u8 *)((s8*)(D_800D2D28) + 0xC8) != 0) {
        AddPrim(D_800CCB04 + 4, (*(u8 *)((s8*)(D_800D3278) + 0x7F4) * 0x28) + 0x7A4 + D_800D3278);
    }
    if (*(u8 *)((s8*)(D_800D2D28) + 0xC9) != 0) {
        func_80034888(D_800D2DAC, D_800CCB04 + 4, D_800CCB34);
    }
}
#endif

INCLUDE_ASM("asm/battle/nonmatchings/main2", func_8007500C);
INCLUDE_ASM("asm/battle/nonmatchings/main2", func_80075168);
INCLUDE_ASM("asm/battle/nonmatchings/main2", func_80075938);
#endif
#ifndef XENO_PC_PORT
extern u8 D_800C492A;
#endif
extern void func_8008FAD8(void);
extern void func_800742A0(void);
extern void func_80075938(void);
extern void func_80073538(void);
extern void func_80073A58(void);
extern void func_80073B64(void);
extern void func_80073E88(void);
extern void func_80073F08(void);
extern void func_8007500C(void);
extern void func_80074EEC(void);
extern void func_800745EC(void);
extern void func_80074D4C(void);
extern void func_80073FB8(void);
extern void func_80088B80(void);
extern void func_80074AB8(void);
/* func_80076418.s: the per-frame battle render chain, skipped entirely once
 * D_800C492A is set. */
void func_80076418(void) {
    if (D_800C492A == 0) {
        func_8008FAD8();
        func_800742A0();
        func_80075938();
        func_80073538();
        func_80073A58();
        func_80073B64();
        func_80073E88();
        func_80073F08();
        func_8007500C();
        func_80074EEC();
        func_800745EC();
        func_80074D4C();
        func_80073FB8();
        func_80088B80();
        func_80074AB8();
    }
}


extern u8 D_8005959C;
extern void func_8008FAD8(void);
extern void func_801DE594(void);
extern void func_80073FB8(void);


/* func_800764B4.s */
void func_800764B4(void) {
    D_8005959C = 0;
    func_8008FAD8();
    func_801DE594();
    func_80073FB8();
}
/* func_800764EC.s */
void func_800764EC(void) {
    func_8008FAD8();
    func_80073538();
    func_80073F08();
    func_8007500C();
    func_80074F70();
    func_80073FB8();
    func_80088B80();
    func_80074AB8();
}
