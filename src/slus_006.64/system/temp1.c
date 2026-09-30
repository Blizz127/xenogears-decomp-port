#include "common.h"
#ifndef XENO_PC_PORT
void func_8001D3F4(void *);
void func_800230A8(void *arg0);
#endif

#ifdef XENO_PC_PORT
#include <assert.h>
#include <stdio.h>
#include "psx_memory.h"
#include "guest_prim_link.h"

extern char* getenv(const char*);
extern int printf(const char*, ...);

static int XenoWalkAnimDumpEnabled(void)
{
    static int s_enabled = -1;

    if (s_enabled < 0) {
        const char* env = getenv("XENO_WALK_ANIM_DUMP");
        s_enabled = (env != NULL && env[0] != '\0' && env[0] != '0');
    }
    return s_enabled;
}
#else
/* <assert.h> is unavailable under the matching build's -nostdinc MIPS
 * preprocessor. The assert(0) below marks an unimplemented path in a function
 * not yet byte-matched, so a no-op assert compiles safely there. (uintptr_t
 * comes from include/types.h for both builds.) */
#define assert(x) ((void)0)
#endif
#include "field/actor.h"
#include "psyq/libgpu.h"
#include "psyq/libgte.h"
#include "system/memory.h"

// Sprite / Animation functions

/* Retail temp1 is two TUs up to 0x80024F64: temp1 proper (..0x80022FC4,
 * gcc-2.7.2-psx) and temp1a (0x80022FC4..0x80024F64, gcc-2.7.2-cdk +
 * aspsx 2.67), which src/slus_006.64/system/temp1a.c builds by including
 * this file with TEMP1_PART_A2 defined. The port builds both halves here. */
#ifdef TEMP1_PART_A2
#define TEMP1_ASM "asm/slus_006.64/nonmatchings/system/temp1a"
#else
#define TEMP1_ASM "asm/slus_006.64/nonmatchings/system/temp1"
#endif

#if defined(XENO_PC_PORT) || !defined(TEMP1_PART_A2)
extern void func_800BA8F4(void* pSpriteData);

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp1", func_80022B2C);
#else
void func_80022B2C(u8* pSpriteData)
{
    u8* p = pSpriteData;
    s32 vel, dv, pos, floor;

    if ((*(u32*)(p + 0x3C) >> 26) & 1) {
        vel = *(s32*)(p + 0x10);
        dv = (s32)((u32)func_80022CAC(p, vel >> 4) << 4);
        *(u32*)(p + 0x4) += (u32)dv;
        *(s32*)(p + 0x10) = (s32)((u32)vel + *(u32*)(p + 0x1C));
        return;
    }

    func_800BA8F4(p);

    vel = *(s32*)(p + 0x10);
    if (vel > 0 && *(s32*)(p + 0x1C) > 0) {
        floor = *(s16*)(p + 0x84);
        if (*(s16*)(p + 0x6) == (s16)floor) {
            return;
        }
        dv = (s32)((u32)func_80022CAC(p, vel >> 4) << 4);
        pos = (s32)(*(u32*)(p + 0x4) + (u32)dv);
        *(s32*)(p + 0x4) = pos;
        if ((pos >> 16) < floor) {
            *(u32*)(p + 0x10) += *(u32*)(p + 0x1C);
            return;
        }

        /* Landed: snap to the floor and bounce. */
        *(s32*)(p + 0x4) = (s32)((u32)floor << 16);
        pos = (s32)((0u - (u32)vel) * ((*(u32*)(p + 0xA8) >> 1) & 0x3FF));
        if (pos < 0) {
            pos += 0xFF;
        }
        pos >>= 8;
        *(s32*)(p + 0x10) = pos;
        if (pos < 0) {
            pos = -pos;
        }
        dv = *(s32*)(p + 0x1C);
        if (dv < 0) {
            dv = (s32)(0u - (u32)dv);
        }
        if (pos < dv) {
            *(s32*)(p + 0x10) = 0;
        }
        return;
    }

    dv = (s32)((u32)func_80022CAC(p, vel >> 4) << 4);
    pos = (s32)(*(u32*)(p + 0x4) + (u32)dv);
    *(s32*)(p + 0x4) = pos;
    floor = *(s16*)(p + 0x84);
    if ((pos >> 16) >= floor) {
        *(s32*)(p + 0x4) = (s32)((u32)floor << 16);
    }
    *(u32*)(p + 0x10) += *(u32*)(p + 0x1C);
}
#endif

s32 func_80022CAC(void* pSpriteData, s32 value)
{
    u16 factor = *(u16*)((u8*)pSpriteData + 0x3A);
    if (factor != 0) {
        s32 adj;
        /* MULT/MFLO keeps the low word; the wide product avoids C overflow. */
        value = (s32)((s64)value * factor);
        adj = value;
        if (value < 0) adj = value + 0x3FF;
        value = adj >> 10;
    }
    return value;
}

void func_80022CDC(u8* pSprite) {
    s32 val;
    val = func_80022CAC(pSprite, *(s32*)(pSprite + 0x0C) >> 4);
    *(u32*)(pSprite + 0x00) += (u32)val << 4;
    val = func_80022CAC(pSprite, *(s32*)(pSprite + 0x14) >> 4);
    *(u32*)(pSprite + 0x08) += (u32)val << 4;
    func_80022B2C(pSprite);
}

extern void func_8001D2B0(void* pSpriteData, s16 frameIndex);

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp1", func_80022D44);
#else
void func_80022D44(void* pSpriteData) {
    u8* pData = pSpriteData;
    u32 flagsA8 = *(u32*)(pData + 0xA8);
    u16* pFrameList = (u16*)(uintptr_t)*(u32*)(pData + 0x54);
    s32 frameIndex = (flagsA8 >> 11) & 0x3F;
    u16 frame = pFrameList[frameIndex];
    s32 tileIndex = frame & 0x1FF;
    u32 flagsAC = *(u32*)(pData + 0xAC);
    u32 flags3C;

    if (frame & 0x200) {
        flagsAC |= 0x8;
    } else {
        flagsAC &= ~0x8;
    }
    *(u32*)(pData + 0xAC) = flagsAC;

    flags3C = *(u32*)(pData + 0x3C) & ~0x8;
    flags3C |= ((((flagsAC >> 3) & 0x1) ^ ((flagsAC >> 2) & 0x1)) << 3);
    *(u32*)(pData + 0x3C) = flags3C;

    func_8001D2B0(pData, tileIndex);
}
#endif

#ifndef XENO_PC_PORT
void func_80022DF4(pWork)
void* pWork;
{
    u8* pWorkData = pWork;
    u8* pSprite = *(u8**)(pWorkData + 4);
    void (*onFreeCallback)(void*);

    AnimScriptTick(pSprite);
    func_80022CDC(pSprite);
    if (*(u32*)(pSprite + 0x64) != 0) {
        if (((*(u32*)(pSprite + 0xAC) >> 6) & 1) == 0) {
            return;
        }
        AnimScriptTick(pSprite);
        func_80022CDC(pSprite);
        if (*(u32*)(pSprite + 0x64) != 0) {
            return;
        }
    }
    onFreeCallback = *(void (**)(void*))(pWorkData + 0xC);
    onFreeCallback(pWork);
}
#endif

#ifdef XENO_PC_PORT
extern s32 D_800592EC;
#else
/* temp1's own .sbss word: retail reaches it only %gp_rel, which cc1 emits
 * for a same-TU definition (plain externs assemble absolute here). */
s32 D_800592EC;
#endif

void func_80022E8C(void)
{
    D_800592EC++;
    func_80022DF4();
}

void func_80022EB8(void* pWork)
{
    u8* pWorkData = (u8*)pWork;
    u32* pSprite = (u32*)(uintptr_t)*(u32*)(pWorkData + 0x04);
    u32* pSub;
    void* pTask;

    pSub = (u32*)(uintptr_t)pSprite[0x20/4];
    if (pSub != NULL) {
        pTask = (void*)(uintptr_t)pSub[0x2C/4];
        if (pTask != NULL) {
            func_80025180(pTask);
        }
    }

    if ((pSprite[0x3C/4] & 3) == 1) {
        u32* pParent = (u32*)(uintptr_t)pSprite[0x20/4];
        void* pFree = (void*)(uintptr_t)pParent[0x34/4];
        if (pFree != NULL) {
            HeapFree(pFree);
        }
    }

    if ((pSprite[0xAC/4] >> 5) & 1) {
        func_8001CE74(pWorkData);
    }

    if ((pSprite[0xB0/4] >> 11) & 1) {
        func_8001D034(pWorkData);
    }

    if ((pSprite[0x3C/4] & 3) == 1) {
        func_8001D3F4(pSprite);
    }

    TimerWorkListRemoveTask(pWorkData);
    WorkListRemoveTask(pWorkData + 0x1C);
    HeapFree(pWork);
}
#endif /* temp1 proper */

#if defined(XENO_PC_PORT) || defined(TEMP1_PART_A2)

void func_80022FC4(u8* pSprite, s32 count, s32 tag) {
    u32 pSub = *(u32*)(pSprite + 0x20);
    void* pExisting = *(void**)(pSub + 0x2C);
    if (pExisting) {
        HeapFree(pExisting);
    }
    {
        void* pNew = HeapAlloc(count * 24, tag);
        u32 pSub2 = *(u32*)(pSprite + 0x20);
        *(void**)(pSub2 + 0x2C) = pNew;
        *(void**)(pSub2 + 0x30) = pNew;
    }
}

void func_8002303C(void* pSpriteData, s32 size, s32 flags) {
#ifdef XENO_PC_PORT
    u8* pData = (u8*)pSpriteData;
    void* pAlloc = HeapAlloc(size * 4, flags);
    u32* pSub = (u32*)(uintptr_t)*(u32*)(pData + 0x7C);
    u32* pSrc;
    pSub[0x18/4] = (u32)(uintptr_t)pAlloc;
    pSrc = (u32*)(uintptr_t)*(u32*)(pData + 0x7C);
    {
        u32* pSrcData = (u32*)(uintptr_t)*(u32*)(pData + 0x24);
        u16* pAllocH = (u16*)(uintptr_t)pSrc[0x18/4];
        /* retail reads the header words through the +0x24 pointer */
        pAllocH[1] = *(u16*)((u8*)pSrcData + 6);
        pAllocH[0] = *(u16*)((u8*)pSrcData + 4);
    }
#else
    *(void**)(*(u8**)((u8*)pSpriteData + 0x7C) + 0x18) = HeapAlloc(size * 4, flags);
    *(u16*)(*(u8**)(*(u8**)((u8*)pSpriteData + 0x7C) + 0x18) + 2) = *(u16*)(*(u8**)((u8*)pSpriteData + 0x24) + 6);
    **(u16**)(*(u8**)((u8*)pSpriteData + 0x7C) + 0x18) = *(u16*)(*(u8**)((u8*)pSpriteData + 0x24) + 4);
#endif
}

