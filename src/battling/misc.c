/* battling misc: retail functions in retail order (asm/battling/CB8.s).
 * Functions still listed as INCLUDE_ASM are not byte-matching C yet. The whole
 * file is retail-build only. */
#include "common.h"
#include "system/memory.h"

#ifndef XENO_PC_PORT
extern s32 D_800912F4;
extern u8 D_80099D98[];
extern u8 D_80099D98__s_u8 asm("D_80099D98");
s32 func_8007F97C(void);
#endif

#ifndef XENO_PC_PORT
extern s32 D_80092AD8;
void func_8007BB7C(void);
#endif

#ifndef XENO_PC_PORT
s32 GetStringEntry(s32, s32);
void func_80034714(s32 *, s32);
void func_8007191C(s32);
extern s32 D_80090F38[];
extern s32 D_800925DC[];
extern s8 D_80092604[];
extern s32 D_80092880[];
extern s8 D_80092884__a_s8[] asm("D_80092884");
extern s32 D_800928C8__a_s32[] asm("D_800928C8");
extern s32 D_80092900__a_s32[] asm("D_80092900");
extern s32 D_80092904[];
extern s32 D_80092954__a_s32[] asm("D_80092954");
extern s8 D_800929BC[];
extern s8 D_80099D9D__a_s8[] asm("D_80099D9D");
extern s8 D_80099D9E__a_s8[] asm("D_80099D9E");
void func_800719F0(void);

extern u8 D_80099DA1__a[] asm("D_80099DA1");
extern u8 D_80099D9A__a[] asm("D_80099D9A");
extern u8 D_80099D9D__a[] asm("D_80099D9D");
extern u8 D_80099D9E__a[] asm("D_80099D9E");
extern u8 D_80099D9F__a[] asm("D_80099D9F");
extern u8 D_80099DA2__a[] asm("D_80099DA2");
void func_80080090(void);
void func_800800CC(void);
void func_80080180(void);
void func_800801BC(void);
void func_800801F8(void);
void func_80080268(void);

extern u8 D_80099D98[];
extern u8 D_80099D9B[];
extern u8 D_80099D9C[];
extern s32 D_8006F980[];
void func_80080054(void);
void func_80080108(void);
void func_80080144(void);
void func_80088940(void);

void func_800346D4(s32 *);
void func_80083C0C(s32);
extern s32 D_80092954;
extern s32 D_800925F8;
extern s32 D_800925FC;
extern s32 D_80097058;
extern s32 D_800970E0;
extern s32 D_80098774;
extern s32 D_800987FC;
void func_80032F54(s32 *, s32, s32, s32, s32, s32, s32);
extern s32 D_800925D4;
extern s32 D_800925D8;
extern s32 D_8009868C;
extern s8 D_8009293C;
extern s32 D_800910C4;
extern s8 D_800928D4;
extern s32 D_80092900;
extern s32 D_80092A00;
extern s32 D_80092A10;
extern s32 D_80092A20;
extern s32 D_800970F8;
extern s32 D_80098814;
extern s8 D_80099D9D;
extern s8 D_80099D9E;
void SetGeomScreen(s32);
void func_80030988(s32, s32, s32, s32);
void func_8008976C(s32, s32);
void func_80089D5C(s32);
void func_8008A5BC(s32);
void func_8008BC04();
extern s32 D_800910F0;
extern s32 D_80092610;
void func_80074678(void *, s16, s16);
void func_80074998(void *);
void func_8008B0D8(void *);
void func_8008B730(void *, u8, u8);
void func_800732CC();
void func_80088AF8();
void func_8008DC28();
extern s32 D_8009292C;
extern s8 D_80099D9A;
extern s8 D_80099DA1;
extern s8 D_80099DA2;
extern s16 D_80099DA4;
extern s8 D_800928FC;
extern s32 D_80092918;
extern s32 D_80092944;
extern s32 D_80092950;
extern s16 D_80097102;
extern s16 D_8009881E;
extern s32 D_80092640;
extern s32 D_800928AC;
void func_8007BB7C();
void func_8007E24C();
void func_8008D580(s32);
void func_8008E620();
extern s8 D_80091150;
extern s8 D_80091151;
extern s32 D_80092644;
extern s32 D_80092890;
extern s32 D_80097014;
extern s32 D_80097080;
extern void *D_8009790C;
extern s32 D_80098730;
extern s32 D_8009879C;
extern void *D_80099028;
extern s32 D_800926DC;
extern s16 D_800926E8;
extern s16 D_800926EC;
extern s32 D_800912DC;
extern s8 D_8009273C;
extern s8 D_80092740;
extern s32 D_80092744;
extern u8 D_80099D9F;
u8 func_8007FF70(u8, s32, s32);
extern u8 D_80092884;
extern s32 D_80092924;
void func_800719F0();
void func_8008509C(s32, s32);
extern s32 D_80091660;
extern s32 D_80092734;
void func_80039FF8();
void func_80080964(s32);
s32 func_800891C0(s32);
extern s16 D_80091672;
extern s16 D_800916AE;
extern s8 D_80092758;
extern s32 D_800928C8;
extern s32 D_800928D8;
extern s32 D_80092940;
void ArchiveCdDataSync(s32);
extern s32 D_80092760;
void func_8007F8B4();
void func_80080B58();
void func_8008EB4C(s32);
extern s16 D_800915BE;
extern s16 D_80091636;
extern s16 D_80091762;
extern s8 D_800926FC;
extern s8 D_8009275C;
extern u16 D_8009286C;
extern s8 D_8009A1C4;
extern s8 D_8009A1C5;
extern s8 D_8009A1C6;
extern s16 D_8009A1CA;
extern s16 D_8009A1CE;
extern s8 D_8009A2BC;
extern s8 D_8009A2BD;
extern s8 D_8009A2BE;
extern s16 D_8009A2C2;
extern s16 D_8009A2C6;
extern s32 D_80092790;
void func_80080D20(s32);
void func_80086E24();
void func_8008AC0C(s32);
void func_8008AE1C(s32);
extern u8 D_800928A0;
void func_8008E3CC(s32, s32, s32);
void func_8007F258(s32, s32);
void func_8008E120();
extern s16 D_80092780;
extern s32 D_80092938;
void func_8003A838(s32, s32, s32);
void func_8008E064();
extern s32 D_80092784;
extern s32 D_80092948;
void *func_80089C54();
void func_80089C88(s32, void *);
void func_80089E2C(void *, s32);
s32 func_80089FC4();
void func_8008A184(s32, s32 *);
extern s32 D_80091FB0;
extern s32 D_80059488;
void func_80083BB4(s32);
extern u8 D_80092920;
s32 ArchiveDecodeAlignedSize(u16);
void ChangeGameState(s32);
void DrawSync(s32);
void Vsync(s32);
void func_8003852C(s32);
void func_800399D4(s32);
void func_80039C4C(s32);
void func_80088A40();
extern s8 D_8005061C;
extern s32 D_800917F0;
extern s32 D_800927C4;
void *func_8008820C();
void func_800888B0();
void func_800889C8();
extern s32 D_800927F4;
extern u16 g_C1ButtonState;
void func_800707A8(void);
void func_80070F80(s32 arg0);
void func_800718C0(void);
void func_800720C4(void);
void func_800720D4(void);
void func_800725A8(void);
void func_800726B4(void);
void func_80074AB4(void *arg0);
s32 func_8007570C(void *arg0, void *arg1, s32 arg2);
s32 func_80075738(void *arg0, void *arg1);
void func_80075748(void);
void func_8007639C(void *arg0, s8 arg1);
void func_80076424(void *arg0);
void func_80077A88(void *arg0);
void func_80078E94(void *arg0);
void func_80078ED4(void *arg0);
void func_80079A8C(void);
void func_80079B04(void);
void func_80079B0C(void);
void func_80079D08(void *arg0);
void func_80079DE0(void);
void func_8007A6D0(void *arg0);
void func_8007A730(void *arg0);
void func_8007A884(void);
s32 func_8007CD14(s32 arg0, s32 arg1, s32 arg2, s32 arg3);
s32 func_8007E624(void);
void func_8007E894(s16 arg0, s16 arg1);
void func_8007E954(s32 arg0);
void func_8007F834(void);
void func_8007F8E4(void);
void func_80080234(void);
void func_800808F4(void);
void func_80080920(void);
s32 func_800809BC(void);
void func_800809D8(void);
void func_80080A58(void);
void func_80080AA0(s32 arg0);
void func_80080C48(s32 arg0);
void func_80080D10(void);
void func_80081E00(void);
void func_80081E6C(void);
s32 func_80083CD8(void);
s32 func_800849E0(void *arg0);
void func_80084A40(void *arg0);
s32 func_80084A64(void *arg0);
void func_80084AE0(void);
void func_80084B48(void);
void func_80084BEC(void *arg0);
void func_80084FD0(void);
void func_80085014(void);
void func_80085070(void);
void func_8008518C(void *arg0, s32 arg1);
void func_800851D4(void);
void func_80085E34(void *arg0, void *arg1);
void func_80085E60(void *arg0, void *arg1);
void func_80085E90(s32 arg0, s16 *arg1, s32 arg2);
void func_80085EAC(s32 arg0, s16 *arg1, s32 arg2);
void func_800882D4(s32 arg0, s32 arg1, s32 arg2, s16 arg3);
void func_80088BD4(void);
void func_80088C28(void);


