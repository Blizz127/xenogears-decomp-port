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

INCLUDE_ASM("asm/battling/nonmatchings/main", func_80089210);
INCLUDE_ASM("asm/battling/nonmatchings/main", func_80089330);
INCLUDE_ASM("asm/battling/nonmatchings/main", func_80089534);

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
INCLUDE_ASM("asm/battling/nonmatchings/main", func_800898BC);
INCLUDE_ASM("asm/battling/nonmatchings/main", func_80089A98);
INCLUDE_ASM("asm/battling/nonmatchings/main", func_80089B44);

void *func_80089C54(void) {
    HeapSetCurrentContentType(8);
    return func_80089B44(HeapAlloc(0x9C, 0));
}

INCLUDE_ASM("asm/battling/nonmatchings/main", func_80089C88);

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

INCLUDE_ASM("asm/battling/nonmatchings/main", func_80089EB4);
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

INCLUDE_ASM("asm/battling/nonmatchings/main", func_8008A040);

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
INCLUDE_ASM("asm/battling/nonmatchings/main", func_8008A2B8);

void func_8008A3A0(void) {

}


void func_8008A3A8(void *arg0) {
    HeapDelayedFree(*(s32 *)((s8*)(arg0) + 4), 3);
    HeapFree(arg0);
}

INCLUDE_ASM("asm/battling/nonmatchings/main", func_8008A3E0);

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

INCLUDE_ASM("asm/battling/nonmatchings/main", func_8008A63C);
INCLUDE_ASM("asm/battling/nonmatchings/main", func_8008A6F8);
INCLUDE_ASM("asm/battling/nonmatchings/main", func_8008A78C);
INCLUDE_ASM("asm/battling/nonmatchings/main", func_8008A7E0);

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

INCLUDE_ASM("asm/battling/nonmatchings/main", func_8008ACB8);
INCLUDE_ASM("asm/battling/nonmatchings/main", func_8008AE1C);
INCLUDE_ASM("asm/battling/nonmatchings/main", func_8008AF6C);

void func_8008B070(s32 arg0) {
    s32 temp_v0;

    HeapSetCurrentContentType(0xA);
    temp_v0 = HeapAlloc(ArchiveDecodeSize(arg0), 0);
    ArchiveReadFileToBuffer(arg0, temp_v0, 0, 0);
    ArchiveCdDataSync(0);
    func_8008AF6C(temp_v0);
}

INCLUDE_ASM("asm/battling/nonmatchings/main", func_8008B0D8);
INCLUDE_ASM("asm/battling/nonmatchings/main", func_8008B13C);
INCLUDE_ASM("asm/battling/nonmatchings/main", func_8008B38C);
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

INCLUDE_ASM("asm/battling/nonmatchings/main", func_8008B650);
INCLUDE_ASM("asm/battling/nonmatchings/main", func_8008B730);
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
INCLUDE_ASM("asm/battling/nonmatchings/main", func_8008BB00);
INCLUDE_ASM("asm/battling/nonmatchings/main", func_8008BB1C);
INCLUDE_ASM("asm/battling/nonmatchings/main", func_8008BB3C);
INCLUDE_ASM("asm/battling/nonmatchings/main", func_8008BC04);
INCLUDE_ASM("asm/battling/nonmatchings/main", func_8008BCC8);
INCLUDE_ASM("asm/battling/nonmatchings/main", func_8008BD70);
INCLUDE_ASM("asm/battling/nonmatchings/main", func_8008BE4C);

void func_8008C0BC(void *arg0, s32 arg1) {
    *(s32 *)((s8*)(arg0) + 0) = 5;
    *(s32 *)((s8*)(arg0) + 4) = arg1;
}

