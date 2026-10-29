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
s32 func_8007FF70(s32, s32, s32); /* retail: no andi on entry or return */
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

s32 func_800707D8(s32 arg0, s32 arg1, s32 arg2) {
    s32 var_v0;

    var_v0 = arg0 - arg1;
    if (var_v0 < 0) {
        var_v0 += 1;
        var_v0 -= arg2;
    } else {
        var_v0 -= 1;
        var_v0 += arg2;
    }
    return var_v0 / arg2;
}

extern s32 D_800925F4;
extern s32 D_8009867C[];
extern s32 D_80098680;
extern s32 D_80098684;
s32 func_800707D8(s32 arg0, s32 arg1, s32 arg2);

void func_80070808(void *arg0, s32 arg1) {
    D_8009867C[0] += func_800707D8(*(s32 *)((s8*)(arg0) + 0), D_8009867C[0], arg1);
    D_80098684 += func_800707D8(*(s32 *)((s8*)(arg0) + 8), D_80098684, arg1);
    D_80098680 += func_800707D8(*(s32 *)((s8*)(arg0) + 4), D_80098680 + D_800925F4, arg1);
}

extern s32 D_8009871C[];
extern s32 D_80098720__s asm("D_80098720");
extern s32 D_80098724;
s32 func_800707D8(s32 arg0, s32 arg1, s32 arg2);
void func_800828F8(void *, void *, s32);

void func_800708C4(void *arg0, s32 arg1) {
    s32 sp10[3];

    sp10[0] = func_800707D8(*(s32 *)((s8*)(arg0) + 0), D_8009871C[0], arg1);
    sp10[1] = func_800707D8(*(s32 *)((s8*)(arg0) + 4), D_80098720__s, arg1);
    sp10[2] = func_800707D8(*(s32 *)((s8*)(arg0) + 8), D_80098724, arg1);
    func_800828F8(D_8009871C, sp10, 0x3D00);
    D_8009871C[0] += sp10[0];
    D_80098720__s += sp10[1];
    D_80098724 += sp10[2];
}

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

extern s32 D_80092934;

s32 func_80070FD8(void *arg0) {
    typedef struct { u32 x; u32 y; u32 z; u32 w; } Vec4_70FD8;
    Vec4_70FD8 v = *(Vec4_70FD8 *)arg0;
    register s32 var_v0 asm("$2");

    v.x = v.x - 0x3F80;
    v.z = v.z - 0x3F80;
    var_v0 = ((ratan2(v.x, v.z) & 0xFFF) >= 0x201 ? 0x800 : 0xC00) - D_80092934;
    *(volatile s32 *)((s8 *)arg0 + 0x58) = var_v0;
    *(s32 *)((s8 *)arg0 + 0x48) = 0xFF;
    *(s16 *)((s8 *)arg0 + 0xCE) = 0;
    return 0;
}

INCLUDE_ASM("asm/battling/nonmatchings/misc", func_8007107C);
extern s32 D_800925E0;
extern s32 D_800925E4;
extern u8 *D_80092868;
extern u8 D_800929E4[];

void func_80071724(void *arg0) {
    *(s32 *)(D_80092868 + 0x7C) = D_800925E0 | (D_800925E4 << 0x10);
    AddPrim(arg0, D_80092868 + 0x74);
    AddPrim(arg0, D_800929E4 + D_800928A0 * 8);
}

INCLUDE_ASM("asm/battling/nonmatchings/misc", func_80071794);

void func_800718C0(void) {
    D_800925D8 = -1;
    D_800925D4 = 0;
    func_80032F54(&D_8009868C, 0x140, 0x30, 0x1C, 0x9A, 0x40, 4);
}

extern s32 D_8009105C[];
extern s32 D_800925E0;
extern s32 D_800925E4;
extern s32 D_800925E8;
extern s32 D_800925EC;
extern s16 D_80092600;
extern s8 D_80092608;
extern s16 D_80098692;
extern s16 D_80098698;
extern s16 D_800970C4;
extern s16 D_800970CC;
extern s16 D_800987E0;
extern s16 D_800987E8;

void func_8007191C(s32 arg0) {
    D_80092608 = arg0 == 0;
    if (arg0 == 0) {
        func_8008EB4C(0x37);
        D_80098698 = 2;
        D_80098692 = 0xB4;
    } else {
        D_80098698 = 4;
        D_80098692 = 0x9A;
    }
    func_80070F80(D_8009105C[arg0]);
    D_800925E8 = 0xA0;
    D_800925E0 = 0xA0;
    D_800925EC = 0x6D;
    D_800925E4 = 0x6D;
    D_80092600 = 0;
    D_800987E0 = D_800987E8;
    D_800970C4 = D_800970CC;
}


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

void func_800346A4(s32 *);
void func_80034800(s32 *, s32, s32, s32);
void func_80034874(s32 *, s32);
void func_80034888(s32 *, s32, s32);
void func_80036420();
void func_8007107C();
void func_8007099C(s32);
void func_80079DF0(void *, void *);
s32 func_800707D8(s32 arg0, s32 arg1, s32 arg2);
void func_80071724(void *arg0);
extern s32 D_800925E0;
extern s32 D_800925E4;
extern s32 D_800925E8;
extern s32 D_800925EC;
extern u8 D_800925F0;
extern s32 D_800928E8;
extern u16 g_C1ButtonStatePressedOnce;
extern u16 g_C1ButtonStateReleased;
extern u8 D_80092604__u8 asm("D_80092604");
extern s32 D_8009868C__a[] asm("D_8009868C");
extern s32 D_80097010;
extern s32 D_8009872C;

void func_80071AD0(void) {
    s32 *p;

    if ((D_800925F0 != 0) && (D_800928E8 & 4)) {
        func_80071724((void *)D_80092938);
    }
    func_80036420();
    p = D_8009868C__a;
    if ((*(u8*)&D_80092608) != 0) {
        if (g_C1ButtonStatePressedOnce & 0x1000) {
            func_8008EB4C(0x1E);
            D_80092604__u8 -= 1;
        }
        if (g_C1ButtonStatePressedOnce & 0x4000) {
            func_8008EB4C(0x1E);
            D_80092604__u8 += 1;
        }
        if ((s8)D_80092604__u8 >= 8) {
            D_80092604__u8 = 0;
        }
        if ((s8)D_80092604__u8 < 0) {
            D_80092604__u8 = 7;
        }
        func_80034800(D_80092954__a_s32, (D_800928E8 * 7) & 0x3F, 0xC0, 0x10);
        func_80034874(D_80092954__a_s32, (s8)D_80092604__u8);
        if (g_C1ButtonStateReleased & 0x20) {
            func_8008EB4C(0x21);
            func_8007191C((s8)D_80092604__u8 + 1);
            func_800346A4(p);
        }
    }
    if (*(u8 *)D_800925F8 == 0) {
        func_8007191C(0);
    }
    func_8007107C();
    D_800925E0 += func_800707D8(D_800925E8, D_800925E0, 4);
    D_800925E4 += func_800707D8(D_800925EC, D_800925E4, 4);
    if ((*(u8*)&D_80092608) != 0) {
        func_80034888(D_80092954__a_s32, D_80092938, D_800928A0);
    }
    if (D_800925D4 != D_800925D8) {
        *((s8 *)p + 0x68) = 3;
        func_800346A4(p);
        func_80034714(p, GetStringEntry(D_80092880[0], D_800925D4));
        D_800925D8 = D_800925D4;
    }
    func_80079DF0(&D_8009872C, &D_80097010);
    func_8007099C(D_80092904[0]);
}

typedef struct { s32 x; s32 y; s32 z; s32 w; } Vec4_71DA4;
extern s32 D_80091084[]; /* 8 {dx, dz} pairs; the dz half is retail D_80091088 (D_80091084 + 4) */

void func_80071DA4(s32 *arg0) {
    Vec4_71DA4 v;
    s32 *p;
    s32 i;
    s32 cnt;
    s32 best;
    s32 bestval;
    s32 temp;

    p = arg0;
    cnt = 0;
    do {
        bestval = p[1] = func_80082488(p, 1);
        i = 0;
        do {
            v = *(Vec4_71DA4 *)p;
            v.x += D_80091084[i * 2] * 0xC0;
            v.z += D_80091084[i * 2 + 1] * 0xC0;
            temp = func_80082488(&v, 1);
            if (temp < bestval - 0x40) {
                best = i;
                bestval = temp;
            }
            i++;
        } while (i < 8);
        if (bestval >= p[1] - 0x40) {
            p[1] = func_80082488(p, 1);
            break;
        }
        p[0] -= D_80091084[best * 2] * 0xC0;
        cnt++;
        p[2] -= D_80091084[best * 2 + 1] * 0xC0;
    } while (cnt < 0x14);
    arg0[0xB0 / 4] = func_80082488(arg0, 1);
    arg0[0xD0 / 4] = (arg0[0xD0 / 4] & 0x9FFFFFFF) | ((((u32)func_800828C4(arg0) >> 0x18) & 3) << 0x1D);
}

void func_80071DA4(s32 *);
extern s32 D_8009284C;
extern s32 D_80097010;
extern s16 D_800970C4;
extern s16 D_800970CC;
extern s32 D_80098720[];
extern s32 D_8009872C;
extern s16 D_800987E0;
extern s16 D_800987E8;

s32 func_80071F8C(s32 arg0) {
    switch (arg0) {
    case 0:
        func_80039C4C(D_80092948);
        func_80039FF8();
        return;
    case 1:
        func_80071DA4(&D_8009872C);
        if (((D_800987E0 << 8) / D_800987E8) > ((D_800970C4 << 8) / D_800970CC)) {
            D_800925D4 = 0x43;
            return;
        }
        D_800925D4 = 0x44;
        return;
    case 2:
        func_80071DA4(&D_80097010);
        return;
    case 3:
        D_80098720[0] = -D_8009284C;
        if (D_80098720[0] < -0x600) {
            D_80098720[0] = -0x600;
        }
        return;
    }
}


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

s32 func_8008A2B8(s32);
s32 func_8008A3E0(s32);
s32 func_8008C2C0(s32);
void func_80078F00(void *);
extern void *D_80092614;
extern s8 D_80092618;
extern s32 D_8009261C;
extern s32 D_80092620;
extern s32 D_80092624;
extern s32 D_80092628;
extern s32 D_8009262C;
extern u16 D_80092632;
extern s16 D_800928D0;
extern s32 D_80096FA8;
extern s32 D_80096FAC;
extern s32 D_80096FB0;

typedef struct {
    u8 pad0[0x14];
    s16 unk14;
    s16 unk16;
    s16 unk18;
    s16 unk1A;
    s16 unk1C;
    u16 unk1E;
} func_800725B0_s;

typedef struct {
    u8 pad0[0x5C];
    s32 unk5C;
    u8 pad60[0x89C];
    func_800725B0_s *unk8FC;
} func_800725B0_arg;

void func_800725B0(void *arg0) {
    func_800725B0_s *temp_v0;

    func_80030988(5, 4, 0x40, 0x40);
    D_800910F0 = func_8008A3E0(func_8008A2B8(0x10));
    D_80092610 = func_8008C2C0(((func_800725B0_arg *)arg0)->unk5C);
    func_8008976C(0x280, 0xDA);
    SetGeomScreen(0x400);
    D_800928D0 = 0;
    D_80092614 = arg0;
    func_80078F00(arg0);
    D_80096FB0 = 0;
    D_80096FAC = 0;
    D_80096FA8 = 0;
    temp_v0 = *(func_800725B0_s **)((s8*)(arg0) + 0x8FC);
    D_80092618 = 1;
    D_8009261C = temp_v0->unk14;
    D_80092620 = temp_v0->unk16;
    D_80092624 = temp_v0->unk18;
    D_80092628 = temp_v0->unk1A;
    D_8009262C = temp_v0->unk1C;
    D_80092632 = temp_v0->unk1E;
}


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
/* GTE GPF12 interpolation: out[i] = (in[i] * arg2) >> 12 via cop2 IR1..IR3. */
void func_80073064(void *arg0, void *arg1, s32 arg2) {
    register s32 t0 asm("$8") = (s32) *(s16 *)((s8 *)arg0 + 0);
    register s32 t1 asm("$9") = (s32) *(s16 *)((s8 *)arg0 + 2);
    register s32 t2 asm("$10") = (s32) *(s16 *)((s8 *)arg0 + 4);

    __asm__ volatile ("mtc2 %0, $8" : : "r"(arg2));
    __asm__ volatile ("mtc2 %0, $9" : : "r"(t0));
    __asm__ volatile ("mtc2 %0, $10" : : "r"(t1));
    __asm__ volatile ("mtc2 %0, $11" : : "r"(t2));
    __asm__ volatile ("nop\n\tnop\n\t.word 0x4B98003D"); /* gte_gpf12 */
    __asm__ volatile ("mfc2 %0, $9" : "=r"(t0));
    __asm__ volatile ("mfc2 %0, $10" : "=r"(t1));
    __asm__ volatile ("mfc2 %0, $11" : "=r"(t2));
    *(volatile s16 *)((s8 *)arg1 + 0) = (s16) t0;
    *(volatile s16 *)((s8 *)arg1 + 2) = (s16) t1;
    *(volatile s16 *)((s8 *)arg1 + 4) = (s16) t2;
}

/* GTE GPF12 interpolation: out[i] = (in[i] * arg2) >> 12 via cop2 IR1..IR3. */
void func_800730AC(void *arg0, void *arg1, s32 arg2) {
    register s32 t0 asm("$8") = (s32) *(s16 *)((s8 *)arg0 + 0);
    register s32 t1 asm("$9") = (s32) *(s16 *)((s8 *)arg0 + 2);
    register s32 t2 asm("$10") = (s32) *(s16 *)((s8 *)arg0 + 4);

    __asm__ volatile ("mtc2 %0, $8" : : "r"(arg2));
    __asm__ volatile ("mtc2 %0, $9" : : "r"(t0));
    __asm__ volatile ("mtc2 %0, $10" : : "r"(t1));
    __asm__ volatile ("mtc2 %0, $11" : : "r"(t2));
    __asm__ volatile ("nop\n\tnop\n\t.word 0x4B90003D"); /* gte_gpf0 */
    __asm__ volatile ("mfc2 %0, $9" : "=r"(t0));
    __asm__ volatile ("mfc2 %0, $10" : "=r"(t1));
    __asm__ volatile ("mfc2 %0, $11" : "=r"(t2));
    *(volatile s16 *)((s8 *)arg1 + 0) = (s16) t0;
    *(volatile s16 *)((s8 *)arg1 + 2) = (s16) t1;
    *(volatile s16 *)((s8 *)arg1 + 4) = (s16) t2;
}

/* GTE GPF12 interpolation: out[i] = (in[i] * arg2) >> 12 via cop2 IR1..IR3. */
void func_800730F4(void *arg0, void *arg1, s32 arg2) {
    register s32 t0 asm("$8") = (s32) *(s16 *)((s8 *)arg0 + 0);
    register s32 t1 asm("$9") = (s32) *(s16 *)((s8 *)arg0 + 4);
    register s32 t2 asm("$10") = (s32) *(s16 *)((s8 *)arg0 + 8);

    __asm__ volatile ("mtc2 %0, $8" : : "r"(arg2));
    __asm__ volatile ("mtc2 %0, $9" : : "r"(t0));
    __asm__ volatile ("mtc2 %0, $10" : : "r"(t1));
    __asm__ volatile ("mtc2 %0, $11" : : "r"(t2));
    __asm__ volatile ("nop\n\tnop\n\t.word 0x4B98003D"); /* gte_gpf12 */
    __asm__ volatile ("mfc2 %0, $9" : "=r"(t0));
    __asm__ volatile ("mfc2 %0, $10" : "=r"(t1));
    __asm__ volatile ("mfc2 %0, $11" : "=r"(t2));
    *(volatile s16 *)((s8 *)arg1 + 0) = (s16) t0;
    *(volatile s16 *)((s8 *)arg1 + 2) = (s16) t1;
    *(volatile s16 *)((s8 *)arg1 + 4) = (s16) t2;
}

INCLUDE_ASM("asm/battling/nonmatchings/misc", func_8007313C);
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_800731F8);
/* The retail copy loop uses t0; preserve its register and byte-count contract. */
void func_800732AC(void *dst, void *src, s32 bytes) {
    register s32 word asm("$8");
    s32 end = bytes + (s32)src;
    do {
        word = *(s32 *)src;
        src = (u8 *)src + 4;
        *(s32 *)dst = word;
        dst = (u8 *)dst + 4;
    } while ((s32)src != end);
}

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
extern s32 D_80092648;
extern s32 D_80092A24;
extern s32 D_80092A28;
extern s32 D_80092A2C;
extern u8 D_80091C0C[];
void func_8008D680(void *, void *, s32);
void func_8008DA48(void *, void *, void *);

