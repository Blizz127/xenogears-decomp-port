/*
 * Retail certificate for the battle overlay's tiny leaves (0x8009795C,
 * 0x8009F5B0, 0x800B3348, 0x800B3350, 0x800B89F4, 0x800BAF40, 0x800BDD34,
 * 0x80077980, 0x8009E268, 0x800A577C, 0x800A578C, 0x800BF0B4, 0x80079934,
 * 0x800AA788, 0x800B14B8).
 *
 * Each case loads its retail slice from disc/battle.bin, runs it on the MIPS
 * adapter with the globals and a fixture arena mapped to their retail
 * addresses, then runs the shipped C from src/battle/main*.c and compares the
 * return register and the whole arena.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "battle_mips_adapter.h"

extern void bzero(void* p, unsigned long n);

extern void func_8009795C(void);
extern void func_8009F5B0(void);
extern void func_800B3348(void);
extern void func_800B3350(void);
extern void func_800B89F4(void);
extern void func_800BAF40(void);
extern void func_800BDD34(void);
extern void func_80077980(void);
extern void func_8009E268(void);
extern u32 func_800A577C(void);
extern u32 func_800A578C(void);
extern void func_800BF0B4(u32 value);
extern void func_80079934(u32* pValue);
extern void func_800AA788(u32 value);
extern void func_800B14B8(void);
extern void func_800A5E9C(u32 a, u32 b);
extern void func_800B00D0(void);
extern void func_800769E8(void* pRect, void* pData);
extern void func_8007A9A8(u8** ppBoard);
extern void func_8007A900(u8** ppBoard, u8 index);
extern void func_8007E7C0(u8** ppBoard);
extern void func_80078D48(u32 unused, u8 index);
extern void func_80080C6C(u8 index);
extern u8* func_800B168C(u8* pArg, u32 iValue);
extern void func_8008BC40(u8 flag);
extern void func_8008CCCC(u8 flag);
extern void func_8008B108(u8 flag);
extern void func_8008AA74(u32 v);
extern void func_80077610(void);
extern void func_8007A8B4(u8** ppBoard, u8* pDst);
extern void func_80085C88(u8 index);
extern void func_800AA760(u32 index, u8 value);
extern void func_8007B3B0(u8** ppBoard, u8 x);
extern void func_8007BA88(u8 index);
extern void func_8007BAB8(u8 index);
extern void func_800B6A50(u8* p);
extern void func_8007D1A8(u8** ppBoard, u8 index);
extern void func_8008FDE4(void);
extern void func_800764B4(void);
extern void func_80076AC8(u8* p);
extern void func_8007765C(void);
extern void func_8007AAB8(u8** ppBoard, u8 index);
extern void func_8008AC50(void);
extern void func_8009AB00(u8 index);
extern void func_800B7330(u8* p);

extern void SetSemiTrans(void* p, s32 v);
extern void SetShadeTex(void* p, s32 v);
extern void HeapFree(u32 p);
extern void func_8008FAD8(void);
extern void func_801DE594(void);
extern void func_80073FB8(void);
extern void func_800716D8(void);
extern s32 ArchiveDataSync(void);
extern void* func_8008ABB8(s32 size, s32 mode);
extern void func_800A22A8(u8* p);
extern void func_800A2D1C(u8* p);
extern void func_800B7364(u32 p);
extern void func_800B9020(u8* p);
extern void* func_8008AC00(s32 size);
extern void func_80076B68(u8* p);
extern void func_80076BAC(u8* p);
extern void func_80076BF0(u8* p);
extern void func_80076C34(u8* p);
extern void func_80079054(u8 a, u8 idx);
extern u8 D_800D2E5D[];
extern u8 D_800D2E60[];
extern u8 D_800D2E61[];
extern void func_8007AF5C(u8** ppBoard, u8 index);
extern void func_8007AFAC(u8** ppBoard, u8 index);
extern void func_80079114(u8 a, u8 idx);
extern void func_800764EC(void);
extern void func_80073538(void);
extern void func_80073F08(void);
extern void func_8007500C(void);
extern void func_80074F70(void);
extern void func_80088B80(void);
extern void func_80074AB8(void);
extern void func_8007A280(u8 a0, u8 a1, u32 a2, u32 a3);
extern void func_80079E18(u8 index);
extern void func_80079E4C(u8 index);
extern u32 func_80079E7C(u16 v);
extern void func_80078C9C(u8 a, u8 b);
extern u32 func_8009A7B8(u8 index);
extern void func_8009E3C8(void);
extern u32 func_80089C9C(u32 a, u32 b);
extern void func_800785D4(u8 a, u8 b);
extern u8 D_800D3725[];
extern u8 D_800C402F[];
extern u32 D_800C3D60;
extern u8 D_800C3E50;
extern void func_800A3484(s16* p);
extern void func_800AEEEC(u8* p);
extern u32 func_800B3B6C(void);
extern void func_800BCAA4(void);
extern void func_800BFD88(u32 a0, u32 a1);
extern u32 D_800C3558;
extern u8 D_800C37C8;
extern void func_800BC2F0(s32 v);
extern void func_800BFC80(u32 a0, u32 a1, u32 a2);
extern void func_8007FCE8(void);
extern void func_8007FDEC(void);
extern void func_80078CEC(u8 a, u8 b);
extern void func_8007893C(u8 a, u8 b);
extern u32 D_800D367C;
extern u32 D_800C3DE8;
extern u8 D_800D2E5F[];
extern void func_80078658(u8 a, u8 b);
extern void func_800787E0(u8 a, u8 b);
extern void func_8007887C(u8 a);
extern void func_8008B168(u8 index);
extern void func_8008BC98(u8 index);
extern void func_8008CD28(u8 index);
extern void func_8008C360(u8 a);
extern void func_8008C3F0(u8 index);
extern u32 func_80089C08(u8 index);
extern void func_800BC404(u32 v);
extern void func_800BCD98(u32 v);
extern void func_8008FA60(u8 v);
extern void func_800BC460(u32 p);
extern u16 D_800C3CDC;
extern u16 D_80059454;
extern void func_8007AB30(u8** ppBoard, u8 index);
extern void func_8007AB68(u8** ppBoard, u8 index);
extern void func_8007ABA0(u8** ppBoard, u8 index);
extern void func_8007E674(u8 idx);
extern void func_8008AA40(u8 a);
extern void func_8009E508(void);
extern void func_800B8D7C(void);
extern void func_800BCAD0(void);
extern u32 D_800D2C60[];
extern u8 D_800D2C8B[];
extern u32 D_8005919C;
extern u8* D_800C3E34;
extern void func_80039DB8(u32 v);
extern void func_8002A498(u32 v);
extern void func_800B8D04(void);
extern u8 D_800CCE4A[];
extern u8* D_800C3EAC;
extern u32 D_800C3D60;
extern u8 D_800C3E50;

extern void HeapChangeCurrentUser(s32 user, void* p);
extern void* HeapAlloc(s32 size, s32 mode);
extern void WorkListRemoveTask(u32 p);
extern void TimerWorkListRemoveTask(u32 p);
extern void func_80025180(u32 p);
extern void func_800245D8(u32 p, s32 v);
extern void func_80021BF8(u32 p, u32 v);

u32 D_800D39CC;
u8 D_800C3B74;
u8 D_800C3D6C;
u8* D_800D2D28;
u8* D_800D2DC8;
u8* D_800C3610;
u32 D_800D2D40;
u32 D_800D2D48;
u16 D_800D39E0;
u16 D_8005A3A0[0x100];
/* Retail places D_800D2E5D / D_800D2E60 / D_800D2E62 three and two bytes
 * apart, so mirror that layout in one host buffer with aliases. */
static u8 s_e5dBuf[0x48];
/* Retail's addresses are 0x5D / 0x60 / 0x62; keep the parity so the u16
 * accesses through D_800D2E62 stay naturally aligned on the host too. */
