/* battling main: retail functions in retail order (asm/battling/193A0.s).
 * Functions still listed as INCLUDE_ASM are not byte-matching C yet. The whole
 * file is retail-build only. */
#include "common.h"
#include "system/memory.h"

#ifndef XENO_PC_PORT
void func_8007639C(void *, s32);
void func_8008EF74(s32 arg0, void *arg1);
void func_8008F060(s32 arg0, void *arg1);
#endif

#ifndef XENO_PC_PORT
void func_8008D0A4(void *arg0, void *arg1);

s32 ArchiveDecodeSize();
void ArchiveReadFileToBuffer(s32, s32, s32, s32);
void SetGeomOffset(s32, s32);
void SetGeomScreen(s32);
void func_8002DFF0(s32, u32);
void func_80089210(s32, u32);
void func_80089534(); /* (s16, s16); func_8008976C forwards its s32 arguments unconverted */
extern s16 D_8009285C;
extern s16 D_8009286C;
void *func_80089B44(s32);
void func_80089EB4(void *);
s32 *func_80089F8C(s32 *);
void func_8002CBBC(s32);
extern s16 D_80092800;
extern s16 D_80092804;
extern s16 D_80092808;
extern s16 D_8009280C;
s32 GetTPage(s32, s32, s16, s16);
void func_8002C8CC(void *, s32, s32);
void func_8002CB54(void *, void *, void *);
void func_8002CC54(s32);
void func_8002CC74(s16, s16);
s32 func_800303C8(void *, s32);
void func_800732AC(s32, s32, s32);
extern s32 D_80092810;
void func_80030A30(s32, s32);
void ClearOTagR(s32, s16);
extern s32 D_80050100;
extern u8 D_800928A0;
extern s32 D_800928E4;
extern s32 D_80091C30;
s32 GetRCnt(s32);
extern s32 D_80092820;
void ArchiveCdDataSync(s32);
void func_8008AF6C(s32);
void func_8008C298();
extern s32 D_80092828;
void func_8008BD70(s32, s32, s32, s32);
extern s32 D_80092838;
extern s32 D_8009283C;
extern s32 D_80092840;
extern s32 D_80092844;
extern s32 D_80091F60;
extern s32 D_80091F70;
extern s32 D_80091F80;
extern s32 D_80091F90;
extern s32 D_80091FA0;
void func_800767C8();
void func_80090E10();
s32 func_8008F580(void *);
s32 rand();
extern s32 D_8009284C;
void func_8007639C(void *, s32);
extern u8 D_80092884;
void func_8008F7B8(void *);
s32 func_8008F5B4(void *, s32);
void func_8008FFEC(void *, s32);
s32 func_800891C0(s32 arg0);
void func_800896C4(s32 arg0, u32 arg1);
void func_8008973C(void);
void *func_80089C54(void);
void func_80089CD8(void *arg0);
void func_80089D5C(void *arg0);
void func_80089E2C(void *arg0, s32 arg1);
void func_80089E3C(s32 *arg0);
void func_80089E48(s32 *arg0);
void func_80089E54(void *arg0, s32 arg1);
void func_80089E64(void *arg0, s32 arg1);
void func_80089E74(void);
void func_80089FC4(void);
void func_80089FF8(void *arg0);
void func_8008A0B4(void *arg0, s8 arg1, s8 arg2, s8 arg3);
void func_8008A0F4(void *arg0);
void func_8008A110(s16 arg0, s16 arg1);
void func_8008A128(s16 arg0, s16 arg1);
void func_8008A140(s16 arg0, s16 arg1, s16 arg2, s16 arg3);
void func_8008A168(void);
void func_8008A184(void *arg0, void *arg1);
void func_8008A254(void);
void func_8008A3A0(void);
void func_8008A3A8(void *arg0);
void func_8008A5BC(void *arg0);
void func_8008A618(void);
void func_8008A62C(void);
void func_8008ABAC(void *arg0);
void func_8008AC0C(void *arg0);
void func_8008AC7C(s32 arg0);
void func_8008AC8C(void);
void func_8008B070(s32 arg0);
s16 func_8008B5FC(s32 arg0, s32 arg1, s32 arg2);
void func_8008C0BC(void *arg0, s32 arg1);
void func_8008C120(void *arg0);
void func_8008C2C0(s32 arg0);
void func_8008C2E8(void *arg0);
void func_8008CD54(void);
void func_8008CED4(void);
void func_8008DCA8(s32 arg0);
void func_8008DF30(void);
void func_8008E064(void);
void func_8008E6F8(void *arg0);
void func_8008EF00(s32 arg0, void *arg1);
void func_8008F260(s32 arg0, void *arg1, s8 arg2);
s32 func_8008F4F4(void *arg0, s32 arg1);
s32 func_8008F570(void *arg0);
s32 func_8008F720(void *arg0, s32 arg1);
void func_8008F900(void *arg0);
void func_8008FC7C(void *arg0);
void func_8008FE80(s32 arg0);
s32 func_80090258(void *arg0, s32 arg1);
void func_80090504(void *arg0);
void func_80090C88(void *arg0);

INCLUDE_ASM("asm/battling/nonmatchings/main", BattlingMain);

s32 func_800891C0(s32 arg0) {
    s32 temp_v0;

    temp_v0 = HeapAlloc(ArchiveDecodeSize(), 1);
    ArchiveReadFileToBuffer(arg0, temp_v0, 0, 0);
    return temp_v0;
}

typedef struct { s32 w[8]; } BattlingScaleMatrix;
extern BattlingScaleMatrix D_80091C0C__mtx asm("D_80091C0C");
extern BattlingScaleMatrix D_8009A2D8;
extern BattlingScaleMatrix D_80096FE0;
extern s16 D_80096FE8;

void func_80089210(s32 arg0, u32 arg1) {
    s32 h = arg1;
    D_8009A2D8 = D_80091C0C__mtx;
    D_80096FE0 = D_80091C0C__mtx;
    *(s16 *)&D_80096FE0 = (((arg0 << 12) / 320) * h) / arg0;
    D_80096FE8 = (((h << 12) / 240) * h) / arg0;
}

void *SetDefDispEnv();
extern u8 D_8009A134[];
extern s16 D_8009A13C;
extern s16 D_8009A13E;
extern s16 D_8009A140;
extern s16 D_8009A142;
extern s8 D_8009A144;
extern s16 D_8009A234;
extern s16 D_8009A236;
extern s16 D_8009A238;
extern s16 D_8009A23A;
extern s8 D_8009A23C;
typedef struct { u8 pad[3]; u8 len; u32 rgbc; s16 x0; s16 y0; s16 w; s16 h; } BattlingClearTile;
extern BattlingClearTile D_8009A1C0;
extern BattlingClearTile D_8009A2B8;

void func_80089330(arg0, arg1)
s32 arg0;
s32 arg1;
{
    if (arg1 > 0x100) {
        SetDefDispEnv(D_8009A134, 0, 0, arg0, arg1);
        SetDefDispEnv(D_8009A134 + 0xF8, 0, 0, arg0, arg1);
        D_8009A23C = 1;
        D_8009A144 = 1;
        D_8009A13C = 0;
        D_8009A13E = 0x10;
        D_8009A140 = 0x100;
        D_8009A142 = 0xD4;
        D_8009A234 = 0;
        D_8009A236 = 0x10;
        D_8009A238 = 0x100;
        D_8009A23A = 0xD4;
    } else {
        SetDefDispEnv(D_8009A134, 0, 0x100, arg0, arg1);
        SetDefDispEnv(D_8009A134 + 0xF8, 0, 0, arg0, arg1);
        D_8009A23C = 0;
        D_8009A144 = 0;
        D_8009A13C = 0;
        D_8009A13E = 0xA;
        D_8009A140 = 0x100;
        D_8009A142 = arg1;
        D_8009A234 = 0;
        D_8009A236 = 0xA;
        D_8009A238 = 0x100;
        D_8009A23A = arg1;
    }
    D_8009285C = arg0;
    D_8009286C = arg1;
    func_80089210((s16)arg0, (s16)arg1);
    {
        register s32 t asm("$2");
        t = 3;
        D_8009A1C0.len = t;
        t = 0x60000000;
        D_8009A1C0.rgbc = t;
        t = arg0;
        D_8009A1C0.w = t;
        D_8009A1C0.x0 = 0;
        D_8009A1C0.y0 = 0;
        t = arg1;
        D_8009A1C0.h = t;
        D_8009A2B8 = D_8009A1C0;
    }
}

void *SetDefDrawEnv();
void func_80045534(void *, void *);
void func_800453E8(void *, s32);
void func_8004546C(void *, s32);
/* D_8009A0D8: 2 x 0xF8-byte draw buffers: DRAWENV at +0 (tpage +0x14,
 * dtd +0x16, dfe +0x17, isbg +0x18, dr_env +0x1C), DR_AREA +0xD0,
 * DR_OFFSET +0xDC. */
extern u8 D_8009A0D8[];

void func_80089534(s32 w, s32 h) {
    u16 tpage;

    SetGeomOffset(w / 2, h / 2);
    SetGeomScreen(0x180);
    func_8002DFF0(w, h);
    if (h > 0x100) {
        SetDefDrawEnv(D_8009A0D8, 0, 0, w, h);
        SetDefDrawEnv(D_8009A0D8 + 0xF8, 0, 0, w, h);
        D_8009A0D8[0xF8 + 0x17] = 0;
        D_8009A0D8[0x17] = 0;
    } else {
        SetDefDrawEnv(D_8009A0D8, 0, 0, w, h);
        SetDefDrawEnv(D_8009A0D8 + 0xF8, 0, 0x100, w, h);
    }
    D_8009A0D8[0x16] = D_8009A0D8[0xF8 + 0x16] = 1;
    D_8009A0D8[0xF8 + 0x18] = 0;
    D_8009A0D8[0x18] = 0;
    tpage = GetTPage(0, 2, 0x280, 0);
    *(u16 *)(D_8009A0D8 + 0xF8 + 0x14) = tpage;
    *(u16 *)(D_8009A0D8 + 0x14) = tpage;
    func_80045534(D_8009A0D8 + 0x1C, D_8009A0D8);
    func_80045534(D_8009A0D8 + 0xF8 + 0x1C, D_8009A0D8 + 0xF8);
    func_800453E8(D_8009A0D8 + 0xD0, (s32)D_8009A0D8);
    func_800453E8(D_8009A0D8 + 0xF8 + 0xD0, (s32)(D_8009A0D8 + 0xF8));
    func_8004546C(D_8009A0D8 + 0xDC, (s32)(D_8009A0D8 + 8));
    func_8004546C(D_8009A0D8 + 0xF8 + 0xDC, (s32)(D_8009A0D8 + 0xF8 + 8));
}


void func_800896C4(s32 arg0, u32 arg1) {
    SetGeomOffset(arg0 / 2, (s32) (arg1 + (arg1 >> 0x1F)) >> 1);
    SetGeomScreen((s32) (arg0 << 8) / arg0);
    func_8002DFF0(arg0, arg1);
    func_80089210(arg0, arg1);
}


void func_8008973C(void) {
    func_80089534(D_8009285C, D_8009286C);
}

void func_8008976C(s32 arg0, s32 arg1) {
    func_80089330();
    func_80089534(arg0, arg1);
}
INCLUDE_ASM("asm/battling/nonmatchings/main", func_800897AC);
void ApplyMatrix(void *, void *, s32 *);
void MulMatrix2(void *, void *);
void OuterProduct12(s32 *, s32 *, s32 *);
long VectorNormal();
extern s32 D_80096F98[];
extern s32 D_80097000[];
extern s32 D_8009A0C8[];
extern s32 D_8009A918[];

void func_800898BC(void *arg0, void *arg1, void *arg2, void *arg3) {
    D_8009A0C8[0] = *(s16 *)((u8 *)arg2 + 0) - *(s16 *)((u8 *)arg1 + 0);
    D_8009A0C8[1] = *(s16 *)((u8 *)arg2 + 2) - *(s16 *)((u8 *)arg1 + 2);
    D_8009A0C8[2] = *(s16 *)((u8 *)arg2 + 4) - *(s16 *)((u8 *)arg1 + 4);
    D_8009A918[0] = *(s16 *)((u8 *)arg3 + 0);
    D_8009A918[1] = *(s16 *)((u8 *)arg3 + 2);
    D_8009A918[2] = *(s16 *)((u8 *)arg3 + 4);
    VectorNormal(D_8009A0C8, D_80096F98);
    OuterProduct12(D_8009A918, D_80096F98, D_8009A0C8);
    VectorNormal(D_8009A0C8, D_80097000);
    OuterProduct12(D_80096F98, D_80097000, D_8009A0C8);
    VectorNormal(D_8009A0C8, D_8009A918);
    *(s16 *)((u8 *)arg0 + 0x0) = D_80097000[0];
    *(s16 *)((u8 *)arg0 + 0x2) = D_80097000[1];
    *(s16 *)((u8 *)arg0 + 0x4) = D_80097000[2];
    *(s16 *)((u8 *)arg0 + 0x6) = D_8009A918[0];
    *(s16 *)((u8 *)arg0 + 0x8) = D_8009A918[1];
    *(s16 *)((u8 *)arg0 + 0xA) = D_8009A918[2];
    *(s16 *)((u8 *)arg0 + 0xC) = D_80096F98[0];
    *(s16 *)((u8 *)arg0 + 0xE) = D_80096F98[1];
    *(s16 *)((u8 *)arg0 + 0x10) = D_80096F98[2];
    ApplyMatrix(arg0, arg1, D_8009A0C8);
    MulMatrix2(&D_80096FE0, arg0);
    *(s32 *)((u8 *)arg0 + 0x14) = -D_8009A0C8[0];
    *(s32 *)((u8 *)arg0 + 0x18) = -D_8009A0C8[1];
    *(s32 *)((u8 *)arg0 + 0x1C) = -D_8009A0C8[2];
}

INCLUDE_ASM("asm/battling/nonmatchings/main", func_80089A98);
typedef struct { s32 w[8]; } BattlingInitMatrix;
extern BattlingInitMatrix D_80091C0C;
void *func_80089B44(s32 arg0) {
    *(s32 *)((u8 *)arg0 + 0x0) = 0;
    *(s32 *)((u8 *)arg0 + 0x8c) = 0;
    *(s32 *)((u8 *)arg0 + 0x94) = 0;
    *(s32 *)((u8 *)arg0 + 0x90) = 0;
    *(s32 *)((u8 *)arg0 + 0x8) = 0;
    *(s32 *)((u8 *)arg0 + 0x4) = 0;
    *(s32 *)((u8 *)arg0 + 0x3c) = 0;
    *(s32 *)((u8 *)arg0 + 0x38) = 0;
    *(s32 *)((u8 *)arg0 + 0x34) = 0;
    *(s16 *)((u8 *)arg0 + 0x48) = 0;
    *(s16 *)((u8 *)arg0 + 0x46) = 0;
    *(s16 *)((u8 *)arg0 + 0x44) = 0;
    *(s16 *)((u8 *)arg0 + 0x30) = 0;
    *(s16 *)((u8 *)arg0 + 0x2e) = 0;
    *(s16 *)((u8 *)arg0 + 0x2c) = 0;
    *(BattlingInitMatrix *)((u8 *)arg0 + 0x6C) = D_80091C0C;
    *(BattlingInitMatrix *)((u8 *)arg0 + 0x4C) = *(BattlingInitMatrix *)((u8 *)arg0 + 0x6C);
    *(BattlingInitMatrix *)((u8 *)arg0 + 0xC) = *(BattlingInitMatrix *)((u8 *)arg0 + 0x4C);
    return (void *)arg0;
}


void *func_80089C54(void) {
    HeapSetCurrentContentType(8);
    return func_80089B44(HeapAlloc(0x9C, 0));
}

void func_80089C88(void *parent, void *child) {
    void *p;

    *(void **)((u8 *)child + 0x8C) = parent;
    if (*(void **)((u8 *)parent + 0x94) == NULL) {
        *(void **)((u8 *)parent + 0x94) = child;
        return;
    }
    p = *(void **)((u8 *)parent + 0x94);
    while (*(void **)((u8 *)p + 0x90) != NULL) {
        p = *(void **)((u8 *)p + 0x90);
    }
    *(void **)((u8 *)p + 0x90) = child;
}


void func_80089CD8(void *arg0) {
    void *temp_v0;
    void *temp_v1;
    void *var_v1;

    if (arg0 != NULL) {
        temp_v1 = *(void **)((s8*)(arg0) + 0x8C);
        if (temp_v1 != NULL) {
            *(void **)((s8*)(arg0) + 0x8C) = NULL;
            temp_v0 = *(void **)((s8*)(temp_v1) + 0x94);
            if (temp_v0 != NULL) {
                if (temp_v0 == arg0) {
                    *(void **)((s8*)(temp_v1) + 0x94) = (void *) *(void **)((s8*)(arg0) + 0x90);
                } else {
                    var_v1 = temp_v0;
                    if (*(s32 *)((s8*)(var_v1) + 0x90) != arg0) {
                        do {
                            var_v1 = *(void **)((s8*)(var_v1) + 0x90);
                        } while (*(s32 *)((s8*)(var_v1) + 0x90) != arg0);
                    }
                    *(void **)((s8*)(var_v1) + 0x90) = (void *) *(void **)((s8*)(arg0) + 0x90);
                }
                *(void **)((s8*)(arg0) + 0x90) = NULL;
            }
        }
    }
}


void func_80089D5C(void *arg0) {
    s32 temp_v1;
    void *temp_a0;

    if (arg0 != NULL) {
        func_80089D5C(*(void **)((s8*)(arg0) + 0x94));
        func_80089D5C(*(void **)((s8*)(arg0) + 0x90));
        temp_v1 = *(s32 *)((s8*)(arg0) + 0);
        switch (temp_v1) {                          /* irregular */
        case 1:
            func_80089FF8(*(void **)((s8*)(arg0) + 4));
            break;
        case 2:
            func_80089EB4(*(void **)((s8*)(arg0) + 4));
            break;
        case 5:
            func_8008C120(*(void **)((s8*)(arg0) + 4));
            break;
        }
        temp_a0 = *(void **)((s8*)(arg0) + 4);
        if (temp_a0 != NULL) {
            HeapFree(temp_a0);
        }
        HeapFree(arg0);
    }
}


void func_80089E2C(void *arg0, s32 arg1) {
    *(s32 *)((s8*)(arg0) + 4) = arg1;
    *(s32 *)((s8*)(arg0) + 0) = 1;
}


void func_80089E3C(s32 *arg0) {
    *arg0 = 3;
}


void func_80089E48(s32 *arg0) {
    *arg0 = 4;
}


void func_80089E54(void *arg0, s32 arg1) {
    *(s32 *)((s8*)(arg0) + 4) = arg1;
    *(s32 *)((s8*)(arg0) + 0) = 2;
}