void func_8007334C(void *arg0, void *arg1) {
    register void *temp_s0 asm("$16");
    register s32 var_a2 asm("$6");
    s32 temp18;
    register s32 t4 asm("$12");
    register s32 t5 asm("$13");
    register s32 t6 asm("$14");

    var_a2 = D_80092648;
    temp_s0 = (void *) D_80092644;
    *(s16 *)((s8 *)temp_s0 + 0x14) = (s16) D_80092A24;
    *(s16 *)((s8 *)temp_s0 + 0x16) = (s16) D_80092A28;
    temp18 = D_80092A2C;
    *(s16 *)((s8 *)temp_s0 + 0x34) = 0;
    *(s16 *)((s8 *)temp_s0 + 0x36) = 0;
    *(s16 *)((s8 *)temp_s0 + 0x38) = 0;
    *(s16 *)((s8 *)temp_s0 + 0x18) = (s16) temp18;
    func_8008D680(temp_s0, &D_80091C0C[0], var_a2);
    t4 = *(s32 *)((s8 *)arg1 + 0x14);
    t5 = *(s32 *)((s8 *)arg1 + 0x18);
    __asm__ volatile ("ctc2 %0, $5" : : "r"(t4));
    t6 = *(s32 *)((s8 *)arg1 + 0x1C);
    __asm__ volatile ("ctc2 %0, $6" : : "r"(t5));
    __asm__ volatile ("ctc2 %0, $7" : : "r"(t6));
    t4 = *(s32 *)((s8 *)arg1 + 0x0);
    t5 = *(s32 *)((s8 *)arg1 + 0x4);
    __asm__ volatile ("ctc2 %0, $0" : : "r"(t4));
    __asm__ volatile ("ctc2 %0, $1" : : "r"(t5));
    t4 = *(s32 *)((s8 *)arg1 + 0x8);
    t5 = *(s32 *)((s8 *)arg1 + 0xC);
    t6 = *(s32 *)((s8 *)arg1 + 0x10);
    __asm__ volatile ("ctc2 %0, $2" : : "r"(t4));
    __asm__ volatile ("ctc2 %0, $3" : : "r"(t5));
    __asm__ volatile ("ctc2 %0, $4" : : "r"(t6));
    func_8008DA48(temp_s0, arg0, arg1);
}

INCLUDE_ASM("asm/battling/nonmatchings/misc", func_80073424);
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_80073644);
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_80073B7C);
void func_80073CA4(s32 arg0) {
    s32 var_a1;
    s32 var_a2;
    void *temp_v1;

    var_a2 = 0;
    do {
        var_a1 = var_a2 * 0x54 + 0x104;
        temp_v1 = (void *)(arg0 + var_a1);
        if (*(u8 *)((s8*)(temp_v1) + 0x45) == 2) {
            *(u8 *)((s8*)(temp_v1) + 0x45) = 1U;
        } else {
            *(u8 *)((s8*)(temp_v1) + 0x45) = 0U;
        }
        var_a2 += 1;
    } while (var_a2 < 0x10);
}

INCLUDE_ASM("asm/battling/nonmatchings/misc", func_80073CEC);
s32 func_80073DE4(void *arg0, s32 arg1) {
    s32 temp_a1;
    register s32 var_v0 asm("$2");

    temp_a1 = *(s16 *)((s8*)(arg0) + 0xB6) + arg1;
    var_v0 = 1;
    if (temp_a1 >= 0x1001) {
        var_v0 = ((temp_a1 - 0xFF1) / 20) < *(s16 *)((s8*)(arg0) + 0xB4);
    }
    return var_v0;
}

s32 func_80073E2C(void *arg0, s32 arg1, s32 arg2) {
    s32 q;

    if (D_800928C8 == 4 || D_800928C8 == 6) {
        return 1;
    }
    *(s16 *)((s8*)(arg0) + 0xB6) += arg1;
    if (*(s16 *)((s8*)(arg0) + 0xB6) > 0x1000) {
        q = (*(s16 *)((s8*)(arg0) + 0xB6) - 0x1000) / 20;
        if (q >= *(s16 *)((s8*)(arg0) + 0xB4)) {
            *(s16 *)((s8*)(arg0) + 0xB6) -= arg1;
            *(s16 *)((s8*)(arg0) + 0xBA) = 0;
            if (func_80083CD8() != 4) {
                func_8008EB4C(0x2E);
            }
            return 0;
        }
        *(s16 *)((s8*)(arg0) + 0xB6) = 0x1000;
        *(s16 *)((s8*)(arg0) + 0xBA) = q;
        switch (arg2) {
        case 1:
            *(s32 *)((s8*)(arg0) + 0x1654) += *(s16 *)((s8*)(arg0) + 0xBA);
            break;
        case 2:
            *(s32 *)((s8*)(arg0) + 0x1658) += *(s16 *)((s8*)(arg0) + 0xBA);
            break;
        }
    }
    return 1;
}

void func_80073B7C(void *, s32, s32, s32 *);
void func_8007C100(void *);
void func_8007C880(s32, s32 *, s32, s32);
void func_8007CD44(s32, s32 *, s32 *, s32);
void func_8007D190(s32 *, s32);
s32 func_8007D25C(s32);
void func_8007D65C(s32, s32, s32);

/* Spawn an effect at bone-vertex positions: types 0x10-0x1F take two points, the rest use the A/B midpoint. */
void func_80073F34(void *arg0, void *arg1) {
    s32 sp10[3];
    s32 sp20[3];
    s32 temp_s3;
    u8 temp_a2;

    temp_s3 = func_8007CD14(((u32) *(u32 *)((s8*)(arg0) + 0xD0) >> 0x1B) & 1, *(u8 *)((s8*)(arg1) + 2), *(s16 *)((s8*)(arg1) + 4), 1);
    func_80073B7C(arg0, *(u8 *)((s8*)(arg1) + 2), *(s16 *)((s8*)(arg1) + 4), sp10);
    if (func_8007D25C(*(u8 *)((s8*)(arg1) + 1)) != 0) {
        func_80073B7C(arg0, *(u8 *)((s8*)(arg1) + 3), *(s16 *)((s8*)(arg1) + 6), sp20);
        temp_a2 = *(u8 *)((s8*)(arg1) + 1);
        if (temp_a2 == 0x10) {
            func_8007CD44(((u32) *(u32 *)((s8*)(arg0) + 0xD0) >> 0x1B) & 1, sp10, sp20, temp_s3);
            return;
        }
        func_8007D65C((s32)sp10, (s32)sp20, temp_a2);
        return;
    }
    if ((*(u8 *)((s8*)(arg1) + 2) != *(u8 *)((s8*)(arg1) + 3)) || (*(s16 *)((s8*)(arg1) + 4) != *(s16 *)((s8*)(arg1) + 6))) {
        func_80073B7C(arg0, *(u8 *)((s8*)(arg1) + 3), *(s16 *)((s8*)(arg1) + 6), sp20);
        sp10[0] = (sp10[0] + sp20[0]) / 2;
        sp10[1] = (sp10[1] + sp20[1]) / 2;
        sp10[2] = (sp10[2] + sp20[2]) / 2;
    }
    func_8007C100((s8 *)arg0 + 0x15D4);
    temp_a2 = *(u8 *)((s8*)(arg1) + 1);
    if (temp_a2 >= 0x20U) {
        func_8007C880(((u32) *(u32 *)((s8*)(arg0) + 0xD0) >> 0x1B) & 1, sp10, temp_s3, temp_a2 - 0x20);
        return;
    }
    func_8007D190(sp10, temp_a2);
}

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
extern s32 D_80092654;
extern s32 D_80092658;
s32 SquareRoot0(s32);
void Square0(s32 *, s32 *);

/* Ray (arg0,arg1)->(arg2,arg3) vs circle (arg4,arg5) radius arg6; first hit goes to D_80092654/D_80092658. */
s32 func_80075888(s32 arg0, s32 arg1, s32 arg2, s32 arg3, s32 arg4, s32 arg5, s32 arg6) {
    s32 vec[4];
    s32 sq[4];
    s32 cx;
    s32 cy;
    s32 len;
    s32 ux;
    s32 uy;
    s32 proj;
    s32 perp;
    s32 d2;
    s32 hit;

    vec[0] = arg2 - arg0;
    vec[1] = arg3 - arg1;
    cx = arg4 - arg0;
    cy = arg5 - arg1;
    vec[2] = arg6;
    Square0(vec, sq);
    len = SquareRoot0(sq[0] + sq[1]);
    if (len == 0) {
        return 0;
    }
    ux = (vec[0] << 12) / len;
    uy = (vec[1] << 12) / len;
    proj = (cx * ux + cy * uy) / 4096;
    perp = (-(cx * uy) + cy * ux) / 4096;
    d2 = perp * perp - sq[2];
    hit = (u32)d2 >> 31;
    if (proj < 0) {
        hit = 0;
    }
    if (len + arg6 < proj) {
        hit = 0;
    }
    if (hit != 0) {
        if (d2 < 0) {
            d2 = -d2;
        }
        d2 = proj - SquareRoot0(d2);
        D_80092654 = d2 * ux / 4096 + arg0;
        D_80092658 = d2 * uy / 4096 + arg1;
    }
    return hit;
}

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
extern s32 D_80091198[];
extern u8 D_80096FB8[];
extern u8 D_80096FBC[];

void func_800764CC(void *arg0) {
    s32 temp_lo;
    u32 temp_v1;
    u32 f;
    s32 var_a0;
    u8 idx;
    s16 *p;

    *(s32 *)((s8*)(arg0) + 0x40) = 0;
    *(s32 *)((s8*)(arg0) + 0x100) = 0;
    if (func_80083CD8() != 7) {
        f = *(u32 *)((s8*)(arg0) + 0xD0) & 0xFFFF7FFF;
        *(u32 *)((s8*)(arg0) + 0xD0) = f & ~2;
    }
    temp_lo = *(s16 *)((s8*)(arg0) + 0x15F4) * D_8009292C;
    temp_v1 = *(u32 *)((s8*)(arg0) + 0xD0) & 0xFFF7FFFF;
    *(u32 *)((s8*)(arg0) + 0xD0) = temp_v1;
    *(s16 *)((s8*)(arg0) + 0x15F2) = temp_lo >> 8;
    var_a0 = 0xF0;
    if (!(temp_v1 & 0x08000000)) {
        var_a0 = 0x28;
    }
    func_8007E894(var_a0, 0x28);
    idx = *(u8 *)((s8*)(arg0) + 0x9C3);
    if (idx != 0) {
        *(s32 *)(D_80096FB8 + ((*(u32 *)((s8*)(arg0) + 0xD0) >> 0x1B) & 1) * 0x14) = D_80091198[idx];
        p = *(s16 **)((s8*)(arg0) + 0x1600);
        D_80096FBC[((*(u32 *)((s8*)(arg0) + 0xD0) >> 0x1B) & 1) * 0x14] = (*((u8 *)p + *(u8 *)((s8*)(arg0) + 0x9C3) + 9) * *p) / 100;
    }
}

extern char D_8006FD04[];
extern char D_8006FD08[];
extern char D_8006FD10[];
extern s32 D_8009112C;
extern u8 D_80096FC4[];
extern u8 D_80096FC8[];
extern s32 D_80096FB8__a_s32[] asm("D_80096FB8");
void func_8007E894__int(s32, s32) asm("func_8007E894");

/* D_80096FB8__a_s32: second D_80096FB8 read via an alias so gcc does not share one symbol address (extra frame slot) */
void func_8007661C(void *arg0) {
    char buf[16];
    s32 var_a0;
    s32 var_s0;
    s32 var_s2;

    var_s0 = 0xA;
    var_s2 = 0x14;
    if (*(u32 *)((s8*)(arg0) + 0xD0) & 0x08000000) {
        var_s2 = 0xB4;
    }
    if (D_800928C8 != 4) {
        var_s0 = 0xB2;
    }
    func_8007E894__int(var_s2, var_s0);
    Sprintf(buf, D_8006FD04, *(s32 *)(D_80096FB8 + ((*(u32 *)((s8*)(arg0) + 0xD0) >> 0x1B) & 1) * 0x14));
    var_a0 = 0x100;
    if (D_80096FB8__a_s32[((*(u32 *)((s8*)(arg0) + 0xD0) >> 0x1B) & 1) * 5] == D_8009112C) {
        var_a0 = 0xE7;
    }
    func_8007E954(var_a0);
    func_8007EBE0(buf);
    func_8007E954(0x100);
    func_8007E894__int(var_s2 + 0x34, var_s0);
    Sprintf(buf, D_8006FD08, D_80096FBC[((*(u32 *)((s8*)(arg0) + 0xD0) >> 0x1B) & 1) * 0x14]);
    func_8007EBE0(buf);
    var_s0 += 0x14;
    func_8007E894__int(var_s2, var_s0);
    Sprintf(buf, D_8006FD10, *(s32 *)(D_80096FC4 + ((*(u32 *)((s8*)(arg0) + 0xD0) >> 0x1B) & 1) * 0x14) >> 4, *(s32 *)(D_80096FC8 + ((*(u32 *)((s8*)(arg0) + 0xD0) >> 0x1B) & 1) * 0x14) >> 4);
    func_8007EBE0(buf);
}

INCLUDE_ASM("asm/battling/nonmatchings/misc", func_800767C8);
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_80076884);
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_80077038);
s32 rcos(s32);
s32 rsin(s32);

s32 func_80077584(arg0, arg1, arg2, arg3)
    void *arg0;
    s32 arg1;
    s32 arg2;
    s32 arg3;
{
    *(s32 *)((s8*)(arg0) + 0x20) += rcos(arg1) >> arg2;
    *(s32 *)((s8*)(arg0) + 0x28) += rsin(arg1) >> arg2;
    if (*(s32 *)((s8*)(arg0) + 0x14) >= -0x8B) {
        *(s32 *)((s8*)(arg0) + 0x14) -= arg3;
    }
    if (*(s32 *)((s8*)(arg0) + 0x14) < -0x82) {
        *(s32 *)((s8*)(arg0) + 0x14) = -0x82;
    }
}

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
void func_80036258(s32, s32);

s32 func_800776A8(void *arg0, s32 arg1) {
    s32 flags;

    if (func_80083CD8() != 4) {
        flags = *(s32 *)((s8*)(arg0) + 0xD0);
        if (flags & 0x08000000) {
            if ((D_800928C8 == 2 || D_800928C8 == 4) && (D_80099D9C[0] & 1) && !(flags & 0x40)) {
                func_80036258(1, arg1);
            }
        } else {
            if ((D_80099D9B[0] & 1) && !(flags & 0x40)) {
                func_80036258(0, arg1);
            }
        }
    }
}

INCLUDE_ASM("asm/battling/nonmatchings/misc", func_80077770);
extern u8 D_80091178[];
extern u8 D_80091179[];

s32 func_80077A38(void *arg0, s32 arg1) {
    s32 temp_v0;

    temp_v0 = *(u8 *)((s8*)(arg0) + 0x9C3) * 2;
    if (arg1 != 0) {
        *(u8 *)((s8*)(arg0) + 0x9C3) = D_80091178[temp_v0];
    } else {
        *(u8 *)((s8*)(arg0) + 0x9C3) = D_80091179[temp_v0];
    }
    return *(s32 *)((s8*)(arg0) + 0x80) + (*(u8 *)((s8*)(arg0) + 0x9C3) * 4);
}


void func_80077A88(void *arg0) {
    *(s32 *)((s8*)(arg0) + 0xD0) = (s32) (*(s32 *)((s8*)(arg0) + 0xD0) | 0x80000);
}

INCLUDE_ASM("asm/battling/nonmatchings/misc", func_80077A9C);
typedef struct {
    s32 unk[4];
} func_80078154_copy16;

void func_80078154(void *arg0) {
    *(func_80078154_copy16 *)arg0 = *(func_80078154_copy16 *)((s8*)(arg0) + 0x93C);
    *(s8 *)((s8*)(arg0) + 0x4C) = 0;
    *(s8 *)((s8*)(arg0) + 0xC4) = 0;
    *(s8 *)((s8*)(arg0) + 0xC5) = 0;
    *(s32 *)((s8*)(arg0) + 0xD0) = (s32) (*(s32 *)((s8*)(arg0) + 0xD0) | 0x02000000);
}

INCLUDE_ASM("asm/battling/nonmatchings/misc", func_80078194);
s32 func_80088838(void *, void *);

s32 func_80078704(void *arg0, void *arg1, void *arg2, void **arg3) {
    s32 a;
    s32 b;
    s32 c;
    s32 p;
    s32 q;
    s32 d1;
    s32 d2;
    s32 e1;
    s32 e2;

    a = (*(s32 *)((u8 *)arg0 + 8) - *(s32 *)((u8 *)arg1 + 8)) * *(s32 *)((u8 *)arg2 + 0x6C)
      + (*(s32 *)((u8 *)arg1 + 0) - *(s32 *)((u8 *)arg0 + 0)) * *(s32 *)((u8 *)arg2 + 0x74)
      + *(s32 *)((u8 *)arg0 + 0) * *(s32 *)((u8 *)arg1 + 8)
      - *(s32 *)((u8 *)arg1 + 0) * *(s32 *)((u8 *)arg0 + 8);
    b = (*(s32 *)((u8 *)arg0 + 8) - *(s32 *)((u8 *)arg1 + 8)) * *(s32 *)((u8 *)arg2 + 0x92C)
      + (*(s32 *)((u8 *)arg1 + 0) - *(s32 *)((u8 *)arg0 + 0)) * *(s32 *)((u8 *)arg2 + 0x934)
      + *(s32 *)((u8 *)arg0 + 0) * *(s32 *)((u8 *)arg1 + 8)
      - *(s32 *)((u8 *)arg1 + 0) * *(s32 *)((u8 *)arg0 + 8);
    c = (*(s32 *)((u8 *)arg0 + 8) - *(s32 *)((u8 *)arg1 + 8)) * *(s32 *)((u8 *)arg2 + 0x93C)
      + (*(s32 *)((u8 *)arg1 + 0) - *(s32 *)((u8 *)arg0 + 0)) * *(s32 *)((u8 *)arg2 + 0x944)
      + *(s32 *)((u8 *)arg0 + 0) * *(s32 *)((u8 *)arg1 + 8)
      - *(s32 *)((u8 *)arg1 + 0) * *(s32 *)((u8 *)arg0 + 8);
    if (a < 0) {
        a = -1;
    } else if (a > 0) {
        a = 1;
    }
    if (b < 0) {
        b = -1;
    } else if (b > 0) {
        b = 1;
    }
    if (c < 0) {
        c = -1;
    } else if (c > 0) {
        c = 1;
    }
    p = b * a;
    q = c * a;
    if (p < 0) {
        if (q < 0) {
            d1 = func_80088838((u8 *)arg2 + 0x92C, arg0);
            d2 = func_80088838((u8 *)arg2 + 0x93C, arg0);
            if (d2 < d1) {
                *arg3 = (u8 *)arg2 + 0x92C;
            } else {
                *arg3 = (u8 *)arg2 + 0x93C;
            }
            return 0;
        }
        *arg3 = (u8 *)arg2 + 0x92C;
        return 1;
    }
    if (q < 0) {
        *arg3 = (u8 *)arg2 + 0x93C;
        return 2;
    }
    e1 = func_80088838((u8 *)arg2 + 0x92C, arg0);
    e2 = func_80088838((u8 *)arg2 + 0x93C, arg0);
    if (e1 < e2) {
        *arg3 = (u8 *)arg2 + 0x92C;
    } else {
        *arg3 = (u8 *)arg2 + 0x93C;
    }
    return 3;
}

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

