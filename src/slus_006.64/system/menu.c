#include "common.h"
#include "psyq/libgte.h"
#include "system/controller.h"
#include "system/menu.h"
#include "system/memory.h"

void MenuInitializeGfxEnvironment(GfxEnvironment* pGfxEnv) {
    pGfxEnv->drawEnv.dtd = 1;
    pGfxEnv->dispEnv.screen.y = 10;
    pGfxEnv->dispEnv.screen.w = 256;
    pGfxEnv->drawEnv.isbg = 0;
    pGfxEnv->drawEnv.r0 = 0;
    pGfxEnv->drawEnv.g0 = 0;
    pGfxEnv->drawEnv.b0 = 0;
    pGfxEnv->dispEnv.screen.x = 0;
    pGfxEnv->dispEnv.screen.h = 216;
}

void MenuInitializeGfxEnvironments(void) {
    SetGeomOffset(160, 112);
    SetGeomScreen(0x200);
    SetDefDispEnv(&g_Menu->gfxEnvs[0].dispEnv, 0, 0xE0, 0x140, 0xE0);
    SetDefDrawEnv(&g_Menu->gfxEnvs[0].drawEnv, 0, 0, 0x140, 0xE0);
    SetDefDispEnv(&g_Menu->gfxEnvs[1].dispEnv, 0, 0, 0x140, 0xE0);
    SetDefDrawEnv(&g_Menu->gfxEnvs[1].drawEnv, 0, 0xE0, 0x140, 0xE0);
    MenuInitializeGfxEnvironment(&g_Menu->gfxEnvs[0]);
    MenuInitializeGfxEnvironment(&g_Menu->gfxEnvs[1]);
}

void func_8001BEEC(void) {
    g_Menu->translation.vz = 0x800;
    g_Menu->unk228 = 0x800;
    g_Menu->rotation.vz = 0;
    g_Menu->rotation.vy = 0;
    g_Menu->rotation.vx = 0;
    g_Menu->translation.vy = 0;
    g_Menu->translation.vx = 0;
    g_Menu->unk21C = 0;
    g_Menu->unk21A = 0;
    g_Menu->unk218 = 0;
    g_Menu->unk224 = 0;
    g_Menu->unk220 = 0;
    g_Menu->unk2E8 = 1;
    g_Menu->transitionEffectState = 0;
}

/* Keep the retail debug-index reloads local to this input routine. */
void MenuProcessControllerInput(void) {
    u_char input = MENU_INPUT_IDLE;
    if (func_80036410() != 0) {
        ControllerResetState();
        g_Menu->input = input;
        return;
    }
    while (ControllerPopState()) {
        if (g_C1ButtonStatePressedOnce & CTRL_BTN_RIGHT) {
            input = MENU_INPUT_RIGHT;
            break;
        }
        if (g_C1ButtonStatePressedOnce & CTRL_BTN_DOWN) {
            input = MENU_INPUT_DOWN;
            break;
        }
        if (g_C1ButtonStatePressedOnce & CTRL_BTN_LEFT) {
            input = MENU_INPUT_LEFT;
            break;
        }
        if (g_C1ButtonStatePressedOnce & CTRL_BTN_UP) {
            input = MENU_INPUT_UP;
            break;
        }
        if (g_C1ButtonStatePressedOnce & CTRL_BTN_CIRCLE) {
            input = MENU_INPUT_CONFIRM;
            break;
        }
        if (g_C1ButtonStatePressedOnce & CTRL_BTN_SELECT) {
            input = 12;
            g_Menu->unk1E94 = g_Menu->unk1E94 == 0;
            break;
        }
        if (g_C1ButtonStatePressedOnce & CTRL_BTN_L1) {
            if ((*(volatile u8*)&g_Menu->unk1E95)) {
                (*(volatile u8*)&g_Menu->unk1E95) -= 1;
            }
            break;
        }
        if (g_C1ButtonStatePressedOnce & CTRL_BTN_L2) {
            (*(volatile u8*)&g_Menu->unk1E95) += 1;
            break;
        }
    }
    g_Menu->input = input;
}

extern s32* D_8005917C;

void func_8001C074(void) {
    GfxEnvironment* env;
    MenuProcessControllerInput();
    env = &g_Menu->gfxEnvs[0];
    if (g_Menu->pGfxEnv == env) {
        env = &g_Menu->gfxEnvs[1];
    }
    g_Menu->pGfxEnv = env;
    g_Menu->renderContext = g_Menu->renderContext == 0;
    ClearOTagR(g_Menu->pGfxEnv->ot, 0x10);
    if (*D_8005917C != -1) {
        if (g_Menu->unk1E94 != 0) {
            HeapDebugDump(3, g_Menu->unk1E95, 0xF, 0x80AC);
        }
        if (*D_8005917C != -1) {
            FontDrawLetters(g_Menu->pGfxEnv->ot);
        }
    }
    DrawSync(0);
    Vsync(0);
    PutDrawEnv(&g_Menu->pGfxEnv->drawEnv);
    PutDispEnv(&g_Menu->pGfxEnv->dispEnv);
    DrawOTag(&g_Menu->pGfxEnv->ot[15]);
}

