/*
 * Retail certificate for the CDK-compiled battle task spawners
 * (src/battle/mainc75.c / mainc76.c / mainc97.c):
 *
 *   func_800B5924  0x98  p = TimerWorkListAllocateTask(*(u32*)(a0+0x6C), 0x18)
 *                        TimerWorkListSetTaskCallback(p, func_800B5854)
 *                        p+0x1C = a0, p+0x28 = func_800B57E4(a0), p+0x2C = a1,
 *                        p+0x30 = a2, p+0x24 = (s8)*(a0+0xAF),
 *                        *(u32*)(a0+0xAC) |= 0x20; return p
 *   func_800B5C18  0xA8  p = TimerWorkListAllocateTask(*(u32*)(a0+0x6C), 0x18)
 *                        TimerWorkListSetTaskCallback(p, func_800B5B3C)
 *                        p+0x1C = a0, p+0x20 = *(u32*)(a0+0x74),
 *                        p+0x2C = *(u16*)(a0+0x34), p+0x30 = (s8)*(a0+0xAF),
 *                        p+0x24 = a1[0] & 0xF, p+0x28 = a1[0] >> 4,
 *                        func_800B5B3C(p); return p
 *   func_800BF7C8  0x94  p = TimerWorkListAllocateTask(*(u32*)(a0+0x6C), 0x14)
 *                        TimerWorkListSetTaskCallback(p, func_800BF73C)
 *                        p+0x2C = a2, p+0x1C = a0, p+0x20 = (s8)*(a0+0xAF),
 *                        p+0x24 = func_800B57E4(a0), p+0x28 = a1,
 *                        *(u32*)(a0+0xAC) |= 0x20
 *
 * These three bodies only reproduce retail under gcc-2.7.2-cdk-psx: the CDK
 * cc1 materialises the callback address as `lui` before the call with the
 * `addiu` in the jal delay slot, where the PSY-Q 2.7.2 cc1 emits a single `la`
 * that gas expands in place.  The matching build already proves them
 * byte-identical (battle.bin keeps its pin); this test guards them against
 * future toolchain drift.
 *
 * The retail slices are read from disc/battle.bin and executed on the MIPS
 * adapter.  TimerWorkListAllocateTask / TimerWorkListSetTaskCallback /
 * func_800B57E4 / func_800B5B3C live outside the slices, so both sides call the
 * same host stubs (the oracle through the bridge) and the recorded arguments
 * are compared, together with the returned pointer, the task buffer and the
 * actor bytes.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "battle_mips_adapter.h"

extern u8* func_800B5924(u8* a0, u32 a1, u32 a2);
extern u8* func_800B5C18(u8* a0, u8* a1);
extern void func_800BF7C8(u8* a0, u32 a1, u32 a2);
extern void func_800B5DC4(u8* a0);
extern void func_800B56E4(u8* a0);
extern void func_800BDC78(u8* a0);
extern void func_800B64D4(u8* a0, u8* a1);
extern void func_800B73A0(void);
extern s32 func_800B16A4(u8* p);
extern void func_800B6438(u8* p);
extern void func_8007ADF4(u8** ppBoard, u8 index);
extern void func_800BF5E8(void);
extern u8* func_800B35C0(void);
extern void func_800B7C28(void);
extern void func_800BCD8C(void);
extern void func_800BF730(u32 v);
extern void func_800BC454(u16 v);
extern void func_800BC3F8(u32 v);
extern void func_800B8054(u16 v);
extern void func_800BE108(void);
extern void func_800BED30(void);
extern void func_800B9B30(void);
extern void func_800B8048(u32 v);
extern void func_800B7134(void);
extern void func_800B16F0(void);
extern void func_800B8068(void);
extern void func_800B3588(u8* p);
extern void func_800B383C(u8* p);
extern void func_800BDCF8(u8* p);
extern void func_800C0F70(void);
extern void func_800B397C(u8* a0, u8* a1, u32 a2, u32 a3, u8 a4);
extern void func_800BEDE8(void);
extern void func_800BF3A4(void);
extern void func_800B3C2C(u8* p);
extern void func_800BB7F8(void);
extern void func_800BB690(u8* p);
extern void func_800BC404(u8* p);
extern u32 func_800BF354(void);
extern void func_800BF998(void);
extern void func_800BCB54(void);
extern void func_800BEBC4(void);
extern void func_800B8D04(void);
extern void func_800B8840(void);
extern void func_800BF9EC(void);
extern void func_800BF600(u8* a0, u8* a1);
extern void func_800BEB04(void);
extern void func_800BFBA0(void);
extern void func_800B8774(void);
extern void func_800B9258(void);
extern void func_800BE0DC(void);

u8* TimerWorkListAllocateTask(u32 owner, u32 size);
void TimerWorkListSetTaskCallback(void* pTask, void* callback);
u32 func_800B57E4(u8* s);
void func_800B5B3C(u8* p);
void func_800B5854(void);
void func_800BF73C(void);
void func_800B5CC0(void);
void func_800BDC14(void);
void func_800BDF1C(void);
void func_8001E148(u32 v);
void func_800C08CC(u32 a0, void* a1, void* a2);
void func_800B51B0(void);
void func_800245D8(u32 a0, u32 a1);
u8 D_800C3EB0[0x9000];
u32 WorkListsAddTasks(u32 a0, u32 a1, void* a2, void* a3, void* a4);
void func_800B7424(u32 p);
void func_800B7364(void);
void func_800B6F0C(void);
void func_800B7134(void);
void WorkListSetTaskCallback(void* pTask, void* callback);
void D_80025A88(void);
u32 D_800C3CE8[1];
u8 D_800D3420[0x400];
u32 D_800C3548[1];
void WorkListTaskSetOnFreeCallback(void* pTask, void* callback);
void func_800B3358(void);
void func_800B3588(u8* p);
u8 D_800D2FDC[1];
u16 D_800C3D14[1];
u32 D_800C3628[1];
u16 D_800C3740[1];
u32 D_800C367C[1];
u16 D_800591B4[1];
u8 D_800591B1[1];
u32 D_800D2D68[1];
u32 D_800C374C[1];
u32 D_800C3E20[1];
u32 D_800C3610[1];
u16 D_800D2E54[1];
u32 D_800C3E1C[1];
u32 g_GfxCurOT[1];
u32 D_800C3CB4[1];
u32 D_800C3BEC[1];
u32 D_800C3BF0[1];
u32 D_800C3558[1];
u32 D_800C3A6C[1];
u8 D_800C355C[1];
u32 D_80059464[1];
u8 D_800591AC[1];
void func_800B7160(void);
void func_800B7C34(void);
void func_800B7E94(void);
void func_800B39C0(u8* a0, u8* a1, u32 a2, u32 a3, u32 a4);
void HeapFree(u32 p);
void SoundFreeWdsEntry(u32 p);
u8 D_800D3350[1];
u32 D_800C3618[1];
u32 D_800C3560[1];
u32 D_800C3674[1];
u32 D_800C3678[1];
u8 D_800C3CC4[1];
u32 D_800C3CBC[1];
u8 D_800C3CB8[1];
u8 D_800C37C8[1];
u16 D_800C3CDC[1];
u16 D_80059454[1];
u16 D_800D2D4C[1];
u16 D_800D36BC[1];
u32 D_800C3748[1];
u16 D_80059494[1];
void func_800C1140(u32 p);
void func_800A6F98(void);
void func_800BC2F0(u32 v);
void func_800A9540(u32 v);
void func_800BC460(u8* p);
u32 func_800C0FAC(u32 p);
void func_800A9FF0(u32 v);
void func_800BEC18(void);
void func_800BB620(u8* p);
void Vsync(u32 v);
void func_800B8354(void);
void func_800BF9EC(void);
u32 func_800BF720(void);
void func_800BE790(void);
void WorkListsReset(void);
void GfxAllocateWorkBuffers(u32 a0, u32 a1);
void func_800B89F4(void);
u8 D_800C3621[1];
u32 ArchiveSetIndex(u32 dir, u32 entry);
u32 ArchiveDecodeAlignedSize(u32 index);
u32 HeapAlloc(u32 size, u32 flag);
void ArchiveReadFileToBuffer(u32 a0, void* buf, u32 a2, u32 a3);
void func_8002DDE4(void* a0, u32 a1, u32 a2, u32 a3, u32 s0, u32 s1, u32 s2);
void func_80021BF8(u8* p, void* cb);
u8 D_800591B2[1];
u8 D_800591B3[1];
u8 D_800591B0[1];
void ArchiveGetArchiveOffsetIndices(u32* a, u32* b);
void DrawSync(u32 v);
void EnterCriticalSection(void);
void FlushCache(void);
void ExitCriticalSection(void);
u8 D_800C3620[1];
u8 D_800C3622[1];
u32 D_8005919C[1];
u32 D_800D2D54[1];
u32 SoundFindWdsEntry(u32 id);
u32 SoundLoadWdsFile(void* p, u32 a1);
u32 func_8003BDFC(u32 v);
void func_800BADD4(u32 i);
void GfxFreeWorkBuffers(void);
void WorkListsFreeAllEntries(void);
void func_8003852C(u32 p);
void func_800A9F94(void);
void func_800A4820(void);
u8 D_800591AD[1];
u32 D_800591A8[1];
u32 D_80050104[1];
u16 D_800D3634[1];
u32 D_800C3750[1];
u32 D_800C3E1C[1];
u32 D_800D363C[1];
u32 D_800D3640[1];

#define SLICE_5924 0x800B5924u
#define SLICE_5924_LEN 0x98u
#define SLICE_5C18 0x800B5C18u
#define SLICE_5C18_LEN 0xA8u
#define SLICE_F7C8 0x800BF7C8u
#define SLICE_F7C8_LEN 0x94u
#define SLICE_5DC4 0x800B5DC4u
#define SLICE_5DC4_LEN 0x30u
#define SLICE_56E4 0x800B56E4u
#define SLICE_56E4_LEN 0x48u
#define SLICE_DC78 0x800BDC78u
#define SLICE_DC78_LEN 0x80u
#define SLICE_64D4 0x800B64D4u
#define SLICE_64D4_LEN 0x44u
#define SLICE_73A0 0x800B73A0u
#define SLICE_73A0_LEN 0x4Cu
#define SLICE_B16A4 0x800B16A4u
#define SLICE_B16A4_LEN 0x4Cu
#define SLICE_B6438 0x800B6438u
#define SLICE_B6438_LEN 0x2Cu
#define SLICE_7ADF4 0x8007ADF4u
#define SLICE_7ADF4_LEN 0x44u
#define SLICE_BF5E8 0x800BF5E8u
#define SLICE_BF5E8_LEN 0x18u
#define SLICE_B35C0 0x800B35C0u
#define SLICE_B35C0_LEN 0x98u
#define SLICE_7C28 0x800B7C28u
#define SLICE_7C28_LEN 0x0Cu
#define SLICE_BCD8C 0x800BCD8Cu
#define SLICE_BCD8C_LEN 0x0Cu
#define SLICE_BF730 0x800BF730u
#define SLICE_BF730_LEN 0x0Cu
#define SLICE_BC454 0x800BC454u
#define SLICE_BC454_LEN 0x0Cu
#define SLICE_BC3F8 0x800BC3F8u
#define SLICE_BC3F8_LEN 0x0Cu
#define SLICE_B8054 0x800B8054u
#define SLICE_B8054_LEN 0x14u
#define SLICE_BE108 0x800BE108u
#define SLICE_BE108_LEN 0x14u
#define SLICE_BED30 0x800BED30u
#define SLICE_BED30_LEN 0x1Cu
#define SLICE_B9B30 0x800B9B30u
#define SLICE_B9B30_LEN 0x24u
#define SLICE_8048 0x800B8048u
#define SLICE_8048_LEN 0x0Cu
#define SLICE_7134 0x800B7134u
#define SLICE_7134_LEN 0x2Cu
#define SLICE_16F0 0x800B16F0u
#define SLICE_16F0_LEN 0x30u
#define SLICE_8068 0x800B8068u
#define SLICE_8068_LEN 0x30u
#define SLICE_3588 0x800B3588u
#define SLICE_3588_LEN 0x38u
#define SLICE_383C 0x800B383Cu
#define SLICE_383C_LEN 0x3Cu
#define SLICE_DCF8 0x800BDCF8u
#define SLICE_DCF8_LEN 0x3Cu
#define SLICE_C0F70 0x800C0F70u
#define SLICE_C0F70_LEN 0x3Cu
#define SLICE_397C 0x800B397Cu
#define SLICE_397C_LEN 0x44u
#define SLICE_EDE8 0x800BEDE8u
#define SLICE_EDE8_LEN 0x44u
#define SLICE_F3A4 0x800BF3A4u
#define SLICE_F3A4_LEN 0x44u
#define SLICE_3C2C 0x800B3C2Cu
#define SLICE_3C2C_LEN 0x48u
#define SLICE_BB7F8 0x800BB7F8u
#define SLICE_BB7F8_LEN 0x4Cu
#define SLICE_BB690 0x800BB690u
#define SLICE_BB690_LEN 0x50u
#define SLICE_BC404 0x800BC404u
#define SLICE_BC404_LEN 0x50u
#define SLICE_F354 0x800BF354u
#define SLICE_F354_LEN 0x50u
#define SLICE_F998 0x800BF998u
#define SLICE_F998_LEN 0x54u
#define SLICE_CB54 0x800BCB54u
#define SLICE_CB54_LEN 0x60u
#define SLICE_EBC4 0x800BEBC4u
#define SLICE_EBC4_LEN 0x54u
#define SLICE_8D04 0x800B8D04u
#define SLICE_8D04_LEN 0x78u
#define SLICE_8840 0x800B8840u
#define SLICE_8840_LEN 0x84u
#define SLICE_F9EC 0x800BF9ECu
#define SLICE_F9EC_LEN 0xB0u
#define SLICE_F600 0x800BF600u
#define SLICE_F600_LEN 0xCCu
#define SLICE_BEB04 0x800BEB04u
#define SLICE_BEB04_LEN 0xC0u
#define SLICE_BFBA0 0x800BFBA0u
#define SLICE_BFBA0_LEN 0xE0u
#define SLICE_B8774 0x800B8774u
#define SLICE_B8774_LEN 0xCCu
#define SLICE_9258 0x800B9258u
#define SLICE_9258_LEN 0x2Cu
#define SLICE_E0DC 0x800BE0DCu
#define SLICE_E0DC_LEN 0x2Cu

#define ALLOC_ENTRY 0x8001CD08u
#define SETCB_ENTRY 0x8001CD6Cu
#define HELPER_ENTRY 0x800B57E4u
#define TAIL_ENTRY 0x800B5B3Cu
#define VDF1C_ENTRY 0x800BDF1Cu
#define V1E148_ENTRY 0x8001E148u
#define VC08CC_ENTRY 0x800C08CCu
#define V245D8_ENTRY 0x800245D8u
#define VWADD_ENTRY 0x8001D1D8u
#define VB7424_ENTRY 0x800B7424u
#define VWLSET_ENTRY 0x8001CD64u
#define VWLFREE_ENTRY 0x8001CD74u
#define VB7160_ENTRY 0x800B7160u
#define VB7C34_ENTRY 0x800B7C34u
#define VB7E94_ENTRY 0x800B7E94u
#define VB39C0_ENTRY 0x800B39C0u
#define VHEAPFREE_ENTRY 0x800320E8u
#define VSOUNDFREE_ENTRY 0x80038310u
#define VWLREMOVE_ENTRY 0x8001CB48u
#define VTWLREMOVE_ENTRY 0x8001CD94u
#define VC1140_ENTRY 0x800C1140u
#define VA6F98_ENTRY 0x800A6F98u
#define VBC2F0_ENTRY 0x800BC2F0u
#define VA9540_ENTRY 0x800A9540u
#define VBC460_ENTRY 0x800BC460u
#define VC0FAC_ENTRY 0x800C0FACu
#define VA9FF0_ENTRY 0x800A9FF0u
#define VBEC18_ENTRY 0x800BEC18u
#define VVSYNC_ENTRY 0x8004B54Cu
#define V8354_ENTRY 0x800B8354u
#define VF9EC_ENTRY 0x800BF9ECu
#define VF720_ENTRY 0x800BF720u
#define VBE790_ENTRY 0x800BE790u
#define VWLReset_ENTRY 0x8001C944u
#define VGfxAlloc_ENTRY 0x80024F64u
#define V89F4_ENTRY 0x800B89F4u
#define VED30_ENTRY 0x800BED30u
#define VE108_ENTRY 0x800BE108u
#define VBB7F8_ENTRY 0x800BB7F8u
#define VBCD8C_ENTRY 0x800BCD8Cu
#define V7C28_ENTRY 0x800B7C28u
#define VF3A4_ENTRY 0x800BF3A4u
#define VDECSIZE_ENTRY 0x800288ECu
#define VHEAPALLOC_ENTRY 0x80031BDCu
#define VREADBUF_ENTRY 0x800295D8u
#define VDDE4_ENTRY 0x8002DDE4u
#define V21BF8_ENTRY 0x80021BF8u
#define VGETIND_ENTRY 0x800284B4u
#define VSETIDX_ENTRY 0x80028470u
#define VDSYNC_ENTRY 0x800445D0u
#define VENTERCS_ENTRY 0x800404D4u
#define VFLUSHC_ENTRY 0x80040454u
#define VEXITCS_ENTRY 0x800404E4u
#define VSFIND_ENTRY 0x800383ECu
#define VSLOAD_ENTRY 0x80037FD8u
#define VBDFC_ENTRY 0x8003BDFCu
#define VBADD4_ENTRY 0x800BADD4u
#define VGFFFREE_ENTRY 0x80024FB8u
#define VWLFREEALL_ENTRY 0x8001C8DCu
#define V3852C_ENTRY 0x8003852Cu
#define VA9F94_ENTRY 0x800A9F94u
#define VA4820_ENTRY 0x800A4820u

#define ACTOR 0x80100000u
#define AUX 0x80102000u
#define ARG 0x80101000u
#define TASK 0x80180000u

static u8 s_ram[0x200000];
static unsigned s_checks;

typedef struct {
    u32 allocCalls;
    u32 allocOwner;
    u32 allocSize;
    u32 cbCalls;
    u32 cbTask;
    u32 cbCallback;
    u32 helperCalls;
    u32 helperArg;
    u32 helperRet;
    u32 tailCalls;
    u32 tailTask;
    u32 df1cCalls;
    u32 e148Calls;
    u32 e148Arg;
    u32 c08ccCalls;
    u32 c08ccA0;
    u32 c08ccA1;
    u32 c08ccA2;
    u32 d245Calls;
    u32 d245A0;
    u32 d245A1;
    u32 waddCalls;
    u32 waddA0;
    u32 waddA1;
    u32 waddA2;
    u32 waddA3;
    u32 waddA4;
    u32 b7424Calls;
    u32 b7424A0;
    u32 wlsetCalls;
    u32 wlsetTask;
    u32 wlsetCallback;
    u32 wlfreeCalls;
    u32 wlfreeTask;
    u32 wlfreeCallback;
    u32 b7160Calls;
    u32 b7c34Calls;
    u32 b7e94Calls;
    u32 b39c0Calls;
    u32 b39c0A0;
    u32 b39c0A1;
    u32 b39c0A2;
    u32 b39c0A3;
    u32 b39c0A4;
    u32 heapFreeCalls;
    u32 heapFreeA0;
    u32 soundFreeCalls;
    u32 soundFreeA0;
    u32 wlRemoveCalls;
    u32 wlRemoveA0;
    u32 twlRemoveCalls;
    u32 twlRemoveA0;
    u32 c1140Calls;
    u32 c1140A0;
    u32 a6f98Calls;
    u32 bc2f0Calls;
    u32 bc2f0A0;
    u32 a9540Calls;
    u32 a9540A0;
    u32 bc460Calls;
    u32 bc460A0;
    u32 c0facCalls;
    u32 c0facA0;
    u32 a9ff0Calls;
    u32 a9ff0A0;
    u32 bec18Calls;
    u32 vsyncCalls;
    u32 vsyncA0;
    u32 b8354Calls;
    u32 bf9ecCalls;
    u32 bf720Calls;
    u32 be790Calls;
    u32 wlResetCalls;
    u32 gfxAllocCalls;
    u32 gfxAllocA0;
    u32 b89f4Calls;
    u32 setIdxCalls;
    u32 setIdxDir;
    u32 setIdxEntry;
    u32 decSizeCalls;
    u32 heapAllocCalls;
    u32 readBufCalls;
    u32 dde4Calls;
    u32 b21bf8Calls;
    u32 getIndCalls;
    u32 dsyncCalls;
    u32 enterCsCalls;
    u32 flushCacheCalls;
    u32 exitCsCalls;
    u32 sfindCalls;
    u32 sloadCalls;
    u32 bdfcCalls;
    u32 badd4Calls;
    u32 gffFreeCalls;
    u32 wlFreeAllCalls;
    u32 b3852cCalls;
    u32 a9f94Calls;
    u32 a4820Calls;
} Trace;

static Trace s_trace;

static u32 guest_of(const void* p)
{
    if (p == NULL) {
        return 0;
    }
    if ((const u8*)p >= s_ram && (const u8*)p < s_ram + sizeof(s_ram)) {
        return 0x80000000u + (u32)((const u8*)p - s_ram);
    }
    return (u32)(uintptr_t)p;
}

u8* TimerWorkListAllocateTask(u32 owner, u32 size)
{
    s_trace.allocCalls++;
    s_trace.allocOwner = owner;
    s_trace.allocSize = size;
    return s_ram + (TASK & 0x1FFFFFu);
}

void TimerWorkListSetTaskCallback(void* pTask, void* callback)
{
    s_trace.cbCalls++;
    s_trace.cbTask = guest_of(pTask);
    s_trace.cbCallback = (u32)(uintptr_t)callback;
}

u32 func_800B57E4(u8* s)
{
    u32 ga = guest_of(s);

    s_trace.helperCalls++;
    s_trace.helperArg = ga;
    s_trace.helperRet = 0x5000u + (ga & 0xFFFu);
    return s_trace.helperRet;
}

void func_800B5B3C(u8* p)
{
    s_trace.tailCalls++;
    s_trace.tailTask = guest_of(p);
}

void func_800B5854(void)
{
}

void func_800BF73C(void)
{
}

void func_800B5CC0(void)
{
}

void func_800BDC14(void)
{
}

void func_800BDF1C(void)
{
    s_trace.df1cCalls++;
}

void func_8001E148(u32 v)
{
    s_trace.e148Calls++;
    s_trace.e148Arg = v;
}

void func_800C08CC(u32 a0, void* a1, void* a2)
{
    s_trace.c08ccCalls++;
    s_trace.c08ccA0 = a0;
    s_trace.c08ccA1 = guest_of(a1);
    s_trace.c08ccA2 = (u32)(uintptr_t)a2;
}

void func_800B51B0(void)
{
}

void func_800245D8(u32 a0, u32 a1)
{
    s_trace.d245Calls++;
    s_trace.d245A0 = guest_of((void*)(uintptr_t)a0);
    s_trace.d245A1 = a1;
}

u32 WorkListsAddTasks(u32 a0, u32 a1, void* a2, void* a3, void* a4)
{
    s_trace.waddCalls++;
    s_trace.waddA0 = a0;
    s_trace.waddA1 = a1;
    s_trace.waddA2 = (u32)(uintptr_t)a2;
    s_trace.waddA3 = (u32)(uintptr_t)a3;
    s_trace.waddA4 = (u32)(uintptr_t)a4;
    return 0x80190000u;
}

void func_800B7424(u32 p)
{
    s_trace.b7424Calls++;
    s_trace.b7424A0 = p;
}

/* Only their addresses are observed (passed to WorkListsAddTasks), but the
 * linker keeps the landed func_800B7364 body, which needs its own callees. */
