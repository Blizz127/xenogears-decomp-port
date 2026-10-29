/* Host port body of func_800B89FC, "open the battle command window".  The
 * byte-exact C stays in mainc104.c under #ifndef XENO_PC_PORT (the retail
 * build compiles it; the port never sees it), so there is no host-body fence
 * to add: this TU is the only definition an adopted host build can link.
 *
 * Every overlay access goes through BG_*; every overlay address is a u32.
 * The statement order is the retail instruction order (see the notes), which
 * matters because the callees read the globals this body stores: D_800C3610
 * and the task +0x20/+0x40 words before func_800BEFF4, D_800C3608 before
 * func_800BED4C returns, D_800C3620/D_800C3622 before their next call, and
 * D_800C3A6C before the sound-load wait loop. */
#include "common.h"
#ifdef XENO_PC_PORT
#include <stdio.h>
#include <stdlib.h>
#include "battle_host_guest.h"

/* Main-executable services (native C, host pointers). */
extern int ArchiveSetIndex(int directoryIndex, int entryIndex);
extern int ArchiveDecodeAlignedSize(unsigned int entryIndex);
extern void* HeapAlloc(unsigned int allocSize, unsigned int allocFlags);
extern unsigned int HeapFree(void* pMem);
extern int ArchiveReadFileToBuffer(int index, void* pBuffer, unsigned int arg2,
                                   unsigned int flags);
extern void* SoundLoadWdsFile(void* pWdsFile, int mode);
extern int func_8001EE68(u8* arg0);
extern void func_80021BF8(void* arg0, s32 arg1);
extern s16 func_8003BDFC(s32 flags);

/* Battle-overlay callees.  Pointer-argument callees take a host pointer
 * (BG_PTR of the guest address), as the other port bodies pass them. */
extern void func_800B8354(void);
extern void func_800B8D04(void);
extern void func_800BC3F8(u32 v);
extern void func_800BC454(u16 v);
extern void func_800BE790(void);
extern u32 func_800BED4C(void);               /* returns the new task's address */
extern void func_800BEE2C(u32 slot, u32 arg, u32 mode);
extern void func_800BEFF4(u32 slot);          /* leaves table[slot] in task +4 */
extern void func_800BF0B4(u32 value);
extern void func_800BF0C4(u8* spr);
extern void func_800BF2B8(u8* spr);
extern void func_800BF3E8(u8* spr);
extern void func_800BFBA0(void);
extern void func_800C0F70(void);

#define CMD_TASK 0x800C3610u      /* D_800C3610: the command task (guest address) */
#define CMD_FREE_MASK 0x800C3608u /* D_800C3608: u16 */
#define CMD_STEP 0x800C360Cu      /* D_800C360C: step index */
#define CMD_GFX_BUF 0x800C3618u   /* D_800C3618: loaded sprite file buffer */
#define CMD_GFX_SLOT 0x800C361Cu  /* D_800C361C: slot that buffer belongs to */
#define CMD_BANK_ARMED 0x800C3620u
#define CMD_BANK_LOADED 0x800C3622u
#define CMD_FLAG_3624 0x800C3624u
#define CMD_WDS 0x800C3A6Cu       /* D_800C3A6C: loaded WDS entry */
#define CMD_CLEAR_3DF0 0x800C3DF0u
#define CMD_ROWS 0x800C3EB0u      /* D_800C3EB0: 28-byte rows, 72-byte steps */
#define CMD_ROW(index) (CMD_ROWS + (index) * 28u)
#define CMD_CB_B9B30 0x800B9B30u  /* func_800B9B30, passed as a guest word */

/* func_800B89FC.s: open the battle command window for combatant `index`
 * (main12's turn start calls it with resume 1).  `target` goes to task +0x40
 * as a plain word.  The D_800C3EB0 row byte +8 is read at each decision point
 * (retail re-reads it); the sprite is the task's +4 word after the
 * func_800BF3E8 call and is not reloaded afterwards. */