void func_800707A8(void) {
    func_80083C0C(3);
    func_800346D4(&D_80092954);
}

INCLUDE_ASM("asm/battling/nonmatchings/misc", func_800707D8);
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_80070808);
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_800708C4);
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_8007099C);
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_80070C7C);

void func_80070F80(s32 arg0) {
    D_800925F8 = arg0;
    D_80098774 = 0;
    D_80097058 = 0;
    D_800925FC = 0;
    D_800987FC &= 0xFFFF7FFF;
    D_800970E0 &= 0xFFFF7FFF;
}

INCLUDE_ASM("asm/battling/nonmatchings/misc", func_80070FD8);
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_8007107C);
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_80071724);
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_80071794);

void func_800718C0(void) {
    D_800925D8 = -1;
    D_800925D4 = 0;
    func_80032F54(&D_8009868C, 0x140, 0x30, 0x1C, 0x9A, 0x40, 4);
}

INCLUDE_ASM("asm/battling/nonmatchings/misc", func_8007191C);

void func_800719F0(void) {
    D_80099D9D__a_s8[0] = 0;
    D_80099D9E__a_s8[0] = 0;
    func_80083C0C(7);
    D_800928C8__a_s32[0] = 5;
    D_80092884__a_s8[0] = 0;
    func_80032F54(D_80092954__a_s32, 0x140, 0x70, 0xA2, 0x2A, 0x1C, 8);
    func_80034714(D_80092954__a_s32, GetStringEntry(D_80092880[0], 0x42));
    D_800929BC[0] = 0x1E;
    D_800925DC[0] = 0;
    func_80070F80(D_80090F38);
    D_80092604[0] = 0;
    D_80092904[0] = 0;
    D_80092900__a_s32[0] = 0;
    func_8007191C(9);
}

INCLUDE_ASM("asm/battling/nonmatchings/misc", func_80071AD0);
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_80071DA4);
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_80071F8C);

void func_800720C4(void) {
    D_8009293C = 0;
}


void func_800720D4(void) {
    if ((*(u8*)&D_8009293C) == 0) {
        func_80070F80(&D_800910C4);
        (*(u8*)&D_8009293C) = 1;
        D_800928D4 = 0;
        D_80099D9D = 0;
        D_80099D9E = 0;
        D_80092900 = 0;
        D_800925D4 = 0;
        D_800925D8 = 0;
        D_80092A00 = 1;
        D_80092A10 = 1;
        D_80092A20 = 1;
        D_800970F8 = 0;
        D_80098814 = 0;
    }
}

INCLUDE_ASM("asm/battling/nonmatchings/misc", func_80072170);

void func_800725A8(void) {

}

INCLUDE_ASM("asm/battling/nonmatchings/misc", func_800725B0);

void func_800726B4(void) {
    func_8008BC04();
    func_8007F834();
    func_80030988(1, 1, 0x40, 0x40);
    func_8008A5BC(D_800910F0);
    func_80089D5C(D_80092610);
    func_8008976C(0x140, 0xDA);
    SetGeomScreen(0xC0);
    func_80083C0C(3);
    func_8007E954(0x100);
    func_80080D10();
}