#ifndef XENO_PC_PORT
void func_800230A8(void *arg0) {
    void *temp_a0;

    /* retail tests the flag as a 1-bit field compared with 1 */
    if (((struct { u32 b0 : 1; }*)((s8*)arg0 + 0xA8))->b0 == 1) {
        temp_a0 = *(void **)((s8*)(*(void **)((s8*)(arg0) + 0x7C)) + 0x18);
        if (temp_a0 != NULL) {
            HeapFree(temp_a0);
        }
    }
    func_8001D3F4(arg0);
    HeapFree(*(void **)((s8*)(*(void **)((s8*)(arg0) + 0x20)) + 0x2C));
    HeapFree(arg0);
}
#else
void func_800230A8(void* pSpriteData)
{
    u32* sprite = (u32*)pSpriteData;

    if ((sprite[0xA8 / 4] & 1) != 0) {
        u32* pSub = (u32*)(uintptr_t)sprite[0x7C / 4];
        if (pSub != NULL && pSub[0x18 / 4] != 0) {
            HeapFree((void*)(uintptr_t)pSub[0x18 / 4]);
        }
    }

    func_8001D3F4(pSpriteData);

    {
        u32* pParent = (u32*)(uintptr_t)sprite[0x20 / 4];
        if (pParent != NULL) {
            HeapFree((void*)(uintptr_t)pParent[0x2C / 4]);
        }
    }

    HeapFree(pSpriteData);
}
#endif

#ifndef XENO_PC_PORT
INCLUDE_ASM(TEMP1_ASM, func_80023124);
#else
s32 func_80023124(s32 pointA, s32 pointB) {
    s16 ax = (s16)(pointA & 0xFFFF);
    s16 ay = (s16)((pointA >> 16) & 0xFFFF);
    s16 bx = (s16)(pointB & 0xFFFF);
    s16 by = (s16)((pointB >> 16) & 0xFFFF);
    return (-ratan2(ay - by, ax - bx)) & 0xFFF;
}
#endif

#ifndef XENO_PC_PORT
INCLUDE_ASM(TEMP1_ASM, func_80023170);
#else
void func_80023170(void* pSpriteData, s16 animFrame, s32 flagA, s32 flagB) {
    u8* pData = (u8*)pSpriteData;
    u32 flags3C = *(u32*)(pData + 0x3C);
    u32 flagsA8 = *(u32*)(pData + 0xA8);
    s32 bitB = (flagB & 1) << 4;
    s32 bitA = (flagA & 1) << 3;

    *(u16*)(pData + 0x9E) = 0;
    flags3C = (flags3C & ~0x30) | bitB;
    flags3C = (flags3C & ~0x08) | bitA;
    *(u32*)(pData + 0x3C) = flags3C;

    flagsA8 = (flagsA8 & 0xFFCFFFFF) & 0xFFF1FFFF;
    *(u32*)(pData + 0xA8) = flagsA8;

    func_8001D2B0(pSpriteData, animFrame);
}
#endif

s32 func_800231E0(u8* a0) {
    s32 p = *(s32*)(a0 + 0xC);
    p += (s32)a0;
    return *(s32*)p;
}

s32 func_800231F8(u8* a0) {
    s32 p = *(s32*)(a0 + 0xC);
    p += (s32)a0;
    return *(s32*)(p + 4) + 1;
}

extern s32 D_80059198;
extern void func_800248D4(void* pSpriteData);

void AnimScriptTick(void* pSpriteData) {
    u8* pData = pSpriteData;
    s32 i;

    for (i = 0; i != D_80059198 + 1; i++) {
        s16 waitTimer = *(s16*)(pData + 0x9E);

        if (waitTimer != 0) {
            waitTimer--;
            *(s16*)(pData + 0x9E) = waitTimer;

            if (waitTimer == 0) {
                func_800248D4(pData);
            }
        }
    }
}

#ifndef XENO_PC_PORT
INCLUDE_ASM(TEMP1_ASM, func_80023290);
#else
void func_80023290(u8* pSprite, s32 animType) {
    u32 flags3C;
    animType &= 7;
    flags3C = *(u32*)(pSprite + 0x3C);
    flags3C = (flags3C & 0xFFFFFF1F) | (animType << 5);
    *(u32*)(pSprite + 0x3C) = flags3C;
    if (animType != 0) {
        pSprite[0x2B] |= 0x2;
    } else {
        pSprite[0x2B] &= ~0x2;
    }
    {
        u32 type = (*(u32*)(pSprite + 0x40) >> 13) & 0xF;
        if (type == 8 || type == 9) {
            u32 val = (*(u32*)(pSprite + 0x3C) >> 5) & 7;
            if (val != 0) {
                *(u32*)(pSprite + 0x3C) = (*(u32*)(pSprite + 0x3C) & 0xFFFFFF1F) | ((val - 1) << 5);
            }
        } else {
            func_8001F6B0(pSprite);
        }
    }
}
#endif

void func_80023340(void* pSpriteData, s32 count) {
#ifdef XENO_PC_PORT
    u8* pData = pSpriteData;
    u8* pBase = (u8*)(uintptr_t)*(u32*)(pData + 0x20);
    void* pFramesData;

    HeapFree((void*)(uintptr_t)*(u32*)(pBase + 0x2C));
    pFramesData = HeapAlloc(count * 0x18, 0);
    *(u32*)(pBase + 0x30) = (u32)(uintptr_t)pFramesData;
    *(u32*)(pBase + 0x2C) = (u32)(uintptr_t)pFramesData;
#else
    s32 buf;
    u8* pSub;

    HeapFree(*(void**)(*(u8**)((u8*)pSpriteData + 0x20) + 0x2C));
    buf = (s32)HeapAlloc(count * 0x18, 0);
    pSub = *(u8**)((u8*)pSpriteData + 0x20);
    *(s32*)(pSub + 0x30) = buf;
    *(s32*)(pSub + 0x2C) = buf;
#endif
}

// Allocate struct stuff
/* Transcribed from asm/slus_006.64/nonmatchings/system/temp1/func_800233A4.s
 * (0x800233A4-0x80023440). Allocates a dataSize+0xEC wrapper for pOwner, links
 * it into both the timer and the work list (inner node at +0x1C), initialises the
 * +0x38 sub-structure, points both nodes' +4 field at it, then installs the
 * timer callback func_80022DF4 and the free callback func_80022EB8, returning the
 * wrapper. The port keeps its own owner in pc_port/src/game_overrides.c, so this
 * definition is weak under XENO_PC_PORT. */
extern u8 D_800591AF;
extern void func_80023804(void* p);
extern void func_80022DF4(void* pWork);
extern void func_80022EB8(void* pWork);
extern void TimerWorkListAddTask(void* pOwner, void* pTask);
extern void WorkListAddTask(void* pTask, void* pNode);
extern void TimerWorkListSetTaskCallback(void* pTask, void (*pCallback)(void*));
extern void WorkListTaskSetOnFreeCallback(void* pTask, void (*pCallback)(void*));

#ifdef XENO_PC_PORT
__attribute__((weak))
#endif
void* func_800233A4(void* pOwner, s32 dataSize) {
    u8* pWrapper;
    u8* pInner;

    pWrapper = HeapAlloc(dataSize + 0xEC, D_800591AF);
    TimerWorkListAddTask(pOwner, pWrapper);
    pInner = pWrapper + 0x1C;
    WorkListAddTask(pWrapper, pInner);
    func_80023804(pWrapper + 0x38);
    *(u32*)(pWrapper + 4) = (u32)(uintptr_t)(pWrapper + 0x38);
    *(u32*)(pInner + 4) = (u32)(uintptr_t)(pWrapper + 0x38);
    TimerWorkListSetTaskCallback(pWrapper, func_80022DF4);
    WorkListTaskSetOnFreeCallback(pWrapper, func_80022EB8);
    return pWrapper;
}
/*
Matches on  GCC 2.7.2-970404, ASPSX 2.67

typedef struct {
    WorkListEntry task1;
    WorkListEntry task2;
    SpriteData spriteData;
} AnimTask;

extern u8 D_800591AF;
extern WorkListCallback_t func_80022DF4[];
extern WorkListCallback_t func_80022EB8[];

AnimTask* func_800233A4(void* pData, int dataSize) {
    AnimTask* pEntry;
    WorkListEntry* pTask1;
    WorkListEntry* pTask2;

    pEntry = HeapAlloc(dataSize + 0xEC, D_800591AF);
    pTask1 = &pEntry->task1;
    pTask2 = &pEntry->task2;
    TimerWorkListAddTask(pData, pTask1);
    WorkListAddTask(pEntry, pTask2);
    func_80023804(&pEntry->spriteData);
    pTask1->unk4 = &pEntry->spriteData;
    pTask2->unk4 = &pEntry->spriteData;
    TimerWorkListSetTaskCallback(pEntry, &func_80022DF4);
    WorkListTaskSetOnFreeCallback(pEntry, &func_80022EB8);
    return pEntry;
}
*/

s32 func_80023440(void* pData)
{
    u16 val = *(u16*)pData;
    s32 result = (val >> 8) & 0x7;
    if ((val >> 14) & 1) {
        result += 8;
    }
    return result;
}

#ifndef XENO_PC_PORT
INCLUDE_ASM(TEMP1_ASM, func_80023468);
#else
/* Transcribed from asm/slus_006.64/nonmatchings/system/temp1/func_80023468.s
 * (0x80023468-0x800234AC, 17 instructions). Small dispatch on `type`: retail
 * rejects `type >= 0x10` up front, then jtbl_80018664 maps cases 0/5/6/10-14 to
 * 1, cases 1/4/8/9 to 0 and cases 2/7/15 to 2 — while **case 3 falls through to
 * the same exit as the out-of-range path, which returns the caller's a1
 * register**. That register value is modelled here as the second parameter
 * (retail's callers pass whatever a1 holds, and the C callers in this repo pass
 * an explicit value); a one-argument call therefore yields that slot's content,
 * exactly like retail. */
