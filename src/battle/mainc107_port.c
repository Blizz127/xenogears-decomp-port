/* Host port body of the command-step "approach and face off" action.  The
 * byte-exact C is retail-only in mainc107.c. */
#include "common.h"
#ifdef XENO_PC_PORT
#include "battle_host_guest.h"

extern void func_800BEFF4(u32 slot);
extern void func_800BF3E8(u8* actor);
extern void func_800B9B54(u8* a, u8* b);
extern void func_800BC460(u32 mask);
extern void func_800BF9EC(void);
extern void func_800B9508(u8* rec);

/* 4-bit battle slot of a record: +0xA8 bits 30-31 low, +0xAC bits 0-1 high. */
static u32 bat4_c107_slot(u32 rec) {
    u32 lo = BG_U32(rec + 0xA8) >> 30;

    return ((BG_U32(rec + 0xAC) & 3) << 2) | lo;
}

/* func_800B905C.s: func_800BEFF4 with the +0x15B byte of D_800C3EB0's
 * D_800C360C-th 0x48-byte record, then func_800BF3E8 on the command task's
 * (D_800C3610) sprite s (task +4).  When D_800C3EB0[0xA78] is set: set
 * +0xAC bit 6 on s and its target t (+0x74) and call func_800B9B54(s, t);
 * with mask = the two sprites' slot bits, call func_800BC460(mask) before
 * and after placing s.  s+0xA0 = t.x - 0x50 or t.x + 0x50 (positions are
 * 16.16 words read as their high half): away from t's slot home (+0xE/+0x10
 * of the 28-byte slot row) - 0x50 when t.x is below the home x; at home -
 * 0x50 when s.x is below t.x.  s+0xA4 = t.z, s+0xA2 = 0.  Then copy the xyz
 * of D_800D30A0 vectors 0 and 1 (halfwords 0..2 and 4..6) to D_800D3354 /
 * D_800D335C and call func_800B9B54(s, t) again.  Always ends with
 * func_800BF9EC and func_800B9508(s).  Sprites are guest words; t is
 * re-read from +0x74 after the first func_800B9B54 (retail reloads it). */
void func_800B905C(void) {
    u32 base = 0x800C3EB0; /* D_800C3EB0 */
    u32 s;
    u32 t;
    u32 mask;
    u32 row;
    s16 x;

    func_800BEFF4(BG_U8(base + BG_S32(0x800C360C) * 0x48 + 0x15B)); /* D_800C360C */
    s = BG_U32(BG_U32(0x800C3610) + 4);                              /* D_800C3610 */
    func_800BF3E8(BG_PTR(s));
    if (BG_U8(base + 0xA78) != 0) {
        t = BG_U32(s + 0x74);
        BG_U32(t + 0xAC) |= 0x40;
        BG_U32(s + 0xAC) |= 0x40;
        func_800B9B54(BG_PTR(s), BG_PTR(t));
        t = BG_U32(s + 0x74);
        mask = (u16)((1u << bat4_c107_slot(s)) | (1u << bat4_c107_slot(t)));
        func_800BC460(mask);
        row = base + bat4_c107_slot(t) * 28;
        x = (s16)(BG_S32(t + 0) >> 16);
        if (x != BG_U16(row + 0xE) || (BG_S32(t + 8) >> 16) != BG_U16(row + 0x10)) {
            if (x < BG_U16(row + 0xE)) {
                BG_S16(s + 0xA0) = (s16)(x - 0x50);
            } else {
                BG_S16(s + 0xA0) = (s16)(x + 0x50);
            }
        } else if ((BG_S32(s + 0) >> 16) < x) {
            BG_S16(s + 0xA0) = (s16)(x - 0x50);
        } else {
            BG_S16(s + 0xA0) = (s16)(x + 0x50);
        }
        BG_S16(s + 0xA4) = (s16)(BG_S32(t + 8) >> 16);
        BG_S16(s + 0xA2) = 0;
        func_800BC460(mask);
        BG_S16(0x800D3354) = BG_S16(0x800D30A0);     /* D_800D3354 / D_800D30A0 */
        BG_S16(0x800D3354 + 2) = BG_S16(0x800D30A0 + 2);
        BG_S16(0x800D3354 + 4) = BG_S16(0x800D30A0 + 4);
        BG_S16(0x800D335C) = BG_S16(0x800D30A0 + 8); /* D_800D335C, vector 1 */
        BG_S16(0x800D335C + 2) = BG_S16(0x800D30A0 + 10);
        BG_S16(0x800D335C + 4) = BG_S16(0x800D30A0 + 12);
        func_800B9B54(BG_PTR(s), BG_PTR(t));
    }
    func_800BF9EC();
    func_800B9508(BG_PTR(s));
}
#endif
