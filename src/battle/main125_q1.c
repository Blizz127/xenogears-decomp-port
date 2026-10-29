/* Retail run 1 of main125: the byte-exact C bodies on one side of a
 * retail-asm gap; see the BATTLE_SUB note in main125.c. */
#define BATTLE_TU_SUB 1
#include "common.h"
#ifndef XENO_PC_PORT
extern void *HeapAlloc(u_int allocSize, u_int allocFlags);
extern u_int HeapFree(void *pMem);
extern void func_800AA320(u32 arg0, u32 arg1, u32 arg2);

/* func_800BEE2C.s: run func_800AA320(arg0 & 0xFFFF, arg1 & 0xFFFF, arg2) on a
 * 0x1000-byte HeapAlloc stack: the caller's $sp is saved at buf+0xF9C and
 * $sp set to buf+0xF98 around the call (GCC inline asm, as func_800B626C in
 * main87 does), then HeapFree(buf).  The prototypes repeat main125.c's. */
void func_800BEE2C(u32 arg0, u32 arg1, u32 arg2) {
    u8* stack = HeapAlloc(0x1000, 1);

    __asm__ volatile(
        "\taddu  $t0, %0, $zero\n"
        "\tsw    $sp, 0($t0)\n"
        "\taddiu $t0, $t0, -4\n"
        "\taddu  $sp, $t0, $zero\n"
        :
        : "r"(stack + 0xF9C)
        : "t0", "memory");
    func_800AA320(arg0 & 0xFFFF, arg1 & 0xFFFF, arg2);
    __asm__ volatile(
        "\taddiu $sp, $sp, 4\n"
        "\tlw    $sp, 0($sp)\n"
        :
        :
        : "memory");
    HeapFree(stack);
}
#endif
#include "main125.c"