void func_800346A4(s32 *);                   /* extern */
void func_8008DCA8(s32);                     /* extern */
extern s8 D_80050622;
extern s8 D_80091144;
extern s8 D_80091145;
extern s32 D_80092638;
extern s32 D_8009263C;
extern s32 D_80092648;
extern s8 D_80092664;
extern s8 D_800928B4;
extern s8 D_800928F0;
extern s8 D_800928F4;
extern s32 D_8009290C;
extern s32 D_8009294C;
extern u8 D_80097010__a[] asm("D_80097010");
extern u8 D_8009872C__a[] asm("D_8009872C");

void func_80078F00(void *);
void func_800831C8(void);

s32 func_80079B44(void) {
    *(s16 *)(D_8009872C__a + 0xCC) = 0x400;
    *(s16 *)(D_80097010__a + 0xCC) = -0x400;
    func_80078F00(D_8009872C__a);
    func_80078F00(D_80097010__a);
    func_8008D580(D_80092644);
    func_8007E24C();
    func_8007BB7C();
    func_800831C8();
    D_800928F4 = 1;
    D_8009290C = -1;
    func_8008DCA8(0);
    D_80092638 = 0;
    D_80092640 = 0;
    D_80092890 = 0;
    D_8009263C = 0x5A;
    D_8009294C = 0;
    D_80092648 = 0;
    func_8007F834();
    D_80050622 = 0;
    func_8008E620();
    func_800720C4();
    D_800928D4 = 0;
    D_800928F0 = 0;
    D_80091144 = 0;
    D_80091145 = 0;
    D_80092664 = 0;
    D_80092950 += 1;
    func_800346A4(&D_8009868C);
    D_80099D9A = 0;
    if (*(u8 *)&D_8005061C != 0) {
        switch ((D_80092950 - 1) % 5) {
        case 3:
            D_800928B4 = 1;
            return;
        case 4:
            D_800928B4 = 2;
            return;
        default:
            D_800928B4 = 0;
            break;
        }
    }
}


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

typedef struct {
    u32 pad0 : 2;
    u32 b2 : 1;
    u32 pad3 : 12;
    u32 b15 : 1;
    u32 pad16 : 3;
    u32 b19 : 1;
    u32 pad20 : 12;
} func_80079D6C_d0;
typedef struct {
    u16 pad0 : 8;
    u16 b8 : 1;
    u16 pad9 : 7;
} func_80079D6C_fa;
typedef struct {
    u16 pad0 : 13;
    u16 b13 : 1;
    s16 b14 : 1;
    u16 pad15 : 1;
} func_80079D6C_f6;

void func_80079D6C(void *arg0) {
    ((func_80079D6C_d0 *)((s8*)(arg0) + 0xD0))->b2 = ((func_80079D6C_fa *)((s8*)(*(void **)((s8*)(arg0) + 0x15CC)) + 0xA))->b8;
    ((func_80079D6C_d0 *)((s8*)(arg0) + 0xD0))->b15 = ((func_80079D6C_f6 *)((s8*)(*(void **)((s8*)(arg0) + 0x15CC)) + 6))->b13;
    ((func_80079D6C_d0 *)((s8*)(arg0) + 0xD0))->b19 = ((func_80079D6C_f6 *)((s8*)(*(void **)((s8*)(arg0) + 0x15CC)) + 6))->b14;
}


void func_80079DE0(void) {
    D_80092640 = 0;
}

INCLUDE_ASM("asm/battling/nonmatchings/misc", func_80079DF0);
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_8007A21C);
extern u16 g_C1ButtonStateReleased;
extern s32 D_8006FD4C;
extern s32 D_80092648;
extern u8 D_8009287C;
extern u8 D_800928C0;
extern s32 D_800928E8;
extern u16 g_C2ButtonState;
extern u16 g_C2ButtonStateReleased;

typedef struct {
    s32 unk[4];
} func_8007A344_copy16;
typedef struct {
    u8 pad0[0x15CC];
    s32 p15CC;
    void *p15D0;
} func_8007A344_s;
void func_80036420();
void func_8007D65C(s32, s32, s32);

void func_8007A344(void *arg0, void *arg1) {
    s32 temp_v0;
    s32 temp_v0_2;
    s32 temp_v0_3;
    s32 temp_v1;
    u16 var_a0;
    u8 temp_a0;
    void *temp_v1_2;
    void *temp_v1_3;

    temp_a0 = D_800928C0 + 1;
    temp_v0 = (D_800928C0 * 0xC) + 0x9CC;
    temp_v1 = arg0 + temp_v0;
    temp_v0_2 = arg1 + temp_v0;
    D_800928C0 = temp_a0;
    ((func_8007A344_s *)arg0)->p15CC = temp_v1;
    ((func_8007A344_s *)arg1)->p15CC = temp_v0_2;
    temp_v0_3 = ((temp_a0 & 0xFF) * 0xC) + 0x9CC;
    ((func_8007A344_s *)arg0)->p15D0 = (void *) (arg0 + temp_v0_3);
    ((func_8007A344_s *)arg1)->p15D0 = (void *) (arg1 + temp_v0_3);
    if (D_800928AC == 0xFF) {
        temp_v1_2 = *(void **)((s8*)(arg0) + 0x15D0);
        *(u16 *)((s8*)(temp_v1_2) + 6) = (u16) (*(u16 *)((s8*)(temp_v1_2) + 6) | 0x1000);
        temp_v1_3 = *(void **)((s8*)(arg1) + 0x15D0);
        *(u16 *)((s8*)(temp_v1_3) + 6) = (u16) (*(u16 *)((s8*)(temp_v1_3) + 6) | 0x1000);
        *(s8 *)((s8*)(*(void **)((s8*)(arg0) + 0x15D0)) + 0xA) = 1;
        *(s8 *)((s8*)(*(void **)((s8*)(arg1) + 0x15D0)) + 0xA) = 1;
        *(s8 *)((s8*)(*(void **)((s8*)(arg0) + 0x15D0)) + 9) = 1;
        *(s8 *)((s8*)(*(void **)((s8*)(arg1) + 0x15D0)) + 9) = 1;
    }
    if (D_800928E8 & 8) {
        func_8007E894(0x10, 0x10);
        func_8007EBE0(&D_8006FD4C);
    }
    func_80073644(arg0);
    func_80073644(arg1);
    func_800764CC(arg0);
    func_800764CC(arg1);
    *(func_8007A344_copy16 *)((s8*)(arg0) + 0x95C) = *(func_8007A344_copy16 *)((s8*)(arg0) + 0x93C);
    *(func_8007A344_copy16 *)((s8*)(arg1) + 0x95C) = *(func_8007A344_copy16 *)((s8*)(arg1) + 0x93C);
    *(func_8007A344_copy16 *)((s8*)(arg0) + 0x6C) = *(func_8007A344_copy16 *)((s8*)(arg0) + 0);
    *(func_8007A344_copy16 *)((s8*)(arg1) + 0x6C) = *(func_8007A344_copy16 *)((s8*)(arg1) + 0);
    D_80092648 = 0;
    if ((u8) D_8009287C >= 2U) {
        D_8009287C = 1;
    }
    func_80036420();
    if (*(u8 *)&D_800928FC == 1) {
        if (g_C2ButtonStateReleased & 0x20) {
            func_80083C0C(8);
        }
        var_a0 = g_C2ButtonState;
    } else {
        if (g_C1ButtonStateReleased & 0x20) {
            func_80083C0C(8);
        }
        var_a0 = g_C1ButtonState;
    }
    func_800832C0(var_a0);
    func_80074AB4(arg0);
    func_80074AB4(arg1);
    func_80078E94(arg0);
    func_80078E94(arg1);
    *(s32 *)((s8*)(arg0) + 0xD0) = (s32) ((*(s32 *)((s8*)(arg0) + 0xD0) & 0x9FFFFFFF) | ((func_800828C4(arg0) & 0x03000000) << 5));
    *(s32 *)((s8*)(arg1) + 0xD0) = (s32) ((*(s32 *)((s8*)(arg1) + 0xD0) & 0x9FFFFFFF) | ((func_800828C4(arg1) & 0x03000000) << 5));
    func_80079D6C(arg0);
    func_80079D6C(arg1);
    func_8007920C(arg0);
    func_8007920C(arg1);
    func_80075B50(arg0);
    func_80075B50(arg1);
    func_8007BACC();
    if (D_80092884 != 0) {
        func_8007D65C((s32)arg0 + 0x94C, (s32)arg1 + 0x94C, 0x13);
    }
    if (D_800928AC == 0xFF) {
        func_8008369C();
    }
    if (D_800928AC == 0) {
        func_80083C0C(8);
    }
    D_800928AC--;
}


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
void func_8007A768(void *);
extern s32 D_80098798;
extern s32 D_8009707C;
extern s32 D_80092A34;
extern s32 D_80092A44;
extern s32 D_80092A54;
extern s32 D_80092A64;
extern s32 D_80099068;
extern s32 D_8009794C;
extern s32 D_80099088;
extern s32 D_8009796C;

void func_8007AC3C(void) {
    struct Vector { s32 x, y, z, pad; };

    D_800928AC = 0x96;
    D_8009292C = 0x100;
    SetGeomScreen(0x200);
    *(struct Vector *)D_8009872C__a = *(struct Vector *)&D_80092A34;
    *(struct Vector *)D_80097010__a = *(struct Vector *)&D_80092A44;
    *(struct Vector *)&D_80099068 = *(struct Vector *)&D_80092A54;
    *(struct Vector *)&D_8009794C = *(struct Vector *)&D_80092A64;
    *(struct Vector *)&D_80099088 = *(struct Vector *)&D_80099068;
    *(struct Vector *)&D_8009796C = *(struct Vector *)&D_8009794C;
    *(struct Vector *)&D_80098798 = *(struct Vector *)D_8009872C__a;
    *(struct Vector *)&D_8009707C = *(struct Vector *)D_80097010__a;
    if (D_80092890 == 0) {
        func_8007A768(D_8009872C__a);
    } else {
        func_8007A768(D_80097010__a);
    }
}

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
extern s32 D_800926A4;
extern s32 D_800926B0;
extern s32 D_800926B4;
extern u8 D_80092B2E[];

void func_8007BACC(void) {
    s32 temp_v1_2;
    s32 var_a2;
    s8 temp_v1;
    u16 temp_v0;
    s32 temp_v0_2;

    for (var_a2 = 0; var_a2 < 0x3C; var_a2++) {
        u8 *e = &D_80092B2E[0] + var_a2 * 0x7C;
        if (*(u8 *)(e - 6) != 0) {
            temp_v1 = *(s8 *)(e - 5);
            if (temp_v1 == *(u8 *)(e - 4)) {
                *(u8 *)(e - 6) = 0;
            } else {
                *(s8 *)(e - 5) = (s8) (temp_v1 + 1);
                if (*(u8 *)(e - 3) == 0) {
                    temp_v0 = *(u16 *)(e + 0x10) + *(u16 *)(e + 0xE);
                    *(u16 *)(e + 0x10) = temp_v0;
                    *(u16 *)(e) = (u16) (*(u16 *)(e) + temp_v0);
                }
            }
        }
    }
    temp_v1_2 = D_800926A4 + 1;
    temp_v0_2 = D_800926B0;
    D_800926B0 = 0;
    D_800926A4 = temp_v1_2;
    D_800926B4 = temp_v0_2;
}


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
extern u8 D_800926B8[];
void func_8007C100(void* arg0) {
    struct Bytes { u8 bytes[4]; };
    *(struct Bytes*)D_800926B8 = *(struct Bytes*)arg0;
}

INCLUDE_ASM("asm/battling/nonmatchings/misc", func_8007C124);
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_8007C280);
/* One-texel POLY_FT4 effect: claims a free slot of the 0x3C-entry pool unless an active effect with the same owner and id exists, and links the owner's previous-frame effect. */
typedef struct {
    s32 tag;
    u8 r0, g0, b0, code;
    s16 x0, y0;
    u8 u0, v0; u16 clut;
    s16 x1, y1;
    u8 u1, v1; u16 tpage;
    s16 x2, y2;
    u8 u2, v2; u16 pad2;
    s16 x3, y3;
    u8 u3, v3; u16 pad3;
} Ft4_7C880;
typedef struct {
    Ft4_7C880 prim[2];
    u8 active, timer, limit, type;
    s16 x, y, z;
    u8 pad5A[0x12];
    void *link;
    u16 frame;
    s16 id;
    s32 val;
    s32 owner;
} Fx_7C880;
extern Fx_7C880 D_80092AD8__fx[] asm("D_80092AD8");
extern u16 D_80092694;
extern u8 D_80092698;
extern u8 D_8009269C;
extern u16 D_800926A0;
extern s16 D_80091228[];

void func_8007C880(s32 arg0, s32 *arg1, s32 arg2, s32 arg3) {
    Fx_7C880 *fx;
    Fx_7C880 *p;
    s32 i;

    p = D_80092AD8__fx;
    for (i = 0; i < 0x3C; i++, p++) {
        if (p->active != 0 && p->owner == arg2 && p->id == D_800928E8) {
            return;
        }
    }
    fx = D_80092AD8__fx;
    for (i = 0; i < 0x3C; i++) {
        if (fx->active == 0) {
            break;
        }
        fx++;
    }
    if (i == 0x3C) {
        return;
    }
    fx->prim[0].u0 = fx->prim[0].u1 = fx->prim[0].u2 = fx->prim[0].u3 = D_80092698 * 4 + 8 + arg0 * 4;
    fx->prim[0].v0 = fx->prim[0].v1 = fx->prim[0].v2 = fx->prim[0].v3 = D_8009269C;
    fx->prim[0].tpage = D_80092694;
    fx->prim[0].clut = D_800926A0;
    fx->prim[0].code &= 0xFE;
    fx->prim[1] = fx->prim[0];
    fx->limit = 7;
    fx->type = 1;
    fx->active = 1;
    fx->timer = 0;
    fx->x = arg1[0];
    fx->y = arg1[1];
    fx->z = arg1[2];
    fx->id = D_800928E8;
    fx->owner = arg2;
    fx->link = 0;
    fx->frame = D_800926A4;
    fx->val = D_80091228[arg3];
    p = D_80092AD8__fx;
    for (i = 0; i < 0x3C; i++, p++) {
        if (p->active != 0 && p->owner == arg2 && p != fx && p->type == 1 && p->frame == (u16)(D_800926A4 - 1)) {
            fx->link = p;
        }
    }
}

INCLUDE_ASM("asm/battling/nonmatchings/misc", func_8007CAA4);

s32 func_8007CD14(s32 arg0, s32 arg1, s32 arg2, s32 arg3) {
    return (arg3 & 0x7F) | ((arg0 << 7) & 0x80) | (arg1 << 0x18) | ((arg2 << 8) & 0xFFFF00);
}

/* Two-point POLY_FT4 effect (type 2): claims a free slot of the 0x3C-entry pool unless an active effect with the same owner and id exists, and links the owner's previous-frame type-2 effect. */
typedef struct {
    Ft4_7C880 prim[2];
    u8 active, timer, limit, type;
    s16 x, y, z;
    u8 pad5A[6];
    void *link;
    s16 x2, y2, z2;
    u16 frame;
    s16 id;
    s16 pad6E;
    s32 owner;
    s32 pad74[2];
} Fx_7CD44;
extern Fx_7CD44 D_80092AD8__fy[] asm("D_80092AD8");

void func_8007CD44(s32 arg0, s32 *arg1, s32 *arg2, s32 arg3) {
    Fx_7CD44 *fx;
    Fx_7CD44 *p;
    s32 i;

    p = D_80092AD8__fy;
    for (i = 0; i < 0x3C; i++, p++) {
        if (p->active != 0 && p->owner == arg3 && p->id == D_800928E8) {
            return;
        }
    }
    fx = D_80092AD8__fy;
    for (i = 0; i < 0x3C; i++) {
        if (fx->active == 0) {
            break;
        }
        fx++;
    }
    if (i == 0x3C) {
        return;
    }
    fx->limit = 7;
    fx->type = 2;
    fx->active = 1;
    fx->timer = 0;
    fx->x = arg1[0];
    fx->y = arg1[1];
    fx->z = arg1[2];
    fx->id = D_800928E8;
    fx->x2 = arg2[0];
    fx->y2 = arg2[1];
    fx->z2 = arg2[2];
    fx->frame = D_800926A4;
    fx->prim[0].u0 = fx->prim[0].u1 = fx->prim[0].u2 = fx->prim[0].u3 = D_80092698 * 4 + 8 + arg0 * 4;
    fx->prim[0].v0 = fx->prim[0].v1 = fx->prim[0].v2 = fx->prim[0].v3 = D_8009269C;
    fx->owner = arg3;
    fx->link = 0;
    fx->prim[0].tpage = D_80092694;
    fx->prim[0].clut = D_800926A0;
    fx->prim[0].code &= 0xFE;
    fx->prim[1] = fx->prim[0];
    p = D_80092AD8__fy;
    for (i = 0; i < 0x3C; i++, p++) {
        if (p->active != 0 && p->owner == arg3 && p != fx && p->type == 2 && p->frame == (u16)(D_800926A4 - 1)) {
            fx->link = p;
        }
    }
}