INCLUDE_ASM("asm/battling/nonmatchings/misc", func_8007273C);
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_80072858);
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_80072D18);
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_80073064);
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_800730AC);
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_800730F4);
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_8007313C);
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_800731F8);
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_800732AC);
void func_800732CC(void) {
    void *temp_v0;

    temp_v0 = func_8008D3F4(3, 0);
    (*(u8 *)((u8 *)(temp_v0) + 0x74)) = 0x80;
    (*(u8 *)((u8 *)(temp_v0) + 0x75)) = 0x80;
    (*(u8 *)((u8 *)(temp_v0) + 0x76)) = 0xC0;
    func_8008D5C0(temp_v0, 0x60);
    (*(s16 *)((u8 *)(temp_v0) + 0)) = 4;
    (*(s16 *)((u8 *)(temp_v0) + 0x44)) = 0x300;
    (*(s16 *)((u8 *)(temp_v0) + 0x48)) = 8;
    (*(s16 *)((u8 *)(temp_v0) + 0x4A)) = 0x20;
    (*(s16 *)((u8 *)(temp_v0) + 0x68)) = 0;
    (*(s16 *)((u8 *)(temp_v0) + 0x6A)) = 0x20;
    D_80092644 = temp_v0;
}
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_8007334C);
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_80073424);
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_80073644);
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_80073B7C);
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_80073CA4);
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_80073CEC);
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_80073DE4);
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_80073E2C);
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_80073F34);
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_800740E4);
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_80074678);
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_80074998);

void func_80074AB4(void *arg0) {
    u8 temp_a2;
    void *temp_s1;
    void *temp_s2;
    void *temp_v1;

    temp_v1 = *(void **)((s8*)(arg0) + 0x15CC);
    temp_s2 = *(void **)((s8*)(arg0) + 0x15D0);
    *(s32 *)((s8*)(arg0) + 0) = (s32) *(s16 *)((s8*)(temp_v1) + 0);
    *(s32 *)((s8*)(arg0) + 4) = (s32) *(s16 *)((s8*)(temp_v1) + 2);
    *(s32 *)((s8*)(arg0) + 8) = (s32) *(s16 *)((s8*)(temp_v1) + 4);
    *(u8 *)((s8*)(arg0) + 0x4C) = (u8) *(u8 *)((s8*)(temp_s2) + 8);
    *(s32 *)((s8*)(arg0) + 0x54) = (s32) ((s32) (*(u16 *)((s8*)(temp_v1) + 6) << 0x14) >> 0x14);
    temp_s1 = *(s32 *)((s8*)(*(void **)((s8*)(*(void **)((s8*)(arg0) + 0x5C)) + 4)) + 8) + (*(u8 *)((s8*)(temp_s2) + 8) * 0x14);
    if (*(u16 *)((s8*)(temp_s2) + 6) & 0x1000) {
        func_80074998(arg0);
        func_8008B0D8(temp_s1);
    }
    func_80074678(arg0, *(s16 *)((s8*)(temp_s1) + 0x12), *(u8 *)((s8*)(*(void **)((s8*)(arg0) + 0x15D0)) + 9));
    temp_a2 = *(u8 *)((s8*)(temp_s2) + 0xA);
    if (temp_a2 != 0) {
        func_8008B730(temp_s1, *(u8 *)((s8*)(temp_s2) + 9), temp_a2);
    }
}

INCLUDE_ASM("asm/battling/nonmatchings/misc", func_80074BA4);
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_80075060);
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_800751C8);

s32 func_8007570C(void *arg0, void *arg1, s32 arg2) {
    s32 var_v0;

    var_v0 = *(s16 *)((s8*)(arg1) + 0x15EC) * arg2;
    if (var_v0 < 0) {
        var_v0 += 0x1F;
    }
    return (var_v0 >> 5) - *(s16 *)((s8*)(arg0) + 0x15EE);
}


s32 func_80075738(void *arg0, void *arg1) {
    return *(s16 *)((s8*)(arg1) + 0x15EA) - *(s16 *)((s8*)(arg0) + 0x15E8);
}


void func_80075748(void) {

}

INCLUDE_ASM("asm/battling/nonmatchings/misc", func_80075750);
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_80075888);
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_80075A4C);
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_80075B50);

void func_8007639C(void *arg0, s8 arg1) {
    u8 temp_v0;

    if ((u8) *(u8 *)((s8*)(arg0) + 0x9C2) < 0x20U) {
        temp_v0 = *(u8 *)((s8*)(arg0) + 0x9C0);
        *(u8 *)((s8*)(arg0) + 0x9C0) = (u8) (temp_v0 + 1);
        *(s8 *)((s8*)((arg0 + (temp_v0 & 0x1F))) + 0x9A0) = arg1;
        *(u8 *)((s8*)(arg0) + 0x9C2) = (u8) (*(u8 *)((s8*)(arg0) + 0x9C2) + 1);
    }
}

u8 func_800763E4(u8 *arg0) {
    u8 idx;

    if (arg0[0x9C2] == 0) {
        return 0;
    }
    idx = arg0[0x9C1]++;
    idx = (arg0 + (idx & 0x1F))[0x9A0];
    arg0[0x9C2]--;
    return idx;
}

void func_80076424(void *arg0) {
    *(s8 *)((s8*)(arg0) + 0x9C2) = 0;
    *(s8 *)((s8*)(arg0) + 0x9C0) = 0;
    *(s8 *)((s8*)(arg0) + 0x9C1) = 0;
    *(s8 *)((s8*)(arg0) + 0x9C3) = 0;
}

INCLUDE_ASM("asm/battling/nonmatchings/misc", func_80076438);
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_800764CC);
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_8007661C);
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_800767C8);
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_80076884);
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_80077038);
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_80077584);
void func_8007762C(void *arg0) {
    *(u8 *)((u8 *)arg0 + 0x4C) = 8;
    *(u8 *)((u8 *)arg0 + 0x4E) = 0xFF;
    *(s16 *)((u8 *)arg0 + 0xCA) = 0x3C;
    *(s16 *)((u8 *)arg0 + 0x916) = 0x28;
    *(s32 *)((u8 *)arg0 + 0xE8) = 0x28;
    *(u8 *)((u8 *)arg0 + 0xC4) = 4;
    *(u8 *)((u8 *)arg0 + 0xC5) = 0;
    *(s32 *)((u8 *)arg0 + 0xD0) |= 0x1000;
    *(s32 *)((u8 *)arg0 + 0xD4) &= ~0x10;
    *(s32 *)((u8 *)arg0 + 0xD0) &= ~0x400;
    func_80077584();
}
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_800776A8);
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_80077770);
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_80077A38);