void func_80089E64(void *arg0, s32 arg1) {
    *(s32 *)((s8*)(arg0) + 4) = arg1;
    *(s32 *)((s8*)(arg0) + 0) = 6;
}


void func_80089E74(void) {
    void *temp_v0;

    HeapSetCurrentContentType(4);
    temp_v0 = HeapAlloc(0x1C, 0);
    *(s16 *)((s8*)(temp_v0) + 0x14) = 0x1000;
    *(s16 *)((s8*)(temp_v0) + 0x12) = 0x1000;
    *(s16 *)((s8*)(temp_v0) + 0x10) = 0x1000;
    *(s32 *)((s8*)(temp_v0) + 4) = 0;
    *(s32 *)((s8*)(temp_v0) + 8) = 0;
}

extern s32 D_80091C2C;

void func_80089EB4(void *arg0) {
    s32 i;
    u8 *e;

    if (*(void **)((u8 *)arg0 + 4) != NULL) {
        HeapFree(*(void **)((u8 *)arg0 + 4));
    }
    if (*(void **)((u8 *)arg0 + 8) != NULL) {
        if (D_80091C2C == 0) {
            i = *(s16 *)((u8 *)arg0 + 2);
            while (--i != -1) {
                e = (u8 *)(i * 0x14 + *(s32 *)((u8 *)arg0 + 8));
                if (*(s32 *)e != 0) {
                    HeapFree(*(void **)(e + 4));
                }
            }
        }
        HeapFree(*(void **)((u8 *)arg0 + 8));
    }
}

s32 *func_80089F8C(s32 *arg0) {
    arg0[1] = 1;
    arg0[0] = 0;
    arg0[2] = 0;
    arg0[3] = 0;
    arg0[4] = 0;
    arg0[5] = 0;
    arg0[7] = 0;
    ((u8 *)arg0)[0x1A] = 0x40;
    ((u8 *)arg0)[0x19] = 0x40;
    ((u8 *)arg0)[0x18] = 0x40;
    return arg0;
}

void func_80089FC4(void) {
    HeapSetCurrentContentType(1);
    func_80089F8C(HeapAlloc(0x20, 0));
}


void func_80089FF8(void *arg0) {
    s32 temp_a0;

    temp_a0 = *(s32 *)((s8*)(arg0) + 8);
    if (temp_a0 != 0) {
        HeapDelayedFree(temp_a0, 2);
    }
    func_8002CBBC(*(s32 *)((s8*)(arg0) + 0x14));
}

void func_8002DDE4(s32, s32, s32, s32, s32, s32, s32);

void func_8008A040(s32 arg0) {
    if (D_80092800 > 0) {
        func_8002DDE4(arg0, 1, D_80092800, D_80092804, 1, D_80092808, D_8009280C);
    } else {
        func_8002DDE4(arg0, 0, 0, 0, 0, 0, 0);
    }
}


void func_8008A0B4(void *arg0, s8 arg1, s8 arg2, s8 arg3) {
    void *temp_v1;

    *(s8 *)((s8*)(*(void **)((s8*)(arg0) + 4)) + 0x18) = arg1;
    *(s8 *)((s8*)(*(void **)((s8*)(arg0) + 4)) + 0x19) = arg2;
    *(s8 *)((s8*)(*(void **)((s8*)(arg0) + 4)) + 0x1A) = arg3;
    temp_v1 = *(void **)((s8*)(arg0) + 4);
    *(s32 *)((s8*)(temp_v1) + 0) = (s32) (*(s32 *)((s8*)(temp_v1) + 0) | 0x10);
}


void func_8008A0F4(void *arg0) {
    s32 *temp_v0;

    temp_v0 = *(s32 **)((s8*)(arg0) + 4);
    *temp_v0 &= ~0x10;
}


void func_8008A110(s16 arg0, s16 arg1) {
    D_80092800 = arg0;
    D_80092804 = arg1;
}


void func_8008A128(s16 arg0, s16 arg1) {
    D_80092808 = arg0;
    D_8009280C = arg1;
}


void func_8008A140(s16 arg0, s16 arg1, s16 arg2, s16 arg3) {
    D_80092800 = arg0;
    D_80092804 = arg1;
    D_80092808 = arg2;
    D_8009280C = arg3;
}


void func_8008A168(void) {
    D_80092808 = -1;
    D_80092800 = -1;
}


void func_8008A184(void *arg0, void *arg1) {
    *(void **)((s8*)(arg0) + 0x14) = arg1;
    *(s32 *)((s8*)(arg0) + 0x10) = func_800303C8(arg1, 1);
    func_8002CB54(*(void **)((s8*)(arg0) + 0x14), arg0 + 8, arg0 + 0xC);
    if (D_80092800 >= 0) {
        func_8002CC54(GetTPage(0, 1, D_80092800, D_80092804) & 0xFFFF);
    }
    if (D_80092808 >= 0) {
        func_8002CC74(D_80092808, D_8009280C);
    }
    func_8002C8CC(*(void **)((s8*)(arg0) + 0x14), *(s32 *)((s8*)(arg0) + 8), 2);
    func_800732AC(*(s32 *)((s8*)(arg0) + 0xC), *(s32 *)((s8*)(arg0) + 8), *(s32 *)((s8*)(*(void **)((s8*)(arg0) + 0x14)) + 0x34));
    *(s32 *)((s8*)(arg0) + 0) = (s32) (*(s32 *)((s8*)(arg0) + 0) | 2);
}


void func_8008A254(void) {
    void *temp_v0;

    HeapSetCurrentContentType(0xB);
    temp_v0 = HeapAlloc(0x14, 0);
    *(s32 *)((s8*)(temp_v0) + 8) = 0x10;
    *(s32 *)((s8*)(temp_v0) + 4) = 0x10;
    *(s32 *)((s8*)(temp_v0) + 0) = 0x10;
    *(s16 *)((s8*)(temp_v0) + 0x10) = 0;
    *(s16 *)((s8*)(temp_v0) + 0xE) = 0;
    *(s16 *)((s8*)(temp_v0) + 0xC) = 0;
}

void func_8008A298(s32 arg0) {
    HeapFree(arg0);
}
void *func_8008A2B8(u16 count) {
    u8 *obj;
    u8 *buf;
    s32 i;

    HeapSetCurrentContentType(0xC);
    obj = HeapAlloc(0x68, 0);
    HeapSetCurrentContentType(0xC);
    buf = HeapAlloc(count * 8, 0);
    *(u8 **)(obj + 4) = buf;
    *(u8 **)(obj + 8) = buf + count * 4;
    obj[0x16] = 1;
    obj[0x17] = 0xE;
    *(s32 *)obj = 0;
    *(s16 *)(obj + 0x14) = count;
    *(u8 **)(obj + 0xC) = *(u8 **)(obj + 4) + (count * 4 - 4);
    *(u8 **)(obj + 0x10) = *(u8 **)(obj + 8) + (count * 4 - 4);
    for (i = 1; i != count;) {
        i *= 2;
        if (i > 0x4000) {
            obj[0x17] = 0xE;
            break;
        }
        obj[0x17]--;
    }
    return obj;
}


void func_8008A3A0(void) {

}


void func_8008A3A8(void *arg0) {
    HeapDelayedFree(*(s32 *)((s8*)(arg0) + 4), 3);
    HeapFree(arg0);
}

/* func_8008A254 allocates and returns this 0x14-byte block; the TU
 * prototypes it as void, so the call goes through a returning alias. */
typedef struct {
    s32 unk0;
    s32 unk4;
    s32 unk8;
    s16 unkC;
    s16 unkE;
    s16 unk10;
    s16 pad12;
} BattlingXform;
BattlingXform *func_8008A254__ret(void) asm("func_8008A254");

void *func_8008A3E0(s32 arg0) {
    u8 *obj;
    u8 *n0;
    u8 *n1;
    u8 *n2;
    u8 *n3;

    HeapSetCurrentContentType(9);
    obj = HeapAlloc(0x28C, 0);
    n0 = obj + 0x14;
    n1 = obj + 0xB0;
    n2 = obj + 0x14C;
    n3 = obj + 0x1E8;
    *(s32 *)(obj + 0) = 0;
    *(u8 **)(obj + 4) = n0;
    *(u8 **)(obj + 8) = n1;
    *(u8 **)(obj + 0xC) = n2;
    *(u8 **)(obj + 0x10) = n3;
    func_80089B44(n0);
    func_80089B44(n1);
    func_80089B44(n2);
    func_80089B44(n3);
    func_80089E64(*(void **)(obj + 8), func_8008A254__ret());
    func_80089E64(*(void **)(obj + 0xC), func_8008A254__ret());
    func_80089E64(*(void **)(obj + 0x10), func_8008A254__ret());
    *(s32 *)(*(u8 **)(obj + 8) + 0x38) = *(s32 *)(*(u8 **)(obj + 0xC) + 0x38) = *(s32 *)(*(u8 **)(obj + 0x10) + 0x38) = -2;
    *(s32 *)(*(u8 **)(obj + 8) + 0x34) = 0;
    *(s32 *)(*(u8 **)(obj + 8) + 0x3C) = 1;
    *(s32 *)(*(u8 **)(obj + 0xC) + 0x34) = -1;
    *(s32 *)(*(u8 **)(obj + 0xC) + 0x3C) = -1;
    *(s32 *)(*(u8 **)(obj + 0x10) + 0x34) = 1;
    *(s32 *)(*(u8 **)(obj + 0x10) + 0x3C) = -1;
    (*(BattlingXform **)(*(u8 **)(obj + 8) + 4))->unkC = (*(BattlingXform **)(*(u8 **)(obj + 8) + 4))->unkE = (*(BattlingXform **)(*(u8 **)(obj + 8) + 4))->unk10 = 0x800;
    (*(BattlingXform **)(*(u8 **)(obj + 0xC) + 4))->unkC = (*(BattlingXform **)(*(u8 **)(obj + 0xC) + 4))->unkE = (*(BattlingXform **)(*(u8 **)(obj + 0xC) + 4))->unk10 = 0;
    **(BattlingXform **)(*(u8 **)(obj + 0x10) + 4) = **(BattlingXform **)(*(u8 **)(obj + 0xC) + 4);
    *(s32 *)(obj + 0x284) = arg0;
    obj[0x28A] = 0;
    obj[0x289] = 0;
    obj[0x288] = 0;
    obj[0x28A] = 0x10;
    obj[0x289] = 0x10;
    obj[0x288] = 0x10;
    func_8008ABAC(obj + 8);
    return obj;
}


void func_8008A5BC(void *arg0) {
    HeapFree(*(void **)((s8*)(arg0) + 0xB4));
    HeapFree(*(void **)((s8*)(arg0) + 0x150));
    HeapFree(*(void **)((s8*)(arg0) + 0x1EC));
    func_8008A3A8(*(s32 *)((s8*)(arg0) + 0x284));
    HeapFree(arg0);
}


void func_8008A618(void) {
    D_80092810 = 1;
}


void func_8008A62C(void) {
    D_80092810 = 0;
}

extern s32 D_80050104;
extern s32 D_80092810;
extern u8 D_800928A0;
extern s32 D_800928E4;
void func_8002C700(s32, s32, s32, s32);

void func_8008A63C(void *arg0) {
    register s32 t0 asm("$8");
    register s32 t1 asm("$9");
    register s32 t2 asm("$10");
    register s32 t4 asm("$12");
    register s32 t5 asm("$13");
    register s32 t6 asm("$14");

    D_80050104 = 0;
    if (D_80092810 != 0) {
        if (*(s32 *)((s8 *)arg0 + 0) & 0x10) {
            t0 = *(u8 *)((s8 *)arg0 + 0x18);
            t1 = *(u8 *)((s8 *)arg0 + 0x19);
            t2 = *(u8 *)((s8 *)arg0 + 0x1A);
            __asm__ volatile (
                "sll %0,%3,4\n\t"
                "sll %1,%4,4\n\t"
                "sll %2,%5,4\n\t"
                "ctc2 %0,$13\n\t"
                "ctc2 %1,$14\n\t"
                "ctc2 %2,$15"
                : "=r"(t4), "=r"(t5), "=r"(t6)
                : "r"(t0), "r"(t1), "r"(t2));
        } else {
            t0 = 0x40;
            t1 = 0x40;
            t2 = 0x40;
            __asm__ volatile (
                "sll %0,%3,4\n\t"
                "sll %1,%4,4\n\t"
                "sll %2,%5,4\n\t"
                "ctc2 %0,$13\n\t"
                "ctc2 %1,$14\n\t"
                "ctc2 %2,$15"
                : "=r"(t4), "=r"(t5), "=r"(t6)
                : "r"(t0), "r"(t1), "r"(t2));
        }
    }
    func_8002C700(*(s32 *)((s8 *)arg0 + 0x14),
                  *(s32 *)((s8 *)arg0 + D_800928A0 * 4 + 8),
                  D_800928E4, *(s32 *)((s8 *)arg0 + 4));
}

/* CVECTOR layout (libgte): r, g, b, cd; byte-aligned, so copies use lwl/lwr. */
typedef struct {
    u8 r;
    u8 g;
    u8 b;
    u8 cd;
} BattlingCVec;
/* Same declaration as func_8008A78C's: 2 x 4-byte colours, one per buffer. */
extern s32 D_80092818[];
extern s32 D_80092914;

void func_8008A6F8(BattlingCVec *col) {
    BattlingCVec *cur;

    cur = (BattlingCVec *)&D_80092818[D_800928A0];
    if (col->r == cur->r && col->g == cur->g && col->b == cur->b) {
        D_80092914 = 0;
    } else {
        D_80092914 = 1;
        *cur = *col;
        cur->cd = 0x20;
    }
}

extern s32 D_80092818[];

void func_8008A78C(void *arg0) {
    u8 *model;
    s32 idx;
    s32 n;
    u8 *prim;
    s32 val;

    model = *(u8 **)(*(u8 **)((u8 *)arg0 + 4) + 0xC);
    idx = D_800928A0 * 4;
    prim = *(u8 **)(idx + (s32)model + 0x10);
    n = *(s16 *)(model + 2);
    val = *(s32 *)((u8 *)D_80092818 + idx);
    while (--n != -1) {
        *(s32 *)(prim + 4) = val;
        prim += 0x14;
    }
}

void CompMatrix(s32, void *, void *);
void MulMatrix0(s32, void *, void *);
void RotMatrix(void *, void *);
void TransMatrix(void *, void *);
void func_80030B14(void *);
void func_800731F8(void *, void *);
void func_8008BCC8(void *, s32);
extern s32 D_8009289C;
extern s32 D_80096FA8;
extern s32 D_80096FAC;
extern s32 D_80096FB0;
typedef struct { s32 w[8]; } BattlingNodeMtx;
/* Node tree update: per-kind matrix setup, then recurse into the +0x94 and +0x90 nodes. */

void func_8008A7E0(void *arg0) {
    void (*fn)(void *);
    u8 *q;

    fn = *(void (**)(void *))((u8 *)arg0 + 8);
    if (fn != 0) {
        fn(arg0);
    }
    switch (*(s32 *)arg0) {
    case 2:
        RotMatrix((u8 *)arg0 + 0x44, (u8 *)arg0 + 0xC);
        if (D_8009289C != 0) {
            MulMatrix0(*(s32 *)((u8 *)arg0 + 0x8C) + 0xC, (u8 *)arg0 + 0xC, (u8 *)arg0 + 0x6C);
        } else {
            *(BattlingNodeMtx *)((u8 *)arg0 + 0x6C) = *(BattlingNodeMtx *)((u8 *)arg0 + 0xC);
        }
        func_800731F8((u8 *)arg0 + 0xC, *(u8 **)((u8 *)arg0 + 4) + 0x10);
        *(BattlingNodeMtx *)((u8 *)arg0 + 0x4C) = *(BattlingNodeMtx *)((u8 *)arg0 + 0xC);
        *(s32 *)((u8 *)arg0 + 0x68) = 0;
        *(s32 *)((u8 *)arg0 + 0x64) = 0;
        *(s32 *)((u8 *)arg0 + 0x60) = 0;
        *(s32 *)((u8 *)arg0 + 0x20) = *(s32 *)((u8 *)arg0 + 0x34) - D_80096FA8;
        *(s32 *)((u8 *)arg0 + 0x24) = *(s32 *)((u8 *)arg0 + 0x38) - D_80096FAC;
        *(s32 *)((u8 *)arg0 + 0x28) = *(s32 *)((u8 *)arg0 + 0x3C) - D_80096FB0;
        CompMatrix(*(s32 *)((u8 *)arg0 + 0x8C) + 0xC, (u8 *)arg0 + 0xC, (u8 *)arg0 + 0xC);
        break;
    case 0:
    case 1:
        if (*(s32 *)((u8 *)arg0 + 0x8C) != 0) {
            *(s32 *)((u8 *)arg0 + 0x34) = *(s16 *)((u8 *)arg0 + 0x2C);
            *(s32 *)((u8 *)arg0 + 0x38) = *(s16 *)((u8 *)arg0 + 0x2E);
            *(s32 *)((u8 *)arg0 + 0x3C) = *(s16 *)((u8 *)arg0 + 0x30);
            RotMatrix((u8 *)arg0 + 0x44, (u8 *)arg0 + 0xC);
            TransMatrix((u8 *)arg0 + 0xC, (u8 *)arg0 + 0x34);
            MulMatrix0(*(s32 *)((u8 *)arg0 + 0x8C) + 0x6C, (u8 *)arg0 + 0xC, (u8 *)arg0 + 0x6C);
            CompMatrix(*(s32 *)((u8 *)arg0 + 0x8C) + 0x4C, (u8 *)arg0 + 0xC, (u8 *)arg0 + 0x4C);
            CompMatrix(*(s32 *)((u8 *)arg0 + 0x8C) + 0xC, (u8 *)arg0 + 0xC, (u8 *)arg0 + 0xC);
        }
        if (*(s32 *)arg0 == 1 && !(**(s32 **)((u8 *)arg0 + 4) & 1)) {
            func_80030B14((u8 *)arg0 + 0x6C);
            __asm__ volatile (
                "lw $12, 0(%0)\n\t"
                "lw $13, 4(%0)\n\t"
                "ctc2 $12, $0\n\t"
                "ctc2 $13, $1\n\t"
                "lw $12, 8(%0)\n\t"
                "lw $13, 12(%0)\n\t"
                "lw $14, 16(%0)\n\t"
                "ctc2 $12, $2\n\t"
                "ctc2 $13, $3\n\t"
                "ctc2 $14, $4"
                : : "r"((u8 *)arg0 + 0xC) : "$12", "$13", "$14");
            __asm__ volatile (
                "lw $12, 20(%0)\n\t"
                "lw $13, 24(%0)\n\t"
                "ctc2 $12, $5\n\t"
                "lw $14, 28(%0)\n\t"
                "ctc2 $13, $6\n\t"
                "ctc2 $14, $7"
                : : "r"((u8 *)arg0 + 0xC) : "$12", "$13", "$14");
            func_8008A63C(*(void **)((u8 *)arg0 + 4));
        }
        break;
    case 5:
        if (**(s32 **)((u8 *)arg0 + 4) == 1) {
            *(BattlingNodeMtx *)((u8 *)arg0 + 0xC) = *(BattlingNodeMtx *)(*(u8 **)(*(u8 **)((u8 *)arg0 + 4) + 4) + 0x4C);
            __asm__ volatile (
                "lw $12, 0(%0)\n\t"
                "lw $13, 4(%0)\n\t"
                "ctc2 $12, $0\n\t"
                "ctc2 $13, $1\n\t"
                "lw $12, 8(%0)\n\t"
                "lw $13, 12(%0)\n\t"
                "lw $14, 16(%0)\n\t"
                "ctc2 $12, $2\n\t"
                "ctc2 $13, $3\n\t"
                "ctc2 $14, $4"
                : : "r"((u8 *)arg0 + 0xC) : "$12", "$13", "$14");
            __asm__ volatile (
                "lw $12, 20(%0)\n\t"
                "lw $13, 24(%0)\n\t"
                "ctc2 $12, $5\n\t"
                "lw $14, 28(%0)\n\t"
                "ctc2 $13, $6\n\t"
                "ctc2 $14, $7"
                : : "r"((u8 *)arg0 + 0xC) : "$12", "$13", "$14");
            if (D_80092914 != 0) {
                func_8008A78C(arg0);
            }
            q = *(u8 **)(*(u8 **)((u8 *)arg0 + 4) + 0xC);
            func_8008BCC8(*(void **)(q + 0xC), *(s32 *)(q + 8));
        }
        break;
    }
    if (*(void **)((u8 *)arg0 + 0x94) != 0) {
        func_8008A7E0(*(void **)((u8 *)arg0 + 0x94));
    }
    if (*(void **)((u8 *)arg0 + 0x90) != 0) {
        func_8008A7E0(*(void **)((u8 *)arg0 + 0x90));
    }
}


