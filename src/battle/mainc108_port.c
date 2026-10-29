/* Host port bodies of the command-step actions func_800B9284 and
 * func_800B9508.  The byte-exact C is retail-only in mainc108.c (under
 * #ifndef XENO_PC_PORT), so there is no host-body fence to add here. */
#include "common.h"
#ifdef XENO_PC_PORT
#include <string.h>
#include "battle_host_guest.h"

/* Main-executable services (native C, host pointers). */
extern int func_8001EE68(u8* arg0);
extern void func_80021BF8(void* pSpriteData, s32 value);
extern void func_80021FB8(u8* sprite, u8 value);
extern void func_800245D8(void* pSpriteData, s16 animIndex);
extern void SpriteSetSpecialAnimFile(void* sprite, void* file);
extern void* HeapAlloc(unsigned int allocSize, unsigned int allocFlags);
extern unsigned int HeapFree(void* pMem);
extern void* HeapGetNextBlockHeader(void* pBlock);
extern void func_80079E18(u8 index);
extern void func_80079E4C(u8 index);
extern void func_800B8048(u32 v);
extern void func_800BC404(u32 mask);
extern void func_800BF600(u8* id, u8* actor);
extern void func_800BF730(u32 actor);
extern void func_800B8354(void);
extern void func_800B9B30(void);
extern void func_800BC460(u32 mask);
extern void func_800BEFF4(u32 slot);   /* leaves table[slot] in task +4 */
extern void func_800BF0B4(u32 value);
extern void func_800BF0C4(u8* sprite);
extern u32 func_800BF354(void);
extern void func_800BF3E8(u8* sprite);
extern u32 func_800C0314(void);

/* u16 at +off of the current command step: the 0x48-byte D_800C3EB0 row
 * D_800C360C, re-read at every use. */
static u32 b9284_step_u16(u32 off) {
    return BG_U16(0x800C3EB0 + BG_U32(0x800C360C) * 0x48 + off);
}

/* func_800B9284.s: one command-step action for step kinds 0xF3..0xFA (from
 * func_800B9C78 and func_800B9508).  Most kinds use the step's u16 parameter
 * at +0x172: 0xFA/0xF8/0xF7 set the command task's (D_800C3610) +0x48 flag,
 * call func_80021BF8(actor, 0) and pass the parameter to func_80079E18 /
 * func_80079E4C / D_800C3614; 0xF5 and 0xF4 call func_800B8048(actor) and
 * split the parameter into a 6-bit field for D_800C3DF0 and a 9-bit id for
 * func_800BF600 (with the actor's +0xAF state set to 0x11 first; 0xF5 then
 * also calls func_800BF730(actor) and func_800245D8(actor, 0x13) and sets
 * D_800C3623); 0xF6 passes the step's +0x14E u16 to func_800BC404; 0xF3 ORs
 * the parameter into D_800C3608; 0xF9 and anything else do nothing.  The
 * actor is a sprite record (guest word or compact host address). */
void func_800B9284(u8* actor_ptr, u32 kind) {
    u32 actor = BG_ADDR(actor_ptr);
    s32 param;

    switch (kind) {
    case 0xFA:
        BG_U8(BG_U32(0x800C3610) + 0x48) = 1;
        func_80021BF8(bg_arg(actor), 0);
        func_80079E18((u8)b9284_step_u16(0x172));
        break;
    case 0xF8:
        BG_U8(BG_U32(0x800C3610) + 0x48) = 1;
        func_80021BF8(bg_arg(actor), 0);
        func_80079E4C((u8)b9284_step_u16(0x172));
        break;
    case 0xF7:
        BG_U8(BG_U32(0x800C3610) + 0x48) = 1;
        func_80021BF8(bg_arg(actor), 0);
        BG_U16(0x800C3614) = (u16)b9284_step_u16(0x172); /* D_800C3614 */
        break;
    case 0xF5:
        func_800B8048(actor);
        param = (s32)b9284_step_u16(0x172);
        BG_U16(0x800C3DF0) = (u16)((param >> 9) & 0x3F); /* D_800C3DF0 */
        BG_U8(actor + 0xAF) = 0x11;
        func_800BF600((u8*)(uintptr_t)(u32)(param & 0x1FF), bg_arg(actor));
        func_800BF730(actor);
        func_800245D8(bg_arg(actor), 0x13);
        BG_U8(0x800C3623) = 1; /* D_800C3623 */
        break;
    case 0xF4:
        func_800B8048(actor);
        param = (s32)b9284_step_u16(0x172);
        BG_U16(0x800C3DF0) = (u16)((param >> 9) & 0x3F);
        BG_U8(actor + 0xAF) = 0x11;
        func_800BF600((u8*)(uintptr_t)(u32)(param & 0x1FF), bg_arg(actor));
        break;
    case 0xF6:
        func_800BC404(b9284_step_u16(0x14E));
        break;
    case 0xF3:
        BG_U8(BG_U32(0x800C3610) + 0x48) = 1;
        BG_U16(0x800C3608) = (u16)(BG_U16(0x800C3608) | b9284_step_u16(0x172)); /* D_800C3608 */
        break;
    default:
        break;
    }
}

