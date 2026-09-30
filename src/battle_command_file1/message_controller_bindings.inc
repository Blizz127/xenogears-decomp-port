#include "xeno_battle_command_file1_controller.h"

/* Retail control-record fields accessed by this controller.  The preceding
 * 0x7F6 bytes and bytes 0x800..0x801 are outside this function's typed view. */
struct Xbcf1ControlFields {
    xbcf1_u8 before_x[0x7F6];
    xbcf1_u16 x;      /* 0x7F6 */
    xbcf1_u16 y;      /* 0x7F8 */
    xbcf1_u16 units;  /* 0x7FA */
    xbcf1_u16 rows;   /* 0x7FC */
    xbcf1_u16 flags;  /* 0x7FE */
    xbcf1_u8 before_phase[2];
    xbcf1_u8 phase;   /* 0x802 */
};
#define XBCF1_CONTROL(base) ((struct Xbcf1ControlFields *)(base))
#define XBCF1_U8(base, offset)  (*(xbcf1_u8 *)((xbcf1_u8 *)(base) + (offset)))
#define XBCF1_U16(base, offset) (*(xbcf1_u16 *)((xbcf1_u8 *)(base) + (offset)))
#define XBCF1_S16(base, offset) (*(xbcf1_s16 *)((xbcf1_u8 *)(base) + (offset)))

#ifndef XBCF1_NATIVE_BINDINGS
/* Resident and base-battle globals used by the retail module. */
extern xbcf1_u8 *D_800D3278;
extern xbcf1_u8 *D_800D2D28;
extern xbcf1_u8 *D_800D2DAC;
extern void *D_800D3340;
extern xbcf1_u8 D_800D3014;

/* File-1-local data. Link these names at the stated 0x801E addresses. */
extern const xbcf1_u16 xeno_battle_command_file1_defaults_9C10[5];
extern xbcf1_s32 xeno_battle_command_file1_cycle_9C1C;
extern xbcf1_s32 xeno_battle_command_file1_x_9C30;
extern xbcf1_s32 xeno_battle_command_file1_y_9C34;

/* Module-local calls. Their bodies remain separate retail/asm boundaries. */
extern void xeno_battle_command_file1_helper_6750(
    xbcf1_u8 arg0, xbcf1_u16 arg1, xbcf1_u16 arg2,
    xbcf1_u16 arg3, xbcf1_u16 arg4);
extern void xeno_battle_command_file1_helper_5B00(xbcf1_s32 arg0,
                                                  xbcf1_s32 arg1);

/* Base-battle boundaries. */
extern void func_8008F8F4(xbcf1_s32 arg0, xbcf1_u16 arg1,
                          xbcf1_u16 arg2, xbcf1_u16 arg3,
                          xbcf1_u16 arg4, xbcf1_u32 arg5,
                          xbcf1_u32 arg6);
extern void func_800716D8(void);
extern void func_8008FA60(xbcf1_s32 arg0);

/* Resident retail interfaces.  func_80032F54 is deliberately seven
 * arguments here: retail consumes rows at original SP+0x18.  This declaration
 * must not be replaced by the native port's eight-argument compatibility ABI. */
extern void func_80032F54(void *window, xbcf1_s32 tpage_x,
                          xbcf1_s32 tpage_y, xbcf1_s32 x, xbcf1_s32 y,
                          xbcf1_s32 width, xbcf1_s32 rows);
extern void func_80034614(void *window);
extern void *GetStringEntry(void *table, xbcf1_s32 index);
extern void func_80034714(void *window, void *string);
extern void func_800345E0(void *window);
extern void func_800346D4(void *window);

#endif
