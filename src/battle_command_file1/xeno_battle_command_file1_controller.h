#ifndef XENO_BATTLE_COMMAND_FILE1_CONTROLLER_H
#define XENO_BATTLE_COMMAND_FILE1_CONTROLLER_H

typedef signed char xbcf1_s8;
typedef unsigned char xbcf1_u8;
typedef signed short xbcf1_s16;
typedef unsigned short xbcf1_u16;
typedef signed int xbcf1_s32;
typedef unsigned int xbcf1_u32;

/* Retail function 0x801E6CE8..0x801E71D4.
 * Argument meanings beyond their proven widths remain unnamed. */
xbcf1_s32 xeno_battle_command_file1_controller(
    xbcf1_u16 arg0, xbcf1_u8 arg1, xbcf1_u16 arg2);

#endif