__asm__(".globl D_800D2E5D\n.set D_800D2E5D, s_e5dBuf + 0x1\n");
__asm__(".globl D_800D2E60\n.set D_800D2E60, s_e5dBuf + 0x4\n");
__asm__(".globl D_800D2E61\n.set D_800D2E61, s_e5dBuf + 0x5\n");
__asm__(".globl D_800D2E5F\n.set D_800D2E5F, s_e5dBuf + 0x3\n");
__asm__(".globl D_800D2E62\n.set D_800D2E62, s_e5dBuf + 0x6\n");
extern u8 D_800D2E62[];
u8 g_GameState[0x2300];
u8* D_800D3278;
u32 D_800C3EA4;
u8 D_8005959C;
u8* D_800C34B0;
u8* D_800C3EAC;
u32 D_800C3D60;
u8 D_800C3E50;
u16 D_800C3448[0x40];
u32 D_800C3558;
u8 D_800C37C8;
u32 D_800D367C;
u32 D_800C3DE8;
u16 D_800C3CDC;
u16 D_80059454;
u32 D_800D2C60[0x10];
u8 D_800D2C8B[0x10];
u32 D_800D2E38[0x100];
u32 D_800D2D90[0x100];
u32 D_8005919C;
u8* D_800C3E34;
enum { kE62Size = 0x40 };
static s32 s_asmSyncCalls;

/* Retail keeps the row tables and their neighbouring BSS globals adjacent.
 * Mirror that with one host buffer plus aliases so fixtures that overlap in
 * retail overlap here too (D_800D3410's rows reach into D_800D3430's, and the
 * D_800D343F/D_800D342E descending clears walk through the same area). */
static u8 s_rowArea[0x800];
__asm__(".globl D_800D3344\n.set D_800D3344, s_rowArea + 0x44\n");
__asm__(".globl D_800D3368\n.set D_800D3368, s_rowArea + 0x68\n");
__asm__(".globl D_800D3410\n.set D_800D3410, s_rowArea + 0x110\n");
__asm__(".globl D_800D342E\n.set D_800D342E, s_rowArea + 0x12E\n");
__asm__(".globl D_800D3430\n.set D_800D3430, s_rowArea + 0x130\n");
__asm__(".globl D_800D343F\n.set D_800D343F, s_rowArea + 0x13F\n");
__asm__(".globl D_800D366C\n.set D_800D366C, s_rowArea + 0x36C\n");
__asm__(".globl D_800D3725\n.set D_800D3725, s_rowArea + 0x425\n");
extern u32 D_800D3344;
extern u32 D_800D3368[];
extern u8 D_800D3410[];
extern u8 D_800D342E;
extern u8 D_800D3430[];
extern u8 D_800D343F;
extern u8 D_800D366C;

enum { kRowAreaSize = (int)sizeof(s_rowArea) };

static u8 s_402fBuf[0x200];
static u8 s_ce4aBuf[0x800];
static u8 s_before402f[0x200];
static u8 s_expected402f[0x200];
static u8 s_before2C60[sizeof(D_800D2C60)];
static u8 s_expected2C60[sizeof(D_800D2C60)];
static u8 s_before2C8B[sizeof(D_800D2C8B)];
static u8 s_expected2C8B[sizeof(D_800D2C8B)];
static u8 s_beforeCe4a[0x800];
static u8 s_expectedCe4a[0x800];
__asm__(".globl D_800C402F\n.set D_800C402F, s_402fBuf + 0x100\n");
__asm__(".globl D_800CCE4A\n.set D_800CCE4A, s_ce4aBuf + 0x100\n");
static u8 s_bccBuf[0x40];
__asm__(".globl D_800C3BCC\n.set D_800C3BCC, s_bccBuf + 0x20\n");
extern u8 D_800C3BCC[];

static u8 s_ram[0x200000];
static u8 s_arena[0x800];
static u8 s_big[0xB000];
static u8* s_boardPtr;
static struct { u32 kind, args[2]; } s_log[8];
static unsigned s_logCount;
static unsigned s_checks;

static u8* mapped(u32 a, unsigned w)
{
    static const struct { u32 base; u32 size; u8* host; } kMap[] = {
        { 0x800D39CCu, 4, (u8*)&D_800D39CC },
        { 0x800C3B74u, 1, &D_800C3B74 },
        { 0x800C3D6Cu, 1, &D_800C3D6C },
        { 0x800D2D28u, 4, (u8*)&D_800D2D28 },
        { 0x800D2DC8u, 4, (u8*)&D_800D2DC8 },
        { 0x800C3610u, 4, (u8*)&D_800C3610 },
        { 0x800D2D40u, 4, (u8*)&D_800D2D40 },
        { 0x800D2D48u, 4, (u8*)&D_800D2D48 },
        { 0x800D39E0u, 2, (u8*)&D_800D39E0 },
        { 0x800D3278u, 4, (u8*)&D_800D3278 },
        { 0x8005A3A0u, (u32)sizeof(D_8005A3A0), (u8*)D_8005A3A0 },
        { 0x800D2E5Du, (u32)sizeof(s_e5dBuf) - 1, s_e5dBuf + 1 },
        { 0x800C3EA4u, 4, (u8*)&D_800C3EA4 },
        { 0x8005959Cu, 1, (u8*)&D_8005959C },
        { 0x800C34B0u, 4, (u8*)&D_800C34B0 },
        { 0x800C3EACu, 4, (u8*)&D_800C3EAC },
        { 0x800C3D60u, 4, (u8*)&D_800C3D60 },
        { 0x800C3E50u, 1, (u8*)&D_800C3E50 },
        { 0x800C3558u, 4, (u8*)&D_800C3558 },
        { 0x800C37C8u, 1, (u8*)&D_800C37C8 },
        { 0x800D367Cu, 4, (u8*)&D_800D367C },
        { 0x800C3DE8u, 4, (u8*)&D_800C3DE8 },
        { 0x800D2C8Bu, (u32)sizeof(D_800D2C8B), D_800D2C8B },
        { 0x800D2C60u, (u32)sizeof(D_800D2C60), (u8*)D_800D2C60 },
        { 0x8005919Cu, 4, (u8*)&D_8005919C },
        { 0x800C3E34u, 4, (u8*)&D_800C3E34 },
        { 0x800D3300u, (u32)sizeof(s_rowArea), s_rowArea },
        { 0x8006D634u + 0x1924u, 4, (u8*)g_GameState + 0x1924 },
        { 0x8006D634u, (u32)sizeof(g_GameState), (u8*)g_GameState },
        { 0x800C402Fu - 0x100u, (u32)sizeof(s_402fBuf), s_402fBuf },
        { 0x800CCE4Au - 0x100u, (u32)sizeof(s_ce4aBuf), s_ce4aBuf },
        { 0x800C3BCCu - 0x20u, 0x40, s_bccBuf },  /* the clear walks down */
    };
    unsigned i;
    uintptr_t lo = (uintptr_t)s_arena;

    for (i = 0; i < sizeof(kMap) / sizeof(kMap[0]); ++i) {
        if (a >= kMap[i].base && (uint64_t)a + w <= kMap[i].base + kMap[i].size) {
            return kMap[i].host + (a - kMap[i].base);
        }
    }
    if ((uintptr_t)a >= lo && (uint64_t)a + w <= lo + sizeof(s_arena)) {
        return (u8*)(uintptr_t)a;
    }
    {
        uintptr_t bLo = (uintptr_t)s_big;
        if ((uintptr_t)a >= bLo && (uint64_t)a + w <= bLo + sizeof(s_big)) {
            return (u8*)(uintptr_t)a;
        }
    }
    {
        uintptr_t pLo = (uintptr_t)&s_boardPtr;
        if ((uintptr_t)a >= pLo && (uint64_t)a + w <= pLo + sizeof(s_boardPtr)) {
            return (u8*)(uintptr_t)a;
        }
    }
    return NULL;
}

static int rd(void* u, u32 a, unsigned w, u32* v)
{
    u8* p = mapped(a, w);
    unsigned i;

    (void)u;
    if (p != NULL) {
        *v = 0;
        for (i = 0; i < w; ++i) {
            *v |= (u32)p[i] << (i * 8);
        }
        return 0;
    }
    if (a < 0x80000000u || (uint64_t)a + w > 0x80200000u) {
        return -1;
    }
    *v = 0;
    for (i = 0; i < w; ++i) {
        *v |= (u32)s_ram[(a + i) & 0x1FFFFFu] << (i * 8);
    }
    return 0;
}