s32 func_80023468(s32 type, s32 arg1) {
    switch (type) {
    case 0:
    case 5:
    case 6:
    case 10:
    case 11:
    case 12:
    case 13:
    case 14:
        return 1;
    case 1:
    case 4:
    case 8:
    case 9:
        return 0;
    case 2:
    case 7:
    case 15:
#ifdef TEMP1_23468_MUTANT_SWAP_GROUPS
        return 1;
#else
        return 2;
#endif
    default:
        return arg1;
    }
}
#endif

#ifndef XENO_PC_PORT
INCLUDE_ASM(TEMP1_ASM, func_800234AC);
#else
void func_800234AC(void* pSpriteData) {
    u8* pData = pSpriteData;
    u8* pBase = (u8*)(uintptr_t)*(u32*)(pData + 0x20);
    u8* pDirTransforms = (u8*)(uintptr_t)*(u32*)(pBase + 0x34);
    s32 i;

    for (i = 0; i < 8; i++) {
        u8* pDir = pDirTransforms + i * 8;

        *(u8*)(pDir + 0x0) = 0;
        *(u8*)(pDir + 0x1) = 0;
        *(s16*)(pDir + 0x2) = 0;
        *(s16*)(pDir + 0x4) = 0;
        *(s16*)(pDir + 0x6) = 0;
    }
}
#endif

extern s32 D_800591A8;
extern s32 D_80059198;
extern u8 D_800591AD;
extern void SpriteComputeTransformMatrix(void* pSpriteData);

#ifndef XENO_PC_PORT
INCLUDE_ASM(TEMP1_ASM, func_80023538);
#else
void func_80023538(void* pSpriteData, void* pAnimation) {
    u8* pData = pSpriteData;
    u8* pAnim = pAnimation;
    u8* pBase;
    u8* pState;
    s32 flags0;
    s32 signedValue;
    s32 work;
    s32 tmp;
    s32 denom;
    s32 scale;
    u32 flagsA8;

    *(u32*)(pData + 0x58) = (u32)(uintptr_t)pAnim;
    *(u32*)(pData + 0x64) = (u32)(uintptr_t)(pAnim + *(u16*)(pAnim + 0x2) + 0x2);

    flagsA8 = *(u32*)(pData + 0xA8);
    flagsA8 = (flagsA8 & 0xFFCFFFFF) | ((*(u16*)pAnim & 0x3) << 20);
    *(u32*)(pData + 0xA8) = flagsA8;

    *(u32*)(pData + 0x54) = (u32)(uintptr_t)(pAnim + *(u16*)(pAnim + 0x4) + 0x4);

    flags0 = *(u16*)pAnim;
    signedValue = (flags0 >> 2) & 0x3F;
    if (signedValue & 0x20) {
        signedValue |= ~0x3F;
    }

    work = D_80059198 + 1;
    tmp = work * work * *(s16*)(pData + 0x82);
    scale = signedValue << 10;
    *(s32*)(pData + 0x1C) = scale;
    if (tmp < 0) {
        tmp += 0xFFF;
    }
    tmp = scale * (tmp >> 12);

    denom = (*(u32*)(pData + 0xAC) >> 7) & 0xFFF;
    tmp = tmp * (((0x10000 / denom) * (0x10000 / denom)) >> 8);
    if (tmp < 0) {
        tmp += 0xFF;
    }
    *(s32*)(pData + 0x1C) = tmp >> 8;

    if (!((flags0 >> 11) & 0x1)) {
        *(u32*)(pData + 0x14) = 0;
        *(u32*)(pData + 0x10) = 0;
        *(u32*)(pData + 0x0C) = 0;
        *(u32*)(pData + 0x18) = 0;
    }

    pBase = (u8*)(uintptr_t)*(u32*)(pData + 0x20);
    if (pBase != NULL) {
        if (!((flags0 >> 12) & 0x1)) {
            *(s16*)(pBase + 0x4) = 0;
            *(s16*)(pBase + 0x0) = 0;
            *(s16*)(pBase + 0x2) = 0;
            SpriteComputeTransformMatrix(pData);
        }

        if (!((flags0 >> 13) & 0x1)) {
            if (D_800591AD) {
                SpriteSetScale((SpriteData*)pData, D_800591A8);
            }
        }
        if (D_800591AD) {
            SpriteComputeTransformMatrix(pData);
        }

        if ((*(u32*)(pData + 0x3C) & 0x3) == 1) {
            *(u8*)(pBase + 0x3D) = 0;
            *(u8*)(pBase + 0x3C) = 0;

            if (((*(u32*)(pData + 0x40) >> 20) & 0x1) == 0 &&
                *(u32*)(pBase + 0x34) != 0) {
                func_800234AC(pData);
            }
        }
    }

    *(u8*)(pData + 0x8C) = 0x10;
    *(s16*)(pData + 0x30) = 0;
    flagsA8 = *(u32*)(pData + 0xA8);
    flagsA8 &= 0xFFFFF801;
    flagsA8 &= 0xF03FFFFF;
    flagsA8 &= 0xCFFFFFFF;
    flagsA8 |= 0x20000000;
    flagsA8 |= 0x1F800;
    *(u32*)(pData + 0xA8) = flagsA8;
    *(s16*)(pData + 0x9E) = 1;

    pState = (u8*)(uintptr_t)*(u32*)(pData + 0x7C);
    if (pState != NULL && (flagsA8 & 0x1) == 1) {
        *(u32*)(pState + 0x4) = 0;
        *(u32*)(pState + 0x0) = 0;
        *(s16*)(pState + 0xC) = 0;
    }
}
#endif

// SpriteData stuff
extern s32 D_80059198;

#ifndef XENO_PC_PORT
INCLUDE_ASM(TEMP1_ASM, func_80023804);
#else
void func_80023804(void* pSpriteData) {
    u8* pData = pSpriteData;
    s32 work;
    s32 scale;

    *(s32*)(pData + 0x3C) = 0;
    *(u8*)(pData + 0x2B) = 0x2D;
    *(s32*)(pData + 0x40) = 0;
    *(s16*)(pData + 0x3A) = 0;
    *(s16*)(pData + 0x30) = 0;
    *(s16*)(pData + 0x32) = 0;
    *(s16*)(pData + 0x34) = 0;
    *(s32*)(pData + 0xA8) = 0;

    *(s32*)(pData + 0x3C) &= ~0xF10001C;
    *(s32*)(pData + 0x40) &= ~0x1EFC;
    *(s32*)(pData + 0xA8 + 0x04) = 0;
    *(s32*)(pData + 0xA8 + 0x08) = 0;
    *(u8*)(pData + 0xB0) = 0;
    *(u8*)(pData + 0xAF) = 0;

    work = D_80059198 + 1;
    scale = (work * (work << 14) * *(s16*)(pData + 0x82)) >> 12;
    *(s32*)(pData + 0xAC) = (*(s32*)(pData + 0xAC) & 0xFFF8007F) | 0x8000;
    *(s32*)(pData + 0xA8) &= 0xFFF1FFFF;
    *(s32*)(pData + 0x1C) = scale;

    *(s32*)(pData + 0x64) = 0;
    *(s32*)(pData + 0x70) = 0;
    *(s32*)(pData + 0x44) = 0;
    *(s32*)(pData + 0x68) = 0;
    *(s16*)(pData + 0x80) = 0;
    *(u8*)(pData + 0x8C) = 0x10;
    *(s16*)(pData + 0x84) = 0;
    *(s32*)(pData + 0x6C) = 0;
    *(s32*)(pData + 0x50) = 0;
}
#endif



void func_8002393C(void* arg0) {
    *(s16*)((u8*)arg0 + 0x0) = 0;
    *(s16*)((u8*)arg0 + 0x2) = 0;
    *(s16*)((u8*)arg0 + 0x4) = 0;
    *(s32*)((u8*)arg0 + 0x2C) = 0;
}

void func_80023950(void* arg0) {
    *(s32*)((u8*)arg0 + 0x20) = 0;
}

void func_80023958(void* pSpriteData) {
#ifdef XENO_PC_PORT
    u8* pData = (u8*)pSpriteData;
    void* pSub = pData + 0xB4;
    *(void**)(pData + 0x20) = pSub;
    func_8002393C(pSub);
    {
        u32* pParent = (u32*)(uintptr_t)*(u32*)(pData + 0x20);
        pParent[0x34/4] = 0;
        pParent[0x40/4] = 0;
    }
#else
    u8* pSub = (u8*)pSpriteData + 0xB4;

    *(u8**)((u8*)pSpriteData + 0x20) = pSub;
    func_8002393C(pSub);
    *(s32*)(*(u8**)((u8*)pSpriteData + 0x20) + 0x34) = 0;
    *(s32*)(*(u8**)((u8*)pSpriteData + 0x20) + 0x40) = 0;
#endif
}

void func_800239A0(void* pSpriteData) {
#ifdef XENO_PC_PORT
    u8* pData = pSpriteData;
    u8* pBase = pData + 0xB4;

    *(u32*)(pData + 0x20) = (u32)(uintptr_t)pBase;
    func_8002393C(pBase);
    *(u32*)(pData + 0x7C) = (u32)(uintptr_t)(pData + 0xF4);
    *(u32*)(pBase + 0x34) = (u32)(uintptr_t)(pData + 0x124);
    *(u32*)(pData + 0x24) = (u32)(uintptr_t)(pData + 0x110);
    *(u32*)(pBase + 0x38) = 0;
#else
    u8* pSub = (u8*)pSpriteData + 0xB4;

    *(u8**)((u8*)pSpriteData + 0x20) = pSub;
    func_8002393C(pSub);
    *(u8**)((u8*)pSpriteData + 0x7C) = (u8*)pSpriteData + 0xF4;
    *(u8**)(*(u8**)((u8*)pSpriteData + 0x20) + 0x34) = (u8*)pSpriteData + 0x124;
    *(u8**)((u8*)pSpriteData + 0x24) = (u8*)pSpriteData + 0x110;
    *(s32*)(*(u8**)((u8*)pSpriteData + 0x20) + 0x38) = 0;
#endif
}