extern char D_8001833C[];
extern char D_80018350[];
extern char D_80018364[];
extern char D_80018378[];
extern char D_80018390[];
extern void* D_8004FA9C[];
extern u8 D_80059460;
extern u8 D_80059171;
extern void* D_8005945C;
extern void* D_800658CC;
extern void* D_8006BE24;
extern void* D_8005A4AC;
extern void* D_8005A4B0;
extern GameState g_GameState;

extern void func_8001C074(void);
extern void func_801C62A8(void);
#ifdef XENO_PC_PORT
/* The retail executable names these overlay entry points by address.  In the
 * matching overlay TUs, the same functions have descriptive C names.  Route
 * the native all-in-one link to those real bodies instead of generating
 * address-named oracle stubs; the matching builds remain separate images. */
extern void MemberChangeMenuMain(void);
extern void ShopMenuMain(void);
#define func_801CB0A8 MemberChangeMenuMain
#define func_801CCD28 ShopMenuMain
#else
extern void func_801CB0A8(void);
extern void func_801CCD28(void);
#endif
extern void func_801CBDBC(void);
extern void func_801CE024(void);

void MenuExecute(void) {
    int i = 0;
    int j = 0;
    int running = 1;
    void* pBuf0;
    void* pBuf1;

    if (g_MenuDebugEnabled) {
        do {
            FontPrintf(&D_8001833C, i, D_8004FA9C[i]);

            if (i < 4) {
                if (j < 0xB) {
                    FontPrintf(&D_80018350, j);
                } else {
                    FontPrintf(&D_80018364, j - 0xB);
                }
            } else {
                if (i != 6) {
                    FontPrintf(&D_80018378, j);
                } else {
                    FontPrintf(&D_80018390, j);
                }
            }

            /* Retail jtbl_800183A4 maps input 4 -> exit, 0 -> next page,
             * 1 -> value down; bodies stay in retail listing order. */
            switch (g_Menu->input) {
                case 4:
                    running = 0;
                    break;
                case 0:
                    i++;
                    j = 0;
                    if (i >= 7) {
                        i = 0;
                    }
                    break;
                case 2:
                    i--;
                    j = 0;
                    if (i < 0) {
                        i = 6;
                    }
                    break;
                case 3:
                    if (i < 4) {
                        j++;
                        if (j >= 0x1F) {
                            j = 0;
                        }
                    } else {
                        if (i != 6) {
                            j++;
                        } else {
                            j = (j == 0);
                        }
                    }
                    break;
                case 1:
                    j--;
                    if (j < 0) {
                        if (i < 4) {
                            j = 0x1E;
                        } else {
                            if (i != 6) {
                                j = 0xFF;
                            } else {
                                j = (j == 0);
                            }
                        }
                    }
                    break;
            }

            func_8001C074();
        } while (running & 0xFF);

        D_80059460 = i;
        D_80059171 = j;
        *(u8*)((u8*)g_Menu + 0x84) = 0;
        *(u8*)((u8*)g_Menu + 0x138) = 0;
        SetDispMask(0);
        g_Menu->pGfxEnv = (GfxEnvironment*)((u8*)g_Menu + 0x120);
        SetDispMask(1);
    }

    ArchiveSetIndex(0x10, 0);

    if (g_MenuDebugEnabled) {
        g_GameState.gold = 0x3B9AC9FF;
        HeapChangeCurrentUser(0x2, NULL);
        D_8005945C = HeapAlloc(ArchiveDecodeAlignedSize(0x1), 0);
        ArchiveReadFileToBuffer(0x1, D_8005945C, 0, 0x80);
        ArchiveCdDataSync(0);

        if (D_80059460 == 5) {
            ArchiveSetIndex(0x4, 0);
            D_800658CC = HeapAlloc(0x4, 0x1);
#ifdef XENO_PC_PORT
            /* Retail reserves the top of the heap down to the object
             * overlay's address (0x801DC000) and loads archive 0x6B9 there.
             * The port runs that overlay natively (field_object_overlay.c)
             * and its top-of-heap already reaches below retail's overlay
             * addresses, so the reservation would be negative-sized and the
             * load would overwrite live heap blocks. Keep a block for the
             * HeapFree below; reserve and load nothing. */
            D_8006BE24 = HeapAlloc(0x4, 0x1);
#else
            D_8006BE24 = HeapAlloc((u32)D_800658CC + 0x7FE24000, 0x1);
            ArchiveReadFileToBuffer(0x6B9, (void*)0x801DC000, 0, 0x80);
#endif
            ArchiveCdDataSync(0);
            ArchiveSetIndex(0x10, 0);
            D_8005A4AC = HeapAlloc(0x4000, 0);
            D_8005A4B0 = HeapAlloc(0x4000, 0);
        }

        pBuf0 = HeapAlloc(0x4, 0x1);
#ifdef XENO_PC_PORT
        /* Same for the menu overlay (retail 0x801C5000): the port runs it
         * natively. On the KernelMenu route pBuf0 sits at 0x801BAA80, below
         * the overlay address, so the retail size would be 0xFFFF5A80 and
         * the 0x25908-byte load would clobber the top heap chain. */
        pBuf1 = HeapAlloc(0x4, 0x1);
#else
        pBuf1 = HeapAlloc((u32)pBuf0 + 0x7FE3B000, 0x1);
        ArchiveReadFileToBuffer(D_80059460 + 5, (void*)0x801C5000, 0, 0x80);
#endif
        ArchiveCdDataSync(0);
    }

    ArchiveSetIndex(0x10, 0);

#ifdef XENO_PC_PORT
    /* Dispatch matches retail jtbl_800183BC (asm/slus_006.64/data/800.rodata.s).
     * The matching-build C below follows the fall-through listing order in
     * MenuExecute.s and is left untouched for objdiff; that order swaps title
     * (2), save/name (3), and shop (4).  Slot 2 is the title field's FE57
     * opener (D_800ADB64=2): func_801C62A8 → func_801C58EC. */
    switch (D_80059460) {
        case 0:
            func_801C62A8();
            break;
        case 1:
            func_801CB0A8();
            break;
        case 2:
        case 6:
            func_801C62A8();
            ChangeGameState(1);
            break;
        case 3:
            func_801CBDBC();
            break;
        case 4:
            func_801CCD28();
            break;
        case 5:
            func_801CE024();
            break;
    }
#else
    switch (D_80059460) {
        case 0:
            func_801C62A8();
            break;
        case 1:
            func_801CB0A8();
            break;
        case 3:
            func_801CBDBC();
            break;
        case 4:
            func_801CCD28();
            break;
        case 2:
        case 6:
            func_801C62A8();
            ChangeGameState(1);
            break;
        case 5:
            func_801CE024();
            break;
    }
#endif

    if (g_MenuDebugEnabled) {
        HeapFree(pBuf0);
        HeapFree(pBuf1);
        if (D_80059460 == 5) {
            HeapFree(D_800658CC);
            HeapFree(D_8006BE24);
            HeapFree(D_8005A4AC);
            HeapFree(D_8005A4B0);
        }
        g_MenuDebugEnabled = 1;
        MainLoop(0);
    }
}