INCLUDE_ASM("asm/battling/nonmatchings/main", func_8008C0CC);

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
INCLUDE_ASM("asm/battling/nonmatchings/main", func_8008C7C0);
INCLUDE_ASM("asm/battling/nonmatchings/main", func_8008C828);
INCLUDE_ASM("asm/battling/nonmatchings/main", func_8008C8B4);
INCLUDE_ASM("asm/battling/nonmatchings/main", func_8008C9B8);
INCLUDE_ASM("asm/battling/nonmatchings/main", func_8008CA00);
INCLUDE_ASM("asm/battling/nonmatchings/main", func_8008CA84);
INCLUDE_ASM("asm/battling/nonmatchings/main", func_8008CC2C);
INCLUDE_ASM("asm/battling/nonmatchings/main", func_8008CC54);
INCLUDE_ASM("asm/battling/nonmatchings/main", func_8008CCB0);

void func_8008CD54(void) {

}

INCLUDE_ASM("asm/battling/nonmatchings/main", func_8008CD5C);
INCLUDE_ASM("asm/battling/nonmatchings/main", func_8008CE0C);

void func_8008CED4(void) {

}

INCLUDE_ASM("asm/battling/nonmatchings/main", func_8008CEDC);
INCLUDE_ASM("asm/battling/nonmatchings/main", func_8008CF30);
INCLUDE_ASM("asm/battling/nonmatchings/main", func_8008CF9C);
INCLUDE_ASM("asm/battling/nonmatchings/main", func_8008CFC4);

void func_8008D0A4(void *arg0, void *arg1) {
    s32 temp_hi;
    s32 temp_hi_2;

    temp_hi = rand() % (s16) *(s16 *)((s8*)(arg0) + 0x24);
    *(s16 *)((s8*)(arg1) + 0) = (s16) ((*(u16 *)((s8*)(arg0) + 0x1C) + temp_hi) - *(u16 *)((s8*)(arg0) + 0x2C));
    temp_hi_2 = rand(temp_hi) % (s16) *(s16 *)((s8*)(arg0) + 0x26);
    *(s16 *)((s8*)(arg1) + 2) = (s16) ((*(u16 *)((s8*)(arg0) + 0x1E) + temp_hi_2) - *(u16 *)((s8*)(arg0) + 0x2E));
    *(s16 *)((s8*)(arg1) + 4) = (s16) ((*(u16 *)((s8*)(arg0) + 0x20) + (rand(temp_hi_2) % (s16) *(s16 *)((s8*)(arg0) + 0x28))) - *(u16 *)((s8*)(arg0) + 0x30));
}

INCLUDE_ASM("asm/battling/nonmatchings/main", func_8008D14C);
INCLUDE_ASM("asm/battling/nonmatchings/main", func_8008D208);
INCLUDE_ASM("asm/battling/nonmatchings/main", func_8008D304);
INCLUDE_ASM("asm/battling/nonmatchings/main", func_8008D3F4);
INCLUDE_ASM("asm/battling/nonmatchings/main", func_8008D580);
INCLUDE_ASM("asm/battling/nonmatchings/main", func_8008D5C0);
INCLUDE_ASM("asm/battling/nonmatchings/main", func_8008D680);
INCLUDE_ASM("asm/battling/nonmatchings/main", func_8008D980);
INCLUDE_ASM("asm/battling/nonmatchings/main", func_8008D9F0);
INCLUDE_ASM("asm/battling/nonmatchings/main", func_8008DA48);
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

INCLUDE_ASM("asm/battling/nonmatchings/main", func_8008DCB8);
INCLUDE_ASM("asm/battling/nonmatchings/main", func_8008DDFC);
INCLUDE_ASM("asm/battling/nonmatchings/main", func_8008DE54);

void func_8008DF30(void) {
    D_80092844 = 0;
    D_8009283C = 0;
    D_80092840 = 0;
}

INCLUDE_ASM("asm/battling/nonmatchings/main", func_8008DF50);

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
INCLUDE_ASM("asm/battling/nonmatchings/main", func_8008E120);
INCLUDE_ASM("asm/battling/nonmatchings/main", func_8008E2B8);
INCLUDE_ASM("asm/battling/nonmatchings/main", func_8008E3CC);
INCLUDE_ASM("asm/battling/nonmatchings/main", func_8008E620);
INCLUDE_ASM("asm/battling/nonmatchings/main", func_8008E67C);

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