void func_800239F4(u8* pSprite) {
#ifdef XENO_PC_PORT
    u32 pSub;
    pSub = (u32)(pSprite + 0xB4);
    *(u32*)(pSprite + 0x20) = pSub;
    func_8002393C((void*)pSub);
    pSub = *(u32*)(pSprite + 0x20);
    *(u32*)(pSub + 0x30) = (u32)(pSprite + 0xF4);
    *(u32*)(pSub + 0x34) = 0;
    *(u32*)(pSub + 0x38) = 0;
#else
    u8* pSub = pSprite + 0xB4;

    *(u8**)(pSprite + 0x20) = pSub;
    func_8002393C(pSub);
    *(u8**)(*(u8**)(pSprite + 0x20) + 0x30) = pSprite + 0xF4;
    *(s32*)(*(u8**)(pSprite + 0x20) + 0x34) = 0;
    *(s32*)(*(u8**)(pSprite + 0x20) + 0x38) = 0;
#endif
}

extern u8* D_8006BE10;
extern u8* D_8005A474;
extern s32 func_8001EE74(void* arg0);
extern void func_80023950(void* arg0);
extern void func_80023958(void* pSpriteData);

#ifndef XENO_PC_PORT
INCLUDE_ASM(TEMP1_ASM, func_80023A48);
#else
void* func_80023A48(s32 type, s32 mode, u8* pAnimData, s32 extraSize, u8* pCallback) {
    u8* pResult;
    s32 allocSize;
    u8* pSource = NULL;

    switch (mode) {
        case 0:
            pResult = func_800233A4(pAnimData, extraSize);
            func_80023950(pResult + 0x38);
            allocSize = 0;
            break;
        case 1: {
            s32 frameCount;
            if (type == 5) {
                pSource = D_8006BE10;
            }
            if (type == 6) {
                pSource = D_8005A474;
            }
            frameCount = func_8001EE74(*(void**)pSource) - 1;
            allocSize = frameCount * 24 + 0x58;
            pResult = func_800233A4(pAnimData, allocSize + extraSize);
            func_800239F4(pResult + 0x38);
            break;
        }
        case 2:
            allocSize = 0x54;
            pResult = func_800233A4(pAnimData, 0x54 + extraSize);
            func_80023958(pResult + 0x38);
            break;
        default:
            pResult = NULL;
            allocSize = 0;
            break;
    }
    {
        u8* pSub = pResult + 0x38;
        *(u32*)(pSub + 0x6C) = (u32)pResult;
        *(u16*)(pSub + 0x86) = (u16)(allocSize + 0xEC);
        *(u32*)(pSub + 0x24) = (u32)pSource;
    }
    return pResult;
}
#endif

extern u8 D_800591AC;
extern void func_80024730(u8* pOwner);

#ifndef XENO_PC_PORT
INCLUDE_ASM(TEMP1_ASM, func_80023B84);
#else
void* func_80023B84(void* pSpriteData, void* pScript, void* pAnimPackage) {
    u8* pSprite = pSpriteData;
    void* pEntry = pScript;
    u8* pExtra = pAnimPackage;
    u8* pWrapper;
    u8* pNode;
    s32 kind;
    s32 mode;
    u8 prevFlag;
    u32 b0;
    u32 u24;
    u32 r40;
    u32 r3c;
    u32 vMix;
    u32 ac;
    u32 bit9;

    b0 = *(u32*)(pSprite + 0xB0) | 0x800;
    *(u32*)(pSprite + 0xB0) = b0;
    prevFlag = D_800591AC;
    if (((b0 >> 8) & 1) != 0) {
        D_800591AC = 0;
    }
    kind = func_80023440(pEntry);
    if (kind == 3) {
        kind = (*(u32*)(pSprite + 0x40) >> 13) & 0xF;
    }
    /* Retail leaves entry a1 (pScript/pEntry) in place: func_80023440 never
     * writes a1, and nothing between the two jals touches it. kind is always
     * 0-15 here so the default arm (and thus arg1) is dead, but pass it
     * explicitly per this TU's contract. */
    mode = func_80023468(kind, (s32)(uintptr_t)pEntry);
    pWrapper = func_80023A48(kind, mode, pExtra, 0, *(u8**)(pSprite + 0x6C));
    pNode = pWrapper + 0x38;
    *(u32*)(pWrapper + 0x14) |= 0x20000000;
    r40 = (*(u32*)(pNode + 0x40) & 0xFFFE1FFF) | ((kind & 0xF) << 13);
    *(u32*)(pNode + 0x40) = r40;
    u24 = *(u32*)(pNode + 0x24);
    *(u32*)(pNode + 0x3C) = (*(u32*)(pNode + 0x3C) & ~3) | (mode & 3);
    {
        u32 r40b = r40;
        r40b = (r40b & ~0x1F00) | (*(u32*)(pSprite + 0x40) & 0x1F00);
        *(u32*)(pNode + 0x40) = r40b;
    }
    *(u32*)(pNode + 0x3C) = (*(u32*)(pNode + 0x3C) & ~8) | (*(u32*)(pSprite + 0x3C) & 8);
    *(u32*)(pNode + 0x3C) = (*(u32*)(pNode + 0x3C) & ~0x10) | (*(u32*)(pSprite + 0x3C) & 0x10);
    *(u8*)(pNode + 0x3D) = *(u8*)(pSprite + 0x3D);
    r40 = (*(u32*)(pNode + 0x40) & 0xFFFBFFFF) | (*(u32*)(pSprite + 0x40) & 0x40000);
    *(u32*)(pNode + 0x40) = r40;
    *(u32*)(pNode + 0x3C) = (*(u32*)(pNode + 0x3C) | 0x4000000) & ~4;
    *(u32*)(pNode + 0x18) = *(u32*)(pSprite + 0x18);
    *(u16*)(pNode + 0x32) = *(u16*)(pSprite + 0x32);
    *(u16*)(pNode + 0x2C) = *(u16*)(pSprite + 0x2C);
    *(u16*)(pNode + 0x34) = *(u16*)(pSprite + 0x34);
    {
        u32 b0s = *(u32*)(pNode + 0xB0);
        bit9 = (*(u32*)(pSprite + 0xB0) >> 9) & 1;
        b0s = (b0s & ~0x200) | (bit9 << 9);
        *(u32*)(pNode + 0xB0) = b0s;
        if (bit9 != 0) {
            *(u32*)(pNode + 0x40) = (*(u32*)(pNode + 0x40) & 0xFFFE1FFF) | 0x300;
            *(u16*)(pNode + 0x3A) = *(u16*)(pSprite + 0x3A);
        }
    }
    vMix = ((*(u32*)(pSprite + 0xAC) & 3) << 2) | (*(u32*)(pSprite + 0xA8) >> 30);
    *(u32*)(pNode + 0xA8) = (*(u32*)(pNode + 0xA8) & 0x3FFFFFFF) | (vMix << 30);
    *(u32*)(pNode + 0xAC) = (*(u32*)(pNode + 0xAC) & ~3) | (vMix >> 2);
    *(u32*)(pNode + 0xB0) = (*(u32*)(pNode + 0xB0) & ~0x100) | (*(u32*)(pSprite + 0xB0) & 0x100);
    *(u32*)(pNode + 0xAC) = (*(u32*)(pNode + 0xAC) & ~0x40) | (*(u32*)(pSprite + 0xAC) & 0x40);
    ac = (*(u32*)(pNode + 0xAC) & 0xFFF8007F) | (*(u32*)(pSprite + 0xAC) & 0x7FF80);
    *(u32*)(pNode + 0xAC) = ac;
    ac &= ~4;
    *(u32*)(pNode + 0xA8) &= ~1;
    ac |= (*(u32*)(pSprite + 0xAC) & 4);
    *(u32*)(pNode + 0xAC) = ac;
    if ((*(u32*)(pSprite + 0xA8) & 1) != 0) {
        *(u32*)(pNode + 0x7C) = 0;
    } else {
        *(u32*)(pNode + 0x7C) = *(u32*)(pSprite + 0x7C);
    }
    *(u32*)(pNode + 0x70) = (u32)pSprite;
    *(u32*)(pNode + 0x44) = *(u32*)(pSprite + 0x44);
    *(u32*)(pNode + 0x48) = *(u32*)(pSprite + 0x48);
    *(u32*)(pNode + 0x74) = *(u32*)(pSprite + 0x74);
    *(u16*)(pNode + 0x82) = *(u16*)(pSprite + 0x82);
    *(u32*)(pNode + 0x50) = *(u32*)(pSprite + 0x50);
    *(u8*)(pNode + 0x8D) = *(u8*)(pSprite + 0xAF);
    *(u32*)(pNode + 0x78) = *(u32*)(pSprite + 0x78);
    *(u32*)(pNode + 0x0) = *(u32*)(pSprite + 0x0);
    *(u32*)(pNode + 0x4) = *(u32*)(pSprite + 0x4);
    *(u32*)(pNode + 0x8) = *(u32*)(pSprite + 0x8);
    *(u32*)(pNode + 0xC) = *(u32*)(pSprite + 0xC);
    *(u32*)(pNode + 0x10) = *(u32*)(pSprite + 0x10);
    *(u32*)(pNode + 0x14) = *(u32*)(pSprite + 0x14);
    if (mode != 0) {
        *(u16*)(*(u8**)(pNode + 0x20) + 0x0) = *(u16*)(*(u8**)(pSprite + 0x20) + 0x0);
        *(u16*)(*(u8**)(pNode + 0x20) + 0x2) = *(u16*)(*(u8**)(pSprite + 0x20) + 0x2);
        *(u16*)(*(u8**)(pNode + 0x20) + 0x4) = *(u16*)(*(u8**)(pSprite + 0x20) + 0x4);
        *(u16*)(*(u8**)(pNode + 0x20) + 0x6) = *(u16*)(*(u8**)(pSprite + 0x20) + 0x6);
        *(u16*)(*(u8**)(pNode + 0x20) + 0x8) = *(u16*)(*(u8**)(pSprite + 0x20) + 0x8);
        *(u16*)(*(u8**)(pNode + 0x20) + 0xA) = *(u16*)(*(u8**)(pSprite + 0x20) + 0xA);
    }
    func_80023538(pNode, pEntry);
    /* Keep u24 (dead retail load) and kind live across the call above so the
     * allocator places them in s3/s1; both asms emit zero bytes. */
    __asm__ volatile("" :: "r"(kind));
    __asm__ volatile("" :: "r"(u24));
    func_80024730(pWrapper);
    D_800591AC = prevFlag;
    return pNode;
}
#endif