#define CMD_TASK 0x800C3610u    /* D_800C3610: the command task (guest address) */
#define CMD_STEP 0x800C360Cu    /* D_800C360C: step index */
#define CMD_WAIT 0x800C3614u    /* D_800C3614: s16 step wait counter */
#define CMD_GFX_BUF 0x800C3618u /* D_800C3618: loaded sprite file buffer (raw host word) */
#define CMD_FLAG_3626 0x800C3626u
#define CMD_ROWS 0x800C3EB0u    /* D_800C3EB0: 28-byte rows, 72-byte steps */
#define CMD_DAT_3630 0x800C3630u /* D_800C3630: s16 per-class id base */
#define CMD_DAT_3648 0x800C3648u /* D_800C3648: s16 per-class id limit */
#define CMD_DAT_D3350 0x800D3350u
#define CMD_CB_B9B30 0x800B9B30u /* func_800B9B30, passed as a guest word */

/* The current command step: the 72-byte row D_800C360C of D_800C3EB0, re-read
 * at every use as retail does. */
static u32 b95_step(void) {
    return CMD_ROWS + BG_U32(CMD_STEP) * 0x48u;
}

static void b95_advance(void) {
    BG_U32(CMD_STEP) = BG_U32(CMD_STEP) + 1;
}

/* Sprite slot: +0xAC bits 0..1 above +0xA8 bits 30..31. */
static u32 b95_slot(u32 spr) {
    u32 lo = BG_U32(spr + 0xA8) >> 30;

    return ((BG_U32(spr + 0xAC) & 3u) << 2) | lo;
}

/* func_800B9508.s: run the actor's next battle step code (the byte at step
 * +0x17F).  F3..F8/FA go to func_800B9284; F9 and FB..FF are control codes
 * (F9: animation 5; FF: wait callback, then FE; FE: spawn through
 * func_800BEFF4; FD/FC/FB: callback, +0xB0 byte, record copy); every other
 * code starts an animation (< 0x10) or an effect id (kind - 0x10 plus the
 * D_800C3630 class base, bounded by D_800C3648).  Retail re-reads D_800C3610,
 * D_800C360C and the sprite words at every use and stores task +0x48 before
 * its calls (some in jal delay slots); the order below is the instruction
 * order.  The sprite is a host pointer on entry and a guest word thereafter;
 * heap words (sprite +0x20 block +0x2C/+0x30, D_800C3618) are raw host
 * addresses, as the native readers (temp1.c) store and free them. */