INCLUDE_ASM("asm/battling/nonmatchings/main", func_8008E78C);
INCLUDE_ASM("asm/battling/nonmatchings/main", func_8008E8B0);
INCLUDE_ASM("asm/battling/nonmatchings/main", func_8008EADC);
INCLUDE_ASM("asm/battling/nonmatchings/main", func_8008EB4C);
void func_8008EB88(void *arg0, s32 arg1, s32 arg2, s32 arg3) {
    if (arg1 != 0) {
        func_8008E78C(arg1 + 0x60000, arg3, arg2,
                      (arg1 & 0x7F) | ((*(u32 *)((u8 *)arg0 + 0xD0) >> 0x14) & 0x80));
    }
}
INCLUDE_ASM("asm/battling/nonmatchings/main", func_8008EBD0);
INCLUDE_ASM("asm/battling/nonmatchings/main", func_8008ECEC);
INCLUDE_ASM("asm/battling/nonmatchings/main", func_8008ED6C);
INCLUDE_ASM("asm/battling/nonmatchings/main", func_8008EE1C);

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

INCLUDE_ASM("asm/battling/nonmatchings/main", func_8008EFA8);
INCLUDE_ASM("asm/battling/nonmatchings/main", func_8008F014);

#ifndef XENO_PC_PORT
void func_8008F060(s32 arg0, void *arg1) {
    func_8007639C(arg0, 4);
    *(s16 *)((s8*)(arg1) + 4) = 0x3C;
}
#endif

INCLUDE_ASM("asm/battling/nonmatchings/main", func_8008F094);
INCLUDE_ASM("asm/battling/nonmatchings/main", func_8008F17C);

void func_8008F260(s32 arg0, void *arg1, s8 arg2) {
    *(s8 *)((s8*)(arg1) + 0xF) = arg2;
    func_80090E10();
}

INCLUDE_ASM("asm/battling/nonmatchings/main", func_8008F280);

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

INCLUDE_ASM("asm/battling/nonmatchings/main", func_8008F580);
INCLUDE_ASM("asm/battling/nonmatchings/main", func_8008F5B4);

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

INCLUDE_ASM("asm/battling/nonmatchings/main", func_8008F7B8);

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

INCLUDE_ASM("asm/battling/nonmatchings/main", func_8008F9B0);
INCLUDE_ASM("asm/battling/nonmatchings/main", func_8008FA2C);
INCLUDE_ASM("asm/battling/nonmatchings/main", func_8008FACC);
INCLUDE_ASM("asm/battling/nonmatchings/main", func_8008FBD8);

void func_8008FC7C(void *arg0) {
    void *temp_a0;

    temp_a0 = *(void **)((s8*)(arg0) + 0x15FC);
    *(s32 *)((s8*)(temp_a0) + 0x20) = 0;
    *(s8 *)((s8*)(temp_a0) + 8) = 0;
    *(s16 *)((s8*)(temp_a0) + 4) = (s16) (((2 - *(u8 *)((s8*)(temp_a0) + 0xF)) * 0x1E) + 0x5A);
    func_8008F7B8(temp_a0);
}

INCLUDE_ASM("asm/battling/nonmatchings/main", func_8008FCC8);

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

INCLUDE_ASM("asm/battling/nonmatchings/main", func_8008FF24);
INCLUDE_ASM("asm/battling/nonmatchings/main", func_8008FFEC);
INCLUDE_ASM("asm/battling/nonmatchings/main", func_80090174);

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

INCLUDE_ASM("asm/battling/nonmatchings/main", func_8009031C);

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

INCLUDE_ASM("asm/battling/nonmatchings/main", func_80090580);
INCLUDE_ASM("asm/battling/nonmatchings/main", func_80090894);
INCLUDE_ASM("asm/battling/nonmatchings/main", func_80090990);

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

INCLUDE_ASM("asm/battling/nonmatchings/main", func_80090CC0);
INCLUDE_ASM("asm/battling/nonmatchings/main", func_80090E10);
INCLUDE_ASM("asm/battling/nonmatchings/main", func_80090F38);
#endif
