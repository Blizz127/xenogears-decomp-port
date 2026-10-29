/* Host port bodies of the command target binding (func_800BF3E8) and the
 * approach restart (func_800BF4F0).  The byte-exact C is retail-only in
 * mainc127.c. */
#include "common.h"
#ifdef XENO_PC_PORT
#include "battle_host_guest.h"

extern u32 func_800BEEB4(u32 mask, u32* out, u32 value);
extern u32 func_800BEF24(u8* arg0, u8* arg1);
extern void func_800223B0(void* pSpriteData, s16 angle);
extern void func_800B9C00(u8* rec, u8* other);
extern void func_800BF1EC(u8* obj, s32 anim);
extern void func_800BF0B4(u32 value);
extern void func_800BF0C4(u8* obj);

/* func_800BF3E8.s: publish the current step's target mask (D_800C3EB0 step
 * D_800C360C, +0x14E) in D_800D3634, collect the targets into D_800D363C
 * (func_800BEEB4; none: the actor itself), cross-link actor and first
 * target at +0x74, give the command task (D_800C3610) the target (+0x4C)
 * and its 4-bit class (+0x28), then turn both to face each other
 * (func_800223B0 of func_800BEF24), the target only when its +0xAF state
 * is not 0x15.  Actor and target are sprite records (compact host
 * addresses in the port); every access goes through BG_*. */
void func_800BF3E8(u8* actor_ptr) {
    u32 actor = BG_ADDR(actor_ptr);
    u32 step = 0x800C3EB0u + BG_U32(0x800C360Cu) * 0x48u;
    u32 target;
    u32 task;
    u32 flags;
    u16 found;

    BG_U16(0x800D3634u) = BG_U16(step + 0x14E);
    found = (u16)func_800BEEB4(BG_U16(step + 0x14E),
                               (u32*)BG_PTR(0x800D363Cu), actor);
    BG_U16(0x800D3678u) = found;
    if (found == 0) {
        BG_U32(0x800D363Cu) = actor;
    }
    target = BG_U32(0x800D363Cu);
    task = BG_U32(0x800C3610u);
    BG_U32(actor + 0x74) = target;
    BG_U32(target + 0x74) = actor;
    flags = ((BG_U32(target + 0xAC) & 3) << 2) | (BG_U32(target + 0xA8) >> 30);
    BG_U32(task + 0x4C) = BG_U32(0x800D363Cu);
    BG_U32(task + 0x28) = flags;
    func_800223B0(bg_arg(actor),
                  (s16)func_800BEF24(bg_arg(actor), bg_arg(BG_U32(actor + 0x74))));
    if ((s8)BG_U8(target + 0xAF) != 0x15) {
        func_800223B0(bg_arg(target),
                      (s16)func_800BEF24(bg_arg(BG_U32(actor + 0x74)), bg_arg(actor)));
    }
}

/* func_800BF4F0.s: when the current command's step (the 6-byte D_800C3EB0
 * row of the task's +0x2C index) has no explicit target (both halfwords at
 * +0xA3C/+0xA3E are 0xFFFF), object `a` restarts its approach to `b`: its
 * 16.16 position (+0/+8) is set from its old target (+0xA0/+0xA4, << 16),
 * and the new target is b's x (high half of b +0) 0x50 short on a's side,
 * y 0 and b's z (high half of b +8).  When that target is where `a` already
 * stands (+2/+0xA), func_800B9C00(a, b); otherwise func_800BF1EC(a, 3) and
 * func_800BF0B4(2).  With an explicit target, func_800BF0C4(a).  Both row
 * tests re-read D_800C3610 and its +0x2C as retail does. */
void func_800BF4F0(u8* a_ptr, u8* b_ptr) {
    u32 a = BG_ADDR(a_ptr);
    u32 b = BG_ADDR(b_ptr);
    u32 base = 0x800C3EB0; /* D_800C3EB0 */
    s32 bx;

    if (BG_U16(base + BG_U32(BG_U32(0x800C3610) + 0x2C) * 6 + 0xA3C) == 0xFFFF
        && BG_U16(base + BG_U32(BG_U32(0x800C3610) + 0x2C) * 6 + 0xA3E) == 0xFFFF) {
        BG_S32(a + 0) = (s32)((u32)(s32)BG_S16(a + 0xA0) << 16);
        BG_S32(a + 8) = (s32)((u32)(s32)BG_S16(a + 0xA4) << 16);
        bx = BG_S32(b + 0) >> 16;
        BG_S16(a + 0xA0) = (s16)((BG_S16(a + 2) < bx) ? bx - 0x50 : bx + 0x50);
        BG_S16(a + 0xA4) = (s16)(BG_S32(b + 8) >> 16);
        BG_S16(a + 0xA2) = 0;
        if (BG_S16(a + 0xA0) == BG_S16(a + 2) && BG_S16(a + 0xA4) == BG_S16(a + 0xA)) {
            func_800B9C00(BG_PTR(a), BG_PTR(b));
        } else {
            func_800BF1EC(BG_PTR(a), 3);
            func_800BF0B4(2);
        }
    } else {
        func_800BF0C4(BG_PTR(a));
    }
}
#endif