static int wr(void* u, u32 a, unsigned w, u32 v)
{
    u8* p = mapped(a, w);
    unsigned i;

    (void)u;
    if (p != NULL) {
        for (i = 0; i < w; ++i) {
            p[i] = (u8)(v >> (i * 8));
        }
        return 0;
    }
    if (a < 0x80000000u || (uint64_t)a + w > 0x80200000u) {
        return -1;
    }
    for (i = 0; i < w; ++i) {
        s_ram[(a + i) & 0x1FFFFFu] = (u8)(v >> (i * 8));
    }
    return 0;
}

void LoadImage(void* pRect, void* pData)
{
    if (s_logCount < 8) {
        s_log[s_logCount].kind = 1;
        s_log[s_logCount].args[0] = (u32)(uintptr_t)pRect;
        s_log[s_logCount].args[1] = (u32)(uintptr_t)pData;
        s_logCount++;
    }
}

void DrawSync(s32 mode)
{
    if (s_logCount < 8) {
        s_log[s_logCount].kind = 2;
        s_log[s_logCount].args[0] = (u32)mode;
        s_log[s_logCount].args[1] = 0;
        s_logCount++;
    }
}

void func_80077074(void)
{
    if (s_logCount < 8) {
        s_log[s_logCount].kind = 4;
        s_log[s_logCount].args[0] = 0;
        s_log[s_logCount].args[1] = 0;
        s_logCount++;
    }
}

void func_80085454(s32 v)
{
    if (s_logCount < 8) {
        s_log[s_logCount].kind = 6;
        s_log[s_logCount].args[0] = (u32)v;
        s_log[s_logCount].args[1] = 0;
        s_logCount++;
    }
}

void func_80085618(s32 v)
{
    if (s_logCount < 8) {
        s_log[s_logCount].kind = 7;
        s_log[s_logCount].args[0] = (u32)v;
        s_log[s_logCount].args[1] = 0;
        s_logCount++;
    }
}

void func_80079ED8(u8 a0, u8 a1, u8 a2, s32 a3)
{
    if (s_logCount < 8) {
        s_log[s_logCount].kind = 8;
        s_log[s_logCount].args[0] = a0;
        s_log[s_logCount].args[1] = a1;
        s_logCount++;
    }
    if (s_logCount < 8) {
        s_log[s_logCount].kind = 9;
        s_log[s_logCount].args[0] = a2;
        s_log[s_logCount].args[1] = (u32)a3;
        s_logCount++;
    }
}

void func_8008FC1C(u32 a0, u32 a1, u32 a2, u32 a3, u32 a4)
{
    if (s_logCount < 8) {
        s_log[s_logCount].kind = 10;
        s_log[s_logCount].args[0] = a0;
        s_log[s_logCount].args[1] = a1;
        s_logCount++;
    }
    if (s_logCount < 8) {
        s_log[s_logCount].kind = 11;
        s_log[s_logCount].args[0] = a2;
        s_log[s_logCount].args[1] = a3;
        s_logCount++;
    }
    if (s_logCount < 8) {
        s_log[s_logCount].kind = 12;
        s_log[s_logCount].args[0] = a4;
        s_log[s_logCount].args[1] = 0;
        s_logCount++;
    }
}

static void log2(u32 kind, u32 a, u32 b)
{
    if (s_logCount < 8) {
        s_log[s_logCount].kind = kind;
        s_log[s_logCount].args[0] = a;
        s_log[s_logCount].args[1] = b;
        s_logCount++;
    }
}

void SetSemiTrans(void* p, s32 v)
{
    log2(13, (u32)(uintptr_t)p, (u32)v);
}

void SetShadeTex(void* p, s32 v)
{
    log2(14, (u32)(uintptr_t)p, (u32)v);
}

void HeapFree(u32 p)
{
    log2(15, p, 0);
}

void HeapChangeCurrentUser(s32 user, void* p)
{
    log2(20, (u32)user, (u32)(uintptr_t)p);
}

void* HeapAlloc(s32 size, s32 mode)
{
    log2(21, (u32)size, (u32)mode);
    return s_big + 0x1000;
}

void WorkListRemoveTask(u32 p)
{
    log2(22, p, 0);
}

void TimerWorkListRemoveTask(u32 p)
{
    log2(23, p, 0);
}

void func_80025180(u32 p)
{
    log2(24, p, 0);
}

void func_800245D8(u32 p, s32 v)
{
    log2(25, p, (u32)v);
}

void func_80021BF8(u32 p, u32 v)
{
    log2(26, p, v);
}

void func_8008FAD8(void)
{
    log2(16, 0, 0);
}

void func_801DE594(void)
{
    log2(17, 0, 0);
}

void func_80073FB8(void)
{
    log2(18, 0, 0);
}

void func_80073538(void)
{
    log2(27, 0, 0);
}

void func_80073F08(void)
{
    log2(28, 0, 0);
}

void func_8007500C(void)
{
    log2(29, 0, 0);
}

void func_80074F70(void)
{
    log2(30, 0, 0);
}

void func_80088B80(void)
{
    log2(31, 0, 0);
}

void func_80074AB8(void)
{
    log2(32, 0, 0);
}

void func_8007A280(u8 a0, u8 a1, u32 a2, u32 a3)
{
    log2(33, a0, (a1 << 16) | (a2 & 0xFFFF));
    log2(34, a3, 0);
}

void func_800785D4(u8 a, u8 b)
{
    log2(36, a, b);
}

void func_800BC2F0(s32 v)
{
    log2(37, (u32)v, 0);
}

void func_800BFC80(u32 a0, u32 a1, u32 a2)
{
    log2(38, a0, (a1 & 0xFFFF) | (a2 << 16));
}

void func_80078658(u8 a, u8 b)
{
    log2(39, a, b);
}

void func_800787E0(u8 a, u8 b)
{
    log2(40, a, b);
}

void func_8007887C(u8 a)
{
    log2(41, a, 0);
}

void func_800BCD98(u32 v)
{
    log2(42, v, 0);
}

/* func_8008FA60 is now a landed C body (main46.c): the bridge below runs the
 * shipped code, so the old log stub had to go. */
void func_800BC460(u32 p)
{
    log2(44, p, 0);
}

void func_80039DB8(u32 v)
{
    log2(45, v, 0);
}

void func_8002A498(u32 v)
{
    log2(46, v, 0);
}

/* Production mainc104.c now owns the real func_800B8D04 (its callees are
 * outside this test's link); keep the call-recording spy behind --wrap. */
void __wrap_func_800B8D04(void)
{
    log2(47, 0, 0);
}

/* func_800BC404 graduated from INCLUDE_ASM to a C body in mainc114.c whose
 * unconditional D_80059454 store matches retail (see
 * asm/battle/matchings/mainc114/func_800BC404.s). Use production directly;
 * the old in-test model wrongly gated the store on D_800C37C8. */

void func_800716D8(void)
{
    log2(19, 0, 0);
}

s32 ArchiveDataSync(void)
{
    s_asmSyncCalls++;
    return s_asmSyncCalls < 3;
}

