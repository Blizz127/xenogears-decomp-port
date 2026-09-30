#include "common.h"


#ifndef XENO_PC_PORT
extern u32 func_8007A280(u32 a0, u32 a1, u32 a2, u32 a3);
extern u8 D_800D2E5D[];
extern u8 D_800D2E60[];
extern u8 D_800D2E61[];

void func_8007916C(s32 a0, s32 a1) {
    u32 s1 = a0 & 0xFF;
    u32 s0 = (a1 & 0xFF) << 3;
    u32 r;

    r = func_8007A280(s1, D_800D2E5D[s0], 0, 1);
    func_8007A280(s1, D_800D2E5D[s0], (D_800D2E60[s0] + (r + (D_800D2E61[s0] << 8))) & 0xFFFF, 0);
}

extern void func_8007893C(u8, u8);
extern void func_800785D4(u8, u8);
extern u8* D_800C3EAC;
extern u8 D_800C402F[];

void func_800791FC(u8 a, u8 b) {
    func_8007893C(b, a);
    D_800C402F[D_800C3EAC[0x2DA] * 0x48] = 0xF4;
    func_800785D4(a, b);
}

extern void func_800785D4(u8, u8);
extern u8* D_800C3EAC;
extern u8 D_800C402F[];
extern u16 D_800C3FFE[];
extern u16 D_800D2E62[];

void func_80079270(u8 a, u8 b) {
    D_800C402F[D_800C3EAC[0x2DA] * 0x48] = 0xF6;
    D_800C3FFE[D_800C3EAC[0x2DA] * 0x24] = D_800D2E62[b * 4];
    func_800785D4(a, b);
}

extern void FontPrintf(char* fmt, ...);
extern s32* D_8005917C;
extern char D_8006FB08[];
extern char D_8006FB0C[];
extern char D_8006FB24[];

/* Fatal-error screen: clear the 32 command bytes, then (unless the table at
 * D_8005917C is empty) print the two ids forever, indenting 0..20 times. */
void func_800792F8(u8 a, u8 b) {
    u8 ff;
    s32 k;
    s32 i;
    s32 n;
    s32 j;

    ff = 0xFF;
    for (k = 0x8B8; k >= 0; k -= 0x48) {
        D_800C402F[k] = ff;
    }
    i = 0;
    n = 0;
    if (*D_8005917C != -1) {
        while (1) {
            for (j = 0; j < n; j++) {
                FontPrintf(D_8006FB08);
            }
            FontPrintf(D_8006FB0C);
            i++;
            FontPrintf(D_8006FB24, a, b);
            func_800716D8();
            if (i >= 3) {
                i = 0;
                if (++n >= 21) {
                    n = 0;
                }
            }
        }
    }
}

INCLUDE_ASM("asm/battle/nonmatchings/main12", func_800793F0);
extern u8 D_800D32A1[];
extern u8 D_800D366C;
extern u8 D_800C400B[];

void func_80079674(u8 a) {
    if (D_800D32A1[a * 8] != 0) {
        D_800C400B[D_800C3EAC[0x2DA] * 0x48] = a;
        D_800C402F[D_800C3EAC[0x2DA] * 0x48] = 0x1B;
        D_800C3EAC[0x2DA]++;
    }
    D_800C400B[D_800C3EAC[0x2DA] * 0x48] = a;
    D_800C402F[D_800C3EAC[0x2DA] * 0x48] = 0xFE;
    D_800D366C = 0;
}

extern void func_80085350(void);
extern void func_80085388(void);
extern void func_800716D8(void);
extern s32 func_80080AE4(u8);
extern void func_800B89FC(s32, u8, s32, s32);
extern void func_800793F0(u8);
extern void func_80079674(u8);
extern u8* D_800C3EAC;
extern u8 D_800C400B[];

void func_80079778(u8 a) {
    D_800C3EAC[0x2DA] = 0;
    func_80085350();
    func_80085388();
    D_800C400B[D_800C3EAC[0x2DA] * 0x48] = a;
    func_800716D8();
    func_800B89FC(1, a, 0, func_80080AE4(a));
    func_800793F0(a);
    func_80079674(a);
    while (D_800C3EAC[0x2DB] == 0) {
        func_800716D8();
    }
}

INCLUDE_ASM("asm/battle/nonmatchings/main12", func_80079840);
#endif


#ifndef XENO_PC_PORT
extern u32 D_800D3344;
#endif
#ifndef XENO_PC_PORT
extern u32 D_800D39CC;
#endif
#ifndef XENO_PC_PORT
extern u8 D_800C3B74;
#endif
#ifndef XENO_PC_PORT
extern u8 D_800C3D6C;
#endif
#ifndef XENO_PC_PORT
extern u8* D_800C3610;
#endif


/* func_80079934.s */
void func_80079934(u32* pValue) {
    *pValue += 4;
}