// Before calling this, it's expected that the caller have loaded the correct
// menu overlay to the correct address (0x801C5000).
void MenuMain() {
    g_Menu = HeapAlloc(sizeof(SystemMenu), 0);
    bzero(g_Menu, sizeof(SystemMenu));
    g_Menu->input = 8;
    HeapChangeCurrentUser(HEAP_USER_HIG, NULL);
    g_Menu->pGfxEnv = &g_Menu->gfxEnvs[1];
    g_Menu->unk1E94 = 0;
    g_Menu->unk1E95 = 1;
    g_Menu->unk2D8 = 0;
    g_Menu->shouldDrawMenu = FALSE;
    MenuInitializeGfxEnvironments();
    /* Retail keeps isbg=0 so MoveImage of the field snapshot (704,256) under
     * the menu OT remains visible. isbg=1 only when the debug KernelMenu path
     * is armed — otherwise title/system menus clear to black every frame. */
    if (g_MenuDebugEnabled) {
        g_Menu->gfxEnvs[0].drawEnv.isbg = 1;
        g_Menu->gfxEnvs[1].drawEnv.isbg = 1;
    }
    func_8001BEEC();
    Vsync(0);
    PutDrawEnv(&g_Menu->gfxEnvs[0].drawEnv);
    PutDrawEnv(&g_Menu->gfxEnvs[1].drawEnv);
    PutDispEnv(&g_Menu->gfxEnvs[0].dispEnv);
    PutDispEnv(&g_Menu->gfxEnvs[1].dispEnv);
    SetDispMask(1);

    // This call takes over control flow and runs the loaded menu until it returns
    MenuExecute();

    g_MenuDebugEnabled = 1;
}

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/menu", func_8001C76C);
#endif