/* Transcribed from asm/slus_006.64/nonmatchings/system/temp1/func_80023FD8.s
 * (0x80023FD8-0x80024294, 175 instructions). Sprite-script spawn: resolves the
 * script entry `index` (u16 byte offset at package+0x10, indexed two bytes per
 * entry, read from +2), classifies it with func_80023440 / func_80023468
 * (retail passes the still-live a1 = package to the dispatcher, which is only
 * observable for the case-3 fall-through), allocates through func_80023A48,
 * then tags wrapper+0x14 in place. The optional block runs only when
 * D_800591AD is set and the player sprite D_800C3E1C is non-NULL: retail
 * copies the player's render/tint/colour state into the new node, keeps **two
 * separate** +0x7C transfers, and re-joins the (player+0xA8 >> 30) with
 * (player+0xAC & 3) into node+0xA8 bits 30-31 / node+0xAC bits 0-1. The tail
 * re-stores +0x24 (loaded after func_80023A48 returned), clears
 * +0x44/+0x48/+0x34, folds type into +0x40 bits 13-16 and mode into +0x3C
 * bits 0-1, writes the D_800591A8 duration to +0x82, stores the three s16
 * script values <<16 into +0x0/+0x4/+0x8, then runs func_80023538(node, entry)
 * and func_80024730(wrapper). */
extern void func_80024730(u8* pOwner);
extern u32 D_800C3E1C;

/* The port keeps its host-pointer constructor in pc_port/src/sprite_constructor.c
 * (field_object_overlay.c calls it with host-translated pointers); this matching
 * owner writes the retail 32-bit pointer words, so it is weak under the port. */
#ifdef XENO_PC_PORT
__attribute__((weak))
#endif
#ifndef XENO_PC_PORT
INCLUDE_ASM(TEMP1_ASM, func_80023FD8);
#else
u8* func_80023FD8(s32 index, u8* pAnimData, s16* pPosition, s32 extraSize) {
    u8* pScriptTable;
    u8* pEntry;
    u8* pWrapper;
    u8* pNode;
    u8* pParent;
    u32 source;
    u32 bits40;
    u32 bits3c;
    u32 bitsAC;
    u32 split;
    u16 duration;
    s32 type;
    s32 mode;

    pScriptTable = (u8*)(uintptr_t)*(u32*)(pAnimData + 0x10);
    pEntry = pScriptTable + *(u16*)(pScriptTable + index * 2 + 2);
    type = func_80023440(pEntry);
    mode = func_80023468(type, (s32)(uintptr_t)pAnimData);
    pWrapper = (u8*)func_80023A48(type, mode, pAnimData, extraSize, NULL);
    pNode = pWrapper + 0x38;

    *(u32*)(pWrapper + 0x14) |= 0x20000000;
    source = *(u32*)(pNode + 0x24);
    *(u32*)(pNode + 0x70) = 0;
    *(u32*)(pNode + 0x74) = 0;

    if (D_800591AD != 0) {
        pParent = (u8*)(uintptr_t)D_800C3E1C;
        if (pParent != NULL) {
            bits40 = *(u32*)(pNode + 0x40);
            *(u32*)(pNode + 0x44) = *(u32*)(pParent + 0x44);
            *(u32*)(pNode + 0x48) = *(u32*)(pParent + 0x48);
            *(u32*)(pNode + 0x74) = *(u32*)(pParent + 0x74);
            *(u32*)(pNode + 0x18) = *(u32*)(pParent + 0x18);
            *(u16*)(pNode + 0x32) = *(u16*)(pParent + 0x32);
            bits40 = (bits40 & 0xFFFFE0FF) | (*(u32*)(pParent + 0x40) & 0x1F00);
            *(u32*)(pNode + 0x40) = bits40;
            bits3c = *(u32*)(pNode + 0x3C);
            bits3c = (bits3c & ~0x8) | (*(u32*)(pParent + 0x3C) & 0x8);
            *(u32*)(pNode + 0x3C) = bits3c;
            bits3c = (bits3c & ~0x10) | (*(u32*)(pParent + 0x3C) & 0x10);
            *(u32*)(pNode + 0x3C) = bits3c;
            *(u8*)(pNode + 0x3D) = *(u8*)(pParent + 0x3D);
            *(u16*)(pNode + 0x2C) = *(u16*)(pParent + 0x2C);
            bits3c = (*(u32*)(pNode + 0x3C) | 0x04000000) & ~0x4;
            *(u32*)(pNode + 0x3C) = bits3c;
            bitsAC = (*(u32*)(pNode + 0xAC) & ~0x4) | (*(u32*)(pParent + 0xAC) & 0x4);
            *(u32*)(pNode + 0xAC) = bitsAC;
            bitsAC = (bitsAC & 0xFFF8007F) | (*(u32*)(pParent + 0xAC) & 0x7FF80);
            *(u32*)(pNode + 0xAC) = bitsAC;
            *(u32*)(pNode + 0x7C) = *(u32*)(pParent + 0x7C);
            *(u32*)(pNode + 0x7C) = *(u32*)(pParent + 0x7C);
            split = (*(u32*)(pParent + 0xA8) >> 30) | ((*(u32*)(pParent + 0xAC) & 0x3) << 2);
            *(u32*)(pNode + 0xA8) = (*(u32*)(pNode + 0xA8) & 0x3FFFFFFF) | (split << 30);
            *(u32*)(pNode + 0xAC) = (*(u32*)(pNode + 0xAC) & ~0x3) | (split >> 2);
            *(u32*)(pNode + 0x50) = *(u32*)(pParent + 0x50);
            *(u8*)(pNode + 0x8D) = *(u8*)(pParent + 0xAF);
        }
    }

    bits40 = *(u32*)(pNode + 0x40) & 0xFFFE1FFF;
    bits3c = *(u32*)(pNode + 0x3C);
    duration = (u16)D_800591A8;
    *(u32*)(pNode + 0x44) = 0;
    *(u32*)(pNode + 0x48) = 0;
    *(u16*)(pNode + 0x34) = 0;
    *(u32*)(pNode + 0x24) = source;
    bits40 |= ((u32)type & 0xF) << 13;
    *(u32*)(pNode + 0x40) = bits40;
    bits3c = (bits3c & ~0x3) | ((u32)mode & 0x3);
    *(u32*)(pNode + 0x3C) = bits3c;
    *(u16*)(pNode + 0x82) = duration;
    *(u32*)(pNode + 0x0) = (u32)pPosition[0] << 16;
    *(u32*)(pNode + 0x4) = (u32)pPosition[1] << 16;
    *(u32*)(pNode + 0x8) = (u32)pPosition[2] << 16;
    func_80023538(pNode, pEntry);
    func_80024730(pWrapper);
    return pWrapper;
}
#endif

#ifdef XENO_PC_PORT
extern s32 D_800591B8;
#else
/* temp1's own .sbss word: retail reaches it only %gp_rel, which cc1 emits
 * for a same-TU definition (plain externs assemble absolute here). */
s32 D_800591B8;
#endif
extern void* func_80024524(void* pAnimPackage, s16 texX, s16 texY, s16 clutX, s16 clutY, s16 arg5);
void* func_8002435C(void* pSpriteData, void* pAnimPackage, s16 texX, s16 texY,
                    s16 clutX, s16 clutY, s16 arg6);

void* func_80024294(void* pAnimPackage, s16 texX, s16 texY, s16 clutX, s16 clutY, s16 arg5, s32 arg6) {
    void* pSpriteData;

    D_800591B8 = arg6;
    pSpriteData = func_80024524(pAnimPackage, texX, texY, clutX, clutY, arg5);
    D_800591B8 = 0;

    return pSpriteData;
}

void* func_800242F4(void* pSpriteData, void* pAnimPackage, s16 texX, s16 texY, s16 clutX, s16 clutY, s16 arg6, s32 flags) {
    void* pResult;

    /* Retail 800242F4 preserves a0/a1 and forwards five signed halfwords;
     * the eighth argument (caller SP+1C) supplies the temporary flags. */
    D_800591B8 = flags;
    pResult = func_8002435C(pSpriteData, pAnimPackage, texX, texY, clutX, clutY, arg6);
    D_800591B8 = 0;
    return pResult;
}

extern u8 D_800591AD;
extern s32 func_8001EE74(void* arg0);
extern void func_800222BC(void* pSpriteData, void* pAnimPackageFile);
extern void func_800245D8(void* pSpriteData, s16 animIndex);

#ifndef XENO_PC_PORT
INCLUDE_ASM(TEMP1_ASM, func_8002435C);
#else
void* func_8002435C(void* pSpriteData, void* pAnimPackage, s16 texX, s16 texY, s16 clutX, s16 clutY, s16 arg6) {
    u8* pData = pSpriteData;
    u8* pPackage = pAnimPackage;
    u8* pBase;
    u8* pVramData;
    u8* pAnimations;
    u8* pFrameWorkData;
    s32 frameCount;
    s32 packed;

    func_80023804(pData);
    func_800239A0(pData);
    SpriteSetScale((SpriteData*)pData, 0x1000);

    *(u32*)(pData + 0x3C) = (*(u32*)(pData + 0x3C) & ~0x3) | 0x1;
    *(u32*)(pData + 0x40) &= 0xFFFE1FFF;

    if (D_800591AD) {
        pVramData = (u8*)(uintptr_t)*(u32*)(pData + 0x7C);
        *(u32*)(pData + 0xA8) &= ~0x1;
        *(u32*)(pVramData + 0x8) = 0;
        *(u16*)(pVramData + 0xC) = 0;
    } else {
        pVramData = (u8*)(uintptr_t)*(u32*)(pData + 0x7C);
        *(u32*)(pData + 0xA8) |= 0x1;
        *(u32*)(pVramData + 0x18) = 0;
    }

    *(u32*)(pData + 0x6C) = (u32)(uintptr_t)pData;
    packed = D_800591B8 & 0xF;
    *(u32*)(pData + 0x3C) = (*(u32*)(pData + 0x3C) & 0xFF0FFFFF) | (packed << 20);
    *(u32*)(pData + 0x3C) = (*(u32*)(pData + 0x3C) & 0xFFF0FFFF) | (packed << 16);

    frameCount = func_8001EE74(pPackage + *(u32*)(pPackage + 0x8));
    pFrameWorkData = HeapAlloc(frameCount * 0x18, 0);
    pBase = (u8*)(uintptr_t)*(u32*)(pData + 0x20);
    *(u32*)(pBase + 0x2C) = (u32)(uintptr_t)pFrameWorkData;
    *(u32*)(pBase + 0x30) = (u32)(uintptr_t)pFrameWorkData;

    pVramData = (u8*)(uintptr_t)*(u32*)(pData + 0x24);
    *(s16*)(pVramData + 0x4) = clutX;
    *(s16*)(pVramData + 0x6) = clutY;
    *(s16*)(pVramData + 0x8) = texX;
    *(s16*)(pVramData + 0xA) = texY;

    *(u32*)(pData + 0x48) = (u32)(uintptr_t)pPackage;
    func_800222BC(pData, pPackage);

    pVramData = (u8*)(uintptr_t)*(u32*)(pData + 0x24);
    pAnimations = (u8*)(uintptr_t)*(u32*)(pVramData + 0x10);
    *(u32*)(pData + 0x60) =
        (u32)(uintptr_t)(pAnimations + (((*(u16*)pAnimations & 0x3F) + 1) << 1));

    func_800245D8(pData, 0);

    return pData;
}
#endif

