#ifndef BATTLE_SPAD_STACK_H
#define BATTLE_SPAD_STACK_H

/* Retail runs some heavy battle callees on a stack in the scratchpad:
 * store $sp at addr, point $sp just below it, call, then step back up and
 * reload the saved $sp.  These emit the exact retail sequence, so they are
 * for the matching build only; the host keeps its native stack. */
#define BATTLE_SPAD_STACK_SET(addr)                                                  \
    __asm__ volatile("move $8, %0\n\tsw $29, 0($8)\n\taddiu $8, $8, -4\n\tmove $29, $8" \
                     :                                                               \
                     : "r"(addr)                                                     \
                     : "$8", "memory")
#define BATTLE_SPAD_STACK_RESET() __asm__ volatile("addiu $29, $29, 4\n\tlw $29, 0($29)" : : : "memory")

#endif