static int bridge(void* u, PcPortMipsCpu* c, u32 t)
{
    (void)u;
    if (t == 0x80044894u) {          /* LoadImage */
        LoadImage((void*)(uintptr_t)c->gpr[4], (void*)(uintptr_t)c->gpr[5]);
        return 1;
    }
    if (t == 0x800445D0u) {          /* DrawSync */
        DrawSync((s32)c->gpr[4]);
        return 1;
    }
    if (t == 0x8003F8E8u) {          /* bzero */
        bzero((void*)(uintptr_t)c->gpr[4], (s32)c->gpr[5]);
        return 1;
    }
    if (t == 0x80077074u) {
        func_80077074();
        return 1;
    }
    /* 0x8008AA40 is a landed C body and a case entry: it must stay unbridged
     * so the oracle executes the resident retail slice (a bridged entry would
     * make the oracle call the host C and hide mutants). */
    if (t == 0x80085454u) {
        func_80085454((s32)c->gpr[4]);
        return 1;
    }
    if (t == 0x80085618u) {
        func_80085618((s32)c->gpr[4]);
        return 1;
    }
    if (t == 0x80079ED8u) {
        func_80079ED8((u8)c->gpr[4], (u8)c->gpr[5], (u8)c->gpr[6],
                      (s32)c->gpr[7]);
        return 1;
    }
    if (t == 0x8008FC1Cu) {
        u32 stackArg = 0;

        (void)rd(u, c->gpr[29] + 0x10, 4, &stackArg);
        func_8008FC1C(c->gpr[4], c->gpr[5], c->gpr[6], c->gpr[7], stackArg);
        return 1;
    }
    if (t == 0x80043BFCu) {
        SetSemiTrans((void*)(uintptr_t)c->gpr[4], (s32)c->gpr[5]);
        return 1;
    }
    if (t == 0x80043C24u) {
        SetShadeTex((void*)(uintptr_t)c->gpr[4], (s32)c->gpr[5]);
        return 1;
    }
    if (t == 0x800320E8u) {
        HeapFree(c->gpr[4]);
        return 1;
    }
    if (t == 0x8008FAD8u) {
        func_8008FAD8();
        return 1;
    }
    if (t == 0x801DE594u) {
        func_801DE594();
        return 1;
    }
    if (t == 0x80073FB8u) {
        func_80073FB8();
        return 1;
    }
    if (t == 0x800716D8u) {
        func_800716D8();
        return 1;
    }
    if (t == 0x800286CCu) {
        c->gpr[2] = (u32)ArchiveDataSync();
        return 1;
    }
    if (t == 0x80032498u) {
        HeapChangeCurrentUser((s32)c->gpr[4], (void*)(uintptr_t)c->gpr[5]);
        return 1;
    }
    if (t == 0x80031BDCu) {
        c->gpr[2] = (u32)(uintptr_t)HeapAlloc((s32)c->gpr[4],
                                              (s32)c->gpr[5]);
        return 1;
    }
    if (t == 0x8001CB48u) {
        WorkListRemoveTask(c->gpr[4]);
        return 1;
    }
    if (t == 0x8001CD94u) {
        TimerWorkListRemoveTask(c->gpr[4]);
        return 1;
    }
    if (t == 0x80025180u) {
        func_80025180(c->gpr[4]);
        return 1;
    }
    if (t == 0x800245D8u) {
        func_800245D8(c->gpr[4], (s32)c->gpr[5]);
        return 1;
    }
    if (t == 0x80021BF8u) {
        func_80021BF8(c->gpr[4], c->gpr[5]);
        return 1;
    }
    if (t == 0x80073538u || t == 0x80073F08u || t == 0x8007500Cu ||
        t == 0x80074F70u || t == 0x80088B80u || t == 0x80074AB8u) {
        if (t == 0x80073538u) {
            func_80073538();
        } else if (t == 0x80073F08u) {
            func_80073F08();
        } else if (t == 0x8007500Cu) {
            func_8007500C();
        } else if (t == 0x80074F70u) {
            func_80074F70();
        } else if (t == 0x80088B80u) {
            func_80088B80();
        } else {
            func_80074AB8();
        }
        return 1;
    }
    if (t == 0x8007A280u) {
        func_8007A280((u8)c->gpr[4], (u8)c->gpr[5], c->gpr[6], c->gpr[7]);
        return 1;
    }
    if (t == 0x80089C9Cu) {
        c->gpr[2] = func_80089C9C(c->gpr[4], c->gpr[5]);
        return 1;
    }
    if (t == 0x800785D4u) {
        func_800785D4((u8)c->gpr[4], (u8)c->gpr[5]);
        return 1;
    }
    if (t == 0x800BC2F0u) {
        func_800BC2F0((s32)c->gpr[4]);
        return 1;
    }
    if (t == 0x800BFC80u) {
        func_800BFC80(c->gpr[4], c->gpr[5], c->gpr[6]);
        return 1;
    }
    if (t == 0x80078658u) {
        func_80078658((u8)c->gpr[4], (u8)c->gpr[5]);
        return 1;
    }
    if (t == 0x800787E0u) {
        func_800787E0((u8)c->gpr[4], (u8)c->gpr[5]);
        return 1;
    }
    if (t == 0x8007887Cu) {
        func_8007887C((u8)c->gpr[4]);
        return 1;
    }
    if (t == 0x80089C08u) {
        c->gpr[2] = (u32)func_80089C08((u8)c->gpr[4]);
        return 1;
    }
    if (t == 0x800BC404u) {
        func_800BC404(c->gpr[4]);
        return 1;
    }
    if (t == 0x800BCD98u) {
        func_800BCD98(c->gpr[4]);
        return 1;
    }
    if (t == 0x8008FA60u) {
        func_8008FA60(c->gpr[4]);
        return 1;
    }
    if (t == 0x800BC460u) {
        func_800BC460(c->gpr[4]);
        return 1;
    }
    if (t == 0x80039DB8u) {
        func_80039DB8(c->gpr[4]);
        return 1;
    }
    if (t == 0x8002A498u) {
        func_8002A498(c->gpr[4]);
        return 1;
    }
    if (t == 0x800B8D04u) {
        func_800B8D04();
        return 1;
    }
    return 0;
}

/* kind: 0 void(void), 1 u32(void), 2 void(u32), 3 void(u32*) */
static u32 run_oracle(u32 entry, u32 a0, u32 a1)
{
    PcPortMipsBus bus;
    PcPortMipsCpu cpu;

    bus.read = rd;
    bus.write = wr;
    bus.bridge = bridge;
    PcPortMipsCpuInit(&cpu, &bus);
    cpu.gpr[4] = a0;
    cpu.gpr[5] = a1;
    cpu.gpr[29] = 0x801FF000u;
    cpu.gpr[31] = 0xFFFFFFFCu;
    if (PcPortMipsRun(&cpu, entry, 0xFFFFFFFCu, 2000) != PC_PORT_MIPS_HALTED) {
        fprintf(stderr, "TINY LEAF FAIL oracle %s @%08x\n", cpu.error, entry);
        exit(1);
    }
    return cpu.gpr[2];
}

static void expect_eq(const char* field, u32 actual, u32 expected)
{
    s_checks++;
    if (actual != expected) {
        fprintf(stderr, "TINY LEAF FAIL %s actual=%08x expected=%08x\n", field,
                actual, expected);
        exit(1);
    }
}