void func_8007CF78(void *arg0, void *arg1) {
    struct Vector { s32 x, y, z, pad; };
    s32 sp10[8];
    register s32 t4 asm("$12");
    register s32 t5 asm("$13");
    register s32 t6 asm("$14");

    *(struct Vector *)0x1F800000 = *(struct Vector *)&D_80096FA8;
    TransposeMatrix(arg0, &sp10[0]);
    func_8007BBA0(arg0, &sp10[0], arg1);
    func_8007C280(arg0, &sp10[0], arg1);
    func_8007CAA4(arg0, 0, arg1);
    t4 = *(s32 *)((s8 *)arg0 + 0x0);
    t5 = *(s32 *)((s8 *)arg0 + 0x4);
    __asm__ volatile ("ctc2 %0, $0" : : "r"(t4));
    __asm__ volatile ("ctc2 %0, $1" : : "r"(t5));
    t4 = *(s32 *)((s8 *)arg0 + 0x8);
    t5 = *(s32 *)((s8 *)arg0 + 0xC);
    t6 = *(s32 *)((s8 *)arg0 + 0x10);
    __asm__ volatile ("ctc2 %0, $2" : : "r"(t4));
    __asm__ volatile ("ctc2 %0, $3" : : "r"(t5));
    __asm__ volatile ("ctc2 %0, $4" : : "r"(t6));
    t4 = *(s32 *)((s8 *)arg0 + 0x14);
    t5 = *(s32 *)((s8 *)arg0 + 0x18);
    __asm__ volatile ("ctc2 %0, $5" : : "r"(t4));
    t6 = *(s32 *)((s8 *)arg0 + 0x1C);
    __asm__ volatile ("ctc2 %0, $6" : : "r"(t5));
    __asm__ volatile ("ctc2 %0, $7" : : "r"(t6));
    func_8007D918(arg1);
    func_8007E020(arg1);
    func_8007E3CC(arg1);
}

void func_8007E3CC(void*);
void func_8007D068(void* arg0) {
    struct Vector { s32 x,y,z,pad; };
    *(struct Vector*)0x1F800000 = *(struct Vector*)&D_80096FA8;
    func_8007E3CC(arg0);
}

INCLUDE_ASM("asm/battling/nonmatchings/misc", func_8007D0B4);
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_8007D190);
s32 func_8007D25C(s32 arg0) {
    if (arg0 < 0x10) {
        return 0;
    }
    return arg0 < 0x20;
}
void func_8007D274(void *arg0, void *arg1) {
    *(s32 *)((s8 *)arg1 + 0) = *(s32 *)((s8 *)arg0 + 0) + rand() % 64 - 0x20;
    *(s32 *)((s8 *)arg1 + 4) = *(s32 *)((s8 *)arg0 + 4) + rand() % 64 - 0x20;
    *(s32 *)((s8 *)arg1 + 8) = *(s32 *)((s8 *)arg0 + 8) + rand() % 64 - 0x20;
}

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
extern s32 D_800926BC;
extern s32 D_800926C0;
extern s32 D_800926C4;

void func_8007D6B8(void) {
    register s16 *var_a1 asm("$5");
    register u8 *var_v1 asm("$3");
    s32 allocation;
    register s32 tag asm("$8");
    register s32 color asm("$7");
    register s32 var_a0 asm("$4");
    register s32 var_a2 asm("$6");

    D_800926BC = HeapAlloc(0x9F6, 0);
    D_800926C0 = HeapAlloc(0xBF4, 0);
    allocation = (s32)HeapAlloc(0xBF4, 0);
    var_a2 = 0;
    tag = 2;
    color = 0x6880B0F0;
    var_a1 = D_800926BC;
    var_a0 = 0;
    D_800926C4 = allocation;
    var_v1 = (u8*)var_a1 + 4;
    do {
        *(s8 *)((s8*)((var_a0 + D_800926C0)) + 3) = tag;
        *(s32 *)((s8*)((var_a0 + D_800926C0)) + 4) = color;
        *(s8 *)((s8*)((var_a0 + D_800926C4)) + 3) = tag;
        var_a2 += 1;
        *(s32 *)((s8*)((var_a0 + D_800926C4)) + 4) = color;
        *(s16 *)((s8*)(var_v1) + 0) = 0;
        *(s16 *)((s8*)(var_v1) + -2) = 0;
        *var_a1 = 0;
        *(s8 *)((s8*)(var_v1) + 2) = 0;
        *(s16 *)((s8*)(var_v1) + 0) = 0;
        var_v1 += 10;
        *var_a1 = 0;
        var_a1 += 5;
        var_a0 += 0xC;
    } while (var_a2 < 0xFF);
}

INCLUDE_ASM("asm/battling/nonmatchings/misc", func_8007D7A8);
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_8007D918);
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_8007DB28);
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_8007DC74);
typedef struct {
    u16 x;
    u16 y;
    u16 z;
    s16 life;
    u8 vx;
    u8 vz;
    u16 vy;
} func_8007E020_s;

extern void *D_800926C8;
extern s32 D_800926CC[];

/* 0x218 falling particles (12 bytes each): integrate, project 3 at a time with GTE RTPT into 3 consecutive 0x10-byte prims, link each into arg0's OT by depth. */
void func_8007E020(s32 arg0) {
    func_8007E020_s *p;
    u8 *prim;
    u8 *spad;
    s32 old;
    s32 state;
    s32 i;
    s32 cx;
    s32 cy;
    s32 cz;
    s32 *ot1;
    s32 *ot2;
    s32 *ot3;
    s32 tag;

    state = 0;
    i = 0;
    spad = (u8 *)0x1F800000;
    cx = *(s32 *)spad;
    cy = *(s32 *)(spad + 4);
    cz = *(s32 *)(spad + 8);
    p = D_800926C8;
    prim = (u8 *)D_800926CC[D_800928A0];
    do {
        if (p->life != 0) {
            p->x += (s8)p->vx;
            p->z += (s8)p->vz;
            *(s16 *)(spad + 0x10) = p->x - cx;
            *(s16 *)(spad + 0x12) = p->y - cy;
            *(s16 *)(spad + 0x14) = p->z - cz;
            if (state == 0) {
                __asm__ volatile ("lwc2 $0, 0( %0 );lwc2 $1, 4( %0 )" : : "r"(spad + 0x10));
                state = 1;
            } else if (state == 1) {
                __asm__ volatile ("lwc2 $2, 0( %0 );lwc2 $3, 4( %0 )" : : "r"(spad + 0x10));
                state = 2;
            } else {
                __asm__ volatile ("lwc2 $4, 0( %0 );lwc2 $5, 4( %0 )" : : "r"(spad + 0x10));
                __asm__ volatile ("nop;nop;.word 0x4A280030");
                state = 0;
                __asm__ volatile ("swc2 $12, 0( %0 );swc2 $13, 0( %1 );swc2 $14, 0( %2 )" : : "r"(prim + 8), "r"(prim + 0x18), "r"(prim + 0x28) : "memory");
                __asm__ volatile ("swc2 $17, 0( %0 )" : : "r"(spad + 0xB0));
                ot1 = (s32 *)(((*(s32 *)(spad + 0xB0) >> 4) * 4) + arg0);
                old = *ot1;
                tag = (u32)prim & 0xFFFFFF;
                *ot1 = tag;
                old |= 0x03000000;
                *(s32 *)tag = old;
                __asm__ volatile ("swc2 $18, 0( %0 )" : : "r"(spad + 0xB0));
                ot2 = (s32 *)(((*(s32 *)(spad + 0xB0) >> 4) * 4) + arg0);
                old = *ot2;
                tag = (u32)(prim + 0x10) & 0xFFFFFF;
                *ot2 = tag;
                old |= 0x03000000;
                *(s32 *)tag = old;
                __asm__ volatile ("swc2 $19, 0( %0 )" : : "r"(spad + 0xB0));
                tag = (u32)(prim + 0x20) & 0xFFFFFF;
                ot3 = (s32 *)(((*(s32 *)(spad + 0xB0) >> 4) * 4) + arg0);
                old = *ot3;
                prim += 0x30;
                *ot3 = tag;
                old |= 0x03000000;
                *(s32 *)tag = old;
            }
            p->y += p->vy;
            p->vy += 2;
            if ((s16)p->y >= 0x11) {
                p->y = 0x10;
            }
            p->life -= 1;
        }
        i++;
        p++;
    } while (i < 0x218);
}

extern s32 D_800926BC;
extern void *D_800926C8;
extern s8 D_8009791B;
extern s8 D_80099037;

typedef struct {
    s16 unk0;
    s16 unk2;
    s16 unk4;
    s16 unk6;
    s16 unk8;
    s16 unkA;
} func_8007E24C_s;

void func_8007E24C(void) {
    s32 var_a0;
    s32 var_a0_2;
    s32 var_v1_2;
    void *temp_v0;
    func_8007E24C_s *var_v1;

    var_v1 = D_800926C8;
    for (var_a0 = 0; var_a0 < 0x21C; var_a0++) {
        var_v1[var_a0].unk6 = 0;
        var_v1[var_a0].unk4 = 0;
        var_v1[var_a0].unk2 = 0;
        var_v1[var_a0].unk0 = 0;
    }
    var_a0_2 = 0;
    var_v1_2 = 0;
    D_8009791B = 0;
    D_80099037 = 0;
    do {
        *(s8 *)((s8*)((var_v1_2 + D_800926BC)) + 6) = 0;
        var_a0_2 += 1;
        temp_v0 = var_v1_2 + D_800926BC;
        *(s16 *)((s8*)(temp_v0) + 4) = 0;
        *(s16 *)((s8*)(temp_v0) + 2) = 0;
        *(s16 *)((s8*)(temp_v0) + 0) = 0;
        var_v1_2 += 0xA;
    } while (var_a0_2 < 0xFF);
}

extern s32 D_800926B0;
extern s32 D_800926B4;
extern s32 D_80094818;

typedef struct {
    u8 pad0[3];
    s8 unk3;
    u8 pad4[3];
    s8 unk7;
    u8 pad8[0x18];
} func_8007E2D8_s;

void func_8007E2D8(void) {
    s32 i;

    for (i = 0; i < 100; i++) {
        ((func_8007E2D8_s *)&D_80094818)[i].unk3 = 3;
        ((func_8007E2D8_s *)&D_80094818)[i].unk7 = 0x40;
    }
    D_800926B0 = 0;
    D_800926B4 = 0;
}

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
void AddPrim(void *, void *);
extern u8 D_80092708;
extern u8 D_800928A0;
extern s32 D_800928E8;
extern u8 D_800954E0[];

void func_8007E574(void *arg0) {
    if (D_80092708 != 0) {
        if (D_800928E8 & 2) {
            AddPrim(arg0, &D_800954E0[0] + D_800928A0 * 0x1C);
            AddPrim(arg0, &D_800954E0[0] - 8 + D_800928A0 * 0x1C);
        }
        D_80092708 -= 1;
    }
}


s32 func_8007E624(void) {
    return D_800926DC;
}

extern u8 *D_800926D4[];
extern u16 D_800926E0;
extern u16 D_800926E4;
typedef struct {
    s32 tp_tag, tp_code;
    u8 pad[3], len;
    u8 r, g, b, code;
    s16 x0, y0;
    u8 u0, v0;
    u16 clut;
    s16 w, h;
} Hud_8007E634;
extern Hud_8007E634 D_800954D8;
extern Hud_8007E634 D_800954F4;
void OpenTIM(s32);
void ReadTIM(void *);
s32 GetTPage(s32, s32, s32, s32);
void SetDrawTPage(s32 *, s32, s32, s32);
s32 LoadImage(void *, void *);

/* text/HUD init: two 100-quad POLY_FT4 buffers, the font TIM's CLUT and TPAGE, and a DR_TPAGE + SPRT bar from the second TIM (copied for the second buffer) */
void func_8007E634(s32 *arg0) {
    struct TimRect { s16 x, y, w, h; };
    struct { s32 mode; struct TimRect *crect; s16 *caddr; struct TimRect *prect; u16 *paddr; } tim;
    s32 unused[2];
    s32 i;
    s16 *pal;

    D_800926D4[0] = HeapAlloc(0xFA0, 0);
    D_800926D4[1] = HeapAlloc(0xFA0, 0);
    for (i = 0; i < 0x64; i++) {
        D_800926D4[0][i * 0x28 + 3] = 0;
        D_800926D4[1][i * 0x28 + 3] = 0;
    }
    OpenTIM(arg0[0xF]);
    ReadTIM(&tim);
    pal = tim.caddr;
    pal[2] = -0x6F9D;
    pal[0] = 0;
    pal[3] = -1;
    LoadImage(tim.crect, tim.caddr);
    LoadImage(tim.prect, tim.paddr);
    D_800926E4 = GetClut(tim.crect->x, tim.crect->y);
    D_800926E0 = GetTPage(0, 1, tim.prect->x, tim.prect->y);
    D_800926DC = 0;
    OpenTIM(arg0[0x19]);
    ReadTIM(&tim);
    pal = tim.caddr;
    pal[0] = 0;
    LoadImage(tim.crect, tim.caddr);
    LoadImage(tim.prect, tim.paddr);
    D_800954E0[3] = 4;
    D_800954E0[7] = 0x65;
    SetDrawTPage((s32 *)&D_800954D8, 0, 0, GetTPage(0, 1, tim.prect->x, tim.prect->y) & 0xFFFF);
    D_800954D8.clut = GetClut(tim.crect->x, tim.crect->y);
    D_800954D8.x0 = 0x40;
    D_800954D8.y0 = 0xBE;
    D_800954D8.w = 0xC4;
    D_800954D8.h = 0xD;
    D_800954D8.u0 = tim.prect->x * 4;
    D_800954D8.v0 = (u16)tim.prect->y;
    D_800954F4 = D_800954D8;
}


void func_8007E894(s16 arg0, s16 arg1) {
    D_800926E8 = arg0;
    D_800926EC = arg1;
}

INCLUDE_ASM("asm/battling/nonmatchings/misc", func_8007E8AC);

void func_8007E954(s32 arg0) {
    D_800912DC = arg0;
}

extern u8 *D_800926D4[];
extern u16 D_800926E0;
extern u16 D_800926E4;
extern u8 D_800926F0;
extern u8 D_800926F4;
extern u8 D_800926F8;
u8 *func_8007E8AC__s32(s32) asm("func_8007E8AC");

/* queue one glyph quad (POLY_FT4) at the pen and advance it; char 0x28 only advances. s32 with no return value, as retail (callers discard it). */
s32 func_8007E964(s32 arg0) {
    s32 xy;
    s32 x1;
    u8 *prim;
    u8 *glyph;
    u16 tpage;
    u16 clut;
    s32 count;

    if (D_800926DC < 0x65) {
        prim = D_800926D4[D_800928A0];
        prim += D_800926DC * 0x28;
        glyph = func_8007E8AC__s32(arg0);
        if (glyph != NULL) {
            if (arg0 != 0x28) {
                arg0 = *(u8 *)(glyph + 2) | 3;
                xy = D_800926EC << 16;
                *(s32 *)(prim + 8) = D_800926E8 | xy;
                x1 = D_800926E8 + ((arg0 * D_800912DC) >> 8);
                *(s32 *)(prim + 0x10) = x1 | xy;
                *(s32 *)(prim + 0x18) = D_800926E8 | ((D_800926EC + *(u8 *)(glyph + 3)) << 16);
                *(s32 *)(prim + 0x20) = x1 | ((D_800926EC + *(u8 *)(glyph + 3)) << 16);
                *(s16 *)(prim + 0xC) = *(u8 *)(glyph + 0) | (*(u8 *)(glyph + 1) << 8);
                *(s16 *)(prim + 0x14) = (*(u8 *)(glyph + 0) + arg0) | (*(u8 *)(glyph + 1) << 8);
                *(s16 *)(prim + 0x1C) = *(u8 *)(glyph + 0) | ((*(u8 *)(glyph + 1) + (*(u8 *)(glyph + 3) + 1)) << 8);
                *(s16 *)(prim + 0x24) = (*(u8 *)(glyph + 0) + arg0) | ((*(u8 *)(glyph + 1) + (*(u8 *)(glyph + 3) + 1)) << 8);
                tpage = D_800926E0;
                clut = D_800926E4;
                *(s8 *)(prim + 3) = 9;
                *(s32 *)(prim + 4) = D_800926F0 | (D_800926F4 << 8) | (D_800926F8 << 16) | 0x2C000000;
                count = D_800926DC;
                *(u16 *)(prim + 0x16) = tpage;
                *(u16 *)(prim + 0xE) = clut;
                D_800926DC = count + 1;
            }
            D_800926E8 = (u16)D_800926E8 + 2 + ((*(u8 *)(glyph + 2) * D_800912DC) >> 8);
        }
    }
}

u8 *func_8007E8AC(u8);

s32 func_8007EB6C(u8 *arg0) {
    s32 width;
    s32 total;
    u8 c;

    c = *arg0;
    total = 0;
    while (c != 0) {
        arg0++;
        width = func_8007E8AC(c)[2] * D_800912DC;
        c = *arg0;
        total += (width >> 8) + 2;
    }
    return total;
}

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
s32 func_8007EB6C(u8 *);

void func_8007EC54(u8 *arg0) {
    s32 savedX;
    u8 c;

    savedX = D_800926E8;
    D_800926E8 -= func_8007EB6C(arg0) / 2;
    c = *arg0;
    while (c != 0) {
        arg0++;
        func_8007E964(c);
        c = *arg0;
    }
    D_800926E8 = savedX;
    D_800926EC += 0x14;
}

s32 func_8007EB6C(u8 *);

