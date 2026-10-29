/* Host port body of the command-step wrap-up.  The byte-exact C is
 * retail-only in main106.c. */
#include "common.h"
#ifdef XENO_PC_PORT
#include "battle_host_guest.h"

extern s32 D_80059464; /* main-executable timer-entry count */
extern int DrawSync(int mode);
extern void func_80021BF8(void* pSpriteData, s32 value);
extern void func_8003A89C(void* manager, s32 level, s32 steps);
extern void func_80080BD0(void);
extern u32 func_800C0314(void);
extern u32 func_800BF720(void);
extern void func_800BE790(void);
extern void func_800C0564(void);
extern void func_800B8D7C(void);
extern void func_800BC3F8(u32 v);
extern void func_800BE0DC(void);
extern void func_800BF9EC(void);
extern void func_800BFA9C(void);
extern void func_800BC454(u16 v);
extern void func_800BEDE8(void);
extern void func_800BA4E0(s32 arg0);

/* func_800B8EBC.s: with a command task (D_800C3610), clear D_800C3624 /
 * D_800C3DF0 / D_800C3622, run func_80080BD0 and DrawSync(0), read the
 * task's +0x40 word, func_80021BF8(actor, 0) and func_800C0314, wait
 * (func_800BE790) until func_800BF720() equals D_80059464, run
 * func_800C0564, func_800B8D7C, func_800BC3F8(1), func_800BE0DC,
 * func_800BF9EC, func_800BFA9C, clear D_800C3688, func_800BC454(0xC0),
 * func_800BEDE8 and func_800BA4E0 (the +0x40 word), call
 * func_8003A89C(D_800C3E54, 0x7F, 0x50) when D_800C35D4 is set, clear it,
 * and clear bit 6 of +0xAC on each of the 11 records at D_800C3EB0 +0x8C8C.
 * Retail loads the actor (task +4) before its NULL test of the task; the
 * port loads it after, as that load has no effect.  The actor is a sprite
 * guest word. */
void func_800B8EBC(void) {
    u32 task = BG_U32(0x800C3610); /* D_800C3610 */
    u32 actor;
    s32 arg;
    s32 i;

    if (task == 0) {
        return;
    }
    actor = BG_U32(task + 4);
    BG_U8(0x800C3624) = 0; /* D_800C3624 */
    BG_U16(0x800C3DF0) = 0; /* D_800C3DF0 */
    BG_U8(0x800C3622) = 0; /* D_800C3622 */
    func_80080BD0();
    DrawSync(0);
    arg = BG_S32(BG_U32(0x800C3610) + 0x40);
    func_80021BF8(bg_arg(actor), 0);
    func_800C0314();
    for (;;) {
        u32 v = func_800BF720();

        if ((u32)D_80059464 == v) {
            break;
        }
        func_800BE790();
    }
    func_800C0564();
    func_800B8D7C();
    func_800BC3F8(1);
    func_800BE0DC();
    func_800BF9EC();
    func_800BFA9C();
    BG_U8(0x800C3688) = 0; /* D_800C3688 */
    func_800BC454(0xC0);
    func_800BEDE8();
    func_800BA4E0(arg);
    if (BG_U8(0x800C35D4) != 0) { /* D_800C35D4 */
        func_8003A89C(bg_arg(BG_U32(0x800C3E54)), 0x7F, 0x50); /* D_800C3E54 */
    }
    BG_U8(0x800C35D4) = 0;
    for (i = 0; i != 11; i++) {
        u32 rec = BG_U32(0x800C3EB0 + 0x8C8C + (u32)i * 4);

        if (rec != 0) {
            BG_U32(rec + 0xAC) &= ~0x40u;
        }
    }
}
#endif
