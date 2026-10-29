/* Host port body of the battle command-menu state-5 handler func_80081B58 (the
 * state reached right after the Attack accept).  The byte-exact C stays in
 * mainasm_12068.c (retail build only; gcc 2.6.3); an adopted host TU cannot
 * compile it, because its D_800C3EAC / D_800D2D28 are guest-RAM words, not
 * typed pointers. */
#include "common.h"
#ifdef XENO_PC_PORT
#include "battle_host_guest.h"

extern void func_80077980(void);
extern void func_80080B64(u8 actor);
extern void func_8008189C(u32 actor);
extern void func_800819A4(u32 actor);
extern u32 func_80084854(u32 target, u32 direction);
/* Retail calls it with the hit count in a0 although the body takes none, and
 * func_80085EB4's host body has four (unused) parameters while retail passes
 * two: both are left unprototyped so the call keeps retail's argument words. */
extern void func_80085D34();
extern u32 func_80085EB4();
extern void func_800861D0(u32 state, u32 actor);
/* func_800877E0 is also a world-map function (src/world_map/main.c, a different
 * overlay at the same address) linked into the port, so the battle function is
 * reached through a distinctly named guest-call trampoline. */
extern u32 xeno_battle_guest_800877E0(u32 actor, u32 target);
extern void func_800879A8(u8 a, u8 b);
extern u32 func_80087AF0(u32 actor, u32 column);
extern void func_8008AA40(u8 sound);
extern void func_8009413C(u8 index, u8 flag);
extern void func_800B8DA4(void);

/* D_800C3EAC: the command context pointer (a guest word); D_800D2D28: the
 * per-actor battle block pointer; D_800D3014: the menu state; D_800D366C: a
 * flag.  Retail reloads the context/block pointers at every use, so no body
 * below caches them across a call (or across a store a callee could see). */
#define CMD_CONTEXT() BG_U32(0x800C3EACu)
#define ACTOR_BLOCK() BG_U32(0x800D2D28u)
#define MENU_STATE() BG_U8(0x800D3014u)
#define FLAG_366C() BG_U8(0x800D366Cu)
#define HIT_SCALE(row, col) BG_U8(0x800C31D4u + (row) * 8u + (col))

/* func_80081B58.s (mainasm_12068.c is the retail C reference).  `arg0` is the
 * actor byte; every callee gets it masked, as retail does (andi 0xFF).  The
 * two signed compares (+0x2D4 - 3, +0x2D4 - +0x2DF) are int arithmetic on the
 * loaded bytes (bltz).  Retail's v0 on return is residue; the driver ignores
 * it. */