void func_8007ECF0(u8 *arg0) {
    s32 savedX;
    u8 c;

    savedX = D_800926E8;
    D_800926E8 -= func_8007EB6C(arg0);
    c = *arg0;
    while (c != 0) {
        arg0++;
        func_8007E964(c);
        c = *arg0;
    }
    D_800926E8 = savedX;
    D_800926EC += 0x14;
}

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
s32 GetTPage(s32, s32, s32, s32);
extern s32 D_80092888;
extern s32 D_800928EC;
extern s32 D_8009270C;

/* POLY_FT4 at arg1 from D_8009270C rect *D_800928EC[arg0]; arg4 == 0x40 is the unscaled quad */
void func_8007F05C(s32 arg0, void *arg1, s32 arg2, s32 arg3, s32 arg4) {
    s32 d;
    s32 x0;
    s32 xa;
    s32 y0;
    s32 ya;
    s32 x1;
    s32 y2;
    s32 u;
    void *rect;

    if (arg2 != 0) {
        arg3 += 0xD3;
    } else {
        arg3 += 0x33;
    }
    if (D_80092888 - 1 < arg0) {
        arg0 -= D_80092888;
    }
    if (arg0 < 0) {
        arg0 += D_80092888;
    }
    arg0 = **(s32 **)((arg0 * 4) + D_800928EC);
    rect = (void *)(D_8009270C + arg0 * 0x14);
    if (arg4 == 0x40) {
        ((s8 *)arg1)[3] = 9;
        ((s8 *)arg1)[7] = 0x2D;
        ((s32 *)arg1)[2] = arg3 | 0x300000;
        ((s32 *)arg1)[4] = (arg3 + 0x3C) | 0x300000;
        ((s32 *)arg1)[6] = arg3 | 0x700000;
        ((s32 *)arg1)[8] = (arg3 + 0x3C) | 0x700000;
    } else {
        arg4 += 0x40;
        ((s8 *)arg1)[3] = 9;
        ((s32 *)arg1)[1] = arg4 | (arg4 << 8) | (arg4 << 16) | 0x2C000000;
        arg4 -= 0x40;
        arg4 >>= 4;
        x0 = arg3 - (d = arg4 - 4);
        y0 = 0x34 - arg4;
        ((s32 *)arg1)[2] = x0 | (y0 << 16);
        xa = x0 + 0x34;
        x1 = xa + arg4 * 2;
        ya = y0 + 0x38;
        y2 = (ya + arg4 * 2) << 16;
        ((s32 *)arg1)[4] = x1 | (y0 << 16);
        ((s32 *)arg1)[6] = x0 | y2;
        ((s32 *)arg1)[8] = x1 | y2;
    }
    u = *(s16 *)((s8 *)rect + 8) * 2;
    ((s16 *)arg1)[6] = u | (*(u16 *)((s8 *)rect + 0xA) << 8);
    ((s16 *)arg1)[10] = (u + 0x3B) | (*(u16 *)((s8 *)rect + 0xA) << 8);
    ((s16 *)arg1)[14] = u | ((*(u16 *)((s8 *)rect + 0xA) + 0x3F) << 8);
    ((s16 *)arg1)[18] = (u + 0x3B) | ((*(u16 *)((s8 *)rect + 0xA) + 0x3F) << 8);
    ((s16 *)arg1)[7] = GetClut(*(s16 *)((s8 *)rect + 0), *(s16 *)((s8 *)rect + 2));
    ((s16 *)arg1)[11] = GetTPage(1, 0, *(s16 *)((s8 *)rect + 8) & 0xFF80, *(s16 *)((s8 *)rect + 0xA));
    AddPrim((void *)D_80092938, arg1);
}

INCLUDE_ASM("asm/battling/nonmatchings/misc", func_8007F258);

void func_8007F834(void) {
    D_80092740 = 0;
    D_8009273C = 0;
    D_80092744 = 0;
}

extern s32 D_800912F0;
extern s32 D_80092734;
extern s32 D_800970E4[];
extern s32 D_80098800[];
void func_80083C0C(s32);
void func_8007F834(void);

void func_8007F854(void) {
    s32 unused[2]; /* retail frame 0x20, args area untouched */
    s32 a;
    s32 b;

    D_800912F0 = 1;
    func_80083C0C(1);
    D_80092734 = 0;
    func_8007F834();
    a = D_80098800[0];
    b = D_800970E4[0];
    a |= 0xC;
    b |= 0xC;
    D_80098800[0] = a;
    D_800970E4[0] = b;
}

void func_8007F8B4(void) {
    u8 unused[8]; /* retail frame has 8 bytes of locals no code touches */

    func_80083C0C(1);
    D_80092734 = 0;
    func_8007F834();
}


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

void func_8007F948(void *arg0, s32 arg1) {
    if (*(s16 *)((s8*)(arg0) + 0x12) == arg1) {
        func_8007EE08(1);
    } else {
        func_8007EE08(0);
    }
}


#ifndef XENO_PC_PORT
s32 func_8007F97C(void) {
    return *(s32 *)((u8 *) &D_800912F4 + (D_80099D98__s_u8 * 4));
}
#endif

extern char D_8006FF5C[];
extern char D_8006FF60[];
extern char D_8006FF68[];
extern char D_8006FF6C[];
extern char D_8006FF74[];
extern u8 D_80091300[];
extern s32 D_8009130C[];
void func_8007E894__int(s32, s32) asm("func_8007E894");

void func_8007F9A0(void *arg0) {
    char buf[8];
    char *str;

    func_8007E894__int((*(s16 *)((s8*)(arg0) + 0x24) + *(s16 *)((s8*)(arg0) + 0x28)) - 0xA, *(s16 *)((s8*)(arg0) + 0x14));
    func_8007EE08(0);
    func_8007F948(arg0, 0);
    func_8007ECF0((u8 *)func_8007F97C());
    func_8007F948(arg0, 1);
    Sprintf(buf, D_8006FF5C, D_80099DA1__a[0] + 1);
    D_8009292C = D_8009130C[D_80099DA1__a[0]];
    D_80099DA4 = D_8009292C;
    func_8007ECF0((u8 *)buf);
    func_8007F948(arg0, 2);
    Sprintf(buf, D_8006FF60, D_80091300[D_80099D9A__a[0]]);
    func_8007ECF0((u8 *)buf);
    func_8007F948(arg0, 3);
    str = D_8006FF6C;
    if (D_80099D9D__a[0] != 0) {
        str = D_8006FF68;
    }
    func_8007ECF0((u8 *)str);
    func_8007F948(arg0, 4);
    str = D_8006FF74;
    if (D_80099D9E__a[0] != 0) {
        str = D_8006FF68;
    }
    func_8007ECF0((u8 *)str);
    func_8007EE08(0);
}

extern char D_8006FF7C[];
extern u8 D_8009132C[];
void func_8007E894__int(s32, s32) asm("func_8007E894");
void func_8007EE08(s32);
void func_8007F948(void *, s32);
void func_80081100(s32, s32);

void func_8007FB0C(void *arg0) {
    u8 sp10[8];

    func_8007E894__int(*(s16 *)((s8 *)arg0 + 0x24) + *(s16 *)((s8 *)arg0 + 0x28) - 0xA,
                        *(s16 *)((s8 *)arg0 + 0x14));
    func_8007EE08(0);
    func_8007ECF0(&D_8006FF7C[0]);
    func_8007F948(arg0, 1);
    func_8007ECF0(((u8 **)D_8009132C)[D_80099DA2__a[0]]);
    func_80081100(D_80099DA2__a[0] + 0x15, 1);
    func_8007F948(arg0, 2);
    Sprintf(&sp10[0], D_8006FF60, *(D_80091300 + D_80099D9A__a[0]));
    func_8007ECF0(&sp10[0]);
    func_8007EE08(0);
}

s32 ControllerGetType(s32);
extern char D_8006FF80[];
extern char D_8006FF90[];
extern s32 D_800916D8;
extern void *D_800916DC;
extern s16 D_800916EA;
extern s32 D_80091714;
extern void *D_80091718;
extern s16 D_80091726;
extern s32 D_80092710;
extern s32 D_8009272C;
extern s32 D_80092730;
extern s32 D_80092754;

void func_8007FBEC(void) {
    s32 *win;
    s32 flag;
    u8 unused[8]; /* retail frame has 8 bytes of locals no code touches */

    func_8007E894(0xA0, 0x8C);
    win = &D_800916D8;
    flag = !(D_80092710 & 1);
    if ((flag != 0) && (D_800916EA == 0)) {
        D_8009272C = 1;
    } else {
        D_8009272C = 0;
    }
    func_8007E894(0x50, 0x8C);
    if ((ControllerGetType(0) == 4) && (D_80099D9D__a[0] == 0)) {
        if (flag != 0) {
            func_8007F948(win, 1);
        }
        func_8007EC54((u8 *)((D_80099D9B[0] & 1) ? D_8006FF80 : D_8006FF90));
        *(u8 *)((s8 *)D_800916DC + 0x14) &= 0xFB;
    } else {
        *(u8 *)((s8 *)D_800916DC + 0x14) |= 4;
    }
    func_8007EE08(0);
    flag = !(D_80092710 & 2);
    if ((flag != 0) && (D_80092754 == 0)) {
        flag = 0;
    }
    win = &D_80091714;
    if ((flag != 0) && (D_80091726 == 0)) {
        D_80092730 = 1;
    } else {
        D_80092730 = 0;
    }
    func_8007E894(0xF0, 0x8C);
    if ((ControllerGetType(1) == 4) && (D_80099D9E__a[0] == 0)) {
        if (flag != 0) {
            func_8007F948(win, 1);
        }
        func_8007EC54((u8 *)((D_80099D9C[0] & 1) ? D_8006FF80 : D_8006FF90));
        *(u8 *)((s8 *)D_80091718 + 0x14) &= 0xFB;
    } else {
        *(u8 *)((s8 *)D_80091718 + 0x14) |= 4;
    }
    func_8007EE08(0);
    func_8007F258(D_80092938, 1);
}

INCLUDE_ASM("asm/battling/nonmatchings/misc", func_8007FE48);
extern s32 D_8009274C;
extern s32 D_80092750;
s32 func_8007FF70(s32 arg0, s32 arg1, s32 arg2) {
    s32 var_s0;
    s32 step;
    s32 sfx;
    s32 buttons;

    var_s0 = arg0;
    step = 1;
    if (arg2 & 4) {
        step = -1;
    }
    sfx = 0;
    if (arg2 & 2) {
        buttons = D_8009274C;
    } else {
        buttons = D_80092750;
    }
    if (buttons & 0x2000) {
        var_s0 += step;
    }
    if (buttons & 0x8000) {
        var_s0 -= step;
    }
    if (buttons & 0xA000) {
        sfx = 1;
    }
    if (arg2 & 1) {
        if (var_s0 == -1) {
            var_s0 = arg1;
        }
        if (arg1 < var_s0) {
            var_s0 = 0;
        }
    } else {
        if (var_s0 == -1) {
            sfx = 0;
            var_s0 = 0;
        }
        if (arg1 < var_s0) {
            sfx = 0;
            var_s0 = arg1;
        }
    }
    if (sfx != 0) {
        func_8008EB4C(0x20);
    }
    return var_s0;
}


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
extern s32 D_80092700;
extern s32 D_80092704;
extern s32 D_800928EC;
void func_8008509C(s32, s32);

void func_80080570(void) {
    s32 **tbl;

    register s32 var_s0 asm("$16");
    register s32 var_s1 asm("$17");
    register s32 t asm("$2");

    tbl = *(s32 **)(void *) &D_800928EC;
    var_s0 = *tbl[D_80092700];
    var_s1 = *tbl[D_80092704];
    switch (var_s0 - 4) {
    case 0:
    case 3:
    case 7:
    case 26:
    case 27:
        var_s0 = (u32) var_s1 < 1;
        break;
    }
    func_8008509C(0, var_s0);
    switch (var_s1 - 4) {
    case 0:
    case 3:
    case 7:
    case 26:
    case 27:
        t = var_s0 ^ 1;
        var_s1 = t != 0;
        break;
    }
    func_8008509C(1, var_s1);
}

s32 ArchiveDecodeSector(s32);
s32 ArchiveReadFileFromCdSector(s32, void *, s32, s32, u32);
s32 LoadImage(void *, void *);
extern s32 D_80092700;
extern s32 D_80092704;
extern s32 D_8009270C;
extern s32 D_80092710;
extern s32 D_80092714;
extern s32 D_80092720;

void func_80080644(s32 arg0, s32 arg1) {
    u8 *buf;
    u8 *buf2;
    u8 *rect;

    buf = HeapAlloc(0x2000, 0);
    ArchiveReadFileFromCdSector(ArchiveDecodeSector(6) + (arg0 * 2), buf, 0x1000, 0, 0);
    buf2 = buf + 0x1000;
    ArchiveReadFileFromCdSector(ArchiveDecodeSector(6) + (arg1 * 2), buf2, 0x1000, 0, 0);
    D_80092700 = arg0;
    D_80092704 = arg1;
    D_80092714 = arg0;
    D_80092720 = arg1;
    D_80092710 = 3;
    ArchiveCdDataSync(0);
    rect = (u8 *)D_8009270C + (arg0 * 0x14);
    LoadImage(rect, buf);
    LoadImage(rect + 8, buf + 0x100);
    rect = (u8 *)D_8009270C + (arg1 * 0x14);
    LoadImage(rect, buf2);
    LoadImage(rect + 8, buf + 0x1100);
    HeapDelayedFree(buf, 2);
}

extern s8 D_80091369;
extern s8 D_80091391;
extern s16 D_800916E2;
extern s16 D_8009171E;
extern s32 D_80092718;
extern s32 D_8009271C;
extern s32 D_80092724;
extern s32 D_80092728;

void func_80080780(s32 arg0) {
    u8 *rect;
    s32 i;
    s32 off;

    if (D_80092940 == 0) {
        ArchiveCdDataSync(0);
        rect = (u8 *)D_8009270C;
        for (i = 0; i < 0x31; i++) {
            off = i << 12;
            LoadImage(rect, (void *)(D_800928D8 + off));
            LoadImage(rect + 8, (void *)(D_800928D8 + off + 0x100));
            rect += 0x14;
        }
        HeapFree((void *)D_800928D8);
        D_80092940 = 1;
    }
    D_800928C8 = arg0;
    D_800916E2 = (arg0 == 4) ? 3 : 4;
    D_8009171E = 5;
    if (arg0 == 3) {
        D_80091369 = 0x27;
        D_80091391 = 0x28;
    } else {
        D_80091369 = 0x25;
        D_80091391 = 0x26;
    }
    func_80083C0C(1);
    D_80092710 = 0;
    D_80092728 = 0;
    D_80092724 = 0;
    D_8009271C = 0;
    D_80092718 = 0;
    D_80092714 = D_80092700;
    D_80092720 = D_80092704;
    func_80080964(5);
}


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

void func_8007313C(s32, s32);                    /* extern */

void func_80080AE8(void) {
    struct { s16 x,y,w,h; } rect;
    register s32 width asm("$3");

    if (D_80092760 != 0) {
        DrawSync(0);
        width = 0x140;
        rect.x = width;
        rect.y = 0x100;
        rect.w = width;
        rect.h = 0xDA;
        func_8007313C(D_80092760, D_80092760 + 0x21E80);
        LoadImage(&rect.x, D_80092760);
    }
}

extern s32 D_80092760;
extern u8 D_800928A0;
extern u8 D_8009A0D8__a[] asm("D_8009A0D8");

void func_80080B58(void) {
    typedef struct { s16 a; s16 b; s16 c; s16 d; } Rect80B58;
    Rect80B58 sp10;
    s32 idx;

    if (D_80092760 == 0) {
        HeapToggleErrorHandler(1);
        D_80092760 = HeapAlloc(0x22100, 0);
        HeapToggleErrorHandler(0);
    }
    DrawSync(0);
    idx = (D_800928A0 + 1) & 1;
    sp10 = *(Rect80B58 *)(D_8009A0D8__a + idx * 0xF8);
    MoveImage(&sp10, 0x140, 0x100);
    if (D_80092760 != 0) {
        StoreImage(&sp10, D_80092760);
    }
    DrawSync(0);
}


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

void func_800811AC(s32);
void SetDrawMove(void *, void *, s32, s32);
s32 GetTPage(s32, s32, s32, s32);
void SetDrawTPage(s32 *, s32, s32, s32);
extern u8 *D_800926D4[];
extern u8 D_800954C8[];
extern u8 D_80095498[];
extern u8 D_8009A0DA__a[] asm("D_8009A0DA");

/* flush the queued prims into arg0; with D_80092734/D_800912F0 set, also its prim, a DR_TPAGE and a DR_MOVE */
void func_80080D20(s32 arg0) {
    struct { s16 x, y, w, h; } rect;
    s32 i;
    u8 *p;
    s32 off;

    p = D_800926D4[D_800928A0];
    for (i = 0; i < D_800926DC; i++, p += 0x28) {
        AddPrim(arg0, p);
    }
    D_800926DC = 0;
    func_800811AC(arg0);
    if ((D_80092734 != 0 && *(u8 *)&D_80092758 != 0) || D_800912F0 != 0) {
        if (D_80092734 != 0) {
            off = D_800928A0 * 0x10 + 0x1C;
            AddPrim(arg0, D_80092734 + off);
            SetDrawTPage((s32 *)(D_800928A0 * 8 + D_800954C8), 0, 0, GetTPage(0, 2, 0, 0) & 0xFFFF);
            AddPrim(arg0, D_800928A0 * 8 + D_800954C8);
        }
        rect.x = 0x140;
        rect.y = 0x100;
        rect.w = 0x140;
        rect.h = 0xDA;
        SetDrawMove(D_800928A0 * 0x18 + D_80095498, &rect, *(s16 *)(D_8009A0D8__a + D_800928A0 * 0xF8), *(s16 *)(D_8009A0DA__a + D_800928A0 * 0xF8));
        AddPrim(arg0, D_800928A0 * 0x18 + D_80095498);
    }
    D_800912F0 = 0;
}