void func_800B6F0C(void)
{
}


void WorkListRemoveTask(u32 p)
{
    s_trace.wlRemoveCalls++;
    s_trace.wlRemoveA0 = p;
}

void TimerWorkListRemoveTask(u32 p)
{
    s_trace.twlRemoveCalls++;
    s_trace.twlRemoveA0 = p;
}

void func_80025180(u32 p)
{
    (void)p;
}

void func_800B7160(void)
{
    s_trace.b7160Calls++;
}

void func_800B7C34(void)
{
    s_trace.b7c34Calls++;
}

void func_800B7E94(void)
{
    s_trace.b7e94Calls++;
}

void func_800B39C0(u8* a0, u8* a1, u32 a2, u32 a3, u32 a4)
{
    s_trace.b39c0Calls++;
    s_trace.b39c0A0 = guest_of(a0);
    s_trace.b39c0A1 = guest_of(a1);
    s_trace.b39c0A2 = a2;
    s_trace.b39c0A3 = a3;
    s_trace.b39c0A4 = a4;
}

void HeapFree(u32 p)
{
    s_trace.heapFreeCalls++;
    s_trace.heapFreeA0 = guest_of((void*)(uintptr_t)p);
}

void SoundFreeWdsEntry(u32 p)
{
    s_trace.soundFreeCalls++;
    s_trace.soundFreeA0 = guest_of((void*)(uintptr_t)p);
}

void func_800C1140(u32 p)
{
    s_trace.c1140Calls++;
    s_trace.c1140A0 = p;
}

void func_800A6F98(void)
{
    s_trace.a6f98Calls++;
}

void func_800BC2F0(u32 v)
{
    s_trace.bc2f0Calls++;
    s_trace.bc2f0A0 = v;
}

void func_800A9540(u32 v)
{
    s_trace.a9540Calls++;
    s_trace.a9540A0 = v;
}

void func_800BC460(u8* p)
{
    s_trace.bc460Calls++;
    s_trace.bc460A0 = guest_of(p);
}

u32 func_800C0FAC(u32 p)
{
    s_trace.c0facCalls++;
    s_trace.c0facA0 = p;
    return 0x12340000u + p;
}

void func_800A9FF0(u32 v)
{
    s_trace.a9ff0Calls++;
    s_trace.a9ff0A0 = v;
}

void func_800BEC18(void)
{
    s_trace.bec18Calls++;
}

void func_800BB620(u8* p)
{
    (void)p;
}

void Vsync(u32 v)
{
    s_trace.vsyncCalls++;
    s_trace.vsyncA0 = v;
}

void func_800BE790(void)
{
    s_trace.be790Calls++;
    D_80059464[0] = D_80059464[0] + 1;
    D_800C3CE8[0] = 1;
}

void WorkListsReset(void)
{
    s_trace.wlResetCalls++;
}

void GfxAllocateWorkBuffers(u32 a0, u32 a1)
{
    s_trace.gfxAllocCalls++;
    s_trace.gfxAllocA0 = a0;
    (void)a1;
}

u32 ArchiveDataSync(void)
{
    return 0;
}

u32 ArchiveSetIndex(u32 dir, u32 entry)
{
    s_trace.setIdxCalls++;
    s_trace.setIdxDir = dir;
    s_trace.setIdxEntry = entry;
    return 0;
}

u32 ArchiveDecodeAlignedSize(u32 index)
{
    s_trace.decSizeCalls++;
    (void)index;
    return 0x100u;
}

u32 HeapAlloc(u32 size, u32 flag)
{
    s_trace.heapAllocCalls++;
    (void)size;
    (void)flag;
    return (u32)(uintptr_t)(s_ram + 0x150000);
}

void ArchiveReadFileToBuffer(u32 a0, void* buf, u32 a2, u32 a3)
{
    s_trace.readBufCalls++;
    (void)a0;
    (void)buf;
    (void)a2;
    (void)a3;
}

void func_8002DDE4(void* a0, u32 a1, u32 a2, u32 a3, u32 s0, u32 s1, u32 s2)
{
    s_trace.dde4Calls++;
    (void)a0;
    (void)a1;
    (void)a2;
    (void)a3;
    (void)s0;
    (void)s1;
    (void)s2;
}

void func_80021BF8(u8* p, void* cb)
{
    s_trace.b21bf8Calls++;
    (void)p;
    (void)cb;
}

void ArchiveGetArchiveOffsetIndices(u32* a, u32* b)
{
    s_trace.getIndCalls++;
    *a = 0x11u;
    *b = 0x22u;
}

void DrawSync(u32 v)
{
    s_trace.dsyncCalls++;
    (void)v;
}

void EnterCriticalSection(void)
{
    s_trace.enterCsCalls++;
}

void FlushCache(void)
{
    s_trace.flushCacheCalls++;
}

void ExitCriticalSection(void)
{
    s_trace.exitCsCalls++;
}

u32 SoundFindWdsEntry(u32 id)
{
    s_trace.sfindCalls++;
    (void)id;
    return 0;
}

u32 SoundLoadWdsFile(void* p, u32 a1)
{
    s_trace.sloadCalls++;
    (void)p;
    (void)a1;
    return 0x5A5A0000u;
}

u32 func_8003BDFC(u32 v)
{
    s_trace.bdfcCalls++;
    (void)v;
    return 0;
}

void func_800BADD4(u32 i)
{
    s_trace.badd4Calls++;
    (void)i;
}

void GfxFreeWorkBuffers(void)
{
    s_trace.gffFreeCalls++;
}

void WorkListsFreeAllEntries(void)
{
    s_trace.wlFreeAllCalls++;
}

void func_8003852C(u32 p)
{
    s_trace.b3852cCalls++;
    (void)p;
}

void func_800A9F94(void)
{
    s_trace.a9f94Calls++;
}

void func_800A4820(void)
{
    s_trace.a4820Calls++;
}

void WorkListSetTaskCallback(void* pTask, void* callback)
{
    s_trace.wlsetCalls++;
    s_trace.wlsetTask = guest_of(pTask);
    s_trace.wlsetCallback = (u32)(uintptr_t)callback;
}

void D_80025A88(void)
{
}

void WorkListTaskSetOnFreeCallback(void* pTask, void* callback)
{
    s_trace.wlfreeCalls++;
    s_trace.wlfreeTask = guest_of(pTask);
    s_trace.wlfreeCallback = (u32)(uintptr_t)callback;
}

void func_800B3358(void)
{
}


static int byte_rd(u8* base, u32 a, u32 start, unsigned len, unsigned w, u32* v)
{
    unsigned i;

    if (a < start || (uint64_t)a + w > (uint64_t)start + len) {
        return -1;
    }
    *v = 0;
    for (i = 0; i < w; ++i) {
        *v |= (u32)base[(a - start) + i] << (i * 8);
    }
    return 0;
}

static int byte_wr(u8* base, u32 a, u32 start, unsigned len, unsigned w, u32 v)
{
    unsigned i;

    if (a < start || (uint64_t)a + w > (uint64_t)start + len) {
        return -1;
    }
    for (i = 0; i < w; ++i) {
        base[(a - start) + i] = (u8)(v >> (i * 8));
    }
    return 0;
}