void func_800B89FC(s32 resume, s32 index, s32 column, s32 target) {
    u32 idx = (u32)index;
    u32 task;
    u32 spr;
    u32 mask;
    void* buf;

    func_800BC3F8(0);
    if (BG_U32(CMD_TASK) != 0) {
        /* Retail spins on `break 1`: a command task already exists. */
        fprintf(stderr, "[xeno-port][battle] func_800B89FC: command task "
                        "0x%08x already exists (retail break 1 spin)\n",
                BG_U32(CMD_TASK));
        abort();
    }
    func_800BE790();
    func_800BE790();
    BG_U8(CMD_FLAG_3624) = 0;
    /* sllv takes the shift count mod 32; the sh runs in the jal delay slot,
     * before func_800BED4C. */
    mask = BG_U16(CMD_FREE_MASK) & ~(1u << (idx & 31u));
    BG_U16(CMD_FREE_MASK) = (u16)mask;
    task = func_800BED4C();
    BG_U32(CMD_TASK) = task;
    BG_U32(task + 0x40) = (u32)target;
    BG_U32(task + 0x20) = idx;
    func_800BEFF4(idx);
    /* Retail passes func_800BEFF4's v0 (table[idx], which it also stored at
     * task +4) to func_800BF3E8; the native body returns void. */
    func_800BF3E8(BG_PTR(BG_U32(BG_U32(CMD_TASK) + 4)));
    BG_U16(CMD_CLEAR_3DF0) = 0;
    spr = BG_U32(BG_U32(CMD_TASK) + 4);
    if (BG_U8(CMD_ROW(idx) + 8) == 0) {
        func_800C0F70();
        BG_U8(CMD_BANK_ARMED) = 0;
        func_800BC454(0xC0);
        if (func_8001EE68((u8*)bg_arg(BG_U32(BG_U32(spr + 0x24)))) == 0) {
            u32 gfx = BG_U32(CMD_GFX_BUF);

            BG_U8(CMD_BANK_LOADED) = 0;
            if (gfx == 0 ||
                (((BG_U32(spr + 0xAC) & 3u) << 2) | (BG_U32(spr + 0xA8) >> 30)) !=
                    BG_U32(CMD_GFX_SLOT)) {
                func_800BF2B8(BG_PTR(spr));
            }
        } else {
            if (BG_U8(CMD_BANK_LOADED) == 0) {
                func_800B8354();
                ArchiveSetIndex(0x2C, 0);
                buf = HeapAlloc((unsigned int)ArchiveDecodeAlignedSize(7), 0);
                ArchiveReadFileToBuffer(7, buf, 0, 0x80);
                func_800B8354();
                func_800C0F70();
                /* D_800C3A6C keeps the entry pointer as a raw u32, as
                 * func_800C0F70 hands it back to SoundFreeWdsEntry. */
                BG_U32(CMD_WDS) = (u32)(uintptr_t)SoundLoadWdsFile(buf, 0);
                while ((s16)func_8003BDFC(0) != 0) {
                    func_800BE790();
                }
                HeapFree(buf);
            }
            BG_U8(CMD_BANK_LOADED) = 1;
        }
    } else {
        func_800B8D04();
        func_800BFBA0();
        func_800BF0B4(8);
        func_800BEE2C(idx, (u32)column, 0x1A);
        func_800BC454(0xC0);
    }
    if (resume == 0) {
        if (BG_U8(CMD_ROW(idx) + 8) != 0) {
            func_800BEE2C(idx, BG_U16(CMD_ROWS + BG_U32(CMD_STEP) * 0x48u + 0x14E), 2);
        } else {
            func_80021BF8(BG_PTR(spr), (s32)CMD_CB_B9B30);
            func_800BF0C4(BG_PTR(spr));
        }
    } else {
        func_800BF0B4(BG_U8(CMD_ROW(idx) + 8) != 0 ? 9 : 4);
    }
}
#endif
