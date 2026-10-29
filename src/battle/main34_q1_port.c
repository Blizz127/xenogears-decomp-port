/* Host port body of the battle command-menu driver func_80080160.  The
 * byte-exact C stays in main34_q1.c (retail build and the matching tests); an
 * adopted host TU cannot compile it, because its D_800C3EAC / D_800D2D28 are
 * guest-RAM words, not typed pointers. */
#include "common.h"
#ifdef XENO_PC_PORT
#include "battle_host_guest.h"

extern void func_800716D8(void);
extern void func_8008AA74(u32 sound);
extern void func_8007FEC4(void);
extern void func_8007FF14(u32 actor);
extern void func_80085E78(void);
extern void func_8009AB00(u8 index);
extern u16 func_80089C08(u8 index);
extern void func_800BC404(u32 mask);
extern void func_800BCD98(u32 mask);
extern void func_8007FBE0(u8 count, u8 sel);
extern s32 func_8008D598(u8 actor, u8 menu, u8 fade);
/* Retail calls it with (actor, 0x100); the host body takes no parameters, so
 * the declaration is left unprototyped to keep both words at the call. */
extern void func_800BAF40();
extern void func_8007FCE8(void);
extern void func_8007FDEC(void);
extern void func_800800E8(u8 index);
extern void func_8007FB70(u8 index);
extern void func_8008115C(u32 actor);
extern void func_80081318(u32 actor);
extern void func_80081504(u32 actor);
extern void func_800816F8(u32 actor);
extern s32 func_80081B58(u32 actor);
extern void func_800820A4(u32 actor);
extern void func_800822C4(u32 actor);
extern void func_80082504(u32 actor);
extern void func_80082820(u32 actor);
extern void func_800829F4(u32 actor);
extern void func_80082BB0(u32 actor);
extern void func_80082D4C(u32 actor);
extern void func_80082F7C(u32 actor);
extern void func_800830A8(u32 actor);
extern void func_80083340(u32 actor);
extern void func_80083580(u32 actor);
extern void func_80083748(u32 actor);
extern s32 func_80083948(u32 actor);

/* D_800C3EAC: the command context pointer (a guest word); 64-byte actor rows
 * from its base.  D_800D2D28: the per-actor battle block pointer.  Retail
 * reloads both pointers at every use, so no body below caches them across a
 * call (or across a store the callee could see). */
#define CMD_CONTEXT() BG_U32(0x800C3EACu)
#define CMD_ROW(actor) (CMD_CONTEXT() + (actor) * 0x40u)
#define ACTOR_BLOCK() BG_U32(0x800D2D28u)
/* g_GameState (0x8006D634): BG_* resolves it exactly as the interpreter does
 * (the native copy below +0x2358, guest RAM past it). */
#define GS(offset) (0x8006D634u + (offset))
#define ACT_COUNT(actor) (0x800D32A0u + (actor) * 8u)
#define ACT_ALT(actor) (0x800D32A1u + (actor) * 8u)

/* func_80080160.s (main34_q1.c): the retail C is the reference; the comments
 * there give the CSE-sensitive re-reads.  `arg0` is the actor byte; every
 * callee gets it masked, as retail does (andi 0xFF). */