void func_8008ABAC(void *arg0) {
    func_80030A30(0, *(s32 *)((s8*)(*(void **)((s8*)(arg0) + 0)) + 4));
    func_80030A30(1, *(s32 *)((s8*)(*(void **)((s8*)(arg0) + 4)) + 4));
    func_80030A30(2, *(s32 *)((s8*)(*(void **)((s8*)(arg0) + 8)) + 4));
}


void func_8008AC0C(void *arg0) {
    ClearOTagR(*(s32 *)((s8*)(((D_800928A0 * 4) + arg0)) + 4), *(s16 *)((s8*)(arg0) + 0x14));
    D_800928E4 = *(s32 *)((s8*)(((D_800928A0 * 4) + arg0)) + 4);
    D_80050100 = (s32) *(u8 *)((s8*)(arg0) + 0x17);
}


void func_8008AC7C(s32 arg0) {
    D_80091C30 = arg0;
}


void func_8008AC8C(void) {
    D_80092820 = GetRCnt(0xF2000001);
}

typedef struct {
    unsigned addr : 24;
    unsigned len : 8;
} BattlingOtTag;

void func_8008ACB8(s32 arg0) {
    void *ctx;
    s32 start;
    s32 limit;
    s32 i;
    s32 elapsed;
    BattlingOtTag *ent;
    BattlingOtTag *p;

    ctx = (void *)D_80091C30;
    if (ctx != NULL) {
        start = D_80092820;
        D_80091C30 = 0;
        limit = arg0 != 0 ? arg0 * 0xF0 : 0xC0;
        i = *(s16 *)((s8 *)ctx + 0x14) - 1;
        if (i >= 4) {
            do {
                elapsed = GetRCnt(0xF2000001) - start;
                if (elapsed < 0) {
                    elapsed += 0x10000;
                }
                if (limit < elapsed) {
                    break;
                }
                ent = ((BattlingOtTag **)*(s32 *)((s8 *)((D_800928A0 * 4) + ctx) + 4))[i];
                p = (BattlingOtTag *)(ent->addr + 0x80000000);
                if (p->len == 0) {
                    while (i >= 5) {
                        p = (BattlingOtTag *)(p->addr + 0x80000000);
                        i--;
                        if (p->len != 0) {
                            break;
                        }
                    }
                    ent->addr = (u32)p;
                }
                i--;
            } while (i >= 4);
        }
    }
}

void AddPrim(void *ot, void *p);
void AddPrims(void *ot, void *p0, void *p1);
void func_800453E8(void *, s32);
void func_8004546C(void *, s32);
extern s32 D_80092868;
extern s32 D_80092938;

void func_8008AE1C(void *arg0) {
    void *temp_v0;

    func_8008AC7C((s32)arg0);
    temp_v0 = (void *)((D_800928A0 * 4) + (s32)arg0);
    AddPrims((void *)D_80092938, *(void **)((s8*)(temp_v0) + 0xC), *(void **)((s8*)(temp_v0) + 4));
    if (!(*(u8 *)((s8*)(arg0) + 0x16) & 4)) {
        func_800453E8(arg0 + ((D_800928A0 * 0xC) + 0x18), D_80092868);
    }
    if (!(*(u8 *)((s8*)(arg0) + 0x16) & 8)) {
        func_8004546C(arg0 + ((D_800928A0 * 0xC) + 0x30), D_80092868 + 8);
    }
    if (*(u8 *)((s8*)(arg0) + 0x16) & 0x10) {
        AddPrim((void *)D_80092938, arg0 + ((D_800928A0 * 0x10) + 0x48));
    }
    AddPrim((void *)D_80092938, arg0 + ((D_800928A0 * 0xC) + 0x30));
    AddPrim((void *)D_80092938, arg0 + ((D_800928A0 * 0xC) + 0x18));
}

/* Retail returns arg0 (v0 = s0 at the end), but the TU prototypes it as
 * `void func_8008AF6C(s32)`. The definition is given a C name of its own
 * bound to the retail symbol; alternatively change that TU prototype to
 * `void *func_8008AF6C(u8 *)` and define it under its own name. */
void *func_8008AF6C__ret(u8 *arg0) asm("func_8008AF6C");

/* Relocates a loaded block: every stored pointer is rebased by the distance
 * between its old load address (+0x1C) and arg0. */
void *func_8008AF6C__ret(u8 *arg0) {
    s32 delta;
    s32 sub;
    s32 *table;
    u32 i;
    s32 *p;

    delta = arg0 - *(u8 **)(arg0 + 0x1C);
    sub = *(s32 *)(arg0 + 0xC);
    *(u8 **)(arg0 + 0x1C) = arg0;
    *(s32 *)(arg0 + 0) += delta;
    *(s32 *)(arg0 + 4) += delta;
    *(s32 *)(arg0 + 0x10) += delta;
    *(s32 *)(arg0 + 0x14) += delta;
    *(s32 *)(arg0 + 0x18) += delta;
    *(s32 *)(arg0 + 0x20) += delta;
    *(s32 *)(arg0 + 0x24) += delta;
    if (sub != 0) {
        *(s32 *)(arg0 + 0xC) = sub + delta;
        func_8008A040(sub + delta);
    }
    table = *(s32 **)(arg0 + 8);
    if (table != NULL) {
        *(s32 **)(arg0 + 8) = (s32 *)((u8 *)table + delta);
        for (i = 1; i < **(s32 **)(arg0 + 8) + 1; i++) {
            p = (s32 *)(i * 4 + *(s32 *)(arg0 + 8));
            if (*p != 0) {
                *p += delta;
            }
        }
    }
    return arg0;
}


void func_8008B070(s32 arg0) {
    s32 temp_v0;

    HeapSetCurrentContentType(0xA);
    temp_v0 = HeapAlloc(ArchiveDecodeSize(arg0), 0);
    ArchiveReadFileToBuffer(arg0, temp_v0, 0, 0);
    ArchiveCdDataSync(0);
    func_8008AF6C(temp_v0);
}

typedef struct {
    s32 unk0;
    s32 unk4;
    s32 unk8;
    s16 unkC;
    s16 unkE;
    s16 unk10;
    s16 unk12;
} BattlingAnimEntry;

void func_8008B0D8(void *arg0) {
    s32 i;
    BattlingAnimEntry *e;

    *(s16 *)((u8 *)arg0 + 0x10) = 0;
    *(s16 *)((u8 *)arg0 + 0x12) = 0;
    e = *(BattlingAnimEntry **)((u8 *)arg0 + 8);
    for (i = 0; i < *(s16 *)(*(u8 **)arg0 + 6); i++) {
        e->unkC = 0;
        e->unkE = 0;
        e->unk4 = e->unk0;
        e->unk10 = 0;
        e++;
    }
}

INCLUDE_ASM("asm/battling/nonmatchings/main", func_8008B13C);
void func_8002C3E8(s32);
void func_8008B13C(void *, void *, void *);
void *func_80089E74__r(void) asm("func_80089E74");
void *func_80089FC4__r(void) asm("func_80089FC4");

typedef struct { s16 parent; s16 anim; u16 f4; u16 f6; u16 f8; u16 fA; u16 fC; u16 fE; } BattlingJointDef;
typedef struct { s32 a; s32 b; s32 c; s32 d; s32 e; } BattlingOutEntry;

void *func_8008B38C(void *arg0) {
    void *root;
    s32 tbl2;
    s32 base;
    u32 *hdr;
    u32 count;
    BattlingJointDef *defs;
    void **arr;
    void *tmp;
    register void *node asm("$17");
    void *anim;
    BattlingOutEntry *dst;
    u32 i;
    s32 off;
    s16 idx;
    void *parent;

    base = *(s32 *)((u8 *)arg0 + 4);
    hdr = *(u32 **)((u8 *)arg0 + 0);
    tbl2 = *(s32 *)((u8 *)arg0 + 8);
    count = *hdr;
    defs = (BattlingJointDef *)(hdr + 1);
    func_8002C3E8(base);
    HeapSetCurrentContentType(0x12);
    arr = HeapAlloc(count * 4, 0);
    tmp = func_80089E74__r();
    root = func_80089C54();
    func_80089E54(root, (s32)tmp);
    *(void ***)((u8 *)tmp + 4) = arr;
    *(s16 *)((u8 *)tmp + 0) = count;
    *(BattlingJointDef **)((u8 *)tmp + 0x18) = defs;
    for (i = 0; i < count; i++) {
        node = func_80089C54();
        arr[i] = node;
        if (defs[i].anim != -1) {
            anim = func_80089FC4__r();
            func_80089E2C(node, (s32)anim);
            off = defs[i].anim * 0x38 + 0x10;
            func_8008A184(anim, (void *)(base + off));
        }
        idx = defs[i].parent;
        if (idx == -1) {
            parent = root;
        } else {
            parent = arr[idx];
        }
        func_80089C88(parent, node);
        *(u16 *)((u8 *)node + 0x44) = defs[i].f4;
        *(u16 *)((u8 *)node + 0x46) = defs[i].f6;
        *(u16 *)((u8 *)node + 0x48) = defs[i].f8;
        *(u16 *)((u8 *)node + 0x2C) = defs[i].fA;
        *(u16 *)((u8 *)node + 0x2E) = defs[i].fC;
        *(u16 *)((u8 *)node + 0x30) = defs[i].fE;
    }
    if (tbl2 != 0) {
        base = tbl2;
        HeapSetCurrentContentType(0x11);
        dst = HeapAlloc(*(u32 *)base * 0x14, 0);
        *(BattlingOutEntry **)((u8 *)tmp + 8) = dst;
        *(s16 *)((u8 *)tmp + 2) = *(u32 *)base;
        for (i = 0; i < *(u32 *)base; i++) {
            if (((void **)base)[i + 1] != NULL) {
                func_8008B13C(((void **)base)[i + 1], &dst[i], root);
            } else {
                dst[i].a = 0;
            }
        }
    }
    return root;
}

void func_8008B5DC(void *arg0, s32 arg1) {
    func_8008B730(arg0, arg1, 1);
}

s16 func_8008B5FC(s32 arg0, s32 arg1, s32 arg2) {
    s16 temp_a0;
    s16 var_v0;
    s32 temp_v1;

    temp_a0 = arg0 & 0xFFF;
    temp_v1 = (temp_a0 - arg1) & 0xFFF;
    var_v0 = temp_a0;
    if (temp_v1 != 0) {
        if (temp_v1 < 0x800) {
            var_v0 = temp_a0 - (temp_v1 / arg2);
        } else {
            var_v0 = temp_a0 + ((s32) (0x1000 - temp_v1) / arg2);
        }
    }
    return var_v0;
}

s32 func_8008B650__wide(s32, s32, s32, s32) asm("func_8008B650");
s32 func_8008B650__wide(s32 current, s32 target, s32 step, s32 unused) {
    s16 value = current;
    s16 goal = target;
    s32 delta = (current - target) & 0xFFF;
    if (delta != 0) {
        if (delta == 0x800) {
            if (rand() & 1) goto decrease;
            else goto increase;
        }
        if (delta < 0x800) {
        decrease:
            value -= step;
            if (((value - goal) & 0xFFF) > 0x800) value = goal;
        } else {
        increase:
            value += step;
            if (((value - goal) & 0xFFF) < 0x800) value = goal;
        }
    }
    return value;
}

typedef struct { s16 len; s16 pad; s16 n1; s16 n2; } BattlingAnimHdr;
typedef struct { u16 *dst; u16 goal; s16 flag; } BattlingAnimK1;
typedef struct { u32 pad0; u8 *stream; u16 *dst; u16 count; u16 val; s16 delta; s16 flag; } BattlingAnimK2;
typedef struct { BattlingAnimHdr *hdr; BattlingAnimK1 *k1; BattlingAnimK2 *k2; u8 pad[6]; s16 cur; } BattlingAnimPlayer;

/* steps == 0 falls off the end with no return value, as retail does (the only caller ignores it); a port build compiling this TU needs an explicit return. */
s32 func_8008B730(BattlingAnimPlayer *p, s32 steps, s32 div) {
    BattlingAnimK1 *a;
    BattlingAnimK2 *b;
    register s32 r asm("$2");
    s32 i;
    s32 k;
    s16 cur;
    s32 len;
    s8 c;
    u16 x;
    u16 v;

    if (p->hdr == 0) {
        return 1;
    }
    if (steps != 0) {
        cur = p->cur;
        len = p->hdr->len;
        if (len < cur + steps) {
            steps = len - cur;
        }
        p->cur = cur + steps;
        div -= steps;
        if (div <= 0) {
            div = 1;
        }
        a = p->k1;
        if (div == 1) {
            for (i = 0; i < p->hdr->n1; i++, a++) {
                *a->dst = a->goal;
            }
        } else {
            for (i = 0; i < p->hdr->n1; i++, a++) {
                v = *a->dst;
                if (a->flag != 0) {
                    *a->dst = func_8008B5FC((s16)v, (s16)a->goal, div);
                } else {
                    *a->dst = v + (((s16)a->goal - (s16)v) / div);
                }
            }
        }
        b = p->k2;
        for (i = 0; i < p->hdr->n2; i++, b++) {
            for (k = 0; k < steps; k++) {
                if (b->count != 0) {
                    b->count--;
                } else {
                    c = *b->stream++;
                    if (c & 0x80) {
                        x = c & 0x3F;
                        if (c & 0x40) {
                            x |= (*b->stream++ << 24) >> 18;
                            b->delta = x;
                        } else {
                            b->count = x;
                        }
                    } else {
                        b->delta = (s8)(c << 1) >> 1;
                    }
                }
                b->val += b->delta;
            }
            if (div == 1) {
                *b->dst = b->val;
            } else {
                v = *b->dst;
                if (b->flag != 0) {
                    *b->dst = func_8008B5FC((s16)v, (s16)b->val, div);
                } else {
                    *b->dst = v + (((s16)b->val - (s16)v) / div);
                }
            }
        }
        r = 0;
        if (p->cur == p->hdr->len) {
            r = div == 1;
        }
        return r;
    }
}

void *func_8008BA2C(s32 arg0, s32 arg1, s32 arg2, s32 arg3) {
    s32 *p;
    s32 i;
    s32 cursor;
    u8 *obj;

    HeapSetCurrentContentType(3);
    obj = HeapAlloc(0x84, 2);
    i = 0x1F;
    p = (s32 *)(obj + 0x7C);
    do {
        *p = 0;
        i--;
        p--;
    } while (i >= 0);
    *(s32 *)(obj + 0x80) = arg2;
    *(s32 *)(obj + 0x70) = func_800405E4();
    *(s32 *)(obj + 0x7C) = arg0;
    *(s32 *)(obj + 0x10) = arg1;
    cursor = *(s32 *)(obj + 0x80) + arg3 * 4;
    *(s32 *)(obj + 0x74) = cursor;
    *(s32 *)(obj + 0x78) = cursor;
    func_8008BB3C(obj);
    return obj;
}
void func_8008BAE0(s32 arg0) {
    HeapFree(arg0);
}
extern s32 D_80096D88;
extern s32 D_80096D8C;

/* Hand-written in retail (context-switch module, see func_8008BB1C): reads the
 * saved pair into $t0/$t1, so both temporaries are pinned. */
void func_8008BB00(s32 *arg0) {
    register s32 t0 asm("$8");
    register s32 t1 asm("$9");

    t0 = D_80096D88;
    t1 = D_80096D8C;
    arg0[0] = t0;
    arg0[1] = t1;
}

/* Retail is hand-written: it belongs to the context-switch module with
 * func_8008BB00 / func_8008BB3C / func_8008BC04 (which save $k0/$k1/$sp into
 * D_80096D88). gcc 2.7.2 copies through $v0 (two scalars) or $v0/$v1 (an
 * 8-byte struct, move_by_pieces), never $t0, so the temporary is pinned. */
void func_8008BB1C(s32 *arg0) {
    register s32 t asm("$8");

    t = arg0[0];
    D_80096D88 = t;
    t = arg0[1];
    D_80096D8C = t;
}

INCLUDE_ASM("asm/battling/nonmatchings/main", func_8008BB3C);
INCLUDE_ASM("asm/battling/nonmatchings/main", func_8008BC04);
/* VECTOR layout (libgte): vx, vy, vz, pad. Shared by func_8008BCC8 / func_8008E78C. */
#ifndef BATTLING_VEC_DEFINED
#define BATTLING_VEC_DEFINED
typedef struct {
    s32 vx;
    s32 vy;
    s32 vz;
    s32 pad;
} BattlingVec;
#endif
long VectorNormal();
void func_8008C3A8(s32, s32, u16);
extern BattlingVec D_8009A2C8;

void func_8008BCC8(void *arg0, s32 arg1) {
    BattlingVec dir;
    s32 unused[4];

    dir.vx = -8;
    dir.vy = -8;
    dir.vz = -16;
    VectorNormal(&dir, &D_8009A2C8);
    D_8009A2C8.vx <<= 4;
    D_8009A2C8.vy <<= 4;
    D_8009A2C8.vz <<= 4;
    func_8008C3A8(*(s32 *)((u8 *)arg0 + 8), arg1, *(u16 *)((u8 *)arg0 + 2));
}