s32 GetTPage(s32, s32, s32, s32);
void SetDrawTPage(s32 *, s32, s32, s32);
void SetShadeTex(void *, s32);
void SetSprt(void *);
/* two battle text labels: pixel buffer ptr + double-buffered SPRT (+0x2C w, +0x2E x), one double-buffered DR_TPAGE */
typedef struct { s32 tag; s32 rgb; s32 xy; s16 uv; u16 clut; s16 w; s16 h; } BattleLabelSprt;
typedef struct { s32 pixels; BattleLabelSprt sprt[2]; s16 w; s16 x; } BattleLabel;
typedef struct { s32 tag; s32 code; } BattleLabelTPage;
extern BattleLabel D_80095510__lbl[2] asm("D_80095510");
extern BattleLabelTPage D_80095570__tp[2] asm("D_80095570");
extern u16 g_SystemPalette1;
extern u16 g_SystemPalette2;

void func_80080F04(void) {
    D_80095510__lbl[0].pixels = D_80095510__lbl[1].pixels = (s32)HeapAlloc(0x6B4, 0);
    D_80095510__lbl[0].sprt[0].xy = 0xB40000;
    D_80095510__lbl[0].sprt[0].uv = 0x3000;
    SetSprt(&D_80095510__lbl[0].sprt[0]);
    SetShadeTex(&D_80095510__lbl[0].sprt[0], 1);
    D_80095510__lbl[0].sprt[0].h = 0xD;
    D_80095510__lbl[0].sprt[0].clut = g_SystemPalette1;
    D_80095510__lbl[0].sprt[1] = D_80095510__lbl[0].sprt[0];
    D_80095510__lbl[1].sprt[0].xy = 0xC30000;
    D_80095510__lbl[1].sprt[0].uv = 0x3000;
    SetSprt(&D_80095510__lbl[1].sprt[0]);
    SetShadeTex(&D_80095510__lbl[1].sprt[0], 1);
    D_80095510__lbl[1].sprt[0].h = 0xD;
    D_80095510__lbl[1].sprt[0].clut = g_SystemPalette2;
    D_80095510__lbl[1].sprt[1] = D_80095510__lbl[1].sprt[0];
    SetDrawTPage((s32 *)&D_80095570__tp[0], 0, 0, GetTPage(0, 0, 0x140, 0x30) & 0xFFFF);
    D_80095570__tp[1] = D_80095570__tp[0];
}

s32 SystemRenderStringEntry(s32, s32, s32, s32);

void func_80081094(void *arg0, s32 arg1, s32 arg2) {
    s32 temp_v0;

    temp_v0 = SystemRenderStringEntry(GetStringEntry(D_80092880[0], arg1), *(s32 *)((s8*)(arg0) + 0), 0x3F, arg2);
    *(s16 *)((s8*)(arg0) + 0x2C) = temp_v0;
    *(s16 *)((s8*)(arg0) + 0x2E) = (0x140 - temp_v0) / 2;
}

extern s32 D_80095510;
extern s32 D_80095540;

void func_80081100(s32 arg0, s32 arg1) {
    struct { s16 x,y,w,h; } rect;

    if (arg1 == 0) {
        if (arg0 != *(u8*)&D_8009273C) {
            D_8009273C = arg0;
            func_80081094(&D_80095510, arg0, 0);
            goto block_5;
        }
    } else if (arg0 != *(u8*)&D_80092740) {
        D_80092740 = arg0;
        func_80081094(&D_80095540, arg0, 1);
block_5:
        rect.x = 0x140;
        rect.y = 0x30;
        rect.w = 0x42;
        rect.h = 0xD;
        LoadImage(&rect, D_80095510);
    }
}

INCLUDE_ASM("asm/battling/nonmatchings/misc", func_800811AC);
void func_800812BC(void *arg0) {
    typedef struct { /* libgpu TILE layout; the window holds two of them */
        u32 tag;
        u8 r0;
        u8 g0;
        u8 b0;
        u8 code;
        s16 x0;
        s16 y0;
        s16 w;
        s16 h;
    } WinTile;
    WinTile *tile;
    void *entry;
    s32 i;
    s32 maxHalf;
    s16 half;

    maxHalf = 0;
    for (i = 0; i < *(s16 *)((s8 *)arg0 + 8); i++) {
        entry = (void *)(*(s32 *)((s8 *)arg0 + 4) + i * 0x14);
        half = func_8007EB6C(*(u8 **)((s8 *)entry + 4)) / 2;
        *(s16 *)((s8 *)entry + 0x12) = half;
        maxHalf = (maxHalf < half) ? half : maxHalf;
    }
    tile = (WinTile *)((s8 *)arg0 + 0x1C);
    *((u8 *)tile + 3) = 3;
    tile->w = maxHalf * 2 + 0x14;
    tile->x0 = 0x96 - maxHalf;
    *(s16 *)((s8 *)arg0 + 0x12) = 0;
    *(s32 *)((s8 *)arg0 + 0x20) = 0x60102020;
    *(s16 *)((s8 *)arg0 + 0x14) = 0x6D - *(s16 *)((s8 *)arg0 + 8) * 10;
    tile->code |= 2;
    tile->h = *(s16 *)((s8 *)arg0 + 8) * 20 + 0x14;
    tile->y0 = *(u16 *)((s8 *)arg0 + 0x14) - 0xA;
    for (i = 0; i < *(s16 *)((s8 *)arg0 + 8); i++) {
        entry = (void *)(*(s32 *)((s8 *)arg0 + 4) + i * 0x14);
        if (*(u8 *)entry & 2) {
            *(s16 *)((s8 *)entry + 0x12) = maxHalf;
        }
    }
    if (*(u8 *)arg0 != 0) {
        tile->w += *(u8 *)arg0;
        tile->x0 -= *(u8 *)arg0 >> 1;
        *(s16 *)((s8 *)arg0 + 0x16) = 0xA0 - (*(u8 *)arg0 >> 1) - maxHalf;
    }
    *(WinTile *)((s8 *)arg0 + 0x2C) = *(WinTile *)((s8 *)arg0 + 0x1C);
    func_8007EE08(0);
}

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
extern s32 D_800916D8;
void func_8007E894__int(s32, s32) asm("func_8007E894");


void func_8008151C__def(s32 arg0) asm("func_8008151C");

void func_8008151C__def(s32 arg0) {
    s32 (*temp_v0_2)(void *);
    s16 temp_a1;
    s32 temp_a0;
    s32 temp_a0_2;
    register s32 var_s1 asm("$17");
    register s32 var_s2 asm("$18");
    void *temp_v0;
    s32 unused[2]; /* retail frame 0x28, args area untouched */

    if (arg0 != NULL) {
        D_80092734 = arg0;
        if (arg0 != &D_800916D8) {
            if (*(u8 *)((s8*)(arg0) + 0) != 0) {
                func_8007E894__int(*(s16 *)((s8*)(arg0) + 0x16), *(s16 *)((s8*)(arg0) + 0x14));
            } else {
                func_8007E894__int(0xA0, *(s16 *)((s8*)(arg0) + 0x14));
            }
            var_s1 = 0;
            if (*(s16 *)((s8*)(arg0) + 8) > 0) {
                var_s2 = 0;
                do {
                    func_8007F948(arg0, var_s1);
                    if (*(u8 *)((s8*)(arg0) + 0) != 0) {
                        temp_a0 = *(s32 *)((s8*)((var_s2 + *(s32 *)((s8*)(arg0) + 4))) + 4);
                        var_s2 += 0x14;
                        func_8007EBE0(temp_a0);
                    } else {
                        temp_v0 = var_s2 + *(s32 *)((s8*)(arg0) + 4);
                        temp_a0_2 = *(s32 *)((s8*)(temp_v0) + 4);
                        temp_a1 = *(s16 *)((s8*)(temp_v0) + 0x12);
                        var_s2 += 0x14;
                        func_8007ED84(temp_a0_2, temp_a1);
                    }
                    var_s1 += 1;
                } while (var_s1 < *(s16 *)((s8*)(arg0) + 8));
            }
            temp_v0_2 = *(s32 (**)(void *))((s8*)(arg0) + 0xC);
            if (temp_v0_2 != NULL) {
                temp_v0_2(arg0);
            }
        }
    }
}

INCLUDE_ASM("asm/battling/nonmatchings/misc", func_8008162C);
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_80081A44);
void func_80080AE8(void);
void func_80081A44(void);
void func_8008151C(s32);
void func_8008162C(s32, s32);
void func_80036420();
extern s32 D_800916D8;
extern u16 g_C1ButtonStateReleased;

void func_80081D2C(void) {
    s32 temp_s0;
    u8 unused[8]; /* retail frame has 8 bytes of locals no code touches */

    if (*(u8 *)&D_8009275C != 0) {
        func_80080AE8();
        D_8009275C = 0;
    }
    func_80036420();
    temp_s0 = D_80092734;
    if (temp_s0 == (s32) &D_800916D8) {
        func_80081A44();
        return;
    }
    if (temp_s0 != 0) {
        if ((g_C1ButtonStateReleased & 1) && ((u8) D_800926FC < 5U)) {
            D_800926FC += 1;
            func_80080AE8();
        }
        func_8008162C(temp_s0, *(u8 *)&D_800928FC);
    }
    func_8008151C(D_80092734);
}


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
typedef struct { s16 a; s16 b; s16 c; s16 d; } Pair82458;
extern Pair82458 D_80092768__pair asm("D_80092768");

void func_80082458(void *arg0) {
    *(Pair82458 *)arg0 = D_80092768__pair;
}

INCLUDE_ASM("asm/battling/nonmatchings/misc", func_80082488);
extern s32 D_800928DC;

s16 func_80082880(void *arg0) {
    s32 unused[12]; /* retail frame is 0x30, no spills or saves */

    register s32 a asm("$3") = (s32) ((u32) *(u16 *)((u8 *)(arg0) + 0) << 16);
    register s32 b asm("$2") = (s32) ((u32) *(u16 *)((u8 *)(arg0) + 4) << 16);

    a = a >> 24;
    b = (b >> 24) << 7;
    return *(s16 *)((b + a) * 4 + *(s32 *)&D_800928DC);
}

extern s32 D_800928DC;

s32 func_800828C4(void *arg0) {
    register s32 a asm("$3") = (s32) *(s32 *)((s8 *)(arg0) + 0) >> 8;
    s32 b = ((s32) *(s32 *)((s8 *)(arg0) + 8) >> 8) << 7;

    return *(s32 *)((b + a) * 4 + *(s32 *)&D_800928DC);
}

INCLUDE_ASM("asm/battling/nonmatchings/misc", func_800828F8);
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_80082A70);
void OpenTIM(s32);
void ReadTIM(void *);
extern s32 D_80092788[];
extern s32 D_8009278C;
extern u16 D_800927A0;
extern u16 D_800927A4;
extern u16 D_800927A8;

/* uploads the TIM pointed to by arg0->0x6C, then builds two 64-entry POLY_FT4 buffers of mirrored 64x64 quads */
void func_80082C4C(void *arg0) {
    struct TimRect { s16 x, y, w, h; };
    struct { s32 mode; struct TimRect *crect; u16 *caddr; struct TimRect *prect; u16 *paddr; } tim;
    typedef struct { u8 t0, t1, t2, len, r0, g0, b0, code; s16 x0, y0; u8 u0, v0; u16 clut; s16 x1, y1; u8 u1, v1; u16 tpage; s16 x2, y2; u8 u2, v2; u16 pad1; s16 x3, y3; u8 u3, v3; u16 pad2; } Ft4;
    s32 i;
    s32 v;
    u16 clut;
    u16 tpage;
    u16 clut2;
    u16 tpage2;
    u16 *pal;
    Ft4 *p;

    OpenTIM(*(s32 *)((s8 *)arg0 + 0x6C));
    ReadTIM(&tim);
    pal = tim.caddr;
    for (i = 0; i < 0x100; i++) {
        v = *pal | ~0x7FFF;
        *pal = v;
        pal++;
    }
    LoadImage(tim.crect, tim.caddr);
    LoadImage(tim.prect, tim.paddr);
    D_800927A0 = GetClut(tim.crect->x, tim.crect->y);
    D_800927A4 = GetTPage(1, 0, tim.prect->x, tim.prect->y);
    D_800927A8 = (u8)tim.prect->y;
    D_80092788[0] = (s32)HeapAlloc(0xA00, 0);
    D_8009278C = (s32)HeapAlloc(0xA00, 0);
    p = (Ft4 *)D_80092788[0];
    for (i = 0; i < 0x40; i += 2) {
        p->len = 9;
        p->code = 0x2C;
        p[1].len = 9;
        p[1].code = 0x2C;
        clut = D_800927A0;
        tpage = D_800927A4;
        p->u0 = 0x7F;
        p->v0 = (u8)D_800927A8 + 0x3F;
        p->u1 = 0x7F;
        p->v1 = D_800927A8;
        p->u2 = 0x3F;
        p->v2 = (u8)D_800927A8 + 0x3F;
        p->u3 = 0x3F;
        p->clut = clut;
        p->tpage = tpage;
        p->v3 = D_800927A8;
        p++;
        clut2 = D_800927A0;
        tpage2 = D_800927A4;
        p->u0 = 0x3F;
        p->v0 = (u8)D_800927A8 + 0x3F;
        p->u1 = 0x3F;
        p->v1 = D_800927A8;
        p->u2 = 0;
        p->clut = clut2;
        p->tpage = tpage2;
        p->v2 = (u8)D_800927A8 + 0x3F;
        p->u3 = 0;
        p->v3 = D_800927A8;
        p++;
    }
    func_800732AC((void *)D_8009278C, (void *)D_80092788[0], 0xA00);
}

INCLUDE_ASM("asm/battling/nonmatchings/misc", func_80082E60);
s32 rand(void);
s32 rcos(s32);
s32 rsin(s32);
extern s32 D_80092770;
extern s32 D_80092774;
extern s32 D_8009290C;
extern s32 D_8009871C[];
extern s32 D_80098720[];
extern s32 D_80098724;

void func_800831C8(void) {
    s32 r;
    s32 ang;

    r = (rand() & 0x1FFF) + 0x800;
    ang = (rand() % 0x600) + 0x500;
    D_8009871C[0] = ((rcos(ang) * r) >> 12) + 0x4000;
    D_80098724 = ((rsin(ang) * r) >> 12) + 0x4000;
    D_80098720[0] = -((rand() & 0x7FF) + 0x400);
    D_80092770 = 0x100;
    D_80092774 = 0x40;
    (*(s8*)&D_8009287C) = 0x40;
    D_8009290C = 0x400;
}

extern s32 D_800927AC;

void func_800832C0(s32 arg0) {
    if (arg0 & 0x8000) {
        D_800927AC += 0x20;
    }
    if (arg0 & 0x2000) {
        D_800927AC -= 0x20;
    }
}

INCLUDE_ASM("asm/battling/nonmatchings/misc", func_80083310);
extern s32 D_800927AC;
extern s32 D_800927B0;
void func_80083310(s32);

void func_8008369C(void) {
    s32 temp_ret;

    D_800927AC = rand();
    temp_ret = rand();
    D_800927B0 = (temp_ret % 12) + 4;
    if (rand() & 1) {
        D_800927B0 = -D_800927B0;
    }
    func_80083310(0);
}

INCLUDE_ASM("asm/battling/nonmatchings/misc", func_80083738);
void func_80083B54(void* arg0, void* arg1) {
    struct Vector { s32 x, y, z, pad; };
    *(struct Vector*)arg0 = *(struct Vector*)D_8009871C;
    *(struct Vector*)arg1 = *(struct Vector*)D_8009867C;
}

void ClearImage(s16 *, s32, s32, s32); /* extern */

void func_80083BB4(s32 arg0) {
    struct { s16 x,y,w,h; } rect;

    rect.x = 0;
    rect.y = 0;
    if (arg0 != 0) {
        rect.w = 0x280;
    } else {
        rect.w = 0x140;
    }
    rect.h = 0x1E0;
    ClearImage(&rect.x, 0, 0, 0);
    DrawSync(0);
}

extern s32 D_80092794;
extern s32 D_8009294C;
void func_8007A21C(s32);
void func_8007AC3C(void);
void func_800725B0(void *);
extern u8 D_80097010__a[] asm("D_80097010");
extern u8 D_8009872C__a[] asm("D_8009872C");

void func_80083C0C(s32 arg0) {
    D_80092794 = arg0;
    switch (arg0) {
    case 3:
        func_80081E6C();
        return;
    case 4:
        func_8007A21C(D_8009294C);
        return;
    case 8:
        func_8007AC3C();
        return;
    case 6:
        if (*(s16 *)(D_8009872C__a + 0xF2) < *(s16 *)(D_80097010__a + 0xF2)) {
            func_800725B0(D_80097010__a);
        } else {
            func_800725B0(D_8009872C__a);
        }
        return;
    }
}


s32 func_80083CD8(void) {
    return D_80092790;
}

INCLUDE_ASM("asm/battling/nonmatchings/misc", func_80083CE8);
void func_80030A30(s32, void *);                 /* extern */
extern void *D_8009288C;
extern s32 D_8009871C[];
extern s32 D_80098724;