void func_80080160(u32 arg0) {
    u32 actor = arg0 & 0xFF;
    u32 ctx;
    u32 row;
    u32 i;
    u32 mask;
    u32 t;
    s32 ret;
    u8 v;

    BG_U8(CMD_CONTEXT() + 0x2EA) = 1;
    BG_U8(CMD_CONTEXT() + 0x2E9) = 0;
    BG_U8(CMD_CONTEXT() + 0x2DE) = 0;
    BG_U8(CMD_CONTEXT() + 0x2E0) = 0;
    BG_U8(CMD_CONTEXT() + 0x2E1) = 0;
    BG_U8(CMD_CONTEXT() + 0x2E2) = 0xFF;
    BG_U8(CMD_CONTEXT() + 0x2E3) = 0;
    BG_U8(CMD_CONTEXT() + 0x2E4) = 0;
    BG_U8(CMD_CONTEXT() + 0x2DC) = 0;
    ctx = CMD_CONTEXT();
    v = BG_U8(0x800D32A4u + actor * 8u);
    BG_U8(ctx + 0x2D5) = v;
    BG_U8(ctx + 0x2D4) = v;
    BG_U8(ACTOR_BLOCK() + actor + 0x90) = 3;
    BG_U8(CMD_CONTEXT() + 0x2E7) = 0;
    BG_U8(0x800C3E29u) = 0xFF;
    BG_U8(0x800C4929u) = 0;
    while (BG_U8(ACTOR_BLOCK() + actor + 0x90) != 1) {
        func_800716D8();
    }
    BG_U8(0x800D366Cu) = 1;
    func_8008AA74(0x5A);
    ctx = CMD_CONTEXT();
    BG_U8(0x800D366Cu) = 0;
    for (i = 0; i < 16; i++) {
        BG_U16(ctx + actor * 0x40u + 0x1C + i * 2u) =
            BG_U16(0x800CCD62u + actor * 0x170u) & BG_U16(0x800C3234u + i * 2u);
    }
    if (BG_U8(0x800D2D24u + actor) == 7 && BG_U8(ACT_ALT(actor)) != 0) {
        row = CMD_ROW(actor);
        BG_U16(row + 0x36) = BG_U16(0x800C324Eu);
        BG_U16(row + 0x24) = BG_U16(0x800C323Cu);
    }
    if (BG_U8(0x800D2D24u + actor) == 4) {
        if (BG_U8(ACT_ALT(actor)) == 0) {
            if (BG_U8(GS(0x2286) + BG_U8(GS(0x56B))) == 0) {
                BG_U16(CMD_ROW(actor) + 0x1C) = BG_U16(0x800C3234u);
            }
            if (BG_U8(GS(0x2286) + BG_U8(GS(0x56E))) == 0) {
                BG_U16(CMD_ROW(actor) + 0x20) = BG_U16(0x800C3238u);
            }
        } else {
            if (BG_U8(GS(0x22B6) + BG_U8(GS(0x97C) + BG_U8(GS(0x59C)) * 0xA4u)) == 0) {
                BG_U16(CMD_ROW(actor) + 0x1C) = BG_U16(0x800C3234u);
            }
            t = BG_U8(0x800D2D24u + actor);
            if (BG_U8(GS(0x22B6) + BG_U8(GS(0x97F) + BG_U8(GS(0x30C) + t * 0xA4u) * 0xA4u)) == 0) {
                BG_U16(CMD_ROW(actor) + 0x20) = BG_U16(0x800C3238u);
            }
        }
    }
    if (BG_U8(ACT_ALT(actor)) == 0) {
        ctx = CMD_CONTEXT();
        if (BG_U16(ctx + actor * 0x40u + 0x2E) == 0) {
            BG_U8(ctx + 0x2DD) = 1;
            BG_U8(0x800C3E28u) = 0;
        } else {
            BG_U8(ctx + 0x2DD) = 4;
            BG_U8(0x800C3E28u) = 3;
        }
        if (BG_U16(0x800CCD64u + actor * 0x170u) & 2) {
            BG_U8(CMD_CONTEXT() + 0x2DD) = 2;
            BG_U8(0x800C3E28u) = 1;
        }
        func_8007FEC4();
    } else {
        ctx = CMD_CONTEXT();
        if (BG_U16(ctx + actor * 0x40u + 0x2E) == 0) {
            BG_U8(ctx + 0x2DD) = 0x10;
            BG_U8(0x800C3E28u) = 0;
        } else {
            BG_U8(ctx + 0x2DD) = 0x13;
            BG_U8(0x800C3E28u) = 3;
        }
        func_8007FF14(actor);
    }
    func_80085E78();
    func_8009AB00(actor);
    mask = func_80089C08(actor);
    row = CMD_ROW(actor);
    func_800BC404((mask | func_80089C08(BG_U8(row + 0x3C))) & 0xFFFF);
    row = CMD_ROW(actor);
    t = BG_U8(row + 0x3C);
    if (BG_U8(row + 0x3C) == 0xFF) {
        t = 0;
    }
    func_800BCD98(func_80089C08(t) & 0xFFFF);
    while (BG_U8(CMD_CONTEXT() + 0x2DE) == 0 && BG_U8(0x800C48EAu) == 0 &&
           BG_U8(CMD_CONTEXT() + 0x2DB) == 0) {
        if (BG_U8(ACT_ALT(actor)) == 0) {
            func_8007FBE0(BG_U8(CMD_CONTEXT() + 0x2D5), BG_U8(CMD_CONTEXT() + 0x2D4));
        }
        ctx = CMD_CONTEXT();
        if (BG_U8(ctx + 0x2E2) != BG_U8(ctx + 0x2DD)) {
            BG_U8(ctx + 0x2F6) = 0;
            ctx = CMD_CONTEXT();
            BG_U8(ctx + 0x2E2) = BG_U8(ctx + 0x2DD);
            BG_U8(ACTOR_BLOCK() + 0xCB) = 0;
            ctx = CMD_CONTEXT();
            /* The result is stored through a pointer retail loads after the
             * call. */
            ret = func_8008D598(actor, BG_U8(ctx + 0x2DD), BG_U8(ctx + 0x2E0));
            BG_U8(ACTOR_BLOCK() + 0xA3) = (u8)ret;
            BG_U8(ACTOR_BLOCK() + 0xCB) = 1;
        }
        if (BG_U8(0x800D3014u) == 0xFF) {
            break;
        }
        func_800716D8();
        ctx = CMD_CONTEXT();
        if (BG_U8(ctx + 0x2DB) != 0) {
            break;
        }
        if (BG_U8(ctx + 0x2E0) == 0) {
            BG_U8(0x800D366Cu) = 1;
        }
        switch (BG_U8(ctx + 0x2DD)) {
        case 1:
            func_8008115C(actor);
            break;
        case 2:
            func_80081318(actor);
            break;
        case 3:
            func_80081504(actor);
            break;
        case 4:
            func_800816F8(actor);
            break;
        case 0x64:
            BG_U8(CMD_CONTEXT() + 0x2E2) = 0xFF;
            /* fall through */
        case 5:
            func_80081B58(actor);
            break;
        case 7:
            func_800820A4(actor);
            break;
        case 8:
            func_800822C4(actor);
            break;
        case 9:
            func_80082504(actor);
            break;
        case 10:
            func_80082820(actor);
            break;
        case 0x10:
            func_800829F4(actor);
            break;
        case 0x11:
            func_80082BB0(actor);
            break;
        case 0x12:
            func_80082D4C(actor);
            break;
        case 0x13:
            func_80082F7C(actor);
            break;
        case 0x15:
            func_800830A8(actor);
            break;
        case 0x16:
            func_80083340(actor);
            break;
        case 0x17:
            func_80083580(actor);
            break;
        case 0x18:
            func_80083748(actor);
            break;
        case 0x65:
            BG_U8(CMD_CONTEXT() + 0x2E2) = 0xFF;
            /* fall through */
        case 0x19:
            func_80083948(actor);
            break;
        default:
            break;
        }
    }
    BG_U8(ACTOR_BLOCK() + 0xCB) = 0;
    BG_U8(ACTOR_BLOCK() + 0xAF) = 0;
    BG_U8(ACTOR_BLOCK() + actor + 0x90) = 0;
    BG_U8(ACTOR_BLOCK() + actor + 0x7F) = 0;
    if (BG_U8(CMD_CONTEXT() + 0x2E3) != 0) {
        ctx = CMD_CONTEXT();
        /* A u8 store, then a re-read of the stored byte for the clamp. */
        BG_U8(ACT_COUNT(actor)) = (u8)(BG_U8(ACT_COUNT(actor)) + BG_U8(ctx + 0x2D4));
        if (BG_U8(ACT_COUNT(actor)) >= 0x1D) {
            BG_U8(ACT_COUNT(actor)) = 0x1C;
        }
    }
    BG_U8(0x800D366Cu) = 0;
    func_800BAF40(actor, 0x100);
    if (BG_U8(CMD_CONTEXT() + 0x2DE) != 0 || BG_U8(0x800C48EAu) != 0) {
        func_8007FCE8();
        func_8007FDEC();
        func_800800E8(actor);
        func_8007FB70(actor);
    }
    BG_U16(0x800D2E06u + actor * 2u) = BG_U16(0x800D2E06u + actor * 2u - 0x16u);
    BG_U8(ACTOR_BLOCK() + actor + 0x7C) = 1;
    BG_U8(0x800C4928u) = 0;
}
#endif