void func_8008C4B0(void *, s16);                 /* extern */
void func_8008C620(void *, s16);                 /* extern */
extern s32 D_80059424;
extern void *D_80059528;
extern s32 D_8005953C;
extern s32 D_80059568;
extern s32 D_800595C0;

typedef struct { u32 unused0; u16 add, count; u32 unused8[2]; void *data; } BattlingPacketGroup;
void func_8008BD70(s32 arg0, s32 arg1, s32 arg2, s32 arg3) {
    s32 count = ((BattlingPacketGroup *)arg0)->count;
    register void *data asm("$3") = ((BattlingPacketGroup *)arg0)->data;
    void *packet;
    register s32 packet_size asm("$2");
    register s32 end asm("$2");
    register s32 added asm("$4") = ((BattlingPacketGroup *)arg0)->add;
    D_80059424 = arg1;
    D_80059568 = arg2;
    D_8005953C = arg3;
    D_800595C0 += added;
    --count;
    goto test;
    do {
        packet = *(void *volatile *)&D_80059528;
        D_80059528 = (u8 *)packet + 4;
        if (*(u8 *)packet & 8) {
            --count;
            func_8008C620((u8 *)packet + 4, *(s16 *)((u8 *)packet + 2));
        } else {
            --count;
            func_8008C4B0((u8 *)packet + 4, *(s16 *)((u8 *)packet + 2));
        }
        packet_size = *(s16 *)((u8 *)packet + 2) * 8;
        data = (u8 *)D_80059528 + packet_size;
    test:
        end = -1;
        D_80059528 = data;
    } while (count != end);
}

void func_8008BE4C(void *arg0, void *arg1) {
    s32 n;
    s32 cnt3;
    s32 cnt4;
    s32 m;
    s32 k;
    void *pkt;
    void *p;
    void *q;
    void *r;

    *(u16 *)((s8*)(arg0) + 0) = (u16) *(u16 *)((s8*)(arg1) + 2);
    *(u16 *)((s8*)(arg0) + 2) = (u16) *(u16 *)((s8*)(arg1) + 4);
    *(s32 *)((s8*)(arg0) + 4) = (s32) *(s32 *)((s8*)(arg1) + 8);
    *(void **)((s8*)(arg0) + 0xC) = arg1;
    D_80059528 = *(void **)((s8*)(arg1) + 0x10);
    HeapSetCurrentContentType(0x13);
    p = HeapAlloc((s16) *(u16 *)((s8*)(arg0) + 0) * 8, 2);
    n = (s16) *(u16 *)((s8*)(arg0) + 0);
    *(void **)((s8*)(arg0) + 8) = p;
    while (--n != -1) {
        *(s16 *)((s8*)(p) + 2) = 0;
        p += 8;
    }
    HeapSetCurrentContentType(5);
    cnt3 = 0;
    n = *(u16 *)((s8*)(arg1) + 6);
    cnt4 = 0;
    while (--n != -1) {
        pkt = D_80059528;
        D_80059528 = pkt + 4;
        if (*(u8 *)((s8*)(pkt) + 0) & 8) {
            cnt4 += *(s16 *)((s8*)(pkt) + 2);
        } else {
            cnt3 += *(s16 *)((s8*)(pkt) + 2);
        }
        D_80059528 += *(s16 *)((s8*)(pkt) + 2) * 8;
    }
    n = (cnt3 * 0x14) + (cnt4 * 0x18);
    q = HeapAlloc(n * 2, 2);
    *(void **)((s8*)(arg0) + 0x10) = q;
    *(void **)((s8*)(arg0) + 0x14) = (void *) (q + n);
    m = *(u16 *)((s8*)(arg1) + 6);
    D_80059528 = *(void **)((s8*)(arg1) + 0x10);
    while (--m != -1) {
        pkt = D_80059528;
        D_80059528 = pkt + 4;
        if (*(u8 *)((s8*)(pkt) + 0) & 8) {
            k = *(s16 *)((s8*)(pkt) + 2);
            r = q + 4;
            while (--k != -1) {
                *(s8 *)((s8*)(r) + -1) = 5;
                *(s32 *)((s8*)(r) + 0) = 0x28403030;
                r += 0x18;
                q += 0x18;
            }
        } else {
            k = *(s16 *)((s8*)(pkt) + 2);
            r = q + 4;
            while (--k != -1) {
                *(s8 *)((s8*)(r) + -1) = 4;
                *(s32 *)((s8*)(r) + 0) = 0x20403030;
                r += 0x14;
                q += 0x14;
            }
        }
        D_80059528 += *(s16 *)((s8*)(pkt) + 2) * 8;
    }
    func_800732AC(*(s32 *)((s8*)(arg0) + 0x14), *(s32 *)((s8*)(arg0) + 0x10), n);
}


void func_8008C0BC(void *arg0, s32 arg1) {
    *(s32 *)((s8*)(arg0) + 0) = 5;
    *(s32 *)((s8*)(arg0) + 4) = arg1;
}

typedef struct {
    s32 type;
    s32 *src;
    s32 unk8;
    s32 unkC;
} BattlingTreeEntry;

void *func_8008C0CC(s32 *arg0) {
    BattlingTreeEntry *entry;

    HeapSetCurrentContentType(7);
    entry = HeapAlloc(0x10, 2);
    entry->type = *arg0;
    entry->src = arg0;
    entry->unk8 = D_80092828;
    entry->unkC = 0;
    return entry;
}


void func_8008C120(void *arg0) {
    s32 temp_a0_2;
    void *temp_a0;
    void *temp_s0;

    temp_s0 = *(void **)((s8*)(arg0) + 0xC);
    if (temp_s0 != NULL) {
        temp_a0 = *(void **)((s8*)(temp_s0) + 8);
        if (temp_a0 != NULL) {
            HeapFree(temp_a0);
        }
        temp_a0_2 = *(s32 *)((s8*)(temp_s0) + 0x10);
        if (temp_a0_2 != 0) {
            HeapDelayedFree(temp_a0_2, 2);
        }
        HeapFree(temp_s0);
    }
}

extern s32 D_80092824;
void *func_8008C188(void *arg0, void *arg1) {
    s32 count;
    void *node;
    s32 buf;
    void *child;
    s32 *entry;

    HeapSetCurrentContentType(6);
    node = func_80089C54();
    entry = func_8008C0CC(arg0);
    func_8008C0BC(node, entry);
    if (entry[0] == 1) {
        count = *(s32 *)(*(u8 **)((u8 *)arg0 + 4) + 0x14);
        HeapSetCurrentContentType(5);
        buf = HeapAlloc(0x18, 2);
        entry[3] = buf;
        func_8008BE4C(buf, count);
    }
    child = *(void **)((u8 *)arg0 + 0x94);
    D_80092824++;
    if (child != NULL) {
        func_80089C88(node, func_8008C188(child, node));
    }
    child = *(void **)((u8 *)arg0 + 0x90);
    if (child != NULL) {
        func_80089C88(arg1, func_8008C188(child, arg1));
    }
    return node;
}
extern s32 D_80092824;
/* K&R definition: func_8008C1DC calls it with no arguments, and retail
 * forwards whatever a0 holds to func_8008C188. */
void func_8008C298(arg0)
    s32 arg0;
{
    D_80092824 = 0;
    func_8008C188(arg0, 0);
}

void func_8008C2C0(s32 arg0) {
    D_80092828 = arg0;
    func_8008C298();
}


void func_8008C2E8(void *arg0) {
    void *temp_a0;
    void *temp_a0_2;
    void *temp_a0_3;
    void *temp_v0;

    temp_a0 = *(void **)((s8*)(arg0) + 4);
    if ((*(s32 *)((s8*)(temp_a0) + 0) == 1) && !(**(s32 **)((s8*)(*(void **)((s8*)(temp_a0) + 4)) + 4) & 1)) {
        temp_v0 = *(void **)((s8*)(temp_a0) + 0xC);
        func_8008BD70(*(s32 *)((s8*)(temp_v0) + 0xC), *(s32 *)((s8*)(((D_800928A0 * 4) + temp_v0)) + 0x10), D_800928E4 + 4, *(s32 *)((s8*)(temp_v0) + 8));
    }
    temp_a0_2 = *(void **)((s8*)(arg0) + 0x94);
    if (temp_a0_2 != NULL) {
        func_8008C2E8(temp_a0_2);
    }
    temp_a0_3 = *(void **)((s8*)(arg0) + 0x90);
    if (temp_a0_3 != NULL) {
        func_8008C2E8(temp_a0_3);
    }
}

INCLUDE_ASM("asm/battling/nonmatchings/main", func_8008C3A8);
INCLUDE_ASM("asm/battling/nonmatchings/main", func_8008C4B0);
INCLUDE_ASM("asm/battling/nonmatchings/main", func_8008C620);
/* 8-byte, 2-byte-aligned vector (SVECTOR layout): copies of it compile to
 * lwl/lwr + swl/swr pairs. Shared by func_8008C7C0 / func_8008C9B8 / func_8008CC2C / func_8008CCB0 / func_8008CF9C. */
#ifndef BATTLING_SVEC_DEFINED
#define BATTLING_SVEC_DEFINED
typedef struct {
    s16 vx;
    s16 vy;
    s16 vz;
    s16 pad;
} BattlingSVec;
#endif

void func_8008C7C0(void *arg0) {
    *(BattlingSVec *)((u8 *)arg0 + 0x20) = *(BattlingSVec *)arg0;
    *(BattlingSVec *)((u8 *)arg0 + 0x18) = *(BattlingSVec *)((u8 *)arg0 + 0x20);
    *(BattlingSVec *)((u8 *)arg0 + 0x10) = *(BattlingSVec *)((u8 *)arg0 + 0x18);
}

/* GPU LINE_F4 primitive (libgpu LINE_F4 layout): tag, r0 g0 b0 code,
 * x0 y0 .. x3 y3, pad (0x55555555 terminator). */
typedef struct {
    u32 tag;
    u8 r0;
    u8 g0;
    u8 b0;
    u8 code;
    s16 x0;
    s16 y0;
    s16 x1;
    s16 y1;
    s16 x2;
    s16 y2;
    s16 x3;
    s16 y3;
    u32 pad;
} BattlingLineF4;

void func_8008C828(void *arg0, void *arg1) {
    BattlingLineF4 *line;

    line = (BattlingLineF4 *)((u8 *)arg0 + 0x28);
    *((u8 *)line + 3) = 6;
    line->code = 0x4C;
    line->pad = 0x55555555;
    line->code |= 2;
    line->r0 = *((u8 *)arg1 + 0x74);
    line->g0 = *((u8 *)arg1 + 0x75);
    line->b0 = *((u8 *)arg1 + 0x76);
    line[1] = line[0];
}

typedef struct { s16 x, y, z, pad; } __attribute__((packed)) BattlingTrailVec;
s32 RotTransPers4();
void func_80031750(s32, void *);
void func_8008C8B4(void *arg0, s32 arg1) {
    s32 depth;
    void *p;
    s32 t;
    p = (u8 *)arg0 + (D_800928A0 * 0x1C + 0x28);
    t = RotTransPers4(arg0, (u8 *)arg0 + 0x10, (u8 *)arg0 + 0x18, (u8 *)arg0 + 0x20, (u8 *)p + 8, (u8 *)p + 0xC, (u8 *)p + 0x10, (u8 *)p + 0x14, &depth, &depth);
    *(BattlingTrailVec *)((u8 *)arg0 + 0x20) = *(BattlingTrailVec *)((u8 *)arg0 + 0x18);
    *(BattlingTrailVec *)((u8 *)arg0 + 0x18) = *(BattlingTrailVec *)((u8 *)arg0 + 0x10);
    *(BattlingTrailVec *)((u8 *)arg0 + 0x10) = *(BattlingTrailVec *)arg0;
    func_80031750(arg1 + (t >> 2) * 4, p);
}

/* 8-byte, 2-byte-aligned vector (SVECTOR layout): copies of it compile to
 * lwl/lwr + swl/swr pairs. Shared by func_8008C7C0 / func_8008C9B8 / func_8008CC2C / func_8008CCB0 / func_8008CF9C. */
#ifndef BATTLING_SVEC_DEFINED
#define BATTLING_SVEC_DEFINED
typedef struct {
    s16 vx;
    s16 vy;
    s16 vz;
    s16 pad;
} BattlingSVec;
#endif

void func_8008C9B8(void *arg0) {
    *(BattlingSVec *)((u8 *)arg0 + 0x18) = *(BattlingSVec *)arg0;
    *(BattlingSVec *)((u8 *)arg0 + 0x10) = *(BattlingSVec *)((u8 *)arg0 + 0x18);
}

/* GPU LINE_F3 primitive (libgpu LINE_F3 layout): tag, r0 g0 b0 code, x0 y0,
 * x1 y1, x2 y2, pad (0x55555555 terminator). */
typedef struct {
    u32 tag;
    u8 r0;
    u8 g0;
    u8 b0;
    u8 code;
    s16 x0;
    s16 y0;
    s16 x1;
    s16 y1;
    s16 x2;
    s16 y2;
    u32 pad;
} BattlingLineF3;

void func_8008CA00(void *arg0, void *arg1) {
    BattlingLineF3 *line;

    line = (BattlingLineF3 *)((u8 *)arg0 + 0x20);
    *((u8 *)line + 3) = 5;
    line->code = 0x48;
    line->pad = 0x55555555;
    line->code |= 2;
    line->r0 = *((u8 *)arg1 + 0x74);
    line->g0 = *((u8 *)arg1 + 0x75);
    line->b0 = *((u8 *)arg1 + 0x76);
    line[1] = line[0];
}

extern s32 D_8009282C;
extern s32 D_80092830;
void func_80031708(s32, void *);

void func_8008CA84(void *arg0, s32 arg1) {
    s32 otz;
    u8 *prim;
    u8 idx;
    u16 *dst;
    u16 *cam;

    dst = (u16 *)D_8009282C;
    cam = (u16 *)D_80092830;
    dst[0] = *(u16 *)arg0 - cam[0];
    dst[1] = *(u16 *)((u8 *)arg0 + 2) - cam[1];
    dst[2] = *(u16 *)((u8 *)arg0 + 4) - cam[2];
    dst[4] = *(u16 *)((u8 *)arg0 + 0x10) - cam[0];
    dst[5] = *(u16 *)((u8 *)arg0 + 0x12) - cam[1];
    dst[6] = *(u16 *)((u8 *)arg0 + 0x14) - cam[2];
    dst[8] = *(u16 *)((u8 *)arg0 + 0x18) - cam[0];
    dst[9] = *(u16 *)((u8 *)arg0 + 0x1A) - cam[1];
    dst[10] = *(u16 *)((u8 *)arg0 + 0x1C) - cam[2];
    __asm__ volatile("lwc2 $0,0(%0)\n\tlwc2 $1,4(%0)\n\tlwc2 $2,8(%0)\n\tlwc2 $3,12(%0)\n\tlwc2 $4,16(%0)\n\tlwc2 $5,20(%0)" : : "r"(D_8009282C) : "memory");
    __asm__ volatile("nop\n\tnop\n\t.word 0x4A280030");
    idx = D_800928A0;
    *(BattlingSVec *)((u8 *)arg0 + 0x18) = *(BattlingSVec *)((u8 *)arg0 + 0x10);
    *(BattlingSVec *)((u8 *)arg0 + 0x10) = *(BattlingSVec *)arg0;
    prim = (u8 *)arg0 + (idx * 24 + 0x20);
    __asm__ volatile("swc2 $12,0(%0)\n\tswc2 $13,0(%1)\n\tswc2 $14,0(%2)" : : "r"(prim + 8), "r"(prim + 0xC), "r"(prim + 0x10) : "memory");
    __asm__ volatile("mfc2 $12,$19\n\tnop\n\tsra $12,$12,2\n\tsw $12,0(%0)" : : "r"(&otz) : "$12", "memory");
    func_80031708(arg1 + (otz >> 2) * 4, prim);
}

/* 8-byte, 2-byte-aligned vector (SVECTOR layout): copies of it compile to
 * lwl/lwr + swl/swr pairs. Shared by func_8008C7C0 / func_8008C9B8 / func_8008CC2C / func_8008CCB0 / func_8008CF9C. */
#ifndef BATTLING_SVEC_DEFINED
#define BATTLING_SVEC_DEFINED
typedef struct {
    s16 vx;
    s16 vy;
    s16 vz;
    s16 pad;
} BattlingSVec;
#endif

void func_8008CC2C(void *arg0) {
    *(BattlingSVec *)((u8 *)arg0 + 0x10) = *(BattlingSVec *)arg0;
}

/* GPU LINE_F2 primitive (libgpu LINE_F2 layout): tag, r0 g0 b0 code, x0 y0, x1 y1. */
typedef struct {
    u32 tag;
    u8 r0;
    u8 g0;
    u8 b0;
    u8 code;
    s16 x0;
    s16 y0;
    s16 x1;
    s16 y1;
} BattlingLineF2;

void func_8008CC54(void *arg0, void *arg1) {
    BattlingLineF2 *line;

    line = (BattlingLineF2 *)((u8 *)arg0 + 0x18);
    *((u8 *)line + 3) = 3;
    line->code = 0x42;
    line->r0 = *((u8 *)arg1 + 0x74);
    line->g0 = *((u8 *)arg1 + 0x75);
    line->b0 = *((u8 *)arg1 + 0x76);
    line[1] = line[0];
}

/* 8-byte, 2-byte-aligned vector (SVECTOR layout): copies of it compile to
 * lwl/lwr + swl/swr pairs. Shared by func_8008C7C0 / func_8008C9B8 / func_8008CC2C / func_8008CCB0 / func_8008CF9C. */
#ifndef BATTLING_SVEC_DEFINED
#define BATTLING_SVEC_DEFINED
typedef struct {
    s16 vx;
    s16 vy;
    s16 vz;
    s16 pad;
} BattlingSVec;
#endif
long RotTransPers3();
void func_800316C0(u32 *pPrim, u32 nextAddr);

/* Projects a 2-point line (current and previous position) into the LINE_F2
 * of the current buffer, then links it into the OT. */
void func_8008CCB0(void *arg0, u32 *ot) {
    s32 tmp;
    u8 *prim;
    s32 z;

    prim = (u8 *)arg0 + (D_800928A0 * 16 + 0x18);
    z = RotTransPers3(arg0, (u8 *)arg0 + 0x10, &tmp, prim + 8, prim + 0xC, &tmp, &tmp, &tmp);
    *(BattlingSVec *)((u8 *)arg0 + 0x10) = *(BattlingSVec *)arg0;
    func_800316C0(ot + (z >> 2), (u32)prim);
}


void func_8008CD54(void) {

}