int main(void)
{
    static const struct { u32 addr, size; s32 kind; const char* name; } kCases[] = {
        { 0x8009795Cu, 0x40, 0, "func_8009795C" },
        { 0x8009F5B0u, 0x40, 0, "func_8009F5B0" },
        { 0x800B3348u, 0x40, 0, "func_800B3348" },
        { 0x800B3350u, 0x40, 0, "func_800B3350" },
        { 0x800B89F4u, 0x40, 0, "func_800B89F4" },
        { 0x800BAF40u, 0x40, 0, "func_800BAF40" },
        { 0x800BDD34u, 0x40, 0, "func_800BDD34" },
        { 0x80077980u, 0x40, 0, "func_80077980" },
        { 0x8009E268u, 0x40, 0, "func_8009E268" },
        { 0x800A577Cu, 0x40, 1, "func_800A577C" },
        { 0x800A578Cu, 0x40, 1, "func_800A578C" },
        { 0x800BF0B4u, 0x40, 2, "func_800BF0B4" },
        { 0x80079934u, 0x40, 3, "func_80079934" },
        { 0x800AA788u, 0x40, 2, "func_800AA788" },
        { 0x800B14B8u, 0x40, 0, "func_800B14B8" },
        { 0x800A5E9Cu, 0x40, 4, "func_800A5E9C" },
        { 0x800B00D0u, 0x40, 5, "func_800B00D0" },
        { 0x800769E8u, 0x40, 6, "func_800769E8" },
        { 0x8007A9A8u, 0x40, 7, "func_8007A9A8" },
        { 0x8007E7C0u, 0x40, 7, "func_8007E7C0" },
        { 0x80078D48u, 0x40, 8, "func_80078D48" },
        { 0x80080C6Cu, 0x40, 9, "func_80080C6C" },
        { 0x800B168Cu, 0x40, 10, "func_800B168C" },
        { 0x8007A900u, 0x40, 11, "func_8007A900" },
        { 0x8008BC40u, 0x58, 12, "func_8008BC40" },
        { 0x8008CCCCu, 0x5C, 12, "func_8008CCCC" },
        { 0x8008B108u, 0x60, 12, "func_8008B108" },
        { 0x8008AA74u, 0x2C, 13, "func_8008AA74" },
        { 0x80077610u, 0x4C, 14, "func_80077610" },
        { 0x8007A8B4u, 0x4C, 15, "func_8007A8B4" },
        { 0x80085C88u, 0x44, 16, "func_80085C88" },
        { 0x800AA760u, 0x28, 17, "func_800AA760" },
        { 0x8007B3B0u, 0x34, 18, "func_8007B3B0" },
        { 0x8007BA88u, 0x30, 19, "func_8007BA88" },
        { 0x8007BAB8u, 0x30, 19, "func_8007BAB8" },
        { 0x800B6A50u, 0x2C, 20, "func_800B6A50" },
        { 0x8007D1A8u, 0x34, 21, "func_8007D1A8" },
        { 0x8008FDE4u, 0x34, 22, "func_8008FDE4" },
        { 0x800764B4u, 0x38, 23, "func_800764B4" },
        { 0x80076AC8u, 0x38, 24, "func_80076AC8" },
        { 0x8007765Cu, 0x3C, 25, "func_8007765C" },
        { 0x8007AAB8u, 0x3C, 26, "func_8007AAB8" },
        { 0x8008AC50u, 0x38, 27, "func_8008AC50" },
        { 0x8009AB00u, 0x38, 28, "func_8009AB00" },
        { 0x800B7330u, 0x34, 29, "func_800B7330" },
        { 0x8008ABB8u, 0x48, 30, "func_8008ABB8" },
        { 0x800A22A8u, 0x40, 31, "func_800A22A8" },
        { 0x800A2D1Cu, 0x40, 32, "func_800A2D1C" },
        { 0x800B7364u, 0x3C, 33, "func_800B7364" },
        { 0x800B9020u, 0x3C, 34, "func_800B9020" },
        { 0x8008AC00u, 0x50, 35, "func_8008AC00" },
        { 0x80076B68u, 0x44, 36, "func_80076B68" },
        { 0x80076BACu, 0x44, 37, "func_80076BAC" },
        { 0x80076BF0u, 0x44, 38, "func_80076BF0" },
        { 0x80076C34u, 0x44, 39, "func_80076C34" },
        { 0x80079054u, 0x44, 40, "func_80079054" },
        { 0x8007AF5Cu, 0x50, 41, "func_8007AF5C" },
        { 0x8007AFACu, 0x50, 42, "func_8007AFAC" },
        { 0x80079114u, 0x58, 43, "func_80079114" },
        { 0x800764ECu, 0x58, 44, "func_800764EC" },
        { 0x80079E18u, 0x34, 45, "func_80079E18" },
        { 0x80079E4Cu, 0x30, 46, "func_80079E4C" },
        { 0x80079E7Cu, 0x5C, 47, "func_80079E7C" },
        { 0x80078C9Cu, 0x50, 48, "func_80078C9C" },
        { 0x8009A7B8u, 0x2C, 49, "func_8009A7B8" },
        { 0x8009E3C8u, 0x48, 50, "func_8009E3C8" },
        { 0x800A3484u, 0x0C, 51, "func_800A3484" },
        { 0x800AEEECu, 0x0C, 52, "func_800AEEEC" },
        { 0x800B3B6Cu, 0x28, 53, "func_800B3B6C" },
        { 0x800BCAA4u, 0x2C, 54, "func_800BCAA4" },
        { 0x800BFD88u, 0x20, 55, "func_800BFD88" },
        { 0x8007FCE8u, 0x50, 56, "func_8007FCE8" },
        { 0x8007FDECu, 0x50, 57, "func_8007FDEC" },
        { 0x80078CECu, 0x5C, 58, "func_80078CEC" },
        { 0x8007893Cu, 0x5C, 59, "func_8007893C" },
        { 0x8008B168u, 0xBC, 60, "func_8008B168" },
        { 0x8008BC98u, 0xB8, 61, "func_8008BC98" },
        { 0x8008CD28u, 0xBC, 62, "func_8008CD28" },
        { 0x8008C360u, 0x90, 63, "func_8008C360" },
        { 0x8008C3F0u, 0xB8, 64, "func_8008C3F0" },
        { 0x8007AB30u, 0x38, 65, "func_8007AB30" },
        { 0x8007AB68u, 0x38, 66, "func_8007AB68" },
        { 0x8007ABA0u, 0x38, 67, "func_8007ABA0" },
        { 0x8007E674u, 0x2C, 68, "func_8007E674" },
        { 0x8008AA40u, 0x34, 69, "func_8008AA40" },
        { 0x8009E508u, 0x34, 70, "func_8009E508" },
        { 0x800B8D7Cu, 0x28, 71, "func_800B8D7C" },
        { 0x800BCAD0u, 0x2C, 72, "func_800BCAD0" },
    };
    FILE* f;
    unsigned i;
    static const u32 kArgs[] = { 0x00000000u, 0x00000001u, 0x00000007u, 0x000000FFu };
    unsigned a;

    f = fopen("disc/battle.bin", "rb");
    if (f == NULL) {
        fprintf(stderr, "TINY LEAF FAIL cannot open disc/battle.bin\n");
        return 1;
    }
    for (i = 0; i < sizeof(kCases) / sizeof(kCases[0]); ++i) {
        if (fseek(f, (long)(kCases[i].addr - 0x8006FAF0), SEEK_SET) != 0 ||
            fread(s_ram + (kCases[i].addr & 0x1FFFFFu), 1, kCases[i].size, f) !=
                kCases[i].size) {
            fprintf(stderr, "TINY LEAF FAIL cannot read %s\n", kCases[i].name);
            return 1;
        }
    }
    fclose(f);

    /* The retail allocator wrapper is also callable from other cases (the
     * 0x670 parking case), so keep its slice resident instead of bridging the
     * entry (bridging it would make the oracle run the host C). */
    {
        FILE* g = fopen("disc/battle.bin", "rb");
        static const struct { u32 addr, size; } kResident[] = {
            { 0x8008ABB8u, 0x48 },   /* allocator wrapper */
            { 0x80076AC8u, 0x38 },   /* semi-transparency helper */
            { 0x8007765Cu, 0x3C },   /* parked-block free */
            { 0x80077980u, 0x10 },   /* sprite-global clear */
        };
        unsigned r;

        for (r = 0; r < sizeof(kResident) / sizeof(kResident[0]); ++r) {
            if (g == NULL ||
                fseek(g, (long)(kResident[r].addr - 0x8006FAF0), SEEK_SET) != 0 ||
                fread(s_ram + (kResident[r].addr & 0x1FFFFFu), 1,
                      kResident[r].size, g) != kResident[r].size) {
                fprintf(stderr, "TINY LEAF FAIL cannot read resident slice %u\n", r);
                return 1;
            }
        }
        fclose(g);
    }

    for (a = 0; a < 4; ++a) {
        for (i = 0; i < sizeof(kCases) / sizeof(kCases[0]); ++i) {
            u32 arg = kArgs[a];
            u32 idx = arg & 7u;
            u8 before[sizeof(s_arena)];
            u8 expected[sizeof(s_arena)];
            u32 gExpected[7];
            u8 expectedBcc[sizeof(s_bccBuf)];
            u16 expectedTable[sizeof(D_8005A3A0) / 2];
            u8 expectedE62[kE62Size];
            static u8 beforeRow[kRowAreaSize];
            static u8 expectedRow[kRowAreaSize];
            static u8 beforeBig[sizeof(s_big)];
            static u8 expectedBig[sizeof(s_big)];
            u32 gExpectedG;
            u32 gExpectedH;
            struct { u32 kind, args[2]; } expectedLog[8];
            unsigned expectLogCount;
            u32 cRetPtr = 0;
            u32 oracleRet;
            u32 cRet = 0;
            u32 pArena = (u32)(uintptr_t)s_arena;


            D_800D39CC = 0x22220000u + arg;
            D_800C3B74 = 0x5A;
            D_800C3D6C = 0x5A;
            D_800D2D28 = s_arena + 0x40;
            D_800D2DC8 = s_arena + 0x80;
            D_800C3610 = s_arena;
            D_800D3278 = s_arena + 0x100;
            s_boardPtr = s_arena + 0x180;
            s_arena[0x181] = (u8)(arg & 7);
            s_arena[0x182] = (u8)((arg + 2) & 7);
            D_800D2D40 = 0x5A5A0000u + arg;
            D_800D2D48 = 0x5A5A0100u + arg;
            D_800D39E0 = (u16)(0x5A00u + arg);
            memset(s_arena, 0x33, sizeof(s_arena));
            memset(s_bccBuf, 0x33, sizeof(s_bccBuf));
            memset(D_8005A3A0, 0x33, sizeof(D_8005A3A0));
            {
                unsigned k;

                for (k = 0; k < kE62Size; ++k) {
                    D_800D2E62[k] = (u8)(0x40 + k);
                }
                for (k = 0; k < kRowAreaSize; ++k) {
                    s_rowArea[k] = (u8)(0x10 + k);
                }
            }
            D_800D3344 = 0x11110000u + arg;
            D_800D366C = (u8)(arg & 1u);
            D_800C3EA4 = (u32)(uintptr_t)s_big;
            D_8005959C = 0x5A;
            D_800C34B0 = s_big + 0x400;
            D_800C3EAC = s_big + 0x400;
            D_800C3D60 = (u32)(uintptr_t)(s_big + 0x800);
            D_800C3E50 = 2;
            D_800C3558 = (u32)(uintptr_t)(s_big + 0x200);
            D_800C37C8 = (u8)(arg & 1u);
            D_800D367C = (u32)(uintptr_t)(s_big + 0x2100);
            D_800C3DE8 = (u32)(uintptr_t)(s_big + 0x2200);
            D_8005919C = (u32)(uintptr_t)(s_big + 0x300);
            D_800C3E34 = s_big + 0x320;
            *(u16*)(s_big + 0x300 + 0x14) = 0x1234;
            *(u16*)(s_big + 0x320 + 0x7A) = 0xFFFF;
            *(u16*)(D_800D2DC8 + 0x7C) = 0xFFFF;
            {
                unsigned k2;

                for (k2 = 0; k2 < 0x10; ++k2) {
                    D_800D2C60[k2] = 0xFFFF;
                    D_800D2C8B[k2] = 0x55;
                }
            }
            D_800D2D28[0xAE] = 1;
            D_800D2D28[0x96] = 1;
            s_asmSyncCalls = 0;
            memset(s_big, 0x44, sizeof(s_big));
            {
                unsigned k;

                for (k = 0; k < 0x10; ++k) {
                    D_800D3368[k] = (u32)(uintptr_t)(s_big + 0x2000);
                }
                s_big[0x2000 + 0x2A] = 0x77;
                *(u8**)(s_big + 0x2000 + 0x70) = s_big + 0x3000;
                *(u32*)(s_big + 0x300C) = 0x11112222u;
                *(u32*)(s_big + 0x3010) = 0x33334444u;
                *(u32*)(s_big + 0x3014) = 0x55556666u;
                *(u32*)(g_GameState + 0x1924) = 0x5A5A0000u + arg;
                for (k = 0; k < sizeof(g_GameState); ++k) {
                    g_GameState[k] = (u8)(k * 7 + 3);
                }
                *(u32*)(g_GameState + 0x1924) = 0x5A5A0000u + arg;
                *(D_800C34B0 + 0x15A) = 0x5B;
                *(u32*)(s_big + 0x1800) = (u32)(uintptr_t)(s_big + 0x1900);
                *(u16*)(s_big + 0x1804) = 7;
                *(u16*)(s_big + 0x1806) = 3;
                *(s8*)(s_big + 0x1B00 + 0xB0) = -5;
                for (k = 0; k < 0x40; ++k) {
                    D_800D2E5D[k] = (u8)(0x20 + k);
                    D_800D2E60[k] = (u8)(0x60 + k);
                }
                memset(s_big + 0x1C00, 0x22, 0x40);
                *(u8*)(s_big + 0x400 + 0x2DA) = 2;
                *(u8*)(s_big + 0x200 + 0x41) = 0x5A;
                for (k = 0; k < 0x40; ++k) {
                    D_800C3448[k] = 0;
                }
            }
            s_logCount = 0;
            memcpy(before, s_arena, sizeof(s_arena));
            memcpy(beforeRow, s_rowArea, kRowAreaSize);
            memcpy(beforeBig, s_big, sizeof(s_big));
            memset(s_402fBuf, 0x55, sizeof(s_402fBuf));
            memset(s_ce4aBuf, 0x55, sizeof(s_ce4aBuf));
            memcpy(s_before402f, s_402fBuf, sizeof(s_402fBuf));
            memcpy(s_beforeCe4a, s_ce4aBuf, sizeof(s_ce4aBuf));
            memcpy(s_before2C60, D_800D2C60, sizeof(D_800D2C60));
            memcpy(s_before2C8B, D_800D2C8B, sizeof(D_800D2C8B));

            /* kinds 3/6/7/10 take pointers; the rest take the scalar */
            oracleRet = run_oracle(kCases[i].addr,
                                   kCases[i].kind == 3 ? (pArena + 0x20) :
                                   kCases[i].kind == 6 ? pArena :
                                   kCases[i].kind == 7 ? (u32)(uintptr_t)&s_boardPtr :
                                   kCases[i].kind == 11 ? (u32)(uintptr_t)&s_boardPtr :
                                   kCases[i].kind == 15 ? (u32)(uintptr_t)&s_boardPtr :
                                   kCases[i].kind == 16 ? (u32)(uintptr_t)idx :
                                   kCases[i].kind == 17 ? idx :
                                   kCases[i].kind == 18 ? (u32)(uintptr_t)&s_boardPtr :
                                   kCases[i].kind == 19 ? idx :
                                   kCases[i].kind == 20 ? (u32)(uintptr_t)(s_big + 0x2000) :
                                   kCases[i].kind == 21 ? (u32)(uintptr_t)&s_boardPtr :
                                   kCases[i].kind == 24 ? (u32)(uintptr_t)(s_big + 0x800) :
                                   kCases[i].kind == 26 ? (u32)(uintptr_t)&s_boardPtr :
                                   kCases[i].kind == 28 ? idx :
                                   kCases[i].kind == 29 ? (u32)(uintptr_t)(s_big + 0x800) :
                                   kCases[i].kind == 31 ? (u32)(uintptr_t)(s_big + 0x1800) :
                                   kCases[i].kind == 32 ? (u32)(uintptr_t)(s_big + 0x1800) :
                                   kCases[i].kind == 33 ? (u32)(uintptr_t)(s_big + 0x1A00) :
                                   kCases[i].kind == 34 ? (u32)(uintptr_t)(s_big + 0x1B00) :
                                   kCases[i].kind == 30 ? 0x670 :
                                   kCases[i].kind == 35 ? 0x101 :
                                   kCases[i].kind == 36 ? (u32)(uintptr_t)(s_big + 0x1C00) :
                                   kCases[i].kind == 37 ? (u32)(uintptr_t)(s_big + 0x1C00) :
                                   kCases[i].kind == 38 ? (u32)(uintptr_t)(s_big + 0x1C00) :
                                   kCases[i].kind == 39 ? (u32)(uintptr_t)(s_big + 0x1C00) :
                                   kCases[i].kind == 41 ? (u32)(uintptr_t)&s_boardPtr :
                                   kCases[i].kind == 42 ? (u32)(uintptr_t)&s_boardPtr :
                                   kCases[i].kind == 45 ? idx :
                                   kCases[i].kind == 46 ? idx :
                                   kCases[i].kind == 47 ? 0x1234 :
                                   kCases[i].kind == 48 ? arg :
                                   kCases[i].kind == 49 ? idx :
                                   kCases[i].kind == 51 ? (u32)(uintptr_t)(s_big + 0x1D00) :
                                   kCases[i].kind == 52 ? (u32)(uintptr_t)(s_big + 0x1E00) :
                                   kCases[i].kind == 55 ? arg :
                                   kCases[i].kind == 58 ? arg :
                                   kCases[i].kind == 59 ? idx :
                                   kCases[i].kind == 60 ? idx :
                                   kCases[i].kind == 61 ? idx :
                                   kCases[i].kind == 62 ? idx :
                                   kCases[i].kind == 63 ? (arg & 1u) :
                                   kCases[i].kind == 64 ? idx :
                                   kCases[i].kind == 65 ? (u32)(uintptr_t)&s_boardPtr :
                                   kCases[i].kind == 66 ? (u32)(uintptr_t)&s_boardPtr :
                                   kCases[i].kind == 67 ? (u32)(uintptr_t)&s_boardPtr :
                                   kCases[i].kind == 68 ? (arg & 7u) :
                                   kCases[i].kind == 69 ? arg :
                                   kCases[i].kind == 72 ? 0 :
                                   kCases[i].kind == 9 ? (idx & 3u) :
                                   kCases[i].kind == 10 ? (pArena + 0x20) : arg,
                                   kCases[i].kind == 4 ? (arg + 1) :
                                   kCases[i].kind == 6 ? (pArena + 0x20) :
                                   kCases[i].kind == 15 ? (pArena + 0x20) :
                                   kCases[i].kind == 8 ? idx :
                                   kCases[i].kind == 21 ? idx :
                                   kCases[i].kind == 26 ? idx :
                                   kCases[i].kind == 30 ? 0 :
                                   kCases[i].kind == 40 ? idx :
                                   kCases[i].kind == 41 ? idx :
                                   kCases[i].kind == 42 ? idx :
                                   kCases[i].kind == 43 ? idx :
                                   kCases[i].kind == 48 ? idx :
                                   kCases[i].kind == 55 ? idx :
                                   kCases[i].kind == 58 ? idx :
                                   kCases[i].kind == 59 ? idx :
                                   kCases[i].kind == 65 ? idx :
                                   kCases[i].kind == 66 ? idx :
                                   kCases[i].kind == 67 ? idx :
                                   kCases[i].kind == 11 ? idx : arg);
            memcpy(expected, s_arena, sizeof(s_arena));
            gExpected[0] = D_800D3344;
            gExpected[1] = D_800D39CC;
            gExpected[2] = D_800C3B74;
            gExpected[3] = D_800C3D6C;
            gExpected[4] = D_800D2D40;
            gExpected[5] = D_800D2D48;
            gExpected[6] = D_800D39E0;
            memcpy(expectedBcc, s_bccBuf, sizeof(s_bccBuf));
            memcpy(expectedTable, D_8005A3A0, sizeof(D_8005A3A0));
            memcpy(expectedE62, D_800D2E62, kE62Size);
            memcpy(expectedRow, s_rowArea, kRowAreaSize);
            memcpy(expectedBig, s_big, sizeof(s_big));
            memcpy(s_expected402f, s_402fBuf, sizeof(s_402fBuf));
            memcpy(s_expectedCe4a, s_ce4aBuf, sizeof(s_ce4aBuf));
            memcpy(s_expected2C60, D_800D2C60, sizeof(D_800D2C60));
            memcpy(s_expected2C8B, D_800D2C8B, sizeof(D_800D2C8B));
            gExpectedG = D_800D366C;
            gExpectedH = D_8005959C;
            memcpy(expectedLog, s_log, sizeof(s_log));
            expectLogCount = s_logCount;

            memcpy(s_arena, before, sizeof(s_arena));
            memcpy(s_rowArea, beforeRow, kRowAreaSize);
            memcpy(s_big, beforeBig, sizeof(s_big));
            memcpy(s_402fBuf, s_before402f, sizeof(s_402fBuf));
            memcpy(s_ce4aBuf, s_beforeCe4a, sizeof(s_ce4aBuf));
            memcpy(D_800D2C60, s_before2C60, sizeof(D_800D2C60));
            memcpy(D_800D2C8B, s_before2C8B, sizeof(D_800D2C8B));
            s_asmSyncCalls = 0;
            s_logCount = 0;
            switch (kCases[i].kind) {
            case 0:
                if (kCases[i].addr == 0x80077980u) {
                    func_80077980();
                } else if (kCases[i].addr == 0x8009E268u) {
                    func_8009E268();
                } else if (kCases[i].addr == 0x800B14B8u) {
                    func_800B14B8();
                } else if (kCases[i].addr == 0x8009795Cu) {
                    func_8009795C();
                } else if (kCases[i].addr == 0x8009F5B0u) {
                    func_8009F5B0();
                } else if (kCases[i].addr == 0x800B3348u) {
                    func_800B3348();
                } else if (kCases[i].addr == 0x800B3350u) {
                    func_800B3350();
                } else if (kCases[i].addr == 0x800B89F4u) {
                    func_800B89F4();
                } else if (kCases[i].addr == 0x800BAF40u) {
                    func_800BAF40();
                } else {
                    func_800BDD34();
                }
                break;
            case 1:
                cRet = (kCases[i].addr == 0x800A577Cu) ? func_800A577C()
                                                       : func_800A578C();
                break;
            case 2:
                if (kCases[i].addr == 0x800BF0B4u) {
                    func_800BF0B4(arg);
                } else {
                    func_800AA788(arg);
                }
                break;
            case 3:
                func_80079934((u32*)(uintptr_t)(pArena + 0x20));
                break;
            case 4:
                func_800A5E9C(arg, arg + 1);
                break;
            case 5:
                func_800B00D0();
                break;
            case 6:
                func_800769E8((void*)(uintptr_t)pArena, (void*)(uintptr_t)(pArena + 0x20));
                break;
            case 7:
                if (kCases[i].addr == 0x8007A9A8u) {
                    func_8007A9A8(&s_boardPtr);
                } else {
                    func_8007E7C0(&s_boardPtr);
                }
                break;
            case 8:
                func_80078D48(arg, (u8)idx);
                break;
            case 9:
                func_80080C6C((u8)(idx & 3u));
                break;
            case 10:
                cRetPtr = (u32)(uintptr_t)func_800B168C(s_arena + 0x20, arg);
                break;
            case 11:
                func_8007A900(&s_boardPtr, (u8)idx);
                break;
            case 12:
                if (kCases[i].addr == 0x8008BC40u) {
                    func_8008BC40((u8)arg);
                } else if (kCases[i].addr == 0x8008CCCCu) {
                    func_8008CCCC((u8)arg);
                } else {
                    func_8008B108((u8)arg);
                }
                break;
            case 13:
                func_8008AA74(arg);
                break;
            case 14:
                func_80077610();
                break;
            case 15:
                func_8007A8B4(&s_boardPtr, (u8*)(uintptr_t)(pArena + 0x20));
                break;
            case 16:
                func_80085C88((u8)idx);
                break;
            case 17:
                func_800AA760(idx, (u8)arg);
                break;
            case 18:
                func_8007B3B0(&s_boardPtr, (u8)arg);
                break;
            case 19:
                if (kCases[i].addr == 0x8007BA88u) {
                    func_8007BA88((u8)idx);
                } else {
                    func_8007BAB8((u8)idx);
                }
                break;
            case 20:
                func_800B6A50(s_big + 0x2000);
                break;
            case 21:
                func_8007D1A8(&s_boardPtr, (u8)idx);
                break;
            case 22:
                func_8008FDE4();
                break;
            case 23:
                func_800764B4();
                break;
            case 24:
                func_80076AC8(s_big + 0x800);
                break;
            case 25:
                func_8007765C();
                break;
            case 26:
                func_8007AAB8(&s_boardPtr, (u8)idx);
                break;
            case 27:
                func_8008AC50();
                break;
            case 28:
                func_8009AB00((u8)idx);
                break;
            case 29:
                func_800B7330(s_big + 0x800);
                break;
            case 30:
                cRetPtr = (u32)(uintptr_t)func_8008ABB8(0x670, 0);
                break;
            case 31:
                func_800A22A8(s_big + 0x1800);
                break;
            case 32:
                func_800A2D1C(s_big + 0x1800);
                break;
            case 33:
                func_800B7364((u32)(uintptr_t)(s_big + 0x1A00));
                break;
            case 34:
                func_800B9020(s_big + 0x1B00);
                break;
            case 35:
                cRetPtr = (u32)(uintptr_t)func_8008AC00(0x101);
                break;
            case 36:
                func_80076B68(s_big + 0x1C00);
                break;
            case 37:
                func_80076BAC(s_big + 0x1C00);
                break;
            case 38:
                func_80076BF0(s_big + 0x1C00);
                break;
            case 39:
                func_80076C34(s_big + 0x1C00);
                break;
            case 40:
                func_80079054((u8)arg, (u8)idx);
                break;
            case 41:
                func_8007AF5C(&s_boardPtr, (u8)idx);
                break;
            case 42:
                func_8007AFAC(&s_boardPtr, (u8)idx);
                break;
            case 43:
                func_80079114((u8)arg, (u8)idx);
                break;
            case 44:
                func_800764EC();
                break;
            case 45:
                func_80079E18((u8)idx);
                break;
            case 46:
                func_80079E4C((u8)idx);
                break;
            case 47:
                cRet = func_80079E7C(0x1234);
                break;
            case 48:
                func_80078C9C((u8)arg, (u8)idx);
                break;
            case 49:
                cRet = func_8009A7B8((u8)idx);
                break;
            case 50:
                func_8009E3C8();
                break;
            case 51:
                func_800A3484((s16*)(s_big + 0x1D00));
                break;
            case 52:
                func_800AEEEC(s_big + 0x1E00);
                break;
            case 53:
                cRet = func_800B3B6C();
                break;
            case 54:
                func_800BCAA4();
                break;
            case 55:
                func_800BFD88(arg, idx);
                break;
            case 56:
                func_8007FCE8();
                break;
            case 57:
                func_8007FDEC();
                break;
            case 58:
                func_80078CEC((u8)arg, (u8)idx);
                break;
            case 59:
                func_8007893C((u8)idx, (u8)idx);
                break;
            case 60:
                func_8008B168((u8)idx);
                break;
            case 61:
                func_8008BC98((u8)idx);
                break;
            case 62:
                func_8008CD28((u8)idx);
                break;
            case 63:
                func_8008C360((u8)(arg & 1u));
                break;
            case 64:
                func_8008C3F0((u8)idx);
                break;
            case 65:
                func_8007AB30(&s_boardPtr, (u8)idx);
                break;
            case 66:
                func_8007AB68(&s_boardPtr, (u8)idx);
                break;
            case 67:
                func_8007ABA0(&s_boardPtr, (u8)idx);
                break;
            case 68:
                func_8007E674((u8)(arg & 7u));
                break;
            case 69:
                func_8008AA40((u8)arg);
                break;
            case 70:
                func_8009E508();
                break;
            case 71:
                func_800B8D7C();
                break;
            case 72:
                func_800BCAD0();
                break;
            default:
                break;
            }
            /* only the getters have a defined return value; the void leaves
             * leave v0 as whatever retail computed last */
            if (kCases[i].kind == 1) {
                expect_eq("ret", cRet, oracleRet);
            }
            expect_eq("arena", (u32)memcmp(s_arena, expected, sizeof(s_arena)), 0);
            expect_eq("g0", D_800D3344, gExpected[0]);
            expect_eq("g1", D_800D39CC, gExpected[1]);
            expect_eq("g2", D_800C3B74, gExpected[2]);
            expect_eq("g3", D_800C3D6C, gExpected[3]);
            expect_eq("g4", D_800D2D40, gExpected[4]);
            expect_eq("g5", D_800D2D48, gExpected[5]);
            expect_eq("g6", D_800D39E0, gExpected[6]);
            expect_eq("bcc", (u32)memcmp(s_bccBuf, expectedBcc, sizeof(s_bccBuf)), 0);
            expect_eq("table", (u32)memcmp(D_8005A3A0, expectedTable, sizeof(D_8005A3A0)), 0);
            expect_eq("e62", (u32)memcmp(D_800D2E62, expectedE62, kE62Size), 0);
            if (memcmp(s_rowArea, expectedRow, kRowAreaSize) != 0) {
                fprintf(stderr, "TINY LEAF FAIL d430 case=%s arg=%u\n",
                        kCases[i].name, arg);
                return 1;
            }
            s_checks++;
            expect_eq("big", (u32)memcmp(s_big, expectedBig, sizeof(s_big)), 0);
            expect_eq("402f", (u32)memcmp(s_402fBuf, s_expected402f,
                                          sizeof(s_402fBuf)), 0);
            expect_eq("ce4a", (u32)memcmp(s_ce4aBuf, s_expectedCe4a,
                                          sizeof(s_ce4aBuf)), 0);
            if (memcmp(D_800D2C60, s_expected2C60, sizeof(D_800D2C60)) != 0) {
                unsigned k4;

                for (k4 = 0; k4 < sizeof(D_800D2C60); ++k4) {
                    if (((u8*)D_800D2C60)[k4] != s_expected2C60[k4]) {
                        fprintf(stderr, "TINY LEAF FAIL 2c60 case=%s off=%u actual=%02x expected=%02x\n",
                                kCases[i].name, k4, ((u8*)D_800D2C60)[k4],
                                s_expected2C60[k4]);
                        break;
                    }
                }
                return 1;
            }
            expect_eq("2c60", (u32)memcmp(D_800D2C60, s_expected2C60,
                                          sizeof(D_800D2C60)), 0);
            expect_eq("2c8b", (u32)memcmp(D_800D2C8B, s_expected2C8B,
                                          sizeof(D_800D2C8B)), 0);
            expect_eq("g7", D_800D366C, gExpectedG);
            expect_eq("g8", D_8005959C, gExpectedH);
            if (s_logCount != expectLogCount) {
                fprintf(stderr, "TINY LEAF FAIL logcount case=%s actual=%u expected=%u\n",
                        kCases[i].name, s_logCount, expectLogCount);
                return 1;
            }
            if (memcmp(s_log, expectedLog, sizeof(s_log)) != 0) {
                unsigned k2;

                for (k2 = 0; k2 < 4; ++k2) {
                    fprintf(stderr, "DBG %s log[%u] c=(%u,%08x,%08x) o=(%u,%08x,%08x)\n",
                            kCases[i].name, k2, s_log[k2].kind, s_log[k2].args[0],
                            s_log[k2].args[1], expectedLog[k2].kind,
                            expectedLog[k2].args[0], expectedLog[k2].args[1]);
                }
            }
            expect_eq("log", (u32)memcmp(s_log, expectedLog, sizeof(s_log)), 0);
            if (kCases[i].kind == 10) {
                expect_eq(kCases[i].name, cRetPtr, oracleRet);
            }
            if (kCases[i].kind == 30) {
                expect_eq(kCases[i].name, cRetPtr, oracleRet);
            }
            if (kCases[i].kind == 35) {
                expect_eq(kCases[i].name, cRetPtr, oracleRet);
            }
            if (kCases[i].kind == 47 || kCases[i].kind == 49) {
                expect_eq(kCases[i].name, cRet, oracleRet);
            }
            if (kCases[i].kind == 53) {
                expect_eq(kCases[i].name, cRet, oracleRet);
            }
        }
    }

    printf("BATTLE TINY LEAVES PASS checks=%u (retail slices on the MIPS adapter "
           "vs the shipped C)\n", s_checks);
    return 0;
}