void func_80077A88(void *arg0) {
    *(s32 *)((s8*)(arg0) + 0xD0) = (s32) (*(s32 *)((s8*)(arg0) + 0xD0) | 0x80000);
}

INCLUDE_ASM("asm/battling/nonmatchings/misc", func_80077A9C);
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_80078154);
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_80078194);
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_80078704);
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_80078920);
void func_80078D20(void *arg0) {
    s32 temp_a0;
    s32 temp_a1;
    s32 temp_v1;
    u8 unused[16];

    (*(s32 *)((u8 *)(arg0) + 0xB0)) = func_80082488(arg0, 1);
    temp_v1 = ((*(s32 *)((u8 *)(arg0) + 0xD0)) & 0x9FFFFFFF) | ((((u32)func_800828C4(arg0) >> 0x18) & 3) << 0x1D);
    (*(s32 *)((u8 *)(arg0) + 0xD0)) = temp_v1;
    if ((temp_v1 & 0x60000000) == 0x60000000) {
        (*(s8 *)((u8 *)(arg0) + 0x90B)) = 0xF;
    }
    func_800828F8(arg0, arg0 + 0x10, 0x3E80);
    temp_a1 = (*(s32 *)((u8 *)(arg0) + 0xD0)) & 0xFFFBFFFF;
    (*(s32 *)((u8 *)(arg0) + 0xD0)) = temp_a1;
    if ((*(s32 *)((u8 *)(arg0) + 0xB0)) < ((*(s32 *)((u8 *)(arg0) + 4)) + (*(s32 *)((u8 *)(arg0) + 0x14)))) {
        (*(s32 *)((u8 *)(arg0) + 0xD0)) = (s32) (temp_a1 | 0x40000);
        if ((*(u8 *)((u8 *)(arg0) + 0xC4)) == 4) {
            if (!((*(s32 *)((u8 *)(arg0) + 0xD4)) & 0x10)) {
                func_8008EBD0(arg0, 0xD, arg0, 2);
                func_800776A8(arg0, 6);
            }
            temp_a0 = (*(s32 *)((u8 *)(arg0) + 0x14));
            (*(s32 *)((u8 *)(arg0) + 0xD4)) = (s32) ((*(s32 *)((u8 *)(arg0) + 0xD4)) | 0x10);
            if (temp_a0 >= 0x40) {
                (*(s32 *)((u8 *)(arg0) + 0x14)) = (s32) (-temp_a0 / 3);
            } else {
                goto block_8;
            }
        } else {
block_8:
            (*(s32 *)((u8 *)(arg0) + 0x14)) = 0;
        }
        (*(s32 *)((u8 *)(arg0) + 4)) = (s32) (*(s32 *)((u8 *)(arg0) + 0xB0));
    }
    (*(s32 *)((u8 *)(arg0) + 0)) = (s32) ((*(s32 *)((u8 *)(arg0) + 0)) + (*(s32 *)((u8 *)(arg0) + 0x10)));
    (*(s32 *)((u8 *)(arg0) + 4)) = (s32) ((*(s32 *)((u8 *)(arg0) + 4)) + (*(s32 *)((u8 *)(arg0) + 0x14)));
    (*(s32 *)((u8 *)(arg0) + 8)) = (s32) ((*(s32 *)((u8 *)(arg0) + 8)) + (*(s32 *)((u8 *)(arg0) + 0x18)));
}

void func_80078E94(void *arg0) {
    *(s32 *)((s8*)(*(void **)((s8*)(arg0) + 0x5C)) + 0x34) = (s32) *(s32 *)((s8*)(arg0) + 0);
    *(s32 *)((s8*)(*(void **)((s8*)(arg0) + 0x5C)) + 0x38) = (s32) *(s32 *)((s8*)(arg0) + 4);
    *(s32 *)((s8*)(*(void **)((s8*)(arg0) + 0x5C)) + 0x3C) = (s32) *(s32 *)((s8*)(arg0) + 8);
    *(s16 *)((s8*)(*(void **)((s8*)(arg0) + 0x5C)) + 0x46) = (s16) *(s32 *)((s8*)(arg0) + 0x54);
}


void func_80078ED4(void *arg0) {
    *(s16 *)((s8*)(arg0) + 0) = 0x100;
    *(s16 *)((s8*)(arg0) + 4) = 0x10;
    *(s16 *)((s8*)(arg0) + 2) = 0;
    *(s16 *)((s8*)(arg0) + 6) = 0;
    *(s16 *)((s8*)(arg0) + 8) = 0;
    *(s16 *)((s8*)(arg0) + 0xA) = 0x30;
    *(s16 *)((s8*)(arg0) + 0xC) = 0x30;
}

INCLUDE_ASM("asm/battling/nonmatchings/misc", func_80078F00);
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_8007920C);
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_800796B8);

void func_80079A8C(void) {
    func_800732CC();
    func_8008DC28();
    func_80088AF8();
    D_80099D9D = 1;
    D_80099DA1 = 3;
    D_8009292C = 0x100;
    D_80099D9E = 0;
    D_80099D9A = 0;
    D_80099DA2 = 0;
    D_80099DA4 = 0x100;
}


void func_80079B04(void) {

}


void func_80079B0C(void) {
    D_80092950 = 0;
    D_8009881E = 0;
    D_80097102 = 0;
    D_80092918 = 0;
    D_80092944 = 0;
    D_800928FC = 0;
}

INCLUDE_ASM("asm/battling/nonmatchings/misc", func_80079B44);

