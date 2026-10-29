#include "common.h"


extern u8* TimerWorkListAllocateTask(u32 owner, u32 size);
extern void TimerWorkListSetTaskCallback(void* pTask, void* callback);
extern u32 func_800B57E4(u8* s);
extern void func_800B5B3C(u8* task);
extern void func_800B5854(void);
extern void func_800BF73C(void);
extern void func_800B5CC0(void);
extern void func_800BDC14(void);
extern void func_800BDF1C(void);
extern void func_8001E148(u32 v);
extern void func_800C08CC(u32 a0, void* a1, void* a2);
extern void func_800B51B0(void);
extern void func_800245D8(u32 a0, u32 a1);
#ifndef XENO_PC_PORT
extern u8 D_800C3EB0[];
#endif
extern u32 WorkListsAddTasks(u32 a0, u32 a1, void* a2, void* a3, void* a4);
extern void func_800B7424(u32 p);
extern void func_800B7364(void);
extern void func_800B6F0C(void);
extern void func_800B7134(void);
extern void WorkListSetTaskCallback(void* pTask, void* callback);
extern void D_80025A88(void);
#ifndef XENO_PC_PORT
extern u8 D_800D3420[];
#endif
#ifndef XENO_PC_PORT
extern u32 D_800C3CE8[];
#endif
extern void WorkListTaskSetOnFreeCallback(void* pTask, void* callback);
extern void func_800B3358(u8* p);
extern void func_800B3588(u8* p);
#ifndef XENO_PC_PORT
extern u32 D_800C3548[];
#endif


#ifndef XENO_PC_PORT
extern s32 D_800C360C[];
extern u32 D_800C3610[];
extern void func_800BEFF4(u32 arg0);
extern void func_800BF3E8(u8* rec);
extern void func_800B9B54(u8* a, u8* b);
extern void func_800BC460(u32 mask);
extern void func_800BF9EC(void);
extern void func_800B9508(u8* rec);
extern s16 D_800D30A0[];
extern s16 D_800D3354[];
extern s16 D_800D335C[];

/* 4-bit battle slot: +0xA8 bits 30-31 low, +0xAC bits 0-1 high, whole-word
 * reads with the low part first, as retail (cf. main135's C11CC_SLOT). */
#define B905C_SLOT(p) ({ u32 lo_ = *(u32*)((p) + 0xA8) >> 30; ((*(u32*)((p) + 0xAC) & 3) << 2) | lo_; })

/* func_800B905C.s, byte-exact: func_800BEFF4 with the +0x15B byte of
 * D_800C3EB0's D_800C360C-th 0x48-byte record, then func_800BF3E8 on the
 * command task's sprite s (D_800C3610 task +4).  When D_800C3EB0[0xA78]
 * is set: set +0xAC bit 6 on s and its target t (+0x74) and call
 * func_800B9B54(s, t); with mask = the two sprites' slot bits, call
 * func_800BC460(mask) before and after placing s.  s+0xA0 = t.x - 0x50 or
 * t.x + 0x50: away from t's slot home (+0xE/+0x10 of the 28-byte slot
 * record) - 0x50 when t.x is below the home x; at home - 0x50 when s.x is
 * below t.x.  s+0xA2 = 0, s+0xA4 = t.z.  Then copy the xyz of D_800D30A0
 * vectors 0 and 1 to D_800D3354 / D_800D335C and call func_800B9B54(s, t)
 * again.  Always ends with func_800BF9EC and func_800B9508(s).  Codegen:
 * positions are 16.16 words read as >> 16 (lh of the high half) and x is
 * an s16; the second vector is read through a pointer (retail rebases
 * it); +0xA4 is stored before +0xA2.  Matches under the CDK compiler
 * only. */
void func_800B905C(void) {
    u8* base = D_800C3EB0;
    u8* s;
    u8* t;
    u16 mask;
    u8* rec;
    s16 x;
    s16* src;

    func_800BEFF4(base[D_800C360C[0] * 0x48 + 0x15B]);
    s = *(u8**)(D_800C3610[0] + 4);
    func_800BF3E8(s);
    if (base[0xA78] != 0) {
        t = *(u8**)(s + 0x74);
        *(u32*)(t + 0xAC) |= 0x40;
        *(u32*)(s + 0xAC) |= 0x40;
        func_800B9B54(s, t);
        t = *(u8**)(s + 0x74);
        mask = (1 << B905C_SLOT(s)) | (1 << B905C_SLOT(t));
        func_800BC460(mask);
        rec = base + B905C_SLOT(t) * 28;
        x = *(s32*)(t + 0) >> 16;
        if (x != *(u16*)(rec + 0xE) || *(s32*)(t + 8) >> 16 != *(u16*)(rec + 0x10)) {
            if (x < *(u16*)(rec + 0xE)) {
                *(s16*)(s + 0xA0) = x - 0x50;
            } else {
                *(s16*)(s + 0xA0) = x + 0x50;
            }
        } else if (*(s32*)(s + 0) >> 16 < x) {
            *(s16*)(s + 0xA0) = x - 0x50;
        } else {
            *(s16*)(s + 0xA0) = x + 0x50;
        }
        *(s16*)(s + 0xA4) = *(s32*)(t + 8) >> 16;
        *(s16*)(s + 0xA2) = 0;
        func_800BC460(mask);
        D_800D3354[0] = D_800D30A0[0];
        D_800D3354[1] = D_800D30A0[1];
        D_800D3354[2] = D_800D30A0[2];
        src = &D_800D30A0[4];
        D_800D335C[0] = src[0];
        D_800D335C[1] = src[1];
        D_800D335C[2] = src[2];
        func_800B9B54(s, t);
    }
    func_800BF9EC();
    func_800B9508(s);
}
#endif

/* func_800B9258.s */
#ifdef XENO_PC_PORT
#include "battle_host_guest.h"
#endif
void func_800B9258(void) {
#ifdef XENO_PC_PORT
    /* The shared guest-RAM alias of D_800C3610 is pointer-typed (another TU
     * declares it u8*), so `D_800C3610[0]` would read the byte it points at.
     * Retail loads the word itself and bumps +0x34 of what it points to. */
    u32 p = BG_U32(0x800C3610);

    if (p != 0) {
        BG_U32(p + 0x34) += 1;
    }
#else
    u8* p = (u8*)(u32)D_800C3610[0];

    if (p != 0) {
        *(u32*)(p + 0x34) = *(u32*)(p + 0x34) + 1;
    }
#endif
}