/* GPU TILE primitive (libgpu TILE layout): tag, r0 g0 b0 code, x0 y0, w h. */
typedef struct {
    u32 tag;
    u8 r0;
    u8 g0;
    u8 b0;
    u8 code;
    s16 x0;
    s16 y0;
    s16 w;
    s16 h;
} BattlingTile;

void func_8008CD5C(void *arg0, void *arg1) {
    BattlingTile *tile;
    s16 h;

    tile = (BattlingTile *)((u8 *)arg0 + 0x10);
    *((u8 *)tile + 3) = 3;
    tile->code = 0x62;
    h = rand() % 2 + 2;
    tile->h = h;
    tile->w = h * 2;
    tile->r0 = *((u8 *)arg1 + 0x74);
    tile->g0 = *((u8 *)arg1 + 0x75);
    tile->b0 = *((u8 *)arg1 + 0x76);
    tile[1] = tile[0];
}

extern s32 D_80092830;
void func_80031804(s32, void *);
void func_8008CE0C(void *arg0, s32 arg1) {
    s16 v[4];
    s32 depth;
    void *p;
    register s32 *vptr asm("$2");
    v[0] = *(u16 *)arg0 - *(u16 *)D_80092830;
    v[1] = *(u16 *)((u8 *)arg0 + 2) - *(u16 *)(D_80092830 + 2);
    v[2] = *(u16 *)((u8 *)arg0 + 4) - *(u16 *)(D_80092830 + 4);
    /* Finish the vector stores before allocating the GTE address in v0. */
    __asm__ volatile("" : : : "$2", "memory");
    vptr = (s32 *)v;
    __asm__ volatile("lwc2 $0,0(%0)\n\tlwc2 $1,4(%0)\n\tnop\n\tnop\n\t.word 0x4A180001" : : "r"(vptr) : "memory");
    p = (u8 *)arg0 + (D_800928A0 * 16 + 16);
    vptr = (s32 *)((u8 *)p + 8);
    __asm__ volatile("swc2 $14,0(%0)" : : "r"(vptr) : "memory");
    vptr = &depth;
    __asm__ volatile("mfc2 $12,$19\n\tnop\n\tsra $12,$12,2\n\tsw $12,0(%0)" : : "r"(vptr) : "$12", "memory");
    func_80031804(arg1 + (depth >> 2) * 4, p);
}


void func_8008CED4(void) {

}

/* GPU TILE_1 primitive (libgpu TILE_1 layout): tag, r0 g0 b0 code, x0 y0. */
typedef struct {
    u32 tag;
    u8 r0;
    u8 g0;
    u8 b0;
    u8 code;
    s16 x0;
    s16 y0;
} BattlingTile1;

void func_8008CEDC(void *arg0, void *arg1) {
    BattlingTile1 *tile;

    tile = (BattlingTile1 *)((u8 *)arg0 + 0x10);
    *((u8 *)tile + 3) = 2;
    tile->code = 0x6A;
    tile->r0 = *((u8 *)arg1 + 0x74);
    tile->g0 = *((u8 *)arg1 + 0x75);
    tile->b0 = *((u8 *)arg1 + 0x76);
    tile[1] = tile[0];
}

long RotTransPers();
void func_80031870(u32 *pPrim, u32 nextAddr);

void func_8008CF30(void *arg0, u32 *ot) {
    s32 flag;
    u8 *prim;

    prim = (u8 *)arg0 + (D_800928A0 * 0xC + 0x10);
    func_80031870(ot + (RotTransPers(arg0, prim + 8, &flag, &flag) >> 2), (u32)prim);
}

/* 8-byte, 2-byte-aligned vector (SVECTOR layout): copies of it compile to
 * lwl/lwr + swl/swr pairs. Shared by func_8008C7C0 / func_8008C9B8 / func_8008CC2C / func_8008CCB0 / func_8008CF9C. */
#ifndef BATTLING_SVEC_DEFINED
#define BATTLING_SVEC_DEFINED
typedef struct {
    s16 vx;
    s16 vy;
    s16 vz;
    s16 pad;
} BattlingSVec;
#endif

void func_8008CF9C(void *arg0, BattlingSVec *out) {
    *out = *(BattlingSVec *)((u8 *)arg0 + 0x1C);
}

void ApplyRotMatrix(s16 *, s32 *);
void func_8008CFC4(void *arg0, void *arg1) {
    s16 v[4];
    s32 r[3];
    s32 t;
    v[0] = rand() % *(s16 *)((u8 *)arg0 + 0x24) - *(u16 *)((u8 *)arg0 + 0x2C);
    v[1] = rand() % *(s16 *)((u8 *)arg0 + 0x26) - *(u16 *)((u8 *)arg0 + 0x2E);
    v[2] = rand() % *(s16 *)((u8 *)arg0 + 0x28) - *(u16 *)((u8 *)arg0 + 0x30);
    ApplyRotMatrix(v, r);
    *(s16 *)arg1 = *(u16 *)((u8 *)arg0 + 0x1C) + r[0];
    *(s16 *)((u8 *)arg1 + 2) = *(u16 *)((u8 *)arg0 + 0x1E) + r[1];
    *(s16 *)((u8 *)arg1 + 4) = *(u16 *)((u8 *)arg0 + 0x20) + r[2];
}


void func_8008D0A4(void *arg0, void *arg1) {
    s32 temp_hi;
    s32 temp_hi_2;

    temp_hi = rand() % (s16) *(s16 *)((s8*)(arg0) + 0x24);
    *(s16 *)((s8*)(arg1) + 0) = (s16) ((*(u16 *)((s8*)(arg0) + 0x1C) + temp_hi) - *(u16 *)((s8*)(arg0) + 0x2C));
    temp_hi_2 = rand(temp_hi) % (s16) *(s16 *)((s8*)(arg0) + 0x26);
    *(s16 *)((s8*)(arg1) + 2) = (s16) ((*(u16 *)((s8*)(arg0) + 0x1E) + temp_hi_2) - *(u16 *)((s8*)(arg0) + 0x2E));
    *(s16 *)((s8*)(arg1) + 4) = (s16) ((*(u16 *)((s8*)(arg0) + 0x20) + (rand(temp_hi_2) % (s16) *(s16 *)((s8*)(arg0) + 0x28))) - *(u16 *)((s8*)(arg0) + 0x30));
}

void ApplyRotMatrix(s16 *, s32 *);
void func_8008D14C(void *arg0, void *arg1) {
    s16 v[4];
    s32 r[3];
    s32 t;
    t = rand() % *(s16 *)((u8 *)arg0 + 0x24);
    v[0] = t - *(u16 *)((u8 *)arg0 + 0x2C);
    v[1] = 0;
    v[2] = rand() % *(s16 *)((u8 *)arg0 + 0x28) - *(u16 *)((u8 *)arg0 + 0x30);
    ApplyRotMatrix(v, r);
    *(s16 *)arg1 = *(u16 *)((u8 *)arg0 + 0x1C) + r[0];
    *(s16 *)((u8 *)arg1 + 2) = *(u16 *)((u8 *)arg0 + 0x1E) + r[1];
    *(s16 *)((u8 *)arg1 + 4) = *(u16 *)((u8 *)arg0 + 0x20) + r[2];
}

s32 rcos(s32);
s32 rsin(s32);
void func_8008D208(void *arg0, void *arg1) {
    s16 v[4];
    s32 r[3];
    s32 angle, radius, t;
    angle = rand();
    radius = rand();
    t = rcos(angle) * (radius % *(s16 *)((u8 *)arg0 + 0x24));
    v[1] = 0;
    v[0] = t >> 13;
    v[2] = (rsin(angle) * (radius % *(s16 *)((u8 *)arg0 + 0x28))) >> 13;
    ApplyRotMatrix(v, r);
    *(s16 *)arg1 = *(u16 *)((u8 *)arg0 + 0x1C) + r[0];
    *(s16 *)((u8 *)arg1 + 2) = *(u16 *)((u8 *)arg0 + 0x1E) + r[1];
    *(s16 *)((u8 *)arg1 + 4) = *(u16 *)((u8 *)arg0 + 0x20) + r[2];
}

INCLUDE_ASM("asm/battling/nonmatchings/main", func_8008D304);
extern u8 D_80091C74[];
extern s32 D_80091CC4[];
extern s32 D_80091CDC;

void *func_8008D3F4(s32 type, s32 arg1) {
    u8 *obj;
    s32 *rec;

    HeapSetCurrentContentType(0x15);
    obj = HeapAlloc(0x7C, 0);
    *(u16 *)(obj + 0x24) = 0x1000;
    *(u16 *)(obj + 0x26) = 0x1000;
    *(u16 *)(obj + 0x28) = 0x1000;
    *(s16 *)(obj + 0x44) = 1;
    *(s16 *)(obj + 0x4E) = arg1;
    *(s16 *)(obj + 2) = 0;
    *(s16 *)(obj + 4) = 0;
    *(s16 *)(obj + 6) = 0;
    *(s16 *)(obj + 0x14) = 0;
    *(s16 *)(obj + 0x16) = 0;
    *(s16 *)(obj + 0x18) = 0;
    *(s16 *)(obj + 0x34) = 0;
    *(s16 *)(obj + 0x36) = 0;
    *(s16 *)(obj + 0x38) = 0;
    *(s16 *)(obj + 0x3C) = 0;
    *(s16 *)(obj + 0x3E) = 0;
    *(s16 *)(obj + 0x40) = 0;
    *(s16 *)(obj + 0) = 0;
    *(s16 *)(obj + 0x46) = 0;
    *(s16 *)(obj + 0x48) = 0x100;
    *(s16 *)(obj + 0x4A) = 0x100;
    *(s16 *)(obj + 0x2C) = (s16)*(u16 *)(obj + 0x24) / 2;
    *(s16 *)(obj + 0x2E) = (s16)*(u16 *)(obj + 0x26) / 2;
    *(s16 *)(obj + 0x30) = (s16)*(u16 *)(obj + 0x28) / 2;
    *(s32 *)(obj + 0x5C) = D_80091CC4[(s16)arg1];
    *(s32 *)(obj + 0x60) = D_80091CDC;
    *(s32 *)(obj + 0x10) = 0;
    *(s16 *)(obj + 0x66) = type;
    *(s16 *)(obj + 0x64) = 0;
    *(s16 *)(obj + 0x6A) = 100;
    obj[0x74] = 0xFF;
    obj[0x75] = 0xFF;
    obj[0x76] = 0xFF;
    *(s16 *)(obj + 0x68) = 0;
    rec = (s32 *)(D_80091C74 + *(s16 *)(obj + 0x66) * 16);
    *(s16 *)(obj + 0xC) = rec[3];
    *(s32 *)(obj + 0x50) = rec[1];
    *(s32 *)(obj + 0x54) = rec[2];
    *(s32 *)(obj + 0x58) = rec[0];
    return obj;
}

void func_8008D580(void *arg0) {
    s32 i;
    u8 *p;

    p = *(u8 **)((s8*)(arg0) + 0x10);
    for (i = 0; i < *(s16 *)((s8*)(arg0) + 0xA); i++) {
        *(s16 *)(p + 6) = 0;
        p += *(s16 *)((s8*)(arg0) + 0xC);
    }
}

/* The 8-byte `vars` area in retail's frame is unused stack. A top-tested
 * `for (i = 0; i < n; i++)` produces it too (a dead CCmode pseudo gets a
 * stack slot), but that form flips the s1/s2 allocation of arg0 and i, so
 * the slot is reproduced with an unused local instead. */
void func_8008D5C0(void *arg0, s32 arg1) {
    void (*init)(void *, void *);
    s32 i;
    u8 *p;
    s32 unused[2];

    if (*(void **)((u8 *)arg0 + 0x10) != NULL) {
        HeapDelayedFree(*(void **)((u8 *)arg0 + 0x10), 3);
    }
    *(s16 *)((u8 *)arg0 + 0xA) = arg1;
    HeapSetCurrentContentType(0x14);
    p = HeapAlloc(*(s16 *)((u8 *)arg0 + 0xC) * *(s16 *)((u8 *)arg0 + 0xA), 0);
    init = *(void (**)(void *, void *))((u8 *)arg0 + 0x58);
    i = 0;
    *(void **)((u8 *)arg0 + 0x10) = p;
    if (*(s16 *)((u8 *)arg0 + 0xA) > 0) {
        do {
            init(p, arg0);
            i++;
            *(s16 *)(p + 6) = 0;
            p += *(s16 *)((u8 *)arg0 + 0xC);
        } while (i < *(s16 *)((u8 *)arg0 + 0xA));
    }
}

#ifndef BATTLING_SVEC_DEFINED
#define BATTLING_SVEC_DEFINED
typedef struct {
    s16 vx;
    s16 vy;
    s16 vz;
    s16 pad;
} BattlingSVec;
#endif

/* libgte MATRIX viewed as 8 words: the by-value copy of arg1 below is a gcc block move. */
typedef struct { s32 w[8]; } BattlingEmitMtx;
void MulMatrix(void *, void *);
void SetRotMatrix(void *);

/* Particle emitter spawn: random rotated directions for free slots, then their init/update callbacks. */
void func_8008D680(void *arg0, void *arg1, s32 arg2) {
    BattlingSVec v;
    BattlingSVec w;
    BattlingEmitMtx a;
    BattlingEmitMtx b;
    BattlingEmitMtx c;
    void (*update)(void *);
    void (*init)(void *, void *);
    s32 i;
    s32 n;
    u8 *elem;
    s32 ang;
    s32 rnd;
    s32 cs;

    update = *(void (**)(void *))((u8 *)arg0 + 0x50);
    init = *(void (**)(void *, void *))((u8 *)arg0 + 0x5C);
    b = *(BattlingEmitMtx *)arg1;
    *(s16 *)((u8 *)arg0 + 0x1C) = *(u16 *)((u8 *)arg0 + 0x14) + *(s32 *)((u8 *)arg1 + 0x14);
    *(s16 *)((u8 *)arg0 + 0x1E) = *(u16 *)((u8 *)arg0 + 0x16) + *(s32 *)((u8 *)arg1 + 0x18);
    *(s16 *)((u8 *)arg0 + 0x20) = *(u16 *)((u8 *)arg0 + 0x18) + *(s32 *)((u8 *)arg1 + 0x1C);
    RotMatrix((u8 *)arg0 + 0x34, &a);
    MulMatrix(&b, &a);
    RotMatrix((u8 *)arg0 + 0x3C, &c);
    MulMatrix(&c, &b);
    SetRotMatrix(&c);
    n = arg2;
    elem = *(u8 **)((u8 *)arg0 + 0x10);
    for (i = 0; i < *(s16 *)((u8 *)arg0 + 0xA); i++) {
        if (*(s16 *)(elem + 6) == 0) {
            if (--n == -1) {
                break;
            }
            ang = rand() % *(s16 *)((u8 *)arg0 + 0x44);
            rnd = rand();
            v.vy = -rsin(ang);
            cs = rcos(ang);
            v.vx = (rcos(rnd) * cs) >> 12;
            v.vz = (rsin(rnd) * cs) >> 12;
            rnd = *(s16 *)((u8 *)arg0 + 0x48) + rand() % *(s16 *)((u8 *)arg0 + 0x4A);
            /* gte_ldv0 */
            __asm__ volatile("lwc2 $0,0(%0)\n\tlwc2 $1,4(%0)" : : "r"(&v) : "memory");
            /* gte_rtv0 */
            __asm__ volatile("nop\n\tnop\n\t.word 0x4A486012");
            /* gte_stsv */
            __asm__ volatile("mfc2 $12,$9\n\tmfc2 $13,$10\n\tmfc2 $14,$11\n\tsh $12,0(%0)\n\tsh $13,2(%0)\n\tsh $14,4(%0)" : : "r"(&w) : "$12", "$13", "$14", "memory");
            /* gte_lddp */
            __asm__ volatile("mtc2 %0,$8" : : "r"(rnd));
            /* gte_ldsv */
            __asm__ volatile("lhu $12,0(%0)\n\tlhu $13,2(%0)\n\tlhu $14,4(%0)\n\tmtc2 $12,$9\n\tmtc2 $13,$10\n\tmtc2 $14,$11" : : "r"(&w) : "$12", "$13", "$14", "memory");
            /* gte_gpf12 */
            __asm__ volatile("nop\n\tnop\n\t.word 0x4B98003D");
            /* gte_stsv */
            __asm__ volatile("mfc2 $12,$9\n\tmfc2 $13,$10\n\tmfc2 $14,$11\n\tsh $12,0(%0)\n\tsh $13,2(%0)\n\tsh $14,4(%0)" : : "r"(elem + 8) : "$12", "$13", "$14", "memory");
        }
        elem += *(s16 *)((u8 *)arg0 + 0xC);
    }
    SetRotMatrix(&b);
    n = arg2;
    elem = *(u8 **)((u8 *)arg0 + 0x10);
    for (i = 0; i < *(s16 *)((u8 *)arg0 + 0xA); i++) {
        if (*(s16 *)(elem + 6) == 0) {
            if (--n == -1) {
                break;
            }
            init(arg0, elem);
            *(s16 *)(elem + 6) = *(u16 *)((u8 *)arg0 + 0x6A);
            update(elem);
        }
        elem += *(s16 *)((u8 *)arg0 + 0xC);
    }
}

s32 func_80082488(void *, s32);

void func_8008D980(void *arg0) {
    s32 y;

    if (*(s16 *)((u8 *)arg0 + 0xA) > 0) {
        y = *(s16 *)((u8 *)arg0 + 2);
        if (func_80082488(arg0, 0) < y) {
            *(s16 *)((u8 *)arg0 + 0xA) = -*(s16 *)((u8 *)arg0 + 0xA) / 2;
        }
    }
}

void func_8008D9F0(void *arg0) {
    s32 v;

    if (*(s16 *)((s8*)(arg0) + 2) > 0) {
        v = -*(s16 *)((s8*)(arg0) + 0xA) / 2;
        *(s16 *)((s8*)(arg0) + 0xA) = v;
        v = (s16)v;
        if (v < 0) {
            v = -v;
        }
        if (v < 8) {
            *(s16 *)((s8*)(arg0) + 6) = 0;
        }
    }
}

extern s32 D_8009282C;
extern s32 D_80092830;
extern s32 D_80096FA8;
extern s32 D_80096FAC;
extern s32 D_80096FB0;