void func_80079D08(void *arg0) {
    void *temp_a1;
    void *temp_a1_2;
    void *temp_a1_3;

    temp_a1 = *(void **)((s8*)(arg0) + 0x15CC);
    *(u16 *)((s8*)(temp_a1) + 0xA) = (u16) ((*(u16 *)((s8*)(temp_a1) + 0xA) & 0xFEFF) | ((*(u32 *)((s8*)(arg0) + 0xD0) << 6) & 0x100));
    temp_a1_2 = *(void **)((s8*)(arg0) + 0x15CC);
    *(u16 *)((s8*)(temp_a1_2) + 6) = (u16) ((*(u16 *)((s8*)(temp_a1_2) + 6) & 0xDFFF) | (((u32) *(u32 *)((s8*)(arg0) + 0xD0) >> 2) & 0x2000));
    temp_a1_3 = *(void **)((s8*)(arg0) + 0x15CC);
    *(u16 *)((s8*)(temp_a1_3) + 6) = (u16) ((*(u16 *)((s8*)(temp_a1_3) + 6) & 0xBFFF) | (((u32) *(u32 *)((s8*)(arg0) + 0xD0) >> 5) & 0x4000));
}

INCLUDE_ASM("asm/battling/nonmatchings/misc", func_80079D6C);

void func_80079DE0(void) {
    D_80092640 = 0;
}

INCLUDE_ASM("asm/battling/nonmatchings/misc", func_80079DF0);
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_8007A21C);
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_8007A344);

void func_8007A6D0(void *arg0) {
    D_8009292C = 0x100;
    if (D_800928AC < 0x78) {
        *(s8 *)((s8*)(arg0) + 0x4F) = 0x10;
        *(s8 *)((s8*)(arg0) + 0x52) = 0;
        *(s8 *)((s8*)(arg0) + 0x4C) = 0x10;
        *(s32 *)((s8*)(arg0) + 0xD0) = (s32) (*(s32 *)((s8*)(arg0) + 0xD0) | 0x400);
        return;
    }
    *(s8 *)((s8*)(arg0) + 0x4F) = 0x10;
    *(s8 *)((s8*)(arg0) + 0x52) = 0;
    *(s8 *)((s8*)(arg0) + 0x4C) = 0;
    *(s32 *)((s8*)(arg0) + 0xD0) = (s32) (*(s32 *)((s8*)(arg0) + 0xD0) | 0x400);
}


void func_8007A730(void *arg0) {
    D_8009292C = 0x100;
    *(s8 *)((s8*)(arg0) + 0x4F) = 0x10;
    *(s8 *)((s8*)(arg0) + 0x52) = 0;
    *(s8 *)((s8*)(arg0) + 0x4C) = 9;
    *(s32 *)((s8*)(arg0) + 0xD0) = (s32) (*(s32 *)((s8*)(arg0) + 0xD0) | 0x02000400);
}

INCLUDE_ASM("asm/battling/nonmatchings/misc", func_8007A768);

void func_8007A884(void) {
    s8 var_v0_2;
    void *var_v0;

    func_8007E24C();
    func_8008D580(D_80092644);
    func_8007BB7C();
    func_8007F834();
    func_8008E620();
    if (D_80092890 != 0) {
        var_v0 = D_8009790C;
    } else {
        var_v0 = D_80099028;
    }
    if (*(s16 *)((s8*)(var_v0) + 0x2C) != 0) {
        D_80091150 = 1;
        var_v0_2 = 0x10;
    } else {
        D_80091150 = 2;
        var_v0_2 = -1;
    }
    D_80091151 = var_v0_2;
    D_80098730 = 0x100;
    D_80097014 = 0x100;
    D_8009879C = 0x100;
    D_80097080 = 0x100;
}

INCLUDE_ASM("asm/battling/nonmatchings/misc", func_8007A958);
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_8007AC3C);
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_8007AE10);
void func_8007B210(void *arg0, s8 arg1) {
    *(s32 *)((u8 *)arg0 + 0x15CC) = (s32)((u8 *)arg0 + 0x9CC);
    *(u8 *)((u8 *)arg0 + 0x4F) = 0x10;
    *(s8 *)((u8 *)arg0 + 0x4C) = arg1;
    *(u8 *)((u8 *)arg0 + 0x52) = 0;
    D_8009292C = 0x100;
    func_80074BA4();
    func_80074678(arg0, *(s16 *)((u8 *)arg0 + 0x998), *(s16 *)((u8 *)arg0 + 0x99A));
}
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_8007B270);
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_8007B388);
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_8007BACC);

#ifndef XENO_PC_PORT
void func_8007BB7C(void) {
    u8 *var_v1;
    s32 var_v0;

    var_v1 = (u8 *) &D_80092AD8;
    var_v0 = 0x3B;
    do {
        *(s8 *)((s8*)(var_v1) + 0x50) = 0;
        var_v0 -= 1;
        var_v1 += 0x7C;
    } while (var_v0 >= 0);
}
#endif

INCLUDE_ASM("asm/battling/nonmatchings/misc", func_8007BBA0);
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_8007C100);
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_8007C124);
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_8007C280);
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_8007C880);
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_8007CAA4);

s32 func_8007CD14(s32 arg0, s32 arg1, s32 arg2, s32 arg3) {
    return (arg3 & 0x7F) | ((arg0 << 7) & 0x80) | (arg1 << 0x18) | ((arg2 << 8) & 0xFFFF00);
}