void func_800B9508(u8* rec_ptr) {
    u32 rec = BG_ADDR(rec_ptr);
    u32 kind;
    u32 task;
    u32 row;
    u32 other;
    u32 blk;
    u32 v;

    if (BG_S16(CMD_WAIT) != 0) {
        BG_U16(CMD_WAIT) = (u16)(BG_U16(CMD_WAIT) - 1u);
        return;
    }
    kind = BG_U8(b95_step() + 0x17F);
    switch (kind) {
    case 0xF3:
    case 0xF4:
    case 0xF5:
    case 0xF6:
    case 0xF7:
    case 0xF8:
    case 0xFA:
        func_800B9284(rec_ptr, kind);
        b95_advance();
        return;
    case 0xF9:
        func_800245D8(bg_arg(rec), 5);
        func_800BC404(0);
        return;
    case 0xFF:
        func_80021BF8(bg_arg(rec), 0);
        BG_U8(BG_U32(CMD_TASK) + 0x48) = 1;
        if (func_800C0314() == 0) {
            v = BG_U32(rec + 0xA8);
            if (((v >> 28) & 3u) == 0 && BG_U8(CMD_ROWS + b95_slot(rec) * 28u + 7) == 0) {
                func_800245D8(bg_arg(rec), (s16)BG_S8(rec + 0xB0));
            }
            return;
        }
        /* fall through */
    case 0xFE:
        BG_U8(BG_U32(CMD_TASK) + 0x48) = 0;
        func_800BEFF4(BG_U32(BG_U32(CMD_TASK) + 0x20));
        /* Retail takes func_800BEFF4's v0 (table[slot]); the native body
         * returns void and leaves the same word in task +4. */
        rec = BG_U32(BG_U32(CMD_TASK) + 4);
        func_800BF3E8(BG_PTR(rec));
        func_80021BF8(bg_arg(rec), (s32)CMD_CB_B9B30);
        func_800BF0B4(5);
        row = CMD_ROWS + BG_U32(BG_U32(CMD_TASK) + 0x24) * 28u;
        /* lh against lhu: values >= 0x8000 never compare equal. */
        if ((s32)BG_S16(rec + 2) == (s32)BG_U16(row + 0xE) &&
            (s32)BG_S16(rec + 0xA) == (s32)BG_U16(row + 0x10)) {
            func_800B9B30();
            return;
        }
        task = BG_U32(CMD_TASK);
        BG_U16(rec + 0xA0) = BG_U16(CMD_ROWS + BG_U32(task + 0x24) * 28u + 0xE);
        v = BG_U16(CMD_ROWS + BG_U32(task + 0x24) * 28u + 0x10);
        BG_U16(rec + 0xA2) = 0;
        BG_U16(rec + 0xA4) = (u16)v;
        func_800245D8(bg_arg(rec), 4);
        return;
    case 0xFD:
        BG_U8(BG_U32(CMD_TASK) + 0x48) = 1;
        if (BG_U16(b95_step() + 0x172) == 0) {
            BG_U8(BG_U32(CMD_TASK) + 0x48) = 0;
            func_800BF0C4(bg_arg(rec));
            func_80021BF8(bg_arg(rec), (s32)CMD_CB_B9B30);
        }
        b95_advance();
        return;
    case 0xFC:
        BG_U8(BG_U32(CMD_TASK) + 0x48) = 1;
        func_80021BF8(bg_arg(rec), 0);
        func_80021FB8((u8*)bg_arg(rec), (u8)BG_U16(b95_step() + 0x172));
        b95_advance();
        return;
    case 0xFB: {
        u8 first[4];

        BG_U8(BG_U32(CMD_TASK) + 0x48) = 1;
        func_80021BF8(bg_arg(rec), 0);
        other = BG_U32(CMD_ROWS + (u32)BG_U16(b95_step() + 0x172) * 4u + 0x8C8Cu);
        v = BG_U32(rec + 0x7C);
        BG_U32(rec + 0x24) = BG_U32(other + 0x24);
        /* lwl/lwr + swl/swr: one unaligned word, loaded before it is stored. */
        memcpy(first, BG_PTR(BG_U32(other + 0x7C) + 0xE), 4);
        memcpy(BG_PTR(v + 0xE), first, 4);
        v = BG_U32(rec + 0x3C);
        blk = BG_U32(rec + 0x20);
        BG_U32(rec + 0x3C) = v | 0x40000000u;
        HeapFree((void*)(uintptr_t)BG_U32(blk + 0x2C));
        v = (u32)(uintptr_t)HeapGetNextBlockHeader(
            (void*)(uintptr_t)BG_U32(BG_U32(other + 0x20) + 0x2C));
        v = (u32)(uintptr_t)HeapAlloc(v, 0);
        blk = BG_U32(rec + 0x20);
        BG_U32(blk + 0x2C) = v;
        BG_U32(blk + 0x30) = v;
        b95_advance();
        return;
    }
    default:
        other = BG_U32(rec + 0x74);
        BG_U16(CMD_FLAG_3626) = 0;
        func_800B8048(rec);
        func_80021BF8(bg_arg(rec), (s32)CMD_CB_B9B30);
        v = BG_U8(b95_step() + b95_slot(other) + 0x150);
        BG_U32(BG_U32(rec + 0x7C) + 8) = v;
        if (func_8001EE68((u8*)bg_arg(BG_U32(BG_U32(rec + 0x24)))) == 0) {
            if (kind >= 0x10) {
                s32 id = (s32)kind - 0x10;
                u32 cls = BG_U8(CMD_ROWS + b95_slot(rec) * 28u + 6);

                id += BG_S16(CMD_DAT_3630 + cls * 2u);
                BG_U8(rec + 0xAF) = (id < BG_S16(CMD_DAT_3648 + cls * 2u)) ? 0x1C : 0x11;
                func_800BF600((u8*)(uintptr_t)(u32)id, bg_arg(rec));
                func_800BF730(rec);
            } else {
                s32 anim = (s32)kind;

                if (BG_U8(CMD_DAT_D3350) == 0 && BG_U32(CMD_GFX_BUF) != 0) {
                    func_800B8354();
                    v = func_800BF354();
                    /* the +0x50 store sits in the jal delay slot, after the
                     * D_800C3618 reload and before the call */
                    BG_U32(rec + 0x50) = v;
                    SpriteSetSpecialAnimFile(bg_arg(rec), (void*)(uintptr_t)BG_U32(CMD_GFX_BUF));
                }
                if (BG_U32(BG_U32(CMD_TASK) + 0x20) == b95_slot(rec)) {
                    func_800245D8(bg_arg(rec), (s16)~anim);
                } else {
                    func_800245D8(bg_arg(rec), (s16)anim);
                }
            }
        } else {
            func_800245D8(bg_arg(rec), (s16)kind);
            v = 1u << (b95_slot(rec) & 31u);
            func_800BC460(v | (1u << (b95_slot(BG_U32(rec + 0x74)) & 31u)));
        }
        BG_U8(BG_U32(CMD_TASK) + 0x48) = 0;
        b95_advance();
        return;
    }
}
#endif
