/* Host port body of the battle command-task constructor.  The byte-exact
 * C is retail-only in mainc124.c. */
#include "common.h"
#ifdef XENO_PC_PORT
#include "battle_host_guest.h"

extern void* HeapAlloc(u_int allocSize, u_int allocFlags);
extern void func_800BF0B4(u32 value);
extern s32 D_80059464;
extern u8 D_800591AC;

/* D_800C3610: the command task (guest word). */
#define CMD_TASK() BG_U32(0x800C3610u)

/* func_800BED4C.s: allocate the 0x50-byte task (guest address, as retail's
 * heap returns), install func_800B9F78 as its +8 callback, clear
 * D_800C360C, initialise the fields in retail's store order, call
 * func_800BF0B4(0) (retail stores +0x34 in its delay slot), clear
 * D_80059464, set D_800591AC and return the re-read task. */
u32 func_800BED4C(void) {
    u32 task = BG_ADDR(HeapAlloc(0x50, 0));

    BG_U32(task + 0x8) = 0x800B9F78u;
    CMD_TASK() = task;
    BG_U32(0x800C360Cu) = 0;
    BG_U8(task + 0x4A) = 0;
    BG_U8(CMD_TASK() + 0x48) = 1;
    BG_U32(CMD_TASK() + 0x30) = 0;
    BG_U8(CMD_TASK() + 0x49) = 0;
    BG_U32(CMD_TASK() + 0x2C) = 1;
    BG_U32(CMD_TASK() + 0x4) = 0;
    BG_U32(CMD_TASK() + 0x34) = 0;
    func_800BF0B4(0);
    D_80059464 = 0;
    D_800591AC = 1;
    return CMD_TASK();
}
#endif