s32 func_80081B58(u32 arg0) {
    u32 actor = arg0 & 0xFF;
    u32 ctx;
    u32 hits;
    u32 left;
    u32 state;
    u32 target;

    FLAG_366C() = 0;
    if (BG_U8(CMD_CONTEXT() + 0x2E1) != 0) {
        return 0;
    }
    BG_U8(CMD_CONTEXT() + 0x2DF) = 0;
    func_8008189C(actor);
    switch (MENU_STATE()) {
    case 5:
        ctx = CMD_CONTEXT();
        if (BG_U8(ctx + 0x2D4) != BG_U8(ctx + 0x2D5)) {
            func_80080B64(actor);
            BG_U8(CMD_CONTEXT() + 0x2E1) = 1;
        } else {
            func_800B8DA4();
            FLAG_366C() = 1;
            BG_U8(ACTOR_BLOCK() + 0x7B) = 0;
            BG_U8(ACTOR_BLOCK() + 0xAF) = 0;
            BG_U8(CMD_CONTEXT() + 0x2DD) = 1;
            func_80077980();
            func_8009413C(actor, 1);
        }
        break;
    case 4:
        if (BG_U16(CMD_CONTEXT() + (actor << 6) + 0x20) != 0) {
            MENU_STATE() = 5;
            func_8008AA40(0x4F);
        }
        break;
    case 7:
        if (BG_U16(CMD_CONTEXT() + (actor << 6) + 0x1E) != 0) {
            MENU_STATE() = 5;
            func_8008AA40(0x4F);
        }
        break;
    case 6:
        if (BG_U16(CMD_CONTEXT() + (actor << 6) + 0x1C) != 0) {
            MENU_STATE() = 5;
            func_8008AA40(0x4F);
        }
        break;
    }
    if (BG_U8(CMD_CONTEXT() + 0x2E9) == 0) {
        state = MENU_STATE();
        switch (state) {
        case 0:
        case 1:
        case 2:
        case 3:
            /* Retail has one block per state (direction 0..3 = the state);
             * the arm is identical.  The target byte is read from the context
             * loaded before the state, the result is stored through a fresh
             * context load. */
            target = BG_U8(CMD_CONTEXT() + 0x2E8);
            BG_U8(CMD_CONTEXT() + 0x2E8) = (u8)func_80084854(target, state);
            ctx = CMD_CONTEXT();
            func_800879A8(BG_U8(ctx + 0x2D3), BG_U8(ctx + 0x2E8));
            func_8008AA40(0x4C);
            break;
        }
        xeno_battle_guest_800877E0(actor, BG_U8(CMD_CONTEXT() + 0x2E8));
    }
    BG_U8(ACTOR_BLOCK() + 0xAF) = 1;
    switch (MENU_STATE()) {
    case 5:
        break;
    case 4:
        if ((u8)func_80085EB4(4, actor) != 0) {
            ctx = CMD_CONTEXT();
            if ((s32)BG_U8(ctx + 0x2D4) - 3 >= 0) {
                BG_U8(ctx + 0x2E4) = (u8)(BG_U8(ctx + 0x2E4) + 1);
            }
        }
        ctx = CMD_CONTEXT();
        BG_U8(ctx + 0x2DF) = (u8)(BG_U8(ctx + 0x2DF) + 1);
        /* fall through */
    case 7:
        ctx = CMD_CONTEXT();
        BG_U8(ctx + 0x2DF) = (u8)(BG_U8(ctx + 0x2DF) + 1);
        /* fall through */
    case 6:
        ctx = CMD_CONTEXT();
        BG_U8(ctx + 0x2DF) = (u8)(BG_U8(ctx + 0x2DF) + 1);
        ctx = CMD_CONTEXT();
        if ((s32)BG_U8(ctx + 0x2D4) - (s32)BG_U8(ctx + 0x2DF) >= 0) {
            func_8008AA40(0x4D);
            func_800819A4(actor);
            if (BG_U8(ACTOR_BLOCK() + 0xCB) != 0) {
                BG_U8(CMD_CONTEXT() + 0x2DD) = 0x64;
                BG_U8(CMD_CONTEXT() + 0x2E2) = 0xFF;
                BG_U8(CMD_CONTEXT() + 0x2E0) = 1;
            } else {
                BG_U8(CMD_CONTEXT() + 0x2DD) = 5;
                BG_U8(CMD_CONTEXT() + 0x2E2) = 5;
            }
            ctx = CMD_CONTEXT();
            left = BG_U8(ctx + 0x2D4);
            hits = BG_U8(ctx + 0x2DF);
            BG_U8(ctx + 0x2D4) = (u8)(left - hits);
            func_80085D34(hits);
            func_800861D0(MENU_STATE(), actor);
            target = func_80087AF0(actor, BG_U8(CMD_CONTEXT() + 0x2DF));
            BG_U8(CMD_CONTEXT() + 0x2E5) = (u8)target;
            ctx = CMD_CONTEXT();
            BG_U16(0x800D2E06u + actor * 2u) =
                (u16)((s32)HIT_SCALE(BG_U8(ctx + 0x2D5), BG_U8(ctx + 0x2D4)) *
                      100 / 56);
            BG_U8(ctx + 0x2E3) = 1;
        } else {
            func_8008AA40(0x4F);
        }
        if (BG_U8(CMD_CONTEXT() + 0x2D4) == 0) {
            func_80080B64(actor);
            BG_U8(CMD_CONTEXT() + 0x2E1) = 1;
        }
        if (BG_U8(CMD_CONTEXT() + 0x2E5) != 0) {
            func_80080B64(actor);
            BG_U8(CMD_CONTEXT() + 0x2E1) = 1;
        }
        break;
    }
    return 0;
}
#endif /* XENO_PC_PORT */