INCLUDE_ASM("asm/battling/nonmatchings/misc", func_8007CD44);
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_8007CF78);
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_8007D068);
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_8007D0B4);
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_8007D190);
s32 func_8007D25C(s32 arg0) {
    if (arg0 < 0x10) {
        return 0;
    }
    return arg0 < 0x20;
}
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_8007D274);
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_8007D334);
void func_8007D65C(s32 arg0, s32 arg1, s32 arg2) {
    switch (arg2) {
    case 0x11:
        arg2 = 0;
        break;
    case 0x12:
        arg2 = 1;
        break;
    case 0x13:
        arg2 = 2;
        break;
    default:
        return;
    }
    func_8007D334(arg0, arg1, arg2);
}
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_8007D6B8);
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_8007D7A8);
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_8007D918);
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_8007DB28);
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_8007DC74);
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_8007E020);
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_8007E24C);
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_8007E2D8);
extern s32 D_800926B0;
extern s32 D_80094818;
void func_8007E31C(void *arg0, void *arg1, void *arg2) {
    void *temp_v1;

    if (D_800926B0 < 0x64) {
        temp_v1 = (D_800926B0 << 5) + (u8 *)&D_80094818;
        (*(s16 *)((u8 *)(temp_v1) + 0x10)) = (s16) (*(s32 *)((u8 *)(arg0) + 0));
        (*(s16 *)((u8 *)(temp_v1) + 0x12)) = (s16) (*(s32 *)((u8 *)(arg0) + 4));
        (*(s16 *)((u8 *)(temp_v1) + 0x14)) = (s16) (*(s32 *)((u8 *)(arg0) + 8));
        (*(s16 *)((u8 *)(temp_v1) + 0x18)) = (s16) (*(s32 *)((u8 *)(arg1) + 0));
        (*(s16 *)((u8 *)(temp_v1) + 0x1A)) = (s16) (*(s32 *)((u8 *)(arg1) + 4));
        (*(s16 *)((u8 *)(temp_v1) + 0x1C)) = (s16) (*(s32 *)((u8 *)(arg1) + 8));
        (*(u8 *)((u8 *)(temp_v1) + 4)) = (u8) (*(u8 *)((u8 *)(arg2) + 0));
        (*(u8 *)((u8 *)(temp_v1) + 5)) = (u8) (*(u8 *)((u8 *)(arg2) + 1));
        (*(u8 *)((u8 *)(temp_v1) + 6)) = (u8) (*(u8 *)((u8 *)(arg2) + 2));
        D_800926B0 += 1;
    }
}
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_8007E3CC);
extern u8 D_80092708;
void func_8007E528(s32 arg0) {
    if (D_80092708 == 0 && arg0 == 0xA) {
        func_8008EB4C(0x24);
    }
    D_80092708 = arg0;
}
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_8007E574);

s32 func_8007E624(void) {
    return D_800926DC;
}

INCLUDE_ASM("asm/battling/nonmatchings/misc", func_8007E634);

void func_8007E894(s16 arg0, s16 arg1) {
    D_800926E8 = arg0;
    D_800926EC = arg1;
}

INCLUDE_ASM("asm/battling/nonmatchings/misc", func_8007E8AC);

void func_8007E954(s32 arg0) {
    D_800912DC = arg0;
}

INCLUDE_ASM("asm/battling/nonmatchings/misc", func_8007E964);
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_8007EB6C);
void func_8007EBE0(u8 *arg0) {
    s32 savedX;
    u8 c;

    c = *arg0;
    savedX = D_800926E8;
    while (c != 0) {
        arg0++;
        func_8007E964(c);
        c = *arg0;
    }
    D_800926E8 = savedX;
    D_800926EC += 0x14;
}
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_8007EC54);
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_8007ECF0);
void func_8007ED84(u8 *arg0, s32 arg1) {
    s32 savedX;
    u8 unused[8]; /* retail frame has 8 bytes of locals no code touches */
    u8 c;

    savedX = D_800926E8;
    D_800926E8 = savedX - arg1;
    c = *arg0;
    while (c != 0) {
        arg0++;
        func_8007E964(c);
        c = *arg0;
    }
    D_800926E8 = savedX;
    D_800926EC += 0x14;
}
extern u8 D_800926F0;
extern u8 D_800926F4;
extern u8 D_800926F8;
void func_8007EE08(s32 arg0) {
    s32 unset = 0xFF;

    if (arg0 != 0) {
        D_800926F4 = 0xFF;
        D_800926F8 = 0;
        D_800926F0 = D_80059488 * 0x14;
    } else {
        D_800926F0 = unset;
        D_800926F4 = unset;
        D_800926F8 = unset;
    }
}
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_8007EE68);
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_8007EEE8);
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_8007EFB4);
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_8007F05C);
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_8007F258);

void func_8007F834(void) {
    D_80092740 = 0;
    D_8009273C = 0;
    D_80092744 = 0;
}

INCLUDE_ASM("asm/battling/nonmatchings/misc", func_8007F854);
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_8007F8B4);

void func_8007F8E4(void) {
    s32 var_a0;

    func_80080C48(0);
    if ((D_80099D9F != 0) || (D_80092950 == 1)) {
        var_a0 = 3;
    } else {
        D_80092950 -= 1;
        var_a0 = 6;
    }
    func_80083C0C(var_a0);
}

INCLUDE_ASM("asm/battling/nonmatchings/misc", func_8007F948);

#ifndef XENO_PC_PORT
s32 func_8007F97C(void) {
    return *(s32 *)((u8 *) &D_800912F4 + (D_80099D98__s_u8 * 4));
}
#endif

INCLUDE_ASM("asm/battling/nonmatchings/misc", func_8007F9A0);
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_8007FB0C);
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_8007FBEC);
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_8007FE48);
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_8007FF70);

void func_80080054(void) {
    D_80099D98[0] = func_8007FF70(D_80099D98[0], 2, 0);
}


void func_80080090(void) {
    D_80099DA1__a[0] = func_8007FF70(D_80099DA1__a[0], 7, 2);
}


void func_800800CC(void) {
    D_80099D9A__a[0] = func_8007FF70(D_80099D9A__a[0], 4, 2);
}


void func_80080108(void) {
    D_80099D9B[0] = func_8007FF70(D_80099D9B[0], 1, 1);
}


void func_80080144(void) {
    D_80099D9C[0] = func_8007FF70(D_80099D9C[0], 1, 1);
}


void func_80080180(void) {
    D_80099D9D__a[0] = func_8007FF70(D_80099D9D__a[0], 1, 1);
}


void func_800801BC(void) {
    D_80099D9E__a[0] = func_8007FF70(D_80099D9E__a[0], 1, 1);
}


void func_800801F8(void) {
    D_80099D9F__a[0] = func_8007FF70(D_80099D9F__a[0], 3, 2);
}


void func_80080234(void) {
    D_80092884 = func_8007FF70(D_80092884, 1, 1);
}


void func_80080268(void) {
    D_80099DA2__a[0] = func_8007FF70(D_80099DA2__a[0], 0xD, 3);
}

INCLUDE_ASM("asm/battling/nonmatchings/misc", func_800802A4);
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_8008040C);
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_80080570);
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_80080644);
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_80080780);

void func_800808F4(void) {
    D_80092924 = 1;
    func_8007F834();
}


void func_80080920(void) {
    D_80092924 = 1;
    func_800719F0();
    func_8008509C(0, 0);
    func_8008509C(1, 1);
}

extern s32 D_80092738;
extern u8 D_800915AC[];
void func_80080964(s32 arg0) {
    s32 prev;

    if (arg0 == 0xFF) {
        D_80092734 = D_80092738;
        return;
    }
    prev = D_80092734;
    D_80092734 = (s32)&D_800915AC[arg0 * 0x3C];
    D_80092738 = prev;
}