void func_8008DA48(void *arg0, s32 arg1) {
    void (*update)(void *, s32);
    s32 i;
    s32 v;
    s32 unused[10];
    u8 *p;


    D_8009282C = 0x1F800000;
    D_80092830 = 0x1F800030;
    *(s16 *)0x1F800030 = D_80096FA8;
    *(s16 *)0x1F800032 = D_80096FAC;
    *(s16 *)0x1F800034 = D_80096FB0;
    p = *(u8 **)((u8 *)arg0 + 0x10);
    update = *(void (**)(void *, s32))((u8 *)arg0 + 0x54);
    for (i = 0; i < *(s16 *)((u8 *)arg0 + 0xA); i++) {
        if (*(s16 *)(p + 6) != 0) {
            *(s16 *)(p + 6) -= 1;
            *(u16 *)(p + 0xA) += *(u16 *)((u8 *)arg0 + 0);
            *(u16 *)(p + 0) += *(u16 *)(p + 8);
            *(u16 *)(p + 2) += *(u16 *)(p + 0xA);
            *(u16 *)(p + 4) += *(u16 *)(p + 0xC);
            if (*(s16 *)(p + 2) > 0) {
                v = -*(s16 *)(p + 0xA) * 2 / 3;
                *(s16 *)(p + 0xA) = v;
                v = (s16)v;
                if (v < 0) {
                    v = -v;
                }
                if (v < 4) {
                    *(s16 *)(p + 6) = 0;
                }
            }
            update(p, arg1);
        }
        p += *(s16 *)((u8 *)arg0 + 0xC);
    }
}

INCLUDE_ASM("asm/battling/nonmatchings/main", func_8008DBC0);
extern void *D_80092834;
void *func_8008D3F4(s32, s32);
void func_8008D5C0(void *, s32);

void func_8008DC28(void) {
    s32 state;
    void *sprite;

    sprite = func_8008D3F4(1, 0);
    *(u8 *)((u8 *)sprite + 0x74) = 0xFF;
    *(u8 *)((u8 *)sprite + 0x75) = 0xA0;
    *(s8 *)((u8 *)sprite + 0x76) = 0x70;
    func_8008D5C0(sprite, 0x100);
    state = 4;
    *(s16 *)sprite = state;
    *(s16 *)((u8 *)sprite + 0x44) = 0x60;
    *(s16 *)((u8 *)sprite + 0x4A) = 0x60;
    *(s16 *)((u8 *)sprite + 0x48) = state;
    *(s16 *)((u8 *)sprite + 0x68) = 0;
    *(s16 *)((u8 *)sprite + 0x6A) = 0x20;
    D_80092834 = sprite;
}


void func_8008DCA8(s32 arg0) {
    D_80092838 = arg0;
}

void func_80048E94(void *, void *);
void func_8008D680(void *, void *, s32);
void func_8008DBC0(void *, s32, void *);
void func_8008DA48__3arg(void *, s32, void *) asm("func_8008DA48");

/* Particle emitter sprite (func_8008D3F4 object): origin at 0x14, two angle
 * triples at 0x34 and 0x3C (the RotMatrix inputs of func_8008D680). */
typedef struct {
    u8 pad0[0x14];
    s16 x;
    s16 y;
    s16 z;
    u8 pad1[0x34 - 0x1A];
    s16 ax0;
    s16 ay0;
    s16 az0;
    s16 pad3A;
    s16 ax1;
    s16 ay1;
    s16 az1;
} BattlingEmitter;

/* MATRIX layout (libgte): 3x3 s16 rotation, 2 bytes padding, s32 translation. */
typedef struct {
    s16 m[9];
    s16 pad;
    s32 t[3];
} BattlingMatrix;

void func_8008DCB8(s32 arg0, void *arg1, void *arg2, s32 *arg3) {
    BattlingMatrix mat;
    BattlingMatrix base;
    BattlingEmitter *sprite;

    sprite = D_80092834;
    if (D_80092838 >= 0x10) {
        func_8008DBC0(arg1, 0x27, &base);
        func_80048E94(&base, &mat);
        mat.t[2] = 0;
        mat.t[1] = 0;
        mat.t[0] = 0;
        sprite->x = arg3[0];
        sprite->y = arg3[1];
        sprite->z = arg3[2];
        sprite->ax1 = 0;
        sprite->ay1 = 0;
        sprite->az1 = 0;
        sprite->ax0 = 0;
        sprite->ay0 = 0;
        sprite->az0 = 0x800;
        func_8008D680(sprite, &mat, D_80092838 >> 4);
        D_80092838 -= 4;
    }
    /* gte_SetTransMatrix, gte_SetRotMatrix */
    __asm__ volatile (
        "lw $12, 20(%0)\n\t"
        "lw $13, 24(%0)\n\t"
        "ctc2 $12, $5\n\t"
        "lw $14, 28(%0)\n\t"
        "ctc2 $13, $6\n\t"
        "ctc2 $14, $7"
        : : "r"(arg2) : "$12", "$13", "$14");
    __asm__ volatile (
        "lw $12, 0(%0)\n\t"
        "lw $13, 4(%0)\n\t"
        "ctc2 $12, $0\n\t"
        "ctc2 $13, $1\n\t"
        "lw $12, 8(%0)\n\t"
        "lw $13, 12(%0)\n\t"
        "lw $14, 16(%0)\n\t"
        "ctc2 $12, $2\n\t"
        "ctc2 $13, $3\n\t"
        "ctc2 $14, $4"
        : : "r"(arg2) : "$12", "$13", "$14");
    func_8008DA48__3arg(sprite, arg0, arg2);
}

/* GTE rtv0 (MVMVA 1,0,0,3,0) then gpf12 scale by IR0; 2 of 3 outputs stored. */
void func_8008DDFC(void *arg0, void *arg1, s32 arg2) {
    register s32 t0 asm("$8");
    register s32 t1 asm("$9");
    register s32 t2 asm("$10");

    __asm__ volatile ("lwc2 $0, 0(%0)" : : "r"(arg0));
    __asm__ volatile ("lwc2 $1, 4(%0)" : : "r"(arg0));
    __asm__ volatile ("mtc2 %0, $8" : : "r"(arg2));
    __asm__ volatile ("nop\n\tnop\n\t.word 0x4A486012"); /* gte_rtv0 */
    __asm__ volatile ("mfc2 %0, $25" : "=r"(t0));
    __asm__ volatile ("mfc2 %0, $26" : "=r"(t1));
    __asm__ volatile ("mfc2 %0, $27" : "=r"(t2));
    __asm__ volatile ("mtc2 %0, $9" : : "r"(t0));
    __asm__ volatile ("mtc2 %0, $10" : : "r"(t1));
    __asm__ volatile ("mtc2 %0, $11" : : "r"(t2));
    __asm__ volatile ("nop\n\tnop\n\t.word 0x4B98003D"); /* gte_gpf12 */
    __asm__ volatile ("mfc2 %0, $9" : "=r"(t0));
    __asm__ volatile ("mfc2 %0, $10" : "=r"(t1));
    __asm__ volatile ("mfc2 %0, $11" : "=r"(t2));
    *(s16 *)((s8 *)arg1 + 0) = t0;
    *(s16 *)((s8 *)arg1 + 2) = t1;
    *(s16 *)((s8 *)arg1 + 4) = t2;
}

INCLUDE_ASM("asm/battling/nonmatchings/main", func_8008DE54);

void func_8008DF30(void) {
    D_80092844 = 0;
    D_8009283C = 0;
    D_80092840 = 0;
}

s32 LoadImage();
/* RECT layout (libgpu): x, y, w, h. */
typedef struct {
    s16 x;
    s16 y;
    s16 w;
    s16 h;
} BattlingRect;
extern u16 D_80091CE0[];

void func_8008DF50(void) {
    BattlingRect rect;
    u16 *a;
    u16 *b;
    u16 *clut;
    s32 i;

    if (D_80092844 == 0) {
        D_80092844 = HeapAlloc(0x1500, 1);
        D_8009283C = HeapAlloc(0x2BC0, 1);
        D_80092840 = HeapAlloc(0x2BC0, 1);
    }
    i = 0x1570;
    a = (u16 *)D_8009283C + 0x1570;
    b = (u16 *)D_80092840 + 0x1570;
    do {
        *b = 0;
        *a = 0;
        a--;
        i--;
        b--;
    } while (i != -1);
    for (i = 0; i < 0x1500; i++) {
        ((u8 *)D_80092844)[i] = 0;
    }
    for (i = 0, clut = D_80091CE0; i < 0x100; i++) {
        *clut |= 0x8000;
        clut++;
    }
    rect.y = 0x1FD;
    rect.w = 0xFF;
    rect.x = 0;
    rect.h = 1;
    LoadImage(&rect, D_80091CE0);
}


void func_8008E064(void) {
    if (D_80092844 != 0) {
        HeapDelayedFree(D_80092844, 2);
        HeapDelayedFree(D_8009283C, 2);
        HeapDelayedFree(D_80092840, 2);
        D_80092844 = 0;
        D_8009283C = 0;
        D_80092840 = 0;
    }
}

void func_8008E0C8(void) {
    s32 n;
    u32 *src;
    u32 *dst;
    u32 v;
    u8 *out;

    n = 0xA7F;
    out = (u8 *)D_80092844;
    dst = (u32 *)D_8009283C;
    src = (u32 *)D_80092840;
    do {
        v = *src++;
        n--;
        *dst++ = v;
        *out++ = v;
        *out++ = v >> 16;
    } while (n != -1);
}
void func_8008E120(void) {
    s16 *cur;
    s16 *up;
    s16 *right;
    s16 *left;
    s16 *down_right;
    s16 *down_left;
    s16 *row;
    s16 val;
    s16 out;
    s32 next;
    s32 i;
    s32 x;
    s32 y;
    s32 off;
    s32 v;
    s32 sum;

    if (D_80092844 != 0) {
        val = 0;
        i = 0;
        next = D_80092840;
        row = (s16 *)(next + 0x2920);
        do {
            switch (rand() & 3) {
            case 0:
                val = 0x180;
                break;
            case 1:
                val = 0;
                break;
            default:
                out = val;
                goto store;
            }
            out = val;
store:
            row[i + 0x70] = out;
            row[i] = out;
            i += 1;
        } while (i < 0x70);
        cur = (s16 *)D_8009283C;
        up = cur - 0x70;
        right = cur + 1;
        left = cur - 1;
        down_right = cur + 0x71;
        down_left = cur + 0x6F;
        y = 0x2F;
        do {
            i = y * 0x70 + 1;
            off = (y - 1) * 0xE0;
            x = 1;
            do {
                sum = up[i] + right[i] + left[i] + down_right[i] + down_left[i];
                i++;
                v = sum / 5;
                if (v >= 4) {
                    v -= 3;
                }
                *(s16 *)(next + off + x * 2) = v;
                x++;
            } while (x < 0x70);
            y -= 1;
        } while (y >= 2);
        func_8008E0C8();
    }
}

INCLUDE_ASM("asm/battling/nonmatchings/main", func_8008E2B8);
s16 GetClut(s32, s32);
void SetDrawMode(void *, s32, s32, s32, s32);
extern u8 D_80096D90[];
extern u8 D_80096DE0[];
extern u8 D_80096E00[];

/* Effect overlay: upload the buffer, queue a tinted POLY_FT4 from it, an optional TILE, then a DR_MODE. */
void func_8008E3CC(s32 arg0, s32 arg1, s32 arg2) {
    BattlingRect rect;
    s32 c;
    void *tile;
    void *prim;

    prim = (D_800928A0 * 0x28) + D_80096D90;
    *(s8 *)((s8*)(prim) + 3) = 9;
    c = (arg1 * 2) / 3;
    *(s32 *)((s8*)(prim) + 4) = (s32) (c | (c << 8) | (c << 0x10) | 0x2C000000);
    if (c == 0x80) {
        *(u8 *)((s8*)(prim) + 7) = *(u8 *)((s8*)(prim) + 7) | 1;
    } else {
        *(u8 *)((s8*)(prim) + 7) = *(u8 *)((s8*)(prim) + 7) & 0xFE;
    }
    *(s32 *)((s8*)(prim) + 0x10) = 0x140;
    *(s32 *)((s8*)(prim) + 0x18) = 0xDA0000;
    *(s16 *)((s8*)(prim) + 0x14) = 0x6F;
    *(s16 *)((s8*)(prim) + 0x1C) = 0x2A00;
    *(s32 *)((s8*)(prim) + 8) = 0;
    *(s32 *)((s8*)(prim) + 0x20) = 0xDA0140;
    *(s16 *)((s8*)(prim) + 0xC) = 0;
    *(s16 *)((s8*)(prim) + 0x24) = 0x2A6F;
    *(u8 *)((s8*)(prim) + 7) = (u8) (*(u8 *)((s8*)(prim) + 7) | 2);
    *(s16 *)((s8*)(prim) + 0x16) = GetTPage(1, 1, 0x140, 0x100);
    *(s16 *)((s8*)(prim) + 0xE) = GetClut(0, 0x1FD);
    AddPrim((void *)arg0, prim);
    rect.x = 0x140;
    rect.y = 0x100;
    rect.w = 0x38;
    rect.h = 0x2B;
    LoadImage(&rect, D_80092844);
    if (arg2 != 0) {
        tile = (D_800928A0 * 0x10) + D_80096DE0;
        c = arg1 * 2;
        if (arg1 >= 0x81) {
            *(s32 *)((s8*)(tile) + 4) = (s32) (c | (arg1 << 9) | (arg1 << 0x11) | 0x60000000);
            *(s8 *)((s8*)(tile) + 3) = 3;
            *(s32 *)((s8*)(tile) + 8) = 0;
            *(s32 *)((s8*)(tile) + 0xC) = 0xDA0140;
            *(u8 *)((s8*)(tile) + 7) = (u8) (*(u8 *)((s8*)(tile) + 7) | 2);
            AddPrim((void *)arg0, tile);
        }
    }
    SetDrawMode((D_800928A0 * 0xC) + D_80096E00, 0, 1, GetTPage(0, 2, 0, 0) & 0xFFFF, 0);
    AddPrim((void *)arg0, (D_800928A0 * 0xC) + D_80096E00);
}

void func_80039FF8();
/* D_80096EA0: 4 x 0x24-byte sound slots (+1 active, +2 voice, +3 owner,
 * +4 u16 age, +8 sound id, +0xC voice mask, +0x10 VECTOR pos, +0x20 pos src). */
extern u8 D_80096EA0[];

void func_8008E620(void) {
    s32 mask;
    u32 i;
    u8 *slot;

    mask = 0x300;
    func_80039FF8();
    for (i = 0; i < 4; i++) {
        slot = D_80096EA0 + i * 0x24;
        *(s32 *)(slot + 0xC) = mask;
        mask <<= 2;
        slot[1] = 0;
        *(s16 *)(slot + 4) = 0;
        slot[2] = i * 2;
    }
}

extern u8 D_80096EA0[];

void func_8008E67C(void) {
    if (*(u16 *)(D_80096EA0 + 4) != 0xFFFF) {
        *(u16 *)(D_80096EA0 + 4) += 1;
    }
    if (*(u16 *)(D_80096EA0 + 0x28) != 0xFFFF) {
        *(u16 *)(D_80096EA0 + 0x28) += 1;
    }
    if (*(u16 *)(D_80096EA0 + 0x4C) != 0xFFFF) {
        *(u16 *)(D_80096EA0 + 0x4C) += 1;
    }
    if (*(u16 *)(D_80096EA0 + 0x70) != 0xFFFF) {
        *(u16 *)(D_80096EA0 + 0x70) += 1;
    }
}


void func_8008E6F8(void *arg0) {
    s32 *var_v0;
    u8 temp_v1;

    temp_v1 = *(u8 *)((s8*)(arg0) + 0x909);
    switch (temp_v1) {                              /* irregular */
    case 9:
        var_v0 = &D_80091FA0;
        break;
    case 29:
        var_v0 = &D_80091F60;
        break;
    case 27:
        var_v0 = &D_80091F80;
        break;
    case 36:
        var_v0 = &D_80091F70;
        break;
    default:
        var_v0 = &D_80091F90;
        break;
    }
    *(s32 **)((s8*)(arg0) + 0x1664) = var_v0;
}

void func_80039F9C(s32, s32, s32, s32);
extern u8 D_80096F0C[];

void func_8008E78C(s32 arg0, s32 arg1, void *a2_, s32 a3_) {
    register void *arg2 asm("$18");
    register s32 arg3 asm("$20");
    u8 *best;
    u8 *p;
    u8 *q;
    s32 oldest;
    s32 i;
    u16 age;

    arg2 = a2_;
    arg3 = a3_;
    q = 0;
    oldest = 0;
    best = D_80096F0C;
    i = 0;
    p = D_80096F0C - 0x6C;
loop:
    if (p[1] == 0 || (arg1 == 0 && p[0] == 0)) {
        best = p;
    } else {
        age = *(u16 *)(p + 4);
        if (oldest < age) {
            oldest = age;
            best = p;
        }
        i++;
        p += 0x24;
        if (i < 4) {
            goto loop;
        }
    }
    func_8008E67C();
    q = best;
    q[0] = arg1;
    *(s32 *)(q + 8) = arg0;
    q[1] = 1;
    q[3] = arg3;
    *(void **)(q + 0x20) = arg2;
    if (arg2 != 0) {
        *(BattlingVec *)(q + 0x10) = *(BattlingVec *)arg2;
    }
    *(s16 *)(q + 4) = 0;
    if (arg1 == 0) {
        func_80039F9C(*(s32 *)(q + 8), q[2], 0x7F, 0x40);
    }
}

extern s32 D_80096FA8__vec[] asm("D_80096FA8");
void func_8003A55C(s32, s32);
void func_8003A344(s32, s32);

/* Positional sound update: GTE-project each slot against camera D_80096FA8, pan from screen X, volume from depth. */
void func_8008E8B0(void) {
    s32 sz;
    u16 v[3];
    s16 sxy[2];
    s32 i;
    u8 *slot;
    s32 sx, vol, pan;

    for (i = 0; i < 4; i++) {
        slot = D_80096EA0 + i * 0x24;
        if (slot[1] != 0 && slot[0] != 0) {
            if (slot[0] == 1) {
                v[0] = *(s32 *)(slot + 0x10);
                v[1] = *(s32 *)(slot + 0x14);
                v[2] = *(s32 *)(slot + 0x18);
            } else {
                v[0] = **(s32 **)(slot + 0x20);
                v[1] = (*(s32 **)(slot + 0x20))[1];
                v[2] = (*(s32 **)(slot + 0x20))[2];
            }
            v[0] -= D_80096FA8__vec[0];
            v[1] -= D_80096FA8__vec[1];
            v[2] -= D_80096FA8__vec[2];
            __asm__ volatile("lwc2 $0,0(%0)\n\tlwc2 $1,4(%0)\n\tnop\n\tnop\n\t.word 0x4A180001" : : "r"(v) : "memory");
            __asm__ volatile("swc2 $14,0(%0)" : : "r"(sxy) : "memory");
            __asm__ volatile("swc2 $19,0(%0)" : : "r"(&sz) : "memory");
            sx = sxy[0];
            if (sx < 0) {
                sx = 0;
            }
            if (sx > 0x140) {
                sx = 0x140;
            }
            vol = (0x3000 - sz) * 0x7F / 0x3000;
            if (vol < 0x28) {
                vol = 0x28;
            }
            if (vol > 0x7F) {
                vol = 0x7F;
            }
            pan = sx * 0x7F / 0x140;
            if (*(u16 *)(slot + 4) != 0) {
                func_8003A55C(slot[2], pan);
                func_8003A344(slot[2], vol);
            } else {
                func_80039F9C(*(s32 *)(slot + 8), slot[2], (s16)vol, (s16)pan);
            }
        }
    }
    func_8008E67C();
}