void func_80083DCC(void *arg0, void *arg1, s32 arg2) {
    u8 unused[0x20]; /* retail frame has 0x20 bytes of locals no code touches */
    s32 temp_a0;
    u8 temp_v1;
    void *temp_a0_10;
    void *temp_a0_2;
    void *temp_a0_3;
    void *temp_a0_4;
    void *temp_a0_5;
    void *temp_a0_6;
    void *temp_a0_7;
    void *temp_a0_8;
    void *temp_a0_9;
    void *temp_a1;
    void *temp_v0_2;

    temp_v1 = *(u8 *)((s8*)(arg1) + 0x918);
    temp_a1 = *(void **)((s8*)(((arg2 * 4) + arg0)) + 8);
    temp_a0 = temp_v1 & 0xFF;
    if (temp_a0 != 0) {
        *(u8 *)((s8*)(arg1) + 0x918) = temp_v1 - 0x18;
        if (temp_a0 < *(u8 *)((s8*)(arg1) + 0x918)) {
            *(u8 *)((s8*)(arg1) + 0x918) = 0U;
        }
    }
    if (*(s32 *)((s8*)(arg1) + 0xD4) & 0x20) {
        *(u16 *)((s8*)(*(void **)((s8*)(temp_a1) + 4)) + 0xC) = (u16) ((s32) (*(u8 *)((s8*)(*(void **)((s8*)(arg1) + 0xD8)) + 0x15D4) * temp_a0) / 16);
        *(u16 *)((s8*)(*(void **)((s8*)(temp_a1) + 4)) + 0xE) = (u16) ((s32) (*(u8 *)((s8*)(*(void **)((s8*)(arg1) + 0xD8)) + 0x15D5) * temp_a0) / 16);
        *(u16 *)((s8*)(*(void **)((s8*)(temp_a1) + 4)) + 0x10) = (u16) ((s32) (*(u8 *)((s8*)(*(void **)((s8*)(arg1) + 0xD8)) + 0x15D6) * temp_a0) / 16);
    } else {
        *(u16 *)((s8*)(*(void **)((s8*)(temp_a1) + 4)) + 0xC) = (u16) (temp_a0 * 0x10);
        *(u16 *)((s8*)(*(void **)((s8*)(temp_a1) + 4)) + 0xE) = (u16) (temp_a0 * 8);
        *(u16 *)((s8*)(*(void **)((s8*)(temp_a1) + 4)) + 0x10) = 0U;
    }
    if (*(u8 *)((s8*)(D_8009288C) + 0x10) != 0) {
        temp_a0_2 = *(void **)((s8*)(temp_a1) + 4);
        *(u16 *)((s8*)(temp_a0_2) + 0xC) = (u16) ((s16) *(u16 *)((s8*)(temp_a0_2) + 0xC) / 2);
        temp_a0_3 = *(void **)((s8*)(temp_a1) + 4);
        *(u16 *)((s8*)(temp_a0_3) + 0xE) = (u16) ((s16) *(u16 *)((s8*)(temp_a0_3) + 0xE) / 2);
        temp_a0_4 = *(void **)((s8*)(temp_a1) + 4);
        *(u16 *)((s8*)(temp_a0_4) + 0x10) = (u16) ((s16) *(u16 *)((s8*)(temp_a0_4) + 0x10) / 2);
    }
    *(s32 *)((s8*)(*(void **)((s8*)(temp_a1) + 4)) + 0) = (s32) *(s32 *)((s8*)(arg1) + 0);
    *(s32 *)((s8*)(*(void **)((s8*)(temp_a1) + 4)) + 4) = (s32) *(s32 *)((s8*)(arg1) + 4);
    *(s32 *)((s8*)(*(void **)((s8*)(temp_a1) + 4)) + 8) = (s32) *(s32 *)((s8*)(arg1) + 8);
    temp_a0_5 = *(void **)((s8*)(temp_a1) + 4);
    *(s32 *)((s8*)(temp_a0_5) + 0) = (s32) (*(s32 *)((s8*)(temp_a0_5) + 0) - *(s32 *)((s8*)(*(void **)((s8*)(arg1) + 0xD8)) + 0));
    temp_a0_6 = *(void **)((s8*)(temp_a1) + 4);
    *(s32 *)((s8*)(temp_a0_6) + 4) = (s32) (*(s32 *)((s8*)(temp_a0_6) + 4) - *(s32 *)((s8*)(*(void **)((s8*)(arg1) + 0xD8)) + 4));
    temp_a0_7 = *(void **)((s8*)(temp_a1) + 4);
    *(s32 *)((s8*)(temp_a0_7) + 8) = (s32) (*(s32 *)((s8*)(temp_a0_7) + 8) - *(s32 *)((s8*)(*(void **)((s8*)(arg1) + 0xD8)) + 8));
    func_80030A30(arg2, *(void **)((s8*)(temp_a1) + 4));
    temp_a1 = *(void **)((s8*)(arg0) + 0x10);
    temp_v0_2 = *(void **)((s8*)(temp_a1) + 4);
    *(s16 *)((s8*)(temp_v0_2) + 0x10) = 0;
    *(s16 *)((s8*)(temp_v0_2) + 0xE) = 0;
    *(s16 *)((s8*)(temp_v0_2) + 0xC) = 0;
    *(s32 *)((s8*)(*(void **)((s8*)(temp_a1) + 4)) + 0) = (s32) *(s32 *)((s8*)(arg1) + 0);
    *(s32 *)((s8*)(*(void **)((s8*)(temp_a1) + 4)) + 4) = (s32) *(s32 *)((s8*)(arg1) + 4);
    *(s32 *)((s8*)(*(void **)((s8*)(temp_a1) + 4)) + 8) = (s32) *(s32 *)((s8*)(arg1) + 8);
    temp_a0_8 = *(void **)((s8*)(temp_a1) + 4);
    *(s32 *)((s8*)(temp_a0_8) + 0) = (s32) (*(s32 *)((s8*)(temp_a0_8) + 0) - D_8009871C[0]);
    temp_a0_9 = *(void **)((s8*)(temp_a1) + 4);
    *(s32 *)((s8*)(temp_a0_9) + 4) = (s32) (*(s32 *)((s8*)(temp_a0_9) + 4) - D_80098720[0]);
    temp_a0_10 = *(void **)((s8*)(temp_a1) + 4);
    *(s32 *)((s8*)(temp_a0_10) + 8) = (s32) (*(s32 *)((s8*)(temp_a0_10) + 8) - D_80098724);
    func_80030A30(2, *(void **)((s8*)(temp_a1) + 4));
}

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

void ArchiveSetIndex(s32, s32);
extern s8 D_80097919;
extern s8 D_80099035;
extern s32 D_800927B4[];

void func_8008509C(s32 arg0, s32 arg1) {
    s32 *temp_s0;
    s32 *base;

    if (arg0 != 0) {
        D_80097919 = arg1;
    } else {
        D_80099035 = arg1;
    }
    base = D_800927B4;
    temp_s0 = &base[arg0];
    if (*temp_s0 != 0) {
        HeapFree((void *)*temp_s0);
        *temp_s0 = 0;
    }
    ArchiveSetIndex(0x30, 1);
    *temp_s0 = func_800891C0(arg1 + 2);
    ArchiveSetIndex(0x30, 0);
}

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

s32 func_80085264(void) {
    s32 result = 0;
    if (D_80092794 == 5 || D_80092794 == 7) {
        result = 1;
    } else if (D_80092794 == 1) {
        if (D_800928C8 == 4 && *(u8*)&D_8005061C == 0) {
            result = 1;
        }
    }
    return result;
}

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
typedef struct { s16 x, y; } Pos_864B4;
typedef struct { s16 x, y, w; u16 h; } Frame_864B4;
typedef struct { u8 pad0[4]; Pos_864B4 *clut; u8 pad1[4]; Frame_864B4 *tex; } Sprite_864B4;
typedef struct {
    s32 tag;
    u8 r0, g0, b0, code;
    s16 x0, y0;
    u8 u0, v0; u16 clut;
    s16 x1, y1;
    u8 u1, v1; u16 tpage;
    s16 x2, y2;
    u8 u2, v2; u16 pad2;
    u16 x3; s16 y3;
    u8 u3, v3; u16 pad3;
} Quad_864B4;
/* POLY_FT4 at arg3 from the frame at arg0, copied to arg3 + 0x28. arg4 is the GetTPage colour mode, domain 0..2;
 * outside it var_s3 stays unset (retail multiplies by the caller's leftover $s3). The only caller, func_800868E0, passes 0 at both call sites. */
void func_800864B4(Sprite_864B4 *arg0, s32 arg1, s32 arg2, Quad_864B4 *arg3, s32 arg4) {
    s32 var_s3;
    s32 temp_a1;

    switch (arg4) {
    case 0:
        var_s3 = 4;
        break;
    case 1:
        var_s3 = 2;
        break;
    case 2:
        var_s3 = 1;
        break;
    }
    ((u8 *)arg3)[3] = 9;
    arg3->code = 0x2D;
    arg3->clut = GetClut(arg0->clut->x, arg0->clut->y);
    arg3->tpage = GetTPage(arg4, 0, arg0->tex->x, arg0->tex->y);
    arg3->x0 = arg3->x2 = arg1;
    temp_a1 = arg1 + arg0->tex->w * var_s3;
    if (var_s3 == 2) {
        arg3->x3 = temp_a1 + 1;
    } else {
        arg3->x3 = temp_a1;
    }
    arg3->y0 = arg3->y1 = arg2;
    arg3->x1 = arg3->x3 = arg3->x3;
    arg3->y2 = arg3->y3 = arg0->tex->h + arg2;
    arg3->u0 = arg3->u2 = arg0->tex->x * var_s3;
    arg3->u1 = arg3->u3 = (arg0->tex->x + arg0->tex->w) * var_s3;
    arg3->v0 = arg3->v1 = arg0->tex->y;
    arg3->v2 = arg3->v3 = arg0->tex->y + arg0->tex->h;
    *(Quad_864B4 *)((u8 *)arg3 + 0x28) = *arg3;
}

typedef struct { s16 x, y; } Pos_866D4;
typedef struct { s16 x, y, w; u16 h; } Frame_866D4;
typedef struct { u8 pad0[4]; Pos_866D4 *clut; u8 pad1[4]; Frame_866D4 *tex; } Sprite_866D4;
typedef struct {
    s32 tag;
    u8 r0, g0, b0, code;
    s16 x0, y0;
    u8 u0, v0; u16 clut;
    s16 x1, y1;
    u8 u1, v1; u16 tpage;
    s16 x2, y2;
    u8 u2, v2; u16 pad2;
    s16 x3, y3;
    u8 u3, v3; u16 pad3;
} Quad_866D4;
/* mirrored POLY_FT4 at arg3 from the frame at arg0, copied to arg3 + 0x28. arg4 is the GetTPage colour mode, domain 0..2;
 * outside it var_s2 stays unset (retail multiplies by the caller's leftover $s2). The only caller, func_800868E0, passes 0 at both call sites. */
void func_800866D4(Sprite_866D4 *arg0, s32 arg1, s32 arg2, Quad_866D4 *arg3, s32 arg4) {
    s32 var_s2;

    switch (arg4) {
    case 0:
        var_s2 = 4;
        break;
    case 1:
        var_s2 = 2;
        break;
    case 2:
        var_s2 = 1;
        break;
    }
    ((u8 *)arg3)[3] = 9;
    arg3->code = 0x2D;
    arg3->clut = GetClut(arg0->clut->x, arg0->clut->y);
    arg3->tpage = GetTPage(arg4, 0, arg0->tex->x, arg0->tex->y);
    arg3->x0 = arg3->x2 = arg1 + arg0->tex->w * var_s2;
    arg3->x1 = arg3->x3 = arg1;
    arg3->u0 = arg3->u2 = arg0->tex->x * var_s2 - 1;
    arg3->u1 = arg3->u3 = (arg0->tex->x + arg0->tex->w) * var_s2 - 1;
    arg3->y0 = arg3->y1 = arg2;
    arg3->y2 = arg3->y3 = arg0->tex->h + arg2;
    arg3->v0 = arg3->v1 = arg0->tex->y;
    arg3->v2 = arg3->v3 = (u8)arg0->tex->y + (u8)arg0->tex->h;
    *(Quad_866D4 *)((u8 *)arg3 + 0x28) = *arg3;
}

INCLUDE_ASM("asm/battling/nonmatchings/misc", func_800868E0);
extern s32 D_8009A300;
void func_80086E24(void) {
    AddPrim(D_80092938, (D_800928A0 * 0x310) + (u8 *)&D_8009A300);
}
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_80086E70);
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_80086FF8);
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_80087068);
extern u8 D_8009A2F8[];
void func_80085EC8(void*);
void func_800875EC(void) {
    struct Record { u32 words[0xC4]; };
    struct Record* record = (struct Record*)D_8009A2F8;
    func_80085EC8(record);
    record[1] = record[0];
}

extern u8 *D_800927CC;
extern u8 *D_800927D0;

void func_80087650(void) {
    s32 var_v1;
    u8 *temp_v0;

    var_v1 = 0;
    do {
        *(D_800927CC + var_v1) = 0;
        temp_v0 = D_800927D0 + var_v1;
        var_v1 += 1;
        *temp_v0 = 0xFF;
    } while (var_v1 < 0x80);
}

extern u8 D_80091834[];
extern u8 D_800918B4[];
extern u8 *D_800927CC;
extern u8 *D_800927D0;

void func_80087698(s32 x0, s32 y0, s32 x1, s32 y1) {
    s32 tmp;
    s32 x;
    s32 dx;
    s32 v;
    u8 *p;

    if (y0 != y1) {
        if (y1 < y0) {
            tmp = x1;
            x1 = x0;
            x0 = tmp;
            tmp = y1;
            y1 = y0;
            y0 = tmp;
        }
        x = x0 << 8;
        dx = ((x1 - x0) << 8) / (y1 - y0);
        for (; y0 < y1; y0++, x += dx) {
            if (y0 >= 0) {
                if (y0 >= 0x80) {
                    return;
                }
                v = x >> 8;
                p = D_800927D0 + y0;
                if (v < *p) {
                    s32 t = D_80091834[y0];
                    if (v < t) {
                        v = t;
                    }
                    *p = v;
                }
                p = D_800927CC + y0;
                if (*p < v) {
                    s32 t = D_800918B4[y0] + 1;
                    if (t < v) {
                        v = t;
                    }
                    *p = v;
                }
            }
        }
    }
}

extern s32 D_80059598;
extern s32 D_80091934;
void func_800732AC(void *, void *, s32);
void func_80072D18(s32, s32, s32);

void func_8008779C(s32 arg0, s32 arg1, s32 arg2) {
    D_80059598 = (D_80059598 & 0xFFFFFF) | 0x24000000;
    __asm__ volatile ("lwc2\t$6, 0( %0 )" : : "r"(&D_80059598)); /* gte_ldrgb(&D_80059598) */
    func_800732AC((void *)0x1F800120, &D_80091934, 0x30);
    func_80072D18(arg0, arg1, arg2);
}

extern u8 *D_800927CC;
extern u8 *D_800927D0;
extern u8 *D_80092854[];
extern u8 *D_80092858;
void func_800732AC(void *, void *, s32);

void func_80087830(void) {
    s32 i;
    u8 *p;

    D_800927CC = (u8 *)0x1F800000;
    D_800927D0 = (u8 *)0x1F800080;
    D_80092854[0] = HeapAlloc(0xE100, 0);
    D_80092858 = HeapAlloc(0xE100, 0);
    p = D_80092854[0];
    for (i = 0; i < 0x708; i++) {
        p[3] = 7;
        p[7] = 0x24;
        p += 0x20;
    }
    func_800732AC(D_80092858, D_80092854[0], 0xE100);
}

extern s16 D_80091956[];
extern u16 D_800927D4;
extern u16 D_800927D8;
extern u8 D_800927DC;
extern u8 D_800927E0;
void OpenTIM(s32);
void ReadTIM(void *);

void func_800878DC(s32 *arg0) {
    struct TimRect { s16 x, y, w, h; };
    struct { s32 mode; struct TimRect *crect; u16 *caddr; struct TimRect *prect; u16 *paddr; } tim;
    s32 unused[2]; /* retail frame has 8 bytes of locals no code touches */
    s32 i;
    s32 v;
    u16 *pal;

    for (i = 0; i < 4; i++) {
        OpenTIM(arg0[i + 0x12]);
        ReadTIM(&tim);
        D_80091956[i * 2] = GetClut(tim.crect->x, tim.crect->y);
        *(s16 *)((u8 *)D_80091956 + i * 4 - 2) = GetTPage(1, 1, tim.prect->x, tim.prect->y);
        LoadImage(tim.crect, tim.caddr);
        LoadImage(tim.prect, tim.paddr);
    }
    OpenTIM(arg0[0xE]);
    ReadTIM(&tim);
    D_800927D8 = GetClut(tim.crect->x, tim.crect->y);
    D_800927D4 = GetTPage(0, 2, tim.prect->x, tim.prect->y);
    pal = tim.caddr;
    D_800927DC = tim.prect->x * 4;
    D_800927E0 = (u16)tim.prect->y;
    pal[0] = 0;
    for (i = 1; i < 0x10; i++) {
        v = pal[i] | ~0x7FFF;
        pal[i] = v;
    }
    LoadImage(tim.crect, tim.caddr);
    LoadImage(tim.prect, tim.paddr);
    for (i = 0x1C; i < 0x25; i++) {
        OpenTIM(arg0[i]);
        ReadTIM(&tim);
        LoadImage(tim.crect, tim.caddr);
        LoadImage(tim.prect, tim.paddr);
    }
}

extern u16 D_800927D4;
extern u16 D_800927D8;
extern u8 D_800927DC;
extern u8 D_800927E0;