s32 func_800809BC(void) {
    return D_80092734 == (s32) &D_80091660;
}


void func_800809D8(void) {
    func_80039FF8();
    D_80092734 = 0;
    func_80080964(3);
    D_80091672 = 0;
    D_800916AE = 0;
    D_800928C8 = 0;
    D_80092758 = 0;
    func_8007F834();
    D_80092924 = 0;
    func_80080AA0(0);
    D_80092940 = 0;
    D_800928D8 = func_800891C0(6);
}


void func_80080A58(void) {
    if (D_80092940 == 0) {
        ArchiveCdDataSync(0);
        HeapFree(D_800928D8);
        D_80092940 = 1;
    }
}


void func_80080AA0(s32 arg0) {
    if (arg0 != 0) {
        D_80092760 = 0;
    }
    if (D_80092760 != 0) {
        HeapFree(D_80092760);
        D_80092760 = 0;
    }
}

INCLUDE_ASM("asm/battling/nonmatchings/misc", func_80080AE8);
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_80080B58);

void func_80080C48(s32 arg0) {
    func_80039FF8();
    func_8008EB4C(0x1F);
    if (arg0 == 1) {
        D_80092734 = 0;
        func_80080964(0);
        D_800915BE = 0;
        D_80091636 = 1;
        goto block_4;
    }
    if (arg0 == 2) {
        D_80092734 = 0;
        func_80080964(7);
        D_80091762 = 1;
block_4:
        func_80083C0C(0);
        D_80092758 = 1;
        D_800926FC = 0;
        D_8009275C = 1;
        func_80080B58();
        return;
    }
    func_8007F8B4();
}


void func_80080D10(void) {
    D_800926DC = 0;
}

INCLUDE_ASM("asm/battling/nonmatchings/misc", func_80080D20);
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_80080F04);
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_80081094);
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_80081100);
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_800811AC);
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_800812BC);
extern s32 D_80092700;
extern s32 D_80092704;
void func_800814AC(void) {
    u8 *entry;
    u32 i;

    i = 0;
    entry = D_800915AC;
    do {
        func_800812BC(entry);
        i++;
        entry += 0x3C;
    } while (i < 8);
    D_80092734 = 0;
    D_80092700 = 0;
    D_80092704 = 1;
    func_80080F04();
}
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_8008151C);
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_8008162C);
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_80081A44);
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_80081D2C);

void func_80081E00(void) {
    s16 temp_v1;

    D_8009A1CA = 0x60;
    D_8009A2C2 = 0x60;
    D_8009A1C4 = 0x7F;
    D_8009A1C5 = 0x7F;
    D_8009A1C6 = 0x7F;
    D_8009A2BC = 0x7F;
    D_8009A2BD = 0x7F;
    D_8009A2BE = 0x7F;
    temp_v1 = D_8009286C - 0x60;
    D_8009A1CE = temp_v1;
    D_8009A2C6 = temp_v1;
}


void func_80081E6C(void) {
    D_8009A1CA = 0;
    D_8009A2C2 = 0;
    D_8009A1C4 = 0;
    D_8009A1C5 = 0;
    D_8009A1C6 = 0;
    D_8009A2BC = 0;
    D_8009A2BD = 0;
    D_8009A2BE = 0;
    (*(u16*)&D_8009A1CE) = D_8009286C;
    (*(u16*)&D_8009A2C6) = D_8009286C;
}

INCLUDE_ASM("asm/battling/nonmatchings/misc", func_80081ECC);
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_80082178);
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_80082300);
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_80082458);
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_80082488);
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_80082880);
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_800828C4);
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_800828F8);
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_80082A70);
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_80082C4C);
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_80082E60);
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_800831C8);
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_800832C0);
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_80083310);
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_8008369C);
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_80083738);
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_80083B54);
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_80083BB4);
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_80083C0C);

s32 func_80083CD8(void) {
    return D_80092790;
}

INCLUDE_ASM("asm/battling/nonmatchings/misc", func_80083CE8);
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_80083DCC);
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_800840CC);
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_800846A0);

s32 func_800849E0(void *arg0) {
    s32 temp_s0;

    temp_s0 = *(s32 *)((s8*)(arg0) + 0x284);
    func_8008AC0C(temp_s0);
    func_80080D20(*(s32 *)((s8*)(((D_800928A0 * 4) + temp_s0)) + 4) + 4);
    func_8008AE1C(temp_s0);
    func_80086E24();
    return 0;
}


void func_80084A40(void *arg0) {
    func_8008AC0C(*(s32 *)((s8*)(arg0) + 0x284));
}


s32 func_80084A64(void *arg0) {
    s32 temp_s0;

    temp_s0 = *(s32 *)((s8*)(arg0) + 0x284);
    func_8008E3CC(*(s32 *)((s8*)(((D_800928A0 * 4) + temp_s0)) + 4) + 8, 0xC0, 0);
    func_80080D20(*(s32 *)((s8*)(((D_800928A0 * 4) + temp_s0)) + 4) + 4);
    func_8008AE1C(temp_s0);
    func_80086E24();
    return 0;
}


void func_80084AE0(void) {
    if (D_80092780 != 0) {
        func_8008E120();
        func_8007F258(D_80092938, 0);
        func_80080D20(D_80092938);
        func_8008E3CC(D_80092938, 0xC0, 1);
        func_8008BC04();
    }
}


void func_80084B48(void) {
    s16 temp_v0;

    if (D_80092780 != 0) {
        temp_v0 = D_80092780 - 4;
        D_80092780 = temp_v0;
        if (temp_v0 <= 0) {
            D_80092780 = 0;
            D_80092784 = 0;
            func_8003A838(D_80092948, 0x100, 0);
            D_8009292C = 0x100;
            func_8008E064();
            return;
        }
        func_8008E3CC(D_80092938, temp_v0, 1);
        return;
    }
    D_80092784 = 0;
}