s32 func_8003A5D0(s32);
extern u8 D_80096EA0[];

void func_8008EADC(void) {
    s32 i;
    u8 *slot;

    for (i = 0; i < 4; i++) {
        slot = D_80096EA0 + i * 0x24;
        if (!(func_8003A5D0(*(s32 *)(slot + 8)) & *(s32 *)(slot + 0xC))) {
            slot[1] = 0;
        }
    }
    func_8008E67C();
}

/* D_80059488 is the VSync frame counter. Retail schedules its load right
 * after `lui $a0` (ahead of the addu), which only a volatile-declared extern
 * reproduces; a `*(volatile s32 *)&D_80059488` cast at the site does not. */
extern volatile s32 D_80059488;

void func_8008EB4C(s32 arg0) {
    if (arg0 != 0) {
        func_8008E78C(arg0 + 0x60000, 0, 0, D_80059488);
    }
}

void func_8008EB88(void *arg0, s32 arg1, s32 arg2, s32 arg3) {
    if (arg1 != 0) {
        func_8008E78C(arg1 + 0x60000, arg3, arg2,
                      (arg1 & 0x7F) | ((*(u32 *)((u8 *)arg0 + 0xD0) >> 0x14) & 0x80));
    }
}
INCLUDE_ASM("asm/battling/nonmatchings/main", func_8008EBD0);
void func_8003A20C(s32);
extern u8 D_80096EA0[];

void func_8008ECEC(s32 arg0) {
    s32 i;
    u8 *slot;

    for (i = 0; i < 4; i++) {
        slot = D_80096EA0 + i * 0x24;
        if (slot[1] != 0 && slot[3] == (arg0 & 0xFF)) {
            func_8003A20C(slot[2]);
            slot[1] = 0;
        }
    }
}

extern u8 D_80091EE0[];
void func_8008ECEC(s32);

void func_8008ED6C(void *arg0, s32 arg1) {
    s32 temp_s2;
    u8 temp_v0;
    u8 temp_v0_2;
    u8 temp_v1;

    temp_v1 = *(u8 *)(s8 *)(*(s32 *)((s8 *)arg0 + 0x1664) + arg1);
    if (temp_v1 != 0xFF) {
        temp_v0 = D_80091EE0[temp_v1 * 2];
        if (temp_v0 != 0) {
            func_8008ECEC((((u32) *(u32 *)((s8 *)arg0 + 0xD0) >> 0x14) & 0x80) | (temp_v0 & 0x7F));
        }
        temp_v0_2 = D_80091EE0[temp_v1 * 2 + 1];
        if (temp_v0_2 != 0) {
            func_8008ECEC((((u32) *(volatile u32 *)((s8 *)arg0 + 0xD0) >> 0x14) & 0x80) | (temp_v0_2 & 0x7F));
        }
    }
}

s32 func_8008B650(s32, s16, s32, s32);

void func_8008EE1C(void *arg0, s16 arg1, s16 arg2) {
    s32 dec;
    s32 inc;
    s32 flag;
    s32 i;
    s32 unused[4];

    dec = *(s32 *)((u8 *)arg0 + 0x9C);
    inc = *(s32 *)((u8 *)arg0 + 0x98);
    if (*(s32 *)((u8 *)arg0 + 0xD0) & 0x100) {
        flag = 0;
        dec = dec * 2 / 3;
    } else {
        flag = 1;
    }
    for (i = 0; i < 2; i++) {
        if (arg2 != 0 && flag != 0) {
            *(s32 *)((u8 *)arg0 + 0x48) += inc;
            if (*(s32 *)((u8 *)arg0 + 0x48) > arg2) {
                *(s32 *)((u8 *)arg0 + 0x48) = arg2;
            }
        } else {
            *(s32 *)((u8 *)arg0 + 0x48) -= dec;
            if (*(s32 *)((u8 *)arg0 + 0x48) < 0) {
                *(s32 *)((u8 *)arg0 + 0x48) = 0;
            }
        }
    }
    *(s32 *)((u8 *)arg0 + 0x58) = func_8008B650(*(s32 *)((u8 *)arg0 + 0x58), arg1, 0x40, flag);
    *(s16 *)((u8 *)arg0 + 0xCE) = 0;
}


void func_8008EF00(s32 arg0, void *arg1) {
    func_800767C8();
    *(s16 *)((s8*)(arg1) + 4) = 0x3C;
}

void func_8008EF30(s32 arg0, void *arg1, s32 arg2) {
    s16 duration;

    func_8008FE80(arg0);
    duration = 0x1E;
    if (arg2 == 0) {
        duration = 0xA;
    }
    *(s16 *)((u8 *)arg1 + 4) = duration;
}

#ifndef XENO_PC_PORT
void func_8008EF74(s32 arg0, void *arg1) {
    func_8007639C(arg0, 3);
    *(s16 *)((s8*)(arg1) + 4) = 0x3C;
}
#endif

typedef struct {
    u32 pad0 : 13;
    u32 flag13 : 1;
    u32 pad14 : 18;
} BattlingAiFlags;

void func_8008EFA8(void *arg0, void *arg1) {
    BattlingAiFlags *flags;

    flags = (BattlingAiFlags *)((u8 *)arg1 + 0x2C);
    flags->flag13 ^= 1;
    if (flags->flag13) {
        *(s32 *)((u8 *)arg0 + 0xD0) |= 2;
        *(s16 *)((u8 *)arg1 + 4) = 0x3C;
    } else {
        *(s32 *)((u8 *)arg0 + 0xD0) &= ~2;
        *(s32 *)((u8 *)arg0 + 0xD0) &= ~0x38;
        *(s16 *)((u8 *)arg1 + 4) = 0x1E;
    }
}

void func_8008F014(s32 arg0, void *arg1) {
    func_8007639C(arg0, 4);
    func_8007639C(arg0, 3);
    *(s16 *)((s8 *)arg1 + 4) = 0x3C;
}


#ifndef XENO_PC_PORT
void func_8008F060(s32 arg0, void *arg1) {
    func_8007639C(arg0, 4);
    *(s16 *)((s8*)(arg1) + 4) = 0x3C;
}
#endif

void func_8008F094(s32 arg0, void *arg1) {
    u16 temp_v0;
    s32 temp_v0_2;

    if (D_8009284C >= 0x801) {
        *(u16 *)((s8*)(arg1) + 4) = 1U;
        *(s16 *)((s8*)(arg1) + 0xC) = 0;
        return;
    }
    temp_v0 = *(u16 *)((s8*)(arg1) + 4) - 1;
    *(u16 *)((s8*)(arg1) + 4) = temp_v0;
    if ((s16) temp_v0 == -1) {
        *(s16 *)((s8*)(arg1) + 0xA) = (s16) (rand() % 1536 + 0x500);
        temp_v0_2 = rand();
        *(s16 *)((s8*)(arg1) + 0xC) = 0xFF;
        *(u16 *)((s8*)(arg1) + 4) = (u16) (temp_v0_2 % 50 + 0xA);
    }
}

void func_8008F17C(s32 arg0, void *arg1) {
    u16 temp_v0;
    s32 temp_v0_2;

    if (D_8009284C < 0x100) {
        *(u16 *)((s8*)(arg1) + 4) = 1U;
        *(s16 *)((s8*)(arg1) + 0xC) = 0;
        return;
    }
    temp_v0 = *(u16 *)((s8*)(arg1) + 4) - 1;
    *(u16 *)((s8*)(arg1) + 4) = temp_v0;
    if ((s16) temp_v0 == -1) {
        *(s16 *)((s8*)(arg1) + 0xA) = (s16) (rand() % 1536 - 0x300);
        temp_v0_2 = rand();
        *(s16 *)((s8*)(arg1) + 0xC) = 0xFF;
        *(u16 *)((s8*)(arg1) + 4) = (u16) (temp_v0_2 % 120 + 0xA);
    }
}


void func_8008F260(s32 arg0, void *arg1, s8 arg2) {
    *(s8 *)((s8*)(arg1) + 0xF) = arg2;
    func_80090E10();
}

extern u8 D_80092848;
extern s8 D_80099D9E;
extern u8 D_80099DA2;

void func_8008F280(void *arg0) {
    u16 temp_v0;
    void *temp_s1;

    temp_s1 = *(void **)((s8*)(arg0) + 0x15FC);
    D_80099D9E = 1;
    if (D_80092848 != D_80099DA2) {
        *(u16 *)((s8*)(temp_s1) + 4) = 0U;
        *(s16 *)((s8*)(temp_s1) + 0xC) = 0;
        *(s32 *)((s8*)(arg0) + 0xD0) &= ~2;
        *(s32 *)((s8*)(arg0) + 0x48) = 0;
        *(s32 *)((s8*)(arg0) + 0xD0) &= ~0x38;
        *(s32 *)((s8*)(temp_s1) + 0x2C) &= ~0x2000;
        D_80092848 = D_80099DA2;
        *(s16 *)((s8*)(arg0) + 0xCE) = (s16) (((u16 *)arg0)[0x66] + 0x800);
    }
    if (*(s32 *)((s8*)(temp_s1) + 0x2C) & 0x2000) {
        *(s32 *)((s8*)(arg0) + 0xD0) |= 2;
    }
    switch (D_80099DA2) {
    default:
        temp_v0 = *(u16 *)((s8*)(temp_s1) + 4) - 1;
        *(u16 *)((s8*)(temp_s1) + 4) = temp_v0;
        if ((s16) temp_v0 != -1) {
            return;
        }
    case 2:
    case 8:
    case 9:
    case 11:
    case 12:
    case 13:
        break;
    }
    switch (D_80099DA2) {
    case 3:
        func_8008EF30(arg0, temp_s1, 0);
        break;
    case 4:
        func_8008EF30(arg0, temp_s1, 1);
        break;
    case 5:
        func_8008EF74(arg0, temp_s1);
        break;
    case 6:
        func_8008F014(arg0, temp_s1);
        break;
    case 7:
        func_8008F060(arg0, temp_s1);
        break;
    case 9:
        func_8008F094(arg0, temp_s1);
        break;
    case 8:
        func_8008F17C(arg0, temp_s1);
        break;
    case 10:
        func_8008EF00(arg0, temp_s1);
        break;
    case 11:
        func_8008F260(arg0, temp_s1, 0);
        return;
    case 12:
        func_8008F260(arg0, temp_s1, 1);
        return;
    case 13:
        func_8008F260(arg0, temp_s1, 2);
        return;
    case 2:
        D_80099D9E = 0;
        return;
    case 1:
        func_8008EFA8(arg0, temp_s1);
        break;
    case 0:
    default:
        *(u16 *)((s8*)(temp_s1) + 4) = 1U;
        break;
    }
    func_8008EE1C(arg0, *(s16 *)((s8*)(temp_s1) + 0xA), *(s16 *)((s8*)(temp_s1) + 0xC));
}


s32 func_8008F4F4(void *arg0, s32 arg1) {
    return ((*(s16 *)((s8*)(arg0) + 0xBC) * arg1) / 255) < *(s16 *)((s8*)(arg0) + 0xB4);
}

s32 func_8008F530(void *arg0, s32 arg1) {
    if (arg1 != 0) {
        return func_80073DE4(arg0, *(s16 *)((u8 *)arg0 + 0xBE));
    }
    return *(s16 *)((u8 *)arg0 + 0xB6) < 0x1000 - *(s16 *)((u8 *)arg0 + 0xBE);
}

s32 func_8008F570(void *arg0) {
    return 0x1000 - *(s16 *)((s8*)(arg0) + 0xBE);
}

s32 func_8008F580(void *arg0) {
    s16 temp_v1;
    s32 temp_a1;

    temp_a1 = *(s32 *)((s8*)(*(void **)((s8*)(arg0) + 0x15FC)) + 0x24);
    temp_v1 = *(s16 *)((s8*)(arg0) + 0xB6);
    if ((temp_a1 - 0x200) >= temp_v1) {
        return 2;
    }
    return temp_a1 >= temp_v1;
}

s32 func_8008F5B4(void *arg0, s32 arg1) {
    void *temp_a0;
    void *temp_s1;

    temp_s1 = *(void **)((s8*)(arg0) + 0x15FC);
    if ((rand() & 0xFF) < ((*(s32 *)((s8*)(temp_s1) + 0x10) * 0x140) >> 4)) {
        if (func_8008F530(arg0, 0) == 0 && func_8008F4F4(arg0, 0xC0) == 0 && (rand() & 0xFF) >= ((*(s32 *)((s8*)(temp_s1) + 0x1C) * 0xC0) >> 4)) {
            if (func_8008F4F4(arg0, 0x80) == 0) {
                return 0;
            }
            if (*(s16 *)((s8*)(*(void **)((s8*)(arg0) + 0xD8)) + 0xB4) < *(s16 *)((s8*)(arg0) + 0xB4)) {
                goto block_8;
            }
            return 0;
        }
        goto block_8;
    }
    if ((rand() & 0xFF) >= ((*(s32 *)((s8*)(temp_s1) + 0x1C) * 0x140) >> 4)) {
        return 0;
    }
block_8:
    if ((rand() & 0xFF) < *(s32 *)((s8*)(temp_s1) + 0x18)) {
        temp_a0 = *(void **)((s8*)(arg0) + 0xD8);
        if (*(u8 *)((s8*)(temp_a0) + 0xC4) == 4) {
            return 0;
        }
        if (func_8008F4F4(temp_a0, 0x20) == 0 && *(s16 *)((s8*)(*(void **)((s8*)(arg0) + 0xD8)) + 0xB4) < *(s16 *)((s8*)(arg0) + 0xB4)) {
            return 0;
        }
    }
    return 1;
}


s32 func_8008F720(void *arg0, s32 arg1) {
    s32 var_v0;
    void *temp_s1;

    temp_s1 = *(void **)((s8*)(arg0) + 0x15FC);
    if ((arg1 == 0) || ((rand() & 0xFF) >= *(s32 *)((s8*)(temp_s1) + 0x10)) || (var_v0 = 0, ((D_8009284C < 0x600) == 0))) {
        if (func_8008F580(arg0) == 0) {
            return (rand() & 0xFF) < *(s32 *)((s8*)(temp_s1) + 0x1C);
        }
        var_v0 = 1;
        /* Duplicate return node #6. Try simplifying control flow for better match */
        return var_v0;
    }
    return var_v0;
}

typedef struct {
    u32 b0 : 8;
    u32 b8 : 1;
    u32 b9 : 1;
    u32 b10 : 1;
    u32 b11 : 1;
    u32 b12 : 1;
    u32 pad13 : 19;
} func_8008F7B8_2C;

void func_8008F7B8(void *arg0) {
    ((func_8008F7B8_2C *)((s8*)(arg0) + 0x2C))->b9 = (rand() & 0xFF) < *(s32 *)((s8*)(arg0) + 0x10);
    ((func_8008F7B8_2C *)((s8*)(arg0) + 0x2C))->b10 = (rand() & 0xFF) < *(s32 *)((s8*)(arg0) + 0x14);
    ((func_8008F7B8_2C *)((s8*)(arg0) + 0x2C))->b12 = rand() & 1;
    ((func_8008F7B8_2C *)((s8*)(arg0) + 0x2C))->b11 = (rand() & 0xFF) < *(s32 *)((s8*)(arg0) + 0x18);
    ((func_8008F7B8_2C *)((s8*)(arg0) + 0x2C))->b8 = ((rand() & 0xFF) < *(s32 *)((s8*)(arg0) + 0x10)) && ((rand() % 10) < 3);
    ((func_8008F7B8_2C *)((s8*)(arg0) + 0x2C))->b0 = rand();
    *(u16 *)((s8*)(arg0) + 0x30) = *(u16 *)((s8*)(*(void **)((s8*)(arg0) + 0)) + 0x1668);
}


void func_8008F900(void *arg0) {
    if (((*(s32 *)((s8*)(*(void **)((s8*)(arg0) + 0xD8)) + 0xD0) & 0x60000000) != 0x20000000) || !(rand() & 3)) {
        if (D_80092884 != 0) {
            func_800767C8(arg0);
            goto block_5;
        }
        if ((*(s32 *)((s8*)(arg0) + 0xD0) & 0x60000000) == 0x20000000) {
block_5:
            func_8007639C(arg0, 4);
        }
        func_8007639C(arg0, 3);
    }
}

s32 func_800828C4(void *);

s32 func_8008F9B0(void *arg0) {
    typedef struct { s32 x; s32 y; s32 z; s32 w; } V4_F9B0;
    V4_F9B0 v = *(V4_F9B0 *)arg0;

    v.x = v.x - 0x3F80;
    v.z = v.z - 0x3F80;
    if (v.x <= 0) {
        goto call_path;
    }
    if (v.z > 0) {
        return 1;
    }
call_path:
    return (func_800828C4(arg0) & 0x03000000) == 0x01000000;
}

INCLUDE_ASM("asm/battling/nonmatchings/main", func_8008FA2C);
INCLUDE_ASM("asm/battling/nonmatchings/main", func_8008FACC);
void func_80076424(void *arg0);

void func_8008FBD8(void *arg0, void *arg1) {
    u8 level;

    level = *((u8 *)arg1 + 0xF);
    if (level >= 2) {
        *(s32 *)((u8 *)arg0 + 0xD0) |= 2;
        func_80076424(arg0);
    } else if (level != 0) {
        if (*((u8 *)arg1 + 0x2C) & 1) {
            *(s32 *)((u8 *)arg0 + 0xD0) |= 2;
            func_80076424(arg0);
        }
    } else {
        if ((u8)(*((u8 *)arg1 + 0x2C) % 6) == 0) {
            *(s32 *)((u8 *)arg0 + 0xD0) |= 2;
            func_80076424(arg0);
        }
    }
}


void func_8008FC7C(void *arg0) {
    void *temp_a0;

    temp_a0 = *(void **)((s8*)(arg0) + 0x15FC);
    *(s32 *)((s8*)(temp_a0) + 0x20) = 0;
    *(s8 *)((s8*)(temp_a0) + 8) = 0;
    *(s16 *)((s8*)(temp_a0) + 4) = (s16) (((2 - *(u8 *)((s8*)(temp_a0) + 0xF)) * 0x1E) + 0x5A);
    func_8008F7B8(temp_a0);
}

void func_80090504__2arg(void *, s32) asm("func_80090504");

