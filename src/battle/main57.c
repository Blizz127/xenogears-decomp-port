#include "common.h"


#ifndef XENO_PC_PORT
extern u8* D_800C34B0;
extern u8* D_800C3E00;
extern u8 g_GameState[];

/* func_8009AFD8.s, byte-exact: by the D_800C34B0+0x5FC2 mode, tick down
 * the g_GameState+0x2286 counter of D_800C3E00's +0x6F id (modes 0-3), of
 * its +0x72 id (mode 6), or of both (modes 7-19; that case falls through
 * into mode 6's).  The switch table is this object's .rodata (placed at
 * jtbl_80070428 by config/battle.yaml).  Retail decrements with
 * `addiu 0xFF`: gcc 2.6.x gives that for -= 1 (the BattleGcc263 preset),
 * while 2.7.2 and the CDK cc1 give `addiu -1`, as does -- under every cc1. */
void func_8009AFD8(void) {
    u32 id;

    switch (D_800C34B0[0x5FC2]) {
    case 0:
    case 1:
    case 2:
    case 3:
        id = D_800C3E00[0x6F];
        break;
    case 7:
    case 8:
    case 9:
    case 10:
    case 11:
    case 12:
    case 13:
    case 14:
    case 15:
    case 16:
    case 17:
    case 18:
    case 19:
        id = D_800C3E00[0x6F];
        if (g_GameState[0x2286 + id] != 0) {
            g_GameState[0x2286 + id] -= 1;
        }
        /* fall through */
    case 6:
        id = D_800C3E00[0x72];
        break;
    default:
        return;
    }
    if (g_GameState[0x2286 + id] != 0) {
        g_GameState[0x2286 + id] -= 1;
    }
}
#endif


#ifndef XENO_PC_PORT
extern u8* D_800C34B0;
#endif


/* func_8009B098.s */
void func_8009B098(void) {
    u8 i;

    for (i = 1; (u8)i < 3; i++) {
        u8* p = (u8*)(((u32)i) * 368 + (u32)(unsigned int)D_800C34B0);

        *(u16*)(p + 0x4C) = 0x64;
        *(u16*)(p + 0x4E) = 0x64;
        p[0x5E] = 0x14;
        p[0x5F] = 0xF;
        *(u16*)(p + 0x7A) = 0x1FBF;
        p[0x149] = 0;
    }
}


/* ---- Port bodies (fo/bat3).  The retail build compiles func_8009AFD8's
 * byte-exact C at the top of this file. ---- */
#ifndef XENO_PC_PORT
extern u8* D_800C34B0;
#endif
#ifndef XENO_PC_PORT
extern u8* D_800C3E00;
#endif
extern u8 g_GameState[];
#ifdef XENO_PC_PORT
static void bat3_dec_counter(u32 id) {
    u8 v = g_GameState[0x2286 + id];

    if (v != 0) {
        g_GameState[0x2286 + id] = v - 1;
    }
}

/* func_8009AFD8.s: by the D_800C34B0+0x5FC2 mode (jtbl_80070428), tick down
 * the g_GameState+0x2286 counter of D_800C3E00's +0x6F id (modes 0-3), of
 * its +0x72 id (mode 6), or of both (modes 7-19). */
void func_8009AFD8(void) {
    u32 mode = D_800C34B0[0x5FC2];

    if (mode >= 0x14) {
        return;
    }
    if (mode < 4) {
        bat3_dec_counter(D_800C3E00[0x6F]);
    } else if (mode == 6) {
        bat3_dec_counter(D_800C3E00[0x72]);
    } else if (mode >= 7) {
        bat3_dec_counter(D_800C3E00[0x6F]);
        bat3_dec_counter(D_800C3E00[0x72]);
    }
}
#endif /* XENO_PC_PORT */