void* func_80024524(void* pAnimPackage, s16 texX, s16 texY, s16 clutX, s16 clutY, s16 arg5) {
    void* pSpriteData;

    pSpriteData = HeapAlloc(0x164, 0);
    *(s16*)((u8*)pSpriteData + 0x86) = 0x164;

    return func_8002435C(pSpriteData, pAnimPackage, texX, texY, clutX, clutY, arg5);
}

extern s32 func_8001EE68(void* arg0);
extern void func_80023538(void* pSpriteData, void* pAnimation);
extern void func_800223B0(void* pSpriteData, s16 arg1);

#ifndef XENO_PC_PORT
INCLUDE_ASM(TEMP1_ASM, func_800245D8);
#else
void func_800245D8(void* pSpriteData, s16 animIndex) {
    u8* pData = pSpriteData;
    u8* pDefaultAnimFile;
    u8* pVramData;
    u8* pAnimations;
    u8* pAnimation;
    s32 flags;

    pDefaultAnimFile = (u8*)(uintptr_t)*(u32*)(pData + 0x48);
    if (pDefaultAnimFile == NULL) {
        *(u32*)(pData + 0x64) = 0;
        return;
    }

    flags = *(u32*)(pData + 0xB0);
    if ((void*)(uintptr_t)*(u32*)(pData + 0x44) == pDefaultAnimFile) {
        flags &= ~0x400;
    } else {
        flags |= 0x400;
    }
    *(u32*)(pData + 0xB0) = flags;

    if (animIndex < 0) {
        func_800222BC(pData, (void*)(uintptr_t)*(u32*)(pData + 0x4C));

        if (D_800591AD) {
            pVramData = (u8*)(uintptr_t)*(u32*)(pData + 0x24);
            if (!func_8001EE68((void*)(uintptr_t)*(u32*)(pVramData + 0x0))) {
                *(s16*)(pVramData + 0x6) = 0x100;
                *(s16*)(pVramData + 0x4) = 0x300;
            }
        }
    } else {
        func_800222BC(pData, pDefaultAnimFile);

        if (D_800591AD) {
            pVramData = (u8*)(uintptr_t)*(u32*)(pData + 0x24);
            *(u32*)(pVramData + 0x4) = *(u32*)((u8*)(uintptr_t)*(u32*)(pData + 0x7C) + 0xE);
        }
    }

    *(s8*)(pData + 0xAF) = (s8)animIndex;
    if (animIndex < 0) {
        animIndex = ~animIndex;
    }

    pVramData = (u8*)(uintptr_t)*(u32*)(pData + 0x24);
    pAnimations = (u8*)(uintptr_t)*(u32*)(pVramData + 0x10);
    pAnimation = pAnimations + *(u16*)(pAnimations + (animIndex * 2) + 0x2);

    *(u32*)(pData + 0x40) |= 0x100000;
    *(u32*)(pData + 0x58) = (u32)(uintptr_t)pAnimation;
    func_80023538(pData, pAnimation);
    func_800223B0(pData, *(s16*)(pData + 0x80));
#ifdef XENO_PC_PORT
    if (XenoWalkAnimDumpEnabled()) {
        printf("[walk-anim] 245D8 sprite=%p anim=%d pose=%d wait=%d pc=%p\n",
               (void*)pData, (int)*(s8*)(pData + 0xAF),
               (int)*(s16*)(pData + 0x34), (int)*(s16*)(pData + 0x9E),
               (void*)(uintptr_t)*(u32*)(pData + 0x64));
    }
#endif
}
#endif

/* Transcribed from asm/slus_006.64/nonmatchings/system/temp1/func_80024730.s
 * (0x80024730-0x800248D4; the next retail symbol func_800248D4 starts there, so
 * the earlier "0x800248C0" note here understated the body by the tail's last
 * three instructions). Animation-state dispatch keyed on bits 13-16 of the
 * state word at pOwner+0x38+0x40; retail's jtbl_800186A4 maps cases 0-6 and 14 to
 * "no state change", 7 to re-arming the timer callback with func_80022E8C, 8/9 to
 * writing a new sprite byte (0x68 / +0x36=3 with 0x60), 10/11 to clearing the
 * +0x34 counter and copying the three-word state block from D_8006F99C /
 * D_8006F9AC, and 12/13 to stepping the mode down by two (rewriting the flag
 * word's bits 13-16) plus the D_8006F99C block. Every path ends in
 * func_80025224(pOwner+0x1C, mode) with the post-transition mode.
 *
 * Case 7 binds the timer callback on **pOwner itself** (the wrapper), not on the
 * inner node: at .L80024878 the `jal TimerWorkListSetTaskCallback` still has the
 * incoming a0 = pOwner, and `addu $a0, $s2, $zero` (s2 = pOwner+0x1C) only sits
 * in the delay slot of the *following* `j .L800248B0`, i.e. it feeds the tail
 * func_80025224 call. The first version of this body passed pOwner+0x1C here;
 * the retail differential caught it.
 * The func_800BC158 cases likewise store before the call, because retail puts
 * those stores in the `jal` delay slot: cases 10/11 `sh $zero,0x34($s0)` and
 * 12/13 both `sh $v1,0x34($s0)` and the masked `sw $a1,0x40($s0)` execute before
 * func_800BC158 runs (the first version stored them afterwards). */
extern u32 D_8006F99C[];
extern u32 D_8006F9AC[];

#ifndef XENO_PC_PORT
INCLUDE_ASM(TEMP1_ASM, func_80024730);
#else
void func_80024730(u8* pOwner) {
    u8* pState = pOwner + 0x38;
    u8* pInner = pOwner + 0x1C;
    s32 mode = (*(u32*)(pState + 0x40) >> 13) & 0xF;
    u32 flags;

    switch (mode) {
    case 7:
        TimerWorkListSetTaskCallback(pOwner, func_80022E8C);
        break;
    case 8:
        *(u8*)(pState + 0x2B) = 0x68;
        *(u16*)(pState + 0x34) = 1;
        break;
    case 9:
        *(u16*)(pState + 0x36) = 3;
        *(u8*)(pState + 0x2B) = 0x60;
        *(u16*)(pState + 0x34) = 1;
        break;
    case 10:
        *(u16*)(pState + 0x34) = 0;
        func_800BC158(pOwner);
        *(u32*)(pState + 0x00) = D_8006F99C[0];
        *(u32*)(pState + 0x04) = D_8006F99C[1];
        *(u32*)(pState + 0x08) = D_8006F99C[2];
        break;
    case 11:
        *(u16*)(pState + 0x34) = 0;
        func_800BC158(pOwner);
        *(u32*)(pState + 0x00) = D_8006F9AC[0];
        *(u32*)(pState + 0x04) = D_8006F9AC[1];
        *(u32*)(pState + 0x08) = D_8006F9AC[2];
        break;
    case 12:
    case 13:
        flags = *(u32*)(pState + 0x40);
        *(u16*)(pState + 0x34) = 1;
        mode = (((flags >> 13) & 0xF) - 2) & 0xF;
        *(u32*)(pState + 0x40) = (flags & 0xFFFE1FFF) | ((u32)mode << 13);
        func_800BC158(pOwner);
        *(u32*)(pState + 0x00) = D_8006F99C[0];
        *(u32*)(pState + 0x04) = D_8006F99C[1];
        *(u32*)(pState + 0x08) = D_8006F99C[2];
        break;
    default:
        break;
    }

    func_80025224(pInner, mode);
}
#endif

extern u8 D_800591AD;
extern s32 g_WorkListCurTimer;
extern void func_800C11CC(void* pSpriteData);
extern void func_80022D44(void* pSpriteData);
extern void func_8001FBE4(void* pSpriteData, u32 opcodeIndex, void* operands);