void func_8008FCC8(void *arg0, void *arg1) {
    *(s16 *)((u8 *)arg1 + 0xC) = 0;
    *((u8 *)arg1 + 0xE) = 0;
    *(s16 *)((u8 *)arg1 + 0xA) = 0;
    if (func_8008FACC(arg0, arg1) != 0) {
        return;
    }
    if (D_8009284C < 0x200) {
        if (*(s32 *)((u8 *)arg1 + 0x2C) & 0x1000) {
            func_80090174(arg0);
        } else if (*((u8 *)arg1 + 0xF) != 0) {
            func_80090894(arg0, 1);
        }
        if (*(*(u8 **)((u8 *)arg0 + 0xD8) + 0xC5) == 2) {
            func_8008FBD8(arg0, arg1);
        }
        if (*(s16 *)((u8 *)arg0 + 0x1668) + 2 < *(s16 *)((u8 *)arg1 + 0x30)) {
            func_80090174(arg0);
        }
    }
    if (*(s32 *)(*(u8 **)((u8 *)arg0 + 0xD8) + 0x8F4) < 0x400) {
        func_80090894(arg0, 3);
        *((u8 *)arg1 + 0xE) = 1;
    }
    switch (*(s32 *)((u8 *)arg1 + 0x20)) {
    case 0:
        if (--*(s16 *)((u8 *)arg1 + 4) < 0) {
            *(s32 *)((u8 *)arg1 + 0x20) += 1;
        }
        if (*(*(u8 **)((u8 *)arg0 + 0xD8) + 0xC4) == 0 && (*(s32 *)((u8 *)arg1 + 0x2C) & 0x1000)) {
            if (func_8008F5B4(arg0, 0) != 0) {
                func_8008F900(arg0);
            }
            func_80090504__2arg(arg0, 0);
        }
        break;
    case 1:
        if (func_8008F530(arg0, 0) != 0) {
            func_80090504__2arg(arg0, 0);
        }
        break;
    }
}


void func_8008FE80(s32 arg0) {
    s32 temp_a0;
    s32 var_s0;

    temp_a0 = rand() % 10;
    var_s0 = 1;
    if (temp_a0 >= 2) {
        var_s0 = 2;
    }
    if (temp_a0 >= 5) {
        var_s0 += 1;
    }
    if (var_s0 != 0) {
        do {
            var_s0 -= 1;
            func_8007639C(arg0, (rand() & 1) + 1);
        } while (var_s0 != 0);
    }
}

void func_8008FF24(void *arg0, void *arg1) {

    if (*(u8 *)((s8*)(*(void **)((s8*)(arg0) + 0xD8)) + 0xC4) == 4) {
        if ((*(s32 *)((s8*)(arg1) + 0x2C) & 0xA00) == 0x200) {
            func_8008F900(arg0);
            *(s16 *)((s8*)(arg1) + 4) = 3;
        }
    } else {
        func_8008FE80((s32)arg0);
        *(s16 *)((s8*)(arg1) + 4) = (2 - *(u8 *)((s8*)(arg1) + 0xF)) * 20 + (s16)(rand() % 20 + 1);
    }
    func_8008F7B8(arg1);
}

/* The TU prototypes this `void func_8008FFEC(void *, s32)`, but retail
 * returns 1 (nothing to pick) or 0, so it is defined under a returning alias. */
s32 func_8008FFEC__ret(void *arg0, void *arg1) asm("func_8008FFEC");
/* D_800925A4: 14 x 3 status ids. */
extern u8 D_800925A4[];

s32 func_8008FFEC__ret(void *arg0, void *arg1) {
    s32 n;
    s32 i;
    s32 k;

    n = *((u8 *)arg0 + 0x911);
    if (n == 0) {
        return 1;
    }
    k = 0;
    n = rand() % n;
    for (i = 0; i < 14; i++, k += 3) {
        if (*(u8 *)(*(s32 *)((u8 *)arg0 + 0x1600) + i + 0xA) != 0 &&
            *(u8 *)(i * 4 + *(s32 *)((u8 *)arg0 + 0x80) + 3) != 0) {
            if (n == 0) {
                if (D_800925A4[k] != 0) {
                    func_8007639C(arg0, D_800925A4[k]);
                }
                if (D_800925A4[k + 1] != 0) {
                    func_8007639C(arg0, D_800925A4[k + 1]);
                }
                if (D_800925A4[k + 2] != 0) {
                    func_8007639C(arg0, D_800925A4[k + 2]);
                }
                break;
            }
            n--;
        }
    }
    *(s16 *)((u8 *)arg1 + 4) = (2 - *((u8 *)arg1 + 0xF)) * 20 + (s16)(rand() % 20 + 1);
    func_8008F7B8(arg1);
    return 0;
}

void func_80090174(void *arg0) {
    u8 *ai;

    ai = *(u8 **)((u8 *)arg0 + 0x15FC);
    ai[8] = 1;
    func_8008FF24(arg0, ai);
    if (ai[0xF] >= 2) {
        ai[9] = rand() % 8 + 1;
    } else if (ai[0xF] != 0) {
        ai[9] = rand() % 6 + 1;
    } else {
        ai[9] = rand() % 4 + 1;
    }
    *(s16 *)(ai + 0xC) = 0;
    ai[0xE] = 0;
    func_8008F7B8(ai);
}


s32 func_80090258(void *arg0, s32 arg1) {
    s32 var_v0;

    if (*(u8 *)((s8*)(arg0) + 0x911) != 0) {
        var_v0 = 1;
        if (rand() & 1) {
            if (!(rand() & 1) || (func_8008F5B4(arg0, 0) == 0)) {
                if (D_8009284C < 0x1001) {
                    func_8008FFEC(arg0, arg1);
                    return 0;
                }
                goto block_7;
            }
            goto block_8;
        }
        /* Duplicate return node #9. Try simplifying control flow for better match */
        return var_v0;
    }
    if (func_8008F5B4(arg0, 0) == 0) {
block_7:
        return 1;
    }
block_8:
    func_8008F900(arg0);
    var_v0 = 0;
    return var_v0;
}

void func_8009031C(void *arg0, void *arg1) {
    u8 temp_v1_2;

    if (func_8008FACC(arg0, arg1) == 0) {
        if ((*(u8 *)((u8 *)arg0 + 0xC5) != 2) && (*(u8 *)(*(u8 **)((u8 *)arg0 + 0xD8) + 0xC5) == 2)) {
            func_8008FBD8(arg0, arg1);
        }
        if (((u32)(*(s32 *)(*(u8 **)((u8 *)arg0 + 0xD8) + 0x8F4) - 0x201) < 0x3FFU) && (*((u8 *)arg1 + 0xF) != 0)) {
            func_8008FBD8(arg0, arg1);
        }
        if ((s16)--*(u16 *)((u8 *)arg1 + 4) <= 0) {
            if (D_8009284C >= 0x301) {
                switch (rand() % 10) {
                case 0:
                    if ((rand() & 0xFF) >= *(s32 *)((u8 *)arg1 + 0x14)) {
                        func_80090894(arg0, 0);
                    }
                    break;
                case 2:
                    if (*(u8 *)((u8 *)arg0 + 0x911) == 0) goto shared;
                    func_8008FFEC(arg0, (s32)arg1);
                    break;
                case 3:
                case 4:
                case 5:
                    if (func_80090258(arg0, (s32)arg1) == 0) break;
                case 1:
shared:
                    func_8008FC7C(arg0);
                    break;
                case 6:
                case 7:
                case 8:
                case 9:
                    func_80090504__2arg(arg0, 0);
                    break;
                }
            }
            temp_v1_2 = *((u8 *)arg1 + 9);
            *((u8 *)arg1 + 9) = temp_v1_2 + 0xFF;
            if (temp_v1_2 != 0) {
                func_8008FF24(arg0, arg1);
                return;
            }
            func_80090894(arg0, rand() & 1);
        }
    }
}


void func_80090504(void *arg0) {
    s32 temp_v1;
    s32 var_v0;
    void *temp_s0;

    temp_s0 = *(void **)((s8*)(arg0) + 0x15FC);
    *(s8 *)((s8*)(temp_s0) + 8) = 3;
    var_v0 = rand();
    temp_v1 = var_v0;
    if (temp_v1 < 0) {
        var_v0 = temp_v1 + 3;
    }
    *(s8 *)((s8*)(temp_s0) + 9) = (s8) ((temp_v1 - ((var_v0 >> 2) * 4)) + 1);
    *(s16 *)((s8*)(temp_s0) + 4) = 0;
    func_8008F7B8(temp_s0);
    *(s8 *)((s8*)(temp_s0) + 0xE) = func_8008F720(arg0, 1);
    *(s8 *)((s8*)(temp_s0) + 0x2E) = 0;
}

extern u8 D_800928C4;

void func_80090580(void *arg0, void *arg1) {
    s16 var_a0;
    s32 temp_a0_2;
    s32 var_v0;
    u8 var_v0_2;
    void *temp_a0;

    if ((func_8008F9B0(*(void **)((u8 *)arg0 + 0xD8)) != 0) && (D_800928C4 != 0) && (*(s32 *)((u8 *)arg0 + 0xD0) & 0x08000000)) {
        func_8008FC7C(arg0);
        return;
    }
    temp_a0 = *(void **)((u8 *)arg0 + 0xD8);
    if ((*(u8 *)(*(u8 **)((u8 *)temp_a0 + 0x8F8) + 0x33) == 1) && (*(s32 *)((u8 *)temp_a0 + 0x8F4) < 0x500) && (*((u8 *)arg1 + 0xF) != 0)) {
        var_a0 = -0x400;
        *((u8 *)arg1 + 0xE) = 1;
        if (*(s32 *)((u8 *)arg1 + 0x2C) & 0x1000) {
            var_a0 = 0x400;
        }
        *(s16 *)((u8 *)arg1 + 0xA) = var_a0;
        if ((*((u8 *)arg1 + 0xF) >= 2U) && ((rand() & 0xFF) < *(s32 *)((u8 *)arg1 + 0x14)) && (func_8008F5B4(arg0, 0) != 0) && (*(s32 *)((u8 *)arg1 + 0x2C) & 0x1000)) {
            if (!(rand() & 3)) {
                func_8007639C(arg0, 4);
            }
            func_8008F900(arg0);
            *((u8 *)arg1 + 0x2E) = 0;
        }
        *((u8 *)arg1 + 0x2E) = *((u8 *)arg1 + 0x2E) + 1;
    }
    if (D_8009284C < 0x180) {
        if (!(rand() & 3)) {
            func_8007639C(arg0, 4);
        }
        func_80090174(arg0);
    }
    if (func_8008F580(arg0) == 0) {
        *((u8 *)arg1 + 0xE) = 0;
    }
    if (*(s16 *)((u8 *)arg1 + 4) < 0) {
        *(s16 *)((u8 *)arg1 + 0xA) = rand() % 1536 - 0x300;
        temp_a0_2 = rand();
        if (*(s32 *)((u8 *)arg1 + 0x2C) & 0x400) {
            var_v0 = temp_a0_2 % 120 + 10;
        } else {
            var_v0 = temp_a0_2 % 100 + 10;
        }
        *(s16 *)((u8 *)arg1 + 4) = var_v0;
        *(s16 *)((u8 *)arg1 + 0xC) = 0xFF;
        if (*((u8 *)arg1 + 9) != 0) {
            var_v0_2 = *((u8 *)arg1 + 9) - 1;
        } else {
            if (!(rand() & 3)) {
                func_8007639C(arg0, 4);
            }
            if (func_80090258(arg0, (s32)arg1) != 0) {
                func_8008FC7C(arg0);
            }
            var_v0_2 = rand() % 4 + 1;
        }
        *((u8 *)arg1 + 9) = var_v0_2;
    }
    *(s16 *)((u8 *)arg1 + 4) = (s16) ((u16) *(s16 *)((u8 *)arg1 + 4) - 1);
}

void func_80090894(void *arg0, s32 arg1) {
    void *p;
    s32 t;
    p = *(void **)((u8 *)arg0 + 0x15FC);
    *(s8 *)((u8 *)p + 8) = 2;
    if (rand() % 3 == 0) func_800767C8(arg0);
    t = rand();
    *(s16 *)((u8 *)p + 4) = 0;
    *(s32 *)((u8 *)p + 0x28) = t % 1536 + 0x100;
    *(s8 *)((u8 *)p + 9) = rand() % 5 + 3;
    func_8008F7B8(p);
    *(s8 *)((u8 *)p + 0xE) = func_8008F720(arg0, 0);
    *(s8 *)((u8 *)p + 0x2E) = 0;
}

extern s32 D_80092850;

void func_80090990(void *arg0, void *arg1) {
    s16 var_a0;
    s32 temp_a0_2;
    s32 var_v0;
    u8 var_v0_2;
    void *temp_a0;

    if (func_8008FACC(arg0, arg1) == 0) {
        temp_a0 = *(void **)((u8 *)arg0 + 0xD8);
        if ((*(u8 *)(*(u8 **)((u8 *)temp_a0 + 0x8F8) + 0x33) == 1) && (*(s32 *)((u8 *)temp_a0 + 0x8F4) < 0x500)) {
            var_a0 = -0x400;
            if (*((u8 *)arg1 + 0xF) != 0) {
                *((u8 *)arg1 + 0xE) = 1;
                if (*(s32 *)((u8 *)arg1 + 0x2C) & 0x1000) {
                    var_a0 = 0x400;
                }
                *(s16 *)((u8 *)arg1 + 0xA) = var_a0;
                if ((*((u8 *)arg1 + 0xF) >= 2U) && (func_8008F5B4(arg0, 0) != 0) && (*(s32 *)((u8 *)arg1 + 0x2C) & 0x1000)) {
                    if (!(rand() & 3)) {
                        func_8007639C(arg0, 4);
                    }
                    func_8008F900(arg0);
                    *((u8 *)arg1 + 0x2E) = 0;
                }
                *((u8 *)arg1 + 0x2E) = *((u8 *)arg1 + 0x2E) + 1;
            }
        }
        if (func_8008F580(arg0) == 0) {
            *((u8 *)arg1 + 0xE) = 0;
        }
        if ((D_8009284C > *(s32 *)((u8 *)arg1 + 0x28)) || ((D_80092884 != 0) && (D_80092850 >= 0x4B1))) {
            if (D_80092884 != 0) {
                func_8007639C(arg0, 4);
            }
            func_8008FFEC(arg0, (s32)arg1);
            func_8008FC7C(arg0);
        }
        if ((*(s16 *)((u8 *)arg1 + 4) < 0) || (*(s16 *)((u8 *)arg1 + 0xC) == 0)) {
            *(s16 *)((u8 *)arg1 + 0xA) = rand() % 1536 + 0x500;
            temp_a0_2 = rand();
            if (*(s32 *)((u8 *)arg1 + 0x2C) & 0x400) {
                var_v0 = temp_a0_2 % 40 + 10;
            } else {
                var_v0 = temp_a0_2 % 60 + 10;
            }
            *(s16 *)((u8 *)arg1 + 4) = var_v0;
            *(s16 *)((u8 *)arg1 + 0xC) = 0xFF;
            if (*((u8 *)arg1 + 9) != 0) {
                var_v0_2 = *((u8 *)arg1 + 9) - 1;
            } else {
                if (func_8008F5B4(arg0, 0) != 0) {
                    func_8007639C(arg0, 4);
                    func_8008F900(arg0);
                }
                var_v0_2 = rand() % 5 + 3;
            }
            *((u8 *)arg1 + 9) = var_v0_2;
        }
        *(s16 *)((u8 *)arg1 + 4) = (s16) ((u16) *(s16 *)((u8 *)arg1 + 4) - 1);
    }
}


void func_80090C88(void *arg0) {
    void *temp_a0;
    void *temp_v1;

    temp_v1 = *(void **)((s8*)(arg0) + 0x1600);
    temp_a0 = *(void **)((s8*)(arg0) + 0x15FC);
    *(s32 *)((s8*)(temp_a0) + 0x10) = (s32) *(u8 *)((s8*)(temp_v1) + 6);
    *(s32 *)((s8*)(temp_a0) + 0x14) = (s32) *(u8 *)((s8*)(temp_v1) + 7);
    *(s32 *)((s8*)(temp_a0) + 0x18) = (s32) *(u8 *)((s8*)(temp_v1) + 8);
    *(s32 *)((s8*)(temp_a0) + 0x1C) = (s32) *(u8 *)((s8*)(temp_v1) + 9);
}

void func_80076424(void *arg0);
/* Retail passes a second argument to these two one-parameter functions
 * (a1 = the AI block / 0); the TU prototypes take one, so call aliases. */
s32 func_8008F570__2arg(void *, void *) asm("func_8008F570");
void func_80090504__2arg(void *, s32) asm("func_80090504");
/* D_80096F30 / D_80096F64: the two 0x34-byte battling AI blocks. */
extern u8 D_80096F30[];
extern u8 D_80096F64[];
extern u8 D_80099D98[];

void func_80090CC0(void *arg0) {
    u8 *ai;

    ai = D_80096F30;
    if (*(s32 *)((s8*)(arg0) + 0xD0) & 0x08000000) {
        ai = D_80096F64;
    }
    *(u8 **)((s8*)(arg0) + 0x15FC) = ai;
    *(s8 *)(ai + 6) = 0x10;
    *(void **)(ai + 0) = arg0;
    *(s16 *)(ai + 4) = 0;
    *(s8 *)(ai + 7) = 0xA;
    *(s16 *)(ai + 0xA) = 0;
    *(s16 *)(ai + 0xC) = 0;
    *(s8 *)(ai + 8) = 0;
    *(s8 *)(ai + 9) = 0;
    *(s32 *)((s8*)(arg0) + 0x48) = 0;
    *(s32 *)(ai + 0x24) = func_8008F570__2arg(arg0, ai);
    *(u8 *)(ai + 0xF) = D_80099D98[0];
    func_80090C88(arg0);
    switch (rand() % 3) {
    case 0:
        func_8008FC7C(arg0);
        break;
    case 1:
        func_80090174(arg0);
        break;
    case 2:
        func_80090894(arg0, 0);
        break;
    case 3:
        func_80090504__2arg(arg0, 0);
        break;
    }
    func_80076424(arg0);
}

void func_80090E10(void *arg0) {
    u8 *ai;

    if (*(s32 *)((u8 *)arg0 + 0xD0) & 0x40) {
        ai = *(u8 **)((u8 *)arg0 + 0x15FC);
        ai[0xF] = D_80099D98[0];
        switch (ai[8]) {
        case 0:
            func_8008FCC8(arg0, ai);
            break;
        case 1:
            func_8009031C(arg0, ai);
            break;
        case 2:
            func_80090990(arg0, ai);
            break;
        case 3:
            func_80090580(arg0, ai);
            break;
        }
        if (ai[0xE] != 0) {
            *(s32 *)((u8 *)arg0 + 0xD0) |= 0x8000;
        }
        if (*(s32 *)(ai + 0x2C) & 0x2000) {
            *(s32 *)((u8 *)arg0 + 0xD0) |= 2;
        }
        func_8008EE1C(arg0, *(s16 *)(ai + 0xA), *(s16 *)(ai + 0xC));
    }
}

INCLUDE_ASM("asm/battling/nonmatchings/main", func_80090F38);
#endif
