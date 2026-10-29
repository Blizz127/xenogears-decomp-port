#ifndef BATTLE_C34B0_H
#define BATTLE_C34B0_H

/* The D_800C34B0 block seen as a struct, for the per-slot state bytes at
 * +0x5FA0.  Written as a member access, the block pointer loads before the
 * slot index (retail's order); pointer arithmetic loads the index first. */
typedef struct {
    u8 pad[0x5FA0];
    u8 state[1];
} BattleC34B0;

#endif