#ifndef XENO_PC_PORT
INCLUDE_ASM(TEMP1_ASM, func_800248D4);
#else
void func_800248D4(void* pSpriteData) {
    u8* pData = pSpriteData;
    u8* pc;
    u8 opcode;

    if (D_800591AD) {
        func_800C11CC(pData);
        return;
    }

reenter:
    if (*(s16*)(pData + 0x9E) != 0) {
        return;
    }

    pc = (u8*)(uintptr_t)*(u32*)(pData + 0x64);
    opcode = *pc;

#ifdef XENO_PC_PORT
    if (XenoWalkAnimDumpEnabled()) {
        static unsigned s_248d4;

        s_248d4++;
        if (s_248d4 <= 80u) {
            printf("[walk-anim] 248D4 n=%u sprite=%p op=0x%02x anim=%d pose=%d wait=%d pc=%p\n",
                   s_248d4, (void*)pData, (unsigned)opcode,
                   (int)*(s8*)(pData + 0xAF), (int)*(s16*)(pData + 0x34),
                   (int)*(s16*)(pData + 0x9E), (void*)pc);
        }
    }
#endif

#ifdef XENO_DIAG_OPCODE_SWEEP
    /* DIAG ONLY: fixed 16-byte records to launcher-preopened fd 3. */
    struct {
        u32 tag;
        u32 spriteData;
        u32 scriptPc;
        s16 actorIndex;
        s8 scriptIndex;
        u8 opcode;
    } record;
    s32 actorIndex = -1;
    s8 scriptIndex = -1;
    s32 i;
    extern long write(int fd, const void* buffer, unsigned long count);

    if (g_FieldActors != NULL && g_FieldNumActors >= 0 && g_FieldNumActors <= 0x100) {
        for (i = 0; i < g_FieldNumActors; i++) {
            if (g_FieldActors[i].pSpriteData == (u32)(uintptr_t)pData) {
                actorIndex = i;
                if (g_FieldActors[i].pActorData != 0) {
                    ActorData* actorData = (ActorData*)(uintptr_t)g_FieldActors[i].pActorData;
                    scriptIndex = (s8)actorData->curScriptIndex;
                }
                break;
            }
        }
    }

    record.tag = 0x5753504F; /* "OPSW" in little-endian byte order */
    record.spriteData = (u32)(uintptr_t)pData;
    record.scriptPc = (u32)(uintptr_t)pc;
    record.actorIndex = (s16)actorIndex;
    record.scriptIndex = scriptIndex;
    record.opcode = opcode;
    write(3, &record, sizeof(record));
#endif

    if (opcode < 0x10) {
        s32 delay = (opcode & 0xF) + 1;
        s32 speed = (*(u32*)(pData + 0xAC) >> 7) & 0xFFF;
        s32 scaledDelay;
        u32 flags;
        u32 subIndex;

        *(u32*)(pData + 0x64) = (u32)(uintptr_t)(pc + 1);
        func_8001D2B0(pData, *(u16*)(pData + 0x34) + 1);

        scaledDelay = delay * speed;
        if (scaledDelay < 0) {
            scaledDelay += 0xFF;
        }
        delay = scaledDelay >> 8;
        if (delay == 0) {
            delay = 1;
        }

        flags = *(u32*)(pData + 0xA8) & 0xF03FFFFF;
        subIndex = (((*(u32*)(pData + 0xA8) >> 22) & 0x3F) + 1) & 0x3F;
        *(s16*)(pData + 0x9E) = *(u16*)(pData + 0x9E) + delay;
        *(u32*)(pData + 0xA8) = flags | (subIndex << 22);

        if (subIndex == 0) {
            flags = *(u32*)(pData + 0xA8) & 0xF03FFFFF;
            subIndex = (((*(u32*)(pData + 0xA8) >> 22) & 0x3F) - 1) & 0x3F;
            *(u32*)(pData + 0xA8) = flags | (subIndex << 22);
            func_800248D4(pData);
        }
        return;
    }

    if (opcode >= 0x10 && opcode < 0x20) {
        s32 delay = (opcode & 0xF) + 1;
        s32 speed = (*(u32*)(pData + 0xAC) >> 7) & 0xFFF;
        s32 scaledDelay;
        u32 flags;
        u32 subIndex;

        *(u32*)(pData + 0x64) = (u32)(uintptr_t)(pc + 1);
        flags = *(u32*)(pData + 0xA8);
        flags = (flags & 0xFFFE07FF) |
                (((((flags >> 11) & 0x3F) + 1) & 0x3F) << 11);
        *(u32*)(pData + 0xA8) = flags;
        func_80022D44(pData);

        scaledDelay = delay * speed;
        if (scaledDelay < 0) {
            scaledDelay += 0xFF;
        }
        delay = scaledDelay >> 8;
        if (delay == 0) {
            delay = 1;
        }

        flags = *(u32*)(pData + 0xA8) & 0xF03FFFFF;
        subIndex = (((*(u32*)(pData + 0xA8) >> 22) & 0x3F) + 1) & 0x3F;
        *(s16*)(pData + 0x9E) = *(u16*)(pData + 0x9E) + delay;
        *(u32*)(pData + 0xA8) = flags | (subIndex << 22);

        if (subIndex == 0) {
            flags = *(u32*)(pData + 0xA8) & 0xF03FFFFF;
            subIndex = (((*(u32*)(pData + 0xA8) >> 22) & 0x3F) - 1) & 0x3F;
            *(u32*)(pData + 0xA8) = flags | (subIndex << 22);
            func_800248D4(pData);
        }
        return;
    }

    /* asm .L80024998: opcodes 0x20-0x2F — same wait/subIndex path as
     * 0x00-0x0F, but func_8001D2B0(frameIndex - 1) instead of +1. */
    if (opcode >= 0x20 && opcode < 0x30) {
        s32 delay = (opcode & 0xF) + 1;
        s32 speed = (*(u32*)(pData + 0xAC) >> 7) & 0xFFF;
        s32 scaledDelay;
        u32 flags;
        u32 subIndex;

        *(u32*)(pData + 0x64) = (u32)(uintptr_t)(pc + 1);
        func_8001D2B0(pData, *(u16*)(pData + 0x34) - 1);

        scaledDelay = delay * speed;
        if (scaledDelay < 0) {
            scaledDelay += 0xFF;
        }
        delay = scaledDelay >> 8;
        if (delay == 0) {
            delay = 1;
        }

        flags = *(u32*)(pData + 0xA8) & 0xF03FFFFF;
        subIndex = (((*(u32*)(pData + 0xA8) >> 22) & 0x3F) + 1) & 0x3F;
        *(s16*)(pData + 0x9E) = *(u16*)(pData + 0x9E) + delay;
        *(u32*)(pData + 0xA8) = flags | (subIndex << 22);

        if (subIndex == 0) {
            flags = *(u32*)(pData + 0xA8) & 0xF03FFFFF;
            subIndex = (((*(u32*)(pData + 0xA8) >> 22) & 0x3F) - 1) & 0x3F;
            *(u32*)(pData + 0xA8) = flags | (subIndex << 22);
            func_800248D4(pData);
        }
        return;
    }

    if (opcode >= 0x30 && opcode < 0x40) {
        s32 delay = (opcode & 0xF) + 1;
        s32 speed = (*(u32*)(pData + 0xAC) >> 7) & 0xFFF;
        s32 scaledDelay;
        u32 flags;
        u32 subIndex;

        *(u32*)(pData + 0x64) = (u32)(uintptr_t)(pc + 1);

        scaledDelay = delay * speed;
        if (scaledDelay < 0) {
            scaledDelay += 0xFF;
        }
        delay = scaledDelay >> 8;
        if (delay == 0) {
            delay = 1;
        }

        flags = *(u32*)(pData + 0xA8) & 0xF03FFFFF;
        subIndex = (((*(u32*)(pData + 0xA8) >> 22) & 0x3F) + 1) & 0x3F;
        *(s16*)(pData + 0x9E) = *(u16*)(pData + 0x9E) + delay;
        *(u32*)(pData + 0xA8) = flags | (subIndex << 22);

        if (subIndex == 0) {
            flags = *(u32*)(pData + 0xA8) & 0xF03FFFFF;
            subIndex = (((*(u32*)(pData + 0xA8) >> 22) & 0x3F) - 1) & 0x3F;
            *(u32*)(pData + 0xA8) = flags | (subIndex << 22);
            func_800248D4(pData);
        }
        return;
    }

    /* asm 800249C0-80024A54: 0x40-0x7F fall through to the shared delay
     * tail with stale $s3 (no 1D2B0). A first-opcode stale delay of 0
     * clamps to 1, same as the 0x00-0x3F tail. */
    if (opcode >= 0x40 && opcode < 0x80) {
        s32 delay = 0;
        s32 speed = (*(u32*)(pData + 0xAC) >> 7) & 0xFFF;
        s32 scaledDelay;
        u32 flags;
        u32 subIndex;

        *(u32*)(pData + 0x64) = (u32)(uintptr_t)(pc + 1);

        scaledDelay = delay * speed;
        if (scaledDelay < 0) {
            scaledDelay += 0xFF;
        }
        delay = scaledDelay >> 8;
        if (delay == 0) {
            delay = 1;
        }

        flags = *(u32*)(pData + 0xA8) & 0xF03FFFFF;
        subIndex = (((*(u32*)(pData + 0xA8) >> 22) & 0x3F) + 1) & 0x3F;
        *(s16*)(pData + 0x9E) = *(u16*)(pData + 0x9E) + delay;
        *(u32*)(pData + 0xA8) = flags | (subIndex << 22);

        if (subIndex == 0) {
            flags = *(u32*)(pData + 0xA8) & 0xF03FFFFF;
            subIndex = (((*(u32*)(pData + 0xA8) >> 22) & 0x3F) - 1) & 0x3F;
            *(u32*)(pData + 0xA8) = flags | (subIndex << 22);
            func_800248D4(pData);
        }
        return;
    }

    /* jtbl_800186E0[0x80] = 0x80024CA0, not the 0xBE packed-frame
     * handler. One-byte terminator: clear A8 bits 28-29, invoke +0x68 if
     * present, else 245D8(+0xB0) when that byte is non-negative. Does not
     * set wait — looping walk/run scripts use 0x82. */
    if (opcode == 0x80) {
        void (*callback)(void*) = (void (*)(void*))(uintptr_t)*(u32*)(pData + 0x68);

        *(u32*)(pData + 0xA8) &= 0xCFFFFFFF;
        if (callback != NULL) {
            callback(pData);
            return;
        }
        if (*(s8*)(pData + 0xB0) >= 0) {
            func_800245D8(pData, *(s8*)(pData + 0xB0));
        }
        *(u32*)(pData + 0xA8) &= 0xCFFFFFFF;
        return;
    }

    /* jtbl_800186E0[0xBE] = 0x80024A84. Advances by 3 bytes rather than
     * D_8004FC40[0xBE] == 2. */
    if (opcode == 0xBE) {
        s32 packed = pc[1] | ((s32)(s8)pc[2] << 8);
        s32 delay = (((packed >> 11) & 0xF) + 1);
        s32 speed = (*(u32*)(pData + 0xAC) >> 7) & 0xFFF;
        s32 scaledDelay = delay * speed;
        s32 frame = packed & 0x1FF;

        if (scaledDelay < 0) {
            scaledDelay += 0xFF;
        }
        delay = scaledDelay >> 8;
        if (delay == 0) {
            delay = 1;
        }

        if ((*(u32*)(pData + 0x3C) & 0x3) != 1) {
            *(s16*)(pData + 0x34) = frame;
        } else {
            u32 flagsAC;
            u32 flags3C;

            if (packed < 0 && frame != 0) {
                u8* table = (u8*)(uintptr_t)*(u32*)(pData + 0x60);
                frame = table[frame - 1];
            }

            flagsAC = (*(u32*)(pData + 0xAC) & ~0x8u) | ((packed >> 6) & 0x8);
            flags3C = *(u32*)(pData + 0x3C) & ~0x8u;
            flags3C |= ((((flagsAC >> 3) & 0x1) ^ ((flagsAC >> 2) & 0x1)) << 3);
            if ((packed >> 10) & 0x1) {
                flags3C |= 0x10;
            } else {
                flags3C &= ~0x10u;
            }

            *(u32*)(pData + 0xAC) = flagsAC;
            *(u32*)(pData + 0x3C) = flags3C;
            func_8001D2B0(pData, frame);
        }

        *(s16*)(pData + 0x9E) = *(u16*)(pData + 0x9E) + delay;
        *(u32*)(pData + 0x64) = (u32)(uintptr_t)(pc + 3);
        return;
    }

    if (opcode == 0x81) {
        s8 animIndex = *(s8*)(pData + 0xAF);
        void (*callback)(void*) = (void (*)(void*))(uintptr_t)*(u32*)(pData + 0x68);
        u32 flags;

        if (animIndex == 0x3F) {
            flags = *(u32*)(pData + 0xA8) & 0xCFFFFFFF;
            *(u32*)(pData + 0xA8) = flags;
            if (callback != NULL) {
                callback(pData);
                return;
            }
            if (*(s8*)(pData + 0xB0) >= 0) {
                func_800245D8(pData, *(s8*)(pData + 0xB0));
            }
            *(u32*)(pData + 0xA8) &= 0xCFFFFFFF;
            return;
        }

        *(s16*)(pData + 0x9E) = 0;
        if (callback != NULL) {
            callback(pData);
        }

        flags = *(u32*)(pData + 0xA8) & 0xCFFFFFFF;
        *(u32*)(pData + 0xA8) = flags | 0x10000000;
        return;
    }

    if (opcode == 0x82) {
        /* asm 80024D40-80024D7C: restart the current animation (loop
         * terminator). Consumes NO operand bytes and never advances the
         * cursor at +0x64 — func_800245D8 rewrites it to the script start.
         * pData+0x10 is saved across the restart (245D8's callees clobber
         * it), the wait timer is zeroed, and the restarted script runs
         * immediately via recursion. */
        void (*callback)(void*) = (void (*)(void*))(uintptr_t)*(u32*)(pData + 0x68);
        s8 animIndex;
        u32 saved10;

        if (callback != NULL) {
            callback(pData);
        }
        /* asm reads these AFTER the callback (80024D58/80024D5C) — the
         * callback may change the current animation. */
        animIndex = *(s8*)(pData + 0xAF);
        saved10 = *(u32*)(pData + 0x10);
        func_800245D8(pData, animIndex);
        *(u32*)(pData + 0x10) = saved10;
        *(s16*)(pData + 0x9E) = 0;
        func_800248D4(pData);
        return;
    }

    if (opcode == 0xB4) {
        s32 delay;
        s32 speed;

        *(u32*)(pData + 0x64) = (u32)(uintptr_t)(pc + 4);
        if (pc[1] & 0x80) {
            *(s16*)(pData + 0x9E) = *(u16*)(pData + 0x9E) + 1;
            g_WorkListCurTimer = (pc[1] & 0x7F) + 1;
            return;
        }

        delay = pc[1] + 2;
        speed = (*(u32*)(pData + 0xAC) >> 7) & 0xFFF;
        delay *= speed;
        if (delay < 0) {
            delay += 0xFF;
        }
        delay >>= 8;
        if (delay == 0) {
            delay = 1;
        }
        *(s16*)(pData + 0x9E) = *(u16*)(pData + 0x9E) + delay;
        return;
    }

    if (opcode == 0xB3) {
        /* jtbl default .L80024EC8 + FBE4 0xB3 (asm 800214AC): set +0xA8
         * speed-index from operand, advance by D_8004FC40[0xB3]==2, re-enter.
         * Old special-case passed opcode-0x8A underflow (FBE4 no-op) and
         * returned with +0x9E==0, freezing AnimScriptTick (well anim-5). */
        extern const u8 D_8004FC40[256];

        func_8001FBE4(pData, opcode, pc + 1);
        *(u32*)(pData + 0x64) =
            (u32)(uintptr_t)(pc + D_8004FC40[opcode]);
        goto reenter;
    }

    if (opcode == 0xA7) {
        /* asm 80024E10-80024EA8 (jtbl[0xA7-0x80]). Advance by
         * D_8004FC40[0xA7]==2, then delay from operand at pc+1:
         * bit7 set → +0x9E += 1 and g_WorkListCurTimer = (op&0x7F)+1;
         * else → +0x9E += max(1, ((op+2)*speed)>>8). */
        extern const u8 D_8004FC40[256];
        u8 op1 = pc[1];
        s32 delay;

        *(u32*)(pData + 0x64) =
            (u32)(uintptr_t)(pc + D_8004FC40[opcode]);
        if (op1 & 0x80) {
            *(s16*)(pData + 0x9E) = *(u16*)(pData + 0x9E) + 1;
            g_WorkListCurTimer = (op1 & 0x7F) + 1;
            return;
        }
        delay = op1 + 2;
        delay *= (*(u32*)(pData + 0xAC) >> 7) & 0xFFF;
        if (delay < 0) {
            delay += 0xFF;
        }
        delay >>= 8;
        if (delay == 0) {
            delay = 1;
        }
        *(s16*)(pData + 0x9E) = *(u16*)(pData + 0x9E) + delay;
        return;
    }

    if (opcode == 0xB2 || opcode == 0xA0) {
        /* jtbl_800186E0[0xB2] and [0xA0] are the shared default .L80024EC8:
         * FBE4, D_8004FC40 stride, re-enter. Returning here left +0x9E==0
         * and froze AnimScriptTick after the first walk/run frame. */
        extern const u8 D_8004FC40[256];

        func_8001FBE4(pData, opcode, pc + 1);
        *(u32*)(pData + 0x64) =
            (u32)(uintptr_t)(pc + D_8004FC40[opcode]);
        goto reenter;
    }

    if (opcode == 0xE4) {
        /* asm 80024DC8-80024E0C: counted relative branch. Pop a byte;
         * zero consumes this three-byte instruction, while nonzero pushes
         * count-1 and joins the same signed-offset tail as opcode 0xE1.
         * Inline the exact +0x8C/+0x8E stack-helper effects because those
         * helpers are INCLUDE_ASM and therefore absent from the PC build. */
        extern const u8 D_8004FC40[256];
        u8 stackIndex = *(u8*)(pData + 0x8C);
        s8 signedStackIndex = (s8)stackIndex;
        u8 count = *(u8*)(pData + 0x8E + signedStackIndex);
        s32 offset;

        /* AnimScriptStackPopU8. */
        *(u8*)(pData + 0x8C) = (u8)(stackIndex + 1);
        if (count == 0) {
            *(u32*)(pData + 0x64) =
                (u32)(uintptr_t)(pc + D_8004FC40[opcode]);
            goto reenter;
        }

        /* AnimScriptStackPushU8(count - 1). */
        stackIndex = (u8)(*(u8*)(pData + 0x8C) - 1);
        *(u8*)(pData + 0x8C) = stackIndex;
        *(u8*)(pData + 0x8E + (s8)stackIndex) = (u8)(count - 1);

        offset = (s16)((u16)pc[1] | ((u16)pc[2] << 8));
        *(u32*)(pData + 0x64) = (u32)(uintptr_t)(pc + offset);
        goto reenter;
    }

    if (opcode == 0xE1) {
        /* asm 80024DEC-80024E0C: signed 16-bit PC-relative jump. The
         * two-byte operand is little-endian and relative to the opcode
         * address itself, then execution re-enters at .L8002490C. */
        s32 offset = (s16)((u16)pc[1] | ((u16)pc[2] << 8));

        *(u32*)(pData + 0x64) = (u32)(uintptr_t)(pc + offset);
        goto reenter;
    }

    if (opcode == 0x86) {
        /* asm 80024C68-80024C9C (jtbl[0x86-0x80]).
         * If *(s32*)(pData+0x10) < 0: set wait timer +0x9E = 1 and return
         * (PC stays on 0x86; AnimScriptTick retries next frame).
         * Else: advance by D_8004FC40[0x86]==1 and re-enter. */
        extern const u8 D_8004FC40[256];

        if ((s32)*(u32*)(pData + 0x10) < 0) {
            *(s16*)(pData + 0x9E) = 1;
            return;
        }
        *(u32*)(pData + 0x64) =
            (u32)(uintptr_t)(pc + D_8004FC40[opcode]);
        goto reenter;
    }

    if (opcode == 0x87) {
        /* asm 80024C80-80024C9C (jtbl[0x87-0x80]), shares wait-tail
         * .L80024C98 with 0x86.
         * If *(s16*)(pData+0x6) < *(s16*)(pData+0x84): wait (+0x9E=1),
         * PC unchanged. Else advance by D_8004FC40[0x87]==1 and re-enter. */
        extern const u8 D_8004FC40[256];

        if (*(s16*)(pData + 0x6) < *(s16*)(pData + 0x84)) {
            *(s16*)(pData + 0x9E) = 1;
            return;
        }
        *(u32*)(pData + 0x64) =
            (u32)(uintptr_t)(pc + D_8004FC40[opcode]);
        goto reenter;
    }

    /* jtbl_800186E0 dedicated handlers still unported — keep loud. */
    if (opcode == 0x85 || opcode == 0x8E ||
        opcode == 0x98 || opcode == 0xC8 ||
        opcode == 0xD4 || opcode == 0xE2 || opcode == 0xFA) {
#ifdef XENO_PC_PORT
        fprintf(stderr, "{\"event\":\"sprite_animation_dedicated_unimplemented\",\"opcode\":%u,\"pc\":\"%p\"}\n", (unsigned)opcode, (void*)pc);
        fflush(stderr);
#endif
        assert(0 && "func_800248D4 dedicated opcode path is not implemented");
    }

    if (opcode >= 0x80) {
        /* Shared default .L80024EC8 (jtbl entry or out-of-range >=0xFB):
         *   func_8001FBE4(pData, opcode, pc+1);
         *   pData+0x64 += D_8004FC40[opcode];
         *   re-enter (.L8002490C).
         * Map15 actor60 skin5 hits 0xC6 then 0x96 through this path. */
        extern const u8 D_8004FC40[256];

        func_8001FBE4(pData, opcode, pc + 1);
        *(u32*)(pData + 0x64) = (u32)(uintptr_t)(pc + D_8004FC40[opcode]);
        func_800248D4(pData);
        return;
    }

#ifdef XENO_PC_PORT
    fprintf(stderr, "{\"event\":\"sprite_animation_opcode_unimplemented\",\"opcode\":%u,\"pc\":\"%p\"}\n", (unsigned)opcode, (void*)pc);
    fflush(stderr);
#endif
    assert(0 && "func_800248D4 opcode path is not implemented");
}
#endif

extern u8 D_800591AD, D_800591AE;
extern s32 D_800591A8;

void func_80024F20(void) {
    D_800591AD = 0;
    D_800591AE = 0;
    D_800591A8 = 0x2000;
    WorkListsReset();
    func_8001D298();
}
#endif /* temp1a */