void func_80084BEC(void *arg0) {
    s32 temp_s2;
    s32 temp_v0;
    void *temp_s1;

    temp_s2 = *(s32 *)((s8*)(*(void **)((s8*)(*(void **)((s8*)(*(void **)((s8*)(arg0) + 0x5C)) + 4)) + 4)) + 0x30);
    temp_s1 = func_80089C54();
    temp_v0 = func_80089FC4();
    func_80089E2C(temp_s1, temp_v0);
    func_8008A184(temp_v0, &D_80091FB0);
    func_80089C88(temp_s2, temp_s1);
    *(s16 *)((s8*)(temp_s1) + 0x46) = 0xC00;
    *(s16 *)((s8*)(temp_s1) + 0x44) = 0;
    *(s16 *)((s8*)(temp_s1) + 0x48) = 0x400;
}

INCLUDE_ASM("asm/battling/nonmatchings/misc", func_80084C88);

void func_80084FD0(void) {
    if ((D_80092784 != 0) && (D_80059488 & 1)) {
        func_8008E120();
    }
}


void func_80085014(void) {
    D_80092920 &= 0xFE;
    func_80083BB4(0);
    func_80083C0C(5);
    D_80099D9E = 1;
    D_80099D9D = 1;
    D_800928C8 = 6;
}


void func_80085070(void) {
    D_80099D9E = 0;
    D_80099D9D = 0;
    D_80092920 |= 1;
}

INCLUDE_ASM("asm/battling/nonmatchings/misc", func_8008509C);
extern s32 D_800927B4[];
void func_80085134(s32 arg0) {
    ArchiveCdDataSync(0);
    if (D_800927B4[arg0] != 0) {
        HeapFree(D_800927B4[arg0]);
        D_800927B4[arg0] = 0;
    }
}

void func_8008518C(void *arg0, s32 arg1) {
    *(s32 *)((s8*)(arg0) + 4) = HeapAlloc(ArchiveDecodeAlignedSize(*(u16 *)((s8*)(arg0) + 0)), arg1);
}


void func_800851D4(void) {
    func_8003852C(D_800927C4);
    if (D_800917F0 != 0) {
        func_80039C4C(D_80092948);
        func_800399D4(D_80092948);
    }
    func_80088A40();
    ChangeGameState(1);
    DrawSync(0);
    Vsync(2);
    D_8005061C = 1;
    MainLoop(0);
}

INCLUDE_ASM("asm/battling/nonmatchings/misc", func_80085264);
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_800852C4);

void func_80085E34(void *arg0, void *arg1) {
    *(u16 *)((s8*)(arg1) + 0) = (u16) (*(u16 *)((s8*)(arg0) + 0) + 0x18);
    *(s16 *)((s8*)(arg1) + 2) = (s16) (*(u16 *)((s8*)(arg0) + 2) + 6);
    *(u16 *)((s8*)(arg1) + 0) = (u16) (*(u16 *)((s8*)(arg1) + 0) + 0x4F);
}


void func_80085E60(void *arg0, void *arg1) {
    *(u16 *)((s8*)(arg1) + 0) = (u16) (0x8B - *(u16 *)((s8*)(arg0) + 0));
    *(s16 *)((s8*)(arg1) + 2) = (s16) (0x20 - *(u16 *)((s8*)(arg0) + 2));
    *(u16 *)((s8*)(arg1) + 0) = (u16) (*(u16 *)((s8*)(arg1) + 0) + 0x4F);
}


void func_80085E90(s32 arg0, s16 *arg1, s32 arg2) {
    s16 var_v0;

    if (arg0 != 0) {
        var_v0 = 0xDA - arg2;
    } else {
        var_v0 = arg2 + 0x67;
    }
    *arg1 = var_v0;
}


void func_80085EAC(s32 arg0, s16 *arg1, s32 arg2) {
    s16 var_v0;

    if (arg0 != 0) {
        var_v0 = 0x20 - arg2;
    } else {
        var_v0 = arg2 + 6;
    }
    *arg1 = var_v0;
}

INCLUDE_ASM("asm/battling/nonmatchings/misc", func_80085EC8);
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_800864B4);
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_800866D4);
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_800868E0);
extern s32 D_8009A300;
void func_80086E24(void) {
    AddPrim(D_80092938, (D_800928A0 * 0x310) + (u8 *)&D_8009A300);
}
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_80086E70);
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_80086FF8);
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_80087068);
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_800875EC);
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_80087650);
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_80087698);
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_8008779C);
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_80087830);
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_800878DC);
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_80087AB0);
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_80087B74);
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_80087E38);
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_80087EA0);
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_8008820C);

void func_800882D4(s32 arg0, s32 arg1, s32 arg2, s16 arg3) {
    void *temp_v0;

    temp_v0 = func_8008820C();
    if (temp_v0 != NULL) {
        *(s16 *)((s8*)(temp_v0) + 0x30) = arg3;
    }
}

INCLUDE_ASM("asm/battling/nonmatchings/misc", func_80088308);
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_8008832C);
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_800884E0);
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_8008859C);
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_80088658);
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_800886FC);
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_80088754);
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_800887A4);
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_80088838);
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_800888B0);
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_800888E4);
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_80088908);

void func_80088940(void) {
    D_8006F980[0] |= 0x10000;
}

INCLUDE_ASM("asm/battling/nonmatchings/misc", func_8008895C);
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_800889C8);
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_80088A40);
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_80088AF8);

void func_80088BD4(void) {
    func_800888B0();
    func_800889C8();
}

INCLUDE_ASM("asm/battling/nonmatchings/misc", func_80088BFC);

void func_80088C28(void) {
    if (g_C1ButtonState & 4) {
        D_800927F4 += 1;
    }
    if (g_C1ButtonState & 1) {
        D_800927F4 -= 1;
    }
    if (D_800927F4 < 0) {
        D_800927F4 = 0;
    }
    HeapDebugDump(1, D_800927F4, 0xA, 0x80AD);
}

extern s32 D_8009A0D8;
void func_80088CBC(s32 arg0) {
    void *temp_s0;

    temp_s0 = (arg0 * 0xF8) + (u8 *)&D_8009A0D8;
    (*(s8 *)((u8 *)(temp_s0) + 0x77)) = 3;
    (*(s8 *)((u8 *)(temp_s0) + 0x7B)) = 0x7D;
    (*(s16 *)((u8 *)(temp_s0) + 0x80)) = 0x3000;
    (*(s16 *)((u8 *)(temp_s0) + 0x82)) = GetClut(0x3F0, 0xC0);
}
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_80088D1C);
#endif