static int rd(void* u, u32 a, unsigned w, u32* v)
{
    unsigned i;

    (void)u;
    if (byte_rd((u8*)D_800D2FDC, a, 0x800D2FDCu, sizeof(D_800D2FDC), w, v) == 0) return 0;
    if (byte_rd((u8*)D_800C3D14, a, 0x800C3D14u, sizeof(D_800C3D14), w, v) == 0) return 0;
    if (byte_rd((u8*)D_800C3628, a, 0x800C3628u, sizeof(D_800C3628), w, v) == 0) return 0;
    if (byte_rd((u8*)D_800C3740, a, 0x800C3740u, sizeof(D_800C3740), w, v) == 0) return 0;
    if (byte_rd((u8*)D_800C367C, a, 0x800C367Cu, sizeof(D_800C367C), w, v) == 0) return 0;
    if (byte_rd((u8*)D_800591B4, a, 0x800591B4u, sizeof(D_800591B4), w, v) == 0) return 0;
    if (byte_rd((u8*)D_800591B1, a, 0x800591B1u, sizeof(D_800591B1), w, v) == 0) return 0;
    if (byte_rd((u8*)D_800D2D68, a, 0x800D2D68u, sizeof(D_800D2D68), w, v) == 0) return 0;
    if (byte_rd((u8*)D_800C374C, a, 0x800C374Cu, sizeof(D_800C374C), w, v) == 0) return 0;
    if (byte_rd((u8*)D_800C3E20, a, 0x800C3E20u, sizeof(D_800C3E20), w, v) == 0) return 0;
    if (byte_rd((u8*)D_800C3610, a, 0x800C3610u, sizeof(D_800C3610), w, v) == 0) return 0;
    if (byte_rd((u8*)D_800D2E54, a, 0x800D2E54u, sizeof(D_800D2E54), w, v) == 0) return 0;
    if (byte_rd((u8*)D_800C3E1C, a, 0x800C3E1Cu, sizeof(D_800C3E1C), w, v) == 0) return 0;
    if (byte_rd((u8*)g_GfxCurOT, a, 0x8005956Cu, sizeof(g_GfxCurOT), w, v) == 0) return 0;
    if (byte_rd((u8*)D_800C3CB4, a, 0x800C3CB4u, sizeof(D_800C3CB4), w, v) == 0) return 0;
    if (byte_rd((u8*)D_800C3BEC, a, 0x800C3BECu, sizeof(D_800C3BEC), w, v) == 0) return 0;
    if (byte_rd((u8*)D_800C3BF0, a, 0x800C3BF0u, sizeof(D_800C3BF0), w, v) == 0) return 0;
    if (byte_rd((u8*)D_800C3558, a, 0x800C3558u, sizeof(D_800C3558), w, v) == 0) return 0;
    if (byte_rd((u8*)D_800C3A6C, a, 0x800C3A6Cu, sizeof(D_800C3A6C), w, v) == 0) return 0;
    if (byte_rd((u8*)D_800C355C, a, 0x800C355Cu, sizeof(D_800C355C), w, v) == 0) return 0;
    if (byte_rd((u8*)D_80059464, a, 0x80059464u, sizeof(D_80059464), w, v) == 0) return 0;
    if (byte_rd((u8*)D_800591AC, a, 0x800591ACu, sizeof(D_800591AC), w, v) == 0) return 0;
    if (byte_rd((u8*)D_800D3350, a, 0x800D3350u, sizeof(D_800D3350), w, v) == 0) return 0;
    if (byte_rd((u8*)D_800C3618, a, 0x800C3618u, sizeof(D_800C3618), w, v) == 0) return 0;
    if (byte_rd((u8*)D_800C3560, a, 0x800C3560u, sizeof(D_800C3560), w, v) == 0) return 0;
    if (byte_rd((u8*)D_800C3674, a, 0x800C3674u, sizeof(D_800C3674), w, v) == 0) return 0;
    if (byte_rd((u8*)D_800C3678, a, 0x800C3678u, sizeof(D_800C3678), w, v) == 0) return 0;
    if (byte_rd((u8*)D_800C3CC4, a, 0x800C3CC4u, sizeof(D_800C3CC4), w, v) == 0) return 0;
    if (byte_rd((u8*)D_800C3CBC, a, 0x800C3CBCu, sizeof(D_800C3CBC), w, v) == 0) return 0;
    if (byte_rd((u8*)D_800C3CB8, a, 0x800C3CB8u, sizeof(D_800C3CB8), w, v) == 0) return 0;
    if (byte_rd((u8*)D_800C37C8, a, 0x800C37C8u, sizeof(D_800C37C8), w, v) == 0) return 0;
    if (byte_rd((u8*)D_800C3CDC, a, 0x800C3CDCu, sizeof(D_800C3CDC), w, v) == 0) return 0;
    if (byte_rd((u8*)D_80059454, a, 0x80059454u, sizeof(D_80059454), w, v) == 0) return 0;
    if (byte_rd((u8*)D_800D2D4C, a, 0x800D2D4Cu, sizeof(D_800D2D4C), w, v) == 0) return 0;
    if (byte_rd((u8*)D_800D36BC, a, 0x800D36BCu, sizeof(D_800D36BC), w, v) == 0) return 0;
    if (byte_rd((u8*)D_800C3748, a, 0x800C3748u, sizeof(D_800C3748), w, v) == 0) return 0;
    if (byte_rd((u8*)D_80059494, a, 0x80059494u, sizeof(D_80059494), w, v) == 0) return 0;
    if (byte_rd((u8*)D_800591AD, a, 0x800591ADu, sizeof(D_800591AD), w, v) == 0) return 0;
    if (byte_rd((u8*)D_800591A8, a, 0x800591A8u, sizeof(D_800591A8), w, v) == 0) return 0;
    if (byte_rd((u8*)D_80050104, a, 0x80050104u, sizeof(D_80050104), w, v) == 0) return 0;
    if (byte_rd((u8*)D_800D3634, a, 0x800D3634u, sizeof(D_800D3634), w, v) == 0) return 0;
    if (byte_rd((u8*)D_800C3750, a, 0x800C3750u, sizeof(D_800C3750), w, v) == 0) return 0;
    if (byte_rd((u8*)D_800D363C, a, 0x800D363Cu, sizeof(D_800D363C), w, v) == 0) return 0;
    if (byte_rd((u8*)D_800D3640, a, 0x800D3640u, sizeof(D_800D3640), w, v) == 0) return 0;
    if (byte_rd((u8*)D_800591B2, a, 0x800591B2u, sizeof(D_800591B2), w, v) == 0) return 0;
    if (byte_rd((u8*)D_800591B3, a, 0x800591B3u, sizeof(D_800591B3), w, v) == 0) return 0;
    if (byte_rd((u8*)D_800591B0, a, 0x800591B0u, sizeof(D_800591B0), w, v) == 0) return 0;
    if (byte_rd((u8*)D_800C3620, a, 0x800C3620u, sizeof(D_800C3620), w, v) == 0) return 0;
    if (byte_rd((u8*)D_800C3622, a, 0x800C3622u, sizeof(D_800C3622), w, v) == 0) return 0;
    if (byte_rd((u8*)D_8005919C, a, 0x8005919Cu, sizeof(D_8005919C), w, v) == 0) return 0;
    if (byte_rd((u8*)D_800D2D54, a, 0x800D2D54u, sizeof(D_800D2D54), w, v) == 0) return 0;
    if (a >= 0x800C3548u && (uint64_t)a + w <= 0x800C3548u + sizeof(D_800C3548)) {
        *v = 0;
        for (i = 0; i < w; ++i) {
            *v |= (u32)((u8*)D_800C3548)[(a - 0x800C3548u) + i] << (i * 8);
        }
        return 0;
    }
    if (a >= 0x800C3CE8u && (uint64_t)a + w <= 0x800C3CE8u + sizeof(D_800C3CE8)) {
        *v = 0;
        for (i = 0; i < w; ++i) {
            *v |= (u32)((u8*)D_800C3CE8)[(a - 0x800C3CE8u) + i] << (i * 8);
        }
        return 0;
    }
    if (a >= 0x800D3420u && (uint64_t)a + w <= 0x800D3420u + sizeof(D_800D3420)) {
        *v = 0;
        for (i = 0; i < w; ++i) {
            *v |= (u32)D_800D3420[(a - 0x800D3420u) + i] << (i * 8);
        }
        return 0;
    }
    if (a >= 0x800C3EB0u && (uint64_t)a + w <= 0x800C3EB0u + sizeof(D_800C3EB0)) {
        *v = 0;
        for (i = 0; i < w; ++i) {
            *v |= (u32)D_800C3EB0[(a - 0x800C3EB0u) + i] << (i * 8);
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
    unsigned i;

    (void)u;
    if (byte_wr((u8*)D_800D2FDC, a, 0x800D2FDCu, sizeof(D_800D2FDC), w, v) == 0) return 0;
    if (byte_wr((u8*)D_800C3D14, a, 0x800C3D14u, sizeof(D_800C3D14), w, v) == 0) return 0;
    if (byte_wr((u8*)D_800C3628, a, 0x800C3628u, sizeof(D_800C3628), w, v) == 0) return 0;
    if (byte_wr((u8*)D_800C3740, a, 0x800C3740u, sizeof(D_800C3740), w, v) == 0) return 0;
    if (byte_wr((u8*)D_800C367C, a, 0x800C367Cu, sizeof(D_800C367C), w, v) == 0) return 0;
    if (byte_wr((u8*)D_800591B4, a, 0x800591B4u, sizeof(D_800591B4), w, v) == 0) return 0;
    if (byte_wr((u8*)D_800591B1, a, 0x800591B1u, sizeof(D_800591B1), w, v) == 0) return 0;
    if (byte_wr((u8*)D_800D2D68, a, 0x800D2D68u, sizeof(D_800D2D68), w, v) == 0) return 0;
    if (byte_wr((u8*)D_800C374C, a, 0x800C374Cu, sizeof(D_800C374C), w, v) == 0) return 0;
    if (byte_wr((u8*)D_800C3E20, a, 0x800C3E20u, sizeof(D_800C3E20), w, v) == 0) return 0;
    if (byte_wr((u8*)D_800C3610, a, 0x800C3610u, sizeof(D_800C3610), w, v) == 0) return 0;
    if (byte_wr((u8*)D_800D2E54, a, 0x800D2E54u, sizeof(D_800D2E54), w, v) == 0) return 0;
    if (byte_wr((u8*)D_800C3E1C, a, 0x800C3E1Cu, sizeof(D_800C3E1C), w, v) == 0) return 0;
    if (byte_wr((u8*)g_GfxCurOT, a, 0x8005956Cu, sizeof(g_GfxCurOT), w, v) == 0) return 0;
    if (byte_wr((u8*)D_800C3CB4, a, 0x800C3CB4u, sizeof(D_800C3CB4), w, v) == 0) return 0;
    if (byte_wr((u8*)D_800C3BEC, a, 0x800C3BECu, sizeof(D_800C3BEC), w, v) == 0) return 0;
    if (byte_wr((u8*)D_800C3BF0, a, 0x800C3BF0u, sizeof(D_800C3BF0), w, v) == 0) return 0;
    if (byte_wr((u8*)D_800C3558, a, 0x800C3558u, sizeof(D_800C3558), w, v) == 0) return 0;
    if (byte_wr((u8*)D_800C3A6C, a, 0x800C3A6Cu, sizeof(D_800C3A6C), w, v) == 0) return 0;
    if (byte_wr((u8*)D_800C355C, a, 0x800C355Cu, sizeof(D_800C355C), w, v) == 0) return 0;
    if (byte_wr((u8*)D_80059464, a, 0x80059464u, sizeof(D_80059464), w, v) == 0) return 0;
    if (byte_wr((u8*)D_800591AC, a, 0x800591ACu, sizeof(D_800591AC), w, v) == 0) return 0;
    if (a >= 0x800C3CE8u && (uint64_t)a + w <= 0x800C3CE8u + sizeof(D_800C3CE8)) {
        for (i = 0; i < w; ++i) {
            ((u8*)D_800C3CE8)[(a - 0x800C3CE8u) + i] = (u8)(v >> (i * 8));
        }
        return 0;
    }
    if (byte_wr((u8*)D_800D3350, a, 0x800D3350u, sizeof(D_800D3350), w, v) == 0) return 0;
    if (byte_wr((u8*)D_800C3618, a, 0x800C3618u, sizeof(D_800C3618), w, v) == 0) return 0;
    if (byte_wr((u8*)D_800C3560, a, 0x800C3560u, sizeof(D_800C3560), w, v) == 0) return 0;
    if (byte_wr((u8*)D_800C3674, a, 0x800C3674u, sizeof(D_800C3674), w, v) == 0) return 0;
    if (byte_wr((u8*)D_800C3678, a, 0x800C3678u, sizeof(D_800C3678), w, v) == 0) return 0;
    if (byte_wr((u8*)D_800C3CC4, a, 0x800C3CC4u, sizeof(D_800C3CC4), w, v) == 0) return 0;
    if (byte_wr((u8*)D_800C3CBC, a, 0x800C3CBCu, sizeof(D_800C3CBC), w, v) == 0) return 0;
    if (byte_wr((u8*)D_800C3CB8, a, 0x800C3CB8u, sizeof(D_800C3CB8), w, v) == 0) return 0;
    if (byte_wr((u8*)D_800C37C8, a, 0x800C37C8u, sizeof(D_800C37C8), w, v) == 0) return 0;
    if (byte_wr((u8*)D_800C3CDC, a, 0x800C3CDCu, sizeof(D_800C3CDC), w, v) == 0) return 0;
    if (byte_wr((u8*)D_80059454, a, 0x80059454u, sizeof(D_80059454), w, v) == 0) return 0;
    if (byte_wr((u8*)D_800D2D4C, a, 0x800D2D4Cu, sizeof(D_800D2D4C), w, v) == 0) return 0;
    if (byte_wr((u8*)D_800D36BC, a, 0x800D36BCu, sizeof(D_800D36BC), w, v) == 0) return 0;
    if (byte_wr((u8*)D_800C3748, a, 0x800C3748u, sizeof(D_800C3748), w, v) == 0) return 0;
    if (byte_wr((u8*)D_80059494, a, 0x80059494u, sizeof(D_80059494), w, v) == 0) return 0;
    if (byte_wr((u8*)D_800591AD, a, 0x800591ADu, sizeof(D_800591AD), w, v) == 0) return 0;
    if (byte_wr((u8*)D_800591A8, a, 0x800591A8u, sizeof(D_800591A8), w, v) == 0) return 0;
    if (byte_wr((u8*)D_80050104, a, 0x80050104u, sizeof(D_80050104), w, v) == 0) return 0;
    if (byte_wr((u8*)D_800D3634, a, 0x800D3634u, sizeof(D_800D3634), w, v) == 0) return 0;
    if (byte_wr((u8*)D_800C3750, a, 0x800C3750u, sizeof(D_800C3750), w, v) == 0) return 0;
    if (byte_wr((u8*)D_800D363C, a, 0x800D363Cu, sizeof(D_800D363C), w, v) == 0) return 0;
    if (byte_wr((u8*)D_800D3640, a, 0x800D3640u, sizeof(D_800D3640), w, v) == 0) return 0;
    if (byte_wr((u8*)D_800591B2, a, 0x800591B2u, sizeof(D_800591B2), w, v) == 0) return 0;
    if (byte_wr((u8*)D_800591B3, a, 0x800591B3u, sizeof(D_800591B3), w, v) == 0) return 0;
    if (byte_wr((u8*)D_800591B0, a, 0x800591B0u, sizeof(D_800591B0), w, v) == 0) return 0;
    if (byte_wr((u8*)D_800C3620, a, 0x800C3620u, sizeof(D_800C3620), w, v) == 0) return 0;
    if (byte_wr((u8*)D_800C3622, a, 0x800C3622u, sizeof(D_800C3622), w, v) == 0) return 0;
    if (byte_wr((u8*)D_8005919C, a, 0x8005919Cu, sizeof(D_8005919C), w, v) == 0) return 0;
    if (byte_wr((u8*)D_800D2D54, a, 0x800D2D54u, sizeof(D_800D2D54), w, v) == 0) return 0;
    if (a >= 0x800C3548u && (uint64_t)a + w <= 0x800C3548u + sizeof(D_800C3548)) {
        for (i = 0; i < w; ++i) {
            ((u8*)D_800C3548)[(a - 0x800C3548u) + i] = (u8)(v >> (i * 8));
        }
        return 0;
    }
    if (a >= 0x800D3420u && (uint64_t)a + w <= 0x800D3420u + sizeof(D_800D3420)) {
        for (i = 0; i < w; ++i) {
            D_800D3420[(a - 0x800D3420u) + i] = (u8)(v >> (i * 8));
        }
        return 0;
    }
    if (a >= 0x800C3EB0u && (uint64_t)a + w <= 0x800C3EB0u + sizeof(D_800C3EB0)) {
        for (i = 0; i < w; ++i) {
            D_800C3EB0[(a - 0x800C3EB0u) + i] = (u8)(v >> (i * 8));
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

static int bridge(void* u, PcPortMipsCpu* c, u32 t)
{
    (void)u;
    switch (t) {
    case ALLOC_ENTRY:
        c->gpr[2] = guest_of(TimerWorkListAllocateTask(c->gpr[4], c->gpr[5]));
        return 1;
    case SETCB_ENTRY:
        TimerWorkListSetTaskCallback((void*)(uintptr_t)c->gpr[4],
                                     (void*)(uintptr_t)c->gpr[5]);
        c->gpr[2] = 0;
        return 1;
    case HELPER_ENTRY:
        c->gpr[2] = func_800B57E4((u8*)(uintptr_t)c->gpr[4]);
        return 1;
    case TAIL_ENTRY:
        func_800B5B3C((u8*)(uintptr_t)c->gpr[4]);
        c->gpr[2] = 0;
        return 1;
    case VDF1C_ENTRY:
        func_800BDF1C();
        c->gpr[2] = 0;
        return 1;
    case V1E148_ENTRY:
        func_8001E148(c->gpr[4]);
        c->gpr[2] = 0;
        return 1;
    case VC08CC_ENTRY:
        func_800C08CC(c->gpr[4], (void*)(uintptr_t)c->gpr[5],
                      (void*)(uintptr_t)c->gpr[6]);
        c->gpr[2] = 0;
        return 1;
    case V245D8_ENTRY:
        func_800245D8(c->gpr[4], c->gpr[5]);
        c->gpr[2] = 0;
        return 1;
    case VB7160_ENTRY:
        func_800B7160();
        c->gpr[2] = 0;
        return 1;
    case VB7C34_ENTRY:
        func_800B7C34();
        c->gpr[2] = 0;
        return 1;
    case VB7E94_ENTRY:
        func_800B7E94();
        c->gpr[2] = 0;
        return 1;
    case VB39C0_ENTRY: {
        u32 a4 = 0;

        rd(NULL, (c->gpr[29] + 0x10) & 0xFFFFFFFFu, 4, &a4);
        func_800B39C0((u8*)(uintptr_t)c->gpr[4], (u8*)(uintptr_t)c->gpr[5],
                      c->gpr[6], c->gpr[7], a4);
        c->gpr[2] = 0;
        return 1;
    }
    case VDECSIZE_ENTRY:
        c->gpr[2] = ArchiveDecodeAlignedSize(c->gpr[4]);
        return 1;
    case VHEAPALLOC_ENTRY:
        c->gpr[2] = guest_of((void*)(uintptr_t)HeapAlloc(c->gpr[4], c->gpr[5]));
        return 1;
    case VREADBUF_ENTRY:
        ArchiveReadFileToBuffer(c->gpr[4], (void*)(uintptr_t)c->gpr[5],
                                c->gpr[6], c->gpr[7]);
        c->gpr[2] = 0;
        return 1;
    case VDDE4_ENTRY: {
        u32 s0 = 0, s1 = 0, s2 = 0;

        rd(NULL, (c->gpr[29] + 0x10) & 0xFFFFFFFFu, 4, &s0);
        rd(NULL, (c->gpr[29] + 0x14) & 0xFFFFFFFFu, 4, &s1);
        rd(NULL, (c->gpr[29] + 0x18) & 0xFFFFFFFFu, 4, &s2);
        func_8002DDE4((void*)(uintptr_t)c->gpr[4], c->gpr[5], c->gpr[6],
                      c->gpr[7], s0, s1, s2);
        c->gpr[2] = 0;
        return 1;
    }
    case VSFIND_ENTRY:
        c->gpr[2] = SoundFindWdsEntry(c->gpr[4]);
        return 1;
    case VSLOAD_ENTRY:
        c->gpr[2] = SoundLoadWdsFile((void*)(uintptr_t)c->gpr[4], c->gpr[5]);
        return 1;
    case VBDFC_ENTRY:
        c->gpr[2] = func_8003BDFC(c->gpr[4]);
        return 1;
    case VBADD4_ENTRY:
        func_800BADD4(c->gpr[4]);
        c->gpr[2] = 0;
        return 1;
    case VGFFFREE_ENTRY:
        GfxFreeWorkBuffers();
        c->gpr[2] = 0;
        return 1;
    case VWLFREEALL_ENTRY:
        WorkListsFreeAllEntries();
        c->gpr[2] = 0;
        return 1;
    case V3852C_ENTRY:
        func_8003852C(c->gpr[4]);
        c->gpr[2] = 0;
        return 1;
    case VA9F94_ENTRY:
        func_800A9F94();
        c->gpr[2] = 0;
        return 1;
    case VA4820_ENTRY:
        func_800A4820();
        c->gpr[2] = 0;
        return 1;
    case VSETIDX_ENTRY:
        c->gpr[2] = ArchiveSetIndex(c->gpr[4], c->gpr[5]);
        return 1;
    case VGETIND_ENTRY: {
        u32 a = 0x11u, b = 0x22u;

        wr(NULL, c->gpr[4], 4, a);
        wr(NULL, c->gpr[5], 4, b);
        s_trace.getIndCalls++;
        c->gpr[2] = 0;
        return 1;
    }
    case VDSYNC_ENTRY:
        DrawSync(c->gpr[4]);
        c->gpr[2] = 0;
        return 1;
    case VENTERCS_ENTRY:
        EnterCriticalSection();
        c->gpr[2] = 0;
        return 1;
    case VFLUSHC_ENTRY:
        FlushCache();
        c->gpr[2] = 0;
        return 1;
    case VEXITCS_ENTRY:
        ExitCriticalSection();
        c->gpr[2] = 0;
        return 1;
    case V21BF8_ENTRY:
        func_80021BF8((u8*)(uintptr_t)c->gpr[4], (void*)(uintptr_t)c->gpr[5]);
        c->gpr[2] = 0;
        return 1;
    case V8354_ENTRY:
        func_800B8354();
        c->gpr[2] = 0;
        return 1;
    case VF9EC_ENTRY:
        func_800BF9EC();
        c->gpr[2] = 0;
        return 1;
    case VF720_ENTRY:
        c->gpr[2] = func_800BF720();
        return 1;
    case VBE790_ENTRY:
        func_800BE790();
        c->gpr[2] = 0;
        return 1;
    case VWLReset_ENTRY:
        WorkListsReset();
        c->gpr[2] = 0;
        return 1;
    case VGfxAlloc_ENTRY:
        GfxAllocateWorkBuffers(c->gpr[4], c->gpr[5]);
        c->gpr[2] = 0;
        return 1;
    case V89F4_ENTRY:
        func_800B89F4();
        c->gpr[2] = 0;
        return 1;
    case VC1140_ENTRY:
        func_800C1140(c->gpr[4]);
        c->gpr[2] = 0;
        return 1;
    case VA6F98_ENTRY:
        func_800A6F98();
        c->gpr[2] = 0;
        return 1;
    case VBC2F0_ENTRY:
        func_800BC2F0(c->gpr[4]);
        c->gpr[2] = 0;
        return 1;
    case VA9540_ENTRY:
        func_800A9540(c->gpr[4]);
        c->gpr[2] = 0;
        return 1;
    case VBC460_ENTRY:
        func_800BC460((u8*)(uintptr_t)c->gpr[4]);
        c->gpr[2] = 0;
        return 1;
    case VC0FAC_ENTRY:
        c->gpr[2] = func_800C0FAC(c->gpr[4]);
        return 1;
    case VA9FF0_ENTRY:
        func_800A9FF0(c->gpr[4]);
        c->gpr[2] = 0;
        return 1;
    case VBEC18_ENTRY:
        func_800BEC18();
        c->gpr[2] = 0;
        return 1;
    case VVSYNC_ENTRY:
        Vsync(c->gpr[4]);
        c->gpr[2] = 0;
        return 1;
    case VWLREMOVE_ENTRY:
        WorkListRemoveTask(c->gpr[4]);
        c->gpr[2] = 0;
        return 1;
    case VTWLREMOVE_ENTRY:
        TimerWorkListRemoveTask(c->gpr[4]);
        c->gpr[2] = 0;
        return 1;
    case VHEAPFREE_ENTRY:
        HeapFree(c->gpr[4]);
        c->gpr[2] = 0;
        return 1;
    case VSOUNDFREE_ENTRY:
        SoundFreeWdsEntry(c->gpr[4]);
        c->gpr[2] = 0;
        return 1;
    case VWADD_ENTRY: {
        u32 a4 = 0;

        rd(NULL, (c->gpr[29] + 0x10) & 0xFFFFFFFFu, 4, &a4);
        c->gpr[2] = WorkListsAddTasks(c->gpr[4], c->gpr[5],
                                      (void*)(uintptr_t)c->gpr[6],
                                      (void*)(uintptr_t)c->gpr[7],
                                      (void*)(uintptr_t)a4);
        return 1;
    }
    case VB7424_ENTRY:
        func_800B7424(c->gpr[4]);
        c->gpr[2] = 0;
        return 1;
    case VWLSET_ENTRY:
        WorkListSetTaskCallback((void*)(uintptr_t)c->gpr[4],
                                (void*)(uintptr_t)c->gpr[5]);
        c->gpr[2] = 0;
        return 1;
    case VWLFREE_ENTRY:
        WorkListTaskSetOnFreeCallback((void*)(uintptr_t)c->gpr[4],
                                      (void*)(uintptr_t)c->gpr[5]);
        c->gpr[2] = 0;
        return 1;
    default:
        return 0;
    }
}

static u32 run_oracle3(u32 entry, u32 a0, u32 a1, u32 a2)
{
    PcPortMipsBus bus;
    PcPortMipsCpu cpu;

    bus.read = rd;
    bus.write = wr;
    bus.bridge = bridge;
    PcPortMipsCpuInit(&cpu, &bus);
    cpu.gpr[4] = a0;
    cpu.gpr[5] = a1;
    cpu.gpr[6] = a2;
    cpu.gpr[29] = 0x801FF000u;
    cpu.gpr[31] = 0xFFFFFFFCu;
    if (PcPortMipsRun(&cpu, entry, 0xFFFFFFFCu, 4000) != PC_PORT_MIPS_HALTED) {
        fprintf(stderr, "BATTLE CDK FAIL oracle %s @%08x\n", cpu.error, entry);
        exit(1);
    }
    return cpu.gpr[2];
}

static u32 run_oracle5(u32 entry, u32 a0, u32 a1, u32 a2, u32 a3, u32 a4)
{
    u8* sp = s_ram + ((0x801FF000u - 0x80000000u) & 0x1FFFFFu);
    PcPortMipsBus bus;
    PcPortMipsCpu cpu;

    *(u32*)(sp + 0x10) = a4;          /* the fifth argument lives on the stack */
    bus.read = rd;
    bus.write = wr;
    bus.bridge = bridge;
    PcPortMipsCpuInit(&cpu, &bus);
    cpu.gpr[4] = a0;
    cpu.gpr[5] = a1;
    cpu.gpr[6] = a2;
    cpu.gpr[7] = a3;
    cpu.gpr[29] = 0x801FF000u;
    cpu.gpr[31] = 0xFFFFFFFCu;
    if (PcPortMipsRun(&cpu, entry, 0xFFFFFFFCu, 4000) != PC_PORT_MIPS_HALTED) {
        fprintf(stderr, "BATTLE CDK FAIL oracle5 %s @%08x\n", cpu.error, entry);
        exit(1);
    }
    return cpu.gpr[2];
}

static u8* actor(void)
{
    return s_ram + (ACTOR & 0x1FFFFFu);
}

static u8* arg(void)
{
    return s_ram + (ARG & 0x1FFFFFu);
}

static u8* task(void)
{
    return s_ram + (TASK & 0x1FFFFFu);
}

static u8* aux(void)
{
    return s_ram + (AUX & 0x1FFFFFu);
}

static void init_fixture(unsigned seed)
{
    u8* a0 = actor();

    memset(a0, 0, 0x200);
    memset(aux(), 0, 0x100);
    *(u32*)(a0 + 0x04) = AUX;
    *(u32*)(a0 + 0x6C) = 0x80030000u + seed * 4u;
    *(u32*)(a0 + 0x74) = 0x11110000u + seed * 0x1111u;
    *(u16*)(a0 + 0x34) = (u16)(0x1000u + seed * 7u);
    *(u8*)(a0 + 0xAF) = (u8)(0x80u + seed * 3u);
    *(u32*)(a0 + 0xAC) = 0x40000000u;
    *(u32*)(a0 + 0x48) = 0x00100000u + seed;
    *(u8*)(a0 + 0x7C) = (u8)(seed & 1);
    *(s32*)(a0 + 0x88) = (s32)(seed & 2) - 1;
    *(u32*)(aux() + 0x38) = 0x22000000u + seed * 0x101u;

    memset(arg(), 0, 0x10);
    arg()[0] = (u8)(seed * 0x13u + 1u);

    memset(task(), 0xA5, 0x40);
    memset(&s_trace, 0, sizeof(s_trace));
}

static void expect_eq(const char* field, u32 actual, u32 expected, unsigned seed);

/* Everything both sides share must agree; the two callback words are address
 * space values and are compared per side instead. */
static void expect_common(const Trace* h, const Trace* o, unsigned seed)
{
    expect_eq("allocCalls", h->allocCalls, o->allocCalls, seed);
    expect_eq("allocOwner", h->allocOwner, o->allocOwner, seed);
    expect_eq("allocSize", h->allocSize, o->allocSize, seed);
    expect_eq("cbCalls", h->cbCalls, o->cbCalls, seed);
    expect_eq("cbTask", h->cbTask, o->cbTask, seed);
    expect_eq("helperCalls", h->helperCalls, o->helperCalls, seed);
    expect_eq("helperArg", h->helperArg, o->helperArg, seed);
    expect_eq("helperRet", h->helperRet, o->helperRet, seed);
    expect_eq("tailCalls", h->tailCalls, o->tailCalls, seed);
    expect_eq("tailTask", h->tailTask, o->tailTask, seed);
    expect_eq("df1cCalls", h->df1cCalls, o->df1cCalls, seed);
    expect_eq("e148Calls", h->e148Calls, o->e148Calls, seed);
    expect_eq("e148Arg", h->e148Arg, o->e148Arg, seed);
    expect_eq("c08ccCalls", h->c08ccCalls, o->c08ccCalls, seed);
    expect_eq("c08ccA0", h->c08ccA0, o->c08ccA0, seed);
    expect_eq("c08ccA1", h->c08ccA1, o->c08ccA1, seed);
    expect_eq("d245Calls", h->d245Calls, o->d245Calls, seed);
    expect_eq("d245A0", h->d245A0, o->d245A0, seed);
    expect_eq("d245A1", h->d245A1, o->d245A1, seed);
    expect_eq("waddCalls", h->waddCalls, o->waddCalls, seed);
    expect_eq("waddA0", h->waddA0, o->waddA0, seed);
    expect_eq("waddA1", h->waddA1, o->waddA1, seed);
    expect_eq("b7424Calls", h->b7424Calls, o->b7424Calls, seed);
    expect_eq("b7424A0", h->b7424A0, o->b7424A0, seed);
    expect_eq("wlsetCalls", h->wlsetCalls, o->wlsetCalls, seed);
    expect_eq("wlsetTask", h->wlsetTask, o->wlsetTask, seed);
    expect_eq("wlfreeCalls", h->wlfreeCalls, o->wlfreeCalls, seed);
    expect_eq("wlfreeTask", h->wlfreeTask, o->wlfreeTask, seed);
    expect_eq("b7160", h->b7160Calls, o->b7160Calls, seed);
    expect_eq("b7c34", h->b7c34Calls, o->b7c34Calls, seed);
    expect_eq("b7e94", h->b7e94Calls, o->b7e94Calls, seed);
    expect_eq("b39c0", h->b39c0Calls, o->b39c0Calls, seed);
    expect_eq("b39c0.a0", h->b39c0A0, o->b39c0A0, seed);
    expect_eq("b39c0.a1", h->b39c0A1, o->b39c0A1, seed);
    expect_eq("b39c0.a2", h->b39c0A2, o->b39c0A2, seed);
    expect_eq("b39c0.a3", h->b39c0A3, o->b39c0A3, seed);
    expect_eq("b39c0.a4", h->b39c0A4, o->b39c0A4, seed);
    expect_eq("heapFree", h->heapFreeCalls, o->heapFreeCalls, seed);
    expect_eq("heapFree.a0", h->heapFreeA0, o->heapFreeA0, seed);
    expect_eq("soundFree", h->soundFreeCalls, o->soundFreeCalls, seed);
    expect_eq("soundFree.a0", h->soundFreeA0, o->soundFreeA0, seed);
    expect_eq("wlRemove", h->wlRemoveCalls, o->wlRemoveCalls, seed);
    expect_eq("twlRemove", h->twlRemoveCalls, o->twlRemoveCalls, seed);
    expect_eq("c1140", h->c1140Calls, o->c1140Calls, seed);
    expect_eq("c1140.a0", h->c1140A0, o->c1140A0, seed);
    expect_eq("a6f98", h->a6f98Calls, o->a6f98Calls, seed);
    expect_eq("bc2f0", h->bc2f0Calls, o->bc2f0Calls, seed);
    expect_eq("bc2f0.a0", h->bc2f0A0, o->bc2f0A0, seed);
    expect_eq("a9540", h->a9540Calls, o->a9540Calls, seed);
    expect_eq("a9540.a0", h->a9540A0, o->a9540A0, seed);
    expect_eq("bc460", h->bc460Calls, o->bc460Calls, seed);
    expect_eq("bc460.a0", h->bc460A0, o->bc460A0, seed);
    expect_eq("c0fac", h->c0facCalls, o->c0facCalls, seed);
    expect_eq("c0fac.a0", h->c0facA0, o->c0facA0, seed);
    expect_eq("a9ff0", h->a9ff0Calls, o->a9ff0Calls, seed);
    expect_eq("a9ff0.a0", h->a9ff0A0, o->a9ff0A0, seed);
    expect_eq("bec18", h->bec18Calls, o->bec18Calls, seed);
    expect_eq("vsync", h->vsyncCalls, o->vsyncCalls, seed);
    expect_eq("vsync.a0", h->vsyncA0, o->vsyncA0, seed);
    expect_eq("be790", h->be790Calls, o->be790Calls, seed);
    expect_eq("wlReset", h->wlResetCalls, o->wlResetCalls, seed);
    expect_eq("gfxAlloc", h->gfxAllocCalls, o->gfxAllocCalls, seed);
    expect_eq("gfxAlloc.a0", h->gfxAllocA0, o->gfxAllocA0, seed);
    expect_eq("setIdx", h->setIdxCalls, o->setIdxCalls, seed);
    expect_eq("setIdx.dir", h->setIdxDir, o->setIdxDir, seed);
    expect_eq("setIdx.entry", h->setIdxEntry, o->setIdxEntry, seed);
    expect_eq("decSize", h->decSizeCalls, o->decSizeCalls, seed);
    expect_eq("heapAlloc", h->heapAllocCalls, o->heapAllocCalls, seed);
    expect_eq("readBuf", h->readBufCalls, o->readBufCalls, seed);
    expect_eq("dde4", h->dde4Calls, o->dde4Calls, seed);
    expect_eq("b21bf8", h->b21bf8Calls, o->b21bf8Calls, seed);
    expect_eq("getInd", h->getIndCalls, o->getIndCalls, seed);
    expect_eq("dsync", h->dsyncCalls, o->dsyncCalls, seed);
    expect_eq("enterCs", h->enterCsCalls, o->enterCsCalls, seed);
    expect_eq("flushCache", h->flushCacheCalls, o->flushCacheCalls, seed);
    expect_eq("exitCs", h->exitCsCalls, o->exitCsCalls, seed);
    expect_eq("sfind", h->sfindCalls, o->sfindCalls, seed);
    expect_eq("sload", h->sloadCalls, o->sloadCalls, seed);
    expect_eq("bdfc", h->bdfcCalls, o->bdfcCalls, seed);
    expect_eq("badd4", h->badd4Calls, o->badd4Calls, seed);
    expect_eq("gffFree", h->gffFreeCalls, o->gffFreeCalls, seed);
    expect_eq("wlFreeAll", h->wlFreeAllCalls, o->wlFreeAllCalls, seed);
    expect_eq("b3852c", h->b3852cCalls, o->b3852cCalls, seed);
    expect_eq("a9f94", h->a9f94Calls, o->a9f94Calls, seed);
    expect_eq("a4820", h->a4820Calls, o->a4820Calls, seed);
}

static void expect_actor_bytes(const u8* oracle_actor, const char* name,
                               unsigned seed)
{
    s_checks++;
    if (memcmp(oracle_actor, actor(), 0x200) != 0) {
        fprintf(stderr, "BATTLE CDK FAIL %s actor bytes seed=%u\n", name, seed);
        exit(1);
    }
}

static void expect_eq(const char* field, u32 actual, u32 expected, unsigned seed)
{
    s_checks++;
    if (actual != expected) {
        fprintf(stderr, "BATTLE CDK FAIL %s seed=%u actual=%08x expected=%08x\n",
                field, seed, actual, expected);
        exit(1);
    }
}

/* Runs one spawner on both sides from the same fixture and compares every
 * observable: return value, recorded callee arguments, callback address, task
 * buffer and actor bytes. */
static void check_spawner(const char* name, u32 entry, u32 host_cb,
                          u32 oracle_cb, unsigned seed)
{
    Trace oracle_trace;
    u8 oracle_task[0x40];
    u8 oracle_actor[0x200];
    u8 host_task[0x40];
    u32 oracle_ret;
    u32 host_ret;

    init_fixture(seed);
    if (entry == SLICE_F7C8) {
        run_oracle3(entry, ACTOR, 0x2000u + seed, 0x3000u + seed * 2u);
        oracle_ret = 0;
    } else if (entry == SLICE_5C18) {
        oracle_ret = run_oracle3(entry, ACTOR, ARG, 0);
    } else {
        oracle_ret = run_oracle3(entry, ACTOR, 0x2000u + seed, 0x3000u + seed * 2u);
    }
    oracle_trace = s_trace;
    memcpy(oracle_task, task(), sizeof(oracle_task));
    memcpy(oracle_actor, actor(), sizeof(oracle_actor));

    init_fixture(seed);
    if (entry == SLICE_F7C8) {
        func_800BF7C8(actor(), 0x2000u + seed, 0x3000u + seed * 2u);
        host_ret = 0;
    } else if (entry == SLICE_5C18) {
        host_ret = guest_of(func_800B5C18(actor(), arg()));
    } else {
        host_ret = guest_of(func_800B5924(actor(), 0x2000u + seed, 0x3000u + seed * 2u));
    }

    expect_eq("ret", host_ret, oracle_ret, seed);
    expect_eq("oracle.cbCallback", oracle_trace.cbCallback, oracle_cb, seed);
    expect_eq("c.cbCallback", s_trace.cbCallback, host_cb, seed);
    expect_eq("c.allocCalls", s_trace.allocCalls, oracle_trace.allocCalls, seed);
    expect_eq("c.allocOwner", s_trace.allocOwner, oracle_trace.allocOwner, seed);
    expect_eq("c.allocSize", s_trace.allocSize, oracle_trace.allocSize, seed);
    expect_eq("c.cbCalls", s_trace.cbCalls, oracle_trace.cbCalls, seed);
    expect_eq("c.cbTask", s_trace.cbTask, oracle_trace.cbTask, seed);
    expect_eq("c.helperCalls", s_trace.helperCalls, oracle_trace.helperCalls, seed);
    expect_eq("c.helperArg", s_trace.helperArg, oracle_trace.helperArg, seed);
    expect_eq("c.helperRet", s_trace.helperRet, oracle_trace.helperRet, seed);
    expect_eq("c.tailCalls", s_trace.tailCalls, oracle_trace.tailCalls, seed);
    expect_eq("c.tailTask", s_trace.tailTask, oracle_trace.tailTask, seed);

    /* +0x1C holds the actor pointer, so each side stores its own address-space
     * value: the oracle the retail guest address, the C run the host pointer.
     * Every other word must agree (the comparison is per word because a 64-bit
     * host pointer store also touches +0x20). */
    memcpy(host_task, task(), sizeof(host_task));
    expect_eq("oracle.task+1C", *(u32*)(oracle_task + 0x1C), ACTOR, seed);
    {
        void* stored = NULL;

        memcpy(&stored, host_task + 0x1C, sizeof(stored));
        expect_eq("c.task+1C", (u32)(uintptr_t)stored,
                  (u32)(uintptr_t)actor(), seed);
    }
    {
        static const unsigned offsets[] = {
            0x00, 0x04, 0x08, 0x0C, 0x10, 0x14, 0x18, 0x20,
            0x24, 0x28, 0x2C, 0x30, 0x34, 0x38, 0x3C
        };
        unsigned k;

        for (k = 0; k < sizeof(offsets) / sizeof(offsets[0]); ++k) {
            char field[32];

            sprintf(field, "%s.task+%02X", name, offsets[k]);
            expect_eq(field, *(u32*)(host_task + offsets[k]),
                      *(u32*)(oracle_task + offsets[k]), seed);
        }
    }
    s_checks++;
    if (memcmp(oracle_actor, actor(), sizeof(oracle_actor)) != 0) {
        fprintf(stderr, "BATTLE CDK FAIL %s actor bytes seed=%u\n", name, seed);
        exit(1);
    }
}

static void check_5dc4(unsigned seed)
{
    Trace o;
    u8 oa[0x200];

    init_fixture(seed);
    run_oracle3(SLICE_5DC4, ACTOR, 0, 0);
    o = s_trace;
    memcpy(oa, actor(), sizeof(oa));

    init_fixture(seed);
    func_800B5DC4(actor());

    expect_common(&s_trace, &o, seed);
    expect_eq("5dc4.cbCalls", s_trace.cbCalls, 1, seed);
    expect_eq("oracle.5dc4.cbCallback", o.cbCallback, 0x800B5CC0u, seed);
    expect_eq("c.5dc4.cbCallback", s_trace.cbCallback,
              (u32)(uintptr_t)func_800B5CC0, seed);
    expect_eq("c.5dc4.actor+34", *(u16*)(actor() + 0x34), 1, seed);
    expect_actor_bytes(oa, "5DC4", seed);
}

static void check_56e4(unsigned seed)
{
    Trace o;

    init_fixture(seed);
    run_oracle3(SLICE_56E4, ACTOR, 0, 0);
    o = s_trace;

    init_fixture(seed);
    /* the aux pointer field is read as u32 and cast back, so it must hold the
     * host address on the C side (the binary is non-PIE, so it fits in 32 bits) */
    *(u32*)(actor() + 0x04) = (u32)(uintptr_t)aux();
    func_800B56E4(actor());

    expect_common(&s_trace, &o, seed);
    expect_eq("5 E148.calls", s_trace.e148Calls, 1, seed);
    expect_eq("5 E148.arg", s_trace.e148Arg, *(u32*)(aux() + 0x38), seed);
    expect_eq("5 C08CC.calls", s_trace.c08ccCalls, 1, seed);
    expect_eq("5 C08CC.a0", s_trace.c08ccA0, 5, seed);
    expect_eq("oracle.56e4.cb", o.c08ccA2, 0x800B51B0u, seed);
    expect_eq("c.56e4.cb", s_trace.c08ccA2, (u32)(uintptr_t)func_800B51B0, seed);
}

static void check_dc78(unsigned seed, unsigned* saw_callback)
{
    Trace o;
    u8 oa[0x200];

    init_fixture(seed);
    run_oracle3(SLICE_DC78, ACTOR, 0, 0);
    o = s_trace;
    memcpy(oa, actor(), sizeof(oa));

    init_fixture(seed);
    func_800BDC78(actor());

    expect_common(&s_trace, &o, seed);
    expect_eq("dc78.df1c", s_trace.df1cCalls, 1, seed);
    expect_eq("oracle.dc78.cb", o.cbCallback,
              o.cbCalls ? 0x800BDC14u : 0u, seed);
    expect_eq("c.dc78.cb", s_trace.cbCallback,
              s_trace.cbCalls ? (u32)(uintptr_t)func_800BDC14 : 0u, seed);
    if (s_trace.cbCalls) {
        (*saw_callback)++;
    }
    expect_actor_bytes(oa, "DC78", seed);
}

static void check_64d4(unsigned seed)
{
    Trace o;
    unsigned k;
    u32 expected;

    /* D_800C3EB0 + (a1[1] << 2) + 0x8C8C indexes a u32 table */
    for (k = 0; k < 4; ++k) {
        *(u32*)(D_800C3EB0 + 0x8C8C + k * 4) = 0x33000000u + k * 0x101u + seed;
    }

    init_fixture(seed);
    arg()[0] = (u8)(0x40u + seed);
    arg()[1] = (u8)(seed & 3);
    run_oracle3(SLICE_64D4, ACTOR, ARG, 0);
    o = s_trace;
    expected = *(u32*)(D_800C3EB0 + 0x8C8C + (seed & 3) * 4);

    init_fixture(seed);
    arg()[0] = (u8)(0x40u + seed);
    arg()[1] = (u8)(seed & 3);
    func_800B64D4(actor(), arg());

    expect_common(&s_trace, &o, seed);
    expect_eq("64d4.calls", s_trace.d245Calls, 1, seed);
    expect_eq("oracle.64d4.a0", o.d245A0, expected, seed);
    expect_eq("c.64d4.a0", s_trace.d245A0, expected, seed);
    expect_eq("oracle.64d4.a1", o.d245A1, 0x40u + seed, seed);
    expect_eq("c.64d4.a1", s_trace.d245A1, 0x40u + seed, seed);
}

static void check_73a0(unsigned seed)
{
    Trace o;

    init_fixture(seed);
    run_oracle3(SLICE_73A0, 0, 0, 0);
    o = s_trace;

    init_fixture(seed);
    func_800B73A0();

    expect_common(&s_trace, &o, seed);
    expect_eq("73a0.waddCalls", s_trace.waddCalls, 1, seed);
    expect_eq("73a0.wadd.a0", s_trace.waddA0, 0x10F7Cu, seed);
    expect_eq("73a0.wadd.a1", s_trace.waddA1, 0u, seed);
    expect_eq("oracle.wadd.a2", o.waddA2, 0x800B6F0Cu, seed);
    expect_eq("oracle.wadd.a3", o.waddA3, 0x800B7134u, seed);
    expect_eq("oracle.wadd.a4", o.waddA4, 0x800B7364u, seed);
    expect_eq("c.wadd.a2", s_trace.waddA2, (u32)(uintptr_t)func_800B6F0C, seed);
    expect_eq("c.wadd.a3", s_trace.waddA3, (u32)(uintptr_t)func_800B7134, seed);
    expect_eq("c.wadd.a4", s_trace.waddA4, (u32)(uintptr_t)func_800B7364, seed);
    expect_eq("73a0.b7424.calls", s_trace.b7424Calls, 1, seed);
    expect_eq("73a0.b7424.a0", s_trace.b7424A0, 0x80190000u, seed);
}

#define SLOT 0x80104100u
#define BOARD 0x80104000u
#define NODE 0x80104200u

static void check_bf5e8(unsigned seed)
{
    Trace o;
    u32 oracle_val;

    init_fixture(seed);
    D_800C3CE8[0] = 0x1000u + seed;
    run_oracle3(SLICE_BF5E8, 0, 0, 0);
    o = s_trace;
    oracle_val = D_800C3CE8[0];

    init_fixture(seed);
    D_800C3CE8[0] = 0x1000u + seed;
    func_800BF5E8();

    expect_common(&s_trace, &o, seed);
    expect_eq("bf5e8.value", D_800C3CE8[0], oracle_val, seed);
    expect_eq("bf5e8.expected", D_800C3CE8[0], 0x1001u + seed, seed);
}

static void check_b6438(unsigned seed)
{
    Trace o;
    u8* a0 = actor();

    init_fixture(seed);
    *(u32*)(a0 + 0x6C) = 0x80110000u;
    run_oracle3(SLICE_B6438, ACTOR, 0, 0);
    o = s_trace;

    init_fixture(seed);
    *(u32*)(a0 + 0x6C) = (u32)(uintptr_t)(s_ram + 0x110000);
    func_800B6438(a0);

    expect_common(&s_trace, &o, seed);
    expect_eq("b6438.calls", s_trace.wlsetCalls, 1, seed);
    expect_eq("oracle.b6438.task", o.wlsetTask, 0x8011001Cu, seed);
    expect_eq("c.b6438.task", s_trace.wlsetTask, 0x8011001Cu, seed);
    expect_eq("oracle.b6438.cb", o.wlsetCallback, 0x80025A88u, seed);
    expect_eq("c.b6438.cb", s_trace.wlsetCallback, (u32)(uintptr_t)D_80025A88, seed);
}

static void check_7adf4(unsigned seed)
{
    Trace o;
    u8* slot = s_ram + (SLOT & 0x1FFFFFu);
    u8* board = s_ram + (BOARD & 0x1FFFFFu);
    u8 oracle_row[0x400];
    u8 index = (u8)(seed & 3);
    unsigned k;

    for (k = 0; k < sizeof(oracle_row); ++k) {
        D_800D3420[k] = (u8)(0x10u + k + seed);
    }
    board[1] = (u8)(seed & 7);
    board[2] = (u8)(0x21u + seed);
    board[3] = (u8)(seed & 0x0Fu);

    init_fixture(seed);
    *(u32*)slot = BOARD;
    run_oracle3(SLICE_7ADF4, SLOT, index, 0);
    o = s_trace;
    memcpy(oracle_row, D_800D3420, sizeof(oracle_row));

    for (k = 0; k < sizeof(oracle_row); ++k) {
        D_800D3420[k] = (u8)(0x10u + k + seed);
    }
    init_fixture(seed);
    *(u32*)slot = (u32)(uintptr_t)board;
    func_8007ADF4((u8**)slot, index);

    expect_common(&s_trace, &o, seed);
    s_checks++;
    if (memcmp(oracle_row, D_800D3420, sizeof(oracle_row)) != 0) {
        fprintf(stderr, "BATTLE CDK FAIL 7ADF4 row seed=%u\n", seed);
        exit(1);
    }
}

static void check_b16a4(unsigned seed)
{
    Trace o;
    u8* p = s_ram + (NODE & 0x1FFFFFu);
    u32 oracle_ret;
    u32 host_ret;
    unsigned k;
    const unsigned off = 0x20;

    memset(p, 0, 0x100);
    *(u32*)(p + 0x10) = off;
    *(u32*)(p + 0x14) = 3;
    for (k = 0; k < 3; ++k) {
        p[off + k * 0x10 + 0] = (u8)(2u + k);
        p[off + k * 0x10 + 1] = (u8)(1u + k);
    }

    init_fixture(seed);
    oracle_ret = run_oracle3(SLICE_B16A4, NODE, 0, 0);
    o = s_trace;

    init_fixture(seed);
    host_ret = (u32)func_800B16A4(p);

    expect_common(&s_trace, &o, seed);
    expect_eq("b16a4.ret", host_ret, oracle_ret, seed);
    expect_eq("b16a4.nonzero", (host_ret != 0) ? 1u : 0u, 1u, seed);
}

static void check_b35c0(unsigned seed)
{
    Trace o;
    u8 oracle_task[0x40];
    u8 host_task[0x40];
    u16 oracle_c[3];
    u32 oracle_slot;
    u32 oracle_ret;
    u32 host_ret;
    unsigned branch = seed & 1;
    u8* t = s_ram + 0x170040;

    /* ---------- retail oracle (branch 1 reuses the slot contents) ---------- */
    memset(t, 0, 0x40);
    *(u16*)(t + 0x1C) = (u16)(0x1111u + seed);
    *(u16*)(t + 0x1E) = (u16)(0x2222u + seed);
    *(u16*)(t + 0x20) = (u16)(0x3333u + seed);
    init_fixture(seed);
    memset(s_ram + (TASK & 0x1FFFFFu), 0xA5, 0x40);
    D_800C3548[0] = branch ? 0x80170040u : 0u;
    oracle_ret = run_oracle3(SLICE_B35C0, 0, 0, 0);
    o = s_trace;
    oracle_slot = D_800C3548[0];
    memcpy(oracle_task, s_ram + (TASK & 0x1FFFFFu), sizeof(oracle_task));
    oracle_c[0] = *(u16*)(t + 0x24);
    oracle_c[1] = *(u16*)(t + 0x26);
    oracle_c[2] = *(u16*)(t + 0x28);

    /* ---------- shipped C ---------- */
    memset(t, 0, 0x40);
    *(u16*)(t + 0x1C) = (u16)(0x1111u + seed);
    *(u16*)(t + 0x1E) = (u16)(0x2222u + seed);
    *(u16*)(t + 0x20) = (u16)(0x3333u + seed);
    init_fixture(seed);
    memset(s_ram + (TASK & 0x1FFFFFu), 0xA5, 0x40);
    D_800C3548[0] = branch ? (u32)(uintptr_t)t : 0u;
    host_ret = guest_of(func_800B35C0());
    memcpy(host_task, s_ram + (TASK & 0x1FFFFFu), sizeof(host_task));

    expect_common(&s_trace, &o, seed);
    expect_eq("b35c0.ret", host_ret, oracle_ret, seed);
    if (branch) {
        expect_eq("b35c0.copy+24", *(u16*)(t + 0x24), oracle_c[0], seed);
        expect_eq("b35c0.copy+26", *(u16*)(t + 0x26), oracle_c[1], seed);
        expect_eq("b35c0.copy+28", *(u16*)(t + 0x28), oracle_c[2], seed);
        expect_eq("b35c0.copy.exp", oracle_c[0], 0x1111u + seed, seed);
    } else {
        s_checks++;
        if (memcmp(oracle_task, host_task, sizeof(oracle_task)) != 0) {
            fprintf(stderr, "BATTLE CDK FAIL b35c0 task seed=%u\n", seed);
            exit(1);
        }
        expect_eq("oracle.b35c0.cb", o.cbCallback, 0x800B3358u, seed);
        expect_eq("c.b35c0.cb", s_trace.cbCallback,
                  (u32)(uintptr_t)func_800B3358, seed);
        expect_eq("b35c0.cbTask", s_trace.cbTask, o.cbTask, seed);
        expect_eq("oracle.b35c0.free", o.wlfreeCallback, 0x800B3588u, seed);
        expect_eq("c.b35c0.free", s_trace.wlfreeCallback,
                  (u32)(uintptr_t)func_800B3588, seed);
        expect_eq("oracle.b35c0.slot", oracle_slot, TASK, seed);
        expect_eq("c.b35c0.slot", guest_of((void*)(uintptr_t)D_800C3548[0]),
                  TASK, seed);
    }
}

static void check_setters(unsigned seed)
{
    Trace o;
    u8 s8;
    u16 s16;
    u32 s32, s32b;
    u16 arg16 = (u16)(0x1200u + seed * 3u);
    u32 arg32 = 0x77000000u + seed * 0x101u;

    D_800D2FDC[0] = (u8)(0x40u + seed);
    init_fixture(seed);
    run_oracle3(SLICE_7C28, 0, 0, 0);
    o = s_trace;
    s8 = D_800D2FDC[0];
    D_800D2FDC[0] = (u8)(0x40u + seed);
    init_fixture(seed);
    func_800B7C28();
    expect_common(&s_trace, &o, seed);
    expect_eq("7C28", D_800D2FDC[0], s8, seed);
    expect_eq("7C28.zero", s8, 0u, seed);

    D_800C3D14[0] = (u16)(0x1500u + seed);
    init_fixture(seed);
    run_oracle3(SLICE_BCD8C, 0, 0, 0);
    o = s_trace;
    s16 = D_800C3D14[0];
    D_800C3D14[0] = (u16)(0x1500u + seed);
    init_fixture(seed);
    func_800BCD8C();
    expect_common(&s_trace, &o, seed);
    expect_eq("BCD8C", D_800C3D14[0], s16, seed);

    D_800C3628[0] = 0x11110000u;
    init_fixture(seed);
    run_oracle3(SLICE_BF730, arg32, 0, 0);
    o = s_trace;
    s32 = D_800C3628[0];
    D_800C3628[0] = 0x11110000u;
    init_fixture(seed);
    func_800BF730(arg32);
    expect_common(&s_trace, &o, seed);
    expect_eq("BF730", D_800C3628[0], s32, seed);
    expect_eq("BF730.arg", s32, arg32, seed);

    D_800C3740[0] = 0x3333u;
    init_fixture(seed);
    run_oracle3(SLICE_BC454, arg16, 0, 0);
    o = s_trace;
    s16 = D_800C3740[0];
    D_800C3740[0] = 0x3333u;
    init_fixture(seed);
    func_800BC454(arg16);
    expect_common(&s_trace, &o, seed);
    expect_eq("BC454", D_800C3740[0], s16, seed);
    expect_eq("BC454.arg", s16, arg16, seed);

    D_800C367C[0] = 0x22220000u;
    init_fixture(seed);
    run_oracle3(SLICE_BC3F8, arg32, 0, 0);
    o = s_trace;
    s32 = D_800C367C[0];
    D_800C367C[0] = 0x22220000u;
    init_fixture(seed);
    func_800BC3F8(arg32);
    expect_common(&s_trace, &o, seed);
    expect_eq("BC3F8", D_800C367C[0], s32, seed);

    D_800591B4[0] = 0x4444u;
    D_800591B1[0] = 0x55u;
    init_fixture(seed);
    run_oracle3(SLICE_B8054, arg16, 0, 0);
    o = s_trace;
    s16 = D_800591B4[0];
    s8 = D_800591B1[0];
    D_800591B4[0] = 0x4444u;
    D_800591B1[0] = 0x55u;
    init_fixture(seed);
    func_800B8054(arg16);
    expect_common(&s_trace, &o, seed);
    expect_eq("B8054.h", D_800591B4[0], s16, seed);
    expect_eq("B8054.b", D_800591B1[0], s8, seed);
    expect_eq("B8054.b.zero", s8, 0u, seed);

    D_800D2D68[0] = 0x66660000u;
    D_800C374C[0] = 0x77770000u;
    init_fixture(seed);
    run_oracle3(SLICE_BE108, 0, 0, 0);
    o = s_trace;
    s32 = D_800D2D68[0];
    s32b = D_800C374C[0];
    D_800D2D68[0] = 0x66660000u;
    D_800C374C[0] = 0x77770000u;
    init_fixture(seed);
    func_800BE108();
    expect_common(&s_trace, &o, seed);
    expect_eq("BE108.a", D_800D2D68[0], s32, seed);
    expect_eq("BE108.b", D_800C374C[0], s32b, seed);
    expect_eq("BE108.a.zero", s32, 0u, seed);
    expect_eq("BE108.b.zero", s32b, 0u, seed);

    D_800C3E20[0] = 0x88880000u;
    D_800C3610[0] = 0x99990000u;
    D_800D2E54[0] = 0xAAAAu;
    init_fixture(seed);
    run_oracle3(SLICE_BED30, 0, 0, 0);
    o = s_trace;
    s32 = D_800C3E20[0];
    s32b = D_800C3610[0];
    s16 = D_800D2E54[0];
    D_800C3E20[0] = 0x88880000u;
    D_800C3610[0] = 0x99990000u;
    D_800D2E54[0] = 0xAAAAu;
    init_fixture(seed);
    func_800BED30();
    expect_common(&s_trace, &o, seed);
    expect_eq("BED30.a", D_800C3E20[0], s32, seed);
    expect_eq("BED30.b", D_800C3610[0], s32b, seed);
    expect_eq("BED30.c", D_800D2E54[0], s16, seed);
    expect_eq("BED30.a.zero", s32, 0u, seed);
}

static void check_b9b30(unsigned seed)
{
    Trace o;
    u8* buf = s_ram + 0x120000;
    u8 oracle_bytes[0x50];

    memset(buf, 0, 0x50);
    buf[0x1C] = (u8)(0x21u + seed);
    D_800C3610[0] = 0x80120000u;
    init_fixture(seed);
    run_oracle3(SLICE_B9B30, 0, 0, 0);
    o = s_trace;
    memcpy(oracle_bytes, buf, sizeof(oracle_bytes));

    memset(buf, 0, 0x50);
    buf[0x1C] = (u8)(0x21u + seed);
    D_800C3610[0] = (u32)(uintptr_t)buf;
    init_fixture(seed);
    func_800B9B30();

    expect_common(&s_trace, &o, seed);
    s_checks++;
    if (memcmp(oracle_bytes, buf, sizeof(oracle_bytes)) != 0) {
        fprintf(stderr, "BATTLE CDK FAIL b9b30 bytes seed=%u\n", seed);
        exit(1);
    }
    expect_eq("b9b30.48", buf[0x48], 1u, seed);
    expect_eq("b9b30.49", buf[0x49], 0x21u + seed, seed);
}

static void check_batch51(unsigned seed)
{
    Trace o;
    u8* a0 = actor();
    u32 s32, s32b;
    u8 s8bytes[0x50];
    u32 argv = 0x5A5A0000u + seed;

    /* func_800B8048(v) */
    D_800C3E1C[0] = 0x1111u;
    init_fixture(seed);
    run_oracle3(SLICE_8048, argv, 0, 0);
    o = s_trace;
    s32 = D_800C3E1C[0];
    D_800C3E1C[0] = 0x1111u;
    init_fixture(seed);
    func_800B8048(argv);
    expect_common(&s_trace, &o, seed);
    expect_eq("8048", D_800C3E1C[0], s32, seed);
    expect_eq("8048.arg", s32, argv, seed);

    /* func_800B7134() */
    g_GfxCurOT[0] = 0x22220000u + seed;
    D_800C3CB4[0] = 0x3333u;
    init_fixture(seed);
    run_oracle3(SLICE_7134, 0, 0, 0);
    o = s_trace;
    s32 = D_800C3CB4[0];
    g_GfxCurOT[0] = 0x22220000u + seed;
    D_800C3CB4[0] = 0x3333u;
    init_fixture(seed);
    func_800B7134();
    expect_common(&s_trace, &o, seed);
    expect_eq("7134", D_800C3CB4[0], s32, seed);
    expect_eq("7134.src", s32, 0x22220000u + seed, seed);
    expect_eq("7134.calls", s_trace.b7160Calls, 1, seed);

    /* func_800B16F0() */
    *(u32*)(s_ram + 0x120800) = 0x40u;
    s_ram[0x120800] = 0;
    s_ram[0x120801] = (u8)(seed + 1);
    D_800C3BEC[0] = 0x80120800u;
    D_800C3BF0[0] = 0x5000u + seed;
    init_fixture(seed);
    run_oracle3(SLICE_16F0, 0, 0, 0);
    o = s_trace;
    s32 = D_800C3BEC[0];
    s32b = D_800C3BF0[0];
    s_ram[0x120800] = 0;
    s_ram[0x120801] = (u8)(seed + 1);
    D_800C3BEC[0] = (u32)(uintptr_t)(s_ram + 0x120800);
    D_800C3BF0[0] = 0x5000u + seed;
    init_fixture(seed);
    func_800B16F0();
    expect_common(&s_trace, &o, seed);
    /* the C side stores a host pointer: normalise before comparing */
    expect_eq("16F0.ptr", guest_of((void*)(uintptr_t)D_800C3BEC[0]), s32, seed);
    expect_eq("16F0.counter", D_800C3BF0[0], s32b, seed);
    expect_eq("16F0.counter.exp", s32b, 0x5001u + seed, seed);

    /* func_800B8068() */
    D_800591B1[0] = 0x11u;
    init_fixture(seed);
    run_oracle3(SLICE_8068, 0, 0, 0);
    o = s_trace;
    s32 = D_800591B1[0];
    D_800591B1[0] = 0x11u;
    init_fixture(seed);
    func_800B8068();
    expect_common(&s_trace, &o, seed);
    expect_eq("8068", D_800591B1[0], s32, seed);
    expect_eq("8068.exp", s32, 1u, seed);

    /* func_800B3588(p) */
    D_800C3548[0] = 0x7777u;
    init_fixture(seed);
    run_oracle3(SLICE_3588, ACTOR, 0, 0);
    o = s_trace;
    s32 = D_800C3548[0];
    D_800C3548[0] = 0x7777u;
    init_fixture(seed);
    func_800B3588(a0);
    expect_common(&s_trace, &o, seed);
    expect_eq("3588.slot", D_800C3548[0], s32, seed);
    expect_eq("3588.slot.exp", s32, 0u, seed);
    expect_eq("3588.heap", o.heapFreeA0, ACTOR, seed);
    expect_eq("3588.heap.c", s_trace.heapFreeA0, ACTOR, seed);
    expect_eq("3588.twl", o.twlRemoveA0, ACTOR, seed);
    expect_eq("3588.twl.c", s_trace.twlRemoveA0, (u32)(uintptr_t)a0, seed);

    /* func_800B383C(p) */
    D_800C3558[0] = 0x6666u;
    init_fixture(seed);
    run_oracle3(SLICE_383C, ACTOR, 0, 0);
    o = s_trace;
    s32 = D_800C3558[0];
    D_800C3558[0] = 0x6666u;
    init_fixture(seed);
    func_800B383C(a0);
    expect_common(&s_trace, &o, seed);
    expect_eq("383C.slot", D_800C3558[0], s32, seed);
    expect_eq("383C.wl", o.wlRemoveA0, ACTOR + 0x1Cu, seed);
    expect_eq("383C.wl.c", s_trace.wlRemoveA0, (u32)(uintptr_t)(a0 + 0x1C), seed);
    expect_eq("383C.twl", o.twlRemoveA0, ACTOR, seed);
    expect_eq("383C.twl.c", s_trace.twlRemoveA0, (u32)(uintptr_t)a0, seed);

    /* func_800BDCF8(p) */
    D_800D2D68[0] = 0x5555u;
    init_fixture(seed);
    run_oracle3(SLICE_DCF8, ACTOR, 0, 0);
    o = s_trace;
    s32 = D_800D2D68[0];
    D_800D2D68[0] = 0x5555u;
    init_fixture(seed);
    func_800BDCF8(a0);
    expect_common(&s_trace, &o, seed);
    expect_eq("DCF8.slot", D_800D2D68[0], s32, seed);
    expect_eq("DCF8.wl", o.wlRemoveA0, ACTOR + 0x1Cu, seed);
    expect_eq("DCF8.wl.c", s_trace.wlRemoveA0, (u32)(uintptr_t)(a0 + 0x1C), seed);
    expect_eq("DCF8.twl", o.twlRemoveA0, ACTOR, seed);
    expect_eq("DCF8.twl.c", s_trace.twlRemoveA0, (u32)(uintptr_t)a0, seed);

    /* func_800C0F70() */
    D_800C3A6C[0] = 0x44440000u + seed;
    init_fixture(seed);
    run_oracle3(SLICE_C0F70, 0, 0, 0);
    o = s_trace;
    s32 = D_800C3A6C[0];
    s32b = o.soundFreeA0;
    D_800C3A6C[0] = 0x44440000u + seed;
    init_fixture(seed);
    func_800C0F70();
    expect_common(&s_trace, &o, seed);
    expect_eq("C0F70.slot", D_800C3A6C[0], s32, seed);
    expect_eq("C0F70.slot.exp", s32, 0u, seed);
    expect_eq("C0F70.free", s32b, 0x44440000u + seed, seed);
    expect_eq("C0F70.free.c", s_trace.soundFreeA0, 0x44440000u + seed, seed);

    /* func_800B397C(a0, a1, a2, a3, a4) */
    D_800C355C[0] = 0x22u;
    init_fixture(seed);
    run_oracle5(SLICE_397C, ACTOR, ARG, 0x1234u + seed, 0x5678u + seed,
                (u32)(0x90u + seed));
    o = s_trace;
    s8bytes[0] = D_800C355C[0];
    D_800C355C[0] = 0x22u;
    init_fixture(seed);
    func_800B397C(a0, arg(), (0x1234u + seed) & 0xFFu, (0x5678u + seed) & 0xFFu,
                  (u8)(0x90u + seed));
    expect_common(&s_trace, &o, seed);
    expect_eq("397C.flag", D_800C355C[0], s8bytes[0], seed);
    expect_eq("397C.flag.exp", s8bytes[0], 0u, seed);
    expect_eq("397C.a2", o.b39c0A2, (0x1234u + seed) & 0xFFu, seed);
    expect_eq("397C.a3", o.b39c0A3, (0x5678u + seed) & 0xFFu, seed);
    expect_eq("397C.a4", o.b39c0A4, 0x90u + seed, seed);

    /* func_800BEDE8() */
    D_800C3610[0] = 0x0F0F0000u + seed;
    D_80059464[0] = 0x2121u;
    D_800591AC[0] = 0x33u;
    init_fixture(seed);
    run_oracle3(SLICE_EDE8, 0, 0, 0);
    o = s_trace;
    s32 = D_800C3610[0];
    s32b = D_80059464[0];
    s8bytes[1] = D_800591AC[0];
    D_800C3610[0] = 0x0F0F0000u + seed;
    D_80059464[0] = 0x2121u;
    D_800591AC[0] = 0x33u;
    init_fixture(seed);
    func_800BEDE8();
    expect_common(&s_trace, &o, seed);
    expect_eq("EDE8.freed", o.heapFreeA0, 0x0F0F0000u + seed, seed);
    expect_eq("EDE8.freed.c", s_trace.heapFreeA0, 0x0F0F0000u + seed, seed);
    expect_eq("EDE8.ptr", D_800C3610[0], s32, seed);
    expect_eq("EDE8.counter", D_80059464[0], s32b, seed);
    expect_eq("EDE8.flag", D_800591AC[0], s8bytes[1], seed);
}

static void check_batch52(unsigned seed)
{
    Trace o;
    u8* a0 = actor();
    u32 s32, s32b, oracle_ret, host_ret;
    u16 s16;
    u8 s8v;

    /* func_800BF3A4() */
    D_800D3350[0] = (u8)(seed & 1);
    D_800C3618[0] = 0x13570000u + seed;
    init_fixture(seed);
    run_oracle3(SLICE_F3A4, 0, 0, 0);
    o = s_trace;
    s32 = D_800C3618[0];
    s8v = D_800D3350[0];
    D_800D3350[0] = (u8)(seed & 1);
    D_800C3618[0] = 0x13570000u + seed;
    init_fixture(seed);
    func_800BF3A4();
    expect_common(&s_trace, &o, seed);
    expect_eq("F3A4.flag", D_800D3350[0], s8v, seed);
    expect_eq("F3A4.arg", s32, 0x13570000u + seed, seed);

    /* func_800B3C2C(p) */
    D_800C3560[0] = 0x2468u;
    init_fixture(seed);
    run_oracle3(SLICE_3C2C, ACTOR, 0, 0);
    o = s_trace;
    s32 = D_800C3560[0];
    D_800C3560[0] = 0x2468u;
    init_fixture(seed);
    func_800B3C2C(a0);
    expect_common(&s_trace, &o, seed);
    expect_eq("3C2C.slot", D_800C3560[0], s32, seed);
    expect_eq("3C2C.slot.exp", s32, 0u, seed);
    expect_eq("3C2C.heap", o.heapFreeA0, ACTOR, seed);
    expect_eq("3C2C.heap.c", s_trace.heapFreeA0, ACTOR, seed);

    /* func_800BB7F8() */
    init_fixture(seed);
    run_oracle3(SLICE_BB7F8, 0, 0, 0);
    o = s_trace;
    s32 = D_800C3674[0];
    s32b = D_800C3CBC[0];
    s8v = D_800C3CC4[0];
    s16 = (u16)D_800C3678[0];
    D_800C3674[0] = 0;
    D_800C3678[0] = 0;
    D_800C3CC4[0] = 0xFFu;
    D_800C3CBC[0] = 0;
    init_fixture(seed);
    func_800BB7F8();
    expect_common(&s_trace, &o, seed);
    expect_eq("BB7F8.674", D_800C3674[0], s32, seed);
    expect_eq("BB7F8.678", (u16)D_800C3678[0], s16, seed);
    expect_eq("BB7F8.cc4", D_800C3CC4[0], s8v, seed);
    expect_eq("BB7F8.cbc", D_800C3CBC[0], s32b, seed);
    expect_eq("BB7F8.674.exp", s32, 0x200u, seed);

    /* func_800BB690(p) */
    *(u32*)(a0 + 0x1C) = 0x37370000u + seed;
    D_800C3CB8[0] = (u8)(seed + 3);
    init_fixture(seed);
    *(u32*)(a0 + 0x1C) = 0x37370000u + seed;
    D_800C3CB8[0] = (u8)(seed + 3);
    run_oracle3(SLICE_BB690, ACTOR, 0, 0);
    o = s_trace;
    s8v = D_800C3CB8[0];
    *(u32*)(a0 + 0x1C) = 0x37370000u + seed;
    D_800C3CB8[0] = (u8)(seed + 3);
    init_fixture(seed);
    *(u32*)(a0 + 0x1C) = 0x37370000u + seed;
    D_800C3CB8[0] = (u8)(seed + 3);
    func_800BB690(a0);
    expect_common(&s_trace, &o, seed);
    expect_eq("BB690.count", D_800C3CB8[0], s8v, seed);
    expect_eq("BB690.count.exp", s8v, (u8)(seed + 4), seed);
    expect_eq("BB690.a9540", s_trace.a9540A0, 0x37370000u + seed, seed);
    expect_eq("oracle.BB690.cb", o.cbCallback, 0x800BB620u, seed);
    expect_eq("c.BB690.cb", s_trace.cbCallback, (u32)(uintptr_t)func_800BB620,
              seed);

    /* func_800BC404(p) */
    init_fixture(seed);
    D_800C37C8[0] = (u8)(seed & 1);
    D_800C3CDC[0] = (u16)(0x3000u + seed);
    D_80059454[0] = 0x0F0Fu;
    run_oracle3(SLICE_BC404, ACTOR, 0, 0);
    o = s_trace;
    s16 = D_80059454[0];
    D_800C37C8[0] = (u8)(seed & 1);
    D_800C3CDC[0] = (u16)(0x3000u + seed);
    D_80059454[0] = 0x0F0Fu;
    init_fixture(seed);
    func_800BC404(a0);
    expect_common(&s_trace, &o, seed);
    expect_eq("BC404.copy", D_80059454[0], s16, seed);
    expect_eq("BC404.copy.exp", s16, (u16)(0x3000u + seed), seed);

    /* func_800BF354() */
    D_800D3350[0] = (u8)(seed & 1);
    D_800C3618[0] = 0x51510000u + seed;
    init_fixture(seed);
    D_800D3350[0] = (u8)(seed & 1);
    D_800C3618[0] = 0x51510000u + seed;
    oracle_ret = run_oracle3(SLICE_F354, 0, 0, 0);
    o = s_trace;
    s8v = D_800D3350[0];
    D_800D3350[0] = (u8)(seed & 1);
    D_800C3618[0] = 0x51510000u + seed;
    init_fixture(seed);
    host_ret = func_800BF354();
    expect_common(&s_trace, &o, seed);
    if ((seed & 1) == 0) {
        expect_eq("F354.ret", host_ret, oracle_ret, seed);
    }
    expect_eq("F354.flag", D_800D3350[0], s8v, seed);
    expect_eq("F354.flag.exp", s8v, 1u, seed);
    if ((seed & 1) == 0) {
        expect_eq("F354.arg", s_trace.c0facA0, 0x51510000u + seed, seed);
    }

    /* func_800BF998() */
    init_fixture(seed);
    D_800D2D4C[0] = (u16)(seed & 3);
    D_800D36BC[0] = (u16)(0x7000u + seed);
    run_oracle3(SLICE_F998, 0, 0, 0);
    o = s_trace;
    s16 = D_800D2D4C[0];
    s32 = D_800D36BC[0];
    D_800D2D4C[0] = (u16)(seed & 3);
    D_800D36BC[0] = (u16)(0x7000u + seed);
    init_fixture(seed);
    func_800BF998();
    expect_common(&s_trace, &o, seed);
    expect_eq("F998.a", D_800D2D4C[0], s16, seed);
    expect_eq("F998.b", D_800D36BC[0], s32, seed);
    expect_eq("F998.a.exp", s16, (u16)((seed & 3) + 1), seed);

    /* func_800BCB54(): task at 0x130000, its +0x1C points at the 0x140000 node */
    s8v = (u8)(seed + 0x11);
    s_ram[0x14002B] = s8v;
    *(u32*)(s_ram + 0x13001C) = 0x80140000u;
    D_800C3748[0] = 0x80130000u;
    init_fixture(seed);
    s_ram[0x14002B] = s8v;
    *(u32*)(s_ram + 0x13001C) = 0x80140000u;
    D_800C3748[0] = 0x80130000u;
    run_oracle3(SLICE_CB54, 0, 0, 0);
    o = s_trace;
    s32 = D_800C3748[0];
    s32b = s_ram[0x14002B];
    s_ram[0x14002B] = s8v;
    *(u32*)(s_ram + 0x13001C) = (u32)(uintptr_t)(s_ram + 0x140000);
    D_800C3748[0] = (u32)(uintptr_t)(s_ram + 0x130000);
    init_fixture(seed);
    s_ram[0x14002B] = s8v;
    *(u32*)(s_ram + 0x13001C) = (u32)(uintptr_t)(s_ram + 0x140000);
    D_800C3748[0] = (u32)(uintptr_t)(s_ram + 0x130000);
    func_800BCB54();
    expect_common(&s_trace, &o, seed);
    expect_eq("CB54.ptr", s32, 0u, seed);
    expect_eq("CB54.ptr.c", D_800C3748[0], 0u, seed);
    expect_eq("CB54.flag", s_ram[0x14002B], s32b, seed);
    expect_eq("CB54.flag.exp", s32b, (u8)(s8v | 1u), seed);

    /* func_800BEBC4() */
    init_fixture(seed);
    *(u16*)(D_800C3EB0 + 0x8000 + 0xC58) = (u16)((seed & 1) ? 0x100u : 0x000u);
    D_80059494[0] = 0x1234u;
    run_oracle3(SLICE_EBC4, 0, 0, 0);
    o = s_trace;
    s16 = D_80059494[0];
    *(u16*)(D_800C3EB0 + 0x8000 + 0xC58) = (u16)((seed & 1) ? 0x100u : 0x000u);
    D_80059494[0] = 0x1234u;
    init_fixture(seed);
    func_800BEBC4();
    expect_common(&s_trace, &o, seed);
    expect_eq("EBC4.fb", D_80059494[0], s16, seed);
    expect_eq("EBC4.calls", s_trace.bec18Calls, 1, seed);
}

static void check_batch53(unsigned seed)
{
    Trace o;
    u32 s32;
    u8 oracle_bytes[0x20];
    unsigned k;

    /* func_800B8D04(): the loop exits when func_800BF720() first matches
     * D_80059464[0]; func_800BE790() bumps that counter once per iteration. */
    init_fixture(seed);
    D_80059464[0] = (u32)(seed & 1);
    D_800C3618[0] = (seed & 1) ? 0x1A1A0000u + seed : 0u;
    D_800D3350[0] = 0;
    D_800D2D68[0] = 1;
    run_oracle3(SLICE_8D04, 0, 0, 0);
    o = s_trace;
    s32 = D_80059464[0];
    D_80059464[0] = (u32)(seed & 1);
    D_800C3618[0] = (seed & 1) ? 0x1A1A0000u + seed : 0u;
    D_800D3350[0] = 0;
    D_800D2D68[0] = 1;
    init_fixture(seed);
    func_800B8D04();
    expect_common(&s_trace, &o, seed);
    expect_eq("8D04.counter", D_80059464[0], s32, seed);
    expect_eq("8D04.counter.exp", s32, 1u, seed);
    if ((seed & 1) != 0) {
        expect_eq("8D04.freed", s_trace.heapFreeA0, 0x1A1A0000u + seed, seed);
        expect_eq("8D04.ptr", D_800C3618[0], 0u, seed);
    }

    /* func_800B8840(): global writes + the call sequence */
    init_fixture(seed);
    D_800591AD[0] = 0;
    D_80059464[0] = 0x1111u;
    D_800591AC[0] = 0x22u;
    D_800591A8[0] = 0x3333u;
    D_80050104[0] = 0x4444u;
    run_oracle3(SLICE_8840, 0, 0, 0);
    o = s_trace;
    memcpy(oracle_bytes, D_800591AD, 1);
    s32 = D_80059464[0];
    D_800591AD[0] = 0;
    D_80059464[0] = 0x1111u;
    D_800591AC[0] = 0x22u;
    D_800591A8[0] = 0x3333u;
    D_80050104[0] = 0x4444u;
    init_fixture(seed);
    func_800B8840();
    expect_common(&s_trace, &o, seed);
    expect_eq("8840.ad", D_800591AD[0], oracle_bytes[0], seed);
    expect_eq("8840.ad.exp", D_800591AD[0], 1u, seed);
    expect_eq("8840.counter", D_80059464[0], s32, seed);
    expect_eq("8840.counter.exp", D_80059464[0], 0u, seed);
    expect_eq("8840.ac", D_800591AC[0], 0u, seed);
    expect_eq("8840.a8", D_800591A8[0], 0x2000u, seed);
    expect_eq("8840.gfxAlloc", s_trace.gfxAllocA0, 0x5000u, seed);
    expect_eq("8840.50104", D_80050104[0], 0u, seed);
    expect_eq("8840.wlReset", s_trace.wlResetCalls, 1, seed);
    (void)k;
}

static void check_batch54(unsigned seed)
{
    Trace o;
    u8* a0 = actor();
    u8* s0 = s_ram + 0x160000;
    u8 s8v;
    u32 s32;

    /* func_800BF9EC() */
    D_800C3621[0] = (u8)(seed & 1);
    init_fixture(seed);
    D_800C3621[0] = (u8)(seed & 1);
    run_oracle3(SLICE_F9EC, 0, 0, 0);
    o = s_trace;
    s8v = D_800C3621[0];
    D_800C3621[0] = (u8)(seed & 1);
    init_fixture(seed);
    func_800BF9EC();
    expect_common(&s_trace, &o, seed);
    expect_eq("F9EC.flag", D_800C3621[0], s8v, seed);
    if ((seed & 1) != 0) {
        expect_eq("F9EC.setIdx", s_trace.setIdxCalls, 1, seed);
        expect_eq("F9EC.dir", s_trace.setIdxDir, 0x2Cu, seed);
        expect_eq("F9EC.decSize", s_trace.decSizeCalls, 1, seed);
        expect_eq("F9EC.heap", s_trace.heapAllocCalls, 1, seed);
        expect_eq("F9EC.read", s_trace.readBufCalls, 1, seed);
        expect_eq("F9EC.dde4", s_trace.dde4Calls, 1, seed);
        expect_eq("F9EC.free", s_trace.heapFreeCalls, 1, seed);
        expect_eq("F9EC.flag.exp", s8v, 0u, seed);
    }

    /* func_800BF600(a0, a1): a1 is the task object, +0x48 gates the early
     * exit and +0xAF selects the wait path. */
    memset(s0, 0, 0x200);
    *(u32*)(s0 + 0x48) = (u32)((seed & 2) ? 1u : 0u);
    *(s8*)(s0 + 0xAF) = (s8)((seed & 4) ? 3 : 0);
    D_800C3CE8[0] = 0x1111u;
    init_fixture(seed);
    memset(s0, 0, 0x200);
    *(u32*)(s0 + 0x48) = (u32)((seed & 2) ? 1u : 0u);
    *(s8*)(s0 + 0xAF) = (s8)((seed & 4) ? 3 : 0);
    D_800C3CE8[0] = 0x1111u;
    run_oracle3(SLICE_F600, ACTOR, 0x80160000u, 0);
    o = s_trace;
    s32 = D_800C3CE8[0];
    memset(s0, 0, 0x200);
    *(u32*)(s0 + 0x48) = (u32)((seed & 2) ? 1u : 0u);
    *(s8*)(s0 + 0xAF) = (s8)((seed & 4) ? 3 : 0);
    D_800C3CE8[0] = 0x1111u;
    init_fixture(seed);
    func_800BF600(a0, s0);
    expect_common(&s_trace, &o, seed);
    expect_eq("F600.counter", D_800C3CE8[0], s32, seed);
    if ((seed & 6) == 0) {
        expect_eq("F600.counter.kept", s32, 0x1111u, seed);
        expect_eq("F600.b7c34", s_trace.b7c34Calls, 1, seed);
    } else {
        expect_eq("F600.b7c34", s_trace.b7c34Calls, 1, seed);
    }
    if ((seed & 4) != 0 && (seed & 2) != 0) {
        expect_eq("F600.b21bf8", s_trace.b21bf8Calls, 2, seed);
        expect_eq("F600.be790", s_trace.be790Calls, 1, seed);
    }
}

static void check_beb04(unsigned seed)
{
    Trace o;
    u8 s8v;
    int ran;

    /* func_800BEB04(): the guard compares D_800591B2 with D_800591B3 */
    ran = ((seed & 1) != (1 + (seed % 3)));
    D_800591B2[0] = (u8)(seed & 1);
    D_800591B3[0] = (u8)(1 + (seed % 3));
    D_800591B0[0] = 0;
    init_fixture(seed);
    D_800591B2[0] = (u8)(seed & 1);
    D_800591B3[0] = (u8)(1 + (seed % 3));
    D_800591B0[0] = 0;
    run_oracle3(SLICE_BEB04, 0, 0, 0);
    o = s_trace;
    s8v = D_800591B0[0];
    D_800591B2[0] = (u8)(seed & 1);
    D_800591B3[0] = (u8)(1 + (seed % 3));
    D_800591B0[0] = 0;
    init_fixture(seed);
    func_800BEB04();
    expect_common(&s_trace, &o, seed);
    expect_eq("BEB04.flag", D_800591B0[0], s8v, seed);
    expect_eq("BEB04.flag.exp", D_800591B0[0], 1u, seed);
    expect_eq("BEB04.b2", D_800591B2[0], D_800591B3[0], seed);
    if (ran) {
        expect_eq("BEB04.setIdx", s_trace.setIdxCalls, 2, seed);
        expect_eq("BEB04.read", s_trace.readBufCalls, 1, seed);
        expect_eq("BEB04.getInd", s_trace.getIndCalls, 1, seed);
        expect_eq("BEB04.dsync", s_trace.dsyncCalls, 1, seed);
        expect_eq("BEB04.vsync", s_trace.vsyncCalls, 1, seed);
        expect_eq("BEB04.cs", s_trace.enterCsCalls, 1, seed);
        expect_eq("BEB04.flush", s_trace.flushCacheCalls, 1, seed);
        expect_eq("BEB04.exitcs", s_trace.exitCsCalls, 1, seed);
    }
}

static void check_batch56(unsigned seed)
{
    Trace o;
    u8 s8v;
    u32 s32;

    /* func_800BFBA0() */
    D_800C3620[0] = (u8)(seed & 1);
    D_800C3622[0] = 0x77u;
    D_800C3A6C[0] = 0;
    s_ram[0x150020] = (u8)(0x10u + seed);
    s_ram[0x150021] = (u8)(seed & 3);
    init_fixture(seed);
    D_800C3620[0] = (u8)(seed & 1);
    D_800C3622[0] = 0x77u;
    D_800C3A6C[0] = 0;
    s_ram[0x150020] = (u8)(0x10u + seed);
    s_ram[0x150021] = (u8)(seed & 3);
    run_oracle3(SLICE_BFBA0, 0, 0, 0);
    o = s_trace;
    s8v = D_800C3620[0];
    s32 = D_800C3A6C[0];
    init_fixture(seed);
    D_800C3620[0] = (u8)(seed & 1);
    D_800C3622[0] = 0x77u;
    D_800C3A6C[0] = 0;
    s_ram[0x150020] = (u8)(0x10u + seed);
    s_ram[0x150021] = (u8)(seed & 3);
    func_800BFBA0();
    expect_common(&s_trace, &o, seed);
    expect_eq("BFBA0.flag", D_800C3620[0], s8v, seed);
    expect_eq("BFBA0.a6c", D_800C3A6C[0], s32, seed);
    if ((seed & 1) != 0) {
        expect_eq("BFBA0.flag.kept", s8v, 1u, seed);
        expect_eq("BFBA0.sfind", s_trace.sfindCalls, 0, seed);
    } else {
        expect_eq("BFBA0.sfind", s_trace.sfindCalls, 1, seed);
        expect_eq("BFBA0.sload", s_trace.sloadCalls, 1, seed);
        expect_eq("BFBA0.bdfc", s_trace.bdfcCalls, 1, seed);
        expect_eq("BFBA0.free", s_trace.heapFreeCalls, 1, seed);
        expect_eq("BFBA0.flag.exp", s8v, 1u, seed);
    }

    /* func_800B8774() */
    *(u32*)(D_800C3EB0 + 0x8000 + 0xC84) = (u32)((seed & 1) ? 0u : 1u);
    D_8005919C[0] = 0x5B5B0000u + seed;
    D_800591AD[0] = 0x33u;
    D_800D2D54[0] = 0x6C6C0000u + seed;
    init_fixture(seed);
    run_oracle3(SLICE_B8774, 0, 0, 0);
    o = s_trace;
    s8v = D_800591AD[0];
    s32 = D_800D2D54[0];
    D_8005919C[0] = 0x5B5B0000u + seed;
    D_800591AD[0] = 0x33u;
    D_800D2D54[0] = 0x6C6C0000u + seed;
    init_fixture(seed);
    func_800B8774();
    expect_common(&s_trace, &o, seed);
    expect_eq("B8774.ad", D_800591AD[0], s8v, seed);
    expect_eq("B8774.ad.exp", s8v, 0u, seed);
    expect_eq("B8774.badd4", s_trace.badd4Calls, 0xB, seed);
    expect_eq("B8774.gffFree", s_trace.gffFreeCalls, 1, seed);
    expect_eq("B8774.wlFreeAll", s_trace.wlFreeAllCalls, 1, seed);
    expect_eq("B8774.b3852c", s_trace.b3852cCalls, 1, seed);
    expect_eq("B8774.a9f94", s_trace.a9f94Calls, 1, seed);
    expect_eq("B8774.a4820", s_trace.a4820Calls, 1, seed);
    expect_eq("B8774.d54", D_800D2D54[0], s32, seed);
}

static void check_batch58(unsigned seed)
{
    Trace o;
    u8* buf = s_ram + 0x190000;
    u32 s32;

    /* func_800B9258(): bumps the +0x34 counter of the D_800C3610 object */
    memset(buf, 0, 0x100);
    *(u32*)(buf + 0x34) = 0x1000u + seed;
    D_800C3610[0] = 0x80190000u;
    init_fixture(seed);
    memset(buf, 0, 0x100);
    *(u32*)(buf + 0x34) = 0x1000u + seed;
    D_800C3610[0] = 0x80190000u;
    run_oracle3(SLICE_9258, 0, 0, 0);
    o = s_trace;
    s32 = *(u32*)(buf + 0x34);
    memset(buf, 0, 0x100);
    *(u32*)(buf + 0x34) = 0x1000u + seed;
    D_800C3610[0] = (u32)(uintptr_t)buf;
    init_fixture(seed);
    memset(buf, 0, 0x100);
    *(u32*)(buf + 0x34) = 0x1000u + seed;
    D_800C3610[0] = (u32)(uintptr_t)buf;
    func_800B9258();
    expect_common(&s_trace, &o, seed);
    expect_eq("9258.count", *(u32*)(buf + 0x34), s32, seed);
    expect_eq("9258.count.exp", s32, 0x1001u + seed, seed);

    /* func_800BE0DC(): frees the D_800D2D68 object when present */
    D_800D2D68[0] = (seed & 1) ? 0x801A0000u : 0u;
    init_fixture(seed);
    D_800D2D68[0] = (seed & 1) ? 0x801A0000u : 0u;
    run_oracle3(SLICE_E0DC, 0, 0, 0);
    o = s_trace;
    D_800D2D68[0] = (seed & 1) ? 0x801A0000u : 0u;
    init_fixture(seed);
    func_800BE0DC();
    expect_common(&s_trace, &o, seed);
    expect_eq("E0DC.slot", D_800D2D68[0], 0u, seed);
    if ((seed & 1) != 0) {
        expect_eq("E0DC.twl", s_trace.twlRemoveCalls, 1, seed);
        expect_eq("E0DC.wl", s_trace.wlRemoveCalls, 1, seed);
    }
}

int main(void)
{
    FILE* f;
    unsigned seed;
    unsigned saw_callback = 0;

    f = fopen("disc/battle.bin", "rb");
    if (f == NULL) {
        fprintf(stderr, "BATTLE CDK FAIL cannot open disc/battle.bin\n");
        return 1;
    }
    /* battle vram 0x8006FAF0 == file 0x0 */
    if (fseek(f, SLICE_5924 - 0x8006FAF0, SEEK_SET) != 0 ||
        fread(s_ram + 0xB5924, 1, SLICE_5924_LEN, f) != SLICE_5924_LEN ||
        fseek(f, SLICE_5C18 - 0x8006FAF0, SEEK_SET) != 0 ||
        fread(s_ram + 0xB5C18, 1, SLICE_5C18_LEN, f) != SLICE_5C18_LEN ||
        fseek(f, SLICE_F7C8 - 0x8006FAF0, SEEK_SET) != 0 ||
        fread(s_ram + 0xBF7C8, 1, SLICE_F7C8_LEN, f) != SLICE_F7C8_LEN ||
        fseek(f, SLICE_5DC4 - 0x8006FAF0, SEEK_SET) != 0 ||
        fread(s_ram + 0xB5DC4, 1, SLICE_5DC4_LEN, f) != SLICE_5DC4_LEN ||
        fseek(f, SLICE_56E4 - 0x8006FAF0, SEEK_SET) != 0 ||
        fread(s_ram + 0xB56E4, 1, SLICE_56E4_LEN, f) != SLICE_56E4_LEN ||
        fseek(f, SLICE_DC78 - 0x8006FAF0, SEEK_SET) != 0 ||
        fread(s_ram + 0xBDC78, 1, SLICE_DC78_LEN, f) != SLICE_DC78_LEN ||
        fseek(f, SLICE_64D4 - 0x8006FAF0, SEEK_SET) != 0 ||
        fread(s_ram + 0xB64D4, 1, SLICE_64D4_LEN, f) != SLICE_64D4_LEN ||
        fseek(f, SLICE_73A0 - 0x8006FAF0, SEEK_SET) != 0 ||
        fread(s_ram + 0xB73A0, 1, SLICE_73A0_LEN, f) != SLICE_73A0_LEN ||
        fseek(f, SLICE_B16A4 - 0x8006FAF0, SEEK_SET) != 0 ||
        fread(s_ram + 0xB16A4, 1, SLICE_B16A4_LEN, f) != SLICE_B16A4_LEN ||
        fseek(f, SLICE_B6438 - 0x8006FAF0, SEEK_SET) != 0 ||
        fread(s_ram + 0xB6438, 1, SLICE_B6438_LEN, f) != SLICE_B6438_LEN ||
        fseek(f, SLICE_7ADF4 - 0x8006FAF0, SEEK_SET) != 0 ||
        fread(s_ram + 0x7ADF4, 1, SLICE_7ADF4_LEN, f) != SLICE_7ADF4_LEN ||
        fseek(f, SLICE_BF5E8 - 0x8006FAF0, SEEK_SET) != 0 ||
        fread(s_ram + 0xBF5E8, 1, SLICE_BF5E8_LEN, f) != SLICE_BF5E8_LEN ||
        fseek(f, SLICE_B35C0 - 0x8006FAF0, SEEK_SET) != 0 ||
        fread(s_ram + 0xB35C0, 1, SLICE_B35C0_LEN, f) != SLICE_B35C0_LEN ||
        fseek(f, SLICE_7C28 - 0x8006FAF0, SEEK_SET) != 0 ||
        fread(s_ram + 0xB7C28, 1, SLICE_7C28_LEN, f) != SLICE_7C28_LEN ||
        fseek(f, SLICE_BCD8C - 0x8006FAF0, SEEK_SET) != 0 ||
        fread(s_ram + 0xBCD8C, 1, SLICE_BCD8C_LEN, f) != SLICE_BCD8C_LEN ||
        fseek(f, SLICE_BF730 - 0x8006FAF0, SEEK_SET) != 0 ||
        fread(s_ram + 0xBF730, 1, SLICE_BF730_LEN, f) != SLICE_BF730_LEN ||
        fseek(f, SLICE_BC454 - 0x8006FAF0, SEEK_SET) != 0 ||
        fread(s_ram + 0xBC454, 1, SLICE_BC454_LEN, f) != SLICE_BC454_LEN ||
        fseek(f, SLICE_BC3F8 - 0x8006FAF0, SEEK_SET) != 0 ||
        fread(s_ram + 0xBC3F8, 1, SLICE_BC3F8_LEN, f) != SLICE_BC3F8_LEN ||
        fseek(f, SLICE_B8054 - 0x8006FAF0, SEEK_SET) != 0 ||
        fread(s_ram + 0xB8054, 1, SLICE_B8054_LEN, f) != SLICE_B8054_LEN ||
        fseek(f, SLICE_BE108 - 0x8006FAF0, SEEK_SET) != 0 ||
        fread(s_ram + 0xBE108, 1, SLICE_BE108_LEN, f) != SLICE_BE108_LEN ||
        fseek(f, SLICE_BED30 - 0x8006FAF0, SEEK_SET) != 0 ||
        fread(s_ram + 0xBED30, 1, SLICE_BED30_LEN, f) != SLICE_BED30_LEN ||
        fseek(f, SLICE_B9B30 - 0x8006FAF0, SEEK_SET) != 0 ||
        fread(s_ram + 0xB9B30, 1, SLICE_B9B30_LEN, f) != SLICE_B9B30_LEN ||
        fseek(f, SLICE_8048 - 0x8006FAF0, SEEK_SET) != 0 ||
        fread(s_ram + 0xB8048, 1, SLICE_8048_LEN, f) != SLICE_8048_LEN ||
        fseek(f, SLICE_7134 - 0x8006FAF0, SEEK_SET) != 0 ||
        fread(s_ram + 0xB7134, 1, SLICE_7134_LEN, f) != SLICE_7134_LEN ||
        fseek(f, SLICE_16F0 - 0x8006FAF0, SEEK_SET) != 0 ||
        fread(s_ram + 0xB16F0, 1, SLICE_16F0_LEN, f) != SLICE_16F0_LEN ||
        fseek(f, SLICE_8068 - 0x8006FAF0, SEEK_SET) != 0 ||
        fread(s_ram + 0xB8068, 1, SLICE_8068_LEN, f) != SLICE_8068_LEN ||
        fseek(f, SLICE_3588 - 0x8006FAF0, SEEK_SET) != 0 ||
        fread(s_ram + 0xB3588, 1, SLICE_3588_LEN, f) != SLICE_3588_LEN ||
        fseek(f, SLICE_383C - 0x8006FAF0, SEEK_SET) != 0 ||
        fread(s_ram + 0xB383C, 1, SLICE_383C_LEN, f) != SLICE_383C_LEN ||
        fseek(f, SLICE_DCF8 - 0x8006FAF0, SEEK_SET) != 0 ||
        fread(s_ram + 0xBDCF8, 1, SLICE_DCF8_LEN, f) != SLICE_DCF8_LEN ||
        fseek(f, SLICE_C0F70 - 0x8006FAF0, SEEK_SET) != 0 ||
        fread(s_ram + 0xC0F70, 1, SLICE_C0F70_LEN, f) != SLICE_C0F70_LEN ||
        fseek(f, SLICE_397C - 0x8006FAF0, SEEK_SET) != 0 ||
        fread(s_ram + 0xB397C, 1, SLICE_397C_LEN, f) != SLICE_397C_LEN ||
        fseek(f, SLICE_EDE8 - 0x8006FAF0, SEEK_SET) != 0 ||
        fread(s_ram + 0xBEDE8, 1, SLICE_EDE8_LEN, f) != SLICE_EDE8_LEN ||
        fseek(f, SLICE_F3A4 - 0x8006FAF0, SEEK_SET) != 0 ||
        fread(s_ram + 0xBF3A4, 1, SLICE_F3A4_LEN, f) != SLICE_F3A4_LEN ||
        fseek(f, SLICE_3C2C - 0x8006FAF0, SEEK_SET) != 0 ||
        fread(s_ram + 0xB3C2C, 1, SLICE_3C2C_LEN, f) != SLICE_3C2C_LEN ||
        fseek(f, SLICE_BB7F8 - 0x8006FAF0, SEEK_SET) != 0 ||
        fread(s_ram + 0xBB7F8, 1, SLICE_BB7F8_LEN, f) != SLICE_BB7F8_LEN ||
        fseek(f, SLICE_BB690 - 0x8006FAF0, SEEK_SET) != 0 ||
        fread(s_ram + 0xBB690, 1, SLICE_BB690_LEN, f) != SLICE_BB690_LEN ||
        fseek(f, SLICE_BC404 - 0x8006FAF0, SEEK_SET) != 0 ||
        fread(s_ram + 0xBC404, 1, SLICE_BC404_LEN, f) != SLICE_BC404_LEN ||
        fseek(f, SLICE_F354 - 0x8006FAF0, SEEK_SET) != 0 ||
        fread(s_ram + 0xBF354, 1, SLICE_F354_LEN, f) != SLICE_F354_LEN ||
        fseek(f, SLICE_F998 - 0x8006FAF0, SEEK_SET) != 0 ||
        fread(s_ram + 0xBF998, 1, SLICE_F998_LEN, f) != SLICE_F998_LEN ||
        fseek(f, SLICE_CB54 - 0x8006FAF0, SEEK_SET) != 0 ||
        fread(s_ram + 0xBCB54, 1, SLICE_CB54_LEN, f) != SLICE_CB54_LEN ||
        fseek(f, SLICE_EBC4 - 0x8006FAF0, SEEK_SET) != 0 ||
        fread(s_ram + 0xBEBC4, 1, SLICE_EBC4_LEN, f) != SLICE_EBC4_LEN ||
        fseek(f, SLICE_8D04 - 0x8006FAF0, SEEK_SET) != 0 ||
        fread(s_ram + 0xB8D04, 1, SLICE_8D04_LEN, f) != SLICE_8D04_LEN ||
        fseek(f, SLICE_8840 - 0x8006FAF0, SEEK_SET) != 0 ||
        fread(s_ram + 0xB8840, 1, SLICE_8840_LEN, f) != SLICE_8840_LEN ||
        fseek(f, SLICE_F9EC - 0x8006FAF0, SEEK_SET) != 0 ||
        fread(s_ram + 0xBF9EC, 1, SLICE_F9EC_LEN, f) != SLICE_F9EC_LEN ||
        fseek(f, SLICE_F600 - 0x8006FAF0, SEEK_SET) != 0 ||
        fread(s_ram + 0xBF600, 1, SLICE_F600_LEN, f) != SLICE_F600_LEN ||
        fseek(f, SLICE_BEB04 - 0x8006FAF0, SEEK_SET) != 0 ||
        fread(s_ram + 0xBEB04, 1, SLICE_BEB04_LEN, f) != SLICE_BEB04_LEN ||
        fseek(f, SLICE_BFBA0 - 0x8006FAF0, SEEK_SET) != 0 ||
        fread(s_ram + 0xBFBA0, 1, SLICE_BFBA0_LEN, f) != SLICE_BFBA0_LEN ||
        fseek(f, SLICE_B8774 - 0x8006FAF0, SEEK_SET) != 0 ||
        fread(s_ram + 0xB8774, 1, SLICE_B8774_LEN, f) != SLICE_B8774_LEN ||
        fseek(f, SLICE_9258 - 0x8006FAF0, SEEK_SET) != 0 ||
        fread(s_ram + 0xB9258, 1, SLICE_9258_LEN, f) != SLICE_9258_LEN ||
        fseek(f, SLICE_E0DC - 0x8006FAF0, SEEK_SET) != 0 ||
        fread(s_ram + 0xBE0DC, 1, SLICE_E0DC_LEN, f) != SLICE_E0DC_LEN) {
        fprintf(stderr, "BATTLE CDK FAIL cannot read the retail slices\n");
        return 1;
    }
    fclose(f);

    for (seed = 0; seed < 8; ++seed) {
        check_spawner("5924", SLICE_5924, (u32)(uintptr_t)func_800B5854,
                      0x800B5854u, seed);
        check_spawner("5C18", SLICE_5C18, (u32)(uintptr_t)func_800B5B3C,
                      0x800B5B3Cu, seed);
        check_spawner("F7C8", SLICE_F7C8, (u32)(uintptr_t)func_800BF73C,
                      0x800BF73Cu, seed);
        check_5dc4(seed);
        check_56e4(seed);
        check_dc78(seed, &saw_callback);
        check_64d4(seed);
        check_73a0(seed);
        check_b16a4(seed);
        check_b6438(seed);
        check_7adf4(seed);
        check_bf5e8(seed);
        check_b35c0(seed);
        check_setters(seed);
        check_b9b30(seed);
        check_batch51(seed);
        check_batch52(seed);
        check_batch53(seed);
        check_batch54(seed);
        check_beb04(seed);
        check_batch56(seed);
        check_batch58(seed);
    }
    if (saw_callback == 0) {
        fprintf(stderr, "BATTLE CDK FAIL dc78 callback branch never taken\n");
        return 1;
    }

    printf("BATTLE CDK SPAWNERS PASS checks=%u (retail slices on the MIPS "
           "adapter vs the shipped C; controlled work-list stubs; dc78 "
           "callback branch seeds=%u)\n", s_checks, saw_callback);
    return 0;
}