void func_80087AB0(void *arg0) {
    struct Packet { u32 tag, color; s16 x0,y0; u16 uv0,clut; s16 x1,y1; u16 uv1,tpage; s16 x2,y2; u16 uv2,pad2; s16 x3,y3; u16 uv3,pad3; };
    s32 u, v;
    s32 temp_a1;
    s32 temp_v0;
    s32 temp_v1;
    void *var_a3;
    void *var_t0;

    var_t0 = arg0 + 0x162C;
    *(s32 *)((s8*)(arg0) + 0x1608) = 0x2C101010;
    *(s8 *)((s8*)(arg0) + 0x1607) = 9;
    *(u8 *)((s8*)(arg0) + 0x160B) = (u8) (*(u8 *)((s8*)(arg0) + 0x160B) | 2);
    var_a3 = arg0 + 0x1604;
    ((struct Packet*)((u8*)arg0 + 0x1604))->clut = (u16) D_800927D8;
    ((struct Packet*)((u8*)arg0 + 0x1604))->tpage = (u16) D_800927D4;
    u = D_800927DC;
    v = D_800927E0;
    temp_v1 = v << 8;
    ((struct Packet*)((u8*)arg0 + 0x1604))->uv0 = (s16) (u | temp_v1);
    temp_a1 = u + 0x3F;
    temp_v0 = (v + 0x3F) << 8;
    ((struct Packet*)((u8*)arg0 + 0x1604))->uv1 = (s16) (temp_a1 | temp_v1);
    ((struct Packet*)((u8*)arg0 + 0x1604))->uv2 = (s16) (u | temp_v0);
    ((struct Packet*)((u8*)arg0 + 0x1604))->uv3 = (s16) (temp_a1 | temp_v0);
    *(struct Packet*)((u8*)arg0 + 0x162C) = *(struct Packet*)((u8*)arg0 + 0x1604);
}

INCLUDE_ASM("asm/battling/nonmatchings/misc", func_80087B74);
extern s32 D_800928F8;
extern s32 D_8009A988;
void func_80087E38(void *arg0) {
    s32 count = D_800928F8;
    register u8 *base asm("$4");
    s16 *p;
    if (count < 31) {
        base = (u8 *)&D_8009A988;
        p = (s16 *)(base + count * 0x68);
        p[0] = *(s32 *)arg0;
        p[1] = *(s32 *)((u8 *)arg0 + 4);
        p[2] = *(s32 *)((u8 *)arg0 + 8);
        D_800928F8 = count + 1;
    }
}

INCLUDE_ASM("asm/battling/nonmatchings/misc", func_80087EA0);
INCLUDE_ASM("asm/battling/nonmatchings/misc", func_8008820C);

void func_800882D4(s32 arg0, s32 arg1, s32 arg2, s16 arg3) {
    void *temp_v0;

    temp_v0 = func_8008820C();
    if (temp_v0 != NULL) {
        *(s16 *)((s8*)(temp_v0) + 0x30) = arg3;
    }
}

extern u8 D_80095968[];

void func_80088308(void) {
    s32 i;

    for (i = 0x141C; i >= 0; i -= 0x34) {
        *(s16 *)(D_80095968 + i) = 0;
    }
}

extern s32 D_80096FA8__vec[] asm("D_80096FA8");
extern u8 D_80095938[];
void func_800316C0(u32 *, u32);

/* 100 line effects (0x34 each): tick the u16 at +0x30, GTE-RTPT the two SVECs at +0x20/+0x28 into this frame's LINE_F2, link into arg0. */
void func_8008832C(u32 *arg0) {
    typedef struct { u16 x; u16 y; u16 z; u16 pad; } Vec8_8832C;
    Vec8_8832C v0;
    Vec8_8832C v1;
    s32 i;
    s32 off;
    u8 *p;

    i = 0;
    do {
        off = i * 0x34;
        if (*(s16 *)(D_80095968 + off) != 0) {
            p = D_80095938 + off;
            *(u16 *)(p + 0x30) -= 1;
            v0 = *(Vec8_8832C *)(p + 0x20);
            v1 = *(Vec8_8832C *)(p + 0x28);
            v0.x -= D_80096FA8__vec[0];
            v0.y -= D_80096FA8__vec[1];
            v0.z -= D_80096FA8__vec[2];
            v1.x -= D_80096FA8__vec[0];
            v1.y -= D_80096FA8__vec[1];
            v1.z -= D_80096FA8__vec[2];
            __asm__ volatile ("lwc2 $0, 0( %0 );lwc2 $1, 4( %0 );lwc2 $2, 0( %1 );lwc2 $3, 4( %1 )" : : "r"(&v0), "r"(&v1));
            __asm__ volatile ("nop;nop;.word 0x4A280030");
            __asm__ volatile ("swc2 $12, 0( %0 );swc2 $13, 0( %1 )" : : "r"(p + D_800928A0 * 16 + 8), "r"(p + D_800928A0 * 16 + 0xC) : "memory");
            *(u8 *)(p + D_800928A0 * 16 + 3) = 3;
            *(u8 *)(p + D_800928A0 * 16 + 7) = 0x40;
            func_800316C0(arg0, (u32)(p + D_800928A0 * 16));
        }
        i++;
    } while (i < 100);
}

s32 SquareRoot0(s32);
s32 VectorNormal__2(void *, void *) asm("VectorNormal");
s32 func_8002DC9C__s32(s32, s32, s32) asm("func_8002DC9C");

void func_800884E0(void *arg0, void *arg1) {
    typedef struct { s32 x; s32 y; s32 z; s32 pad; } Vec4_884E0;
    Vec4_884E0 vector;
    s32 temp_lo;
    s32 temp_lo_2;
    s32 temp_v0;
    s32 var_v0;

    vector = *(Vec4_884E0 *)arg0;
    var_v0 = func_8002DC9C__s32(vector.x, vector.y, vector.z);
    if (var_v0 < 0) {
        var_v0 = -var_v0;
    }
    temp_v0 = SquareRoot0(var_v0);
    temp_lo = vector.x / temp_v0;
    temp_lo_2 = vector.y / temp_v0;
    vector.x = temp_lo;
    vector.y = temp_lo_2;
    vector.z = vector.z / temp_v0;
    VectorNormal__2(&vector, arg1);
}

s32 SquareRoot0(s32);
s32 VectorNormalS__2(void *, void *) asm("VectorNormalS");
s32 func_8002DC9C__s32(s32, s32, s32) asm("func_8002DC9C");

void func_8008859C(void *arg0, void *arg1) {
    typedef struct { s32 x; s32 y; s32 z; s32 pad; } Vec4_8859C;
    Vec4_8859C vector;
    s32 temp_lo;
    s32 temp_lo_2;
    s32 temp_v0;
    s32 var_v0;

    vector = *(Vec4_8859C *)arg0;
    var_v0 = func_8002DC9C__s32(vector.x, vector.y, vector.z);
    if (var_v0 < 0) {
        var_v0 = -var_v0;
    }
    temp_v0 = SquareRoot0(var_v0);
    temp_lo = vector.x / temp_v0;
    temp_lo_2 = vector.y / temp_v0;
    vector.x = temp_lo;
    vector.y = temp_lo_2;
    vector.z = vector.z / temp_v0;
    VectorNormalS__2(&vector, arg1);
}

s32 SquareRoot0(s32);                               /* extern */
s32 VectorNormalS(void *, void *);    /* extern */
s32 func_8002DC9C(s16, s16, s16);                   /* extern */

void func_80088658(void *arg0, void* arg1) {
    struct { s32 x,y,z,pad; } vector;
    s16 temp_a0;
    s16 temp_a1;
    s16 temp_a2;
    s32 temp_lo;
    s32 temp_lo_2;
    s32 temp_v0;
    s32 var_v0;

    temp_a0 = *(s16 *)((s8*)(arg0) + 0);
    vector.x = (s32) temp_a0;
    temp_a1 = *(s16 *)((s8*)(arg0) + 2);
    vector.y = (s32) temp_a1;
    temp_a2 = *(s16 *)((s8*)(arg0) + 4);
    vector.z = (s32) temp_a2;
    var_v0 = func_8002DC9C(temp_a0, temp_a1, temp_a2);
    if (var_v0 < 0) {
        var_v0 = -var_v0;
    }
    temp_v0 = SquareRoot0(var_v0);
    temp_lo = vector.x / temp_v0;
    temp_lo_2 = vector.y / temp_v0;
    vector.x = temp_lo;
    vector.y = temp_lo_2;
    vector.z = vector.z / temp_v0;
    VectorNormalS(&vector, arg1);
}

s32 SquareRoot0(s32);

/* GTE SQR (square of V1) then |V| via SquareRoot0(MAC1 + MAC2 + MAC3). */
s32 func_800886FC(void *arg0) {
    s32 mac[3];

    __asm__ volatile ("lwc2 $9, 0(%0)" : : "r"(arg0));
    __asm__ volatile ("lwc2 $10, 4(%0)" : : "r"(arg0));
    __asm__ volatile ("lwc2 $11, 8(%0)" : : "r"(arg0));
    __asm__ volatile ("nop\n\tnop\n\t.word 0x4AA00428"); /* gte_sqr0 */
    __asm__ volatile ("swc2 $25, 0(%0)" : : "r"(&mac[0]));
    __asm__ volatile ("swc2 $26, 4(%0)" : : "r"(&mac[0]));
    __asm__ volatile ("swc2 $27, 8(%0)" : : "r"(&mac[0]));
    return SquareRoot0(mac[0] + mac[1] + mac[2]);
}

s32 SquareRoot0(s32);

/* GTE SQR (square of V1) then |V| via SquareRoot0(MAC1 + MAC3). */
s32 func_80088754(void *arg0) {
    s32 mac[3];

    __asm__ volatile ("lwc2 $9, 0(%0)" : : "r"(arg0));
    __asm__ volatile ("lwc2 $10, 4(%0)" : : "r"(arg0));
    __asm__ volatile ("lwc2 $11, 8(%0)" : : "r"(arg0));
    __asm__ volatile ("nop\n\tnop\n\t.word 0x4AA00428"); /* gte_sqr0 */
    __asm__ volatile ("swc2 $25, 0(%0)" : : "r"(&mac[0]));
    __asm__ volatile ("swc2 $26, 4(%0)" : : "r"(&mac[0]));
    __asm__ volatile ("swc2 $27, 8(%0)" : : "r"(&mac[0]));
    return SquareRoot0(mac[0] + mac[2]);
}

s32 SquareRoot0(s32);

/* |a1 - a0| via GTE SQR: SquareRoot0(diff.x + diff.y + diff.z). */
s32 func_800887A4(void *arg0, void *arg1) {
    volatile s32 diff[3];
    register s32 temp asm("$2");

    temp = *(s32 *)((s8 *)arg1 + 0);
    temp -= *(s32 *)((s8 *)arg0 + 0);
    diff[0] = temp;
    temp = *(s32 *)((s8 *)arg1 + 4);
    temp -= *(s32 *)((s8 *)arg0 + 4);
    diff[1] = temp;
    temp = *(s32 *)((s8 *)arg1 + 8);
    temp -= *(s32 *)((s8 *)arg0 + 8);
    diff[2] = temp;
    __asm__ volatile ("lwc2 $9, 0(%0)" : : "r"(&diff[0]));
    __asm__ volatile ("lwc2 $10, 4(%0)" : : "r"(&diff[0]));
    __asm__ volatile ("lwc2 $11, 8(%0)" : : "r"(&diff[0]));
    __asm__ volatile ("nop\n\tnop\n\t.word 0x4AA00428"); /* gte_sqr0 */
    __asm__ volatile ("swc2 $25, 0(%0)" : : "r"(&diff[0]));
    __asm__ volatile ("swc2 $26, 4(%0)" : : "r"(&diff[0]));
    __asm__ volatile ("swc2 $27, 8(%0)" : : "r"(&diff[0]));
    return SquareRoot0(diff[0] + diff[1] + diff[2]);
}

s32 SquareRoot0(s32);

/* |a1 - a0| via GTE SQR: SquareRoot0(diff.x + diff.z). */
s32 func_80088838(void *arg0, void *arg1) {
    volatile s32 diff[3];
    register s32 temp asm("$2");

    temp = *(s32 *)((s8 *)arg1 + 0);
    temp -= *(s32 *)((s8 *)arg0 + 0);
    diff[0] = temp;
    temp = *(s32 *)((s8 *)arg1 + 8);
    temp -= *(s32 *)((s8 *)arg0 + 8);
    diff[2] = temp;
    __asm__ volatile ("lwc2 $9, 0(%0)" : : "r"(&diff[0]));
    __asm__ volatile ("lwc2 $10, 4(%0)" : : "r"(&diff[0]));
    __asm__ volatile ("lwc2 $11, 8(%0)" : : "r"(&diff[0]));
    __asm__ volatile ("nop\n\tnop\n\t.word 0x4AA00428"); /* gte_sqr0 */
    __asm__ volatile ("swc2 $25, 0(%0)" : : "r"(&diff[0]));
    __asm__ volatile ("swc2 $26, 4(%0)" : : "r"(&diff[0]));
    __asm__ volatile ("swc2 $27, 8(%0)" : : "r"(&diff[0]));
    return SquareRoot0(diff[0] + diff[2]);
}

extern u8 D_8006F978[];

void func_800888B0(arg0)
    s32 arg0;
{
    s32 bit;

    bit = 1;
    bit <<= arg0 & 7;
    D_8006F978[arg0 >> 3] |= bit;
}

s32 func_800888E4(s32 arg0) {
    s32 bit = 1;
    bit <<= arg0 & 7;
    return D_8006F978[arg0 >> 3] & bit;
}

extern u8 D_8006F978[];

void func_80088908(s32 arg0) {
    s32 bit;

    bit = 1;
    bit <<= arg0 & 7;
    D_8006F978[arg0 >> 3] &= ~bit;
}


void func_80088940(void) {
    D_8006F980[0] |= 0x10000;
}

extern s32 D_80092888;
extern u8 D_80091A6C[];
extern s32 D_800928EC;
extern s8 D_800927EC;

void func_8008895C(void) {
    s32 temp_v1;

    if ((D_8006F980[0] & 0x10000) && (*(u8 *)&D_800927EC == 0)) {
        temp_v1 = D_80092888;
        *(u8 *)&D_800927EC = 1;
        D_80092888 = temp_v1 + 1;
        *(s32 *)(temp_v1 * 4 + D_800928EC) = &D_80091A6C;
    }
}

s32 func_800888E4(s32);
void func_8008895C(void);
s32 func_800889C8__s32(void) asm("func_800889C8");

s32 func_800889C8__s32(void) {
    s32 i;

    for (i = 0; i < 0x31; i++) {
        if (i != 0x16 && func_800888E4(i) == 0) {
            return 0;
        }
    }
    D_8006F980[0] |= 0x10000;
    func_8008895C();
    return 0;
}

typedef struct {
    u32 unk0 : 4;
    u32 unk4 : 1;
    u32 unk5 : 1;
    u32 unk6 : 7;
    u32 unk13 : 3;
    u32 unk16 : 16;
} func_80088A40_bits;
extern func_80088A40_bits D_8006F980__bits[] asm("D_8006F980");

void func_80088A40(void) {
    if (*(u8 *)&D_8005061C != 0) {
        D_8006F980__bits[0].unk0 = 1;
        D_8006F980__bits[0].unk4 = D_80099D9B[0];
        D_8006F980__bits[0].unk5 = D_80099D9C[0];
        D_8006F980__bits[0].unk6 = D_80099D9F;
        D_8006F980__bits[0].unk13 = D_80099D98[0];
    }
}

extern s8 D_800927EC;

void func_80088AF8(void) {
    u32 *pbits;
    u32 bits;
    s32 i;
    u8 *p;

    if (*(u8 *)&D_8005061C != 0) {
        pbits = (u32 *)D_8006F980;
        bits = *pbits;
        D_800927EC = 0;
        if ((bits & 0xF) == 1) {
            D_80099D9B[0] = (bits >> 4) & 1;
            D_80099D9C[0] = (bits >> 5) & 1;
            D_80099D9F = (bits >> 6) & 0x7F;
            D_80099D98[0] = (bits >> 13) & 7;
            if (bits & 0x10000) {
                i = 7;
                p = (u8 *)pbits - 1;
                do {
                    *p = 0;
                    i--;
                    p--;
                } while (i >= 0);
            }
        } else {
            D_80099D9B[0] = 0;
            D_80099D9C[0] = 0;
            D_80099D9F = 2;
            D_80099D98[0] = 0;
            func_80088A40();
        }
    }
}


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
void ArchiveSetIndex(s32, s32);
void DrawSyncCallback(void *);
void FontLoadFont(s32, s32, s32, s32, s32, s32, s32, s32, s32, s32, s32);
void InitGeom();
void func_8008A110(s32, s32);
void func_8008A128(s32, s32);
extern u8 func_80088BFC__data[] asm("func_80088BFC");
extern s32 D_80010000;
extern s32 D_80091BB0;
extern u8 *D_80092868;
extern void *D_80092870;
extern s16 D_80092898;
extern s32 D_8009289C;
extern s32 D_800928CC;
extern s16 D_800928D0;
extern s32 D_800928E8;
extern s32 D_80092930;
extern u8 D_8009A0D8__a[] asm("D_8009A0D8");

void func_80088D1C(void) {
    u8 unused[8]; /* retail frame has 8 bytes of locals no code touches */

    DrawSyncCallback(func_80088BFC__data + 4);
    InitGeom();
    HeapChangeCurrentUser(6, &D_80091BB0);
    ArchiveSetIndex(0x30, 0);
    FontLoadFont(4, 2, 0x138, 0xDA, 0x14, 1, 0x3C0, 0x1F0, 0x3C0, 0x1EF, 0);
    func_80088CBC(0);
    func_80088CBC(1);
    switch (D_80010000) {
    case -1:
        D_800928CC = 2;
        break;
    case 0:
        D_800928CC = 1;
        break;
    default:
        D_800928CC = 0;
        break;
    }
    D_80092868 = D_8009A0D8__a;
    D_80092870 = D_8009A0D8__a + 0xF8;
    func_8008A110(-1, -1);
    func_8008A128(-1, -1);
    D_80092898 = 2;
    D_8009289C = 1;
    D_80092930 = 0;
    D_800928E8 = 0;
    D_800928A0 = 0;
    D_80092920 = 0;
    D_80092930 = 0;
    D_800928D0 = 7;
    func_8008E620();
}

#endif
